#!/usr/bin/env python3
"""Validate the host-simulated Phase 2B warm-context benchmark contract."""

from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
from typing import Any, NoReturn

SCHEMA = "morsehgp3d.phase2b.warm_context_e2e.v1"
SCOPE = "warm_context_e2e"
TIMING_SCOPE = (
    "steady_clock_around_async_api_get_includes_validation_packing_transfers_"
    "kernel_fallback_synchronization_excludes_input_generation"
)
TOP_LEVEL_KEYS = frozenset(
    {
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
)
RESULT_KEYS = frozenset(
    {
        "counters",
        "mad_ns",
        "max_ns",
        "min_ns",
        "p50_ns",
        "p95_ns",
        "predicate",
        "samples_ns",
    }
)
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
PREDICATES = {
    "distance": "compare_squared_distances",
    "orientation": "orientation_3d",
    "power": "power_bisector_side",
}


def fail(message: str) -> NoReturn:
    raise AssertionError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def reject_json_constant(value: str) -> NoReturn:
    raise ValueError(f"non-finite JSON constant: {value}")


def exact_nonnegative_integer(value: Any, context: str) -> int:
    require(
        type(value) is int and value >= 0, f"{context} must be a non-negative integer"
    )
    return value


def nearest_rank(sorted_samples: list[int], percentile: int) -> int:
    require(bool(sorted_samples), "nearest-rank samples must not be empty")
    require(0 < percentile <= 100, "nearest-rank percentile is invalid")
    rank = (len(sorted_samples) * percentile + 99) // 100
    return sorted_samples[rank - 1]


def validate_counter_record(
    counters: Any,
    *,
    cases: int,
    repeats: int,
    context: str,
) -> None:
    require(isinstance(counters, dict), f"{context} must be an object")
    require(frozenset(counters) == COUNTER_KEYS, f"{context} keys changed")
    values = {
        key: exact_nonnegative_integer(counters.get(key), f"{context}.{key}")
        for key in COUNTER_KEYS
    }
    require(
        values["gpu_inputs"] == cases * repeats, f"{context}.gpu_inputs is not closed"
    )
    require(
        values["gpu_fp64_certified"] + values["gpu_unknown_forwarded"]
        == values["gpu_inputs"],
        f"{context} GPU tri-state accounting is not closed",
    )
    require(
        values["cpu_fp64_filtered_certified"]
        + values["cpu_expansion_certified"]
        + values["cpu_multiprecision_certified"]
        == values["gpu_unknown_forwarded"],
        f"{context} CPU fallback accounting is not closed",
    )
    require(
        values["gpu_unknown_forwarded"] > 0
        and values["async_fallback_batches"] == repeats
        and values["exact_zeros"] > 0,
        f"{context} does not expose the simulated asynchronous fallback",
    )
    require(
        values["gpu_known_audited"] == 0, f"{context} unexpectedly enabled auditing"
    )
    require(values["remaining_unknown"] == 0, f"{context} retained unknown decisions")


def validate_output(stdout: bytes) -> None:
    try:
        text = stdout.decode("ascii")
    except UnicodeDecodeError as error:
        fail(f"benchmark output is not ASCII: {error}")
    require(text.endswith("\n"), "benchmark output lacks its final newline")
    lines = text.splitlines()
    require(len(lines) == 1, "benchmark must emit exactly one JSON record")
    line = lines[0]
    try:
        value = json.loads(
            line,
            object_pairs_hook=reject_duplicate_keys,
            parse_constant=reject_json_constant,
        )
    except (json.JSONDecodeError, ValueError) as error:
        fail(f"benchmark emitted invalid JSON: {error}")
    require(isinstance(value, dict), "benchmark record must be an object")
    canonical = json.dumps(
        value, ensure_ascii=True, separators=(",", ":"), sort_keys=True
    )
    require(line == canonical, "benchmark record is not canonical compact JSON")
    require(frozenset(value) == TOP_LEVEL_KEYS, "benchmark top-level keys changed")
    require(value.get("schema") == SCHEMA, "benchmark schema changed")
    require(value.get("scope") == SCOPE, "benchmark scope changed")
    require(value.get("timing_scope") == TIMING_SCOPE, "benchmark timing scope changed")
    require(
        value.get("audit_gpu_signs") is False,
        "benchmark audit mode must be explicit and disabled",
    )
    require(
        value.get("fallback") == "gpu_unknown_to_async_cpu_exact",
        "benchmark fallback policy changed",
    )
    cases = exact_nonnegative_integer(value.get("cases"), "benchmark.cases")
    repeats = exact_nonnegative_integer(value.get("repeats"), "benchmark.repeats")
    require(cases == 9, "host benchmark must use nine cases")
    require(repeats == 3, "host benchmark must use three measured repeats")
    warmup_repeats = exact_nonnegative_integer(
        value.get("warmup_repeats"), "benchmark.warmup_repeats"
    )
    require(warmup_repeats == 1, "benchmark must perform one unmeasured warmup")
    results = value.get("results")
    require(isinstance(results, dict), "benchmark.results must be an object")
    require(
        set(results) == set(PREDICATES), "benchmark results do not cover all predicates"
    )
    for name, predicate in PREDICATES.items():
        record = results.get(name)
        context = f"benchmark.results.{name}"
        require(isinstance(record, dict), f"{context} must be an object")
        require(frozenset(record) == RESULT_KEYS, f"{context} keys changed")
        require(record.get("predicate") == predicate, f"{context}.predicate changed")
        samples = record.get("samples_ns")
        require(isinstance(samples, list), f"{context}.samples_ns must be an array")
        require(len(samples) == repeats, f"{context}.samples_ns cardinality changed")
        exact_samples = [
            exact_nonnegative_integer(sample, f"{context}.samples_ns[{index}]")
            for index, sample in enumerate(samples)
        ]
        require(
            all(sample > 0 for sample in exact_samples), f"{context} has a zero sample"
        )
        sorted_samples = sorted(exact_samples)
        expected_p50 = nearest_rank(sorted_samples, 50)
        expected_p95 = nearest_rank(sorted_samples, 95)
        expected_mad = nearest_rank(
            sorted(abs(sample - expected_p50) for sample in exact_samples), 50
        )
        minimum = exact_nonnegative_integer(record.get("min_ns"), f"{context}.min_ns")
        maximum = exact_nonnegative_integer(record.get("max_ns"), f"{context}.max_ns")
        p50 = exact_nonnegative_integer(record.get("p50_ns"), f"{context}.p50_ns")
        p95 = exact_nonnegative_integer(record.get("p95_ns"), f"{context}.p95_ns")
        mad = exact_nonnegative_integer(record.get("mad_ns"), f"{context}.mad_ns")
        require(
            minimum == sorted_samples[0]
            and maximum == sorted_samples[-1]
            and p50 == expected_p50
            and p95 == expected_p95
            and mad == expected_mad,
            f"{context} latency statistics do not replay from raw samples",
        )
        require(
            0 < minimum <= p50 <= p95 <= maximum,
            f"{context} latency quantiles are inconsistent",
        )
        validate_counter_record(
            record.get("counters"),
            cases=cases,
            repeats=repeats,
            context=f"{context}.counters",
        )


def main(arguments: list[str]) -> int:
    if len(arguments) != 2:
        print("usage: check_warm_context_benchmark.py HARNESS", file=sys.stderr)
        return 2
    harness = Path(arguments[1]).resolve()
    if not harness.is_file():
        print(f"warm-context harness does not exist: {harness}", file=sys.stderr)
        return 2
    try:
        completed = subprocess.run(
            [str(harness), "--benchmark-warm-context-e2e"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
            timeout=20.0,
        )
        require(
            completed.returncode == 0,
            f"benchmark failed with status {completed.returncode}",
        )
        require(not completed.stderr, "benchmark emitted stderr")
        validate_output(completed.stdout)
    except (AssertionError, OSError, subprocess.TimeoutExpired) as error:
        print(f"Warm-context benchmark contract failed: {error}", file=sys.stderr)
        return 1
    print(
        json.dumps(
            {
                "cases": 9,
                "cuda_runtime_executed": False,
                "predicates": list(PREDICATES.values()),
                "repeats": 3,
                "schema": SCHEMA,
                "scope": SCOPE,
            },
            ensure_ascii=True,
            separators=(",", ":"),
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
