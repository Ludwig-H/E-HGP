#!/usr/bin/env python3
"""Differentially qualify the bounded Phase 2B orientation_3d GPU filter."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from fractions import Fraction
import json
from pathlib import Path
import random
import struct
import subprocess
import sys
from typing import Any, NoReturn, Sequence


SCHEMA = "morsehgp3d.phase2b.orientation_3d_filter.v1"
PREDICATE = "orientation_3d"
INTEGER_KNOWN_CASE_COUNT = 2_049
ADVERSARIAL_PAIR_COUNT = 8
ADVERSARIAL_KNOWN_CASE_COUNT = 2 * ADVERSARIAL_PAIR_COUNT
REQUIRED_GPU_KNOWN_CASE_COUNT = (
    INTEGER_KNOWN_CASE_COUNT + ADVERSARIAL_KNOWN_CASE_COUNT
)
OPTIONAL_GPU_CASE_COUNT = 2
SIGN_NAMES = frozenset({"negative", "zero", "positive"})
GPU_SIGN_NAMES = frozenset({"negative", "unknown", "positive"})
STAGE_NAMES = frozenset(
    {"fp64_filtered", "expansion", "cpu_multiprecision"}
)
DECISION_KEYS = frozenset(
    {
        "certification_stage",
        "gpu_filter_sign",
        "kind",
        "predicate",
        "replay_command",
        "replay_id",
        "sign",
    }
)
SUMMARY_KEYS = frozenset({"audit_gpu_signs", "counters", "kind", "schema"})
GPU_COUNTER_KEYS = frozenset(
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
CPU_DECISION_KEYS = frozenset(
    {"certification_stage", "counters", "predicate", "sign"}
)
CPU_COUNTER_KEYS = frozenset(
    {
        "cpu_multiprecision_certified",
        "exact_zeros",
        "expansion_certified",
        "fp32_proposals",
        "fp64_filtered_certified",
        "remaining_unknown",
    }
)


def fail(message: str) -> NoReturn:
    raise AssertionError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def require_exact_keys(
    value: dict[str, Any], expected: frozenset[str], context: str
) -> None:
    actual = frozenset(value)
    if actual != expected:
        fail(
            f"{context} keys differ: missing={sorted(expected - actual)}, "
            f"unexpected={sorted(actual - expected)}"
        )


def require_exact_integer(value: Any, context: str, *, minimum: int = 0) -> int:
    if type(value) is not int or value < minimum:
        fail(f"{context} must be an integer greater than or equal to {minimum}")
    return value


def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def reject_json_constant(value: str) -> NoReturn:
    raise ValueError(f"non-finite JSON constant: {value}")


def parse_jsonl(output: str, context: str) -> list[dict[str, Any]]:
    require(bool(output), f"{context} produced no JSONL output")
    require(output.endswith("\n"), f"{context} output lacks its final newline")
    records: list[dict[str, Any]] = []
    for line_number, line in enumerate(output.splitlines(), start=1):
        require(bool(line), f"{context} contains an empty line at {line_number}")
        try:
            value = json.loads(
                line,
                object_pairs_hook=reject_duplicate_keys,
                parse_constant=reject_json_constant,
            )
        except (json.JSONDecodeError, ValueError) as error:
            fail(f"{context} line {line_number} is invalid JSON: {error}")
        require(
            isinstance(value, dict),
            f"{context} line {line_number} must be a JSON object",
        )
        canonical = json.dumps(
            value, ensure_ascii=True, separators=(",", ":"), sort_keys=True
        )
        require(
            line == canonical,
            f"{context} line {line_number} is not canonical compact JSON",
        )
        records.append(value)
    return records


def binary64_word(value: int) -> str:
    return struct.pack(">d", float(value)).hex()


def binary64_fraction(word: str) -> Fraction:
    require(
        len(word) == 16 and all(character in "0123456789abcdef" for character in word),
        f"non-canonical binary64 word in test corpus: {word!r}",
    )
    bits = int(word, 16)
    exponent_bits = (bits >> 52) & 0x7FF
    fraction_bits = bits & ((1 << 52) - 1)
    require(exponent_bits != 0x7FF, "the exact oracle received a non-finite word")
    sign = -1 if bits >> 63 else 1
    if exponent_bits == 0:
        significand = fraction_bits
        exponent = -1074
    else:
        significand = (1 << 52) | fraction_bits
        exponent = exponent_bits - 1023 - 52
    numerator = sign * significand
    if exponent >= 0:
        return Fraction(numerator << exponent, 1)
    return Fraction(numerator, 1 << -exponent)


def exact_orientation_sign(words: Sequence[str]) -> str:
    require(len(words) == 12, "an orientation_3d case must contain twelve words")
    values = tuple(binary64_fraction(word) for word in words)
    a = values[0:3]
    b = values[3:6]
    c = values[6:9]
    d = values[9:12]
    u = tuple(b[axis] - a[axis] for axis in range(3))
    v = tuple(c[axis] - a[axis] for axis in range(3))
    w = tuple(d[axis] - a[axis] for axis in range(3))
    determinant = (
        u[0] * (v[1] * w[2] - v[2] * w[1])
        - u[1] * (v[0] * w[2] - v[2] * w[0])
        + u[2] * (v[0] * w[1] - v[1] * w[0])
    )
    if determinant < 0:
        return "negative"
    if determinant > 0:
        return "positive"
    return "zero"


@dataclass(frozen=True)
class Case:
    replay_id: int
    words: tuple[str, ...]
    label: str
    required_gpu_sign: str | None = None
    required_stage: str | None = None
    allow_gpu_unknown: bool = False
    unknown_stage: str | None = None

    @property
    def replay_command(self) -> str:
        return f"{PREDICATE} {' '.join(self.words)}"

    @property
    def gpu_input(self) -> str:
        return f"{self.replay_id} {self.replay_command}"

    @property
    def oracle_sign(self) -> str:
        return exact_orientation_sign(self.words)


def make_point(values: Sequence[int]) -> tuple[str, str, str]:
    require(len(values) == 3, "a test point must have three coordinates")
    return (
        binary64_word(values[0]),
        binary64_word(values[1]),
        binary64_word(values[2]),
    )


def make_known_cases() -> list[Case]:
    generator = random.Random(0x4F5249454E543344)
    cases: list[Case] = []
    for index in range(INTEGER_KNOWN_CASE_COUNT):
        a = tuple(generator.randrange(-64, 65) for _ in range(3))
        u = (8 + generator.randrange(8), 0, 0)
        v = (
            generator.randrange(-4, 5),
            9 + generator.randrange(8),
            0,
        )
        z_magnitude = 10 + generator.randrange(8)
        z_sign = 1 if index % 2 == 0 else -1
        w = (
            generator.randrange(-4, 5),
            generator.randrange(-4, 5),
            z_sign * z_magnitude,
        )
        b = tuple(a[axis] + u[axis] for axis in range(3))
        c = tuple(a[axis] + v[axis] for axis in range(3))
        d = tuple(a[axis] + w[axis] for axis in range(3))
        expected = "positive" if z_sign > 0 else "negative"
        replay_id = 80_000_003 + (INTEGER_KNOWN_CASE_COUNT - index) * 29
        words = (*make_point(a), *make_point(b), *make_point(c), *make_point(d))
        case = Case(
            replay_id,
            words,
            f"well-conditioned-{index}",
            expected,
            "fp64_filtered",
        )
        require(
            case.oracle_sign == expected,
            f"generated case {index} does not have its constructed exact sign",
        )
        cases.append(case)
    return cases


def make_adversarial_cases() -> list[Case]:
    # These binary64 bit patterns exercise inexact products, signed branches,
    # mixed exponents, large-offset subtraction, signed zero and cancellation.
    # Their determinants remain separated enough from zero for the directed
    # interval to be required to certify both permutations on the device.
    bases = (
        (
            "all-inexact",
            (
                "0000000000000000", "0000000000000000", "0000000000000000",
                "3fb999999999999a", "0000000000000000", "0000000000000000",
                "3fc999999999999a", "3fd3333333333333", "0000000000000000",
                "3fd999999999999a", "3fe0000000000000", "3fe3333333333333",
            ),
            "positive",
        ),
        (
            "signed-inexact",
            (
                "bff4000000000000", "3fe6666666666666", "c008000000000000",
                "bfb999999999999a", "3fe6666666666666", "c008000000000000",
                "4002000000000000", "bff8000000000000", "c008000000000000",
                "c010000000000000", "4016000000000000", "3ffc000000000000",
            ),
            "negative",
        ),
        (
            "mixed-exponents",
            (
                "0000000000000000", "0000000000000000", "0000000000000000",
                "4c70000000000000", "0000000000000000", "0000000000000000",
                "3370000000000000", "39b0000000000000", "0000000000000000",
                "3690000000000000", "3b40000000000000", "3cd0000000000000",
            ),
            "positive",
        ),
        (
            "large-offset-subtraction",
            (
                "4c70000000000000", "cc70000000000000", "4c70000000000000",
                "4c70000000000004", "cc70000000000000", "4c70000000000000",
                "4c70000000000002", "cc6ffffffffffff8", "4c70000000000000",
                "4c6ffffffffffffe", "cc6ffffffffffffc", "4c70000000000004",
            ),
            "positive",
        ),
        (
            "signed-zero-branches",
            (
                "8000000000000000", "0000000000000000", "8000000000000000",
                "c020000000000000", "8000000000000000", "0000000000000000",
                "4000000000000000", "c010000000000000", "8000000000000000",
                "bff0000000000000", "4008000000000000", "4018000000000000",
            ),
            "positive",
        ),
        (
            "cancellation-gap",
            (
                "0000000000000000", "0000000000000000", "0000000000000000",
                "3ff0000000000000", "3ff0000000000000", "3ff0000000000000",
                "3ff0000000000000", "3ff0000100000000", "3ff0000000000000",
                "3ff0000000000000", "3ff0000000000000", "3ff0000100000000",
            ),
            "positive",
        ),
        (
            "exact-dyadic-translation-2p40",
            (
                "4270000000000000", "4270000000000000", "4270000000000000",
                "4270000000001000", "4270000000000000", "4270000000000000",
                "4270000000000000", "4270000000001000", "4270000000000000",
                "4270000000000000", "4270000000000000", "4270000000001000",
            ),
            "positive",
        ),
        (
            "near-coplanar-2m52",
            (
                "0000000000000000", "0000000000000000", "0000000000000000",
                "3ff0000000000000", "0000000000000000", "0000000000000000",
                "0000000000000000", "3ff0000000000000", "0000000000000000",
                "3ff0000000000000", "3ff0000000000000", "3cb0000000000000",
            ),
            "positive",
        ),
    )
    require(
        len(bases) == ADVERSARIAL_PAIR_COUNT,
        "the adversarial directed-rounding pair count changed",
    )
    cases: list[Case] = []
    for index, (label, words, expected) in enumerate(bases):
        original = Case(
            1_001 + 100 * index,
            words,
            f"adversarial-{label}-{expected}",
            expected,
            "fp64_filtered",
        )
        swapped_words = (
            *words[0:3],
            *words[6:9],
            *words[3:6],
            *words[9:12],
        )
        opposite = "negative" if expected == "positive" else "positive"
        swapped = Case(
            1_038 + 100 * index,
            swapped_words,
            f"adversarial-{label}-{opposite}",
            opposite,
            "fp64_filtered",
        )
        require(
            original.oracle_sign == expected,
            f"adversarial case {label} lost its exact {expected} sign",
        )
        require(
            swapped.oracle_sign == opposite,
            f"adversarial case {label} did not reverse under point exchange",
        )
        cases.extend((original, swapped))
    return cases


def make_optional_gpu_cases() -> list[Case]:
    # These strict signs are important DAZ/FTZ regressions, but a bounded GPU
    # interval may conservatively return unknown. Either outcome is valid; the
    # adaptive CPU fallback must then certify the exact sign by expansion.
    zero = "0000000000000000"
    cases = [
        Case(
            2_007,
            (
                zero, zero, zero,
                "3ff0000000000000", zero, zero,
                zero, "3ff0000000000000", zero,
                "3ff0000000000000", "3ff0000000000000", "0000000000000001",
            ),
            "minimum-subnormal-height",
            allow_gpu_unknown=True,
            unknown_stage="expansion",
        ),
        Case(
            2_044,
            (
                zero, zero, zero,
                "0000000000000001", zero, "2d30000000000000",
                zero, "5f30000000000000", zero,
                "2ec0000000000000", zero, "5f30000000000000",
            ),
            "daz-regression",
            allow_gpu_unknown=True,
            unknown_stage="expansion",
        ),
    ]
    require(
        len(cases) == OPTIONAL_GPU_CASE_COUNT,
        "the optional GPU adversarial case count changed",
    )
    for case in cases:
        require(
            case.oracle_sign == "positive",
            f"optional adversarial case {case.label} lost its exact sign",
        )
    return cases


def make_corpus() -> list[Case]:
    zero = "0000000000000000"
    one = "3ff0000000000000"
    minimum_subnormal = "0000000000000001"
    maximum = "7fefffffffffffff"

    equality = Case(
        7,
        (
            zero, zero, zero,
            one, zero, zero,
            zero, one, zero,
            one, one, zero,
        ),
        "exact-coplanarity",
        "unknown",
        "expansion",
        allow_gpu_unknown=True,
        unknown_stage="expansion",
    )
    underflow = Case(
        11,
        (
            zero, zero, zero,
            minimum_subnormal, zero, zero,
            zero, minimum_subnormal, zero,
            zero, zero, minimum_subnormal,
        ),
        "determinant-underflow",
        "unknown",
        "cpu_multiprecision",
        allow_gpu_unknown=True,
        unknown_stage="cpu_multiprecision",
    )
    overflow = Case(
        13,
        (
            zero, zero, zero,
            maximum, zero, zero,
            zero, maximum, zero,
            zero, zero, maximum,
        ),
        "determinant-overflow",
        "unknown",
        "cpu_multiprecision",
        allow_gpu_unknown=True,
        unknown_stage="cpu_multiprecision",
    )
    cases = make_adversarial_cases() + make_optional_gpu_cases() + make_known_cases()
    cases.insert(0, equality)
    cases.insert(257, underflow)
    cases.insert(1_026, overflow)
    require(len(cases) > 256 * 8, "the corpus must span more than eight CUDA blocks")
    replay_ids = [case.replay_id for case in cases]
    require(
        len(set(replay_ids)) == len(cases),
        "the positive corpus contains a duplicate replay identifier",
    )
    require(
        replay_ids != sorted(replay_ids)
        and all(
            abs(left - right) != 1
            for left, right in zip(replay_ids, replay_ids[1:])
        ),
        "the corpus must retain non-sequential replay identifiers",
    )
    require(
        sum(case.required_gpu_sign in {"negative", "positive"} for case in cases)
        == REQUIRED_GPU_KNOWN_CASE_COUNT,
        "the mandatory GPU-known corpus cardinality changed",
    )
    require(equality.oracle_sign == "zero", "coplanarity oracle is not exact zero")
    require(underflow.oracle_sign == "positive", "underflow oracle sign changed")
    require(overflow.oracle_sign == "positive", "overflow oracle sign changed")
    return cases


def run_process(
    executable: Path,
    arguments: Sequence[str],
    input_text: str,
    timeout_seconds: int,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(executable), *arguments],
        input=input_text,
        capture_output=True,
        encoding="utf-8",
        check=False,
        timeout=timeout_seconds,
    )


def require_success(
    completed: subprocess.CompletedProcess[str], context: str
) -> None:
    if completed.returncode != 0 or completed.stderr:
        fail(
            f"{context} failed: returncode={completed.returncode}, "
            f"stdout={completed.stdout!r}, stderr={completed.stderr!r}"
        )


def validate_decision(
    decision: dict[str, Any], case: Case, index: int
) -> None:
    context = f"GPU decision {index} ({case.label})"
    require_exact_keys(decision, DECISION_KEYS, context)
    require(decision.get("kind") == "decision", f"{context}.kind is invalid")
    require(
        decision.get("predicate") == PREDICATE,
        f"{context}.predicate is invalid",
    )
    require(
        require_exact_integer(decision.get("replay_id"), f"{context}.replay_id")
        == case.replay_id,
        f"{context} changed batch order or replay identifier",
    )
    require(
        decision.get("replay_command") == case.replay_command,
        f"{context} did not preserve the exact canonical CPU replay command",
    )
    sign = decision.get("sign")
    require(sign in SIGN_NAMES, f"{context}.sign is invalid")
    require(sign == case.oracle_sign, f"{context} contradicts the Fraction oracle")
    gpu_sign = decision.get("gpu_filter_sign")
    require(gpu_sign in GPU_SIGN_NAMES, f"{context}.gpu_filter_sign is invalid")
    stage = decision.get("certification_stage")
    require(stage in STAGE_NAMES, f"{context}.certification_stage is invalid")
    if case.required_gpu_sign is not None:
        require(
            gpu_sign == case.required_gpu_sign,
            f"{context} expected GPU tri-state {case.required_gpu_sign}, "
            f"got {gpu_sign}",
        )
    if case.required_stage is not None:
        require(
            stage == case.required_stage,
            f"{context} expected stage {case.required_stage}, got {stage}",
        )
    if gpu_sign == "unknown":
        require(
            case.allow_gpu_unknown,
            f"{context} unexpectedly escaped the bounded GPU-known corpus",
        )
        if case.unknown_stage is not None:
            require(
                stage == case.unknown_stage,
                f"{context} expected fallback stage {case.unknown_stage}, got {stage}",
            )
    else:
        require(gpu_sign == sign, f"{context} GPU-known sign changed during resolution")
        require(stage == "fp64_filtered", f"{context} GPU-known stage is not FP64")


def expected_gpu_counters(
    decisions: Sequence[dict[str, Any]], *, audit_known: bool
) -> dict[str, int]:
    known = sum(decision["gpu_filter_sign"] != "unknown" for decision in decisions)
    unknown = len(decisions) - known
    return {
        "async_fallback_batches": 1 if unknown else 0,
        "cpu_expansion_certified": sum(
            decision["gpu_filter_sign"] == "unknown"
            and decision["certification_stage"] == "expansion"
            for decision in decisions
        ),
        "cpu_fp64_filtered_certified": sum(
            decision["gpu_filter_sign"] == "unknown"
            and decision["certification_stage"] == "fp64_filtered"
            for decision in decisions
        ),
        "cpu_multiprecision_certified": sum(
            decision["gpu_filter_sign"] == "unknown"
            and decision["certification_stage"] == "cpu_multiprecision"
            for decision in decisions
        ),
        "exact_zeros": sum(
            decision["gpu_filter_sign"] == "unknown"
            and decision["sign"] == "zero"
            for decision in decisions
        ),
        "gpu_fp64_certified": known,
        "gpu_inputs": len(decisions),
        "gpu_known_audited": known if audit_known else 0,
        "gpu_unknown_forwarded": unknown,
        "remaining_unknown": 0,
    }


def validate_summary(
    summary: dict[str, Any],
    expected_counters: dict[str, int],
    *,
    audit_known: bool,
    context: str,
) -> None:
    require_exact_keys(summary, SUMMARY_KEYS, context)
    require(summary.get("kind") == "summary", f"{context}.kind is invalid")
    require(summary.get("schema") == SCHEMA, f"{context}.schema is invalid")
    require(
        type(summary.get("audit_gpu_signs")) is bool
        and summary["audit_gpu_signs"] is audit_known,
        f"{context}.audit_gpu_signs is invalid",
    )
    counters = summary.get("counters")
    require(isinstance(counters, dict), f"{context}.counters must be an object")
    require_exact_keys(counters, GPU_COUNTER_KEYS, f"{context}.counters")
    for key, expected in expected_counters.items():
        actual = require_exact_integer(counters.get(key), f"{context}.counters.{key}")
        require(
            actual == expected,
            f"{context}.counters.{key}: expected {expected}, got {actual}",
        )
    require(counters["remaining_unknown"] == 0, f"{context} retained an unknown")
    require(
        counters["gpu_fp64_certified"] + counters["gpu_unknown_forwarded"]
        == counters["gpu_inputs"],
        f"{context} counters do not partition GPU inputs",
    )
    require(
        counters["cpu_fp64_filtered_certified"]
        + counters["cpu_expansion_certified"]
        + counters["cpu_multiprecision_certified"]
        == counters["gpu_unknown_forwarded"],
        f"{context} CPU counters do not resolve every GPU unknown",
    )


def validate_cpu_replay(
    cpu_replay: Path,
    decisions: Sequence[dict[str, Any]],
    cases: Sequence[Case],
    timeout_seconds: int,
) -> None:
    replay_input = "".join(f"{decision['replay_command']}\n" for decision in decisions)
    completed = run_process(
        cpu_replay,
        ["--multiprecision-only", "--decision-only", "--batch"],
        replay_input,
        timeout_seconds,
    )
    require_success(completed, "CPU replay of GPU-emitted orientation commands")
    records = parse_jsonl(completed.stdout, "CPU orientation replay")
    require(
        len(records) == len(cases),
        "CPU replay output cardinality differs from the GPU decision batch",
    )
    for index, (record, gpu_decision, case) in enumerate(
        zip(records, decisions, cases, strict=True)
    ):
        context = f"CPU replay decision {index} ({case.label})"
        require_exact_keys(record, CPU_DECISION_KEYS, context)
        require(record.get("predicate") == PREDICATE, f"{context}.predicate is invalid")
        require(
            record.get("certification_stage") == "cpu_multiprecision",
            f"{context} did not exercise the multiprecision oracle",
        )
        require(record.get("sign") in SIGN_NAMES, f"{context}.sign is invalid")
        require(
            record["sign"] == case.oracle_sign == gpu_decision["sign"],
            f"{context} disagrees with the GPU result or Fraction oracle",
        )
        counters = record.get("counters")
        require(isinstance(counters, dict), f"{context}.counters must be an object")
        require_exact_keys(counters, CPU_COUNTER_KEYS, f"{context}.counters")
        expected_counters = {
            "cpu_multiprecision_certified": 1,
            "exact_zeros": 1 if case.oracle_sign == "zero" else 0,
            "expansion_certified": 0,
            "fp32_proposals": 0,
            "fp64_filtered_certified": 0,
            "remaining_unknown": 0,
        }
        for key, expected in expected_counters.items():
            actual = require_exact_integer(
                counters.get(key), f"{context}.counters.{key}"
            )
            require(
                actual == expected,
                f"{context}.counters.{key}: expected {expected}, got {actual}",
            )


def expect_gpu_rejection(
    gpu_runner: Path,
    input_text: str,
    diagnostic: str,
    timeout_seconds: int,
) -> None:
    completed = run_process(gpu_runner, [], input_text, timeout_seconds)
    require(
        completed.returncode == 1,
        f"invalid GPU input {diagnostic!r} returned {completed.returncode}",
    )
    require(
        not completed.stdout,
        f"invalid GPU input {diagnostic!r} published a partial transaction",
    )
    require(
        completed.stderr.startswith("Phase 2B GPU predicate replay failed:")
        and diagnostic in completed.stderr,
        f"invalid GPU input lacks diagnostic {diagnostic!r}: {completed.stderr!r}",
    )


def run_qualification(
    gpu_runner: Path, cpu_replay: Path, timeout_seconds: int
) -> dict[str, int]:
    cases = make_corpus()
    gpu_input = "".join(f"{case.gpu_input}\n" for case in cases)
    audited = run_process(gpu_runner, ["--audit-known"], gpu_input, timeout_seconds)
    require_success(audited, "audited Phase 2B orientation GPU replay")
    audited_records = parse_jsonl(audited.stdout, "audited orientation GPU replay")
    require(
        len(audited_records) == len(cases) + 1,
        "audited GPU replay must emit one decision per input and one summary",
    )
    decisions = audited_records[:-1]
    summary = audited_records[-1]
    for index, (decision, case) in enumerate(zip(decisions, cases, strict=True)):
        validate_decision(decision, case, index)
    audited_counters = expected_gpu_counters(decisions, audit_known=True)
    validate_summary(
        summary,
        audited_counters,
        audit_known=True,
        context="audited orientation GPU summary",
    )
    require(
        audited_counters["gpu_fp64_certified"] >= REQUIRED_GPU_KNOWN_CASE_COUNT,
        "the mandatory GPU-known orientation corpus was not wholly certified",
    )
    require(
        3 <= audited_counters["gpu_unknown_forwarded"] <= 3 + OPTIONAL_GPU_CASE_COUNT,
        "mandatory and optional orientation fallbacks have invalid cardinality",
    )
    require(
        audited_counters["exact_zeros"] == 1,
        "exact coplanarity must be the sole zero in the corpus",
    )

    validate_cpu_replay(cpu_replay, decisions, cases, timeout_seconds)

    summary_only = run_process(
        gpu_runner, ["--summary-only"], gpu_input, timeout_seconds
    )
    require_success(summary_only, "summary-only Phase 2B orientation GPU replay")
    summary_records = parse_jsonl(
        summary_only.stdout, "summary-only orientation replay"
    )
    require(
        len(summary_records) == 1,
        "--summary-only must publish exactly one JSONL record",
    )
    unaudited_counters = dict(audited_counters)
    unaudited_counters["gpu_known_audited"] = 0
    validate_summary(
        summary_records[0],
        unaudited_counters,
        audit_known=False,
        context="summary-only orientation GPU summary",
    )

    valid_words = next(
        case.words for case in cases if case.required_gpu_sign == "positive"
    )
    nan_words = ("7ff8000000000000", *valid_words[1:])
    expect_gpu_rejection(
        gpu_runner,
        f"91 {PREDICATE} {' '.join(nan_words)}\n",
        "finite",
        timeout_seconds,
    )
    duplicate_line = f"93 {PREDICATE} {' '.join(valid_words)}\n"
    expect_gpu_rejection(
        gpu_runner,
        duplicate_line + duplicate_line,
        "unique",
        timeout_seconds,
    )
    expect_gpu_rejection(
        gpu_runner,
        f"95 {PREDICATE} {' '.join(valid_words[:-1])}\n",
        "12 binary64 words",
        timeout_seconds,
    )
    expect_gpu_rejection(
        gpu_runner,
        f"097 {PREDICATE} {' '.join(valid_words)}\n",
        "canonical decimal",
        timeout_seconds,
    )
    expect_gpu_rejection(
        gpu_runner,
        duplicate_line
        + f"99 compare_squared_distances {' '.join(valid_words[:9])}\n",
        "single predicate",
        timeout_seconds,
    )
    expect_gpu_rejection(gpu_runner, "\n", "empty", timeout_seconds)
    return audited_counters


def positive_integer(value: str) -> int:
    try:
        parsed = int(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("must be an integer") from error
    if parsed <= 0:
        raise argparse.ArgumentTypeError("must be positive")
    return parsed


def main(arguments: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("gpu_runner", type=Path)
    parser.add_argument("cpu_replay", type=Path)
    parser.add_argument("--timeout-seconds", type=positive_integer, default=120)
    options = parser.parse_args(arguments)
    for executable, label in (
        (options.gpu_runner, "GPU runner"),
        (options.cpu_replay, "CPU replay"),
    ):
        if not executable.is_file():
            parser.error(f"{label} does not exist: {executable}")
    try:
        counters = run_qualification(
            options.gpu_runner.resolve(),
            options.cpu_replay.resolve(),
            options.timeout_seconds,
        )
    except (AssertionError, OSError, subprocess.TimeoutExpired) as error:
        print(
            f"Phase 2B orientation-filter qualification failed: {error}",
            file=sys.stderr,
        )
        return 1
    print(
        json.dumps(
            {
                "cases": counters["gpu_inputs"],
                "gpu_fp64_certified": counters["gpu_fp64_certified"],
                "gpu_known_audited": counters["gpu_known_audited"],
                "gpu_unknown_forwarded": counters["gpu_unknown_forwarded"],
                "remaining_unknown": counters["remaining_unknown"],
                "schema": SCHEMA,
            },
            separators=(",", ":"),
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
