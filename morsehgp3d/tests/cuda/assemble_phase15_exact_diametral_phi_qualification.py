#!/usr/bin/env python3
"""Validate and assemble the Phase 15 exact diametral-Phi G4 evidence."""

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

SCHEMA = "morsehgp3d.phase15.exact_diametral_phi_cuda_g4_qualification.v2"
QUALIFICATION_SCHEMA = "morsehgp3d.phase15.exact_diametral_phi_qualification.v2"
ARTIFACT_ROLE = "exact_diametral_phi_component_qualification"
BACKEND = "cuda_g4_exact_fixed_limb_plus_cpu_multiprecision_fallback"
PROFILE = "hgp_reduced"
MODE = "exact_binary64_point_predicate_batch"
DEPLOYMENT_STATUS = "architecture_only"
PUBLIC_STATUS = "not_claimed"
SCIENTIFIC_SCOPE = "bounded_exact_diametral_phi_device_component_only"
DECISION_SEMANTICS = (
    "exact_binary64_dyadic_phi_sign_gpu_interval_then_fixed_limb_128_256_"
    "then_batched_cpu_multiprecision_fallback"
)
EXPECTED_QUERY_COUNT = 7_066
EXPECTED_DETERMINISTIC_RANDOM_QUERY_COUNT = 4_096
EXPECTED_NEAR_SHELL_QUERY_COUNT = 144
EXPECTED_SYMMETRY_QUERY_COUNT = 2_816
EXPECTED_ADVERSARIAL_QUERY_COUNT = 10

QUALIFICATION_KEYS = {
    "adversarial_query_count",
    "all_results_exact",
    "backend",
    "catalog_complete_claimed",
    "component_exact_sign_qualification_passed",
    "cuda_device",
    "cuda_execution_performed",
    "decision_digest_fnv1a",
    "deployment_status",
    "deterministic_random_query_count",
    "deterministic_replay_passed",
    "epsilon_used",
    "interval_zero_certification_forbidden",
    "mode",
    "near_shell_query_count",
    "per_run_cpu_multiprecision_count",
    "per_run_cuda_kernel_launch_count",
    "per_run_cuda_synchronization_count",
    "per_run_disagreement_count",
    "per_run_exact_zero_count",
    "per_run_gpu_exact_fallback_count",
    "per_run_gpu_fixed_limb_dyadic_count",
    "per_run_gpu_fp64_interval_count",
    "per_run_gpu_known_audited_count",
    "per_run_launcher_call_count",
    "per_run_query_count",
    "phase",
    "profile",
    "public_status",
    "public_status_claimed",
    "run_count",
    "scalability_claimed",
    "schema",
    "scientific_catalog_decision_published",
    "symmetry_query_count",
    "total_cuda_kernel_launch_count",
    "total_cuda_synchronization_count",
    "total_evaluated_query_count",
    "total_launcher_call_count",
}

LOG_KEYS = {
    "audit_build",
    "cuobjdump_elf",
    "cuobjdump_ptx",
    "cuobjdump_ptx_stderr",
    "memcheck",
    "qualification",
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
    "qualification",
    "racecheck",
    "release_build",
    "sanitizer_qualification_replay",
}
CLAIM_KEYS = {
    "catalog_complete",
    "hierarchy_reduction_performed",
    "public_status_claimed",
    "scalability_claimed",
    "scientific_result_claimed",
    "slo_claimed",
    "warm_e2e_measured",
}
NO_CLAIMS = {key: False for key in CLAIM_KEYS}
PHASE_CONTEXT = {
    "backend": "reference_cpu",
    "deployment_status": DEPLOYMENT_STATUS,
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
    "component_only",
    "decision_semantics",
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


def fail(message: str) -> NoReturn:
    raise SystemExit(message)


def canonical_json(value: dict[str, Any]) -> str:
    return (
        json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
        + "\n"
    )


def require_exact_integer(value: Any, expected: int, label: str) -> None:
    if type(value) is not int or value != expected:
        fail(f"{label} must be exactly {expected}")


def require_positive_integer(value: Any, label: str) -> int:
    if type(value) is not int or value <= 0:
        fail(f"{label} must be a positive integer")
    return value


def require_exact_boolean(value: Any, expected: bool, label: str) -> None:
    if type(value) is not bool or value is not expected:
        fail(f"{label} must be {str(expected).lower()}")


def require_sha256(value: Any, label: str) -> str:
    if not isinstance(value, str) or SHA256_RE.fullmatch(value) is None:
        fail(f"{label} must be a canonical SHA-256 digest")
    return value


def parse_single_line_json_object(raw: str, label: str) -> dict[str, Any]:
    lines = raw.splitlines()
    if len(lines) != 1 or not lines[0]:
        fail(f"{label} must contain exactly one non-empty JSON line")
    return parse_json_object(raw, label)


def validate_build_log(value: str, label: str) -> None:
    if not value.strip():
        fail(f"{label} must be non-empty")
    if BUILD_FAILURE_RE.search(value) is not None:
        fail(f"{label} contains an obvious build failure")


def validate_sanitizer_qualification_replay(
    value: str,
    label: str,
    qualification: dict[str, Any],
) -> None:
    candidates = [line for line in value.splitlines() if line.startswith("{")]
    if len(candidates) != 1:
        fail(f"{label} must contain exactly one qualification JSON line")
    replay = validate_qualification(
        parse_single_line_json_object(candidates[0], f"{label} qualification")
    )
    canonical_line = canonical_json(replay).removesuffix("\n")
    if candidates[0] != canonical_line:
        fail(f"{label} qualification JSON line must be canonical")
    if canonical_line != canonical_json(qualification).removesuffix("\n"):
        fail(f"{label} did not replay the exact qualification corpus")


def validate_qualification(value: dict[str, Any]) -> dict[str, Any]:
    """Validate the closed stdout schema emitted by the real CUDA binary."""

    require_exact_keys(value, QUALIFICATION_KEYS, "qualification result")
    expected_scalars = {
        "backend": BACKEND,
        "deployment_status": DEPLOYMENT_STATUS,
        "mode": MODE,
        "phase": "15",
        "profile": PROFILE,
        "public_status": PUBLIC_STATUS,
        "schema": QUALIFICATION_SCHEMA,
    }
    for field, expected in expected_scalars.items():
        if value.get(field) != expected:
            fail(f"qualification result.{field} must be {expected}")

    expected_corpus_counts = {
        "adversarial_query_count": EXPECTED_ADVERSARIAL_QUERY_COUNT,
        "deterministic_random_query_count": EXPECTED_DETERMINISTIC_RANDOM_QUERY_COUNT,
        "near_shell_query_count": EXPECTED_NEAR_SHELL_QUERY_COUNT,
        "symmetry_query_count": EXPECTED_SYMMETRY_QUERY_COUNT,
    }
    require_exact_integer(
        value.get("per_run_query_count"),
        EXPECTED_QUERY_COUNT,
        "qualification per_run_query_count",
    )
    for field, expected in expected_corpus_counts.items():
        require_exact_integer(value.get(field), expected, f"qualification {field}")
    if (
        sum(expected_corpus_counts.values()) != value["per_run_query_count"]
    ):
        fail("qualification corpus cardinalities do not close with adversarial cases")
    for field in (
        "per_run_cpu_multiprecision_count",
        "per_run_exact_zero_count",
        "per_run_gpu_exact_fallback_count",
        "per_run_gpu_fixed_limb_dyadic_count",
        "per_run_gpu_fp64_interval_count",
        "per_run_gpu_known_audited_count",
    ):
        require_positive_integer(value.get(field), f"qualification {field}")
    for field in (
        "per_run_cuda_kernel_launch_count",
        "per_run_cuda_synchronization_count",
        "per_run_launcher_call_count",
    ):
        require_exact_integer(value.get(field), 1, f"qualification {field}")
    require_exact_integer(
        value.get("per_run_disagreement_count"),
        0,
        "qualification per_run_disagreement_count",
    )
    require_exact_integer(value.get("run_count"), 2, "qualification run_count")
    for field in (
        "total_cuda_kernel_launch_count",
        "total_cuda_synchronization_count",
        "total_launcher_call_count",
    ):
        require_exact_integer(value.get(field), 2, f"qualification {field}")
    require_exact_integer(
        value.get("total_evaluated_query_count"),
        2 * EXPECTED_QUERY_COUNT,
        "qualification total_evaluated_query_count",
    )
    decision_digest = value.get("decision_digest_fnv1a")
    if (
        type(decision_digest) is not int
        or decision_digest < 0
        or decision_digest > (1 << 64) - 1
    ):
        fail("qualification decision_digest_fnv1a must be a uint64")
    cuda_device = value.get("cuda_device")
    if type(cuda_device) is not int or cuda_device < 0:
        fail("qualification cuda_device must be a non-negative integer")

    for field in (
        "all_results_exact",
        "component_exact_sign_qualification_passed",
        "cuda_execution_performed",
        "deterministic_replay_passed",
        "interval_zero_certification_forbidden",
    ):
        require_exact_boolean(value.get(field), True, f"qualification {field}")
    for field in (
        "catalog_complete_claimed",
        "epsilon_used",
        "public_status_claimed",
        "scalability_claimed",
        "scientific_catalog_decision_published",
    ):
        require_exact_boolean(value.get(field), False, f"qualification {field}")

    if (
        value["per_run_gpu_fp64_interval_count"]
        + value["per_run_gpu_fixed_limb_dyadic_count"]
        + value["per_run_gpu_exact_fallback_count"]
        != value["per_run_query_count"]
    ):
        fail("qualification GPU stage counts must partition every query")
    if value["per_run_gpu_exact_fallback_count"] != value[
        "per_run_cpu_multiprecision_count"
    ]:
        fail("qualification GPU fallback and CPU multiprecision counts differ")
    if (
        value["per_run_gpu_fp64_interval_count"]
        + value["per_run_gpu_fixed_limb_dyadic_count"]
        != value["per_run_gpu_known_audited_count"]
    ):
        fail("qualification audited GPU count does not cover every GPU sign")
    if value["per_run_exact_zero_count"] > value["per_run_query_count"]:
        fail("qualification exact zero count exceeds the query count")
    return value


def _validate_environment_artifact(
    environment_artifact_path: Path,
    *,
    git_sha: str,
    base_image_ref: str,
    image_ref: str,
    image_id: str,
) -> tuple[dict[str, Any], str, str]:
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
    return value, raw, sha256


def validate_artifact(
    value: dict[str, Any],
    raw: str,
    *,
    git_sha: str,
    environment_artifact_path: Path,
) -> dict[str, Any]:
    """Revalidate one pending-shutdown artifact and every embedded proof."""

    if SHA_RE.fullmatch(git_sha) is None:
        fail("git_sha must be a canonical lowercase 40-hex commit ID")
    require_exact_keys(value, ARTIFACT_KEYS, "Phase 15 qualification artifact")
    if raw != canonical_json(value):
        fail("Phase 15 qualification artifact must be canonical JSON")

    expected_scalars = {
        "artifact_role": ARTIFACT_ROLE,
        "backend": BACKEND,
        "decision_semantics": DECISION_SEMANTICS,
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
            fail(f"Phase 15 qualification artifact.{field} must be {expected}")
    require_exact_boolean(value.get("component_only"), True, "component_only")
    require_exact_boolean(
        value.get("scientific_result_claimed"),
        False,
        "scientific_result_claimed",
    )
    if value.get("scientific_public_status") is not None:
        fail("scientific_public_status must be null")
    if value.get("phase_context") != PHASE_CONTEXT:
        fail("Phase 15 administrative context differs")
    claims = value.get("claims")
    if not isinstance(claims, dict):
        fail("Phase 15 claims must be an object")
    require_exact_keys(claims, CLAIM_KEYS, "Phase 15 claims")
    if claims != NO_CLAIMS:
        fail("Phase 15 component artifact must not publish a product claim")

    if value.get("git") != {"clean": True, "sha": git_sha}:
        fail("Phase 15 artifact does not bind the clean qualified Git SHA")
    image = value.get("image")
    if not isinstance(image, dict):
        fail("Phase 15 artifact image is absent")
    require_exact_keys(image, {"base_ref", "id", "ref"}, "image identity")
    base_image_ref = image.get("base_ref")
    image_ref = image.get("ref")
    image_id = image.get("id")
    if base_image_ref != BASE_IMAGE_REF:
        fail("Phase 15 artifact base image is not pinned")
    if image_ref != f"morsehgp3d-phase3:{git_sha}":
        fail("Phase 15 artifact image ref is not tied to the qualified SHA")
    if not isinstance(image_id, str) or IMAGE_ID_RE.fullmatch(image_id) is None:
        fail("Phase 15 artifact image ID is not canonical")

    _, _, environment_sha256 = _validate_environment_artifact(
        environment_artifact_path,
        git_sha=git_sha,
        base_image_ref=base_image_ref,
        image_ref=image_ref,
        image_id=image_id,
    )
    provenance = value.get("provenance")
    expected_provenance = {
        "environment_artifact_schema": PHASE3_SCHEMA,
        "environment_artifact_sha256": environment_sha256,
    }
    if provenance != expected_provenance:
        fail("Phase 15 artifact provenance does not bind the Phase 3 artifact")
    if value.get("vm_lifecycle") != WORKER_LIFECYCLE:
        fail("Phase 15 artifact lifecycle is not pending targeted shutdown")

    binary = value.get("binary")
    if not isinstance(binary, dict):
        fail("Phase 15 artifact binary evidence is absent")
    require_exact_keys(binary, {"qualification_sha256"}, "binary evidence")
    require_sha256(binary.get("qualification_sha256"), "qualification binary")

    logs = value.get("logs")
    log_sha256 = value.get("log_sha256")
    if not isinstance(logs, dict) or not isinstance(log_sha256, dict):
        fail("Phase 15 artifact logs or log hashes are absent")
    require_exact_keys(logs, LOG_KEYS, "Phase 15 logs")
    require_exact_keys(log_sha256, LOG_KEYS, "Phase 15 log hashes")
    for name in LOG_KEYS:
        log = logs.get(name)
        if not isinstance(log, str):
            fail(f"Phase 15 log {name} must be text")
        expected_hash = hashlib.sha256(log.encode("utf-8")).hexdigest()
        if log_sha256.get(name) != expected_hash:
            fail(f"Phase 15 log digest differs for {name}")

    validate_build_log(logs["release_build"], "release build log")
    validate_build_log(logs["audit_build"], "audit build log")
    qualification = validate_qualification(
        parse_single_line_json_object(logs["qualification"], "qualification log")
    )
    if logs["qualification"] != canonical_json(qualification):
        fail("qualification log must be canonical JSON")
    validate_elf_log(logs["cuobjdump_elf"])
    if logs["cuobjdump_ptx"].strip():
        fail("PTX stdout evidence must be empty")
    validate_memcheck_log(logs["memcheck"])
    validate_racecheck_log(logs["racecheck"])
    validate_sanitizer_qualification_replay(
        logs["memcheck"], "memcheck log", qualification
    )
    validate_sanitizer_qualification_replay(
        logs["racecheck"], "racecheck log", qualification
    )

    checks = value.get("checks")
    if not isinstance(checks, dict):
        fail("Phase 15 artifact checks are absent")
    require_exact_keys(checks, CHECK_KEYS, "Phase 15 checks")
    expected_checks = {
        "aot_elf_architectures": ["sm_120"],
        "aot_ptx_entry_count": 0,
        "audit_build": "passed",
        "cuda_audit_workflow": "passed",
        "cuda_release_workflow": "passed",
        "memcheck": "passed",
        "qualification": qualification,
        "racecheck": "passed",
        "release_build": "passed",
        "sanitizer_qualification_replay": "passed",
    }
    if checks != expected_checks:
        fail("Phase 15 artifact checks differ from the embedded proofs")
    return value


def validate_artifact_file(
    artifact_path: Path,
    *,
    git_sha: str,
    environment_artifact_path: Path,
) -> dict[str, Any]:
    """Public shared authority used after transfer by the orchestrator."""

    raw, _ = read_text_evidence(artifact_path, "Phase 15 qualification artifact")
    value = parse_json_object(raw, "Phase 15 qualification artifact")
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

    _, _, environment_sha256 = _validate_environment_artifact(
        args.environment_artifact,
        git_sha=args.git_sha,
        base_image_ref=args.base_image_ref,
        image_ref=args.image_ref,
        image_id=args.image_id,
    )
    evidence_specs = {
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
        "racecheck": (args.racecheck_log, "racecheck log", False),
        "release_build": (args.release_build_log, "release build log", False),
    }
    logs: dict[str, str] = {}
    log_sha256: dict[str, str] = {}
    for name, (path, label, allow_empty) in evidence_specs.items():
        logs[name], log_sha256[name] = read_text_evidence(
            path, label, allow_empty=allow_empty
        )

    validate_build_log(logs["release_build"], "release build log")
    validate_build_log(logs["audit_build"], "audit build log")
    qualification = validate_qualification(
        parse_single_line_json_object(logs["qualification"], "qualification log")
    )
    if logs["qualification"] != canonical_json(qualification):
        fail("qualification log must be canonical JSON")
    architectures = validate_elf_log(logs["cuobjdump_elf"])
    if logs["cuobjdump_ptx"].strip():
        fail("PTX stdout evidence must be empty")
    validate_memcheck_log(logs["memcheck"])
    validate_racecheck_log(logs["racecheck"])
    validate_sanitizer_qualification_replay(
        logs["memcheck"], "memcheck log", qualification
    )
    validate_sanitizer_qualification_replay(
        logs["racecheck"], "racecheck log", qualification
    )

    artifact = {
        "artifact_role": ARTIFACT_ROLE,
        "backend": BACKEND,
        "binary": {
            "qualification_sha256": sha256_file(
                args.binary,
                "Phase 15 exact diametral-Phi qualification binary",
            )
        },
        "checks": {
            "aot_elf_architectures": architectures,
            "aot_ptx_entry_count": 0,
            "audit_build": "passed",
            "cuda_audit_workflow": "passed",
            "cuda_release_workflow": "passed",
            "memcheck": "passed",
            "qualification": qualification,
            "racecheck": "passed",
            "release_build": "passed",
            "sanitizer_qualification_replay": "passed",
        },
        "claims": dict(NO_CLAIMS),
        "component_only": True,
        "decision_semantics": DECISION_SEMANTICS,
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
