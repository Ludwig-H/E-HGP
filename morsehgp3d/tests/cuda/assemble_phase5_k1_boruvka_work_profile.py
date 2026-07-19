#!/usr/bin/env python3
"""Validate and assemble the provisional Phase 5 Morton work-profile artifact."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys
from typing import Any, NoReturn, Sequence


CHECKER_DIRECTORY = Path(__file__).resolve().parents[1] / "configuration"
sys.path.insert(0, str(CHECKER_DIRECTORY))
try:
    from check_phase5_k1_boruvka_morton_work_profile import (
        ContractError,
        SCHEMA as WORK_PROFILE_SCHEMA,
        validate_document,
    )
finally:
    del sys.path[0]


SCHEMA = "morsehgp3d.phase5.k1_boruvka_morton_work_profile_artifact.v1"
PHASE3_SCHEMA = "morsehgp3d.phase3.qualification.v1"
SCIENTIFIC_SCOPE = "empirical_morton_work_profile_only"
BASE_IMAGE_REF = (
    "nvidia/cuda:12.9.2-devel-ubuntu24.04@"
    "sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"
)
SHA_RE = re.compile(r"[0-9a-f]{40}\Z")
IMAGE_ID_RE = re.compile(r"sha256:[0-9a-f]{64}\Z")
EXPECTED_RADII = [1, 4, 16]
EXPECTED_SEED = 1
EXPECTED_MATRIX = tuple(
    (point_count, family)
    for point_count in (64, 256, 1024)
    for family in ("uniform", "clusters", "lattice")
)
WORKER_LIFECYCLE = {
    "guest_shutdown_guard_verified": True,
    "stop_responsibility": "external_orchestrator",
    "worker_mutates_gcp": False,
}


def fail(message: str) -> NoReturn:
    raise SystemExit(message)


def reject_duplicate_json_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    value: dict[str, Any] = {}
    for key, item in pairs:
        if key in value:
            raise ValueError(f"duplicate JSON key: {key}")
        value[key] = item
    return value


def reject_non_finite_json_constant(value: str) -> NoReturn:
    raise ValueError(f"non-finite JSON constant: {value}")


def read_json_object(
    path: Path,
    label: str,
    *,
    canonical_one_line: bool,
) -> tuple[str, dict[str, Any]]:
    if not path.is_file() or path.is_symlink():
        fail(f"{label} must be a regular non-symbolic file")
    try:
        raw = path.read_text(encoding="utf-8")
        value = json.loads(
            raw,
            object_pairs_hook=reject_duplicate_json_keys,
            parse_constant=reject_non_finite_json_constant,
        )
    except (OSError, UnicodeError, json.JSONDecodeError, ValueError) as error:
        fail(f"cannot read {label} {path}: {error}")
    if not isinstance(value, dict):
        fail(f"{label} must be a JSON object")
    if canonical_one_line:
        canonical = json.dumps(
            value,
            ensure_ascii=True,
            sort_keys=True,
            separators=(",", ":"),
        )
        if raw != canonical + "\n":
            fail(f"{label} must be canonical one-line JSON")
    return raw, value


def sha256_file(path: Path, label: str) -> str:
    if not path.is_file() or path.is_symlink():
        fail(f"{label} must be a regular non-symbolic file")
    digest = hashlib.sha256()
    try:
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
    except OSError as error:
        fail(f"cannot hash {label} {path}: {error}")
    return digest.hexdigest()


def require_nonempty_string(value: Any, label: str) -> str:
    if not isinstance(value, str) or not value:
        fail(f"{label} must be a non-empty string")
    return value


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
    for field, expected in {
        "backend": "cuda_g4",
        "mode": "certified",
        "phase": "3",
        "profile": "hgp_reduced",
        "scientific_scope": "environment_reproducibility_only",
        "status": "worker_passed_pending_shutdown",
    }.items():
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
        fail("environment artifact does not preserve the worker lifecycle")

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
        or manifest.get("gpu_compute_capability_major") != 12
        or manifest.get("gpu_compute_capability_minor") != 0
    ):
        fail("environment artifact is not compiled and executed on sm_120")
    if (
        manifest.get("git_sha") != git_sha
        or manifest.get("image_ref") != image_ref
        or manifest.get("image_id") != image_id
    ):
        fail("environment runtime manifest identity differs")
    return manifest


def validate_measurements(
    paths: list[Path],
    *,
    git_sha: str,
) -> list[dict[str, Any]]:
    if len(paths) != len(EXPECTED_MATRIX):
        fail(
            "--profile-log must be supplied exactly nine times in "
            "size-major, family-minor order"
        )

    measurements: list[dict[str, Any]] = []
    for index, (path, expected) in enumerate(
        zip(paths, EXPECTED_MATRIX, strict=True)
    ):
        point_count, family = expected
        label = f"work-profile log {index} ({point_count}/{family})"
        _, value = read_json_object(path, label, canonical_one_line=True)
        try:
            validate_document(value, expected_backend="cuda_g4")
        except ContractError as error:
            fail(f"{label} violates {WORK_PROFILE_SCHEMA}: {error}")

        if value.get("point_count") != point_count:
            fail(f"{label} point_count differs")
        if value.get("generator") != {
            "algorithm": "deterministic_dyadic_v1",
            "family": family,
            "seed": EXPECTED_SEED,
        }:
            fail(f"{label} generator differs")
        if value.get("policies") != {
            "candidate_record_budget": point_count - 1,
            "window_radii": EXPECTED_RADII,
        }:
            fail(f"{label} policies differ")
        if value.get("git") != {"sha": git_sha}:
            fail(f"{label} Git SHA differs")
        measurements.append(value)
    return measurements


def parse_arguments(arguments: Sequence[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--git-sha", required=True)
    parser.add_argument("--base-image-ref", required=True)
    parser.add_argument("--image-ref", required=True)
    parser.add_argument("--image-id", required=True)
    parser.add_argument("--environment-artifact", type=Path, required=True)
    parser.add_argument(
        "--profile-log",
        action="append",
        dest="profile_logs",
        type=Path,
        required=True,
    )
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
        fail("--image-ref is not tied to the measured Git SHA")

    _, environment = read_json_object(
        args.environment_artifact,
        "Phase 3 environment artifact",
        canonical_one_line=False,
    )
    manifest = validate_phase3_environment(
        environment,
        git_sha=args.git_sha,
        base_image_ref=args.base_image_ref,
        image_ref=args.image_ref,
        image_id=args.image_id,
    )
    measurements = validate_measurements(args.profile_logs, git_sha=args.git_sha)

    driver_version = require_nonempty_string(
        manifest.get("cuda_driver_version_string"),
        "environment CUDA driver version",
    )
    gpu_name = require_nonempty_string(
        manifest.get("gpu_name"), "environment GPU name"
    )
    gpu_uuid = require_nonempty_string(
        manifest.get("gpu_uuid"), "environment GPU UUID"
    )
    gpu_vram_bytes = manifest.get("gpu_vram_bytes")
    if type(gpu_vram_bytes) is not int or gpu_vram_bytes <= 0:
        fail("environment GPU VRAM must be a positive integer")

    artifact = {
        "artifact_role": "benchmark_only",
        "backend": "cuda_g4",
        "benchmark_status": "complete",
        "binary": {
            "work_profile_sha256": sha256_file(
                args.binary, "Phase 5 Morton work-profile binary"
            )
        },
        "completed_measurement_count": len(measurements),
        "environment": {
            "compute_capability": "12.0",
            "driver_version": driver_version,
            "gpu_name": gpu_name,
            "gpu_uuid": gpu_uuid,
            "gpu_vram_bytes": gpu_vram_bytes,
        },
        "git": {"clean": True, "sha": args.git_sha},
        "hierarchy_reduction_status": "not_performed",
        "image": {
            "base_ref": args.base_image_ref,
            "id": args.image_id,
            "ref": args.image_ref,
        },
        "measurements": measurements,
        "mode": "benchmark",
        "phase": "5",
        "profile": "hgp_reduced",
        "provenance": {
            "environment_artifact_schema": PHASE3_SCHEMA,
            "environment_artifact_sha256": sha256_file(
                args.environment_artifact, "Phase 3 environment artifact"
            ),
        },
        "expected_measurement_count": len(EXPECTED_MATRIX),
        "qualification_claimed": False,
        "scalability_claimed": False,
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
        artifact,
        ensure_ascii=False,
        sort_keys=True,
        separators=(",", ":"),
    )
    try:
        with output.open("w", encoding="utf-8", newline="\n") as stream:
            stream.write(encoded)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
    except OSError as error:
        fail(f"cannot write Phase 5 Morton work-profile artifact {output}: {error}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
