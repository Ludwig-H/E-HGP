#!/usr/bin/env python3
"""Assemble the guarded q=3 Gabriel pair-carrier negative-lane evidence."""

from __future__ import annotations

import argparse
import copy
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
    "morsehgp3d.phase15.morton_yao48_device_tiled_pair_frontier_"
    "strict_interior_threshold_cuda_g4_qualification.v1"
)
QUALIFICATION_SCHEMA = (
    "morsehgp3d.phase15.morton_yao48_device_tiled_pair_frontier_"
    "strict_interior_threshold_qualification.v1"
)
ARTIFACT_ROLE = (
    "q3_gabriel_exact_diametral_pair_support_negative_only_component_"
    "qualification"
)
BACKEND = "cuda_g4"
PROFILE = "hgp_reduced"
MODE = (
    "native_bounded_q3_gabriel_exact_diametral_pair_support_negative_only_"
    "qualification"
)
DEPLOYMENT_STATUS = "component_only"
PUBLIC_STATUS = "not_claimed"
SCIENTIFIC_SCOPE = "q3_gabriel_exact_diametral_pair_support_negative_only"
FIXTURE = "q3_shell_strict_discriminant"
BINARY_RELATIVE_PATH = (
    "build/morsehgp3d-cuda-release/"
    "morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification"
)
BINARY_CONTAINER_PATH = f"/workspace/repository/{BINARY_RELATIVE_PATH}"
QUALIFICATION_COMMAND = [
    BINARY_CONTAINER_PATH,
    "--prune-semantics",
    "strict_interior_threshold",
    "--scientific-scope",
    SCIENTIFIC_SCOPE,
    "--fixture",
    FIXTURE,
]
SANITIZER_APPLICATION_COMMAND = list(QUALIFICATION_COMMAND)

QUALIFICATION_KEYS = {
    "backend",
    "boundary_shell_preserved",
    "closed_boundary_control",
    "closed_boundary_control_negative_covered",
    "component_only",
    "deployment_status",
    "exact_all_pair_all_witness_oracle",
    "fixture",
    "gamma2_catalog_published",
    "gamma2_prune_or_discard",
    "gamma2_silent_handoff_required",
    "git_sha",
    "global_cell_or_coface_arena_materialized",
    "global_pair_matrix_materialized",
    "hierarchy_reduction_performed",
    "maximum_closed_rank_metadata_invariance_qualified",
    "mode",
    "morse_hgp_hierarchy_claimed",
    "negative_region_interpretation",
    "ordinary_delaunay_materialized",
    "pair_frontier_negative_regions_are_gamma2_discard",
    "point_count_per_cloud",
    "profile",
    "public_status",
    "q",
    "q3_exact_diametral_pair_support_gabriel_negative_only",
    "required_witness_count",
    "run_count",
    "schema",
    "scientific_pair_catalog_published",
    "scientific_scope",
    "strict_cases",
    "strict_interior_target_negative_covered",
    "success",
    "tile_capacity_invariance_qualified",
    "total_ns",
}
CASE_KEYS = {
    "anchor_tile_capacity",
    "cloud",
    "cloud_digest_fnv1a",
    "expected_candidate_pair_mass",
    "expected_certified_pruned_pair_mass",
    "maximum_closed_rank_metadata",
    "rank_result",
    "target_ax_negative_covered",
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
    "candidate_yield_anchor_count",
    "censored_anchor_count",
    "certified_pruned_pair_mass",
    "chunk_count",
    "complete_anchor_count",
    "component_contract_validated",
    "coverage_partition_complete",
    "cpu_recertification_ns",
    "device_total_bytes",
    "every_prune_fully_recertified",
    "fully_recertified_prune_count",
    "fresh_tile_device_arena_allocation_count",
    "fresh_tile_device_arena_reuse_count",
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
    "prune_yield_anchor_count",
    "q3_exact_diametral_pair_support_gabriel_negative_only",
    "qualification_device_to_host_bytes",
    "qualification_output_copy_ns",
    "required_witness_count",
    "resumed_chunk_count",
    "sampled_recertified_prune_count",
    "tile_count",
    "traversal_adoption_ns",
    "traversal_device_capacity_bytes",
    "unbanked_candidate_count",
    "unordered_pair_universe_count",
    "unresolved_pair_mass",
}
RANK_BOOLEAN_KEYS = {
    "bounded_bruteforce_performed",
    "component_contract_validated",
    "coverage_partition_complete",
    "every_prune_fully_recertified",
}
RANK_SCOPE_BOOLEAN_KEYS = {
    "gamma2_prune_or_discard_authorized",
    "gamma2_silent_handoff_required",
    "q3_exact_diametral_pair_support_gabriel_negative_only",
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
    "gamma2_prune_or_discard",
    "memcheck",
    "memcheck_qualification",
    "qualification",
    "racecheck",
    "racecheck_qualification",
    "release_build",
    "sanitizer_scientific_equivalence",
}
CLAIM_KEYS = {
    "full_gamma2",
    "gamma2_catalog",
    "gamma2_prune_or_discard",
    "hierarchy_reduction",
    "industrial_scale",
    "morse_hgp_hierarchy",
    "public_status_claimed",
    "scientific_pair_catalog",
    "slo",
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
    r"\[TIMEOUT\] unité=phase15-device-frontier-strict-interior-qualification, "
    r"borne douce=[1-9][0-9]*s, kill-after=5s, "
    r"réserve post-timeout=60s\.\n"
    r"[0-9a-f]{64}\n\Z"
)
RANK_TIMING_KEYS = {
    "bounded_bruteforce_ns",
    "build_ns",
    "cpu_recertification_ns",
    "launcher_ns",
    "lease_release_ns",
    "minimum_device_free_bytes",
    "node_copy_ns",
    "qualification_output_copy_ns",
    "traversal_adoption_ns",
}


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


def require_integer(value: Any, label: str, *, expected: int | None = None) -> int:
    if type(value) is not int or value < 0:
        fail(f"{label} must be a non-negative integer")
    if expected is not None and value != expected:
        fail(f"{label} must be exactly {expected}")
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
    if not value.strip() or BUILD_FAILURE_RE.search(value) is not None:
        fail(f"{label} is empty or contains a build failure")


def validate_qualification_infrastructure_stderr(value: str) -> None:
    if QUALIFICATION_INFRASTRUCTURE_STDERR_RE.fullmatch(value) is None:
        fail("qualification stderr does not close the guarded timeout contract")


def validate_rank(
    value: dict[str, Any],
    *,
    semantics: str,
    maximum_closed_rank: int,
    candidate_mass: int,
    negative_mass: int,
) -> dict[str, Any]:
    require_exact_keys(value, RANK_KEYS, "q3 negative-lane rank result")
    for field in (
        RANK_KEYS
        - {"prune_semantics"}
        - RANK_BOOLEAN_KEYS
        - RANK_SCOPE_BOOLEAN_KEYS
    ):
        require_integer(value.get(field), f"rank_result.{field}")
    if value.get("prune_semantics") != semantics:
        fail(f"rank_result.prune_semantics must be {semantics}")
    for field in RANK_BOOLEAN_KEYS:
        require_boolean(value.get(field), True, f"rank_result.{field}")
    strict_semantics = semantics == "strict_interior_threshold"
    require_boolean(
        value.get("q3_exact_diametral_pair_support_gabriel_negative_only"),
        strict_semantics,
        "rank_result.q3_exact_diametral_pair_support_gabriel_negative_only",
    )
    require_boolean(
        value.get("gamma2_silent_handoff_required"),
        strict_semantics,
        "rank_result.gamma2_silent_handoff_required",
    )
    require_boolean(
        value.get("gamma2_prune_or_discard_authorized"),
        False,
        "rank_result.gamma2_prune_or_discard_authorized",
    )
    require_integer(
        value.get("maximum_closed_rank"),
        "rank_result.maximum_closed_rank",
        expected=maximum_closed_rank,
    )
    require_integer(
        value.get("required_witness_count"),
        "rank_result.required_witness_count",
        expected=2,
    )
    require_integer(
        value.get("fresh_tile_device_arena_allocation_count"),
        "rank_result.fresh_tile_device_arena_allocation_count",
        expected=1,
    )
    require_integer(
        value.get("fresh_tile_device_arena_reuse_count"),
        "rank_result.fresh_tile_device_arena_reuse_count",
        expected=0,
    )
    for field in ("bounded_exact_pair_count", "unordered_pair_universe_count"):
        require_integer(value.get(field), f"rank_result.{field}", expected=6)
    for field in (
        "candidate_pair_mass",
        "bounded_admitted_pair_count",
        "bounded_candidate_admitted_pair_count",
    ):
        require_integer(value.get(field), f"rank_result.{field}", expected=candidate_mass)
    require_integer(
        value.get("certified_pruned_pair_mass"),
        "rank_result.certified_pruned_pair_mass",
        expected=negative_mass,
    )
    for field in ("bounded_candidate_rejected_pair_count", "unresolved_pair_mass"):
        require_integer(value.get(field), f"rank_result.{field}", expected=0)
    if candidate_mass + negative_mass != 6:
        fail("rank result does not partition the six unordered fixture pairs")
    return value


def validate_case(
    value: dict[str, Any],
    *,
    cloud: str,
    semantics: str,
    maximum_closed_rank: int,
    anchor_tile_capacity: int,
) -> dict[str, Any]:
    require_exact_keys(value, CASE_KEYS, "q3 discriminant case")
    if value.get("cloud") != cloud:
        fail(f"case cloud must be {cloud}")
    require_integer(
        value.get("maximum_closed_rank_metadata"),
        "maximum_closed_rank_metadata",
        expected=maximum_closed_rank,
    )
    require_integer(
        value.get("anchor_tile_capacity"),
        "anchor_tile_capacity",
        expected=anchor_tile_capacity,
    )
    require_integer(value.get("cloud_digest_fnv1a"), "cloud_digest_fnv1a")
    negative_mass = 0 if semantics == "strict_interior_threshold" and cloud == "boundary_shell" else 1
    candidate_mass = 6 - negative_mass
    require_integer(
        value.get("expected_candidate_pair_mass"),
        "expected_candidate_pair_mass",
        expected=candidate_mass,
    )
    require_integer(
        value.get("expected_certified_pruned_pair_mass"),
        "expected_certified_pruned_pair_mass",
        expected=negative_mass,
    )
    require_boolean(
        value.get("target_ax_negative_covered"),
        negative_mass == 1,
        "target_ax_negative_covered",
    )
    rank = value.get("rank_result")
    if not isinstance(rank, dict):
        fail("case rank_result must be an object")
    validate_rank(
        rank,
        semantics=semantics,
        maximum_closed_rank=maximum_closed_rank,
        candidate_mass=candidate_mass,
        negative_mass=negative_mass,
    )
    return value


def validate_qualification(
    value: dict[str, Any],
    *,
    git_sha: str,
) -> dict[str, Any]:
    require_exact_keys(value, QUALIFICATION_KEYS, "q3 Gabriel negative qualification")
    for field, expected in {
        "backend": BACKEND,
        "deployment_status": DEPLOYMENT_STATUS,
        "fixture": FIXTURE,
        "git_sha": git_sha,
        "mode": MODE,
        "negative_region_interpretation": (
            "no_q3_gabriel_triangle_with_exact_diametral_pair_minimal_"
            "support_only"
        ),
        "profile": PROFILE,
        "public_status": PUBLIC_STATUS,
        "schema": QUALIFICATION_SCHEMA,
        "scientific_scope": SCIENTIFIC_SCOPE,
    }.items():
        if value.get(field) != expected:
            fail(f"qualification {field} must be {expected}")
    for field in (
        "boundary_shell_preserved",
        "closed_boundary_control_negative_covered",
        "component_only",
        "exact_all_pair_all_witness_oracle",
        "gamma2_silent_handoff_required",
        "maximum_closed_rank_metadata_invariance_qualified",
        "q3_exact_diametral_pair_support_gabriel_negative_only",
        "strict_interior_target_negative_covered",
        "success",
        "tile_capacity_invariance_qualified",
    ):
        require_boolean(value.get(field), True, field)
    for field in (
        "gamma2_catalog_published",
        "gamma2_prune_or_discard",
        "global_cell_or_coface_arena_materialized",
        "global_pair_matrix_materialized",
        "hierarchy_reduction_performed",
        "morse_hgp_hierarchy_claimed",
        "ordinary_delaunay_materialized",
        "pair_frontier_negative_regions_are_gamma2_discard",
        "scientific_pair_catalog_published",
    ):
        require_boolean(value.get(field), False, field)
    for field, expected in {
        "point_count_per_cloud": 4,
        "q": 3,
        "required_witness_count": 2,
        "run_count": 9,
    }.items():
        require_integer(value.get(field), field, expected=expected)
    require_integer(value.get("total_ns"), "total_ns")

    cases = value.get("strict_cases")
    if not isinstance(cases, list) or len(cases) != 8:
        fail("strict_cases must contain the exact eight-run matrix")
    expected_matrix = {
        (cloud, maximum_closed_rank, tile)
        for cloud in ("boundary_shell", "strict_interior")
        for maximum_closed_rank in (2, 11)
        for tile in (1, 4)
    }
    observed_matrix: set[tuple[str, int, int]] = set()
    cloud_digests: dict[str, int] = {}
    for case in cases:
        if not isinstance(case, dict):
            fail("strict case must be an object")
        cloud = case.get("cloud")
        maximum_closed_rank = case.get("maximum_closed_rank_metadata")
        tile = case.get("anchor_tile_capacity")
        if (
            not isinstance(cloud, str)
            or type(maximum_closed_rank) is not int
            or type(tile) is not int
        ):
            fail("strict case matrix coordinates are malformed")
        observed_matrix.add((cloud, maximum_closed_rank, tile))
        validate_case(
            case,
            cloud=cloud,
            semantics="strict_interior_threshold",
            maximum_closed_rank=maximum_closed_rank,
            anchor_tile_capacity=tile,
        )
        digest = case["cloud_digest_fnv1a"]
        previous = cloud_digests.setdefault(cloud, digest)
        if previous != digest:
            fail("fixture cloud digest changed across metadata/tile runs")
    if observed_matrix != expected_matrix:
        fail("strict case matrix is incomplete, duplicated, or unexpected")

    control = value.get("closed_boundary_control")
    if not isinstance(control, dict):
        fail("closed_boundary_control must be an object")
    validate_case(
        control,
        cloud="boundary_shell",
        semantics="closed_rank_window",
        maximum_closed_rank=3,
        anchor_tile_capacity=4,
    )
    if control["cloud_digest_fnv1a"] != cloud_digests["boundary_shell"]:
        fail("closed control did not replay the boundary-shell cloud")
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
    )


def scientific_signature(value: dict[str, Any]) -> dict[str, Any]:
    signature = copy.deepcopy(value)
    signature.pop("total_ns", None)
    for case in [signature["closed_boundary_control"], *signature["strict_cases"]]:
        rank = case["rank_result"]
        for field in RANK_TIMING_KEYS:
            rank.pop(field, None)
    return signature


def validate_scientific_equivalence(
    direct: dict[str, Any],
    memcheck: dict[str, Any],
    racecheck: dict[str, Any],
) -> None:
    expected = scientific_signature(direct)
    if scientific_signature(memcheck) != expected:
        fail("memcheck changed the q3 Gabriel negative-lane partition")
    if scientific_signature(racecheck) != expected:
        fail("racecheck changed the q3 Gabriel negative-lane partition")


def validate_environment_artifact(
    path: Path,
    *,
    git_sha: str,
    base_image_ref: str,
    image_ref: str,
    image_id: str,
) -> str:
    raw, digest = read_text_evidence(path, "Phase 3 environment artifact")
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
    if SHA_RE.fullmatch(git_sha) is None:
        fail("git_sha must be a canonical lowercase 40-hex commit ID")
    require_exact_keys(value, ARTIFACT_KEYS, "q3 Gabriel negative artifact")
    if raw != canonical_json(value):
        fail("q3 Gabriel negative artifact must be canonical JSON")
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
    require_boolean(value.get("scientific_result_claimed"), False, "scientific_result_claimed")
    if value.get("scientific_public_status") is not None:
        fail("scientific_public_status must be null")
    if value.get("phase_context") != PHASE_CONTEXT:
        fail("Phase 15 administrative context differs")
    if value.get("claims") != NO_CLAIMS:
        fail("q3 pair-carrier evidence must not claim Gamma2 or a hierarchy")
    expected_commands = {
        "memcheck_application": SANITIZER_APPLICATION_COMMAND,
        "qualification": QUALIFICATION_COMMAND,
        "racecheck_application": SANITIZER_APPLICATION_COMMAND,
    }
    if value.get("commands") != expected_commands:
        fail("guarded q3 qualification commands differ")
    if value.get("git") != {"clean": True, "sha": git_sha}:
        fail("artifact does not bind the clean qualified Git SHA")

    image = value.get("image")
    if not isinstance(image, dict):
        fail("artifact image identity is absent")
    require_exact_keys(image, {"base_ref", "id", "ref"}, "image identity")
    base_image_ref = image.get("base_ref")
    image_ref = image.get("ref")
    image_id = image.get("id")
    if (
        base_image_ref != BASE_IMAGE_REF
        or image_ref != f"morsehgp3d-phase3:{git_sha}"
        or not isinstance(image_id, str)
        or IMAGE_ID_RE.fullmatch(image_id) is None
    ):
        fail("artifact image identity is not pinned to the qualified SHA")
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
    require_exact_keys(logs, LOG_KEYS, "q3 qualification logs")
    require_exact_keys(digests, LOG_KEYS, "q3 qualification log hashes")
    for name in LOG_KEYS:
        log = logs.get(name)
        if not isinstance(log, str):
            fail(f"log {name} must be text")
        if digests.get(name) != hashlib.sha256(log.encode("utf-8")).hexdigest():
            fail(f"log digest differs for {name}")
    validate_qualification_infrastructure_stderr(logs["qualification_stderr"])
    validate_build_log(logs["release_build"], "release build log")
    validate_build_log(logs["audit_build"], "audit build log")
    direct = validate_qualification(
        parse_single_json_line(logs["qualification"], "qualification log"),
        git_sha=git_sha,
    )
    architectures = validate_elf_log(logs["cuobjdump_elf"])
    if architectures != ["sm_120"]:
        fail("qualified binary must contain only sm_120 ELF")
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
    require_exact_keys(checks, CHECK_KEYS, "q3 qualification checks")
    if checks != {
        "aot_elf_architectures": ["sm_120"],
        "aot_ptx_entry_count": 0,
        "audit_build": "passed",
        "cuda_audit_workflow": "passed",
        "cuda_release_workflow": "passed",
        "gamma2_prune_or_discard": False,
        "memcheck": "passed",
        "memcheck_qualification": memcheck,
        "qualification": direct,
        "racecheck": "passed",
        "racecheck_qualification": racecheck,
        "release_build": "passed",
        "sanitizer_scientific_equivalence": "passed",
    }:
        fail("artifact checks differ from the embedded q3 proofs")
    return value


def validate_artifact_file(
    artifact_path: Path,
    *,
    git_sha: str,
    environment_artifact_path: Path,
) -> dict[str, Any]:
    raw, _ = read_text_evidence(artifact_path, "q3 Gabriel negative artifact")
    value = parse_json_object(raw, "q3 Gabriel negative artifact")
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
        "cuobjdump_ptx_stderr": (args.ptx_stderr_log, "PTX stderr log", True),
        "memcheck": (args.memcheck_log, "memcheck log", False),
        "qualification": (args.qualification_log, "qualification log", False),
        "qualification_stderr": (
            args.qualification_stderr_log,
            "qualification stderr log",
            False,
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
    validate_qualification_infrastructure_stderr(logs["qualification_stderr"])
    validate_build_log(logs["release_build"], "release build log")
    validate_build_log(logs["audit_build"], "audit build log")
    direct = validate_qualification(
        parse_single_json_line(logs["qualification"], "qualification log"),
        git_sha=args.git_sha,
    )
    architectures = validate_elf_log(logs["cuobjdump_elf"])
    if architectures != ["sm_120"]:
        fail("qualified binary must contain only sm_120 ELF")
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
                args.binary, "q3 Gabriel negative qualification binary"
            )
        },
        "checks": {
            "aot_elf_architectures": architectures,
            "aot_ptx_entry_count": 0,
            "audit_build": "passed",
            "cuda_audit_workflow": "passed",
            "cuda_release_workflow": "passed",
            "gamma2_prune_or_discard": False,
            "memcheck": "passed",
            "memcheck_qualification": memcheck,
            "qualification": direct,
            "racecheck": "passed",
            "racecheck_qualification": racecheck,
            "release_build": "passed",
            "sanitizer_scientific_equivalence": "passed",
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
