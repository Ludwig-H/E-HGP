#!/usr/bin/env python3
"""CLI orchestrator for reproducible, process-isolated 3-D benchmarks."""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import hashlib
import io
import json
import os
import subprocess
import sys
import time
from types import SimpleNamespace
from pathlib import Path
from typing import Any, Iterable

BENCHMARK_DIR = Path(__file__).resolve().parent
PROJECT_DIR = BENCHMARK_DIR.parent
REPOSITORY_DIR = PROJECT_DIR.parent
for candidate in reversed((str(BENCHMARK_DIR), str(PROJECT_DIR), str(REPOSITORY_DIR))):
    if candidate not in sys.path:
        sys.path.insert(0, candidate)

try:
    from .datasets import DATASET_NAMES, DatasetSpec, generate_dataset
    from .io_utils import (
        atomic_write_json,
        atomic_write_text,
        git_provenance,
        runtime_provenance,
    )
except ImportError:  # pragma: no cover - direct-script compatibility
    from datasets import DATASET_NAMES, DatasetSpec, generate_dataset
    from io_utils import (
        atomic_write_json,
        atomic_write_text,
        git_provenance,
        runtime_provenance,
    )


METHOD_ALIASES = {
    "powercover-cpu": "powercover_cpu",
    "powercover_cpu": "powercover_cpu",
    "cpu": "powercover_cpu",
    "powercover-cuda": "powercover_cuda",
    "powercover_cuda": "powercover_cuda",
    "cuda": "powercover_cuda",
    "gpu": "powercover_cuda",
    "hdbscan": "hdbscan",
    "hgp-old": "hgp_old",
    "hgp_old": "hgp_old",
    "hgp": "hgp_old",
}
ENTROPY_ALIASES = {
    "fixed-hard": "fixed_hard",
    "fixed_hard": "fixed_hard",
    "hard": "fixed_hard",
    "kappa1": "fixed_hard",
    "local-distortion": "local_distortion",
    "local_distortion": "local_distortion",
    "local": "local_distortion",
}

_SOURCE_FINGERPRINT_VERSION = "perg-hgp-benchmark-source-v1"
_SOURCE_ROOTS = (
    PROJECT_DIR / "perg_hgp",
    BENCHMARK_DIR,
    REPOSITORY_DIR / "HGP-old" / "src",
    REPOSITORY_DIR / "HGP-old" / "CGALDelaunay",
)
_SOURCE_FILES = (
    PROJECT_DIR / "__init__.py",
    PROJECT_DIR / "pyproject.toml",
)
_SOURCE_EXCLUDED_PARTS = {
    "__pycache__",
    ".pytest_cache",
    "results",
}
_SOURCE_SUFFIXES = {
    ".c",
    ".cc",
    ".cpp",
    ".cu",
    ".cuh",
    ".h",
    ".hpp",
    ".pxd",
    ".py",
    ".pyx",
    ".sh",
    ".so",
    ".toml",
}
_SOURCE_NAMES = {"CMakeLists.txt"}


def _utc_now() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat()


def _csv_values(raw: str | Iterable[str]) -> list[str]:
    values = [raw] if isinstance(raw, str) else list(raw)
    result: list[str] = []
    for value in values:
        result.extend(item.strip() for item in value.split(",") if item.strip())
    return result


def _integer_grid(raw: str, *, minimum: int = 1) -> list[int]:
    result: list[int] = []
    for token in _csv_values(raw):
        if "-" in token:
            left_text, right_text = token.split("-", 1)
            left, right = int(left_text), int(right_text)
            if right < left:
                raise argparse.ArgumentTypeError(
                    f"descending range is invalid: {token}"
                )
            result.extend(range(left, right + 1))
        else:
            result.append(int(token))
    if not result or any(value < minimum for value in result):
        raise argparse.ArgumentTypeError(f"all values must be >= {minimum}")
    return list(dict.fromkeys(result))


def _grid_shapes(raw: str) -> list[tuple[int, int, int]]:
    shapes: list[tuple[int, int, int]] = []
    for token in _csv_values(raw):
        pieces = token.lower().replace("×", "x").split("x")
        if len(pieces) == 1:
            size = int(pieces[0])
            shape = (size, size, size)
        elif len(pieces) == 3:
            shape = tuple(int(piece) for piece in pieces)
        else:
            raise argparse.ArgumentTypeError(f"grid must be N or NxNxN, got {token!r}")
        if any(value < 1 for value in shape):
            raise argparse.ArgumentTypeError("grid dimensions must be positive")
        shapes.append(shape)  # type: ignore[arg-type]
    return list(dict.fromkeys(shapes))


def _normalize_many(raw: str, aliases: dict[str, str], description: str) -> list[str]:
    result: list[str] = []
    for item in _csv_values(raw):
        normalized = aliases.get(item.lower())
        if normalized is None:
            raise ValueError(
                f"unknown {description} {item!r}; choose from {sorted(aliases)}"
            )
        if normalized not in result:
            result.append(normalized)
    return result


def _job_hash(identity: dict[str, Any]) -> str:
    payload = json.dumps(identity, sort_keys=True, separators=(",", ":")).encode(
        "utf-8"
    )
    return hashlib.sha256(payload).hexdigest()[:16]


def _source_fingerprint(
    roots: Iterable[Path] | None = None,
    files: Iterable[Path] | None = None,
) -> str:
    """Hash source bytes so resume cannot cross an implementation change.

    This deliberately does not rely on ``git``: committed, staged, unstaged,
    and untracked source edits all affect the identity.  Generated caches and
    benchmark results are excluded because they are execution artifacts rather
    than implementation inputs.
    """

    selected: set[Path] = set()
    for root in _SOURCE_ROOTS if roots is None else tuple(roots):
        candidate = Path(root)
        if not candidate.is_dir():
            continue
        for path in candidate.rglob("*"):
            if not path.is_file() or any(
                part in _SOURCE_EXCLUDED_PARTS for part in path.parts
            ):
                continue
            if path.suffix not in _SOURCE_SUFFIXES and path.name not in _SOURCE_NAMES:
                continue
            selected.add(path)
    for path in _SOURCE_FILES if files is None else tuple(files):
        candidate = Path(path)
        if candidate.is_file():
            selected.add(candidate)
    if not selected:
        raise RuntimeError("no source files found for benchmark fingerprint")

    digest = hashlib.sha256()
    digest.update(_SOURCE_FINGERPRINT_VERSION.encode("utf-8") + b"\0")
    for path in sorted(selected, key=lambda item: item.as_posix()):
        try:
            label = path.relative_to(REPOSITORY_DIR).as_posix()
        except ValueError:
            label = path.resolve().as_posix()
        digest.update(label.encode("utf-8") + b"\0")
        size = path.stat().st_size
        digest.update(str(size).encode("ascii") + b"\0")
        with open(path, "rb") as stream:
            for chunk in iter(lambda: stream.read(1 << 20), b""):
                digest.update(chunk)
        digest.update(b"\0")
    return f"sha256:{digest.hexdigest()}"


def _method_size_limit(method: str, args: argparse.Namespace) -> int:
    return {
        "powercover_cpu": args.max_n_powercover_cpu,
        "powercover_cuda": args.max_n_powercover_cuda,
        "hdbscan": args.max_n_hdbscan,
        "hgp_old": args.max_n_hgp_old,
    }[method]


def _base_job(
    *,
    args: argparse.Namespace,
    method: str,
    dataset_manifest: Path,
    dataset_identity: dict[str, Any],
    K: int,
    output: Path,
    source_fingerprint: str,
) -> dict[str, Any]:
    labels_path = output / "labels" / "placeholder.npy"
    job: dict[str, Any] = {
        "schema": "perg_hgp.benchmark_job",
        "schema_version": 1,
        "method": method,
        "K": K,
        "source_fingerprint": source_fingerprint,
        "dataset_metadata_path": str(dataset_manifest.resolve()),
        "dataset_identity": dataset_identity,
        "min_cluster_size": args.min_cluster_size,
        "resource_interval_seconds": args.resource_interval,
        "labels_path": str(labels_path.resolve()) if args.save_labels else None,
        "n_jobs": args.n_jobs,
        "allow_single_cluster": args.allow_single_cluster,
        "max_n_cut_scan": args.max_n_cut_scan,
        "cut_count": args.cut_count,
        "max_candidate_cells_per_site": args.max_candidate_cells_per_site,
        "max_active_cells": args.max_active_cells,
        "max_total_cells": args.max_total_cells,
        "save_hierarchy": args.save_hierarchy,
        "hgp_old_root": str(args.hgp_old_root.resolve()),
        "hgp_backend": args.hgp_backend,
        "hgp_method": args.hgp_method,
        "hgp_min_samples": args.hgp_min_samples,
        "hgp_exp_z": args.hgp_exp_z,
        "cgal_root": str(args.cgal_root.resolve()),
        "verbose_algorithm": args.verbose_algorithm,
    }
    return job


def _build_jobs(
    args: argparse.Namespace,
    artifacts: list[Any],
    output: Path,
    *,
    source_fingerprint: str,
) -> list[dict[str, Any]]:
    methods = _normalize_many(args.methods, METHOD_ALIASES, "method")
    entropy_modes = _normalize_many(args.entropy, ENTROPY_ALIASES, "entropy mode")
    K_values = _integer_grid(args.k)
    grid_shapes = _grid_shapes(args.grid)
    jobs: list[dict[str, Any]] = []
    for artifact in artifacts:
        identity = artifact.metadata["identity"]
        n = int(identity["n"])
        for method in methods:
            for K in K_values:
                if K > n:
                    continue
                combinations: list[tuple[str | None, tuple[int, int, int] | None]]
                if method.startswith("powercover_"):
                    combinations = [
                        (entropy, grid)
                        for entropy in entropy_modes
                        for grid in grid_shapes
                    ]
                else:
                    combinations = [(None, None)]
                for entropy, grid in combinations:
                    job = _base_job(
                        args=args,
                        method=method,
                        dataset_manifest=artifact.metadata_path,
                        dataset_identity=identity,
                        K=K,
                        output=output,
                        source_fingerprint=source_fingerprint,
                    )
                    if method.startswith("powercover_"):
                        assert entropy is not None and grid is not None
                        job.update(
                            powercover_backend=(
                                "cpu" if method == "powercover_cpu" else "cuda"
                            ),
                            entropy_mode=entropy,
                            hard_kappa=args.hard_kappa,
                            distortion_budget_absolute=args.distortion_budget_absolute,
                            distortion_budget_relative=args.distortion_budget_relative,
                            local_scale_k=args.local_scale_k,
                            powercover_config={
                                "m_reg": args.m_reg,
                                "grid_shape": list(grid),
                                "chunk_size": args.chunk_size,
                                "power_chunk_size": args.power_chunk_size,
                                "candidate_k_initial": args.candidate_k_initial,
                                "candidate_k_max": args.candidate_k_max,
                                "strict_certification": args.strict_certification,
                                "neighbor_audit_queries": args.neighbor_audit_queries,
                                "require_neighbor_audit": args.require_neighbor_audit,
                                "numerical_margin_factor": args.numerical_margin_factor,
                                "max_vram_fraction": args.max_vram_fraction,
                                "min_resolved_radius": args.min_resolved_radius,
                                "max_relative_spatial_error": args.max_relative_spatial_error,
                                "max_grid_cells": args.max_grid_cells,
                            },
                        )
                    identity_for_hash = {
                        key: value
                        for key, value in job.items()
                        if key
                        not in {
                            "dataset_metadata_path",
                            "labels_path",
                            "hgp_old_root",
                            "cgal_root",
                        }
                    }
                    job_id = _job_hash(identity_for_hash)
                    descriptor = (
                        f"{identity['name']}-n{n}-s{identity['seed']}-" f"{method}-k{K}"
                    )
                    if entropy is not None and grid is not None:
                        descriptor += f"-{entropy}-g{grid[0]}x{grid[1]}x{grid[2]}"
                    safe_descriptor = descriptor.replace("_", "-")
                    stem = f"{safe_descriptor}-{job_id}"
                    job.update(
                        job_id=job_id,
                        output_path=str(
                            (output / "results" / f"{stem}.json").resolve()
                        ),
                        labels_path=(
                            str((output / "labels" / f"{stem}.labels.npy").resolve())
                            if args.save_labels
                            else None
                        ),
                        hierarchy_path=str((output / "hierarchies" / stem).resolve()),
                        log_path=str((output / "logs" / f"{stem}.log").resolve()),
                        size_limit=_method_size_limit(method, args),
                    )
                    jobs.append(job)
    return jobs


def _summary_row(result: dict[str, Any]) -> dict[str, Any]:
    job = result.get("job", {})
    dataset = result.get("dataset", {}).get("identity", job.get("dataset_identity", {}))
    resources = result.get("resources", {})
    method_result = result.get("method_result", {})
    method = result.get("method", job.get("method"))
    row: dict[str, Any] = {
        "job_id": result.get("job_id"),
        "status": result.get("status"),
        "method": method,
        "dataset": dataset.get("name"),
        "n": dataset.get("n"),
        "seed": dataset.get("seed"),
        "K": result.get("K", job.get("K")),
        "entropy_mode": job.get("entropy_mode"),
        "grid_shape": job.get("powercover_config", {}).get("grid_shape"),
        "fit_wall_seconds": None,
        "points_per_second": None,
        "ARI": None,
        "oracle_best_ARI": None,
        "sampled_oracle_best_ARI": None,
        "rss_peak_bytes": resources.get("rss_peak_sampled_bytes"),
        "vram_peak_bytes": resources.get("process_vram_peak_bytes"),
        "error": result.get("error"),
    }
    if method in {"powercover_cpu", "powercover_cuda"}:
        row["fit_wall_seconds"] = method_result.get("fit_wall_seconds")
        cut_scan = method_result.get("cut_scan", {})
        sampled_ari = cut_scan.get(
            "sampled_oracle_best_ARI", cut_scan.get("oracle_best_ARI")
        )
        row["sampled_oracle_best_ARI"] = sampled_ari
        # Schema-v1 compatibility alias for existing summary consumers.
        row["oracle_best_ARI"] = sampled_ari
        row["total_interleaving_radius"] = method_result.get("accuracy_report", {}).get(
            "total_interleaving_radius"
        )
    else:
        row["fit_wall_seconds"] = method_result.get("fit_and_label_wall_seconds")
        row["ARI"] = method_result.get("metrics", {}).get("ARI")
    if (
        isinstance(row.get("fit_wall_seconds"), (int, float))
        and row["fit_wall_seconds"] > 0.0
        and isinstance(row.get("n"), int)
    ):
        row["points_per_second"] = row["n"] / row["fit_wall_seconds"]
    return row


def _write_summaries(output: Path, result_paths: Iterable[Path]) -> dict[str, Any]:
    results: list[dict[str, Any]] = []
    for path in result_paths:
        if not path.exists():
            continue
        try:
            with open(path, "r", encoding="utf-8") as stream:
                results.append(json.load(stream))
        except (OSError, json.JSONDecodeError):
            continue
    rows = [_summary_row(result) for result in results]
    summary = {
        "schema": "perg_hgp.benchmark_summary",
        "schema_version": 1,
        "updated_at": _utc_now(),
        "result_count": len(results),
        "status_counts": {
            status: sum(result.get("status") == status for result in results)
            for status in sorted({str(result.get("status")) for result in results})
        },
        "rows": rows,
    }
    atomic_write_json(output / "summary.json", summary)
    fieldnames = [
        "job_id",
        "status",
        "method",
        "dataset",
        "n",
        "seed",
        "K",
        "entropy_mode",
        "grid_shape",
        "fit_wall_seconds",
        "points_per_second",
        "ARI",
        "sampled_oracle_best_ARI",
        "oracle_best_ARI",
        "total_interleaving_radius",
        "rss_peak_bytes",
        "vram_peak_bytes",
        "error",
    ]
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=fieldnames, extrasaction="ignore")
    writer.writeheader()
    writer.writerows(rows)
    atomic_write_text(output / "summary.csv", stream.getvalue())
    return summary


def _write_skipped(job: dict[str, Any], reason: str) -> None:
    result = {
        "schema": "perg_hgp.benchmark_result",
        "schema_version": 1,
        "job_id": job["job_id"],
        "method": job["method"],
        "K": job["K"],
        "status": "skipped",
        "reason": reason,
        "job": job,
        "started_at": _utc_now(),
        "finished_at": _utc_now(),
    }
    atomic_write_json(job["output_path"], result)


def _resume_result_matches_job(path: Path, job: dict[str, Any]) -> bool:
    """Require exact embedded job identity before reusing an existing result."""

    try:
        with open(path, "r", encoding="utf-8") as stream:
            result = json.load(stream)
    except (OSError, json.JSONDecodeError):
        return False
    if not isinstance(result, dict) or not isinstance(result.get("job"), dict):
        return False
    stored_job = result["job"]
    return (
        result.get("job_id") == job.get("job_id")
        and stored_job == job
        and stored_job.get("source_fingerprint") == job.get("source_fingerprint")
    )


def _execute_jobs(
    args: argparse.Namespace, jobs: list[dict[str, Any]], output: Path
) -> int:
    output_paths = [Path(job["output_path"]) for job in jobs]
    failures = 0
    for index, job in enumerate(jobs, start=1):
        result_path = Path(job["output_path"])
        if (
            args.resume
            and result_path.exists()
            and _resume_result_matches_job(result_path, job)
        ):
            print(f"[{index}/{len(jobs)}] reuse {job['job_id']} ({job['method']})")
            continue
        n = int(job["dataset_identity"]["n"])
        limit = int(job["size_limit"])
        if limit > 0 and n > limit:
            reason = f"n={n} exceeds configured {job['method']} limit {limit}"
            print(f"[{index}/{len(jobs)}] skip {job['job_id']}: {reason}")
            _write_skipped(job, reason)
            _write_summaries(output, output_paths)
            continue
        job_path = output / "jobs" / f"{job['job_id']}.json"
        atomic_write_json(job_path, job)
        print(
            f"[{index}/{len(jobs)}] run {job['method']} "
            f"{job['dataset_identity']['name']} n={n} K={job['K']} "
            f"job={job['job_id']} log={job['log_path']}",
            flush=True,
        )
        if args.dry_run:
            continue
        Path(job["log_path"]).parent.mkdir(parents=True, exist_ok=True)
        command = [
            args.python,
            str(BENCHMARK_DIR / "worker.py"),
            "--job",
            str(job_path),
        ]
        started = time.perf_counter()
        try:
            with open(job["log_path"], "w", encoding="utf-8") as log:
                completed = subprocess.run(
                    command,
                    cwd=REPOSITORY_DIR,
                    stdout=log,
                    stderr=subprocess.STDOUT,
                    timeout=None if args.timeout <= 0 else args.timeout,
                    check=False,
                    env=os.environ.copy(),
                )
            if completed.returncode != 0:
                failures += 1
                if not result_path.exists():
                    atomic_write_json(
                        result_path,
                        {
                            "schema": "perg_hgp.benchmark_result",
                            "schema_version": 1,
                            "job_id": job["job_id"],
                            "method": job["method"],
                            "K": job["K"],
                            "status": "worker_crash",
                            "return_code": completed.returncode,
                            "job": job,
                            "log_path": job["log_path"],
                            "worker_wall_seconds": time.perf_counter() - started,
                        },
                    )
        except subprocess.TimeoutExpired:
            failures += 1
            atomic_write_json(
                result_path,
                {
                    "schema": "perg_hgp.benchmark_result",
                    "schema_version": 1,
                    "job_id": job["job_id"],
                    "method": job["method"],
                    "K": job["K"],
                    "status": "timeout",
                    "timeout_seconds": args.timeout,
                    "job": job,
                    "log_path": job["log_path"],
                    "worker_wall_seconds": time.perf_counter() - started,
                },
            )
        _write_summaries(output, output_paths)
        print(
            f"[{index}/{len(jobs)}] finished {job['job_id']} in "
            f"{time.perf_counter() - started:.3f}s; log={job['log_path']}",
            flush=True,
        )
        if failures and not args.keep_going:
            break
    if not args.dry_run:
        _write_summaries(output, output_paths)
    return 0 if failures == 0 or args.allow_errors else 1


def command_generate(args: argparse.Namespace) -> int:
    specs = [
        DatasetSpec(dataset, n, args.seed, args.dataset_chunk_size)
        for dataset in _csv_values(args.datasets)
        for n in _integer_grid(args.n)
    ]
    for spec in specs:
        artifact = generate_dataset(spec, args.output, force=args.force)
        print(artifact.metadata_path)
    return 0


def command_run(args: argparse.Namespace) -> int:
    try:
        datasets = _csv_values(args.datasets)
        unknown = sorted(set(datasets) - set(DATASET_NAMES) - {"lidar_like"})
        if unknown:
            raise ValueError(f"unknown datasets: {unknown}")
        _normalize_many(args.methods, METHOD_ALIASES, "method")
        _normalize_many(args.entropy, ENTROPY_ALIASES, "entropy mode")
        _integer_grid(args.k)
        _grid_shapes(args.grid)
        if args.repeats < 1:
            raise ValueError("repeats must be positive")
    except (TypeError, ValueError) as error:
        raise SystemExit(str(error)) from error

    if args.output is None:
        stamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        output = BENCHMARK_DIR / "results" / stamp
    else:
        output = args.output
    output = output.resolve()
    for child in ("datasets", "jobs", "results", "labels", "logs", "hierarchies"):
        (output / child).mkdir(parents=True, exist_ok=True)

    artifacts = []
    for repeat in range(args.repeats):
        seed = args.seed + repeat
        for dataset in datasets:
            for n in _integer_grid(args.n):
                spec = DatasetSpec(dataset, n, seed, args.dataset_chunk_size)
                print(f"prepare dataset {spec.name} n={n} seed={seed}", flush=True)
                if args.dry_run:
                    artifacts.append(
                        SimpleNamespace(
                            metadata={"identity": spec.identity_dict()},
                            metadata_path=(
                                output
                                / "datasets"
                                / f"dry-{spec.name}-{spec.fingerprint[:12]}.json"
                            ),
                        )
                    )
                else:
                    artifacts.append(
                        generate_dataset(
                            spec, output / "datasets", force=args.force_datasets
                        )
                    )
    source_fingerprint = _source_fingerprint()
    jobs = _build_jobs(
        args,
        artifacts,
        output,
        source_fingerprint=source_fingerprint,
    )
    manifest = {
        "schema": "perg_hgp.benchmark_session",
        "schema_version": 1,
        "created_at": _utc_now(),
        "output": str(output),
        "argv": sys.argv,
        "runtime": runtime_provenance(),
        "git": git_provenance(REPOSITORY_DIR),
        "source_fingerprint": source_fingerprint,
        "dataset_manifests": [str(item.metadata_path) for item in artifacts],
        "job_count": len(jobs),
        "jobs": jobs,
        "oracle_policy": (
            "PowerCover cut ARI is a truth-guided diagnostic only and is never "
            "a production selection rule"
        ),
    }
    atomic_write_json(output / "session_manifest.json", manifest)
    print(f"session={output} jobs={len(jobs)}", flush=True)
    return _execute_jobs(args, jobs, output)


def command_summarize(args: argparse.Namespace) -> int:
    output = args.directory.resolve()
    result_paths = sorted((output / "results").glob("*.json"))
    summary = _write_summaries(output, result_paths)
    print(json.dumps(summary["status_counts"], sort_keys=True))
    return 0


def _add_shared_dataset_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--datasets",
        default="anisotropic_blobs",
        help=f"comma-separated datasets: {', '.join(DATASET_NAMES)}",
    )
    parser.add_argument(
        "--n",
        default="10000",
        help="point counts, comma/range syntax (e.g. 10000,100000)",
    )
    parser.add_argument("--seed", type=int, default=20260712)
    parser.add_argument(
        "--dataset-chunk-size",
        type=int,
        default=1_000_000,
        help="maximum rows generated at once; values are chunk-size invariant",
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="perg-hgp-bench3d",
        description=(
            "Reproducible 3-D PowerCover3D benchmarks. Each clustering method "
            "runs in a fresh process; datasets are deterministic float32 .npy memmaps."
        ),
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        epilog=(
            "Example: python -m benchmarks run --methods powercover-cuda,hdbscan "
            "--datasets mixture,two_density_corridor --n 100000 --k 1-10 "
            "--entropy fixed-hard,local-distortion --grid 64,96"
        ),
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    generate = subparsers.add_parser(
        "generate",
        help="generate/reuse deterministic .npy datasets only",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    _add_shared_dataset_arguments(generate)
    generate.add_argument("--output", type=Path, required=True)
    generate.add_argument("--force", action="store_true")
    generate.set_defaults(function=command_generate)

    run = subparsers.add_parser(
        "run",
        help="build a matrix and execute every method in a separate process",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    _add_shared_dataset_arguments(run)
    run.add_argument("--output", type=Path, default=None)
    run.add_argument("--repeats", type=int, default=1, help="increment seed per repeat")
    run.add_argument(
        "--methods",
        default="powercover-cpu,hdbscan,hgp-old",
        help="powercover-cpu,powercover-cuda,hdbscan,hgp-old",
    )
    run.add_argument("--k", default="1-10", help="K values, comma/range syntax")
    run.add_argument(
        "--entropy",
        default="fixed-hard,local-distortion",
        help="PowerCover modes: fixed-hard (kappa=1), local-distortion",
    )
    run.add_argument("--grid", default="64", help="PowerCover grids: N or NxNxN")
    run.add_argument("--min-cluster-size", type=int, default=100)
    run.add_argument("--n-jobs", type=int, default=-1)
    run.add_argument("--allow-single-cluster", action="store_true")

    power = run.add_argument_group("PowerCover3D")
    power.add_argument("--hard-kappa", type=float, default=1.0)
    power.add_argument("--distortion-budget-relative", type=float, default=0.10)
    power.add_argument("--distortion-budget-absolute", type=float, default=None)
    power.add_argument("--local-scale-k", type=int, default=None)
    power.add_argument("--m-reg", type=int, default=64)
    power.add_argument("--chunk-size", type=int, default=262_144)
    power.add_argument("--power-chunk-size", type=int, default=65_536)
    power.add_argument("--candidate-k-initial", type=int, default=16)
    power.add_argument("--candidate-k-max", type=int, default=1_023)
    power.add_argument(
        "--strict-certification", action=argparse.BooleanOptionalAction, default=True
    )
    power.add_argument("--neighbor-audit-queries", type=int, default=32)
    power.add_argument(
        "--require-neighbor-audit", action=argparse.BooleanOptionalAction, default=True
    )
    power.add_argument("--numerical-margin-factor", type=float, default=64.0)
    power.add_argument("--max-vram-fraction", type=float, default=0.85)
    power.add_argument("--min-resolved-radius", type=float, default=None)
    power.add_argument("--max-relative-spatial-error", type=float, default=0.25)
    power.add_argument("--max-grid-cells", type=int, default=50_000_000)
    power.add_argument("--save-hierarchy", action="store_true")

    cuts = run.add_argument_group("small-N truth-oracle cut diagnostics")
    cuts.add_argument(
        "--max-n-cut-scan",
        type=int,
        default=100_000,
        help="0 disables labels_at_cut; never use this path for 30M",
    )
    cuts.add_argument("--cut-count", type=int, default=15)
    cuts.add_argument("--max-candidate-cells-per-site", type=int, default=100_000)
    cuts.add_argument("--max-active-cells", type=int, default=5_000_000)
    cuts.add_argument("--max-total-cells", type=int, default=5_000_000)

    hgp = run.add_argument_group("HGP-old")
    hgp.add_argument("--hgp-old-root", type=Path, default=REPOSITORY_DIR / "HGP-old")
    hgp.add_argument("--hgp-backend", default="cgal")
    hgp.add_argument("--hgp-method", default="eom")
    hgp.add_argument("--hgp-min-samples", type=int, default=0)
    hgp.add_argument("--hgp-exp-z", type=float, default=2.0)
    hgp.add_argument(
        "--cgal-root", type=Path, default=REPOSITORY_DIR / "HGP-old" / "CGALDelaunay"
    )

    limits = run.add_argument_group("safety limits (0 means unlimited)")
    limits.add_argument("--max-n-powercover-cpu", type=int, default=100_000)
    limits.add_argument("--max-n-powercover-cuda", type=int, default=0)
    limits.add_argument("--max-n-hdbscan", type=int, default=2_000_000)
    limits.add_argument("--max-n-hgp-old", type=int, default=100_000)

    execution = run.add_argument_group("execution and artifacts")
    execution.add_argument("--python", default=sys.executable)
    execution.add_argument("--timeout", type=float, default=0.0, help="per-job seconds")
    execution.add_argument("--resource-interval", type=float, default=0.05)
    execution.add_argument(
        "--save-labels", action=argparse.BooleanOptionalAction, default=True
    )
    execution.add_argument(
        "--resume", action=argparse.BooleanOptionalAction, default=True
    )
    execution.add_argument("--force-datasets", action="store_true")
    execution.add_argument("--dry-run", action="store_true")
    execution.add_argument(
        "--keep-going", action=argparse.BooleanOptionalAction, default=True
    )
    execution.add_argument("--allow-errors", action="store_true")
    execution.add_argument("--verbose-algorithm", action="store_true")
    run.set_defaults(function=command_run)

    summarize = subparsers.add_parser(
        "summarize", help="rebuild summary.json and summary.csv from result JSON"
    )
    summarize.add_argument("directory", type=Path)
    summarize.set_defaults(function=command_summarize)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    return int(args.function(args))


if __name__ == "__main__":
    raise SystemExit(main())
