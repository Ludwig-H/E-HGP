#!/usr/bin/env python3
"""Validate and assemble the bounded Phase 3 G4 qualification artifact."""

from __future__ import annotations

import argparse
from datetime import datetime
import json
import os
import re
from pathlib import Path
from typing import Any, NoReturn

SCHEMA = "morsehgp3d.phase3.qualification.v1"
RUNTIME_SCHEMA = "morsehgp3d.phase3.runtime.v1"
SHA_RE = re.compile(r"[0-9a-f]{40}\Z")
IMAGE_ID_RE = re.compile(r"sha256:[0-9a-f]{64}\Z")
SM_RE = re.compile(r"sm_[0-9]+")
UUID_RE = re.compile(r"[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\Z")
BASE_IMAGE_REF = (
    "nvidia/cuda:12.9.2-devel-ubuntu24.04@"
    "sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"
)
RUNTIME_ALLOCATION_CEILING_BYTES = 64 * 1024 * 1024
SANITIZER_ALLOCATION_CEILING_BYTES = 4 * 1024 * 1024
CUDA_INVALID_DEVICE_CODE = 101


def fail(message: str) -> NoReturn:
    raise SystemExit(message)


def read_text(path: Path, *, allow_empty: bool = False) -> str:
    try:
        value = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        fail(f"cannot read {path}: {error}")
    if not allow_empty and not value.strip():
        fail(f"required evidence is empty: {path}")
    return value


def reject_duplicate_json_keys(
    pairs: list[tuple[str, Any]],
) -> dict[str, Any]:
    value: dict[str, Any] = {}
    for key, item in pairs:
        if key in value:
            raise ValueError(f"duplicate JSON object key: {key}")
        value[key] = item
    return value


def read_jsonl(path: Path, label: str) -> tuple[str, list[dict[str, Any]]]:
    raw = read_text(path)
    records: list[dict[str, Any]] = []
    for line_number, line in enumerate(raw.splitlines(), start=1):
        if not line.strip():
            fail(f"{label} contains an empty JSONL line at {line_number}")
        try:
            value = json.loads(line, object_pairs_hook=reject_duplicate_json_keys)
        except (json.JSONDecodeError, ValueError) as error:
            fail(f"{label} line {line_number} is invalid JSON: {error}")
        if not isinstance(value, dict):
            fail(f"{label} line {line_number} must be a JSON object")
        records.append(value)
    if not records:
        fail(f"{label} contains no result")
    return raw, records


def require_bool(value: Any, expected: bool, context: str) -> None:
    if type(value) is not bool or value is not expected:
        fail(f"{context} must be {str(expected).lower()}")


def require_exact_keys(value: dict[str, Any], expected: set[str], context: str) -> None:
    actual = set(value)
    if actual != expected:
        fail(
            f"{context} keys differ: missing={sorted(expected - actual)}, "
            f"unexpected={sorted(actual - expected)}"
        )


def require_integer(value: Any, context: str, *, minimum: int | None = None) -> int:
    if type(value) is not int:
        fail(f"{context} must be an integer")
    if minimum is not None and value < minimum:
        fail(f"{context} must be at least {minimum}")
    return value


def require_nonempty_string(value: Any, context: str) -> str:
    if not isinstance(value, str) or not value:
        fail(f"{context} must be a non-empty string")
    return value


def parse_timestamp(value: Any, context: str) -> datetime:
    text = require_nonempty_string(value, context)
    if not text.endswith("Z"):
        fail(f"{context} must use a UTC Z suffix")
    try:
        return datetime.fromisoformat(text[:-1] + "+00:00")
    except ValueError as error:
        fail(f"{context} is not an ISO-8601 timestamp: {error}")


def validate_encoded_version(
    encoded: Any,
    version_string: Any,
    *,
    major_unit: int,
    minor_unit: int,
    expected_major_minor: tuple[int, int] | None,
    context: str,
) -> None:
    numeric = require_integer(encoded, context, minimum=1)
    text = require_nonempty_string(version_string, f"{context}_string")
    major = numeric // major_unit
    minor = (numeric // minor_unit) % (major_unit // minor_unit)
    patch = numeric % minor_unit
    decoded = (major, minor, patch)
    if expected_major_minor is not None and decoded[:2] != expected_major_minor:
        expected = ".".join(str(component) for component in expected_major_minor)
        fail(f"{context} must identify {expected}.x")
    expected_string = ".".join(str(component) for component in decoded)
    if text != expected_string:
        fail(f"{context}_string must be {expected_string} for encoded value {numeric}")


def validate_manifest(
    manifest: Any, *, git_sha: str, image_ref: str, image_id: str, context: str
) -> None:
    if not isinstance(manifest, dict):
        fail(f"{context}.manifest must be an object")
    require_exact_keys(
        manifest,
        {
            "aot_only",
            "backend",
            "build_mode",
            "cccl_version",
            "cccl_version_string",
            "clock_rate_khz",
            "clocks_source",
            "compiled_sm",
            "complete",
            "compilation_during_measurement",
            "cub_version",
            "cub_version_string",
            "cuda_compiler_version",
            "cuda_compiler_version_string",
            "cuda_driver_version",
            "cuda_driver_version_string",
            "cuda_module_loading",
            "cuda_runtime_version",
            "cuda_runtime_version_string",
            "device_index",
            "dlpack_major_version",
            "dlpack_minor_version",
            "dlpack_version_string",
            "forest_semantics",
            "git_sha",
            "gpu_compute_capability_major",
            "gpu_compute_capability_minor",
            "gpu_name",
            "gpu_runtime_sm",
            "gpu_uuid",
            "gpu_vram_bytes",
            "host_compiler",
            "image_id",
            "image_ref",
            "memory_clock_rate_khz",
            "mode",
            "nvrtc_used",
            "profile",
            "scientific_public_status",
            "scientific_result_claimed",
        },
        f"{context}.manifest",
    )
    require_bool(manifest.get("complete"), True, f"{context}.manifest.complete")
    require_bool(manifest.get("aot_only"), True, f"{context}.manifest.aot_only")
    require_bool(
        manifest.get("compilation_during_measurement"),
        False,
        f"{context}.manifest.compilation_during_measurement",
    )
    require_bool(manifest.get("nvrtc_used"), False, f"{context}.manifest.nvrtc_used")
    if manifest.get("git_sha") != git_sha:
        fail(f"{context}.manifest.git_sha does not match the qualified commit")
    if manifest.get("image_ref") != image_ref:
        fail(f"{context}.manifest.image_ref does not match the built image")
    if manifest.get("image_id") != image_id:
        fail(f"{context}.manifest.image_id does not match the built image")
    if manifest.get("backend") != "cuda_g4":
        fail(f"{context}.manifest.backend must be cuda_g4")
    if manifest.get("mode") != "certified":
        fail(f"{context}.manifest.mode must be certified")
    if manifest.get("profile") != "hgp_reduced":
        fail(f"{context}.manifest.profile must be hgp_reduced")
    if manifest.get("compiled_sm") != "sm_120":
        fail(f"{context}.manifest.compiled_sm must be sm_120")
    if manifest.get("gpu_runtime_sm") != "sm_120":
        fail(f"{context}.manifest.gpu_runtime_sm must be sm_120")
    if "public_status" in manifest:
        fail(f"{context} must not expose a scientific public_status")
    require_bool(
        manifest.get("scientific_result_claimed"),
        False,
        f"{context}.manifest.scientific_result_claimed",
    )
    if manifest.get("scientific_public_status") is not None:
        fail(f"{context}.manifest.scientific_public_status must be null")
    if manifest.get("build_mode") != "release":
        fail(f"{context}.manifest.build_mode must be release")
    if manifest.get("clocks_source") != "cudaDeviceProp":
        fail(f"{context}.manifest.clocks_source must be cudaDeviceProp")
    if manifest.get("cuda_module_loading") != "EAGER":
        fail(f"{context}.manifest.cuda_module_loading must be EAGER")
    if manifest.get("forest_semantics") is not None:
        fail(f"{context}.manifest.forest_semantics must be null")
    for field in (
        "cccl_version",
        "clock_rate_khz",
        "cub_version",
        "cuda_compiler_version",
        "cuda_driver_version",
        "cuda_runtime_version",
        "gpu_vram_bytes",
        "memory_clock_rate_khz",
    ):
        require_integer(manifest.get(field), f"{context}.manifest.{field}", minimum=1)
    if (
        require_integer(
            manifest.get("device_index"), f"{context}.manifest.device_index", minimum=0
        )
        != 0
    ):
        fail(f"{context}.manifest.device_index must be zero")
    if (
        require_integer(
            manifest.get("gpu_compute_capability_major"),
            f"{context}.manifest.gpu_compute_capability_major",
        ),
        require_integer(
            manifest.get("gpu_compute_capability_minor"),
            f"{context}.manifest.gpu_compute_capability_minor",
        ),
    ) != (12, 0):
        fail(f"{context}.manifest compute capability must be 12.0")
    if (
        require_integer(
            manifest.get("dlpack_major_version"),
            f"{context}.manifest.dlpack_major_version",
        ),
        require_integer(
            manifest.get("dlpack_minor_version"),
            f"{context}.manifest.dlpack_minor_version",
        ),
    ) != (1, 3):
        fail(f"{context}.manifest DLPack version must be 1.3")
    for field in (
        "cccl_version_string",
        "cub_version_string",
        "cuda_compiler_version_string",
        "cuda_driver_version_string",
        "cuda_runtime_version_string",
        "dlpack_version_string",
        "gpu_name",
        "host_compiler",
    ):
        require_nonempty_string(manifest.get(field), f"{context}.manifest.{field}")
    validate_encoded_version(
        manifest.get("cccl_version"),
        manifest.get("cccl_version_string"),
        major_unit=1_000_000,
        minor_unit=1_000,
        expected_major_minor=(2, 8),
        context=f"{context}.manifest.cccl_version",
    )
    validate_encoded_version(
        manifest.get("cub_version"),
        manifest.get("cub_version_string"),
        major_unit=100_000,
        minor_unit=100,
        expected_major_minor=(2, 8),
        context=f"{context}.manifest.cub_version",
    )
    validate_encoded_version(
        manifest.get("cuda_compiler_version"),
        manifest.get("cuda_compiler_version_string"),
        major_unit=1_000_000,
        minor_unit=1_000,
        expected_major_minor=(12, 9),
        context=f"{context}.manifest.cuda_compiler_version",
    )
    validate_encoded_version(
        manifest.get("cuda_runtime_version"),
        manifest.get("cuda_runtime_version_string"),
        major_unit=1_000,
        minor_unit=10,
        expected_major_minor=(12, 9),
        context=f"{context}.manifest.cuda_runtime_version",
    )
    validate_encoded_version(
        manifest.get("cuda_driver_version"),
        manifest.get("cuda_driver_version_string"),
        major_unit=1_000,
        minor_unit=10,
        expected_major_minor=None,
        context=f"{context}.manifest.cuda_driver_version",
    )
    if manifest.get("dlpack_version_string") != "1.3":
        fail(f"{context}.manifest.dlpack_version_string must be 1.3")
    if UUID_RE.fullmatch(str(manifest.get("gpu_uuid"))) is None:
        fail(f"{context}.manifest.gpu_uuid is not canonical")


def validate_structured_cuda_error(error: dict[str, Any], context: str) -> None:
    require_exact_keys(
        error,
        {"code", "context_healthy", "expected", "message", "name", "operation"},
        context,
    )
    if (
        require_integer(error.get("code"), f"{context}.code", minimum=1)
        != CUDA_INVALID_DEVICE_CODE
    ):
        fail(f"{context}.code must be {CUDA_INVALID_DEVICE_CODE}")
    require_bool(error.get("expected"), True, f"{context}.expected")
    require_bool(error.get("context_healthy"), True, f"{context}.context_healthy")
    if error.get("name") != "cudaErrorInvalidDevice":
        fail(f"{context}.name must be cudaErrorInvalidDevice")
    if error.get("operation") != "cudaGetDeviceProperties(device=-1)":
        fail(f"{context}.operation is not the frozen recoverable error exercise")
    require_nonempty_string(error.get("message"), f"{context}.message")


def validate_runtime_records(
    records: list[dict[str, Any]],
    *,
    label: str,
    git_sha: str,
    image_ref: str,
    image_id: str,
    expected_ceiling_bytes: int,
) -> dict[str, dict[str, Any]]:
    if len(records) != 3:
        fail(f"{label} must contain exactly two measurements and one error record")
    observed_order = [
        (record.get("kind"), record.get("protocol")) for record in records
    ]
    expected_order = [
        ("phase3_runtime_result", "warm"),
        ("phase3_runtime_result", "resident"),
        ("phase3_structured_cuda_error_result", None),
    ]
    if observed_order != expected_order:
        fail(f"{label} records must be ordered warm, resident, structured error")
    protocols: list[str] = []
    measurements: dict[str, dict[str, Any]] = {}
    measurement_times: dict[str, tuple[datetime, datetime]] = {}
    error_record: dict[str, Any] | None = None
    error_timestamp: datetime | None = None
    for index, record in enumerate(records):
        context = f"{label}[{index}]"
        if record.get("schema") != RUNTIME_SCHEMA:
            fail(f"{context}.schema must be {RUNTIME_SCHEMA}")
        if record.get("status") != "ok":
            fail(f"{context}.status must be ok")
        require_bool(
            record.get("compilation_during_measurement"),
            False,
            f"{context}.compilation_during_measurement",
        )
        validate_manifest(
            record.get("manifest"),
            git_sha=git_sha,
            image_ref=image_ref,
            image_id=image_id,
            context=context,
        )

        kind = record.get("kind")
        if kind == "phase3_runtime_result":
            require_exact_keys(
                record,
                {
                    "allocation",
                    "compilation_during_measurement",
                    "determinism",
                    "dlpack",
                    "duration",
                    "kind",
                    "manifest",
                    "protocol",
                    "schema",
                    "status",
                    "structured_cuda_error",
                    "timestamp_end_utc",
                    "timestamp_start_utc",
                },
                context,
            )
            protocol = record.get("protocol")
            if protocol not in {"warm", "resident"}:
                fail(f"{context}.protocol must be warm or resident")
            protocols.append(protocol)
            allocation = record.get("allocation")
            if not isinstance(allocation, dict):
                fail(f"{context}.allocation must be an object")
            require_exact_keys(
                allocation,
                {
                    "allocation_count",
                    "configured_ceiling_bytes",
                    "exact_async_allocation_bytes",
                    "free_bytes_after",
                    "free_bytes_before",
                    "free_delta_bytes",
                    "leak_free",
                    "live_bytes_final",
                    "peak_live_bytes",
                    "release_count",
                    "single_async_arena",
                    "suballocated_without_extra_cuda_allocations",
                    "trimmed_default_pool",
                    "within_configured_ceiling",
                },
                f"{context}.allocation",
            )
            integer_fields = (
                "allocation_count",
                "configured_ceiling_bytes",
                "exact_async_allocation_bytes",
                "free_bytes_after",
                "free_bytes_before",
                "free_delta_bytes",
                "live_bytes_final",
                "peak_live_bytes",
                "release_count",
            )
            for field in integer_fields:
                if type(allocation.get(field)) is not int:
                    fail(f"{context}.allocation.{field} must be an integer")
            configured_ceiling = allocation["configured_ceiling_bytes"]
            if configured_ceiling <= 0:
                fail(f"{context}.allocation.configured_ceiling_bytes must be positive")
            if configured_ceiling != expected_ceiling_bytes:
                fail(
                    f"{context}.allocation.configured_ceiling_bytes must be "
                    f"{expected_ceiling_bytes}"
                )
            if allocation["exact_async_allocation_bytes"] != configured_ceiling:
                fail(
                    f"{context}.allocation.exact_async_allocation_bytes must equal "
                    "configured_ceiling_bytes"
                )
            if allocation["peak_live_bytes"] != configured_ceiling:
                fail(f"{context}.allocation.peak_live_bytes must equal its ceiling")
            if allocation["live_bytes_final"] != 0:
                fail(f"{context}.allocation.live_bytes_final must be zero")
            if allocation["allocation_count"] != 1 or allocation["release_count"] != 1:
                fail(
                    f"{context}.allocation must contain one allocation and one release"
                )
            require_bool(
                allocation.get("leak_free"), True, f"{context}.allocation.leak_free"
            )
            require_bool(
                allocation.get("within_configured_ceiling"),
                True,
                f"{context}.allocation.within_configured_ceiling",
            )
            require_bool(
                allocation.get("trimmed_default_pool"),
                True,
                f"{context}.allocation.trimmed_default_pool",
            )
            require_bool(
                allocation.get("single_async_arena"),
                True,
                f"{context}.allocation.single_async_arena",
            )
            require_bool(
                allocation.get("suballocated_without_extra_cuda_allocations"),
                True,
                f"{context}.allocation.suballocated_without_extra_cuda_allocations",
            )
            if (
                allocation["free_bytes_before"] <= 0
                or allocation["free_bytes_after"] <= 0
            ):
                fail(f"{context}.allocation free-memory readings must be positive")

            determinism = record.get("determinism")
            if not isinstance(determinism, dict):
                fail(f"{context}.determinism must be an object")
            require_exact_keys(
                determinism,
                {
                    "bitwise_equal",
                    "cpu_reference_equal",
                    "cpu_reference_fnv1a64_u32_le",
                    "cpu_reference_sum",
                    "element_count",
                    "kernel",
                    "measured_kernel_runs_compared",
                    "output_fnv1a64_u32_le",
                    "resident_cub_sum",
                    "uses_cub_device_reduce",
                    "warm_cub_sum",
                },
                f"{context}.determinism",
            )
            require_bool(
                determinism.get("bitwise_equal"),
                True,
                f"{context}.determinism.bitwise_equal",
            )
            require_bool(
                determinism.get("cpu_reference_equal"),
                True,
                f"{context}.determinism.cpu_reference_equal",
            )
            for field in (
                "element_count",
                "measured_kernel_runs_compared",
                "resident_cub_sum",
                "warm_cub_sum",
                "cpu_reference_sum",
            ):
                if type(determinism.get(field)) is not int:
                    fail(f"{context}.determinism.{field} must be an integer")
            if determinism["element_count"] <= 0:
                fail(f"{context}.determinism.element_count must be positive")
            if determinism["measured_kernel_runs_compared"] != 2:
                fail(f"{context}.determinism.measured_kernel_runs_compared must be two")
            if not (
                determinism["warm_cub_sum"]
                == determinism["resident_cub_sum"]
                == determinism["cpu_reference_sum"]
            ):
                fail(f"{context}.determinism sums disagree with the CPU reference")
            require_bool(
                determinism.get("uses_cub_device_reduce"),
                True,
                f"{context}.determinism.uses_cub_device_reduce",
            )
            if determinism.get("kernel") != "morsehgp3d_phase3_deterministic_kernel":
                fail(f"{context}.determinism.kernel is not the frozen AOT kernel")
            output_hash = determinism.get("output_fnv1a64_u32_le")
            reference_hash = determinism.get("cpu_reference_fnv1a64_u32_le")
            if (
                not isinstance(output_hash, str)
                or re.fullmatch(r"[0-9a-f]{16}", output_hash) is None
                or output_hash != reference_hash
            ):
                fail(f"{context}.determinism hash disagrees with the CPU reference")

            dlpack = record.get("dlpack")
            if not isinstance(dlpack, dict):
                fail(f"{context}.dlpack must be an object")
            require_exact_keys(
                dlpack,
                {
                    "byte_offset",
                    "copy_operations",
                    "device_identity",
                    "pointer_identity",
                    "zero_copy",
                },
                f"{context}.dlpack",
            )
            require_bool(dlpack.get("zero_copy"), True, f"{context}.dlpack.zero_copy")
            require_bool(
                dlpack.get("device_identity"),
                True,
                f"{context}.dlpack.device_identity",
            )
            require_bool(
                dlpack.get("pointer_identity"),
                True,
                f"{context}.dlpack.pointer_identity",
            )
            if (
                type(dlpack.get("copy_operations")) is not int
                or dlpack["copy_operations"] != 0
            ):
                fail(f"{context}.dlpack.copy_operations must be integer zero")
            if (
                require_integer(
                    dlpack.get("byte_offset"),
                    f"{context}.dlpack.byte_offset",
                    minimum=0,
                )
                != 0
            ):
                fail(f"{context}.dlpack.byte_offset must be zero")
            duration = record.get("duration")
            if not isinstance(duration, dict):
                fail(f"{context}.duration must be an object")
            require_exact_keys(duration, {"gpu_ns", "host_ns"}, f"{context}.duration")
            require_integer(
                duration.get("gpu_ns"), f"{context}.duration.gpu_ns", minimum=1
            )
            require_integer(
                duration.get("host_ns"), f"{context}.duration.host_ns", minimum=1
            )
            start = parse_timestamp(
                record.get("timestamp_start_utc"), f"{context}.timestamp_start_utc"
            )
            end = parse_timestamp(
                record.get("timestamp_end_utc"), f"{context}.timestamp_end_utc"
            )
            if end < start:
                fail(f"{context} ends before it starts")
            error = record.get("structured_cuda_error")
            if not isinstance(error, dict):
                fail(f"{context}.structured_cuda_error must be an object")
            validate_structured_cuda_error(error, f"{context}.structured_cuda_error")
            measurements[protocol] = record
            measurement_times[protocol] = (start, end)
        elif kind == "phase3_structured_cuda_error_result":
            require_exact_keys(
                record,
                {
                    "compilation_during_measurement",
                    "cuda_error",
                    "kind",
                    "manifest",
                    "schema",
                    "status",
                    "timestamp_utc",
                },
                context,
            )
            error = record.get("cuda_error")
            if not isinstance(error, dict):
                fail(f"{context}.cuda_error must be an object")
            validate_structured_cuda_error(error, f"{context}.cuda_error")
            error_timestamp = parse_timestamp(
                record.get("timestamp_utc"), f"{context}.timestamp_utc"
            )
            error_record = record
        else:
            fail(f"{context}.kind is not a successful Phase 3 result")

    if protocols != ["warm", "resident"]:
        fail(f"{label} must contain exactly one warm and one resident result")
    if error_record is None or error_timestamp is None:
        fail(f"{label} must prove structured CUDA error handling")
    warm = measurements["warm"]
    resident = measurements["resident"]
    for field in (
        "allocation",
        "determinism",
        "dlpack",
        "manifest",
        "structured_cuda_error",
    ):
        if warm[field] != resident[field]:
            fail(f"{label} warm and resident {field} evidence must be identical")
    if warm["manifest"] != error_record["manifest"]:
        fail(f"{label} manifests must be identical across all three records")
    if warm["structured_cuda_error"] != error_record["cuda_error"]:
        fail(f"{label} structured CUDA errors must be identical across all records")

    _, warm_end = measurement_times["warm"]
    resident_start, resident_end = measurement_times["resident"]
    if warm_end > resident_start:
        fail(f"{label} resident measurement starts before the warm measurement ends")
    if resident_end > error_timestamp:
        fail(f"{label} structured error record predates the resident measurement end")

    return {
        "manifest": warm["manifest"],
        "structured_cuda_error": warm["structured_cuda_error"],
    }


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--git-sha", required=True)
    parser.add_argument("--base-image-ref", required=True)
    parser.add_argument("--image-ref", required=True)
    parser.add_argument("--image-id", required=True)
    parser.add_argument("--runtime-jsonl", type=Path, required=True)
    parser.add_argument("--sanitizer-jsonl", type=Path, required=True)
    parser.add_argument("--guest-guard-log", type=Path, required=True)
    parser.add_argument("--preflight-log", type=Path, required=True)
    parser.add_argument("--build-log", type=Path, required=True)
    parser.add_argument("--release-log", type=Path, required=True)
    parser.add_argument("--audit-log", type=Path, required=True)
    parser.add_argument("--binding-log", type=Path, required=True)
    parser.add_argument("--elf-log", type=Path, required=True)
    parser.add_argument("--ptx-log", type=Path, required=True)
    parser.add_argument("--ptx-stderr-log", type=Path, required=True)
    parser.add_argument("--sanitizer-log", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_arguments()
    if SHA_RE.fullmatch(args.git_sha) is None:
        fail("--git-sha must be a canonical lowercase 40-hex commit ID")
    if IMAGE_ID_RE.fullmatch(args.image_id) is None:
        fail("--image-id must be a canonical sha256 Docker image ID")
    if args.base_image_ref != BASE_IMAGE_REF:
        fail(f"--base-image-ref must be exactly {BASE_IMAGE_REF}")
    expected_ref = f"morsehgp3d-phase3:{args.git_sha}"
    if args.image_ref != expected_ref:
        fail(f"--image-ref must be exactly {expected_ref}")

    runtime_raw, runtime_records = read_jsonl(args.runtime_jsonl, "runtime")
    sanitizer_raw, sanitizer_records = read_jsonl(
        args.sanitizer_jsonl, "sanitizer runtime"
    )
    runtime_stable_evidence = validate_runtime_records(
        runtime_records,
        label="runtime",
        git_sha=args.git_sha,
        image_ref=args.image_ref,
        image_id=args.image_id,
        expected_ceiling_bytes=RUNTIME_ALLOCATION_CEILING_BYTES,
    )
    sanitizer_stable_evidence = validate_runtime_records(
        sanitizer_records,
        label="sanitizer_runtime",
        git_sha=args.git_sha,
        image_ref=args.image_ref,
        image_id=args.image_id,
        expected_ceiling_bytes=SANITIZER_ALLOCATION_CEILING_BYTES,
    )
    if runtime_stable_evidence != sanitizer_stable_evidence:
        fail("runtime and sanitizer runs disagree on manifest or structured CUDA error")

    logs = {
        "guest_shutdown_guard": read_text(args.guest_guard_log),
        "preflight": read_text(args.preflight_log),
        "docker_build": read_text(args.build_log),
        "cuda_release": read_text(args.release_log),
        "cuda_audit": read_text(args.audit_log),
        "python_binding": read_text(args.binding_log),
        "cuobjdump_elf": read_text(args.elf_log),
        "cuobjdump_ptx": read_text(args.ptx_log, allow_empty=True),
        "cuobjdump_ptx_stderr": read_text(args.ptx_stderr_log, allow_empty=True),
        "compute_sanitizer": read_text(args.sanitizer_log),
    }
    architectures = SM_RE.findall(logs["cuobjdump_elf"])
    if not architectures or set(architectures) != {"sm_120"}:
        fail("cuobjdump ELF evidence must be non-empty and contain only sm_120")
    if logs["cuobjdump_ptx"].strip():
        fail("cuobjdump PTX evidence must contain zero entries")

    artifact = {
        "backend": "cuda_g4",
        "checks": {
            "aot_elf_architectures": sorted(set(architectures)),
            "aot_ptx_entry_count": 0,
            "binding": "passed",
            "compute_sanitizer": "passed",
            "cuda_audit_workflow": "passed",
            "cuda_release_workflow": "passed",
            "guest_shutdown_guard": "verified_before_heavy_work",
            "manifest_complete_for_every_result": True,
            "memory_live_bytes_final": 0,
            "preflight": "passed",
            "warm_and_resident_present": True,
        },
        "git": {"clean": True, "sha": args.git_sha},
        "image": {
            "dockerfile": "containers/cuda12.9-sm120.Dockerfile",
            "base_ref": args.base_image_ref,
            "id": args.image_id,
            "ref": args.image_ref,
        },
        "logs": logs,
        "mode": "certified",
        "phase": "3",
        "profile": "hgp_reduced",
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "runtime_jsonl": runtime_raw,
        "runtime_records": runtime_records,
        "sanitizer_runtime_jsonl": sanitizer_raw,
        "sanitizer_runtime_records": sanitizer_records,
        "schema": SCHEMA,
        "scientific_scope": "environment_reproducibility_only",
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
