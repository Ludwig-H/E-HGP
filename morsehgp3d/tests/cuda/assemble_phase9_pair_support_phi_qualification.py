#!/usr/bin/env python3
"""Validate and assemble the bounded Phase 9 pair-support Phi G4 evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys
from typing import Any, NoReturn, Sequence

from assemble_phase7_h_polytope_qualification import (
    BASE_IMAGE_REF,
    IMAGE_ID_RE,
    PHASE3_SCHEMA,
    SHA_RE,
    WORKER_LIFECYCLE,
    read_text_evidence,
    parse_json_object,
    require_exact_keys,
    sha256_file,
    validate_elf_log,
    validate_environment,
    validate_memcheck_log,
    validate_racecheck_log,
    write_exclusive_atomic,
)


SCHEMA = "morsehgp3d.phase9.pair_support_phi_cuda_g4_qualification.v1"
QUALIFICATION_SCHEMA = (
    "morsehgp3d.phase9.pair_support_phi_cuda_qualification.v1"
)
SCIENTIFIC_SCOPE = "bounded_pair_support_phi_proposal_and_exact_receipt_only"
PROPOSAL_SEMANTICS = (
    "cuda_outward_binary64_diametral_phi_upper_bound_proposal_only"
)
DECISION_SEMANTICS = (
    "cpu_exact_dyadic_aabb_phi_recertified_strict_witness_receipt_only"
)
QUALIFICATION_KEYS = {
    "backend",
    "certified_receipt_count",
    "decision_semantics",
    "descend_count",
    "deterministic",
    "first_epoch",
    "global_support_product_prune_published",
    "mode",
    "node_count",
    "phase",
    "profile",
    "proposal_digest_fnv1a",
    "proposal_semantics",
    "public_status",
    "schema",
    "second_epoch",
    "strict_interior_count",
}


def fail(message: str) -> NoReturn:
    raise SystemExit(message)


def require_exact_integer(value: Any, expected: int, label: str) -> None:
    if type(value) is not int or value != expected:
        fail(f"{label} must be exactly {expected}")


def validate_qualification(value: dict[str, Any]) -> dict[str, Any]:
    require_exact_keys(value, QUALIFICATION_KEYS, "qualification result")
    expected_scalars = {
        "backend": "cuda_g4",
        "decision_semantics": DECISION_SEMANTICS,
        "mode": "certified",
        "phase": "9",
        "profile": "hgp_reduced",
        "proposal_semantics": PROPOSAL_SEMANTICS,
        "schema": QUALIFICATION_SCHEMA,
    }
    for field, expected in expected_scalars.items():
        if value.get(field) != expected:
            fail(f"qualification result.{field} must be {expected}")
    for field, expected in {
        "certified_receipt_count": 1,
        "descend_count": 1,
        "first_epoch": 1,
        "node_count": 5,
        "second_epoch": 2,
        "strict_interior_count": 1,
    }.items():
        require_exact_integer(value.get(field), expected, f"qualification {field}")
    if value.get("deterministic") is not True:
        fail("qualification deterministic must be true")
    if value.get("global_support_product_prune_published") is not False:
        fail("qualification must not publish a global support-product prune")
    if value.get("public_status") is not None:
        fail("qualification public_status must be null")
    digest = value.get("proposal_digest_fnv1a")
    if type(digest) is not int or not 0 <= digest <= (1 << 64) - 1:
        fail("qualification proposal_digest_fnv1a must be an unsigned 64-bit integer")
    return value


def parse_arguments(arguments: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--git-sha", required=True)
    parser.add_argument("--base-image-ref", required=True)
    parser.add_argument("--image-ref", required=True)
    parser.add_argument("--image-id", required=True)
    parser.add_argument("--environment-artifact", type=Path, required=True)
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

    environment_raw, environment_sha256 = read_text_evidence(
        args.environment_artifact, "Phase 3 environment artifact"
    )
    environment = parse_json_object(environment_raw, "Phase 3 environment artifact")
    lifecycle = validate_environment(
        environment,
        git_sha=args.git_sha,
        base_image_ref=args.base_image_ref,
        image_ref=args.image_ref,
        image_id=args.image_id,
    )

    evidence_specs = {
        "qualification": (args.qualification_log, "qualification log", False),
        "cuobjdump_elf": (args.elf_log, "cuobjdump ELF log", False),
        "cuobjdump_ptx": (args.ptx_log, "cuobjdump PTX log", True),
        "cuobjdump_ptx_stderr": (
            args.ptx_stderr_log,
            "cuobjdump PTX stderr log",
            True,
        ),
        "memcheck": (args.memcheck_log, "memcheck log", False),
        "racecheck": (args.racecheck_log, "racecheck log", False),
    }
    logs: dict[str, str] = {}
    log_sha256: dict[str, str] = {}
    for name, (path, label, allow_empty) in evidence_specs.items():
        logs[name], log_sha256[name] = read_text_evidence(
            path, label, allow_empty=allow_empty
        )

    qualification = validate_qualification(
        parse_json_object(logs["qualification"], "qualification log")
    )
    canonical_qualification = json.dumps(
        qualification, ensure_ascii=False, sort_keys=True, separators=(",", ":")
    ) + "\n"
    if logs["qualification"] != canonical_qualification:
        fail("qualification log must be one canonical JSON line")
    architectures = validate_elf_log(logs["cuobjdump_elf"])
    if logs["cuobjdump_ptx"].strip():
        fail("PTX stdout evidence must be empty")
    validate_memcheck_log(logs["memcheck"])
    validate_racecheck_log(logs["racecheck"])

    artifact = {
        "backend": "cuda_g4",
        "binary": {
            "qualification_sha256": sha256_file(
                args.binary, "Phase 9 pair-support Phi qualification binary"
            ),
        },
        "checks": {
            "aot_elf_architectures": architectures,
            "aot_ptx_entry_count": 0,
            "cpu_exact_recertification": "passed",
            "cuda_audit_workflow": "passed",
            "cuda_release_workflow": "passed",
            "memcheck": "passed",
            "qualification": qualification,
            "racecheck": "passed",
        },
        "decision_semantics": DECISION_SEMANTICS,
        "git": {"clean": True, "sha": args.git_sha},
        "image": {
            "base_ref": args.base_image_ref,
            "id": args.image_id,
            "ref": args.image_ref,
        },
        "log_sha256": log_sha256,
        "logs": logs,
        "mode": "certified",
        "phase": "9",
        "profile": "hgp_reduced",
        "proposal_semantics": PROPOSAL_SEMANTICS,
        "provenance": {
            "environment_artifact_schema": PHASE3_SCHEMA,
            "environment_artifact_sha256": environment_sha256,
        },
        "schema": SCHEMA,
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "scientific_scope": SCIENTIFIC_SCOPE,
        "status": "worker_passed_pending_shutdown",
        "vm_lifecycle": lifecycle,
    }
    write_exclusive_atomic(args.output, artifact)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
