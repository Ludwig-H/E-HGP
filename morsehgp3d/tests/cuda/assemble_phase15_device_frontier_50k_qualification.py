#!/usr/bin/env python3
"""Validate and assemble the guarded Phase 15 non-smoke 50k evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
from typing import Any, NoReturn, Sequence

from assemble_phase7_h_polytope_qualification import (
    BASE_IMAGE_REF,
    IMAGE_ID_RE,
    PHASE3_SCHEMA,
    SHA_RE,
    WORKER_LIFECYCLE,
    parse_json_object,
    read_text_evidence,
    require_exact_keys,
    sha256_file,
    validate_environment,
    write_exclusive_atomic,
)

SCHEMA = (
    "morsehgp3d.phase15.morton_yao48_device_tiled_pair_frontier_"
    "50k_cuda_g4_qualification.v1"
)
QUALIFICATION_SCHEMA = (
    "morsehgp3d.phase15.morton_yao48_device_tiled_pair_frontier_"
    "qualification.v1"
)
TIMING_SCHEMA = "morsehgp3d.phase15.device_frontier_50k_timing.v1"
ARTIFACT_ROLE = "device_tiled_pair_frontier_50k_component_qualification"
BACKEND = "cuda_g4"
PROFILE = "hgp_reduced"
MODE = "offline_device_tiled_pair_frontier_qualification"
DEPLOYMENT_STATUS = "component_only"
PUBLIC_STATUS = "not_claimed"
SCIENTIFIC_SCOPE = "complete_50k_rank11_device_frontier_component_coverage_only"
POINT_COUNT = 50_000
MAXIMUM_CLOSED_RANK = 11
ANCHOR_TILE_CAPACITY = 4_096
SAMPLED_PRUNE_LIMIT = 4_096
EXACT_PRUNE_TARGET_CHECK_LIMIT = 1_000_000
SEED = 1_558_325_537_444_281_125
FAMILY = "adversarial_mixed_dyadic"
UNORDERED_PAIR_UNIVERSE = POINT_COUNT * (POINT_COUNT - 1) // 2
BINARY_RELATIVE_PATH = (
    "build/morsehgp3d-cuda-release/"
    "morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification"
)
BINARY_CONTAINER_PATH = f"/workspace/repository/{BINARY_RELATIVE_PATH}"
QUALIFICATION_COMMAND = [
    BINARY_CONTAINER_PATH,
    "--point-count",
    str(POINT_COUNT),
    "--family",
    FAMILY,
    "--maximum-closed-rank",
    str(MAXIMUM_CLOSED_RANK),
    "--anchor-tile-capacity",
    str(ANCHOR_TILE_CAPACITY),
    "--seed",
    str(SEED),
]

QUALIFICATION_KEYS = {
    "all_rank_thresholds_exercised",
    "anchor_tile_capacity",
    "backend",
    "canonicalization_ns",
    "cloud_digest_fnv1a",
    "component_only",
    "coverage_partition_complete",
    "delaunay_materialized",
    "dense_pair_fallback_performed",
    "deployment_status",
    "equality_shell_point_count",
    "exact_prune_target_check_limit",
    "family",
    "generation_ns",
    "git_sha",
    "global_pair_matrix_materialized",
    "guard_size_at_most_50000",
    "guard_size_passed",
    "higher_order_structure_materialized",
    "jitter_grid_point_count",
    "mode",
    "ordinary_delaunay_materialized",
    "ordinary_emst_computed",
    "peak_host_rss_bytes",
    "permutation_sign_point_count",
    "point_count",
    "profile",
    "profile_exit_success",
    "public_status",
    "rank_results",
    "required_rank_triplet_exercised",
    "sampled_prune_limit",
    "schema",
    "scientific_pair_catalog_published",
    "seed",
    "success",
    "tight_cluster_point_count",
    "total_ns",
}
RANK_KEYS = {
    "bank_entry_count",
    "bounded_admitted_pair_count",
    "bounded_bruteforce_ns",
    "bounded_bruteforce_performed",
    "bounded_candidate_admitted_pair_count",
    "bounded_candidate_rejected_pair_count",
    "bounded_exact_pair_count",
    "bounded_non_support_shell_equality_count",
    "build_ns",
    "candidate_pair_mass",
    "candidate_record_count",
    "censored_anchor_count",
    "certified_pruned_pair_mass",
    "complete_anchor_count",
    "component_contract_validated",
    "coverage_partition_complete",
    "cpu_recertification_ns",
    "device_total_bytes",
    "every_prune_fully_recertified",
    "fully_recertified_prune_count",
    "gamma2_prune_or_discard_authorized",
    "gamma2_silent_handoff_required",
    "launcher_ns",
    "lease_release_ns",
    "maximum_closed_rank",
    "minimum_device_free_bytes",
    "node_copy_ns",
    "output_digest_fnv1a",
    "peak_tile_output_device_capacity_bytes",
    "physical_node_visit_count",
    "prune_region_count",
    "prune_semantics",
    "prune_witness_target_check_count",
    "q3_exact_diametral_pair_support_gabriel_negative_only",
    "qualification_device_to_host_bytes",
    "qualification_output_copy_ns",
    "required_witness_count",
    "sampled_recertified_prune_count",
    "tile_count",
    "traversal_adoption_ns",
    "traversal_device_capacity_bytes",
    "unbanked_candidate_count",
    "unordered_pair_universe_count",
    "unresolved_pair_mass",
}
TIMING_KEYS = {
    "command",
    "elapsed_wall_ns",
    "exit_status",
    "finished_epoch_ns",
    "schema",
    "started_epoch_ns",
}
LOG_KEYS = {"build", "qualification", "qualification_stderr", "timing"}
CHECK_KEYS = {
    "build",
    "cuda_audit_workflow",
    "cuda_release_workflow",
    "qualification",
    "qualification_stderr_empty",
    "timed_exit_status",
}
CLAIM_KEYS = {
    "exact_hierarchy_claimed",
    "exact_pair_catalog_claimed",
    "product_claimed",
    "public_status_claimed",
    "scalability_claimed",
    "scientific_result_claimed",
    "slo_claimed",
    "warm_e2e_slo_claimed",
}
NO_CLAIMS = {key: False for key in CLAIM_KEYS}
ARTIFACT_KEYS = {
    "artifact_role",
    "backend",
    "binary",
    "checks",
    "claims",
    "command",
    "component_only",
    "deployment_status",
    "git",
    "image",
    "log_sha256",
    "logs",
    "mode",
    "phase",
    "profile",
    "provenance",
    "schema",
    "scientific_public_status",
    "scientific_result_claimed",
    "scientific_scope",
    "status",
    "timing",
    "vm_lifecycle",
}
SHA256_RE = re.compile(r"[0-9a-f]{64}\Z")
BUILD_FAILURE_RE = re.compile(
    r"(?:^|\n)(?:FAILED:|ninja: build stopped:|CMake Error(?: at|:)|"
    r"[^\n]*(?:fatal error:|undefined reference to|collect2: error:|"
    r"ld(?:\.lld)?: error:))",
    re.IGNORECASE,
)


def fail(message: str) -> NoReturn:
    raise SystemExit(message)


def canonical_json(value: dict[str, Any]) -> str:
    return (
        json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
        + "\n"
    )


def compact_json(value: dict[str, Any]) -> str:
    return json.dumps(
        value, ensure_ascii=False, sort_keys=False, separators=(",", ":")
    ) + "\n"


def require_boolean(value: Any, expected: bool, label: str) -> None:
    if type(value) is not bool or value is not expected:
        fail(f"{label} must be {str(expected).lower()}")


def require_integer(value: Any, label: str, *, minimum: int = 0) -> int:
    if type(value) is not int or value < minimum:
        fail(f"{label} must be an integer >= {minimum}")
    return value


def require_exact_integer(value: Any, expected: int, label: str) -> None:
    if type(value) is not int or value != expected:
        fail(f"{label} must be exactly {expected}")


def require_sha256(value: Any, label: str) -> str:
    if not isinstance(value, str) or SHA256_RE.fullmatch(value) is None:
        fail(f"{label} must be a canonical SHA-256 digest")
    return value


def parse_single_line_json(raw: str, label: str) -> dict[str, Any]:
    lines = raw.splitlines()
    if len(lines) != 1 or not lines[0] or raw != lines[0] + "\n":
        fail(f"{label} must be one non-empty JSON line terminated by LF")
    value = parse_json_object(raw, label)
    if raw != compact_json(value):
        fail(f"{label} must be compact JSON without insignificant whitespace")
    return value


def validate_build_log(raw: str) -> None:
    if not raw.strip():
        fail("target build log must be non-empty")
    if BUILD_FAILURE_RE.search(raw) is not None:
        fail("target build log contains an obvious failure")


def validate_timing(value: dict[str, Any]) -> dict[str, Any]:
    require_exact_keys(value, TIMING_KEYS, "qualification timing")
    if value.get("schema") != TIMING_SCHEMA:
        fail("qualification timing schema differs")
    if value.get("command") != QUALIFICATION_COMMAND:
        fail("qualification timing command differs from the guarded command")
    started = require_integer(
        value.get("started_epoch_ns"), "timing started_epoch_ns", minimum=1
    )
    finished = require_integer(
        value.get("finished_epoch_ns"), "timing finished_epoch_ns", minimum=1
    )
    elapsed = require_integer(
        value.get("elapsed_wall_ns"), "timing elapsed_wall_ns", minimum=1
    )
    require_exact_integer(value.get("exit_status"), 0, "timing exit_status")
    if finished <= started or finished - started != elapsed:
        fail("qualification timing interval does not close")
    return value


def validate_rank(value: dict[str, Any]) -> dict[str, Any]:
    require_exact_keys(value, RANK_KEYS, "rank-11 qualification result")
    exact_integers = {
        "bounded_admitted_pair_count": 0,
        "bounded_bruteforce_ns": 0,
        "bounded_candidate_admitted_pair_count": 0,
        "bounded_candidate_rejected_pair_count": 0,
        "bounded_exact_pair_count": 0,
        "bounded_non_support_shell_equality_count": 0,
        "censored_anchor_count": 0,
        "complete_anchor_count": POINT_COUNT,
        "maximum_closed_rank": MAXIMUM_CLOSED_RANK,
        "required_witness_count": MAXIMUM_CLOSED_RANK - 1,
        "tile_count": 13,
        "unordered_pair_universe_count": UNORDERED_PAIR_UNIVERSE,
        "unresolved_pair_mass": 0,
    }
    for field, expected in exact_integers.items():
        require_exact_integer(value.get(field), expected, f"rank result.{field}")
    require_boolean(
        value.get("bounded_bruteforce_performed"),
        False,
        "rank result.bounded_bruteforce_performed",
    )
    for field in ("component_contract_validated", "coverage_partition_complete"):
        require_boolean(value.get(field), True, f"rank result.{field}")
    require_boolean(
        value.get("every_prune_fully_recertified"),
        value.get("fully_recertified_prune_count") == value.get("prune_region_count"),
        "rank result.every_prune_fully_recertified",
    )
    for field in (
        "gamma2_prune_or_discard_authorized",
        "gamma2_silent_handoff_required",
        "q3_exact_diametral_pair_support_gabriel_negative_only",
    ):
        require_boolean(value.get(field), False, f"rank result.{field}")
    if value.get("prune_semantics") != "closed_rank_window":
        fail("rank result.prune_semantics must be closed_rank_window")
    for field in RANK_KEYS - {
        "bounded_bruteforce_performed",
        "component_contract_validated",
        "coverage_partition_complete",
        "every_prune_fully_recertified",
        "gamma2_prune_or_discard_authorized",
        "gamma2_silent_handoff_required",
        "prune_semantics",
        "q3_exact_diametral_pair_support_gabriel_negative_only",
    }:
        require_integer(value.get(field), f"rank result.{field}")
    if value["candidate_record_count"] != value["candidate_pair_mass"]:
        fail("rank candidate record and pair masses differ")
    if (
        value["candidate_pair_mass"] + value["certified_pruned_pair_mass"]
        != UNORDERED_PAIR_UNIVERSE
    ):
        fail("rank candidate/pruned mass does not close the 50k universe")
    if value["fully_recertified_prune_count"] > value["prune_region_count"]:
        fail("fully recertified prune count exceeds the prune count")
    if value["sampled_recertified_prune_count"] > value["prune_region_count"]:
        fail("sampled recertified prune count exceeds the prune count")
    if not 0 < value["minimum_device_free_bytes"] <= value["device_total_bytes"]:
        fail("rank device memory receipt is invalid")
    return value


def validate_qualification(
    value: dict[str, Any], *, git_sha: str
) -> dict[str, Any]:
    require_exact_keys(value, QUALIFICATION_KEYS, "50k qualification result")
    expected_scalars = {
        "backend": BACKEND,
        "deployment_status": DEPLOYMENT_STATUS,
        "family": FAMILY,
        "git_sha": git_sha,
        "mode": MODE,
        "profile": PROFILE,
        "public_status": PUBLIC_STATUS,
        "schema": QUALIFICATION_SCHEMA,
    }
    for field, expected in expected_scalars.items():
        if value.get(field) != expected:
            fail(f"50k qualification result.{field} must be {expected}")
    exact_integers = {
        "anchor_tile_capacity": ANCHOR_TILE_CAPACITY,
        "exact_prune_target_check_limit": EXACT_PRUNE_TARGET_CHECK_LIMIT,
        "point_count": POINT_COUNT,
        "sampled_prune_limit": SAMPLED_PRUNE_LIMIT,
        "seed": SEED,
    }
    for field, expected in exact_integers.items():
        require_exact_integer(value.get(field), expected, f"qualification {field}")
    for field in (
        "component_only",
        "coverage_partition_complete",
        "guard_size_at_most_50000",
        "guard_size_passed",
        "profile_exit_success",
        "success",
    ):
        require_boolean(value.get(field), True, f"qualification {field}")
    for field in (
        "all_rank_thresholds_exercised",
        "delaunay_materialized",
        "dense_pair_fallback_performed",
        "global_pair_matrix_materialized",
        "higher_order_structure_materialized",
        "ordinary_delaunay_materialized",
        "ordinary_emst_computed",
        "required_rank_triplet_exercised",
        "scientific_pair_catalog_published",
    ):
        require_boolean(value.get(field), False, f"qualification {field}")
    for field in (
        "canonicalization_ns",
        "cloud_digest_fnv1a",
        "generation_ns",
        "peak_host_rss_bytes",
        "total_ns",
    ):
        require_integer(value.get(field), f"qualification {field}", minimum=1)
    for field in (
        "equality_shell_point_count",
        "jitter_grid_point_count",
        "permutation_sign_point_count",
        "tight_cluster_point_count",
    ):
        require_integer(value.get(field), f"qualification {field}", minimum=1)
    ranks = value.get("rank_results")
    if not isinstance(ranks, list) or len(ranks) != 1 or not isinstance(ranks[0], dict):
        fail("50k qualification must contain exactly the explicit rank-11 result")
    validate_rank(ranks[0])
    return value


def _validate_environment_artifact(
    environment_artifact_path: Path,
    *,
    git_sha: str,
    base_image_ref: str,
    image_ref: str,
    image_id: str,
) -> str:
    raw, sha256 = read_text_evidence(
        environment_artifact_path, "Phase 3 environment artifact"
    )
    value = parse_json_object(raw, "Phase 3 environment artifact")
    if raw != canonical_json(value):
        fail("Phase 3 environment artifact must be canonical JSON")
    validate_environment(
        value,
        git_sha=git_sha,
        base_image_ref=base_image_ref,
        image_ref=image_ref,
        image_id=image_id,
    )
    return sha256


def validate_artifact(
    value: dict[str, Any],
    raw: str,
    *,
    git_sha: str,
    environment_artifact_path: Path,
) -> dict[str, Any]:
    if SHA_RE.fullmatch(git_sha) is None:
        fail("git_sha must be a canonical lowercase 40-hex commit ID")
    require_exact_keys(value, ARTIFACT_KEYS, "Phase 15 50k artifact")
    if raw != canonical_json(value):
        fail("Phase 15 50k artifact must be canonical JSON")
    expected_scalars = {
        "artifact_role": ARTIFACT_ROLE,
        "backend": BACKEND,
        "deployment_status": DEPLOYMENT_STATUS,
        "mode": MODE,
        "phase": "15",
        "profile": PROFILE,
        "schema": SCHEMA,
        "scientific_scope": SCIENTIFIC_SCOPE,
        "status": "worker_passed_pending_shutdown",
    }
    for field, expected in expected_scalars.items():
        if value.get(field) != expected:
            fail(f"Phase 15 50k artifact.{field} must be {expected}")
    require_boolean(value.get("component_only"), True, "component_only")
    require_boolean(
        value.get("scientific_result_claimed"), False, "scientific_result_claimed"
    )
    if value.get("scientific_public_status") is not None:
        fail("scientific_public_status must be null")
    if value.get("claims") != NO_CLAIMS:
        fail("Phase 15 50k artifact must not publish product or exactness claims")
    if value.get("command") != QUALIFICATION_COMMAND:
        fail("Phase 15 50k artifact command differs from the guarded command")
    if value.get("git") != {"clean": True, "sha": git_sha}:
        fail("Phase 15 50k artifact does not bind the clean Git SHA")

    image = value.get("image")
    if not isinstance(image, dict):
        fail("Phase 15 50k artifact image is absent")
    require_exact_keys(image, {"base_ref", "id", "ref"}, "image identity")
    if image.get("base_ref") != BASE_IMAGE_REF:
        fail("Phase 15 50k base image is not pinned")
    if image.get("ref") != f"morsehgp3d-phase3:{git_sha}":
        fail("Phase 15 50k image ref is not tied to the Git SHA")
    if not isinstance(image.get("id"), str) or IMAGE_ID_RE.fullmatch(image["id"]) is None:
        fail("Phase 15 50k image ID is not canonical")
    environment_sha256 = _validate_environment_artifact(
        environment_artifact_path,
        git_sha=git_sha,
        base_image_ref=image["base_ref"],
        image_ref=image["ref"],
        image_id=image["id"],
    )
    expected_provenance = {
        "environment_artifact_schema": PHASE3_SCHEMA,
        "environment_artifact_sha256": environment_sha256,
        "qualified_binary_relative_path": BINARY_RELATIVE_PATH,
    }
    if value.get("provenance") != expected_provenance:
        fail("Phase 15 50k provenance differs")
    if value.get("vm_lifecycle") != WORKER_LIFECYCLE:
        fail("Phase 15 50k lifecycle is not pending targeted shutdown")

    binary = value.get("binary")
    if not isinstance(binary, dict):
        fail("Phase 15 50k binary evidence is absent")
    require_exact_keys(binary, {"qualification_sha256"}, "binary evidence")
    require_sha256(binary.get("qualification_sha256"), "qualification binary")

    logs = value.get("logs")
    log_sha256 = value.get("log_sha256")
    if not isinstance(logs, dict) or not isinstance(log_sha256, dict):
        fail("Phase 15 50k logs or hashes are absent")
    require_exact_keys(logs, LOG_KEYS, "Phase 15 50k logs")
    require_exact_keys(log_sha256, LOG_KEYS, "Phase 15 50k log hashes")
    for name in LOG_KEYS:
        log = logs.get(name)
        if not isinstance(log, str):
            fail(f"Phase 15 50k log {name} must be text")
        if log_sha256.get(name) != hashlib.sha256(log.encode()).hexdigest():
            fail(f"Phase 15 50k log digest differs for {name}")
    validate_build_log(logs["build"])
    if logs["qualification_stderr"] != "":
        fail("Phase 15 50k qualification stderr must be empty")
    qualification = validate_qualification(
        parse_single_line_json(logs["qualification"], "50k qualification log"),
        git_sha=git_sha,
    )
    timing = validate_timing(
        parse_single_line_json(logs["timing"], "50k timing log")
    )
    if value.get("timing") != timing:
        fail("Phase 15 50k timing summary differs from its log")
    checks = value.get("checks")
    if not isinstance(checks, dict):
        fail("Phase 15 50k checks are absent")
    require_exact_keys(checks, CHECK_KEYS, "Phase 15 50k checks")
    expected_checks = {
        "build": "passed",
        "cuda_audit_workflow": "passed",
        "cuda_release_workflow": "passed",
        "qualification": qualification,
        "qualification_stderr_empty": True,
        "timed_exit_status": 0,
    }
    if checks != expected_checks:
        fail("Phase 15 50k checks differ from their embedded proofs")
    return value


def validate_artifact_file(
    artifact_path: Path,
    *,
    git_sha: str,
    environment_artifact_path: Path,
) -> dict[str, Any]:
    raw, _ = read_text_evidence(artifact_path, "Phase 15 50k artifact")
    value = parse_json_object(raw, "Phase 15 50k artifact")
    return validate_artifact(
        value,
        raw,
        git_sha=git_sha,
        environment_artifact_path=environment_artifact_path,
    )


def parse_arguments(arguments: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--git-sha", required=True)
    parser.add_argument("--base-image-ref", required=True)
    parser.add_argument("--image-ref", required=True)
    parser.add_argument("--image-id", required=True)
    parser.add_argument("--environment-artifact", type=Path, required=True)
    parser.add_argument("--build-log", type=Path, required=True)
    parser.add_argument("--qualification-log", type=Path, required=True)
    parser.add_argument("--qualification-stderr-log", type=Path, required=True)
    parser.add_argument("--timing-log", type=Path, required=True)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args(arguments)


def main(arguments: Sequence[str] | None = None) -> int:
    args = parse_arguments(arguments)
    if SHA_RE.fullmatch(args.git_sha) is None:
        fail("--git-sha must be a canonical lowercase 40-hex commit ID")
    if IMAGE_ID_RE.fullmatch(args.image_id) is None:
        fail("--image-id must be a canonical sha256 Docker image ID")
    if args.base_image_ref != BASE_IMAGE_REF:
        fail("--base-image-ref is not the pinned CUDA image")
    if args.image_ref != f"morsehgp3d-phase3:{args.git_sha}":
        fail("--image-ref is not tied to the qualified SHA")
    environment_sha256 = _validate_environment_artifact(
        args.environment_artifact,
        git_sha=args.git_sha,
        base_image_ref=args.base_image_ref,
        image_ref=args.image_ref,
        image_id=args.image_id,
    )
    evidence_specs = {
        "build": (args.build_log, "target build log", False),
        "qualification": (args.qualification_log, "qualification log", False),
        "qualification_stderr": (
            args.qualification_stderr_log,
            "qualification stderr log",
            True,
        ),
        "timing": (args.timing_log, "qualification timing log", False),
    }
    logs: dict[str, str] = {}
    log_sha256: dict[str, str] = {}
    for name, (path, label, allow_empty) in evidence_specs.items():
        logs[name], log_sha256[name] = read_text_evidence(
            path, label, allow_empty=allow_empty
        )
    validate_build_log(logs["build"])
    if logs["qualification_stderr"] != "":
        fail("qualification stderr must be empty")
    qualification = validate_qualification(
        parse_single_line_json(logs["qualification"], "50k qualification log"),
        git_sha=args.git_sha,
    )
    timing = validate_timing(
        parse_single_line_json(logs["timing"], "50k timing log")
    )
    artifact = {
        "artifact_role": ARTIFACT_ROLE,
        "backend": BACKEND,
        "binary": {
            "qualification_sha256": sha256_file(
                args.binary, "Phase 15 device-frontier 50k qualification binary"
            )
        },
        "checks": {
            "build": "passed",
            "cuda_audit_workflow": "passed",
            "cuda_release_workflow": "passed",
            "qualification": qualification,
            "qualification_stderr_empty": True,
            "timed_exit_status": 0,
        },
        "claims": dict(NO_CLAIMS),
        "command": list(QUALIFICATION_COMMAND),
        "component_only": True,
        "deployment_status": DEPLOYMENT_STATUS,
        "git": {"clean": True, "sha": args.git_sha},
        "image": {
            "base_ref": args.base_image_ref,
            "id": args.image_id,
            "ref": args.image_ref,
        },
        "log_sha256": log_sha256,
        "logs": logs,
        "mode": MODE,
        "phase": "15",
        "profile": PROFILE,
        "provenance": {
            "environment_artifact_schema": PHASE3_SCHEMA,
            "environment_artifact_sha256": environment_sha256,
            "qualified_binary_relative_path": BINARY_RELATIVE_PATH,
        },
        "schema": SCHEMA,
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "scientific_scope": SCIENTIFIC_SCOPE,
        "status": "worker_passed_pending_shutdown",
        "timing": timing,
        "vm_lifecycle": dict(WORKER_LIFECYCLE),
    }
    validate_artifact(
        artifact,
        canonical_json(artifact),
        git_sha=args.git_sha,
        environment_artifact_path=args.environment_artifact,
    )
    write_exclusive_atomic(args.output, artifact)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
