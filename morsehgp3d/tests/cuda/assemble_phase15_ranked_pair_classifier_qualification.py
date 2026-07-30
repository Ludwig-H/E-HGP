#!/usr/bin/env python3
"""Validate and assemble the guarded Phase 15 rank-2/3 CUDA evidence."""

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
    validate_elf_log,
    validate_environment,
    validate_memcheck_log,
    validate_racecheck_log,
    write_exclusive_atomic,
)


SCHEMA = (
    "morsehgp3d.phase15.morton_yao48_ranked_pair_tile_classifier_"
    "cuda_g4_qualification.v1"
)
QUALIFICATION_SCHEMA = (
    "morsehgp3d.phase15.morton_yao48_ranked_pair_tile_classifier_"
    "qualification.v1"
)
ARTIFACT_ROLE = "support2_closed_rank23_pair_catalog_component_qualification"
BACKEND = "cuda_g4"
PROFILE = "hgp_reduced"
MODE = "native_end_to_end_ranked_pair_catalog_qualification"
DEPLOYMENT_STATUS = "component_only"
PUBLIC_STATUS = "not_claimed"
SCIENTIFIC_SCOPE = "bounded_support2_closed_rank23_pair_catalog_component_only"
POINT_COUNT = 257
MAXIMUM_ORACLE_POINT_COUNT = 512
MAXIMUM_CLOSED_RANK = 3
ANCHOR_TILE_CAPACITY = 17
BINARY_RELATIVE_PATH = (
    "build/morsehgp3d-cuda-release/"
    "morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_qualification"
)
BINARY_CONTAINER_PATH = f"/workspace/repository/{BINARY_RELATIVE_PATH}"
QUALIFICATION_COMMAND = [BINARY_CONTAINER_PATH]
SANITIZER_APPLICATION_COMMAND = [BINARY_CONTAINER_PATH, "--single-run"]

QUALIFICATION_KEYS = {
    "anchor_tile_capacities",
    "backend",
    "bounded_cpu_oracle",
    "catalog_digest_fnv1a",
    "closed_ranks_qualified",
    "component_only",
    "deployment_status",
    "deterministic_dyadic_fixture",
    "exact_fallback_count",
    "exact_level_persisted_in_gpu_soa",
    "exact_levels_divide_by_four_recomputed",
    "exact_levels_match_cpu_oracle",
    "fixture_clustered_point_count",
    "fixture_exact_shell_point_count",
    "fixture_jitter_grid_point_count",
    "fixture_strict_collinear_point_count",
    "full_gamma2_claimed",
    "full_k2_hierarchy_claimed",
    "git_sha",
    "global_cell_or_coface_arena_materialized",
    "global_pair_matrix_materialized",
    "hierarchy_reduction_performed",
    "industrial_scale_claimed",
    "independent_all_pair_all_witness_oracle",
    "intermediate_candidate_d2h",
    "maximum_closed_rank",
    "maximum_oracle_point_count",
    "mode",
    "morse_hgp_hierarchy_claimed",
    "ordinary_delaunay_materialized",
    "point_count",
    "profile",
    "public_pipeline_lbvh_frontier_classifier_finish",
    "public_terminal_wrapper_and_lease_lifetime_qualified",
    "public_status",
    "qualification_final_catalog_and_source_d2h",
    "qualification_capacity_source",
    "rank_three_record_count",
    "rank_two_record_count",
    "requested_orders",
    "run_count",
    "schema",
    "shell_payload_point_id_count",
    "single_run",
    "strict_payload_point_id_count",
    "success",
    "support2_rank23_catalog_qualified",
    "tile_capacity_invariance_qualified",
    "total_commit_count",
    "zero_fallback_required_for_success",
}
LOG_KEYS = {
    "audit_build",
    "cuobjdump_elf",
    "cuobjdump_ptx",
    "cuobjdump_ptx_stderr",
    "memcheck",
    "qualification",
    "qualification_stderr",
    "racecheck",
    "release_build",
}
CHECK_KEYS = {
    "aot_elf_architectures",
    "aot_ptx_entry_count",
    "audit_build",
    "cuda_audit_workflow",
    "cuda_release_workflow",
    "memcheck",
    "memcheck_qualification",
    "qualification",
    "racecheck",
    "racecheck_qualification",
    "release_build",
    "sanitizer_scientific_catalog_equivalence",
}
COMMAND_KEYS = {"memcheck_application", "qualification", "racecheck_application"}
CLAIM_KEYS = {
    "full_gamma2",
    "full_k2_hierarchy",
    "hierarchy_reduction_performed",
    "industrial_scale",
    "morse_hgp_hierarchy",
    "public_status_claimed",
    "slo_claimed",
}
NO_CLAIMS = {key: False for key in CLAIM_KEYS}
PHASE_CONTEXT = {
    "backend": "reference_cpu",
    "deployment_status": "architecture_only",
    "mode": "budgeted",
    "profile": PROFILE,
    "public_status": PUBLIC_STATUS,
}
ARTIFACT_KEYS = {
    "artifact_role",
    "backend",
    "binary",
    "checks",
    "claims",
    "commands",
    "component_only",
    "deployment_status",
    "git",
    "image",
    "log_sha256",
    "logs",
    "mode",
    "phase",
    "phase_context",
    "profile",
    "provenance",
    "schema",
    "scientific_public_status",
    "scientific_result_claimed",
    "scientific_scope",
    "status",
    "vm_lifecycle",
}
SHA256_RE = re.compile(r"[0-9a-f]{64}\Z")
BUILD_FAILURE_RE = re.compile(
    r"(?:^|\n)(?:FAILED:|ninja: build stopped:|CMake Error(?: at|:)|"
    r"[^\n]*(?:fatal error:|undefined reference to|collect2: error:|"
    r"ld(?:\.lld)?: error:))",
    re.IGNORECASE,
)
QUALIFICATION_INFRASTRUCTURE_STDERR_RE = re.compile(
    r"\[TIMEOUT\] unité=phase15-ranked-pair-classifier-qualification, "
    r"borne douce=[1-9][0-9]*s, kill-after=5s, "
    r"réserve post-timeout=60s\.\n"
    r"[0-9a-f]{64}\n\Z"
)


def fail(message: str) -> NoReturn:
    raise SystemExit(message)


def canonical_json(value: dict[str, Any]) -> str:
    return (
        json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
        + "\n"
    )


def require_boolean(value: Any, expected: bool, label: str) -> None:
    if type(value) is not bool or value is not expected:
        fail(f"{label} must be {str(expected).lower()}")


def require_exact_integer(value: Any, expected: int, label: str) -> None:
    if type(value) is not int or value != expected:
        fail(f"{label} must be exactly {expected}")


def require_positive_integer(value: Any, label: str) -> int:
    if type(value) is not int or value <= 0:
        fail(f"{label} must be a positive integer")
    return value


def require_uint64(value: Any, label: str) -> int:
    if type(value) is not int or not 0 <= value <= (1 << 64) - 1:
        fail(f"{label} must be an unsigned 64-bit integer")
    return value


def require_sha256(value: Any, label: str) -> str:
    if not isinstance(value, str) or SHA256_RE.fullmatch(value) is None:
        fail(f"{label} must be a canonical SHA-256 digest")
    return value


def parse_single_json_line(raw: str, label: str) -> dict[str, Any]:
    lines = raw.splitlines()
    if len(lines) != 1 or not lines[0]:
        fail(f"{label} must contain exactly one non-empty JSON line")
    return parse_json_object(lines[0], label)


def validate_build_log(value: str, label: str) -> None:
    if not value.strip():
        fail(f"{label} must be non-empty")
    if BUILD_FAILURE_RE.search(value) is not None:
        fail(f"{label} contains an obvious build failure")


def validate_qualification_infrastructure_stderr(value: str) -> None:
    """Accept only the guarded timeout receipt and targeted docker-rm CID."""

    if QUALIFICATION_INFRASTRUCTURE_STDERR_RE.fullmatch(value) is None:
        fail(
            "qualification stderr must contain exactly the guarded timeout "
            "receipt and one canonical removed-container ID"
        )


def validate_qualification(
    value: dict[str, Any],
    *,
    git_sha: str,
    single_run: bool,
) -> dict[str, Any]:
    """Validate the exact closed stdout schema of the native CUDA binary."""

    require_exact_keys(value, QUALIFICATION_KEYS, "rank-2/3 qualification")
    expected_scalars = {
        "backend": BACKEND,
        "deployment_status": DEPLOYMENT_STATUS,
        "git_sha": git_sha,
        "mode": MODE,
        "profile": PROFILE,
        "public_status": PUBLIC_STATUS,
        "qualification_capacity_source": "bounded_cpu_oracle_only",
        "schema": QUALIFICATION_SCHEMA,
    }
    for field, expected in expected_scalars.items():
        if value.get(field) != expected:
            fail(f"qualification {field} must be {expected}")

    require_exact_integer(value.get("point_count"), POINT_COUNT, "point_count")
    require_exact_integer(
        value.get("maximum_oracle_point_count"),
        MAXIMUM_ORACLE_POINT_COUNT,
        "maximum_oracle_point_count",
    )
    require_exact_integer(
        value.get("maximum_closed_rank"),
        MAXIMUM_CLOSED_RANK,
        "maximum_closed_rank",
    )
    expected_run_count = 1 if single_run else 2
    require_exact_integer(value.get("run_count"), expected_run_count, "run_count")
    expected_capacities = [ANCHOR_TILE_CAPACITY] if single_run else [1, ANCHOR_TILE_CAPACITY]
    if value.get("anchor_tile_capacities") != expected_capacities:
        fail("qualification anchor_tile_capacities differ from the guarded command")
    if value.get("requested_orders") != [1, 2]:
        fail("qualification requested_orders must be [1,2]")
    if value.get("closed_ranks_qualified") != [2, 3]:
        fail("qualification closed_ranks_qualified must be [2,3]")

    for field in (
        "bounded_cpu_oracle",
        "component_only",
        "deterministic_dyadic_fixture",
        "exact_levels_divide_by_four_recomputed",
        "exact_levels_match_cpu_oracle",
        "independent_all_pair_all_witness_oracle",
        "public_pipeline_lbvh_frontier_classifier_finish",
        "public_terminal_wrapper_and_lease_lifetime_qualified",
        "qualification_final_catalog_and_source_d2h",
        "success",
        "support2_rank23_catalog_qualified",
        "zero_fallback_required_for_success",
    ):
        require_boolean(value.get(field), True, field)
    for field in (
        "exact_level_persisted_in_gpu_soa",
        "full_gamma2_claimed",
        "full_k2_hierarchy_claimed",
        "global_cell_or_coface_arena_materialized",
        "global_pair_matrix_materialized",
        "hierarchy_reduction_performed",
        "industrial_scale_claimed",
        "intermediate_candidate_d2h",
        "morse_hgp_hierarchy_claimed",
        "ordinary_delaunay_materialized",
    ):
        require_boolean(value.get(field), False, field)
    require_boolean(value.get("single_run"), single_run, "single_run")
    require_boolean(
        value.get("tile_capacity_invariance_qualified"),
        not single_run,
        "tile_capacity_invariance_qualified",
    )
    require_exact_integer(value.get("exact_fallback_count"), 0, "exact_fallback_count")
    require_uint64(value.get("catalog_digest_fnv1a"), "catalog_digest_fnv1a")
    for field in (
        "fixture_clustered_point_count",
        "fixture_exact_shell_point_count",
        "fixture_jitter_grid_point_count",
        "fixture_strict_collinear_point_count",
        "rank_three_record_count",
        "rank_two_record_count",
        "shell_payload_point_id_count",
        "strict_payload_point_id_count",
        "total_commit_count",
    ):
        require_positive_integer(value.get(field), field)
    if value["fixture_strict_collinear_point_count"] < 3:
        fail("the strict rank-three fixture is incomplete")
    return value


def extract_sanitizer_qualification(
    raw: str,
    label: str,
    *,
    git_sha: str,
) -> dict[str, Any]:
    candidates = [line for line in raw.splitlines() if line.startswith("{")]
    if len(candidates) != 1:
        fail(f"{label} must contain exactly one qualification JSON line")
    return validate_qualification(
        parse_json_object(candidates[0], f"{label} qualification"),
        git_sha=git_sha,
        single_run=True,
    )


def validate_scientific_equivalence(
    direct: dict[str, Any],
    memcheck: dict[str, Any],
    racecheck: dict[str, Any],
) -> None:
    ignored = {
        "anchor_tile_capacities",
        "run_count",
        "single_run",
        "tile_capacity_invariance_qualified",
        "total_commit_count",
    }
    expected = {key: value for key, value in direct.items() if key not in ignored}
    for label, replay in (("memcheck", memcheck), ("racecheck", racecheck)):
        observed = {key: value for key, value in replay.items() if key not in ignored}
        if observed != expected:
            fail(f"{label} changed the qualified scientific catalogue")
    if memcheck["total_commit_count"] != racecheck["total_commit_count"]:
        fail("sanitizer runs disagree on the exact commit count")
    if direct["total_commit_count"] <= memcheck["total_commit_count"]:
        fail("the two-capacity run did not exercise both commit journals")


def validate_environment_artifact(
    environment_artifact_path: Path,
    *,
    git_sha: str,
    base_image_ref: str,
    image_ref: str,
    image_id: str,
) -> str:
    raw, digest = read_text_evidence(
        environment_artifact_path, "Phase 3 environment artifact"
    )
    value = parse_json_object(raw, "Phase 3 environment artifact")
    validate_environment(
        value,
        git_sha=git_sha,
        base_image_ref=base_image_ref,
        image_ref=image_ref,
        image_id=image_id,
    )
    return digest


def validate_artifact(
    value: dict[str, Any],
    raw: str,
    *,
    git_sha: str,
    environment_artifact_path: Path,
) -> dict[str, Any]:
    """Revalidate a pending-shutdown artifact and every embedded proof."""

    if SHA_RE.fullmatch(git_sha) is None:
        fail("git_sha must be a canonical lowercase 40-hex commit ID")
    require_exact_keys(value, ARTIFACT_KEYS, "rank-2/3 qualification artifact")
    if raw != canonical_json(value):
        fail("rank-2/3 qualification artifact must be canonical JSON")
    for field, expected in {
        "artifact_role": ARTIFACT_ROLE,
        "backend": BACKEND,
        "deployment_status": DEPLOYMENT_STATUS,
        "mode": MODE,
        "phase": "15",
        "profile": PROFILE,
        "schema": SCHEMA,
        "scientific_scope": SCIENTIFIC_SCOPE,
        "status": "worker_passed_pending_shutdown",
    }.items():
        if value.get(field) != expected:
            fail(f"artifact {field} must be {expected}")
    require_boolean(value.get("component_only"), True, "component_only")
    require_boolean(
        value.get("scientific_result_claimed"), False, "scientific_result_claimed"
    )
    if value.get("scientific_public_status") is not None:
        fail("scientific_public_status must be null")
    if value.get("phase_context") != PHASE_CONTEXT:
        fail("Phase 15 administrative context differs")
    if value.get("claims") != NO_CLAIMS:
        fail("the support-2 component must not publish a product claim")
    if value.get("commands") != {
        "memcheck_application": SANITIZER_APPLICATION_COMMAND,
        "qualification": QUALIFICATION_COMMAND,
        "racecheck_application": SANITIZER_APPLICATION_COMMAND,
    }:
        fail("the guarded qualification commands differ")
    if value.get("git") != {"clean": True, "sha": git_sha}:
        fail("artifact does not bind the clean qualified Git SHA")

    image = value.get("image")
    if not isinstance(image, dict):
        fail("artifact image identity is absent")
    require_exact_keys(image, {"base_ref", "id", "ref"}, "image identity")
    base_image_ref = image.get("base_ref")
    image_ref = image.get("ref")
    image_id = image.get("id")
    if base_image_ref != BASE_IMAGE_REF:
        fail("artifact base image is not pinned")
    if image_ref != f"morsehgp3d-phase3:{git_sha}":
        fail("artifact image ref does not bind the qualified SHA")
    if not isinstance(image_id, str) or IMAGE_ID_RE.fullmatch(image_id) is None:
        fail("artifact image ID is not canonical")
    environment_sha256 = validate_environment_artifact(
        environment_artifact_path,
        git_sha=git_sha,
        base_image_ref=base_image_ref,
        image_ref=image_ref,
        image_id=image_id,
    )
    if value.get("provenance") != {
        "environment_artifact_schema": PHASE3_SCHEMA,
        "environment_artifact_sha256": environment_sha256,
    }:
        fail("artifact provenance does not bind the Phase 3 evidence")
    if value.get("vm_lifecycle") != WORKER_LIFECYCLE:
        fail("artifact lifecycle is not pending targeted shutdown")

    binary = value.get("binary")
    if not isinstance(binary, dict):
        fail("binary evidence is absent")
    require_exact_keys(binary, {"qualification_sha256"}, "binary evidence")
    require_sha256(binary.get("qualification_sha256"), "qualification binary")

    logs = value.get("logs")
    digests = value.get("log_sha256")
    if not isinstance(logs, dict) or not isinstance(digests, dict):
        fail("logs or log hashes are absent")
    require_exact_keys(logs, LOG_KEYS, "rank-2/3 logs")
    require_exact_keys(digests, LOG_KEYS, "rank-2/3 log hashes")
    for name in LOG_KEYS:
        log = logs.get(name)
        if not isinstance(log, str):
            fail(f"log {name} must be text")
        if digests.get(name) != hashlib.sha256(log.encode("utf-8")).hexdigest():
            fail(f"log digest differs for {name}")
    validate_qualification_infrastructure_stderr(
        logs["qualification_stderr"]
    )
    validate_build_log(logs["release_build"], "release build log")
    validate_build_log(logs["audit_build"], "audit build log")
    direct = validate_qualification(
        parse_single_json_line(logs["qualification"], "qualification log"),
        git_sha=git_sha,
        single_run=False,
    )
    validate_elf_log(logs["cuobjdump_elf"])
    if logs["cuobjdump_ptx"].strip():
        fail("PTX stdout evidence must be empty")
    validate_memcheck_log(logs["memcheck"])
    validate_racecheck_log(logs["racecheck"])
    memcheck = extract_sanitizer_qualification(
        logs["memcheck"], "memcheck log", git_sha=git_sha
    )
    racecheck = extract_sanitizer_qualification(
        logs["racecheck"], "racecheck log", git_sha=git_sha
    )
    validate_scientific_equivalence(direct, memcheck, racecheck)

    checks = value.get("checks")
    if not isinstance(checks, dict):
        fail("artifact checks are absent")
    require_exact_keys(checks, CHECK_KEYS, "rank-2/3 checks")
    if checks != {
        "aot_elf_architectures": ["sm_120"],
        "aot_ptx_entry_count": 0,
        "audit_build": "passed",
        "cuda_audit_workflow": "passed",
        "cuda_release_workflow": "passed",
        "memcheck": "passed",
        "memcheck_qualification": memcheck,
        "qualification": direct,
        "racecheck": "passed",
        "racecheck_qualification": racecheck,
        "release_build": "passed",
        "sanitizer_scientific_catalog_equivalence": "passed",
    }:
        fail("artifact checks differ from the embedded proofs")
    return value


def validate_artifact_file(
    artifact_path: Path,
    *,
    git_sha: str,
    environment_artifact_path: Path,
) -> dict[str, Any]:
    raw, _ = read_text_evidence(artifact_path, "rank-2/3 qualification artifact")
    value = parse_json_object(raw, "rank-2/3 qualification artifact")
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
    parser.add_argument("--release-build-log", type=Path, required=True)
    parser.add_argument("--audit-build-log", type=Path, required=True)
    parser.add_argument("--qualification-log", type=Path, required=True)
    parser.add_argument("--qualification-stderr-log", type=Path, required=True)
    parser.add_argument("--elf-log", type=Path, required=True)
    parser.add_argument("--ptx-log", type=Path, required=True)
    parser.add_argument("--ptx-stderr-log", type=Path, required=True)
    parser.add_argument("--memcheck-log", type=Path, required=True)
    parser.add_argument("--racecheck-log", type=Path, required=True)
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
    environment_sha256 = validate_environment_artifact(
        args.environment_artifact,
        git_sha=args.git_sha,
        base_image_ref=args.base_image_ref,
        image_ref=args.image_ref,
        image_id=args.image_id,
    )

    specs = {
        "audit_build": (args.audit_build_log, "audit build log", False),
        "cuobjdump_elf": (args.elf_log, "cuobjdump ELF log", False),
        "cuobjdump_ptx": (args.ptx_log, "cuobjdump PTX log", True),
        "cuobjdump_ptx_stderr": (
            args.ptx_stderr_log,
            "cuobjdump PTX stderr log",
            True,
        ),
        "memcheck": (args.memcheck_log, "memcheck log", False),
        "qualification": (args.qualification_log, "qualification log", False),
        "qualification_stderr": (
            args.qualification_stderr_log,
            "qualification stderr log",
            True,
        ),
        "racecheck": (args.racecheck_log, "racecheck log", False),
        "release_build": (args.release_build_log, "release build log", False),
    }
    logs: dict[str, str] = {}
    digests: dict[str, str] = {}
    for name, (path, label, allow_empty) in specs.items():
        logs[name], digests[name] = read_text_evidence(
            path, label, allow_empty=allow_empty
        )
    validate_qualification_infrastructure_stderr(
        logs["qualification_stderr"]
    )
    validate_build_log(logs["release_build"], "release build log")
    validate_build_log(logs["audit_build"], "audit build log")
    direct = validate_qualification(
        parse_single_json_line(logs["qualification"], "qualification log"),
        git_sha=args.git_sha,
        single_run=False,
    )
    architectures = validate_elf_log(logs["cuobjdump_elf"])
    if logs["cuobjdump_ptx"].strip():
        fail("PTX stdout evidence must be empty")
    validate_memcheck_log(logs["memcheck"])
    validate_racecheck_log(logs["racecheck"])
    memcheck = extract_sanitizer_qualification(
        logs["memcheck"], "memcheck log", git_sha=args.git_sha
    )
    racecheck = extract_sanitizer_qualification(
        logs["racecheck"], "racecheck log", git_sha=args.git_sha
    )
    validate_scientific_equivalence(direct, memcheck, racecheck)

    artifact = {
        "artifact_role": ARTIFACT_ROLE,
        "backend": BACKEND,
        "binary": {
            "qualification_sha256": sha256_file(
                args.binary, "rank-2/3 qualification binary"
            )
        },
        "checks": {
            "aot_elf_architectures": architectures,
            "aot_ptx_entry_count": 0,
            "audit_build": "passed",
            "cuda_audit_workflow": "passed",
            "cuda_release_workflow": "passed",
            "memcheck": "passed",
            "memcheck_qualification": memcheck,
            "qualification": direct,
            "racecheck": "passed",
            "racecheck_qualification": racecheck,
            "release_build": "passed",
            "sanitizer_scientific_catalog_equivalence": "passed",
        },
        "claims": dict(NO_CLAIMS),
        "commands": {
            "memcheck_application": list(SANITIZER_APPLICATION_COMMAND),
            "qualification": list(QUALIFICATION_COMMAND),
            "racecheck_application": list(SANITIZER_APPLICATION_COMMAND),
        },
        "component_only": True,
        "deployment_status": DEPLOYMENT_STATUS,
        "git": {"clean": True, "sha": args.git_sha},
        "image": {
            "base_ref": args.base_image_ref,
            "id": args.image_id,
            "ref": args.image_ref,
        },
        "log_sha256": digests,
        "logs": logs,
        "mode": MODE,
        "phase": "15",
        "phase_context": dict(PHASE_CONTEXT),
        "profile": PROFILE,
        "provenance": {
            "environment_artifact_schema": PHASE3_SCHEMA,
            "environment_artifact_sha256": environment_sha256,
        },
        "schema": SCHEMA,
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "scientific_scope": SCIENTIFIC_SCOPE,
        "status": "worker_passed_pending_shutdown",
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
