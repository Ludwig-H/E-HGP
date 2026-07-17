#!/usr/bin/env python3
"""Benchmark the Phase 2B GPU predicate replay CLI end to end."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import io
import json
import math
import os
from pathlib import Path
import statistics
import struct
import subprocess
import sys
import time
from typing import Any, NoReturn, Sequence


BENCHMARK_SCHEMA = "morsehgp3d.phase2b.predicates.benchmark.v1"
MAXIMUM_CASE_COUNT = 1_048_576
TEMPLATE_COUNT = 512
SUMMARY_KEYS = frozenset({"audit_gpu_signs", "counters", "kind", "schema"})
COUNTER_KEYS = frozenset(
    {
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
)


@dataclass(frozen=True)
class PredicateConfiguration:
    replay_name: str
    replay_id_base: int
    summary_schema: str


PREDICATES = {
    "distance": PredicateConfiguration(
        "compare_squared_distances",
        10_000_000_000,
        "morsehgp3d.phase2b.distance_filter.v1",
    ),
    "orientation": PredicateConfiguration(
        "orientation_3d",
        20_000_000_000,
        "morsehgp3d.phase2b.orientation_3d_filter.v1",
    ),
    "power": PredicateConfiguration(
        "power_bisector_side",
        30_000_000_000,
        "morsehgp3d.phase2b.power_bisector_side_filter.v1",
    ),
}

POWER_WITNESS_DENOMINATORS = (1, 2, 3, 5, 7, 8)


def fail(message: str) -> NoReturn:
    raise AssertionError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def binary64_word(value: int) -> str:
    return struct.pack(">d", float(value)).hex()


def point_words(point: tuple[int, int, int]) -> tuple[str, str, str]:
    return (
        binary64_word(point[0]),
        binary64_word(point[1]),
        binary64_word(point[2]),
    )


def distance_template(index: int) -> bytes:
    pattern = index % TEMPLATE_COUNT
    witness = (
        (pattern * 17) % 193 - 96,
        (pattern * 31) % 181 - 90,
        (pattern * 47) % 173 - 86,
    )
    near_delta = (
        1 + pattern % 4,
        2 + (pattern // 4) % 3,
        -(1 + (pattern // 12) % 2),
    )
    far_delta = (
        32 + pattern % 16,
        -(8 + (pattern // 16) % 8),
        6 + (pattern // 128) % 4,
    )
    near = tuple(witness[axis] + near_delta[axis] for axis in range(3))
    far = tuple(witness[axis] + far_delta[axis] for axis in range(3))
    near_norm = sum(component * component for component in near_delta)
    far_norm = sum(component * component for component in far_delta)
    require(near_norm < far_norm, "distance template is not well-conditioned")
    left, right = (near, far) if index % 2 == 0 else (far, near)
    words = (*point_words(witness), *point_words(left), *point_words(right))
    return f"compare_squared_distances {' '.join(words)}".encode("ascii")


def orientation_template(index: int) -> bytes:
    pattern = index % TEMPLATE_COUNT
    a = (
        (pattern * 19) % 197 - 98,
        (pattern * 37) % 179 - 89,
        (pattern * 53) % 167 - 83,
    )
    u = (8 + pattern % 8, 0, 0)
    v = ((pattern % 7) - 3, 9 + (pattern // 8) % 8, 0)
    height = 10 + (pattern // 64) % 8
    w = (
        ((pattern // 3) % 7) - 3,
        ((pattern // 5) % 7) - 3,
        height if index % 2 == 0 else -height,
    )
    b = tuple(a[axis] + u[axis] for axis in range(3))
    c = tuple(a[axis] + v[axis] for axis in range(3))
    d = tuple(a[axis] + w[axis] for axis in range(3))
    determinant = u[0] * v[1] * w[2]
    require(
        determinant > 0 if index % 2 == 0 else determinant < 0,
        "orientation template lost its alternating strict sign",
    )
    words = (*point_words(a), *point_words(b), *point_words(c), *point_words(d))
    return f"orientation_3d {' '.join(words)}".encode("ascii")


def power_template(index: int) -> bytes:
    pattern = index % TEMPLATE_COUNT
    cardinality = 1 + pattern % 10
    denominator = POWER_WITNESS_DENOMINATORS[
        (pattern // 10) % len(POWER_WITNESS_DENOMINATORS)
    ]
    center = (
        (pattern * 23) % 127 - 63,
        (pattern * 41) % 113 - 56,
        (pattern * 59) % 109 - 54,
    )
    # A_x is coprime with D for D > 1, so the serialized ExactRational3 is
    # already reduced. The resulting witness is center + (1 / D, 0, 0).
    witness_numerators = (
        denominator * center[0] + (1 if denominator > 1 else 0),
        denominator * center[1],
        denominator * center[2],
    )
    require(
        math.gcd(
            denominator,
            math.gcd(
                abs(witness_numerators[0]),
                math.gcd(
                    abs(witness_numerators[1]),
                    abs(witness_numerators[2]),
                ),
            ),
        )
        == 1,
        "power-bisector witness is not reduced",
    )

    common_points = tuple(
        (
            center[0] + slot + 2,
            center[1] + (3 * slot) % 11 - 5,
            center[2] + (5 * slot) % 13 - 6,
        )
        for slot in range(cardinality - 1)
    )
    near = center
    far = (center[0] + 32, center[1] + 7, center[2] - 5)
    points = (*common_points, near, far)
    common_ids = tuple(range(cardinality - 1))
    near_ids = (*common_ids, cardinality - 1)
    far_ids = (*common_ids, cardinality)
    r_ids, q_ids = (near_ids, far_ids) if index % 2 == 0 else (far_ids, near_ids)

    def homogeneous_cost_difference() -> int:
        coordinate_delta = tuple(
            sum(points[point_id][axis] for point_id in r_ids)
            - sum(points[point_id][axis] for point_id in q_ids)
            for axis in range(3)
        )
        norm_delta = sum(
            sum(coordinate * coordinate for coordinate in points[point_id])
            for point_id in r_ids
        ) - sum(
            sum(coordinate * coordinate for coordinate in points[point_id])
            for point_id in q_ids
        )
        return denominator * norm_delta - 2 * sum(
            witness_numerators[axis] * coordinate_delta[axis] for axis in range(3)
        )

    homogeneous_value = homogeneous_cost_difference()
    require(
        homogeneous_value < 0 if index % 2 == 0 else homogeneous_value > 0,
        "power-bisector template lost its alternating strict sign",
    )
    words = tuple(word for point in points for word in point_words(point))
    command = " ".join(
        (
            "power_bisector_side",
            *(str(value) for value in witness_numerators),
            str(denominator),
            str(len(points)),
            *words,
            str(cardinality),
            *(str(point_id) for point_id in r_ids),
            str(cardinality),
            *(str(point_id) for point_id in q_ids),
        )
    )
    return command.encode("ascii")


def generate_batch(predicate: str, case_count: int) -> bytes:
    require(
        0 < case_count <= MAXIMUM_CASE_COUNT,
        "benchmark case count is outside the runner batch bound",
    )
    configuration = PREDICATES[predicate]
    template_factory = {
        "distance": distance_template,
        "orientation": orientation_template,
        "power": power_template,
    }[predicate]
    templates = tuple(template_factory(index) for index in range(TEMPLATE_COUNT))
    output = io.BytesIO()
    for index in range(case_count):
        replay_id = configuration.replay_id_base + index
        output.write(str(replay_id).encode("ascii"))
        output.write(b" ")
        output.write(templates[index % TEMPLATE_COUNT])
        output.write(b"\n")
    return output.getvalue()


def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def reject_json_constant(value: str) -> NoReturn:
    raise ValueError(f"non-finite JSON constant: {value}")


def require_exact_nonnegative_integer(value: Any, context: str) -> int:
    if type(value) is not int or value < 0:
        fail(f"{context} must be a non-negative integer")
    return value


def validate_summary(
    stdout: bytes,
    predicate: str,
    case_count: int,
    context: str,
) -> None:
    try:
        text = stdout.decode("ascii")
    except UnicodeDecodeError as error:
        fail(f"{context} output is not ASCII JSON: {error}")
    require(text.endswith("\n"), f"{context} output lacks its final newline")
    lines = text.splitlines()
    require(len(lines) == 1, f"{context} must emit exactly one summary")
    line = lines[0]
    try:
        summary = json.loads(
            line,
            object_pairs_hook=reject_duplicate_keys,
            parse_constant=reject_json_constant,
        )
    except (json.JSONDecodeError, ValueError) as error:
        fail(f"{context} emitted invalid JSON: {error}")
    require(isinstance(summary, dict), f"{context} summary must be an object")
    canonical = json.dumps(
        summary, ensure_ascii=True, separators=(",", ":"), sort_keys=True
    )
    require(line == canonical, f"{context} summary is not canonical compact JSON")
    require(
        frozenset(summary) == SUMMARY_KEYS,
        f"{context} summary keys differ from the runner schema",
    )
    configuration = PREDICATES[predicate]
    require(summary.get("kind") == "summary", f"{context}.kind is invalid")
    require(
        summary.get("schema") == configuration.summary_schema,
        f"{context}.schema is invalid",
    )
    require(
        summary.get("audit_gpu_signs") is False,
        f"{context} unexpectedly enabled GPU-known auditing",
    )
    counters = summary.get("counters")
    require(isinstance(counters, dict), f"{context}.counters must be an object")
    require(
        frozenset(counters) == COUNTER_KEYS,
        f"{context}.counters keys differ from the runner schema",
    )
    expected = {
        "async_fallback_batches": 0,
        "cpu_expansion_certified": 0,
        "cpu_fp64_filtered_certified": 0,
        "cpu_multiprecision_certified": 0,
        "exact_zeros": 0,
        "gpu_fp64_certified": case_count,
        "gpu_inputs": case_count,
        "gpu_known_audited": 0,
        "gpu_unknown_forwarded": 0,
        "remaining_unknown": 0,
    }
    for key, expected_value in expected.items():
        actual = require_exact_nonnegative_integer(
            counters.get(key), f"{context}.counters.{key}"
        )
        require(
            actual == expected_value,
            f"{context}.counters.{key}: expected {expected_value}, got {actual}",
        )


def run_once(
    runner: Path,
    predicate: str,
    payload: bytes,
    case_count: int,
    timeout_seconds: float,
    context: str,
) -> float:
    started = time.perf_counter()
    completed = subprocess.run(
        [str(runner), "--summary-only"],
        input=payload,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        timeout=timeout_seconds,
    )
    elapsed = time.perf_counter() - started
    if completed.returncode != 0 or completed.stderr:
        fail(
            f"{context} failed: returncode={completed.returncode}, "
            f"stdout={completed.stdout!r}, stderr={completed.stderr!r}"
        )
    validate_summary(completed.stdout, predicate, case_count, context)
    return elapsed


def rate_record(elapsed: float, case_count: int, input_bytes: int) -> dict[str, float]:
    require(elapsed > 0.0, "the monotonic benchmark clock did not advance")
    return {
        "cases_per_second": case_count / elapsed,
        "seconds": elapsed,
        "stdin_bytes_per_second": input_bytes / elapsed,
    }


def benchmark_predicate(
    runner: Path,
    predicate: str,
    payload: bytes,
    case_count: int,
    repeats: int,
    timeout_seconds: float,
) -> dict[str, Any]:
    run_once(
        runner,
        predicate,
        payload,
        case_count,
        timeout_seconds,
        f"{predicate} unmeasured probe",
    )
    elapsed_samples = [
        run_once(
            runner,
            predicate,
            payload,
            case_count,
            timeout_seconds,
            f"{predicate} measured repeat {repeat + 1}",
        )
        for repeat in range(repeats)
    ]
    minimum = min(elapsed_samples)
    median = statistics.median(elapsed_samples)
    return {
        "input_bytes": len(payload),
        "median_elapsed": rate_record(median, case_count, len(payload)),
        "minimum_elapsed": rate_record(minimum, case_count, len(payload)),
        "predicate": PREDICATES[predicate].replay_name,
    }


def bounded_case_count(value: str) -> int:
    try:
        parsed = int(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("must be an integer") from error
    if parsed <= 0 or parsed > MAXIMUM_CASE_COUNT:
        raise argparse.ArgumentTypeError(
            f"must be between 1 and {MAXIMUM_CASE_COUNT}"
        )
    return parsed


def positive_integer(value: str) -> int:
    try:
        parsed = int(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("must be an integer") from error
    if parsed <= 0:
        raise argparse.ArgumentTypeError("must be positive")
    return parsed


def positive_float(value: str) -> float:
    try:
        parsed = float(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("must be a number") from error
    if not (parsed > 0.0 and math.isfinite(parsed)):
        raise argparse.ArgumentTypeError("must be positive and finite")
    return parsed


def main(arguments: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runner", type=Path)
    parser.add_argument("--cases", type=bounded_case_count, default=262_144)
    parser.add_argument("--repeats", type=positive_integer, default=3)
    parser.add_argument(
        "--predicate",
        choices=("distance", "orientation", "power", "both", "all"),
        default="all",
    )
    parser.add_argument("--timeout-seconds", type=positive_float, default=120.0)
    options = parser.parse_args(arguments)
    if not options.runner.is_file():
        parser.error(f"GPU predicate runner does not exist: {options.runner}")
    if not os.access(options.runner, os.X_OK):
        parser.error(f"GPU predicate runner is not executable: {options.runner}")

    if options.predicate == "all":
        selected = ("distance", "orientation", "power")
    elif options.predicate == "both":
        selected = ("distance", "orientation")
    else:
        selected = (options.predicate,)
    try:
        # Input construction is deliberately complete before the unmeasured
        # probe, and each immutable batch is reused for every measured run.
        # Each invocation remains a cold process with a fresh CUDA context.
        batches = {
            predicate: generate_batch(predicate, options.cases)
            for predicate in selected
        }
        runner = options.runner.resolve()
        results = {
            predicate: benchmark_predicate(
                runner,
                predicate,
                batches[predicate],
                options.cases,
                options.repeats,
                options.timeout_seconds,
            )
            for predicate in selected
        }
    except (AssertionError, OSError, subprocess.TimeoutExpired) as error:
        print(f"Phase 2B predicate benchmark failed: {error}", file=sys.stderr)
        return 1

    print(
        json.dumps(
            {
                "cases": options.cases,
                "repeats": options.repeats,
                "results": results,
                "schema": BENCHMARK_SCHEMA,
                "scope": "cold_process_end_to_end",
            },
            ensure_ascii=True,
            separators=(",", ":"),
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
