#!/usr/bin/env python3
"""Assemble the guarded bounded A×B×W CUDA qualification evidence."""

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


SCHEMA = "morsehgp3d.phase15.exact_pair_block_witness_cuda_g4_qualification.v2"
ARTIFACT_ROLE = "bounded_exact_AxBxW_witness_component_qualification"
BACKEND = "cuda_g4"
PROFILE = "hgp_reduced"
MODE = "native_AxBxW_n128_differential"
SCIENTIFIC_SCOPE = "bounded_AxBxW_exact_witness_sign_component_only"
POINT_COUNT = 128
TASK_COUNT = POINT_COUNT * (POINT_COUNT - 1) // 2
K_MAX = 5
MAXIMUM_CLOSED_RANK = K_MAX + 1
FIXTURE_PROFILE = "signed_zero_subnormal_cancellation_finite_extremes"
BINARY_RELATIVE_PATH = (
    "build/morsehgp3d-cuda-release/"
    "morsehgp3d_gpu_exact_pair_block_witness_cuda_qualification"
)
BINARY_CONTAINER_PATH = f"/workspace/repository/{BINARY_RELATIVE_PATH}"
QUALIFICATION_COMMAND = [BINARY_CONTAINER_PATH]
QUALIFICATION_KEYS = {
    "all_multileaf_AxBxW_nonresidual_task_count",
    "all_multileaf_AxBxW_task_count",
    "all_nonresidual_decisions_match_exact_oracle",
    "backend",
    "certified_count",
    "completed_result_digest",
    "directed_ambiguous_count",
    "directed_arithmetic_overflow_count",
    "directed_negative_count",
    "directed_nonnegative_count",
    "directed_nonnegative_short_circuit_count",
    "fixture_profile",
    "fixed_limb_residual_count",
    "fixed_limb_evaluation_count",
    "global_coverage_closed",
    "k_max",
    "kernel_elapsed_nanoseconds",
    "maximum_closed_rank",
    "mismatch_count",
    "mode",
    "multileaf_first_support_task_count",
    "multileaf_second_support_task_count",
    "multileaf_witness_task_count",
    "nonnegative_count",
    "point_count",
    "profile",
    "qualified",
    "rank_sufficient_witness_task_count",
    "required_witness_point_count",
    "schema_version",
    "submitted_task_digest",
    "subnormal_negative_completed_result_digest",
    "subnormal_negative_directed_ambiguous",
    "subnormal_negative_kernel_elapsed_nanoseconds",
    "subnormal_negative_qualified",
    "subnormal_negative_submitted_task_digest",
    "support_mass_gt_one_task_count",
    "task_count",
    "wall_nanoseconds",
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
CLAIMS = {
    "catalog_complete": False,
    "double_buffered_transactional_frontier": False,
    "full_gamma2": False,
    "hierarchy_or_tree": False,
    "industrial_scale": False,
    "public_status_claimed": False,
    "slo_claimed": False,
}
PHASE_CONTEXT = {
    "backend": "reference_cpu",
    "deployment_status": "architecture_only",
    "mode": "budgeted_component_only_exact_pair_block_frontier",
    "profile": PROFILE,
    "public_status": "not_claimed",
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
BUILD_FAILURE_RE = re.compile(
    r"(?:^|\n)(?:FAILED:|ninja: build stopped:|CMake Error(?: at|:)|"
    r"[^\n]*(?:fatal error:|undefined reference to|collect2: error:|"
    r"ld(?:\.lld)?: error:))",
    re.IGNORECASE,
)


def fail(message: str) -> NoReturn:
    raise SystemExit(message)


def canonical_json(value: dict[str, Any]) -> str:
    return json.dumps(
        value, ensure_ascii=False, sort_keys=True, separators=(",", ":")
    ) + "\n"


def require_int(value: Any, label: str, *, minimum: int = 0) -> int:
    if type(value) is not int or value < minimum:
        fail(f"{label} must be an integer >= {minimum}")
    return value


def require_uint64(value: Any, label: str) -> int:
    result = require_int(value, label)
    if result > (1 << 64) - 1:
        fail(f"{label} must fit in uint64")
    return result


def validate_qualification(value: dict[str, Any]) -> dict[str, Any]:
    """Validate the binary's closed, bounded differential summary."""

    require_exact_keys(value, QUALIFICATION_KEYS, "A×B×W qualification")
    for field, expected in {
        "backend": BACKEND,
        "fixture_profile": FIXTURE_PROFILE,
        "k_max": K_MAX,
        "maximum_closed_rank": MAXIMUM_CLOSED_RANK,
        "mode": MODE,
        "point_count": POINT_COUNT,
        "profile": PROFILE,
        "required_witness_point_count": K_MAX,
        "schema_version": 2,
        "task_count": TASK_COUNT,
    }.items():
        if value.get(field) != expected:
            fail(f"qualification {field} must be {expected}")
    if type(value.get("qualified")) is not bool or not value["qualified"]:
        fail("qualification must be true")
    if (
        type(value.get("all_nonresidual_decisions_match_exact_oracle")) is not bool
        or not value["all_nonresidual_decisions_match_exact_oracle"]
    ):
        fail("all non-residual CUDA decisions must match the exact host oracle")
    if type(value.get("global_coverage_closed")) is not bool or value[
        "global_coverage_closed"
    ]:
        fail("the bounded component must not close global pair coverage")
    if require_int(value.get("mismatch_count"), "mismatch_count") != 0:
        fail("the native CUDA batch differs from the exact host oracle")

    certified = require_int(value.get("certified_count"), "certified_count", minimum=1)
    nonnegative = require_int(
        value.get("nonnegative_count"), "nonnegative_count", minimum=1
    )
    residual = require_int(
        value.get("fixed_limb_residual_count"), "fixed_limb_residual_count"
    )
    if certified + nonnegative + residual != TASK_COUNT:
        fail("certified, nonnegative and fail-closed residual tasks do not partition the batch")
    directed_negative = require_int(
        value.get("directed_negative_count"), "directed_negative_count"
    )
    directed_nonnegative = require_int(
        value.get("directed_nonnegative_count"),
        "directed_nonnegative_count",
        minimum=1,
    )
    directed_ambiguous = require_int(
        value.get("directed_ambiguous_count"), "directed_ambiguous_count"
    )
    directed_arithmetic_overflow = require_int(
        value.get("directed_arithmetic_overflow_count"),
        "directed_arithmetic_overflow_count",
    )
    shortcut = require_int(
        value.get("directed_nonnegative_short_circuit_count"),
        "directed_nonnegative_short_circuit_count",
        minimum=1,
    )
    fixed_limb_evaluations = require_int(
        value.get("fixed_limb_evaluation_count"),
        "fixed_limb_evaluation_count",
    )
    if (
        directed_negative
        + directed_nonnegative
        + directed_ambiguous
        + directed_arithmetic_overflow
        != TASK_COUNT
    ):
        fail("directed proposal counters do not partition submitted tasks")
    if shortcut != directed_nonnegative:
        fail("every directed-nonnegative proposal must take the fail-open shortcut")
    if fixed_limb_evaluations != TASK_COUNT - shortcut:
        fail("fixed-limb evaluations do not equal non-shortcut tasks")

    multileaf_first = require_int(
        value.get("multileaf_first_support_task_count"),
        "multileaf_first_support_task_count",
        minimum=1,
    )
    multileaf_second = require_int(
        value.get("multileaf_second_support_task_count"),
        "multileaf_second_support_task_count",
        minimum=1,
    )
    multileaf_witness = require_int(
        value.get("multileaf_witness_task_count"),
        "multileaf_witness_task_count",
        minimum=1,
    )
    all_multileaf = require_int(
        value.get("all_multileaf_AxBxW_task_count"),
        "all_multileaf_AxBxW_task_count",
        minimum=1,
    )
    all_multileaf_nonresidual = require_int(
        value.get("all_multileaf_AxBxW_nonresidual_task_count"),
        "all_multileaf_AxBxW_nonresidual_task_count",
        minimum=1,
    )
    support_mass_gt_one = require_int(
        value.get("support_mass_gt_one_task_count"),
        "support_mass_gt_one_task_count",
        minimum=1,
    )
    bounded_task_counters = {
        "multileaf_first_support_task_count": multileaf_first,
        "multileaf_second_support_task_count": multileaf_second,
        "multileaf_witness_task_count": multileaf_witness,
        "all_multileaf_AxBxW_task_count": all_multileaf,
        "all_multileaf_AxBxW_nonresidual_task_count": all_multileaf_nonresidual,
        "support_mass_gt_one_task_count": support_mass_gt_one,
    }
    for label, count in bounded_task_counters.items():
        if count > TASK_COUNT:
            fail(f"{label} exceeds submitted task count")
    if all_multileaf > min(multileaf_first, multileaf_second, multileaf_witness):
        fail("all-multileaf A×B×W tasks are not a subset of each multileaf counter")
    if all_multileaf_nonresidual > all_multileaf:
        fail("non-residual all-multileaf tasks exceed all-multileaf tasks")
    if all_multileaf - all_multileaf_nonresidual > residual:
        fail("all-multileaf residual tasks exceed the batch residual count")
    if all_multileaf > support_mass_gt_one:
        fail("all-multileaf support tasks must have unordered pair mass > 1")
    if require_int(
        value.get("rank_sufficient_witness_task_count"),
        "rank_sufficient_witness_task_count",
    ) != TASK_COUNT:
        fail("every task must carry the five-point witness required for closed rank 6")
    require_uint64(value.get("submitted_task_digest"), "submitted_task_digest")
    require_uint64(value.get("completed_result_digest"), "completed_result_digest")
    require_uint64(
        value.get("subnormal_negative_submitted_task_digest"),
        "subnormal_negative_submitted_task_digest",
    )
    require_uint64(
        value.get("subnormal_negative_completed_result_digest"),
        "subnormal_negative_completed_result_digest",
    )
    if (
        type(value.get("subnormal_negative_qualified")) is not bool
        or not value["subnormal_negative_qualified"]
        or type(value.get("subnormal_negative_directed_ambiguous")) is not bool
        or not value["subnormal_negative_directed_ambiguous"]
    ):
        fail("the negative subnormal underflow guard must pass via the ambiguous fixed-limb path")
    require_int(
        value.get("subnormal_negative_kernel_elapsed_nanoseconds"),
        "subnormal_negative_kernel_elapsed_nanoseconds",
        minimum=1,
    )
    require_int(
        value.get("kernel_elapsed_nanoseconds"),
        "kernel_elapsed_nanoseconds",
        minimum=1,
    )
    require_int(value.get("wall_nanoseconds"), "wall_nanoseconds", minimum=1)
    return value


def parse_single_json_line(raw: str, label: str) -> dict[str, Any]:
    lines = raw.splitlines()
    if len(lines) != 1 or not lines[0]:
        fail(f"{label} must contain exactly one non-empty JSON line")
    value = parse_json_object(lines[0], label)
    if raw != canonical_json(value):
        fail(f"{label} must be canonical JSON")
    return value


def extract_sanitizer_qualification(raw: str, label: str) -> dict[str, Any]:
    candidates = [line for line in raw.splitlines() if line.startswith("{")]
    if len(candidates) != 1:
        fail(f"{label} must contain exactly one qualification JSON line")
    return validate_qualification(
        parse_json_object(candidates[0], f"{label} qualification")
    )


def validate_equivalence(*values: dict[str, Any]) -> None:
    ignored = {"kernel_elapsed_nanoseconds", "wall_nanoseconds"}
    expected = {key: value for key, value in values[0].items() if key not in ignored}
    for label, candidate in zip(("memcheck", "racecheck"), values[1:], strict=True):
        observed = {key: value for key, value in candidate.items() if key not in ignored}
        if observed != expected:
            fail(f"{label} changed the bounded exact differential result")


def validate_build_log(value: str, label: str) -> None:
    if not value.strip() or BUILD_FAILURE_RE.search(value) is not None:
        fail(f"{label} is empty or contains a build failure")


def validate_qualification_stderr(value: str) -> None:
    """The split-output worker isolates application stderr from infrastructure."""

    if value != "":
        fail("qualification application stderr must be empty")


def validate_environment_artifact(
    path: Path,
    *,
    git_sha: str,
    base_image_ref: str,
    image_ref: str,
    image_id: str,
) -> str:
    raw, digest = read_text_evidence(path, "Phase 3 environment artifact")
    validate_environment(
        parse_json_object(raw, "Phase 3 environment artifact"),
        git_sha=git_sha,
        base_image_ref=base_image_ref,
        image_ref=image_ref,
        image_id=image_id,
    )
    return digest


def _validate_embedded_evidence(value: dict[str, Any]) -> None:
    logs = value.get("logs")
    digests = value.get("log_sha256")
    if not isinstance(logs, dict) or not isinstance(digests, dict):
        fail("logs or log digests are absent")
    require_exact_keys(logs, LOG_KEYS, "A×B×W logs")
    require_exact_keys(digests, LOG_KEYS, "A×B×W log digests")
    for name in LOG_KEYS:
        log = logs.get(name)
        if not isinstance(log, str):
            fail(f"log {name} must be text")
        if digests.get(name) != hashlib.sha256(log.encode("utf-8")).hexdigest():
            fail(f"log digest differs for {name}")
    validate_qualification_stderr(logs["qualification_stderr"])
    validate_build_log(logs["release_build"], "release build log")
    validate_build_log(logs["audit_build"], "audit build log")
    direct = validate_qualification(
        parse_single_json_line(logs["qualification"], "qualification log")
    )
    architectures = validate_elf_log(logs["cuobjdump_elf"])
    if architectures != ["sm_120"]:
        fail("qualification binary must contain only sm_120 ELF code")
    if logs["cuobjdump_ptx"].strip():
        fail("PTX stdout evidence must be empty")
    validate_memcheck_log(logs["memcheck"])
    validate_racecheck_log(logs["racecheck"])
    memcheck = extract_sanitizer_qualification(logs["memcheck"], "memcheck log")
    racecheck = extract_sanitizer_qualification(logs["racecheck"], "racecheck log")
    validate_equivalence(direct, memcheck, racecheck)
    expected_checks = {
        "aot_elf_architectures": ["sm_120"],
        "aot_ptx_entry_count": 0,
        "audit_build": "passed",
        "cuda_audit_workflow": "passed",
        "cuda_release_workflow": "passed",
        "memcheck": "passed",
        "memcheck_qualification": memcheck,
        "qualification": direct,
        "qualification_stderr_empty": True,
        "racecheck": "passed",
        "racecheck_qualification": racecheck,
        "release_build": "passed",
        "sanitizer_bounded_differential_equivalence": "passed",
    }
    if value.get("checks") != expected_checks:
        fail("artifact checks differ from embedded evidence")


def validate_artifact(
    value: dict[str, Any],
    raw: str,
    *,
    git_sha: str,
    environment_artifact_path: Path,
) -> dict[str, Any]:
    require_exact_keys(value, ARTIFACT_KEYS, "A×B×W artifact")
    if raw != canonical_json(value):
        fail("A×B×W artifact must be canonical JSON")
    for field, expected in {
        "artifact_role": ARTIFACT_ROLE,
        "backend": BACKEND,
        "component_only": True,
        "deployment_status": "component_only",
        "mode": MODE,
        "phase": "15",
        "profile": PROFILE,
        "schema": SCHEMA,
        "scientific_result_claimed": False,
        "scientific_scope": SCIENTIFIC_SCOPE,
        "status": "worker_passed_pending_shutdown",
    }.items():
        if value.get(field) != expected:
            fail(f"artifact {field} must be {expected}")
    if value.get("scientific_public_status") is not None:
        fail("scientific_public_status must be null")
    if value.get("phase_context") != PHASE_CONTEXT or value.get("claims") != CLAIMS:
        fail("the component published an unauthorized phase or product claim")
    if value.get("commands") != {
        "memcheck_application": QUALIFICATION_COMMAND,
        "qualification": QUALIFICATION_COMMAND,
        "racecheck_application": QUALIFICATION_COMMAND,
    }:
        fail("guarded commands differ")
    if value.get("git") != {"clean": True, "sha": git_sha}:
        fail("artifact does not bind the qualified clean SHA")
    image = value.get("image")
    if not isinstance(image, dict):
        fail("image identity is absent")
    require_exact_keys(image, {"base_ref", "id", "ref"}, "image identity")
    if image.get("base_ref") != BASE_IMAGE_REF or image.get("ref") != (
        f"morsehgp3d-phase3:{git_sha}"
    ):
        fail("image identity does not bind the qualified SHA")
    if not isinstance(image.get("id"), str) or IMAGE_ID_RE.fullmatch(image["id"]) is None:
        fail("image ID is not canonical")
    environment_sha256 = validate_environment_artifact(
        environment_artifact_path,
        git_sha=git_sha,
        base_image_ref=image["base_ref"],
        image_ref=image["ref"],
        image_id=image["id"],
    )
    if value.get("provenance") != {
        "environment_artifact_schema": PHASE3_SCHEMA,
        "environment_artifact_sha256": environment_sha256,
    }:
        fail("artifact provenance does not bind Phase 3 evidence")
    if value.get("vm_lifecycle") != WORKER_LIFECYCLE:
        fail("artifact lifecycle must remain pending targeted shutdown")
    binary = value.get("binary")
    if not isinstance(binary, dict):
        fail("binary evidence is absent")
    require_exact_keys(binary, {"qualification_sha256"}, "binary evidence")
    if re.fullmatch(r"[0-9a-f]{64}", str(binary.get("qualification_sha256"))) is None:
        fail("qualification binary digest is not canonical")
    _validate_embedded_evidence(value)
    return value


def validate_artifact_file(
    artifact_path: Path,
    *,
    git_sha: str,
    environment_artifact_path: Path,
) -> dict[str, Any]:
    raw, _ = read_text_evidence(artifact_path, "A×B×W qualification artifact")
    return validate_artifact(
        parse_json_object(raw, "A×B×W qualification artifact"),
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
        fail("--git-sha must be a canonical lowercase commit ID")
    if args.base_image_ref != BASE_IMAGE_REF:
        fail("--base-image-ref is not pinned")
    if args.image_ref != f"morsehgp3d-phase3:{args.git_sha}":
        fail("--image-ref does not bind the qualified SHA")
    if IMAGE_ID_RE.fullmatch(args.image_id) is None:
        fail("--image-id is not canonical")
    environment_sha256 = validate_environment_artifact(
        args.environment_artifact,
        git_sha=args.git_sha,
        base_image_ref=args.base_image_ref,
        image_ref=args.image_ref,
        image_id=args.image_id,
    )
    specs = {
        "audit_build": (args.audit_build_log, False),
        "cuobjdump_elf": (args.elf_log, False),
        "cuobjdump_ptx": (args.ptx_log, True),
        "cuobjdump_ptx_stderr": (args.ptx_stderr_log, True),
        "memcheck": (args.memcheck_log, False),
        "qualification": (args.qualification_log, False),
        "qualification_stderr": (args.qualification_stderr_log, True),
        "racecheck": (args.racecheck_log, False),
        "release_build": (args.release_build_log, False),
    }
    logs: dict[str, str] = {}
    digests: dict[str, str] = {}
    for name, (path, allow_empty) in specs.items():
        logs[name], digests[name] = read_text_evidence(
            path, f"{name} log", allow_empty=allow_empty
        )
    artifact = {
        "artifact_role": ARTIFACT_ROLE,
        "backend": BACKEND,
        "binary": {
            "qualification_sha256": sha256_file(args.binary, "qualification binary")
        },
        "checks": {},
        "claims": dict(CLAIMS),
        "commands": {
            "memcheck_application": list(QUALIFICATION_COMMAND),
            "qualification": list(QUALIFICATION_COMMAND),
            "racecheck_application": list(QUALIFICATION_COMMAND),
        },
        "component_only": True,
        "deployment_status": "component_only",
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
    direct = validate_qualification(
        parse_single_json_line(logs["qualification"], "qualification log")
    )
    validate_memcheck_log(logs["memcheck"])
    validate_racecheck_log(logs["racecheck"])
    memcheck = extract_sanitizer_qualification(logs["memcheck"], "memcheck log")
    racecheck = extract_sanitizer_qualification(logs["racecheck"], "racecheck log")
    artifact["checks"] = {
        "aot_elf_architectures": validate_elf_log(logs["cuobjdump_elf"]),
        "aot_ptx_entry_count": 0,
        "audit_build": "passed",
        "cuda_audit_workflow": "passed",
        "cuda_release_workflow": "passed",
        "memcheck": "passed",
        "memcheck_qualification": memcheck,
        "qualification": direct,
        "qualification_stderr_empty": True,
        "racecheck": "passed",
        "racecheck_qualification": racecheck,
        "release_build": "passed",
        "sanitizer_bounded_differential_equivalence": "passed",
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
