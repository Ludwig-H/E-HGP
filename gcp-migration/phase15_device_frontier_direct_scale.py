#!/usr/bin/env python3
"""Run the Phase 15 component directly at 10M, then 30M, under a hard deadline."""

from __future__ import annotations

import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
from typing import Any, NoReturn

RESULT_SCHEMA = (
    "morsehgp3d.phase15.morton_yao48_device_tiled_pair_frontier_direct_scale.v1"
)
MODE = "device_resident_budgeted_morton_yao48_anchor_tiles_direct_scale"
HARNESS_SCHEMA = "morsehgp3d.phase15.device_frontier_direct_scale_harness.v1"
MANIFEST_SCHEMA = "morsehgp3d.phase15.device_frontier_direct_scale_manifest.v1"
POINT_COUNTS = (10_000_000, 30_000_000)
DEFAULT_SEED = 0x15A048D1E7C93B25
MAX_GUARD_SECONDS = 28_800
TIMEOUT_KILL_AFTER_SECONDS = 30


class ContractError(RuntimeError):
    """The process or its JSON escaped the bounded profiling contract."""


def fail(message: str) -> NoReturn:
    raise ContractError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def natural(value: Any, label: str, *, positive: bool = False) -> int:
    require(type(value) is int and value >= 0, f"{label} must be a natural")
    require(not positive or value > 0, f"{label} must be positive")
    return value


def boolean(value: Any, label: str) -> bool:
    require(type(value) is bool, f"{label} must be a boolean")
    return value


def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            fail(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def decode_one_json_line(payload: bytes, label: str) -> dict[str, Any]:
    try:
        text = payload.decode("utf-8")
    except UnicodeDecodeError as error:
        raise ContractError(f"{label} is not UTF-8") from error
    lines = text.splitlines()
    require(len(lines) == 1 and lines[0].strip(), f"{label} must be one JSON line")
    try:
        value = json.loads(lines[0], object_pairs_hook=reject_duplicate_keys)
    except (json.JSONDecodeError, ContractError) as error:
        raise ContractError(f"{label} is not strict JSON: {error}") from error
    require(isinstance(value, dict), f"{label} must be a JSON object")
    return value


def canonical_json(value: Any) -> bytes:
    return (
        json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)
        + "\n"
    ).encode("ascii")


def sha256_bytes(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def atomic_write(path: Path, payload: bytes) -> None:
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{path.name}.", suffix=".partial", dir=path.parent
    )
    temporary = Path(temporary_name)
    try:
        offset = 0
        while offset < len(payload):
            offset += os.write(descriptor, payload[offset:])
        os.fsync(descriptor)
        os.close(descriptor)
        descriptor = -1
        os.replace(temporary, path)
        directory = os.open(path.parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
        try:
            os.fsync(directory)
        finally:
            os.close(directory)
    finally:
        if descriptor >= 0:
            os.close(descriptor)
        temporary.unlink(missing_ok=True)


def validate_result(
    record: dict[str, Any],
    *,
    point_count: int,
    maximum_closed_rank: int,
    anchor_tile_capacity: int,
    seed: int,
) -> dict[str, Any]:
    expected_strings = {
        "backend": "cuda_g4",
        "deployment_status": "profile_only",
        "family": "affine_uniform_binary64",
        "mode": MODE,
        "profile": "hgp_reduced",
        "public_status": "not_claimed",
        "run_kind": "direct_scale",
        "schema": RESULT_SCHEMA,
    }
    for key, expected in expected_strings.items():
        require(record.get(key) == expected, f"{key} differs from the direct-scale contract")
    require(record.get("point_count") == point_count, "point_count differs")
    require(record.get("maximum_closed_rank") == maximum_closed_rank, "rank differs")
    require(record.get("anchor_tile_capacity") == anchor_tile_capacity, "tile differs")
    require(record.get("seed") == seed, "seed differs")

    required_true = (
        "backpressure_validated",
        "component_only",
        "direct_large_scale_guard_passed",
        "execution_success",
        "pair_mass_partition_validated",
        "profile_only",
    )
    required_false = (
        "dense_pair_fallback_performed",
        "exact_diametral_rank_evaluated",
        "exactness_claimed",
        "global_pair_matrix_materialized",
        "higher_order_structure_materialized",
        "ordinary_delaunay_materialized",
        "ordinary_emst_computed",
        "process_restart_resumable",
        "product_claimed",
        "qualification_claimed",
        "raw_candidate_or_prune_view_accessed",
        "scalability_claimed",
        "scientific_claimed",
        "scientific_pair_catalog_published",
        "warm_e2e_slo_claimed",
    )
    for key in required_true:
        require(boolean(record.get(key), key), f"{key} must be true")
    for key in required_false:
        require(not boolean(record.get(key), key), f"{key} must be false")

    universe = point_count * (point_count - 1) // 2
    candidate = natural(record.get("candidate_pair_mass"), "candidate_pair_mass")
    pruned = natural(
        record.get("certified_pruned_pair_mass"), "certified_pruned_pair_mass"
    )
    unresolved = natural(record.get("unresolved_pair_mass"), "unresolved_pair_mass")
    require(record.get("unordered_pair_universe_count") == universe, "pair universe differs")
    require(candidate + pruned + unresolved == universe, "pair mass does not close")

    coverage = boolean(record.get("coverage_complete"), "coverage_complete")
    require(record.get("coverage_success") is coverage, "coverage receipts differ")
    censored = boolean(record.get("censored"), "censored")
    complete_mass = boolean(
        record.get("complete_pair_mass_validated"), "complete_pair_mass_validated"
    )
    stop_reason = record.get("stop_reason")
    if coverage:
        require(not censored and unresolved == 0, "complete coverage is inconsistent")
        require(complete_mass and stop_reason == "none", "complete mass receipt differs")
        require(record.get("completed_anchor_count") == point_count - 1, "anchor prefix differs")
    else:
        require(censored and not complete_mass, "censored coverage is inconsistent")
        require(
            stop_reason in {"node_visit_capacity", "candidate_capacity", "prune_region_capacity"},
            "censored stop reason differs",
        )

    advances = natural(record.get("advance_count"), "advance_count", positive=True)
    leases = natural(record.get("candidate_tile_lease_count"), "tile leases")
    releases = natural(record.get("candidate_tile_release_count"), "tile releases")
    require(leases == releases, "tile lease/release backpressure does not close")
    require(record.get("candidate_device_to_host_count") == 0, "candidate D2H occurred")

    candidate_per_anchor = natural(
        record.get("fixed_candidate_capacity_per_anchor"), "candidate capacity", positive=True
    )
    node_visits_per_subdivision = natural(
        record.get("fixed_node_visit_capacity_per_anchor"), "node visit slice", positive=True
    )
    prune_per_anchor = natural(
        record.get("fixed_prune_region_capacity_per_anchor"), "prune capacity", positive=True
    )
    banks_per_anchor = natural(
        record.get("fixed_witness_bank_count_per_anchor"), "witness banks", positive=True
    )
    candidate_size = natural(record.get("candidate_record_size_bytes"), "candidate ABI", positive=True)
    prune_size = natural(record.get("prune_region_record_size_bytes"), "prune ABI", positive=True)
    witness_size = natural(record.get("witness_bank_slot_size_bytes"), "witness ABI", positive=True)
    control_size = natural(record.get("anchor_control_record_size_bytes"), "control ABI", positive=True)
    require(
        (candidate_per_anchor, node_visits_per_subdivision, prune_per_anchor, banks_per_anchor)
        == (640, 2048, 2048, 48),
        "fixed frontier capacities differ from the qualified ABI",
    )
    require((candidate_size, prune_size, witness_size, control_size) == (48, 128, 32, 96), "record ABI sizes differ")

    expected_candidate_bytes = anchor_tile_capacity * candidate_per_anchor * candidate_size
    expected_prune_bytes = anchor_tile_capacity * prune_per_anchor * prune_size
    expected_witness_bytes = (
        anchor_tile_capacity * banks_per_anchor * (maximum_closed_rank - 1) * witness_size
    )
    expected_control_bytes = anchor_tile_capacity * control_size
    require(record.get("peak_candidate_device_capacity_bytes") == expected_candidate_bytes, "candidate allocation differs")
    require(record.get("peak_prune_device_capacity_bytes") == expected_prune_bytes, "prune allocation differs")
    require(record.get("peak_witness_bank_device_capacity_bytes") == expected_witness_bytes, "witness allocation differs")
    require(record.get("peak_control_device_capacity_bytes") == expected_control_bytes, "control allocation differs")
    expected_tile_bytes = expected_candidate_bytes + expected_prune_bytes + expected_witness_bytes + expected_control_bytes
    require(record.get("peak_tile_output_device_capacity_bytes") == expected_tile_bytes, "tile allocation mass differs")

    node_count = 2 * point_count - 1
    maximum_subdivisions = (node_count + node_visits_per_subdivision - 1) // node_visits_per_subdivision
    require(
        record.get("maximum_traversal_subdivision_count_per_anchor") == maximum_subdivisions,
        "traversal subdivision ceiling differs",
    )
    subdivisions = natural(record.get("traversal_subdivision_count"), "subdivisions", positive=True)
    require(record.get("kernel_launch_count") == subdivisions, "kernel/subdivision receipt differs")
    require(record.get("synchronization_count") == subdivisions + advances, "sync/subdivision receipt differs")
    require(record.get("resume_control_device_to_host_count") == subdivisions, "resume D2H receipt differs")
    require(record.get("resume_control_device_to_host_byte_count") == subdivisions * 8, "resume D2H byte receipt differs")

    total_memory = natural(record.get("device_total_bytes"), "device total", positive=True)
    initial_free = natural(record.get("initial_device_free_bytes"), "initial device free", positive=True)
    minimum_free = natural(record.get("minimum_device_free_bytes"), "minimum device free", positive=True)
    final_free = natural(record.get("final_device_free_bytes"), "final device free", positive=True)
    require(minimum_free <= initial_free <= total_memory, "initial/minimum device memory differs")
    require(minimum_free <= final_free <= total_memory, "final/minimum device memory differs")
    require(minimum_free + expected_tile_bytes <= total_memory, "reported tile cannot fit device memory")
    require(record.get("device_memory_observation_count") == advances + 4, "memory observation receipt differs")
    natural(record.get("traversal_device_capacity_bytes"), "traversal allocation", positive=True)
    natural(record.get("peak_host_rss_bytes"), "peak host RSS", positive=True)

    return {
        "censored": censored,
        "coverage_complete": coverage,
        "point_count": point_count,
        "stop_reason": stop_reason,
        "total_ns": natural(record.get("total_ns"), "total_ns", positive=True),
        "unresolved_pair_mass": unresolved,
    }


def validate_gnu_timeout(path: str) -> None:
    probe = subprocess.run([path, "--version"], capture_output=True, text=True, check=False)
    require(probe.returncode == 0 and "GNU coreutils" in probe.stdout.splitlines()[0], "GNU timeout is required")


def run_case(
    *,
    binary: Path,
    timeout_binary: str,
    run_directory: Path,
    point_count: int,
    maximum_closed_rank: int,
    anchor_tile_capacity: int,
    seed: int,
    timeout_seconds: int,
    deadline_epoch: int,
    reserve_seconds: int,
) -> tuple[dict[str, Any], bool]:
    artifact = run_directory / f"n{point_count}.json"
    if artifact.is_symlink():
        fail(f"refusing symbolic artifact: {artifact}")
    if artifact.exists():
        payload = artifact.read_bytes()
        record = decode_one_json_line(payload, str(artifact))
        summary = validate_result(
            record,
            point_count=point_count,
            maximum_closed_rank=maximum_closed_rank,
            anchor_tile_capacity=anchor_tile_capacity,
            seed=seed,
        )
        summary["artifact"] = artifact.name
        summary["artifact_sha256"] = sha256_bytes(payload)
        return summary, True

    remaining = deadline_epoch - int(time.time()) - reserve_seconds
    require(remaining >= 30, "verified deadline leaves less than 30 seconds for the next case")
    effective_timeout = min(timeout_seconds, remaining)
    command = [
        timeout_binary,
        f"--kill-after={TIMEOUT_KILL_AFTER_SECONDS}s",
        f"{effective_timeout}s",
        str(binary),
        "--direct-scale",
        "--point-count",
        str(point_count),
        "--family",
        "affine_uniform_binary64",
        "--maximum-closed-rank",
        str(maximum_closed_rank),
        "--anchor-tile-capacity",
        str(anchor_tile_capacity),
        "--seed",
        str(seed),
    ]
    completed = subprocess.run(command, capture_output=True, check=False)
    if completed.returncode != 0:
        stamp = int(time.time())
        atomic_write(run_directory / f"n{point_count}.failed-{stamp}.stdout", completed.stdout)
        atomic_write(run_directory / f"n{point_count}.failed-{stamp}.stderr", completed.stderr)
        if completed.returncode == 124:
            fail(f"n={point_count} reached its {effective_timeout}s hard timeout")
        fail(f"n={point_count} binary exit code is {completed.returncode}")

    try:
        record = decode_one_json_line(completed.stdout, f"n={point_count} stdout")
        summary = validate_result(
            record,
            point_count=point_count,
            maximum_closed_rank=maximum_closed_rank,
            anchor_tile_capacity=anchor_tile_capacity,
            seed=seed,
        )
    except ContractError:
        stamp = int(time.time())
        atomic_write(run_directory / f"n{point_count}.invalid-{stamp}.stdout", completed.stdout)
        atomic_write(run_directory / f"n{point_count}.invalid-{stamp}.stderr", completed.stderr)
        raise
    atomic_write(run_directory / f"n{point_count}.stderr.log", completed.stderr)
    atomic_write(artifact, completed.stdout)
    summary["artifact"] = artifact.name
    summary["artifact_sha256"] = sha256_bytes(completed.stdout)
    return summary, False


def parse_arguments(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run only the direct 10M/30M Phase 15 component profiles."
    )
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--deadline-epoch", required=True, type=int)
    parser.add_argument("--seed", type=int, default=DEFAULT_SEED)
    parser.add_argument("--anchor-tile-capacity", type=int, default=4096)
    parser.add_argument("--maximum-closed-rank", type=int, default=11)
    parser.add_argument("--ten-million-timeout-seconds", type=int, default=7200)
    parser.add_argument("--thirty-million-timeout-seconds", type=int, default=19_800)
    parser.add_argument("--finalization-reserve-seconds", type=int, default=300)
    parser.add_argument(
        "--through-point-count", type=int, choices=POINT_COUNTS, default=30_000_000
    )
    parser.add_argument("--require-coverage", action="store_true")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    arguments = parse_arguments(argv)
    require(arguments.binary.is_absolute(), "--binary must be absolute")
    require(arguments.output_dir.is_absolute(), "--output-dir must be absolute")
    binary = arguments.binary.resolve(strict=True)
    require(binary.is_file() and os.access(binary, os.X_OK), "binary must be executable")
    require(1 <= arguments.anchor_tile_capacity <= 4096, "tile must lie in [1,4096]")
    require(2 <= arguments.maximum_closed_rank <= 11, "rank must lie in [2,11]")
    require(0 <= arguments.seed <= (1 << 64) - 1, "seed must fit uint64")
    require(30 <= arguments.ten_million_timeout_seconds <= MAX_GUARD_SECONDS, "10M timeout is invalid")
    require(30 <= arguments.thirty_million_timeout_seconds <= MAX_GUARD_SECONDS, "30M timeout is invalid")
    require(30 <= arguments.finalization_reserve_seconds <= 900, "reserve is invalid")
    now = int(time.time())
    require(arguments.deadline_epoch <= now + MAX_GUARD_SECONDS, "deadline exceeds eight hours")

    output_directory = arguments.output_dir
    output_directory.mkdir(parents=True, exist_ok=True)
    binary_sha256 = sha256_file(binary)
    request = {
        "anchor_tile_capacity": arguments.anchor_tile_capacity,
        "binary_sha256": binary_sha256,
        "maximum_closed_rank": arguments.maximum_closed_rank,
        "schema": HARNESS_SCHEMA,
        "seed": arguments.seed,
    }
    request_digest = sha256_bytes(canonical_json(request))
    run_directory = output_directory / f"request-{request_digest}"
    run_directory.mkdir(exist_ok=True)
    lock_path = run_directory / ".lock"
    lock_descriptor = os.open(lock_path, os.O_CREAT | os.O_RDWR | getattr(os, "O_NOFOLLOW", 0), 0o600)
    try:
        try:
            fcntl.flock(lock_descriptor, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as error:
            raise ContractError(f"another harness owns {run_directory}") from error

        point_counts = tuple(
            count for count in POINT_COUNTS if count <= arguments.through_point_count
        )
        missing = any(not (run_directory / f"n{count}.json").exists() for count in point_counts)
        timeout_binary = shutil.which("timeout")
        if missing:
            require(timeout_binary is not None, "timeout executable is missing")
            validate_gnu_timeout(timeout_binary)

        summaries: list[dict[str, Any]] = []
        resumed_count = 0
        timeout_by_count = {
            10_000_000: arguments.ten_million_timeout_seconds,
            30_000_000: arguments.thirty_million_timeout_seconds,
        }
        for point_count in point_counts:
            summary, resumed = run_case(
                binary=binary,
                timeout_binary=timeout_binary or "timeout",
                run_directory=run_directory,
                point_count=point_count,
                maximum_closed_rank=arguments.maximum_closed_rank,
                anchor_tile_capacity=arguments.anchor_tile_capacity,
                seed=arguments.seed,
                timeout_seconds=timeout_by_count[point_count],
                deadline_epoch=arguments.deadline_epoch,
                reserve_seconds=arguments.finalization_reserve_seconds,
            )
            summaries.append(summary)
            resumed_count += int(resumed)

        coverage_complete = all(summary["coverage_complete"] for summary in summaries)
        manifest = {
            "artifact_restart_resumable": True,
            "artifact_role": "profile_only",
            "binary": str(binary),
            "binary_sha256": binary_sha256,
            "cases": summaries,
            "completed_epoch": int(time.time()),
            "coverage_complete": coverage_complete,
            "deadline_epoch": arguments.deadline_epoch,
            "exactness_claimed": False,
            "execution_success": True,
            "product_claimed": False,
            "qualification_claimed": False,
            "request_sha256": request_digest,
            "resume_scope": "completed_case_artifacts_for_identical_binary_and_request",
            "resumed_case_count": resumed_count,
            "scalability_claimed": False,
            "schema": MANIFEST_SCHEMA,
            "scientific_claimed": False,
        }
        manifest_path = run_directory / f"manifest-through-{arguments.through_point_count}.json"
        encoded_manifest = canonical_json(manifest)
        atomic_write(manifest_path, encoded_manifest)
        sys.stdout.buffer.write(encoded_manifest)
        if arguments.require_coverage and not coverage_complete:
            return 3
        return 0
    finally:
        os.close(lock_descriptor)


if __name__ == "__main__":
    try:
        raise SystemExit(main(sys.argv[1:]))
    except ContractError as error:
        print(f"[direct-scale] {error}", file=sys.stderr)
        raise SystemExit(1)
