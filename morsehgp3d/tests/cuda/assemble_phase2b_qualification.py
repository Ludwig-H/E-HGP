#!/usr/bin/env python3
"""Validate and assemble the provisional Phase 2B G4 qualification artifact."""

from __future__ import annotations

import argparse
import json
import math
import os
from pathlib import Path
import re
import tempfile
from typing import Any, NoReturn, Sequence


SCHEMA = "morsehgp3d.phase2b.predicates.qualification.v1"
BENCHMARK_SCHEMA = "morsehgp3d.phase2b.predicates.benchmark.v1"
DISTANCE_SCHEMA = "morsehgp3d.phase2b.distance_filter.v1"
ORIENTATION_SCHEMA = "morsehgp3d.phase2b.orientation_3d_filter.v1"
POWER_SCHEMA = "morsehgp3d.phase2b.power_bisector_side_filter.v1"
SHA_RE = re.compile(r"[0-9a-f]{40}\Z")
IMAGE_ID_RE = re.compile(r"sha256:[0-9a-f]{64}\Z")
SM_RE = re.compile(r"sm_[0-9]+")
SUMMARY_KEYS = {
    "cases",
    "gpu_fp64_certified",
    "gpu_known_audited",
    "gpu_unknown_forwarded",
    "remaining_unknown",
    "schema",
}


def fail(message: str) -> NoReturn:
    raise SystemExit(message)


def reject_duplicate_json_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    value: dict[str, Any] = {}
    for key, item in pairs:
        if key in value:
            raise ValueError(f"duplicate JSON object key: {key}")
        value[key] = item
    return value


def read_text(path: Path, context: str, *, allow_empty: bool = False) -> str:
    try:
        value = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as error:
        fail(f"cannot read {context} {path}: {error}")
    if not allow_empty and not value.strip():
        fail(f"{context} is empty: {path}")
    return value


def read_json_object(path: Path, context: str) -> dict[str, Any]:
    raw = read_text(path, context)
    try:
        value = json.loads(raw, object_pairs_hook=reject_duplicate_json_keys)
    except (json.JSONDecodeError, ValueError) as error:
        fail(f"{context} is invalid JSON: {error}")
    if not isinstance(value, dict):
        fail(f"{context} must be a JSON object")
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


def validate_common_summary(
    value: dict[str, Any],
    *,
    context: str,
    schema: str,
    cases: int,
    minimum_known: int,
    maximum_known: int,
    extra_keys: set[str] | None = None,
) -> None:
    expected_keys = SUMMARY_KEYS | (extra_keys or set())
    require_exact_keys(value, expected_keys, context)
    if value.get("schema") != schema:
        fail(f"{context}.schema must be {schema}")
    observed_cases = require_integer(value.get("cases"), f"{context}.cases")
    if observed_cases != cases:
        fail(f"{context}.cases must be {cases}")
    known = require_integer(
        value.get("gpu_fp64_certified"), f"{context}.gpu_fp64_certified"
    )
    if not minimum_known <= known <= maximum_known:
        fail(
            f"{context}.gpu_fp64_certified must be in "
            f"[{minimum_known}, {maximum_known}]"
        )
    audited = require_integer(
        value.get("gpu_known_audited"), f"{context}.gpu_known_audited"
    )
    if audited != known:
        fail(f"{context}.gpu_known_audited must equal its GPU-known count")
    unknown = require_integer(
        value.get("gpu_unknown_forwarded"), f"{context}.gpu_unknown_forwarded"
    )
    if unknown != cases - known:
        fail(f"{context} GPU-known and GPU-unknown counts must partition all cases")
    if require_integer(
        value.get("remaining_unknown"), f"{context}.remaining_unknown"
    ) != 0:
        fail(f"{context}.remaining_unknown must be zero")


def validate_distance(value: dict[str, Any]) -> None:
    validate_common_summary(
        value,
        context="distance differential",
        schema=DISTANCE_SCHEMA,
        cases=2064,
        minimum_known=2061,
        maximum_known=2061,
    )


def validate_orientation(value: dict[str, Any]) -> None:
    validate_common_summary(
        value,
        context="orientation differential",
        schema=ORIENTATION_SCHEMA,
        cases=2070,
        minimum_known=2065,
        maximum_known=2067,
    )


def validate_power(value: dict[str, Any]) -> None:
    validate_common_summary(
        value,
        context="power-bisector differential",
        schema=POWER_SCHEMA,
        cases=2077,
        minimum_known=2065,
        maximum_known=2071,
        extra_keys={"exact_zeros"},
    )
    if require_integer(
        value.get("exact_zeros"), "power-bisector differential.exact_zeros"
    ) != 4:
        fail("power-bisector differential.exact_zeros must be four")


def require_positive_number(value: Any, context: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        fail(f"{context} must be numeric")
    converted = float(value)
    if not math.isfinite(converted) or converted <= 0.0:
        fail(f"{context} must be finite and positive")
    return converted


def validate_benchmark(value: dict[str, Any]) -> None:
    require_exact_keys(
        value, {"cases", "repeats", "results", "schema", "scope"}, "benchmark"
    )
    if value.get("schema") != BENCHMARK_SCHEMA:
        fail(f"benchmark.schema must be {BENCHMARK_SCHEMA}")
    if value.get("scope") != "cold_process_end_to_end":
        fail("benchmark.scope must be cold_process_end_to_end")
    if require_integer(value.get("cases"), "benchmark.cases") != 262144:
        fail("benchmark.cases must be 262144")
    if require_integer(value.get("repeats"), "benchmark.repeats") != 3:
        fail("benchmark.repeats must be three")
    results = value.get("results")
    if not isinstance(results, dict):
        fail("benchmark.results must be an object")
    expected_predicates = {
        "distance": "compare_squared_distances",
        "orientation": "orientation_3d",
    }
    expected_predicates["power"] = "power_bisector_side"
    if set(results) != set(expected_predicates):
        fail("benchmark.results must contain exactly distance/orientation/power")
    for name, replay_name in expected_predicates.items():
        record = results.get(name)
        if not isinstance(record, dict):
            fail(f"benchmark.results.{name} must be an object")
        require_exact_keys(
            record,
            {
                "input_bytes",
                "median_elapsed",
                "minimum_elapsed",
                "predicate",
            },
            f"benchmark.results.{name}",
        )
        if record.get("predicate") != replay_name:
            fail(f"benchmark.results.{name}.predicate must be {replay_name}")
        require_integer(
            record.get("input_bytes"), f"benchmark.results.{name}.input_bytes", minimum=1
        )
        for sample in ("minimum_elapsed", "median_elapsed"):
            rates = record.get(sample)
            if not isinstance(rates, dict):
                fail(f"benchmark.results.{name}.{sample} must be an object")
            require_exact_keys(
                rates,
                {"cases_per_second", "seconds", "stdin_bytes_per_second"},
                f"benchmark.results.{name}.{sample}",
            )
            for field in ("cases_per_second", "seconds", "stdin_bytes_per_second"):
                require_positive_number(
                    rates.get(field), f"benchmark.results.{name}.{sample}.{field}"
                )


def validate_sanitizer_log(path: Path, predicate: str) -> None:
    value = read_text(path, f"{predicate} compute-sanitizer log")
    if "ERROR SUMMARY: 0 errors" not in value:
        fail(f"{predicate} compute-sanitizer log has no zero-error summary")


def write_atomic(path: Path, value: dict[str, Any]) -> None:
    if path.exists() or path.is_symlink():
        fail(f"refusing to replace an existing artifact: {path}")
    parent = path.parent.resolve(strict=True)
    encoded = json.dumps(
        value, ensure_ascii=False, separators=(",", ":"), sort_keys=True
    )
    temporary_name = ""
    try:
        descriptor, temporary_name = tempfile.mkstemp(
            prefix=f".{path.name}.", suffix=".tmp", dir=parent
        )
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as stream:
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
        fail(f"cannot publish qualification artifact {path}: {error}")
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
    parser.add_argument("--distance-summary", type=Path, required=True)
    parser.add_argument("--orientation-summary", type=Path, required=True)
    parser.add_argument("--power-summary", type=Path, required=True)
    parser.add_argument("--benchmark-summary", type=Path, required=True)
    parser.add_argument("--elf-log", type=Path, required=True)
    parser.add_argument("--ptx-log", type=Path, required=True)
    parser.add_argument("--sanitizer-distance-log", type=Path, required=True)
    parser.add_argument("--sanitizer-orientation-log", type=Path, required=True)
    parser.add_argument("--sanitizer-power-log", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args(arguments)


def main(arguments: Sequence[str] | None = None) -> int:
    options = parse_arguments(arguments)
    if SHA_RE.fullmatch(options.git_sha) is None:
        fail("--git-sha must be a canonical forty-character lowercase SHA")
    if IMAGE_ID_RE.fullmatch(options.image_id) is None:
        fail("--image-id must be a canonical sha256 image identifier")

    distance = read_json_object(options.distance_summary, "distance differential")
    orientation = read_json_object(
        options.orientation_summary, "orientation differential"
    )
    power = read_json_object(options.power_summary, "power-bisector differential")
    benchmark = read_json_object(options.benchmark_summary, "benchmark")
    validate_distance(distance)
    validate_orientation(orientation)
    validate_power(power)
    validate_benchmark(benchmark)

    elf = read_text(options.elf_log, "cuobjdump ELF log")
    architectures = sorted(set(SM_RE.findall(elf)))
    if architectures != ["sm_120"]:
        fail(f"AOT architectures must be exactly ['sm_120'], got {architectures}")
    if read_text(options.ptx_log, "cuobjdump PTX log", allow_empty=True).strip():
        fail("the qualified runner must not contain a PTX entry")
    validate_sanitizer_log(
        options.sanitizer_distance_log, "compare_squared_distances"
    )
    validate_sanitizer_log(options.sanitizer_orientation_log, "orientation_3d")
    validate_sanitizer_log(options.sanitizer_power_log, "power_bisector_side")

    artifact = {
        "backend": "cuda_g4",
        "checks": {
            "aot_architectures": architectures,
            "benchmark": benchmark,
            "compute_sanitizer_memcheck": {
                "compare_squared_distances": "passed",
                "orientation_3d": "passed",
                "power_bisector_side": "passed",
            },
            "cpu_replay": "multiprecision_only",
            "differentials": {
                "compare_squared_distances": distance,
                "orientation_3d": orientation,
                "power_bisector_side": power,
            },
            "ptx_entries": 0,
        },
        "git": {"clean": True, "sha": options.git_sha},
        "image_id": options.image_id,
        "mode": "certified",
        "phase": "2B",
        "predicates": [
            "compare_squared_distances",
            "orientation_3d",
            "power_bisector_side",
        ],
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
                "distance": distance,
                "orientation": orientation,
                "power": power,
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
