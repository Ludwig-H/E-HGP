#!/usr/bin/env python3
"""Validate and assemble the provisional Phase 5 K1 Boruvka G4 evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
from typing import Any, NoReturn, Sequence

SCHEMA = "morsehgp3d.phase5.k1_boruvka_gpu_qualification.v1"
REPLAY_SCHEMA = "morsehgp3d.phase5.k1_boruvka_gpu_replay.v1"
PHASE3_SCHEMA = "morsehgp3d.phase3.qualification.v1"
SCIENTIFIC_SCOPE = "gpu_candidate_superset_with_cpu_exact_resolution_only"
PROPOSAL_SEMANTICS = "gpu_stackless_lbvh_fixed_seed_candidate_superset"
DECISION_SEMANTICS = "cpu_exact_seed_replay_and_kappa_resolution"
BASE_IMAGE_REF = (
    "nvidia/cuda:12.9.2-devel-ubuntu24.04@"
    "sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"
)
SHA_RE = re.compile(r"[0-9a-f]{40}\Z")
IMAGE_ID_RE = re.compile(r"sha256:[0-9a-f]{64}\Z")
SM_RE = re.compile(r"sm_[0-9]+")
SANITIZER_ERROR_RE = re.compile(r"ERROR SUMMARY:\s*([0-9]+) errors")
RACECHECK_SUMMARY_LINE_RE = re.compile(
    r"^[ \t]*=+[ \t]*RACECHECK SUMMARY:[ \t]*"
    r"([0-9]+) hazards? displayed[ \t]*"
    r"\([ \t]*([0-9]+) errors?,[ \t]*([0-9]+) warnings?[ \t]*\)"
    r"[ \t]*$",
    re.MULTILINE,
)
RACECHECK_FAILURE_RE = re.compile(
    r"Internal Sanitizer Error|Target application returned an error|"
    r"process (?:didn't|did not) terminate successfully|"
    r"No attachable process found|Potential [^\n]* hazard|Race reported",
    re.IGNORECASE,
)
REPLAY_KEYS = {
    "candidate_count",
    "case_count",
    "count_pass_node_visit_count",
    "decision_semantics",
    "point_count",
    "proposal_semantics",
    "required_candidate_count",
    "schema",
    "status",
    "strict_aabb_prune_count",
    "uniform_component_prune_count",
}
EXPECTED_REPLAY = {
    "candidate_count": 15,
    "case_count": 2,
    "count_pass_node_visit_count": 48,
    "decision_semantics": DECISION_SEMANTICS,
    "point_count": 8,
    "proposal_semantics": PROPOSAL_SEMANTICS,
    "required_candidate_count": 15,
    "schema": REPLAY_SCHEMA,
    "status": "passed",
    "strict_aabb_prune_count": 5,
    "uniform_component_prune_count": 8,
}
WORKER_LIFECYCLE = {
    "guest_shutdown_guard_verified": True,
    "stop_responsibility": "external_orchestrator",
    "worker_mutates_gcp": False,
}


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


def require_exact_keys(value: dict[str, Any], expected: set[str], label: str) -> None:
    actual = set(value)
    if actual != expected:
        fail(
            f"{label} keys differ: missing={sorted(expected - actual)}, "
            f"unexpected={sorted(actual - expected)}"
        )


def read_json_object(path: Path, label: str) -> tuple[str, dict[str, Any]]:
    try:
        raw = path.read_text(encoding="utf-8")
        value = json.loads(raw, object_pairs_hook=reject_duplicate_json_keys)
    except (OSError, UnicodeError, json.JSONDecodeError, ValueError) as error:
        fail(f"cannot read {label} {path}: {error}")
    if not isinstance(value, dict):
        fail(f"{label} must be a JSON object")
    return raw, value


def read_text(path: Path, label: str, *, allow_empty: bool = False) -> str:
    try:
        value = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        fail(f"cannot read {label} {path}: {error}")
    if not allow_empty and not value.strip():
        fail(f"{label} is empty")
    return value


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


def validate_replay(path: Path) -> tuple[str, dict[str, Any]]:
    raw, value = read_json_object(path, "Phase 5 K1 Boruvka replay")
    canonical = json.dumps(
        value, ensure_ascii=True, sort_keys=True, separators=(",", ":")
    )
    if raw != canonical + "\n":
        fail("Phase 5 K1 Boruvka replay must be canonical one-line JSON")
    require_exact_keys(value, REPLAY_KEYS, "Phase 5 K1 Boruvka replay")
    if value != EXPECTED_REPLAY:
        fail("Phase 5 K1 Boruvka replay differs from the closed real-GPU fixture")
    return raw, value


def validate_memcheck_log(value: str) -> None:
    summaries = [int(match) for match in SANITIZER_ERROR_RE.findall(value)]
    if not summaries or any(count != 0 for count in summaries):
        fail("memcheck evidence must contain only zero-error summaries")


def validate_racecheck_log(value: str) -> None:
    summary_lines = [
        line for line in value.splitlines() if "RACECHECK SUMMARY:" in line
    ]
    summaries = [
        tuple(int(count) for count in match.groups())
        for match in RACECHECK_SUMMARY_LINE_RE.finditer(value)
    ]
    invalid = (
        not summaries
        or len(summary_lines) != len(summaries)
        or any(summary != (0, 0, 0) for summary in summaries)
        or SANITIZER_ERROR_RE.search(value) is not None
        or RACECHECK_FAILURE_RE.search(value) is not None
    )
    if invalid:
        fail(
            "racecheck evidence must contain only zero-hazard, zero-error, "
            "zero-warning summaries"
        )


def validate_binary_logs(
    *,
    elf_log: str,
    ptx_log: str,
    memcheck_log: str,
    racecheck_log: str,
) -> list[str]:
    architectures = SM_RE.findall(elf_log)
    if not architectures or set(architectures) != {"sm_120"}:
        fail("ELF evidence must be non-empty and contain only sm_120")
    if ptx_log.strip():
        fail("PTX evidence must contain zero entries")
    validate_memcheck_log(memcheck_log)
    validate_racecheck_log(racecheck_log)
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
    if value.get("scientific_result_claimed") is not False:
        fail("environment artifact must not claim a scientific result")
    if value.get("scientific_public_status") is not None:
        fail("environment artifact scientific_public_status must be null")
    if "public_status" in value:
        fail("environment artifact must not expose public_status")
    if value.get("git") != {"clean": True, "sha": git_sha}:
        fail("environment artifact does not bind the qualified Git SHA")
    if value.get("vm_lifecycle") != WORKER_LIFECYCLE:
        fail("environment artifact does not preserve the non-mutating worker lifecycle")

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
    if (
        manifest.get("compiled_sm") != "sm_120"
        or manifest.get("gpu_runtime_sm") != "sm_120"
    ):
        fail("environment artifact is not compiled and executed on sm_120")
    if (
        manifest.get("gpu_compute_capability_major"),
        manifest.get("gpu_compute_capability_minor"),
    ) != (12, 0):
        fail("environment artifact compute capability differs from 12.0")
    return manifest


def parse_arguments(arguments: Sequence[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--git-sha", required=True)
    parser.add_argument("--base-image-ref", required=True)
    parser.add_argument("--image-ref", required=True)
    parser.add_argument("--image-id", required=True)
    parser.add_argument("--environment-artifact", type=Path, required=True)
    parser.add_argument("--replay-log", type=Path, required=True)
    parser.add_argument("--elf-log", type=Path, required=True)
    parser.add_argument("--ptx-log", type=Path, required=True)
    parser.add_argument("--ptx-stderr-log", type=Path, required=True)
    parser.add_argument("--memcheck-log", type=Path, required=True)
    parser.add_argument("--racecheck-log", type=Path, required=True)
    parser.add_argument("--replay", type=Path, required=True)
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

    _, environment = read_json_object(
        args.environment_artifact, "Phase 3 environment artifact"
    )
    manifest = validate_phase3_environment(
        environment,
        git_sha=args.git_sha,
        base_image_ref=args.base_image_ref,
        image_ref=args.image_ref,
        image_id=args.image_id,
    )
    replay_raw, replay = validate_replay(args.replay_log)
    elf_log = read_text(args.elf_log, "cuobjdump ELF log")
    ptx_log = read_text(args.ptx_log, "cuobjdump PTX log", allow_empty=True)
    ptx_stderr_log = read_text(
        args.ptx_stderr_log,
        "cuobjdump PTX stderr log",
        allow_empty=True,
    )
    memcheck_log = read_text(args.memcheck_log, "memcheck log")
    racecheck_log = read_text(args.racecheck_log, "racecheck log")
    architectures = validate_binary_logs(
        elf_log=elf_log,
        ptx_log=ptx_log,
        memcheck_log=memcheck_log,
        racecheck_log=racecheck_log,
    )

    artifact = {
        "backend": "cuda_g4",
        "binary": {
            "replay_sha256": sha256_file(
                args.replay, "Phase 5 K1 Boruvka replay binary"
            ),
        },
        "checks": {
            "aot_elf_architectures": sorted(set(architectures)),
            "aot_ptx_entry_count": 0,
            "cpu_exact_resolution_complete": True,
            "gpu_candidate_superset_certified": True,
            "memcheck": "passed",
            "racecheck": "passed",
            "replay": replay,
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
            "cuobjdump_elf": elf_log,
            "cuobjdump_ptx": ptx_log,
            "cuobjdump_ptx_stderr": ptx_stderr_log,
            "memcheck": memcheck_log,
            "racecheck": racecheck_log,
            "replay": replay_raw,
        },
        "mode": "certified",
        "phase": "5",
        "profile": "hgp_reduced",
        "provenance": {
            "environment_artifact_schema": PHASE3_SCHEMA,
            "environment_artifact_sha256": sha256_file(
                args.environment_artifact, "Phase 3 environment artifact"
            ),
        },
        "schema": SCHEMA,
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "scientific_scope": SCIENTIFIC_SCOPE,
        "status": "worker_passed_pending_shutdown",
        "vm_lifecycle": dict(WORKER_LIFECYCLE),
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
        fail(f"cannot write Phase 5 qualification artifact {output}: {error}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
