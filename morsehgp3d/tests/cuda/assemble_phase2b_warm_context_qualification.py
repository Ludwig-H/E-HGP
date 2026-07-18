#!/usr/bin/env python3
"""Validate and assemble the provisional Phase 2B warm-context G4 artifact."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import tempfile
from typing import Any, NoReturn, Sequence

SCHEMA = "morsehgp3d.phase2b.warm_context_qualification.v1"
BENCHMARK_SCHEMA = "morsehgp3d.phase2b.warm_context_e2e.v1"
TIMING_SCOPE = (
    "steady_clock_around_async_api_get_includes_validation_packing_transfers_"
    "kernel_fallback_synchronization_excludes_input_generation"
)
CASES = 65536
REPEATS = 31
WARMUP_REPEATS = 1
EXPECTED_INPUTS = CASES * REPEATS
EXPECTED_GPU_KNOWN = 43691 * REPEATS
EXPECTED_GPU_UNKNOWN = 21845 * REPEATS
SHA_RE = re.compile(r"[0-9a-f]{40}\Z")
IMAGE_ID_RE = re.compile(r"sha256:[0-9a-f]{64}\Z")
GPU_UUID_RE = re.compile(r"GPU-[0-9A-Fa-f-]+\Z")
TOP_LEVEL_KEYS = {
    "audit_gpu_signs",
    "cases",
    "fallback",
    "repeats",
    "results",
    "schema",
    "scope",
    "timing_scope",
    "warmup_repeats",
}
RESULT_KEYS = {
    "counters",
    "mad_ns",
    "max_ns",
    "min_ns",
    "p50_ns",
    "p95_ns",
    "predicate",
    "samples_ns",
}
COUNTER_KEYS = {
    "async_fallback_batches",
    "cpu_expansion_certified",
    "cpu_fp64_filtered_certified",
    "cpu_multiprecision_certified",
    "exact_zeros",
    "gpu_fp64_certified",
    "gpu_inputs",
    "gpu_known_audited",
    "gpu_unknown_forwarded",
    "remaining_unknown",
}
PREDICATES = {
    "distance": "compare_squared_distances",
    "orientation": "orientation_3d",
    "power": "power_bisector_side",
}


def fail(message: str) -> NoReturn:
    raise SystemExit(message)


def reject_duplicate_json_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def read_text(path: Path, context: str, *, allow_empty: bool = False) -> str:
    try:
        value = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        fail(f"cannot read {context} {path}: {error}")
    if not allow_empty and not value.strip():
        fail(f"{context} is empty: {path}")
    return value


def require_exact_keys(value: dict[str, Any], expected: set[str], context: str) -> None:
    actual = set(value)
    if actual != expected:
        fail(
            f"{context} keys differ: missing={sorted(expected - actual)}, "
            f"unexpected={sorted(actual - expected)}"
        )


def require_integer(value: Any, context: str, *, minimum: int = 0) -> int:
    if type(value) is not int or value < minimum:
        fail(f"{context} must be an integer at least {minimum}")
    return value


def nearest_rank(sorted_samples: list[int], percentile: int) -> int:
    if not sorted_samples or not 0 < percentile <= 100:
        fail("invalid nearest-rank request")
    rank = (len(sorted_samples) * percentile + 99) // 100
    return sorted_samples[rank - 1]


def validate_counters(value: Any, context: str) -> None:
    if not isinstance(value, dict):
        fail(f"{context} must be an object")
    require_exact_keys(value, COUNTER_KEYS, context)
    counters = {
        key: require_integer(value.get(key), f"{context}.{key}") for key in COUNTER_KEYS
    }
    expected = {
        "async_fallback_batches": REPEATS,
        "exact_zeros": EXPECTED_GPU_UNKNOWN,
        "gpu_fp64_certified": EXPECTED_GPU_KNOWN,
        "gpu_inputs": EXPECTED_INPUTS,
        "gpu_known_audited": 0,
        "gpu_unknown_forwarded": EXPECTED_GPU_UNKNOWN,
        "remaining_unknown": 0,
    }
    for key, expected_value in expected.items():
        if counters[key] != expected_value:
            fail(f"{context}.{key} must be {expected_value}")
    cpu_certified = (
        counters["cpu_fp64_filtered_certified"]
        + counters["cpu_expansion_certified"]
        + counters["cpu_multiprecision_certified"]
    )
    if cpu_certified != EXPECTED_GPU_UNKNOWN:
        fail(f"{context} CPU fallback accounting is not closed")


def validate_benchmark(path: Path) -> dict[str, Any]:
    raw = read_text(path, "warm-context benchmark")
    try:
        value = json.loads(raw, object_pairs_hook=reject_duplicate_json_keys)
    except (json.JSONDecodeError, ValueError) as error:
        fail(f"warm-context benchmark is invalid JSON: {error}")
    if not isinstance(value, dict):
        fail("warm-context benchmark must be an object")
    canonical = json.dumps(
        value, ensure_ascii=True, separators=(",", ":"), sort_keys=True
    )
    if raw != canonical + "\n":
        fail("warm-context benchmark is not canonical one-line JSON")
    require_exact_keys(value, TOP_LEVEL_KEYS, "warm-context benchmark")
    if value.get("schema") != BENCHMARK_SCHEMA:
        fail(f"warm-context benchmark.schema must be {BENCHMARK_SCHEMA}")
    if value.get("scope") != "warm_context_e2e":
        fail("warm-context benchmark.scope must be warm_context_e2e")
    if value.get("timing_scope") != TIMING_SCOPE:
        fail("warm-context benchmark timing scope changed")
    if value.get("audit_gpu_signs") is not False:
        fail("warm-context benchmark must disable GPU-known auditing")
    if value.get("fallback") != "gpu_unknown_to_async_cpu_exact":
        fail("warm-context benchmark fallback policy changed")
    if require_integer(value.get("cases"), "warm-context benchmark.cases") != CASES:
        fail(f"warm-context benchmark.cases must be {CASES}")
    if (
        require_integer(value.get("repeats"), "warm-context benchmark.repeats")
        != REPEATS
    ):
        fail(f"warm-context benchmark.repeats must be {REPEATS}")
    if (
        require_integer(
            value.get("warmup_repeats"), "warm-context benchmark.warmup_repeats"
        )
        != WARMUP_REPEATS
    ):
        fail("warm-context benchmark.warmup_repeats must be one")
    results = value.get("results")
    if not isinstance(results, dict) or set(results) != set(PREDICATES):
        fail("warm-context benchmark results must cover the three predicates")
    for name, predicate in PREDICATES.items():
        record = results.get(name)
        context = f"warm-context benchmark.results.{name}"
        if not isinstance(record, dict):
            fail(f"{context} must be an object")
        require_exact_keys(record, RESULT_KEYS, context)
        if record.get("predicate") != predicate:
            fail(f"{context}.predicate must be {predicate}")
        samples = record.get("samples_ns")
        if not isinstance(samples, list) or len(samples) != REPEATS:
            fail(f"{context}.samples_ns must contain {REPEATS} samples")
        exact_samples = [
            require_integer(sample, f"{context}.samples_ns[{index}]", minimum=1)
            for index, sample in enumerate(samples)
        ]
        sorted_samples = sorted(exact_samples)
        p50 = nearest_rank(sorted_samples, 50)
        expected_statistics = {
            "mad_ns": nearest_rank(
                sorted(abs(sample - p50) for sample in exact_samples), 50
            ),
            "max_ns": sorted_samples[-1],
            "min_ns": sorted_samples[0],
            "p50_ns": p50,
            "p95_ns": nearest_rank(sorted_samples, 95),
        }
        for key, expected_value in expected_statistics.items():
            if require_integer(record.get(key), f"{context}.{key}") != expected_value:
                fail(f"{context}.{key} does not replay from the raw samples")
        validate_counters(record.get("counters"), f"{context}.counters")
    return value


def validate_gpu_info(path: Path) -> dict[str, Any]:
    lines = read_text(path, "GPU inventory").splitlines()
    if len(lines) != 1:
        fail("GPU inventory must contain exactly one device")
    fields = [field.strip() for field in lines[0].split(",")]
    if len(fields) != 5:
        fail("GPU inventory must contain name, UUID, driver, memory and capability")
    name, uuid, driver_version, memory_raw, compute_capability = fields
    if "RTX PRO 6000" not in name:
        fail("GPU inventory is not an RTX PRO 6000")
    if GPU_UUID_RE.fullmatch(uuid) is None:
        fail("GPU inventory UUID is not canonical")
    if not driver_version.startswith("580."):
        fail("GPU driver must belong to branch 580")
    try:
        memory_mib = int(memory_raw)
    except ValueError:
        fail("GPU memory is not an integer MiB value")
    if memory_mib < 95000:
        fail("GPU memory is below the qualified 96 GiB class")
    if compute_capability != "12.0":
        fail("GPU compute capability must be 12.0")
    return {
        "compute_capability": compute_capability,
        "driver_version": driver_version,
        "gpu_memory_mib": memory_mib,
        "gpu_name": name,
        "gpu_uuid": uuid,
    }


def validate_nvcc(path: Path) -> str:
    value = read_text(path, "NVCC version")
    if (
        "release 12.9," not in value
        or re.search(r"\bV12[.]9[.][0-9]+\b", value) is None
    ):
        fail("NVCC version must be a concrete CUDA 12.9 release")
    return next(line.strip() for line in value.splitlines() if "release 12.9," in line)


def sha256_file(path: Path, context: str) -> str:
    if not path.is_file() or path.is_symlink():
        fail(f"{context} must be a regular non-symbolic file")
    digest = hashlib.sha256()
    try:
        with path.open("rb") as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(block)
    except OSError as error:
        fail(f"cannot hash {context} {path}: {error}")
    return digest.hexdigest()


def write_atomic(path: Path, value: dict[str, Any]) -> None:
    if path.exists() or path.is_symlink():
        fail(f"refusing to replace an existing artifact: {path}")
    parent = path.parent.resolve(strict=True)
    encoded = json.dumps(
        value, ensure_ascii=True, separators=(",", ":"), sort_keys=True
    )
    temporary_name = ""
    try:
        descriptor, temporary_name = tempfile.mkstemp(
            prefix=f".{path.name}.", suffix=".tmp", dir=parent
        )
        with os.fdopen(descriptor, "w", encoding="ascii", newline="\n") as stream:
            stream.write(encoded + "\n")
            stream.flush()
            os.fsync(stream.fileno())
        os.link(temporary_name, path, follow_symlinks=False)
        os.unlink(temporary_name)
        temporary_name = ""
        directory_fd = os.open(parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
        try:
            os.fsync(directory_fd)
        finally:
            os.close(directory_fd)
    except FileExistsError:
        fail(f"refusing to replace an existing artifact: {path}")
    except OSError as error:
        fail(f"cannot publish warm-context artifact {path}: {error}")
    finally:
        if temporary_name:
            try:
                os.unlink(temporary_name)
            except FileNotFoundError:
                pass


def parse_arguments(arguments: Sequence[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--git-sha", required=True)
    parser.add_argument("--image-id", required=True)
    parser.add_argument("--benchmark-summary", type=Path, required=True)
    parser.add_argument("--harness", type=Path, required=True)
    parser.add_argument("--gpu-info", type=Path, required=True)
    parser.add_argument("--nvcc-version", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args(arguments)


def main(arguments: Sequence[str] | None = None) -> int:
    options = parse_arguments(arguments)
    if SHA_RE.fullmatch(options.git_sha) is None:
        fail("--git-sha must be a canonical forty-character lowercase SHA")
    if IMAGE_ID_RE.fullmatch(options.image_id) is None:
        fail("--image-id must be a canonical sha256 image identifier")
    benchmark = validate_benchmark(options.benchmark_summary)
    gpu = validate_gpu_info(options.gpu_info)
    nvcc = validate_nvcc(options.nvcc_version)
    harness_sha256 = sha256_file(options.harness, "warm-context harness")
    artifact = {
        "backend": "cuda_g4",
        "benchmark": benchmark,
        "binary": {
            "harness_sha256": harness_sha256,
            "nvcc": nvcc,
        },
        "cuda_runtime_executed": True,
        "environment": {**gpu, "image_id": options.image_id},
        "git": {"clean": True, "sha": options.git_sha},
        "mode": "certified",
        "phase": "2B",
        "profile": "hgp_reduced",
        "public_status": None,
        "schema": SCHEMA,
        "scientific_result_claimed": False,
        "status": "worker_passed_pending_shutdown",
    }
    write_atomic(options.output, artifact)
    print(
        json.dumps(
            {
                "harness_sha256": harness_sha256,
                "schema": SCHEMA,
                "status": "worker_passed_pending_shutdown",
            },
            separators=(",", ":"),
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
