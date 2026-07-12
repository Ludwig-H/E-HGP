#!/usr/bin/env python3
"""Execute exactly one benchmark job in an isolated Python process."""

from __future__ import annotations

import argparse
import datetime as dt
import importlib.metadata
import inspect
import json
import os
import sys
import time
import traceback
from pathlib import Path
from typing import Any, Callable

import numpy as np

# Direct execution is the orchestrator's default.  Put both the benchmark
# directory and project directory first, without requiring package install.
BENCHMARK_DIR = Path(__file__).resolve().parent
PROJECT_DIR = BENCHMARK_DIR.parent
REPOSITORY_DIR = PROJECT_DIR.parent
for candidate in reversed((str(BENCHMARK_DIR), str(PROJECT_DIR), str(REPOSITORY_DIR))):
    if candidate not in sys.path:
        sys.path.insert(0, candidate)

try:
    from .datasets import load_dataset
    from .io_utils import (
        atomic_save_npy,
        atomic_write_json,
        git_provenance,
        runtime_provenance,
        sha256_file,
    )
    from .metrics import clustering_scores, label_summary
    from .resources import ResourceMonitor
except ImportError:  # pragma: no cover - exercised by direct script execution
    from datasets import load_dataset
    from io_utils import (
        atomic_save_npy,
        atomic_write_json,
        git_provenance,
        runtime_provenance,
        sha256_file,
    )
    from metrics import clustering_scores, label_summary
    from resources import ResourceMonitor


class MethodUnavailable(RuntimeError):
    """An optional comparison backend is not installed/importable."""


def _utc_now() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat()


def _software_versions() -> dict[str, str | None]:
    distributions = (
        "numpy",
        "scipy",
        "scikit-learn",
        "hdbscan",
        "torch",
        "cupy-cuda12x",
        "cuvs-cu12",
        "cuml-cu12",
        "nvidia-ml-py",
        "psutil",
        "perg-hgp",
    )
    versions: dict[str, str | None] = {}
    for distribution in distributions:
        try:
            versions[distribution] = importlib.metadata.version(distribution)
        except importlib.metadata.PackageNotFoundError:
            versions[distribution] = None
    return versions


class ProgressRecorder:
    """Keep compact progress evidence and expose stage boundaries in logs."""

    def __init__(self) -> None:
        self.stages: dict[str, dict[str, Any]] = {}

    def __call__(self, event: dict[str, Any]) -> None:
        stage = str(event.get("stage", "unknown"))
        kind = str(event.get("kind", "unknown"))
        record = self.stages.setdefault(
            stage,
            {
                "event_count": 0,
                "progress_event_count": 0,
                "start": None,
                "done": None,
                "last_completed": None,
                "last_total": None,
                "last_details": None,
            },
        )
        record["event_count"] += 1
        if kind == "progress":
            record["progress_event_count"] += 1
        if kind in {"start", "done"}:
            record[kind] = event.get("timestamp") or _utc_now()
            print(
                json.dumps(
                    {
                        "benchmark_progress": {
                            "stage": stage,
                            "kind": kind,
                            "completed": event.get("completed"),
                            "total": event.get("total"),
                        }
                    },
                    sort_keys=True,
                ),
                flush=True,
            )
        record["last_completed"] = event.get("completed")
        record["last_total"] = event.get("total")
        record["last_details"] = event.get("details")


def _synchronize_cuda() -> None:
    """Synchronize whichever CUDA Python runtime is already importable."""

    try:
        import cupy as cp

        cp.cuda.get_current_stream().synchronize()
        return
    except (ImportError, RuntimeError):
        pass
    try:
        import torch

        if torch.cuda.is_available():
            torch.cuda.synchronize()
    except (ImportError, RuntimeError):
        pass


def _candidate_cuts(hierarchy: Any, count: int) -> np.ndarray:
    values: list[np.ndarray] = []
    for forest_name in ("base_forest", "inner_forest", "outer_forest"):
        forest = getattr(hierarchy, forest_name, None)
        for value_name in ("births", "weights"):
            raw_values = getattr(forest, value_name, None)
            if raw_values is None:
                continue
            array = np.asarray(raw_values, dtype=np.float64)
            array = array[np.isfinite(array) & (array >= 0.0)]
            if array.size:
                values.append(array)
    if not values:
        return np.empty(0, dtype=np.float64)
    merged = np.concatenate(values)
    quantiles = np.linspace(0.01, 0.99, max(2, int(count)), dtype=np.float64)
    cuts = np.quantile(merged, quantiles)
    cuts = np.concatenate(([float(merged.min())], cuts, [float(merged.max())]))
    return np.unique(cuts)


def _save_labels(labels: np.ndarray, path: str | None) -> dict[str, Any] | None:
    if path is None:
        return None
    destination = atomic_save_npy(path, np.asarray(labels, dtype=np.int32))
    return {
        "path": str(destination),
        "dtype": "int32",
        "shape": [int(labels.shape[0])],
        "bytes": destination.stat().st_size,
        "sha256": sha256_file(destination),
    }


def _scan_powercover_cuts(
    hierarchy: Any,
    truth: np.ndarray | None,
    job: dict[str, Any],
) -> dict[str, Any]:
    n = int(hierarchy.sites.centers.shape[0])
    maximum_n = int(job.get("max_n_cut_scan", 0))
    base = {
        "enabled": maximum_n > 0,
        "max_n_cut_scan": maximum_n,
        "selection_kind": "truth_oracle_diagnostic_only",
        "production_selection": False,
        "warning": (
            "sampled_oracle_best_ARI uses ground truth over sampled cuts and "
            "must not be reported as an unsupervised production score"
        ),
        "sampled_oracle_best_ARI": None,
        # Compatibility alias retained in schema v1. New readers should use
        # sampled_oracle_best_ARI, whose name exposes the finite cut sample.
        "oracle_best_ARI": None,
        "compatibility_aliases": {"oracle_best_ARI": "sampled_oracle_best_ARI"},
        "oracle_best_cut": None,
        "oracle_best_labels_artifact": None,
    }
    if maximum_n <= 0:
        return {**base, "status": "disabled"}
    if n > maximum_n:
        return {
            **base,
            "status": "skipped_size_limit",
            "n": n,
            "reason": f"n={n} exceeds max_n_cut_scan={maximum_n}",
        }
    if truth is None:
        return {
            **base,
            "status": "skipped_no_ground_truth",
            "reason": "oracle cut scanning requires reference labels",
        }
    labels_at_cut = getattr(hierarchy, "labels_at_cut", None)
    if not callable(labels_at_cut):
        return {**base, "status": "unavailable_no_labels_at_cut"}
    cuts = _candidate_cuts(hierarchy, int(job.get("cut_count", 15)))
    if cuts.size == 0:
        return {**base, "status": "unavailable_no_finite_cut_values"}

    records: list[dict[str, Any]] = []
    best_ari = -np.inf
    best_cut: float | None = None
    best_labels: np.ndarray | None = None
    scan_started = time.perf_counter()
    for radius in cuts:
        assignment_started = time.perf_counter()
        try:
            labels = labels_at_cut(
                float(radius),
                min_cluster_size=int(job["min_cluster_size"]),
                max_sites=maximum_n,
                max_candidate_cells_per_site=int(
                    job.get("max_candidate_cells_per_site", 100_000)
                ),
                max_active_cells=int(job.get("max_active_cells", 5_000_000)),
                max_total_cells=int(job.get("max_total_cells", 5_000_000)),
            )
            assignment_seconds = time.perf_counter() - assignment_started
            labels = np.asarray(labels, dtype=np.int32)
            scores = clustering_scores(truth, labels)
            record = {
                "radius": float(radius),
                "status": "ok",
                "assignment_seconds": assignment_seconds,
                "metrics": scores,
                "labels": label_summary(labels),
            }
            ari = scores.get("ARI")
            if ari is not None and float(ari) > best_ari:
                best_ari = float(ari)
                best_cut = float(radius)
                best_labels = labels.copy()
        except Exception as error:
            record = {
                "radius": float(radius),
                "status": "error",
                "assignment_seconds": time.perf_counter() - assignment_started,
                "error_type": type(error).__name__,
                "error": str(error),
            }
        records.append(record)

    artifact = None
    if best_labels is not None:
        artifact = _save_labels(best_labels, job.get("labels_path"))
    successful = [record for record in records if record["status"] == "ok"]
    return {
        **base,
        "status": "ok" if successful else "all_cuts_failed",
        "candidate_strategy": (
            "quantiles_of_base_inner_outer_vertex_births_and_msf_weights"
        ),
        "candidate_count": int(cuts.size),
        "successful_count": len(successful),
        "scan_wall_seconds": time.perf_counter() - scan_started,
        "assignment_seconds_sum": float(
            sum(float(record["assignment_seconds"]) for record in records)
        ),
        "sampled_oracle_best_ARI": None if best_cut is None else best_ari,
        "oracle_best_ARI": None if best_cut is None else best_ari,
        "oracle_best_cut": best_cut,
        "oracle_best_labels_artifact": artifact,
        "cuts": records,
    }


def _run_powercover(
    points: np.ndarray,
    truth: np.ndarray | None,
    job: dict[str, Any],
) -> dict[str, Any]:
    from perg_hgp.backends.power_cover_3d_cuda import (
        PowerCover3D,
        PowerCoverConfig,
    )

    config_values = dict(job["powercover_config"])
    config_values["K"] = int(job["K"])
    entropy_mode = job["entropy_mode"]
    if entropy_mode == "fixed_hard":
        config_values.update(
            regularization_mode="fixed_kappa",
            kappa=float(job.get("hard_kappa", 1.0)),
            distortion_budget_absolute=None,
            distortion_budget_relative=None,
            local_scale_k=None,
        )
    elif entropy_mode == "local_distortion":
        config_values.update(
            regularization_mode="local_distortion",
            kappa=1.0,
            distortion_budget_absolute=job.get("distortion_budget_absolute"),
            distortion_budget_relative=job.get("distortion_budget_relative"),
            local_scale_k=int(job.get("local_scale_k") or job["K"]),
        )
    else:
        raise ValueError(f"unsupported entropy mode: {entropy_mode}")
    config = PowerCoverConfig(**config_values)
    progress = ProgressRecorder()
    estimator = PowerCover3D(
        config=config,
        backend=job["powercover_backend"],
        progress=progress,
    )
    if job["powercover_backend"] == "cuda":
        _synchronize_cuda()
    fit_started = time.perf_counter()
    hierarchy = estimator.fit(points)
    if job["powercover_backend"] == "cuda":
        _synchronize_cuda()
    fit_wall = time.perf_counter() - fit_started

    hierarchy_artifact = None
    hierarchy_save_seconds = None
    if job.get("save_hierarchy", False):
        destination = Path(job["hierarchy_path"])
        save_started = time.perf_counter()
        hierarchy.save(str(destination), include_sites=False)
        hierarchy_save_seconds = time.perf_counter() - save_started
        hierarchy_artifact = str(destination)

    cut_scan = _scan_powercover_cuts(hierarchy, truth, job)
    return {
        "implementation": "perg_hgp.PowerCover3D",
        "backend": job["powercover_backend"],
        "config": config.to_dict(),
        "fit_wall_seconds": fit_wall,
        "stage_timings_seconds": dict(hierarchy.timings),
        "progress": progress.stages,
        "accuracy_report": hierarchy.report.to_dict(),
        "neighbor_audits": hierarchy.neighbor_audits,
        "memory_estimate": hierarchy.memory_estimate.to_dict(),
        "transform": hierarchy.transform.to_dict(),
        "grid": hierarchy.field.spec.to_dict(),
        "regularization_diagnostics": hierarchy.sites.diagnostics.to_dict(),
        "power_query_diagnostics": hierarchy.field.diagnostics.to_dict(),
        "forest": {
            name: {
                "vertices": int(getattr(hierarchy, name).births.shape[0]),
                "edges": int(getattr(hierarchy, name).weights.shape[0]),
            }
            for name in ("base_forest", "inner_forest", "outer_forest")
        },
        "cut_scan": cut_scan,
        "hierarchy_artifact": hierarchy_artifact,
        "hierarchy_save_seconds": hierarchy_save_seconds,
    }


def _run_hdbscan(
    points: np.ndarray,
    truth: np.ndarray | None,
    job: dict[str, Any],
) -> dict[str, Any]:
    implementation: str
    try:
        from sklearn.cluster import HDBSCAN

        implementation = "sklearn.cluster.HDBSCAN"
        parameters: dict[str, Any] = {
            "min_cluster_size": int(job["min_cluster_size"]),
            "min_samples": int(job["K"]),
            "metric": "euclidean",
            "allow_single_cluster": bool(job.get("allow_single_cluster", False)),
        }
        signature = inspect.signature(HDBSCAN)
        if "n_jobs" in signature.parameters:
            parameters["n_jobs"] = int(job.get("n_jobs", -1))
        estimator = HDBSCAN(**parameters)
        k_semantics = (
            "sklearn min_samples includes the point itself, matching K-NN rank"
        )
    except ImportError:
        try:
            import hdbscan
        except ImportError as error:
            raise MethodUnavailable(
                "neither sklearn.cluster.HDBSCAN nor hdbscan.HDBSCAN is importable"
            ) from error

        implementation = "hdbscan.HDBSCAN"
        parameters = {
            "min_cluster_size": int(job["min_cluster_size"]),
            "min_samples": int(job["K"]),
            "metric": "euclidean",
            "core_dist_n_jobs": int(job.get("n_jobs", -1)),
            "allow_single_cluster": bool(job.get("allow_single_cluster", False)),
        }
        estimator = hdbscan.HDBSCAN(**parameters)
        k_semantics = (
            "contrib hdbscan min_samples excludes self; recorded K is therefore "
            "not exactly the PowerCover rank convention"
        )
    fit_started = time.perf_counter()
    estimator.fit(points)
    labels = np.asarray(estimator.labels_, dtype=np.int32)
    fit_wall = time.perf_counter() - fit_started
    artifact = _save_labels(labels, job.get("labels_path"))
    return {
        "implementation": implementation,
        "parameters": parameters,
        "K_semantics": k_semantics,
        "fit_and_label_wall_seconds": fit_wall,
        "labels": label_summary(labels),
        "metrics": clustering_scores(truth, labels),
        "labels_artifact": artifact,
    }


def _run_hgp_old(
    points: np.ndarray,
    truth: np.ndarray | None,
    job: dict[str, Any],
) -> dict[str, Any]:
    hgp_root = Path(job["hgp_old_root"]).resolve()
    source = hgp_root / "src"
    if str(source) not in sys.path:
        sys.path.insert(0, str(source))
    import_started = time.perf_counter()
    try:
        from hgp_clusterer import HGPClusterer
    except Exception as error:
        raise MethodUnavailable(
            f"HGP-old is not importable from {source}: {type(error).__name__}: {error}"
        ) from error
    import_seconds = time.perf_counter() - import_started
    parameters = {
        "K": int(job["K"]),
        "min_cluster_size": int(job["min_cluster_size"]),
        "min_samples": max(int(job["K"]) + 1, int(job.get("hgp_min_samples", 0))),
        "method": job.get("hgp_method", "eom"),
        "backend": job.get("hgp_backend", "cgal"),
        "cgal_root": job.get("cgal_root", str(hgp_root / "CGALDelaunay")),
        "expZ": float(job.get("hgp_exp_z", 2.0)),
        "verbose": bool(job.get("verbose_algorithm", False)),
    }
    estimator = HGPClusterer(**parameters)
    fit_started = time.perf_counter()
    estimator.fit(points)
    labels = np.asarray(estimator.labels_, dtype=np.int32)
    fit_wall = time.perf_counter() - fit_started
    artifact = _save_labels(labels, job.get("labels_path"))
    return {
        "implementation": "HGP-old.hgp_clusterer.HGPClusterer",
        "import_seconds": import_seconds,
        "parameters": parameters,
        "fit_and_label_wall_seconds": fit_wall,
        "labels": label_summary(labels),
        "metrics": clustering_scores(truth, labels),
        "labels_artifact": artifact,
    }


METHODS: dict[
    str, Callable[[np.ndarray, np.ndarray | None, dict[str, Any]], dict[str, Any]]
] = {
    "powercover_cpu": _run_powercover,
    "powercover_cuda": _run_powercover,
    "hdbscan": _run_hdbscan,
    "hgp_old": _run_hgp_old,
}


def execute_job(job: dict[str, Any]) -> tuple[dict[str, Any], int]:
    started_at = _utc_now()
    started = time.perf_counter()
    result: dict[str, Any] = {
        "schema": "perg_hgp.benchmark_result",
        "schema_version": 1,
        "job_id": job.get("job_id"),
        "method": job.get("method"),
        "K": job.get("K"),
        "started_at": started_at,
        "status": "running",
        "job": job,
        "runtime": runtime_provenance(),
        "software_versions": _software_versions(),
        "git": git_provenance(REPOSITORY_DIR),
    }
    monitor: ResourceMonitor | None = None
    try:
        method = str(job["method"])
        if method not in METHODS:
            raise ValueError(f"unknown benchmark method: {method}")
        points, truth, dataset_metadata = load_dataset(job["dataset_metadata_path"])
        result["dataset"] = dataset_metadata
        result["dataset_manifest"] = str(job["dataset_metadata_path"])
        monitor = ResourceMonitor(float(job.get("resource_interval_seconds", 0.05)))
        with monitor:
            method_result = METHODS[method](points, truth, job)
        result["method_result"] = method_result
        result["resources"] = monitor.to_dict()
        result["status"] = "ok"
        exit_code = 0
    except MethodUnavailable as error:
        if monitor is not None:
            result["resources"] = monitor.to_dict()
        result.update(
            status="unavailable",
            reason=str(error),
            error_type=type(error).__name__,
        )
        exit_code = 0
    except BaseException as error:
        if monitor is not None:
            result["resources"] = monitor.to_dict()
        result.update(
            status="error",
            error_type=type(error).__name__,
            error=str(error),
            traceback=traceback.format_exc(),
        )
        exit_code = 1
    result["finished_at"] = _utc_now()
    result["worker_wall_seconds"] = time.perf_counter() - started
    return result, exit_code


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Execute one process-isolated PowerCover3D/HDBSCAN/HGP-old job "
            "described by JSON. Normally called by benchmarks.cli."
        )
    )
    parser.add_argument("--job", required=True, type=Path, help="JSON job manifest")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    with open(args.job, "r", encoding="utf-8") as stream:
        job = json.load(stream)
    output_path = job.get("output_path")
    if not output_path:
        raise SystemExit("job manifest has no output_path")
    result, exit_code = execute_job(job)
    atomic_write_json(output_path, result)
    print(
        json.dumps(
            {
                "benchmark_result": str(output_path),
                "status": result["status"],
                "worker_wall_seconds": result["worker_wall_seconds"],
            },
            sort_keys=True,
        ),
        flush=True,
    )
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
