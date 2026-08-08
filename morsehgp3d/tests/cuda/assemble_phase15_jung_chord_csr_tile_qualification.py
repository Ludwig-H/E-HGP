#!/usr/bin/env python3
"""Assemble and validate the guarded P15-HOCUDA-P0 G4 observation."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import stat
from typing import Any, NoReturn


TOOL_SCHEMA = "morsehgp3d.phase15.jung_chord_csr_tile.qualification_scale.v1"
ARTIFACT_SCHEMA = (
    "morsehgp3d.phase15.jung_chord_csr_tile.g4_qualification.v1"
)
BASE_IMAGE_REF = (
    "nvidia/cuda:12.9.2-devel-ubuntu24.04@"
    "sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"
)
PHASE = "P15-HOCUDA-P0"
BACKEND = "cuda_g4_plus_reference_cpu_oracle"
EXECUTION_BACKEND = "cuda_g4"
PROFILE = "hgp_reduced"
MODE = "proposal_only_shallow_higher_support_work_falsifier_v1"
ORACLE_TIMEOUT_SECONDS = 120
SCALE_50K_TIMEOUT_SECONDS = 300
SHA256_RE = re.compile(r"[0-9a-f]{64}\Z")
GIT_SHA_RE = re.compile(r"[0-9a-f]{40}\Z")

LOG_NAMES = (
    "preflight.stdout.log",
    "preflight.stderr.log",
    "image-build.stdout.log",
    "image-build.stderr.log",
    "configure.stdout.log",
    "configure.stderr.log",
    "target-build.stdout.log",
    "target-build.stderr.log",
    "oracle32.stdout.json",
    "oracle32.stderr.log",
    "oracle32.memcheck.log",
    "uniform-50000.stdout.json",
    "uniform-50000.stderr.log",
    "eight-clusters-50000.stdout.json",
    "eight-clusters-50000.stderr.log",
)


def fail(message: str) -> NoReturn:
    raise SystemExit(message)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require_regular(path: Path, label: str, *, executable: bool = False) -> None:
    try:
        metadata = path.lstat()
    except OSError as error:
        fail(f"{label} is missing: {error}")
    if not stat.S_ISREG(metadata.st_mode) or path.is_symlink():
        fail(f"{label} must be a regular non-symlink file")
    if executable and metadata.st_mode & 0o111 == 0:
        fail(f"{label} must be executable")


def load_json(path: Path, label: str) -> dict[str, Any]:
    require_regular(path, label)
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        fail(f"{label} is not one UTF-8 JSON object: {error}")
    if not isinstance(value, dict):
        fail(f"{label} must contain a JSON object")
    return value


def require_bool(value: Any, expected: bool, label: str) -> None:
    if type(value) is not bool or value is not expected:
        fail(f"{label} must be {str(expected).lower()}")


def require_int(value: Any, label: str, *, minimum: int = 0) -> int:
    if type(value) is not int or value < minimum:
        fail(f"{label} must be an integer >= {minimum}")
    return value


def validate_tool_result(
    value: dict[str, Any],
    *,
    expected_sha: str,
    family: str,
    point_count: int,
    morton_window: int,
    oracle: bool,
    chord_capacity: int,
    exit_status: int,
) -> str:
    expected_scalars: dict[str, Any] = {
        "schema": TOOL_SCHEMA,
        "git_sha": expected_sha,
        "phase": PHASE,
        "backend": BACKEND,
        "execution_backend": EXECUTION_BACKEND,
        "profile": PROFILE,
        "mode": MODE,
        "deployment_status": "prototype_only",
        "public_status": "not_claimed",
        "native_q3_gate_closed": False,
        "native_q4_gate_closed": False,
        "family": family,
        "point_count": point_count,
        "support_size": 4,
        "maximum_closed_rank": 11,
        "anchor_source": "morton_window_proposal_only",
        "morton_window": morton_window,
        "anchor_completeness_claimed": False,
        "quadratic_chord_pair_baseline_executed": False,
        "unbudgeted_profile": True,
        "output_capacity_bounded": True,
        "slo_eligible": False,
        "component_only": True,
        "diagnostic_census_non_slo": True,
        "global_pair_matrix_materialized": False,
        "support_tuple_universe_materialized": False,
        "higher_order_delaunay_mosaic_materialized": False,
        "scientific_claim": False,
        "oracle_performed": oracle,
        "hostile_q4_points_injected": oracle,
        "exact_q4_boundary_sign_fixture_passed": oracle,
        "cuda_q4_boundary_fixture_exercised": oracle,
    }
    for key, expected in expected_scalars.items():
        if value.get(key) != expected or type(value.get(key)) is not type(expected):
            fail(f"{family}/{point_count}: {key} must be exactly {expected!r}")

    expected_anchor_count = (
        point_count * morton_window
        - morton_window * (morton_window + 1) // 2
    )
    if value.get("anchor_count_a") != expected_anchor_count:
        fail(
            f"{family}/{point_count}: anchor_count_a must be "
            f"{expected_anchor_count}"
        )
    expected_pairs = point_count * (point_count - 1) // 2
    if value.get("unordered_pair_universe") != expected_pairs:
        fail(f"{family}/{point_count}: unordered_pair_universe is inconsistent")

    for key in (
        "resident_chord_count_M",
        "required_device_bytes",
        "installed_device_bytes",
        "anchor_h2d_bytes",
        "control_d2h_bytes",
        "kernels",
        "library_submissions",
        "synchronizations",
        "build_ns",
        "anchor_generation_ns",
        "classify_wall_ns",
    ):
        require_int(value.get(key), f"{family}/{point_count}.{key}")
    if value.get("chord_d2h_bytes_product_path") != 0:
        fail(f"{family}/{point_count}: product path copied chords to the host")
    if require_int(
        value.get("resident_chord_count_M"),
        f"{family}/{point_count}.resident_chord_count_M",
    ) > chord_capacity:
        fail(f"{family}/{point_count}: committed chord count exceeds capacity")

    status = value.get("status")
    stop_reason = value.get("stop_reason")
    if status == "tile_complete":
        if exit_status != 0 or stop_reason != "none":
            fail(f"{family}/{point_count}: complete result has inconsistent exit/stop")
    elif status == "capacity_exhausted":
        if oracle:
            fail("the bounded oracle must not exhaust capacity")
        if exit_status != 1 or stop_reason != "chord_point_id_capacity":
            fail(f"{family}/{point_count}: capacity result has inconsistent exit/stop")
    else:
        fail(f"{family}/{point_count}: forbidden terminal status {status!r}")

    if oracle:
        for key in (
            "oracle_passed",
        ):
            require_bool(value.get(key), True, f"oracle32.{key}")
        for key in (
            "oracle_active_omissions",
            "oracle_false_depth_rejections",
            "oracle_constant_lower_bound_violations",
            "oracle_support_eligible_false_negatives",
            "oracle_support_eligible_unjustified_nonambiguous",
            "oracle_anchor_identity_violations",
            "oracle_csr_structure_violations",
            "oracle_duplicate_or_invalid_chord_ids",
            "oracle_unknown_flags",
        ):
            if value.get(key) != 0:
                fail(f"oracle32.{key} must be zero")
        if value.get("status") != "tile_complete":
            fail("oracle32 must complete")
    else:
        require_bool(value.get("oracle_passed"), False, "50k.oracle_passed")
        if value.get("qualification_snapshot_d2h_bytes") != 0:
            fail(f"{family}/{point_count}: qualification snapshot D2H is forbidden")
    return str(status)


def validate_memcheck(path: Path) -> None:
    require_regular(path, "oracle32 memcheck log")
    raw = path.read_text(encoding="utf-8", errors="strict")
    if "ERROR SUMMARY: 0 errors" not in raw:
        fail("compute-sanitizer did not report an error-free oracle run")
    if "========= ERROR SUMMARY: 0 errors" not in raw:
        fail("compute-sanitizer completion marker is absent")


def expected_commands(binary_path: str, chord_capacity: int) -> list[list[str]]:
    common_50k = [
        binary_path,
        "--point-count",
        "50000",
        "--morton-window",
        "8",
        "--support-size",
        "4",
        "--maximum-closed-rank",
        "11",
        "--chord-capacity",
        str(chord_capacity),
    ]
    return [
        [
            "/usr/local/cuda/bin/compute-sanitizer",
            "--target-processes",
            "all",
            "--tool",
            "memcheck",
            "--leak-check",
            "full",
            "--report-api-errors",
            "no",
            "--error-exitcode=86",
            "--log-file",
            "/results/oracle32.memcheck.log",
            binary_path,
            "--point-count",
            "32",
            "--family",
            "uniform_latin",
            "--morton-window",
            "31",
            "--support-size",
            "4",
            "--maximum-closed-rank",
            "11",
            "--chord-capacity",
            "32768",
            "--oracle",
        ],
        common_50k[:3] + ["--family", "uniform_latin"] + common_50k[3:],
        common_50k[:3] + ["--family", "eight_clusters"] + common_50k[3:],
    ]


def validate_artifact(
    value: dict[str, Any],
    *,
    expected_sha: str,
    log_dir: Path,
    allow_final: bool,
) -> None:
    if GIT_SHA_RE.fullmatch(expected_sha) is None:
        fail("expected SHA is not canonical")
    expected_top = {
        "schema": ARTIFACT_SCHEMA,
        "phase": PHASE,
        "backend": BACKEND,
        "execution_backend": EXECUTION_BACKEND,
        "profile": PROFILE,
        "mode": MODE,
        "deployment_status": "prototype_only",
        "public_status": "not_claimed",
        "native_q3_gate_closed": False,
        "native_q4_gate_closed": False,
        "unbudgeted_profile": True,
        "output_capacity_bounded": True,
        "component_only": True,
        "slo_eligible": False,
        "scientific_result_claimed": False,
    }
    for key, expected in expected_top.items():
        if value.get(key) != expected or type(value.get(key)) is not type(expected):
            fail(f"artifact.{key} must be exactly {expected!r}")
    if value.get("git") != {"clean": True, "sha": expected_sha}:
        fail("artifact git identity is inconsistent")
    image = value.get("image")
    if not isinstance(image, dict) or image.get("base_ref") != BASE_IMAGE_REF:
        fail("artifact image provenance is absent")
    if image.get("ref") != f"morsehgp3d-phase15-jung:{expected_sha}":
        fail("artifact image ref is inconsistent")
    if re.fullmatch(r"sha256:[0-9a-f]{64}", str(image.get("id"))) is None:
        fail("artifact image id is not canonical")
    binary = value.get("binary")
    if not isinstance(binary, dict) or SHA256_RE.fullmatch(
        str(binary.get("sha256"))
    ) is None:
        fail("artifact binary digest is absent")
    binary_path = binary.get("container_path")
    if binary_path != (
        "/workspace/repository/build/morsehgp3d-cuda-release/"
        "morsehgp3d_gpu_jung_chord_csr_tile_qualification"
    ):
        fail("artifact binary path is inconsistent")

    config = value.get("configuration")
    if not isinstance(config, dict) or set(config) != {
        "chord_capacity",
        "maximum_closed_rank",
        "morton_window_50k",
        "oracle_chord_capacity",
        "oracle_point_count",
        "point_count_50k",
        "support_size",
        "timeout_seconds_50k",
        "timeout_seconds_oracle",
    }:
        fail("artifact configuration is incomplete")
    chord_capacity = require_int(
        config.get("chord_capacity"), "configuration.chord_capacity", minimum=1
    )
    expected_config = {
        "chord_capacity": chord_capacity,
        "maximum_closed_rank": 11,
        "morton_window_50k": 8,
        "oracle_chord_capacity": 32768,
        "oracle_point_count": 32,
        "point_count_50k": 50000,
        "support_size": 4,
        "timeout_seconds_50k": SCALE_50K_TIMEOUT_SECONDS,
        "timeout_seconds_oracle": ORACLE_TIMEOUT_SECONDS,
    }
    if config != expected_config:
        fail("artifact configuration differs from the fixed campaign")

    logs = value.get("logs")
    if not isinstance(logs, dict) or set(logs) != set(LOG_NAMES):
        fail("artifact log manifest is incomplete")
    for name in LOG_NAMES:
        digest = logs.get(name)
        if SHA256_RE.fullmatch(str(digest)) is None:
            fail(f"log digest for {name} is not canonical")
        path = log_dir / name
        require_regular(path, f"log {name}")
        if sha256_file(path) != digest:
            fail(f"log digest mismatch for {name}")

    runs = value.get("runs")
    if not isinstance(runs, list) or len(runs) != 3:
        fail("artifact must contain exactly three ordered runs")
    commands = expected_commands(str(binary_path), chord_capacity)
    expected_runs = (
        (
            "oracle32_memcheck",
            "uniform_latin",
            32,
            31,
            True,
            ORACLE_TIMEOUT_SECONDS,
        ),
        (
            "uniform_latin_50000",
            "uniform_latin",
            50000,
            8,
            False,
            SCALE_50K_TIMEOUT_SECONDS,
        ),
        (
            "eight_clusters_50000",
            "eight_clusters",
            50000,
            8,
            False,
            SCALE_50K_TIMEOUT_SECONDS,
        ),
    )
    statuses: list[str] = []
    for index, (
        name,
        family,
        point_count,
        window,
        oracle,
        timeout_seconds,
    ) in enumerate(expected_runs):
        record = runs[index]
        if not isinstance(record, dict) or record.get("name") != name:
            fail(f"run {index} is not the fixed {name} unit")
        if record.get("command") != commands[index]:
            fail(f"run {name} command differs from the fixed campaign")
        if record.get("timeout_seconds") != timeout_seconds:
            fail(f"run {name} timeout differs from the fixed campaign")
        exit_status = require_int(record.get("exit_status"), f"{name}.exit_status")
        result = record.get("result")
        if not isinstance(result, dict):
            fail(f"run {name} result is absent")
        statuses.append(
            validate_tool_result(
                result,
                expected_sha=expected_sha,
                family=family,
                point_count=point_count,
                morton_window=window,
                oracle=oracle,
                chord_capacity=32768 if oracle else chord_capacity,
                exit_status=exit_status,
            )
        )
    validate_memcheck(log_dir / "oracle32.memcheck.log")

    capacity_exhausted = any(status == "capacity_exhausted" for status in statuses)
    checks = value.get("checks")
    expected_checks = {
        "capacity_exhausted_observed": capacity_exhausted,
        "compute_sanitizer_oracle32": "passed",
        "fixed_run_count": 3,
        "intermediate_point_counts_executed": [],
        "oracle32": "passed",
        "targeted_build": "passed",
    }
    if checks != expected_checks:
        fail("artifact checks are inconsistent")
    claims = value.get("claims")
    expected_claims = {
        "anchor_completeness_claimed": False,
        "exact_hierarchy_claimed": False,
        "product_claimed": False,
        "public_status_claimed": False,
        "scalability_claimed": False,
        "scientific_result_claimed": False,
        "slo_claimed": False,
        "warm_e2e_slo_claimed": False,
    }
    if claims != expected_claims:
        fail("artifact claims must all remain false")

    lifecycle = value.get("vm_lifecycle")
    if not isinstance(lifecycle, dict) or lifecycle.get("worker_mutates_gcp") is not False:
        fail("artifact lifecycle does not attest a non-mutating worker")
    worker_status = (
        "worker_observed_capacity_exhausted_pending_shutdown"
        if capacity_exhausted
        else "worker_passed_pending_shutdown"
    )
    final_status = (
        "capacity_exhausted_shutdown_certified"
        if capacity_exhausted
        else "passed_shutdown_certified"
    )
    if allow_final and value.get("status") == final_status:
        required_lifecycle = {
            "guest_shutdown_guard_verified": True,
            "last_start_timestamp": lifecycle.get("last_start_timestamp"),
            "stop_responsibility": "fulfilled_by_external_orchestrator",
            "target_status": "TERMINATED",
            "targeted_stop_verified_at_utc": lifecycle.get(
                "targeted_stop_verified_at_utc"
            ),
            "worker_mutates_gcp": False,
        }
        if lifecycle != required_lifecycle:
            fail("final lifecycle certification is incomplete")
        if not isinstance(lifecycle.get("last_start_timestamp"), str):
            fail("final lifecycle generation is absent")
        if re.fullmatch(
            r"[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z",
            str(lifecycle.get("targeted_stop_verified_at_utc")),
        ) is None:
            fail("final lifecycle verification timestamp is not canonical")
    else:
        if value.get("status") != worker_status:
            fail("worker artifact status is inconsistent with capacity observation")
        if lifecycle != {
            "guest_shutdown_guard_verified": True,
            "stop_responsibility": "external_orchestrator",
            "worker_mutates_gcp": False,
        }:
            fail("worker lifecycle contract is incomplete")


def validate_artifact_file(
    artifact_path: Path,
    *,
    expected_sha: str,
    log_dir: Path,
    allow_final: bool = False,
) -> dict[str, Any]:
    value = load_json(artifact_path, "qualification artifact")
    validate_artifact(
        value,
        expected_sha=expected_sha,
        log_dir=log_dir,
        allow_final=allow_final,
    )
    return value


def finalize_shutdown(
    artifact_path: Path,
    *,
    expected_sha: str,
    log_dir: Path,
    last_start_timestamp: str,
    verified_at_utc: str,
    output_path: Path,
) -> None:
    value = validate_artifact_file(
        artifact_path,
        expected_sha=expected_sha,
        log_dir=log_dir,
    )
    capacity_exhausted = value["checks"]["capacity_exhausted_observed"]
    value["status"] = (
        "capacity_exhausted_shutdown_certified"
        if capacity_exhausted
        else "passed_shutdown_certified"
    )
    value["vm_lifecycle"] = {
        "guest_shutdown_guard_verified": True,
        "last_start_timestamp": last_start_timestamp,
        "stop_responsibility": "fulfilled_by_external_orchestrator",
        "target_status": "TERMINATED",
        "targeted_stop_verified_at_utc": verified_at_utc,
        "worker_mutates_gcp": False,
    }
    validate_artifact(
        value,
        expected_sha=expected_sha,
        log_dir=log_dir,
        allow_final=True,
    )
    encoded = json.dumps(
        value, ensure_ascii=False, sort_keys=True, separators=(",", ":")
    ) + "\n"
    descriptor = os.open(output_path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as stream:
            stream.write(encoded)
            stream.flush()
            os.fsync(stream.fileno())
    except BaseException:
        try:
            output_path.unlink()
        except OSError:
            pass
        raise


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--git-sha", required=True)
    parser.add_argument("--image-id", required=True)
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--container-binary", required=True)
    parser.add_argument("--chord-capacity", required=True, type=int)
    parser.add_argument("--log-dir", required=True, type=Path)
    parser.add_argument("--oracle-exit-status", required=True, type=int)
    parser.add_argument("--uniform-exit-status", required=True, type=int)
    parser.add_argument("--clusters-exit-status", required=True, type=int)
    parser.add_argument("--output", required=True, type=Path)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if GIT_SHA_RE.fullmatch(args.git_sha) is None:
        fail("--git-sha must be canonical")
    if re.fullmatch(r"sha256:[0-9a-f]{64}", args.image_id) is None:
        fail("--image-id must be canonical")
    if args.chord_capacity <= 0:
        fail("--chord-capacity must be positive")
    require_regular(args.binary, "qualification binary", executable=True)
    for name in LOG_NAMES:
        require_regular(args.log_dir / name, f"log {name}")

    oracle = load_json(args.log_dir / "oracle32.stdout.json", "oracle32 stdout")
    uniform = load_json(
        args.log_dir / "uniform-50000.stdout.json", "uniform 50k stdout"
    )
    clusters = load_json(
        args.log_dir / "eight-clusters-50000.stdout.json",
        "eight-clusters 50k stdout",
    )
    for stderr_name in (
        "oracle32.stderr.log",
        "uniform-50000.stderr.log",
        "eight-clusters-50000.stderr.log",
    ):
        if (args.log_dir / stderr_name).read_bytes():
            fail(f"application stderr must be empty: {stderr_name}")

    run_specs = (
        (oracle, "uniform_latin", 32, 31, True, 32768, args.oracle_exit_status),
        (
            uniform,
            "uniform_latin",
            50000,
            8,
            False,
            args.chord_capacity,
            args.uniform_exit_status,
        ),
        (
            clusters,
            "eight_clusters",
            50000,
            8,
            False,
            args.chord_capacity,
            args.clusters_exit_status,
        ),
    )
    statuses = [
        validate_tool_result(
            result,
            expected_sha=args.git_sha,
            family=family,
            point_count=point_count,
            morton_window=window,
            oracle=is_oracle,
            chord_capacity=capacity,
            exit_status=exit_status,
        )
        for result, family, point_count, window, is_oracle, capacity, exit_status
        in run_specs
    ]
    validate_memcheck(args.log_dir / "oracle32.memcheck.log")
    capacity_exhausted = "capacity_exhausted" in statuses
    commands = expected_commands(args.container_binary, args.chord_capacity)
    logs = {name: sha256_file(args.log_dir / name) for name in LOG_NAMES}
    artifact: dict[str, Any] = {
        "backend": BACKEND,
        "execution_backend": EXECUTION_BACKEND,
        "binary": {
            "container_path": args.container_binary,
            "sha256": sha256_file(args.binary),
        },
        "checks": {
            "capacity_exhausted_observed": capacity_exhausted,
            "compute_sanitizer_oracle32": "passed",
            "fixed_run_count": 3,
            "intermediate_point_counts_executed": [],
            "oracle32": "passed",
            "targeted_build": "passed",
        },
        "claims": {
            "anchor_completeness_claimed": False,
            "exact_hierarchy_claimed": False,
            "product_claimed": False,
            "public_status_claimed": False,
            "scalability_claimed": False,
            "scientific_result_claimed": False,
            "slo_claimed": False,
            "warm_e2e_slo_claimed": False,
        },
        "component_only": True,
        "configuration": {
            "chord_capacity": args.chord_capacity,
            "maximum_closed_rank": 11,
            "morton_window_50k": 8,
            "oracle_chord_capacity": 32768,
            "oracle_point_count": 32,
            "point_count_50k": 50000,
            "support_size": 4,
            "timeout_seconds_50k": SCALE_50K_TIMEOUT_SECONDS,
            "timeout_seconds_oracle": ORACLE_TIMEOUT_SECONDS,
        },
        "deployment_status": "prototype_only",
        "git": {"clean": True, "sha": args.git_sha},
        "image": {
            "base_ref": BASE_IMAGE_REF,
            "id": args.image_id,
            "ref": f"morsehgp3d-phase15-jung:{args.git_sha}",
        },
        "logs": logs,
        "mode": MODE,
        "native_q3_gate_closed": False,
        "native_q4_gate_closed": False,
        "output_capacity_bounded": True,
        "phase": PHASE,
        "profile": PROFILE,
        "public_status": "not_claimed",
        "runs": [
            {
                "command": commands[0],
                "exit_status": args.oracle_exit_status,
                "name": "oracle32_memcheck",
                "result": oracle,
                "timeout_seconds": ORACLE_TIMEOUT_SECONDS,
            },
            {
                "command": commands[1],
                "exit_status": args.uniform_exit_status,
                "name": "uniform_latin_50000",
                "result": uniform,
                "timeout_seconds": SCALE_50K_TIMEOUT_SECONDS,
            },
            {
                "command": commands[2],
                "exit_status": args.clusters_exit_status,
                "name": "eight_clusters_50000",
                "result": clusters,
                "timeout_seconds": SCALE_50K_TIMEOUT_SECONDS,
            },
        ],
        "schema": ARTIFACT_SCHEMA,
        "scientific_result_claimed": False,
        "slo_eligible": False,
        "status": (
            "worker_observed_capacity_exhausted_pending_shutdown"
            if capacity_exhausted
            else "worker_passed_pending_shutdown"
        ),
        "unbudgeted_profile": True,
        "vm_lifecycle": {
            "guest_shutdown_guard_verified": True,
            "stop_responsibility": "external_orchestrator",
            "worker_mutates_gcp": False,
        },
    }
    validate_artifact(
        artifact,
        expected_sha=args.git_sha,
        log_dir=args.log_dir,
        allow_final=False,
    )
    if args.output.exists() or args.output.is_symlink():
        fail("--output must not exist")
    encoded = json.dumps(
        artifact, ensure_ascii=False, sort_keys=True, separators=(",", ":")
    ) + "\n"
    descriptor = os.open(args.output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as stream:
            stream.write(encoded)
            stream.flush()
            os.fsync(stream.fileno())
    except BaseException:
        try:
            args.output.unlink()
        except OSError:
            pass
        raise


if __name__ == "__main__":
    main()
