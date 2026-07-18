#!/usr/bin/env python3
"""Validate and assemble the bounded Phase 4 spatial G4 evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
from typing import Any, NoReturn


SCHEMA = "morsehgp3d.phase4.spatial_gpu_reference_and_lbvh_qualification.v3"
SUMMARY_SCHEMA = "morsehgp3d.phase4.spatial_gpu_differential.v1"
LBVH_SUMMARY_SCHEMA = "morsehgp3d.phase4.spatial_gpu_lbvh_differential.v2"
PHASE3_SCHEMA = "morsehgp3d.phase3.qualification.v1"
BASE_IMAGE_REF = (
    "nvidia/cuda:12.9.2-devel-ubuntu24.04@"
    "sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"
)
SHA_RE = re.compile(r"[0-9a-f]{40}\Z")
SHA256_RE = re.compile(r"[0-9a-f]{64}\Z")
IMAGE_ID_RE = re.compile(r"sha256:[0-9a-f]{64}\Z")
SM_RE = re.compile(r"sm_[0-9]+")
SANITIZER_ERROR_RE = re.compile(r"ERROR SUMMARY:\s*([0-9]+) errors")
FULL_CAMPAIGN_SIZES = list(range(1, 1001))
QUICK_CAMPAIGN_SIZES = [1, 2, 3, 4, 17, 257, 1000]
GPU_SPECIFIC_CASE_COUNT = 7
TARGET_CASE_COUNT = 6
REQUIRED_IEEE_COVERAGE = [
    "addition_only_overflow",
    "finite_subnormal_distance",
    "max_finite_query",
    "normal_subnormal_tie",
    "overflow_clamped_query",
    "subnormal_tie",
]
REQUIRED_PROJECTION_COVERAGE = [
    "exact",
    "overflow_clamped",
    "rounded",
    "underflow",
]
LBVH_TARGETED_CASE_COUNT = 13
LBVH_CHUNK_CASE_LIMIT = 128
LBVH_FULL_CASE_COUNT = 1013
LBVH_FULL_GPU_LAUNCH_COUNT = 2019
LBVH_FULL_INPUT_POINT_COUNT = 500550
LBVH_FULL_CHUNK_COUNT = 8
LBVH_QUICK_CASE_COUNT = 20
LBVH_QUICK_GPU_LAUNCH_COUNT = 33
LBVH_QUICK_INPUT_POINT_COUNT = 1334
LBVH_QUICK_CHUNK_COUNT = 1
LBVH_MAXIMUM_POINT_COUNT = 1000
REQUIRED_LBVH_DIRECTED_ENCLOSURE_COVERAGE = [
    "enclosed",
    "exact",
    "unsupported_range",
]
REQUIRED_LBVH_TARGETED_COVERAGE = [
    "addition_only_overflow",
    "cutoff_non_binary64",
    "cutoff_outside_binary64",
    "exact_tie",
    "exclusions",
    "maximum_finite",
    "negative_query_outside_binary64",
    "permuted_input",
    "query_non_binary64",
    "query_outside_binary64",
    "signed_subnormal",
    "singleton",
    "six_way_shell",
    "tri_partition",
]


def fail(message: str) -> NoReturn:
    raise SystemExit(message)


def reject_duplicate_json_keys(
    pairs: list[tuple[str, Any]],
) -> dict[str, Any]:
    value: dict[str, Any] = {}
    for key, item in pairs:
        if key in value:
            raise ValueError(f"duplicate JSON object key: {key}")
        value[key] = item
    return value


def read_json_object(path: Path, label: str) -> dict[str, Any]:
    try:
        with path.open(encoding="utf-8") as stream:
            value = json.load(stream, object_pairs_hook=reject_duplicate_json_keys)
    except (OSError, UnicodeError, json.JSONDecodeError, ValueError) as error:
        fail(f"cannot read {label} {path}: {error}")
    if not isinstance(value, dict):
        fail(f"{label} must be a JSON object")
    return value


def read_text(path: Path, label: str, *, allow_empty: bool = False) -> str:
    try:
        value = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        fail(f"cannot read {label} {path}: {error}")
    if not allow_empty and not value.strip():
        fail(f"{label} is empty")
    return value


def require_exact_keys(
    value: dict[str, Any], expected: set[str], label: str
) -> None:
    actual = set(value)
    if actual != expected:
        fail(
            f"{label} keys differ: missing={sorted(expected - actual)}, "
            f"unexpected={sorted(actual - expected)}"
        )


def require_bool(value: Any, expected: bool, label: str) -> None:
    if type(value) is not bool or value is not expected:
        fail(f"{label} must be {str(expected).lower()}")


def require_integer(value: Any, expected: int, label: str) -> None:
    if type(value) is not int or value != expected:
        fail(f"{label} must be exactly {expected}")


def validate_summary(
    value: dict[str, Any], *, quick: bool
) -> dict[str, Any]:
    label = "quick differential summary" if quick else "full differential summary"
    require_exact_keys(
        value,
        {
            "all_cases_passed",
            "campaign_complete",
            "campaign_sizes_checked",
            "case_count",
            "closed_ball_query_count",
            "cpu_exact_recertification_complete",
            "decision_semantics",
            "gpu_launch_count",
            "ieee_coverage",
            "projection_coverage",
            "proposal_semantics",
            "schema",
            "scope",
            "scientific_public_status",
            "scientific_result_claimed",
            "top_k_query_count",
        },
        label,
    )
    if value.get("schema") != SUMMARY_SCHEMA:
        fail(f"{label} has the wrong schema")
    expected_scope = "quick" if quick else "full"
    if value.get("scope") != expected_scope:
        fail(f"{label}.scope must be {expected_scope}")
    require_bool(value.get("all_cases_passed"), True, f"{label}.all_cases_passed")
    require_bool(
        value.get("campaign_complete"), not quick, f"{label}.campaign_complete"
    )
    require_bool(
        value.get("cpu_exact_recertification_complete"),
        True,
        f"{label}.cpu_exact_recertification_complete",
    )
    require_bool(
        value.get("scientific_result_claimed"),
        False,
        f"{label}.scientific_result_claimed",
    )
    if value.get("scientific_public_status") is not None:
        fail(f"{label}.scientific_public_status must be null")
    if value.get("decision_semantics") != "cpu_exact_all_points":
        fail(f"{label}.decision_semantics is not exact-all-points")
    if value.get("proposal_semantics") != "non_certifying_fp64":
        fail(f"{label}.proposal_semantics is not non-certifying FP64")
    if value.get("projection_coverage") != REQUIRED_PROJECTION_COVERAGE:
        fail(f"{label}.projection_coverage is incomplete")
    if value.get("ieee_coverage") != REQUIRED_IEEE_COVERAGE:
        fail(f"{label}.ieee_coverage is incomplete")

    expected_sizes = QUICK_CAMPAIGN_SIZES if quick else FULL_CAMPAIGN_SIZES
    if value.get("campaign_sizes_checked") != expected_sizes:
        fail(f"{label}.campaign_sizes_checked differs")
    expected_cases = (
        TARGET_CASE_COUNT + len(expected_sizes) + GPU_SPECIFIC_CASE_COUNT
    )
    for field in ("case_count", "closed_ball_query_count", "top_k_query_count"):
        require_integer(value.get(field), expected_cases, f"{label}.{field}")
    require_integer(
        value.get("gpu_launch_count"), 2 * expected_cases, f"{label}.gpu_launch_count"
    )
    return value


def validate_lbvh_summary(
    value: dict[str, Any], *, quick: bool, label: str
) -> dict[str, Any]:
    require_exact_keys(
        value,
        {
            "all_cases_passed",
            "bounded_protocol",
            "campaign_complete",
            "campaign_input_sha256",
            "campaign_result_sha256",
            "campaign_sizes_checked",
            "case_count",
            "certified_pruned_subtree_count",
            "chunk_case_limit",
            "chunk_count",
            "closed_ball_query_count",
            "cover_antichain_complete",
            "cpu_exact_recertification_complete",
            "decision_semantics",
            "directed_enclosure_coverage",
            "exact_partition_complete",
            "gpu_kernel_launch_count",
            "gpu_launch_count",
            "gpu_parallel_round_count",
            "gpu_peak_frontier_count",
            "gpu_processed_node_count",
            "gpu_traversal_round_count",
            "input_point_count",
            "maximum_point_count",
            "point_partition_complete",
            "proposal_semantics",
            "schema",
            "scope",
            "scientific_public_status",
            "scientific_result_claimed",
            "targeted_case_count",
            "targeted_coverage",
            "top_k_query_count",
        },
        label,
    )
    if value.get("schema") != LBVH_SUMMARY_SCHEMA:
        fail(f"{label} has the wrong schema")
    expected_scope = "quick" if quick else "full"
    if value.get("scope") != expected_scope:
        fail(f"{label}.scope must be {expected_scope}")
    for field in (
        "all_cases_passed",
        "bounded_protocol",
        "cover_antichain_complete",
        "cpu_exact_recertification_complete",
        "exact_partition_complete",
        "point_partition_complete",
    ):
        require_bool(value.get(field), True, f"{label}.{field}")
    require_bool(
        value.get("campaign_complete"),
        not quick,
        f"{label}.campaign_complete",
    )
    require_bool(
        value.get("scientific_result_claimed"),
        False,
        f"{label}.scientific_result_claimed",
    )
    if value.get("scientific_public_status") is not None:
        fail(f"{label}.scientific_public_status must be null")
    if value.get("decision_semantics") != (
        "cpu_exact_cover_and_leaf_recertification"
    ):
        fail(f"{label}.decision_semantics differs")
    if value.get("proposal_semantics") != (
        "gpu_resident_parallel_frontier_lbvh_strict_exterior_cover"
    ):
        fail(f"{label}.proposal_semantics differs")
    if value.get("directed_enclosure_coverage") != (
        REQUIRED_LBVH_DIRECTED_ENCLOSURE_COVERAGE
    ):
        fail(f"{label}.directed_enclosure_coverage is incomplete")
    if value.get("targeted_coverage") != REQUIRED_LBVH_TARGETED_COVERAGE:
        fail(f"{label}.targeted_coverage is incomplete")
    expected_sizes = QUICK_CAMPAIGN_SIZES if quick else FULL_CAMPAIGN_SIZES
    if value.get("campaign_sizes_checked") != expected_sizes:
        fail(f"{label}.campaign_sizes_checked differs")
    expected_case_count = LBVH_QUICK_CASE_COUNT if quick else LBVH_FULL_CASE_COUNT
    expected_gpu_launch_count = (
        LBVH_QUICK_GPU_LAUNCH_COUNT if quick else LBVH_FULL_GPU_LAUNCH_COUNT
    )
    expected_input_point_count = (
        LBVH_QUICK_INPUT_POINT_COUNT if quick else LBVH_FULL_INPUT_POINT_COUNT
    )
    expected_chunk_count = (
        LBVH_QUICK_CHUNK_COUNT if quick else LBVH_FULL_CHUNK_COUNT
    )
    for field in (
        "case_count",
        "closed_ball_query_count",
        "top_k_query_count",
    ):
        require_integer(value.get(field), expected_case_count, f"{label}.{field}")
    require_integer(
        value.get("gpu_launch_count"),
        expected_gpu_launch_count,
        f"{label}.gpu_launch_count",
    )
    require_integer(
        value.get("input_point_count"),
        expected_input_point_count,
        f"{label}.input_point_count",
    )
    require_integer(
        value.get("maximum_point_count"),
        LBVH_MAXIMUM_POINT_COUNT,
        f"{label}.maximum_point_count",
    )
    require_integer(
        value.get("chunk_case_limit"),
        LBVH_CHUNK_CASE_LIMIT,
        f"{label}.chunk_case_limit",
    )
    require_integer(
        value.get("chunk_count"),
        expected_chunk_count,
        f"{label}.chunk_count",
    )
    require_integer(
        value.get("targeted_case_count"),
        LBVH_TARGETED_CASE_COUNT,
        f"{label}.targeted_case_count",
    )
    for field in ("campaign_input_sha256", "campaign_result_sha256"):
        if not isinstance(value.get(field), str) or SHA256_RE.fullmatch(
            value[field]
        ) is None:
            fail(f"{label}.{field} must be lowercase hexadecimal SHA-256")
    prunes = value.get("certified_pruned_subtree_count")
    if type(prunes) is not int or prunes <= 0:
        fail(f"{label}.certified_pruned_subtree_count must be positive")
    parallel_counts: dict[str, int] = {}
    for field in (
        "gpu_kernel_launch_count",
        "gpu_parallel_round_count",
        "gpu_peak_frontier_count",
        "gpu_processed_node_count",
        "gpu_traversal_round_count",
    ):
        count = value.get(field)
        if type(count) is not int or count <= 0:
            fail(f"{label}.{field} must be a positive integer")
        parallel_counts[field] = count
    if parallel_counts["gpu_kernel_launch_count"] != parallel_counts[
        "gpu_traversal_round_count"
    ]:
        fail(f"{label} kernel/traversal round counts differ")
    if parallel_counts["gpu_parallel_round_count"] > parallel_counts[
        "gpu_traversal_round_count"
    ]:
        fail(f"{label} parallel rounds exceed traversal rounds")
    if parallel_counts["gpu_traversal_round_count"] > parallel_counts[
        "gpu_processed_node_count"
    ]:
        fail(f"{label} traversal rounds exceed processed nodes")
    if not 1 < parallel_counts["gpu_peak_frontier_count"] <= (
        2 * LBVH_MAXIMUM_POINT_COUNT - 1
    ):
        fail(f"{label}.gpu_peak_frontier_count is outside tree bounds")
    maximum_processed_nodes = 2 * (
        2 * expected_input_point_count - expected_case_count
    )
    if not (
        expected_gpu_launch_count
        <= parallel_counts["gpu_processed_node_count"]
        <= maximum_processed_nodes
    ):
        fail(f"{label}.gpu_processed_node_count is outside campaign bounds")
    return value


def validate_sanitizer_log(value: str, *, label: str) -> None:
    sanitizer_errors = [
        int(match)
        for match in SANITIZER_ERROR_RE.findall(value)
    ]
    if not sanitizer_errors or any(count != 0 for count in sanitizer_errors):
        fail(f"{label} must contain only zero-error summaries")


def validate_binary_logs(
    logs: dict[str, str], *, label: str
) -> list[str]:
    architectures = SM_RE.findall(logs["cuobjdump_elf"])
    if not architectures or set(architectures) != {"sm_120"}:
        fail(f"{label} ELF evidence must contain only sm_120")
    if logs["cuobjdump_ptx"].strip():
        fail(f"{label} PTX evidence must contain zero entries")
    validate_sanitizer_log(
        logs["compute_sanitizer"], label=f"{label} compute-sanitizer evidence"
    )
    return sorted(set(architectures))


def validate_phase3_environment(
    value: dict[str, Any],
    *,
    git_sha: str,
    base_image_ref: str,
    image_ref: str,
    image_id: str,
) -> dict[str, Any]:
    if value.get("schema") != PHASE3_SCHEMA:
        fail("environment artifact has the wrong Phase 3 schema")
    expected_scalars = {
        "backend": "cuda_g4",
        "mode": "certified",
        "phase": "3",
        "profile": "hgp_reduced",
        "scientific_scope": "environment_reproducibility_only",
        "status": "worker_passed_pending_shutdown",
    }
    for field, expected in expected_scalars.items():
        if value.get(field) != expected:
            fail(f"environment artifact field {field} differs")
    require_bool(
        value.get("scientific_result_claimed"),
        False,
        "environment scientific_result_claimed",
    )
    if value.get("scientific_public_status") is not None:
        fail("environment scientific_public_status must be null")
    if "public_status" in value:
        fail("environment artifact must not expose public_status")
    if value.get("git") != {"clean": True, "sha": git_sha}:
        fail("environment artifact does not bind the qualified Git SHA")
    image = value.get("image")
    if not isinstance(image, dict):
        fail("environment artifact image is absent")
    if (
        image.get("base_ref") != base_image_ref
        or image.get("ref") != image_ref
        or image.get("id") != image_id
    ):
        fail("environment artifact image identity differs")
    records = value.get("runtime_records")
    if not isinstance(records, list) or not records:
        fail("environment artifact runtime_records is absent")
    first = records[0]
    if not isinstance(first, dict) or not isinstance(first.get("manifest"), dict):
        fail("environment artifact runtime manifest is absent")
    manifest = first["manifest"]
    if manifest.get("compiled_sm") != "sm_120" or manifest.get("gpu_runtime_sm") != "sm_120":
        fail("environment artifact is not compiled and executed on sm_120")
    if (
        manifest.get("gpu_compute_capability_major"),
        manifest.get("gpu_compute_capability_minor"),
    ) != (12, 0):
        fail("environment artifact compute capability differs from 12.0")
    return manifest


def sha256_file(path: Path, label: str) -> str:
    if not path.is_file() or path.is_symlink():
        fail(f"{label} must be a regular non-symbolic file")
    digest = hashlib.sha256()
    try:
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
    except OSError as error:
        fail(f"cannot hash {label}: {error}")
    return digest.hexdigest()


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--git-sha", required=True)
    parser.add_argument("--base-image-ref", required=True)
    parser.add_argument("--image-ref", required=True)
    parser.add_argument("--image-id", required=True)
    parser.add_argument("--environment-artifact", type=Path, required=True)
    parser.add_argument("--differential-summary", type=Path, required=True)
    parser.add_argument("--quick-summary", type=Path, required=True)
    parser.add_argument("--differential-log", type=Path, required=True)
    parser.add_argument("--elf-log", type=Path, required=True)
    parser.add_argument("--ptx-log", type=Path, required=True)
    parser.add_argument("--ptx-stderr-log", type=Path, required=True)
    parser.add_argument("--sanitizer-log", type=Path, required=True)
    parser.add_argument("--replay", type=Path, required=True)
    parser.add_argument("--checker", type=Path, required=True)
    parser.add_argument("--lbvh-differential-summary", type=Path, required=True)
    parser.add_argument("--lbvh-memcheck-summary", type=Path, required=True)
    parser.add_argument("--lbvh-differential-log", type=Path, required=True)
    parser.add_argument("--lbvh-elf-log", type=Path, required=True)
    parser.add_argument("--lbvh-ptx-log", type=Path, required=True)
    parser.add_argument("--lbvh-ptx-stderr-log", type=Path, required=True)
    parser.add_argument("--lbvh-sanitizer-log", type=Path, required=True)
    parser.add_argument("--lbvh-racecheck-log", type=Path, required=True)
    parser.add_argument("--lbvh-replay", type=Path, required=True)
    parser.add_argument("--lbvh-checker", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_arguments()
    if SHA_RE.fullmatch(args.git_sha) is None:
        fail("--git-sha is not canonical")
    if IMAGE_ID_RE.fullmatch(args.image_id) is None:
        fail("--image-id is not canonical")
    if args.base_image_ref != BASE_IMAGE_REF:
        fail("--base-image-ref is not the pinned CUDA image")
    if args.image_ref != f"morsehgp3d-phase3:{args.git_sha}":
        fail("--image-ref is not tied to the qualified SHA")

    environment = read_json_object(args.environment_artifact, "environment artifact")
    manifest = validate_phase3_environment(
        environment,
        git_sha=args.git_sha,
        base_image_ref=args.base_image_ref,
        image_ref=args.image_ref,
        image_id=args.image_id,
    )
    full_summary = validate_summary(
        read_json_object(args.differential_summary, "full differential summary"),
        quick=False,
    )
    quick_summary = validate_summary(
        read_json_object(args.quick_summary, "quick differential summary"),
        quick=True,
    )
    lbvh_summary = validate_lbvh_summary(
        read_json_object(
            args.lbvh_differential_summary,
            "resident LBVH differential summary",
        ),
        quick=False,
        label="resident LBVH differential summary",
    )
    lbvh_memcheck_summary = validate_lbvh_summary(
        read_json_object(
            args.lbvh_memcheck_summary,
            "resident LBVH memcheck summary",
        ),
        quick=True,
        label="resident LBVH memcheck summary",
    )
    if lbvh_summary["campaign_input_sha256"] == lbvh_memcheck_summary[
        "campaign_input_sha256"
    ]:
        fail("resident LBVH full and quick input roots must differ")
    if lbvh_summary["campaign_result_sha256"] == lbvh_memcheck_summary[
        "campaign_result_sha256"
    ]:
        fail("resident LBVH full and quick result roots must differ")
    reference_logs = {
        "compute_sanitizer": read_text(args.sanitizer_log, "compute-sanitizer log"),
        "cuobjdump_elf": read_text(args.elf_log, "cuobjdump ELF log"),
        "cuobjdump_ptx": read_text(
            args.ptx_log, "cuobjdump PTX log", allow_empty=True
        ),
        "cuobjdump_ptx_stderr": read_text(
            args.ptx_stderr_log, "cuobjdump PTX stderr", allow_empty=True
        ),
        "differential": read_text(args.differential_log, "differential log"),
    }
    lbvh_logs = {
        "compute_sanitizer": read_text(
            args.lbvh_sanitizer_log,
            "resident LBVH compute-sanitizer log",
        ),
        "cuobjdump_elf": read_text(
            args.lbvh_elf_log, "resident LBVH cuobjdump ELF log"
        ),
        "cuobjdump_ptx": read_text(
            args.lbvh_ptx_log,
            "resident LBVH cuobjdump PTX log",
            allow_empty=True,
        ),
        "cuobjdump_ptx_stderr": read_text(
            args.lbvh_ptx_stderr_log,
            "resident LBVH cuobjdump PTX stderr",
            allow_empty=True,
        ),
        "differential": read_text(
            args.lbvh_differential_log,
            "resident LBVH differential log",
        ),
    }
    lbvh_racecheck_log = read_text(
        args.lbvh_racecheck_log,
        "resident LBVH racecheck log",
    )
    reference_architectures = validate_binary_logs(
        reference_logs, label="exhaustive reference replay"
    )
    lbvh_architectures = validate_binary_logs(
        lbvh_logs, label="resident LBVH replay"
    )
    validate_sanitizer_log(
        lbvh_racecheck_log,
        label="resident LBVH racecheck evidence",
    )

    artifact = {
        "backend": "cuda_g4",
        "binary": {
            "checker_sha256": sha256_file(
                args.checker, "reference differential checker"
            ),
            "lbvh_checker_sha256": sha256_file(
                args.lbvh_checker, "resident LBVH differential checker"
            ),
            "lbvh_replay_sha256": sha256_file(
                args.lbvh_replay, "resident LBVH replay"
            ),
            "replay_sha256": sha256_file(
                args.replay, "reference spatial replay"
            ),
        },
        "checks": {
            "aot_elf_architectures": reference_architectures,
            "aot_ptx_entry_count": 0,
            "compute_sanitizer": "passed",
            "cpu_exact_recertification_complete": True,
            "cuda_audit_workflow": "passed",
            "cuda_release_workflow": "passed",
            "differential": full_summary,
            "lbvh_aot_elf_architectures": lbvh_architectures,
            "lbvh_aot_ptx_entry_count": 0,
            "lbvh_compute_sanitizer": "passed",
            "lbvh_cpu_exact_recertification_complete": True,
            "lbvh_differential": lbvh_summary,
            "lbvh_memcheck_differential": lbvh_memcheck_summary,
            "lbvh_racecheck": "passed",
            "quick_memcheck_differential": quick_summary,
        },
        "environment": {
            "compute_capability": "12.0",
            "driver_version": manifest.get("cuda_driver_version_string"),
            "gpu_name": manifest.get("gpu_name"),
            "gpu_uuid": manifest.get("gpu_uuid"),
            "gpu_vram_bytes": manifest.get("gpu_vram_bytes"),
        },
        "git": {"clean": True, "sha": args.git_sha},
        "image": {
            "base_ref": args.base_image_ref,
            "id": args.image_id,
            "ref": args.image_ref,
        },
        "logs": {
            **reference_logs,
            "lbvh_compute_sanitizer": lbvh_logs["compute_sanitizer"],
            "lbvh_cuobjdump_elf": lbvh_logs["cuobjdump_elf"],
            "lbvh_cuobjdump_ptx": lbvh_logs["cuobjdump_ptx"],
            "lbvh_cuobjdump_ptx_stderr": lbvh_logs["cuobjdump_ptx_stderr"],
            "lbvh_differential": lbvh_logs["differential"],
            "lbvh_racecheck": lbvh_racecheck_log,
        },
        "mode": "certified",
        "phase": "4",
        "profile": "hgp_reduced",
        "schema": SCHEMA,
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "scientific_scope": (
            "non_certifying_gpu_proposals_with_cpu_exact_reference_and_parallel_frontier_lbvh_recertification"
        ),
        "status": "worker_passed_pending_shutdown",
        "vm_lifecycle": {
            "guest_shutdown_guard_verified": True,
            "stop_responsibility": "external_orchestrator",
            "worker_mutates_gcp": False,
        },
    }
    output = args.output
    if not output.is_absolute():
        fail("--output must be absolute")
    if output.is_symlink() or not output.exists() or not output.is_file():
        fail("--output must be the regular temporary file reserved by the worker")
    encoded = json.dumps(
        artifact, ensure_ascii=False, sort_keys=True, separators=(",", ":")
    )
    try:
        with output.open("w", encoding="utf-8", newline="\n") as stream:
            stream.write(encoded)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
    except OSError as error:
        fail(f"cannot write qualification artifact {output}: {error}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
