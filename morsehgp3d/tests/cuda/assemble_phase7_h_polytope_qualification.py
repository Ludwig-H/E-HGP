#!/usr/bin/env python3
"""Validate and assemble the bounded Phase 7.8 H-polytope G4 evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import tempfile
from typing import Any, NoReturn, Sequence


SCHEMA = "morsehgp3d.phase7.h_polytope_cuda_g4_qualification.v1"
QUALIFICATION_SCHEMA = "morsehgp3d.phase7.h_polytope_cuda_qualification.v1"
PHASE3_SCHEMA = "morsehgp3d.phase3.qualification.v1"
BASE_IMAGE_REF = (
    "nvidia/cuda:12.9.2-devel-ubuntu24.04@"
    "sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"
)
SCIENTIFIC_SCOPE = "proposal_only_h_polytope_transcript"
PROPOSAL_SEMANTICS = "proposal_only_exhaustive_plane_triple_transcript"
DECISION_SEMANTICS = "reference_cpu_exact_all_constraints"
SHA_RE = re.compile(r"[0-9a-f]{40}\Z")
IMAGE_ID_RE = re.compile(r"sha256:[0-9a-f]{64}\Z")
SM_TOKEN_RE = re.compile(r"\bsm_[A-Za-z0-9_]+\b")
MEMCHECK_SUMMARY_RE = re.compile(r"ERROR SUMMARY:\s*([0-9]+) errors?")
RACECHECK_SUMMARY_LINE_RE = re.compile(
    r"^[ \t]*=+[ \t]*RACECHECK SUMMARY:[ \t]*"
    r"([0-9]+) hazards? displayed[ \t]*"
    r"\([ \t]*([0-9]+) errors?,[ \t]*([0-9]+) warnings?[ \t]*\)"
    r"[ \t]*$",
    re.MULTILINE,
)
SANITIZER_FAILURE_RE = re.compile(
    r"Internal Sanitizer Error|Target application returned an error|"
    r"process (?:didn't|did not) terminate successfully|"
    r"No attachable process found|Program hit cudaError|"
    r"Segmentation fault|Invalid (?:__global__|__shared__|__local__) "
    r"(?:read|write)|Misaligned address|=========\s+(?:ERROR|FATAL):",
    re.IGNORECASE,
)
RACECHECK_FAILURE_RE = re.compile(
    r"Potential [^\n]* hazard|Race reported",
    re.IGNORECASE,
)
WORKER_LIFECYCLE = {
    "guest_shutdown_guard_verified": True,
    "stop_responsibility": "external_orchestrator",
    "worker_mutates_gcp": False,
}
QUALIFICATION_KEYS = {
    "cell_count",
    "deterministic",
    "exact_cpu_recertification",
    "first_epoch",
    "proposal_digest_fnv1a",
    "record_count",
    "schema",
    "second_epoch",
    "strict_reject_count",
    "survivor_count",
    "unknown_count",
}


def fail(message: str) -> NoReturn:
    raise SystemExit(message)


def reject_duplicate_json_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON object key: {key}")
        result[key] = value
    return result


def read_regular_bytes(path: Path, label: str) -> bytes:
    if not path.is_file() or path.is_symlink():
        fail(f"{label} must be a regular non-symbolic file: {path}")
    try:
        return path.read_bytes()
    except OSError as error:
        fail(f"cannot read {label} {path}: {error}")


def read_text_evidence(
    path: Path, label: str, *, allow_empty: bool = False
) -> tuple[str, str]:
    payload = read_regular_bytes(path, label)
    try:
        value = payload.decode("utf-8")
    except UnicodeError as error:
        fail(f"{label} is not valid UTF-8: {error}")
    if not allow_empty and not value.strip():
        fail(f"{label} is empty")
    return value, hashlib.sha256(payload).hexdigest()


def parse_json_object(raw: str, label: str) -> dict[str, Any]:
    try:
        value = json.loads(raw, object_pairs_hook=reject_duplicate_json_keys)
    except (json.JSONDecodeError, ValueError) as error:
        fail(f"{label} must contain exactly one valid JSON object: {error}")
    if not isinstance(value, dict):
        fail(f"{label} must contain exactly one JSON object")
    return value


def require_exact_keys(value: dict[str, Any], expected: set[str], label: str) -> None:
    actual = set(value)
    if actual != expected:
        fail(
            f"{label} keys differ: missing={sorted(expected - actual)}, "
            f"unexpected={sorted(actual - expected)}"
        )


def require_exact_integer(value: Any, expected: int, label: str) -> None:
    if type(value) is not int or value != expected:
        fail(f"{label} must be exactly {expected}")


def require_positive_integer(value: Any, label: str) -> int:
    if type(value) is not int or value <= 0:
        fail(f"{label} must be a positive integer")
    return value


def validate_qualification(value: dict[str, Any]) -> dict[str, Any]:
    require_exact_keys(value, QUALIFICATION_KEYS, "qualification result")
    if value.get("schema") != QUALIFICATION_SCHEMA:
        fail(f"qualification result.schema must be {QUALIFICATION_SCHEMA}")
    require_exact_integer(value.get("cell_count"), 4, "qualification cell_count")
    require_exact_integer(
        value.get("record_count"), 55, "qualification record_count"
    )
    counts = [
        require_positive_integer(value.get(field), f"qualification {field}")
        for field in ("unknown_count", "strict_reject_count", "survivor_count")
    ]
    if sum(counts) != 55:
        fail("qualification status counts must sum to 55")
    require_exact_integer(value.get("first_epoch"), 1, "qualification first_epoch")
    require_exact_integer(
        value.get("second_epoch"), 2, "qualification second_epoch"
    )
    for field in ("deterministic", "exact_cpu_recertification"):
        if value.get(field) is not True:
            fail(f"qualification {field} must be true")
    digest = value.get("proposal_digest_fnv1a")
    if type(digest) is not int or not 0 <= digest <= (1 << 64) - 1:
        fail("qualification proposal_digest_fnv1a must be an unsigned 64-bit integer")
    return value


def validate_environment(
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
    if value.get("scientific_result_claimed") is not False:
        fail("environment artifact must not claim a scientific result")
    if value.get("scientific_public_status") is not None:
        fail("environment artifact scientific_public_status must be null")
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
    checks = value.get("checks")
    if not isinstance(checks, dict):
        fail("environment artifact checks are absent")
    for workflow in ("cuda_audit_workflow", "cuda_release_workflow"):
        if checks.get(workflow) != "passed":
            fail(f"environment artifact {workflow} must be passed")
    lifecycle = value.get("vm_lifecycle")
    if lifecycle != WORKER_LIFECYCLE:
        fail("environment artifact lifecycle is not the guarded worker lifecycle")
    return dict(lifecycle)


def validate_elf_log(value: str) -> list[str]:
    architectures = SM_TOKEN_RE.findall(value)
    if not architectures or set(architectures) != {"sm_120"}:
        fail("ELF evidence must be non-empty and contain exactly sm_120")
    return ["sm_120"]


def validate_memcheck_log(value: str) -> None:
    summary_lines = [
        line for line in value.splitlines() if "ERROR SUMMARY:" in line
    ]
    summaries = [int(count) for count in MEMCHECK_SUMMARY_RE.findall(value)]
    if (
        not summaries
        or len(summary_lines) != len(summaries)
        or any(count != 0 for count in summaries)
        or SANITIZER_FAILURE_RE.search(value) is not None
    ):
        fail("memcheck evidence must contain only zero-error summaries and no failure")


def validate_racecheck_log(value: str) -> None:
    summary_lines = [
        line for line in value.splitlines() if "RACECHECK SUMMARY:" in line
    ]
    summaries = [
        tuple(int(count) for count in match.groups())
        for match in RACECHECK_SUMMARY_LINE_RE.finditer(value)
    ]
    if (
        not summaries
        or len(summary_lines) != len(summaries)
        or any(summary != (0, 0, 0) for summary in summaries)
        or "ERROR SUMMARY:" in value
        or SANITIZER_FAILURE_RE.search(value) is not None
        or RACECHECK_FAILURE_RE.search(value) is not None
    ):
        fail(
            "racecheck evidence must contain only zero-hazard, zero-error, "
            "zero-warning summaries and no failure"
        )


def sha256_file(path: Path, label: str) -> str:
    if not path.is_file() or path.is_symlink():
        fail(f"{label} must be a regular non-symbolic file: {path}")
    digest = hashlib.sha256()
    try:
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
    except OSError as error:
        fail(f"cannot hash {label} {path}: {error}")
    return digest.hexdigest()


def write_exclusive_atomic(path: Path, value: dict[str, Any]) -> None:
    if not path.is_absolute():
        fail("--output must be absolute")
    if path.exists() or path.is_symlink():
        fail(f"refusing to replace an existing artifact: {path}")
    declared_parent = path.parent
    try:
        physical_parent = declared_parent.resolve(strict=True)
    except OSError as error:
        fail(f"cannot resolve output parent {declared_parent}: {error}")
    if physical_parent != declared_parent or not physical_parent.is_dir():
        fail("--output parent must be an existing physical directory")

    encoded = json.dumps(
        value, ensure_ascii=False, sort_keys=True, separators=(",", ":")
    ).encode("utf-8") + b"\n"
    temporary = ""
    try:
        descriptor, temporary = tempfile.mkstemp(
            prefix=f".{path.name}.", suffix=".tmp", dir=physical_parent
        )
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(encoded)
            stream.flush()
            os.fsync(stream.fileno())
        os.link(temporary, path, follow_symlinks=False)
        os.unlink(temporary)
        temporary = ""
        directory_fd = os.open(
            physical_parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0)
        )
        try:
            os.fsync(directory_fd)
        finally:
            os.close(directory_fd)
    except FileExistsError:
        fail(f"refusing to replace an existing artifact: {path}")
    except OSError as error:
        fail(f"cannot publish Phase 7.8 qualification artifact {path}: {error}")
    finally:
        if temporary:
            try:
                os.unlink(temporary)
            except FileNotFoundError:
                pass


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
    architectures = validate_elf_log(logs["cuobjdump_elf"])
    if logs["cuobjdump_ptx"].strip():
        fail("PTX stdout evidence must be empty")
    validate_memcheck_log(logs["memcheck"])
    validate_racecheck_log(logs["racecheck"])

    artifact = {
        "backend": "cuda_g4",
        "binary": {
            "qualification_sha256": sha256_file(
                args.binary, "Phase 7.8 H-polytope qualification binary"
            ),
        },
        "checks": {
            "aot_elf_architectures": architectures,
            "aot_ptx_entry_count": 0,
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
        "mode": "benchmark_only",
        "phase": "7",
        "profile": "generic_core",
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
