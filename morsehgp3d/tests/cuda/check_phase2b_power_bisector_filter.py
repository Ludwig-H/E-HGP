#!/usr/bin/env python3
"""Differentially qualify the bounded Phase 2B power-bisector GPU filter."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from fractions import Fraction
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import sys
from typing import Any, NoReturn, Sequence


SCHEMA = "morsehgp3d.phase2b.power_bisector_side_filter.v1"
PREDICATE = "power_bisector_side"
INTEGER_KNOWN_CASE_COUNT = 2_049
ADVERSARIAL_REQUIRED_KNOWN_CASE_COUNT = 18
ADVERSARIAL_OPTIONAL_CASE_COUNT = 6
REQUIRED_GPU_KNOWN_CASE_COUNT = (
    INTEGER_KNOWN_CASE_COUNT + ADVERSARIAL_REQUIRED_KNOWN_CASE_COUNT
)
REQUIRED_GPU_UNKNOWN_CASE_COUNT = 6
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
WITNESS_DENOMINATORS = (1, 2, 3, 5, 7, 8)

PointWords = tuple[str, str, str]
WitnessRecord = tuple[int, int, int, int]


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


def binary64_word(value: int | float) -> str:
    converted = float(value)
    require(math.isfinite(converted), "a test coordinate must be finite")
    return struct.pack(">d", converted).hex()


def scaled_binary64_word(coefficient: int, exponent: int) -> str:
    converted = math.ldexp(float(coefficient), exponent)
    require(math.isfinite(converted), "a scaled test coordinate must be finite")
    return struct.pack(">d", converted).hex()


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


def exact_point(words: PointWords) -> tuple[Fraction, Fraction, Fraction]:
    return tuple(binary64_fraction(word) for word in words)  # type: ignore[return-value]


def sign_name(value: Fraction) -> str:
    if value < 0:
        return "negative"
    return "zero" if value == 0 else "positive"


def witness_is_canonical(witness: WitnessRecord) -> bool:
    numerator_gcd = math.gcd(
        abs(witness[0]), math.gcd(abs(witness[1]), abs(witness[2]))
    )
    return witness[3] > 0 and math.gcd(numerator_gcd, witness[3]) == 1


@dataclass(frozen=True)
class Case:
    replay_id: int
    witness: WitnessRecord
    points: tuple[PointWords, ...]
    r_ids: tuple[int, ...]
    q_ids: tuple[int, ...]
    label: str
    required_gpu_sign: str | None = None
    required_stage: str | None = None
    allow_gpu_unknown: bool = False
    unknown_stage: str = "cpu_multiprecision"

    @property
    def replay_command(self) -> str:
        tokens = [
            PREDICATE,
            *(str(value) for value in self.witness),
            str(len(self.points)),
            *(word for point in self.points for word in point),
            str(len(self.r_ids)),
            *(str(identifier) for identifier in self.r_ids),
            str(len(self.q_ids)),
            *(str(identifier) for identifier in self.q_ids),
        ]
        return " ".join(tokens)

    @property
    def gpu_input(self) -> str:
        return f"{self.replay_id} {self.replay_command}"

    @property
    def oracle_value(self) -> Fraction:
        # This is deliberately the un-factorized geometric definition. It is
        # independent of both the paired CUDA polynomial and the CPU predicate.
        witness = tuple(
            Fraction(numerator, self.witness[3])
            for numerator in self.witness[:3]
        )
        points = tuple(exact_point(point) for point in self.points)

        def cost(identifiers: Sequence[int]) -> Fraction:
            return sum(
                (
                    sum(
                        (
                            (witness[axis] - points[identifier][axis]) ** 2
                            for axis in range(3)
                        ),
                        start=Fraction(0),
                    )
                    for identifier in identifiers
                ),
                start=Fraction(0),
            )

        return cost(self.r_ids) - cost(self.q_ids)

    @property
    def oracle_sign(self) -> str:
        return sign_name(self.oracle_value)

    @property
    def homogeneous_moment_value(self) -> Fraction:
        points = tuple(exact_point(point) for point in self.points)
        delta_coordinates = tuple(
            sum((points[index][axis] for index in self.r_ids), start=Fraction(0))
            - sum((points[index][axis] for index in self.q_ids), start=Fraction(0))
            for axis in range(3)
        )
        delta_norm = sum(
            (
                sum((coordinate * coordinate for coordinate in points[index]), start=Fraction(0))
                for index in self.r_ids
            ),
            start=Fraction(0),
        ) - sum(
            (
                sum((coordinate * coordinate for coordinate in points[index]), start=Fraction(0))
                for index in self.q_ids
            ),
            start=Fraction(0),
        )
        return self.witness[3] * delta_norm - 2 * sum(
            (
                self.witness[axis] * delta_coordinates[axis]
                for axis in range(3)
            ),
            start=Fraction(0),
        )


def make_point(values: Sequence[int]) -> PointWords:
    require(len(values) == 3, "a test point must have three coordinates")
    return (
        binary64_word(values[0]),
        binary64_word(values[1]),
        binary64_word(values[2]),
    )


def mirror_case(case: Case, replay_id: int, label: str) -> Case:
    mirrored = Case(
        replay_id,
        case.witness,
        case.points,
        case.q_ids,
        case.r_ids,
        label,
        (
            None
            if case.required_gpu_sign is None
            else "negative"
            if case.required_gpu_sign == "positive"
            else "positive"
        ),
        case.required_stage,
        case.allow_gpu_unknown,
        case.unknown_stage,
    )
    require(
        mirrored.oracle_value == -case.oracle_value,
        f"{case.label} did not reverse exactly under R/Q exchange",
    )
    return mirrored


def make_known_cases() -> list[Case]:
    generator = random.Random(0x504F574552484750)
    cases: list[Case] = []
    for index in range(INTEGER_KNOWN_CASE_COUNT):
        cardinality = 1 + index % 10
        overlap = (index // 10) % cardinality
        r_ids = tuple(range(cardinality))
        q_ids = tuple(range(overlap)) + tuple(
            range(cardinality, 2 * cardinality - overlap)
        )
        denominator = WITNESS_DENOMINATORS[index % len(WITNESS_DENOMINATORS)]
        for attempt in range(1_024):
            points = tuple(
                make_point(
                    tuple(generator.randrange(-32, 33) for _ in range(3))
                )
                for _ in range(2 * cardinality - overlap)
            )
            numerators = [generator.randrange(-16, 17) for _ in range(3)]
            while math.gcd(
                denominator,
                math.gcd(
                    abs(numerators[0]),
                    math.gcd(abs(numerators[1]), abs(numerators[2])),
                ),
            ) != 1:
                numerators[0] += 1
            witness = (*numerators, denominator)
            candidate = Case(
                90_000_019 + (INTEGER_KNOWN_CASE_COUNT - index) * 31,
                witness,
                points,
                r_ids,
                q_ids,
                f"integer-known-{index}",
            )
            if candidate.oracle_value != 0:
                break
        else:
            fail(f"integer case {index} exhausted its deterministic retries")
        desired = "positive" if index % 2 == 0 else "negative"
        if candidate.oracle_sign != desired:
            candidate = Case(
                candidate.replay_id,
                candidate.witness,
                candidate.points,
                candidate.q_ids,
                candidate.r_ids,
                candidate.label,
            )
        candidate = Case(
            candidate.replay_id,
            candidate.witness,
            candidate.points,
            candidate.r_ids,
            candidate.q_ids,
            candidate.label,
            desired,
            "fp64_filtered",
        )
        require(
            candidate.oracle_sign == desired,
            f"generated case {index} does not have its constructed exact sign",
        )
        cases.append(candidate)
    return cases


def singleton_case(
    replay_id: int,
    label: str,
    witness: WitnessRecord,
    r: PointWords,
    q: PointWords,
    *,
    required_known: bool,
) -> tuple[Case, Case]:
    original = Case(
        replay_id,
        witness,
        (r, q),
        (0,),
        (1,),
        label,
        None,
        None,
        not required_known,
    )
    require(original.oracle_value != 0, f"{label} unexpectedly became exact zero")
    original = Case(
        original.replay_id,
        original.witness,
        original.points,
        original.r_ids,
        original.q_ids,
        original.label,
        original.oracle_sign if required_known else None,
        "fp64_filtered" if required_known else None,
        not required_known,
    )
    return original, mirror_case(original, replay_id + 37, f"{label}-mirrored")


def make_adversarial_cases() -> list[Case]:
    zero = "0000000000000000"
    negative_zero = "8000000000000000"
    origin = (zero, zero, zero)
    required: list[Case] = []
    optional: list[Case] = []

    required.extend(
        singleton_case(
            1_001,
            "positive-product-quadrants",
            (3, 0, 0, 1),
            origin,
            make_point((2, 0, 0)),
            required_known=True,
        )
    )
    required.extend(
        singleton_case(
            1_101,
            "negative-product-quadrants",
            (0, 0, 0, 1),
            origin,
            make_point((2, 0, 0)),
            required_known=True,
        )
    )
    required.extend(
        singleton_case(
            1_201,
            "mixed-axis-terms",
            (3, -2, 1, 7),
            make_point((2, 3, 5)),
            make_point((-1, 4, 0)),
            required_known=True,
        )
    )

    base = 1 << 40
    required.extend(
        singleton_case(
            1_301,
            "large-offset-paired",
            (3 * base + 1, 3 * base, 3 * base, 3),
            make_point((base + 2, base, base)),
            make_point((base + 1, base, base)),
            required_known=True,
        )
    )
    required.extend(
        singleton_case(
            1_401,
            "signed-zero-branches",
            (1, -1, 2, 1),
            (negative_zero, zero, negative_zero),
            make_point((2, -4, 6)),
            required_known=True,
        )
    )
    required.extend(
        singleton_case(
            1_501,
            "extreme-positive-exponent",
            (0, 0, 0, 1),
            (scaled_binary64_word(1, 500), zero, zero),
            (scaled_binary64_word(1, 499), zero, zero),
            required_known=True,
        )
    )
    required.extend(
        singleton_case(
            1_601,
            "extreme-negative-exponent",
            (0, 0, 0, 1),
            (scaled_binary64_word(1, -500), zero, zero),
            (scaled_binary64_word(1, -499), zero, zero),
            required_known=True,
        )
    )

    shared = tuple(make_point((index - 4, index % 3 - 1, 2 - index % 5)) for index in range(9))
    maximum = Case(
        1_701,
        (3, -2, 1, 7),
        (*shared, make_point((2, 3, 5)), make_point((-1, 4, 0))),
        (*range(9), 9),
        (*range(9), 10),
        "maximum-cardinality-overlap",
        "positive",
        "fp64_filtered",
    )
    require(maximum.oracle_sign == "positive", "maximum-cardinality sign changed")
    required.extend(
        (maximum, mirror_case(maximum, 1_738, "maximum-cardinality-overlap-mirrored"))
    )

    magnitude = scaled_binary64_word(1, 600)
    negative_magnitude = scaled_binary64_word(-1, 600)
    axis_pairing = Case(
        1_801,
        (0, 3, 0, 1),
        (
            (zero, magnitude, zero),
            ("4000000000000000", negative_magnitude, zero),
            ("3ff0000000000000", negative_magnitude, zero),
            ("4008000000000000", magnitude, zero),
        ),
        (0, 1),
        (2, 3),
        "axis-wise-pairing-avoids-lexicographic-overflow",
        "negative",
        "fp64_filtered",
    )
    require(axis_pairing.oracle_value == -6, "axis-wise pairing fixture changed")
    required.extend(
        (
            axis_pairing,
            mirror_case(
                axis_pairing,
                1_838,
                "axis-wise-pairing-avoids-lexicographic-overflow-mirrored",
            ),
        )
    )

    optional.extend(
        singleton_case(
            2_001,
            "all-inexact",
            (2, -3, 5, 7),
            (
                "3fb999999999999a",
                "3fc999999999999a",
                "3fd3333333333333",
            ),
            ("4010000000000000", "400e000000000000", "400c000000000000"),
            required_known=False,
        )
    )
    optional.extend(
        singleton_case(
            2_101,
            "signed-inexact",
            (-5, 4, -2, 7),
            (
                "bfb999999999999a",
                "bfc999999999999a",
                "3fd3333333333333",
            ),
            ("401419999999999a", "c013333333333333", "40114ccccccccccd"),
            required_known=False,
        )
    )
    optional.extend(
        singleton_case(
            2_201,
            "one-ulp-cancellation-gap",
            (0, 0, 0, 1),
            ("3ff0000000000000", zero, zero),
            ("3ff0000000000001", zero, zero),
            required_known=False,
        )
    )

    require(
        len(required) == ADVERSARIAL_REQUIRED_KNOWN_CASE_COUNT,
        "the required adversarial power-bisector count changed",
    )
    require(
        len(optional) == ADVERSARIAL_OPTIONAL_CASE_COUNT,
        "the optional adversarial power-bisector count changed",
    )
    return required + optional


def make_required_unknown_cases() -> list[Case]:
    zero = "0000000000000000"
    maximum = "7fefffffffffffff"
    minimum_subnormal = "0000000000000001"
    origin = (zero, zero, zero)
    one = make_point((1, 0, 0))
    two = make_point((2, 0, 0))

    cases = [
        Case(
            7,
            (3, 0, 0, 2),
            (one, two),
            (1,),
            (0,),
            "binary64-rational-midpoint",
            "unknown",
            "cpu_multiprecision",
            True,
        ),
        Case(
            11,
            (1, -2, 8, 4),
            (
                make_point((-1, 0, 0)),
                make_point((1, 0, 0)),
                make_point((0, -1, 0)),
                make_point((0, 1, 0)),
            ),
            (0, 1),
            (2, 3),
            "distinct-equal-moments",
            "unknown",
            "cpu_multiprecision",
            True,
        ),
        Case(
            13,
            (3, 1, 0, 3),
            (origin, two),
            (0,),
            (1,),
            "non-dyadic-rational-zero",
            "unknown",
            "cpu_multiprecision",
            True,
        ),
        Case(
            17,
            (3, -2, 1, 1),
            tuple(make_point((index, index % 3, -index)) for index in range(10)),
            tuple(range(10)),
            tuple(range(10)),
            "maximum-cardinality-identical-labels",
            "unknown",
            "cpu_multiprecision",
            True,
        ),
        Case(
            19,
            (0, 0, 0, 1),
            ((minimum_subnormal, zero, zero), origin),
            (0,),
            (1,),
            "underflow-product",
            "unknown",
            "cpu_multiprecision",
            True,
        ),
        Case(
            23,
            (0, 0, 0, 1),
            ((maximum, zero, zero), origin),
            (0,),
            (1,),
            "overflow-product",
            "unknown",
            "cpu_multiprecision",
            True,
        ),
    ]
    require(
        len(cases) == REQUIRED_GPU_UNKNOWN_CASE_COUNT,
        "the required GPU-unknown corpus count changed",
    )
    require(
        sum(case.oracle_sign == "zero" for case in cases) == 4,
        "the required unknown corpus must contain exactly four zeros",
    )
    require(
        all(case.oracle_sign != "negative" for case in cases),
        "the required unknown corpus unexpectedly contains a negative case",
    )
    return cases


def make_corpus() -> list[Case]:
    cases = make_adversarial_cases() + make_known_cases()
    unknowns = make_required_unknown_cases()
    for insertion, case in zip((0, 257, 601, 1_026, 1_507, 1_903), unknowns, strict=True):
        cases.insert(insertion, case)
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
    require(
        sum(case.required_gpu_sign == "unknown" for case in cases)
        == REQUIRED_GPU_UNKNOWN_CASE_COUNT,
        "the mandatory GPU-unknown corpus cardinality changed",
    )
    for case in cases:
        require(
            witness_is_canonical(case.witness),
            f"case {case.label} has a non-canonical rational witness",
        )
        require(
            len(case.r_ids) == len(case.q_ids) and 1 <= len(case.r_ids) <= 10,
            f"case {case.label} has an invalid label cardinality",
        )
        require(
            tuple(sorted(set(case.r_ids))) == case.r_ids
            and tuple(sorted(set(case.q_ids))) == case.q_ids,
            f"case {case.label} has non-canonical label identifiers",
        )
        require(
            case.homogeneous_moment_value
            == case.witness[3] * case.oracle_value,
            f"case {case.label} contradicts the independent affine identity",
        )
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


def validate_decision(decision: dict[str, Any], case: Case, index: int) -> None:
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
            f"{context} expected GPU tri-state {case.required_gpu_sign}, got {gpu_sign}",
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
    require_success(completed, "CPU replay of GPU-emitted power-bisector commands")
    records = parse_jsonl(completed.stdout, "CPU power-bisector replay")
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
            actual = require_exact_integer(counters.get(key), f"{context}.counters.{key}")
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


def raw_power_input(
    replay_id: str,
    witness: Sequence[str],
    points: Sequence[PointWords],
    r_ids: Sequence[int],
    q_ids: Sequence[int],
) -> str:
    return " ".join(
        [
            replay_id,
            PREDICATE,
            *witness,
            str(len(points)),
            *(word for point in points for word in point),
            str(len(r_ids)),
            *(str(identifier) for identifier in r_ids),
            str(len(q_ids)),
            *(str(identifier) for identifier in q_ids),
        ]
    ) + "\n"


def validate_invalid_inputs(
    gpu_runner: Path, cases: Sequence[Case], timeout_seconds: int
) -> None:
    known = next(case for case in cases if case.required_gpu_sign == "positive")
    duplicate_line = f"93 {known.replay_command}\n"
    expect_gpu_rejection(
        gpu_runner,
        duplicate_line + duplicate_line,
        "unique",
        timeout_seconds,
    )
    expect_gpu_rejection(
        gpu_runner,
        f"097 {known.replay_command}\n",
        "canonical decimal",
        timeout_seconds,
    )
    expect_gpu_rejection(gpu_runner, "\n", "empty", timeout_seconds)

    zero = "0000000000000000"
    origin = (zero, zero, zero)
    one = make_point((1, 0, 0))
    nan = ("7ff8000000000000", zero, zero)
    expect_gpu_rejection(
        gpu_runner,
        raw_power_input("101", ("0", "0", "0", "1"), (nan, one), (0,), (1,)),
        "finite",
        timeout_seconds,
    )
    expect_gpu_rejection(
        gpu_runner,
        raw_power_input("103", ("0", "0", "0", "0"), (origin, one), (0,), (1,)),
        "strictly positive",
        timeout_seconds,
    )
    expect_gpu_rejection(
        gpu_runner,
        raw_power_input("107", ("+1", "0", "0", "1"), (origin, one), (0,), (1,)),
        "cannot start with '+'",
        timeout_seconds,
    )
    expect_gpu_rejection(
        gpu_runner,
        raw_power_input(
            "109",
            ("9007199254740993", "0", "0", "1"),
            (origin, one),
            (0,),
            (1,),
        ),
        "exactly representable",
        timeout_seconds,
    )
    # The GPU replay must be a valid canonical CPU replay. Accepting this
    # unreduced tuple would publish a replay_command rejected by ExactRational3.
    expect_gpu_rejection(
        gpu_runner,
        raw_power_input("113", ("2", "0", "0", "2"), (origin, one), (0,), (1,)),
        "reduced canonically",
        timeout_seconds,
    )
    expect_gpu_rejection(
        gpu_runner,
        raw_power_input("127", ("0", "0", "0", "1"), (origin, one), (1, 0), (0, 1)),
        "sorted and unique",
        timeout_seconds,
    )
    expect_gpu_rejection(
        gpu_runner,
        raw_power_input("131", ("0", "0", "0", "1"), (origin, one), (0, 0), (0, 1)),
        "sorted and unique",
        timeout_seconds,
    )
    expect_gpu_rejection(
        gpu_runner,
        raw_power_input("137", ("0", "0", "0", "1"), (origin,), (1,), (0,)),
        "outside the point table",
        timeout_seconds,
    )
    expect_gpu_rejection(
        gpu_runner,
        raw_power_input("139", ("0", "0", "0", "1"), (origin, one), (), ()),
        "between one and ten",
        timeout_seconds,
    )
    expect_gpu_rejection(
        gpu_runner,
        raw_power_input("149", ("0", "0", "0", "1"), (origin, one), (0,), (0, 1)),
        "equal declared and serialized sizes",
        timeout_seconds,
    )
    eleven_points = tuple(make_point((index, 0, 0)) for index in range(11))
    eleven_ids = tuple(range(11))
    expect_gpu_rejection(
        gpu_runner,
        raw_power_input(
            "151",
            ("0", "0", "0", "1"),
            eleven_points,
            eleven_ids,
            eleven_ids,
        ),
        "between one and ten",
        timeout_seconds,
    )
    expect_gpu_rejection(
        gpu_runner,
        "157 power_bisector_side 0 0 0 1 21 0 0 0 0\n",
        "between one and twenty",
        timeout_seconds,
    )
    mixed_distance = f"163 compare_squared_distances {' '.join((zero,) * 9)}\n"
    expect_gpu_rejection(
        gpu_runner,
        duplicate_line + mixed_distance,
        "single predicate",
        timeout_seconds,
    )


def run_qualification(
    gpu_runner: Path, cpu_replay: Path, timeout_seconds: int
) -> dict[str, int]:
    cases = make_corpus()
    gpu_input = "".join(f"{case.gpu_input}\n" for case in cases)
    audited = run_process(gpu_runner, ["--audit-known"], gpu_input, timeout_seconds)
    require_success(audited, "audited Phase 2B power-bisector GPU replay")
    audited_records = parse_jsonl(audited.stdout, "audited power-bisector GPU replay")
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
        context="audited power-bisector GPU summary",
    )
    require(
        audited_counters["gpu_fp64_certified"] >= REQUIRED_GPU_KNOWN_CASE_COUNT,
        "the mandatory GPU-known power-bisector corpus was not wholly certified",
    )
    require(
        REQUIRED_GPU_UNKNOWN_CASE_COUNT
        <= audited_counters["gpu_unknown_forwarded"]
        <= REQUIRED_GPU_UNKNOWN_CASE_COUNT + ADVERSARIAL_OPTIONAL_CASE_COUNT,
        "mandatory and optional power-bisector fallbacks have invalid cardinality",
    )
    require(
        audited_counters["exact_zeros"] == 4,
        "the power-bisector corpus must contain exactly four exact zeros",
    )
    require(
        audited_counters["cpu_fp64_filtered_certified"] == 0
        and audited_counters["cpu_expansion_certified"] == 0,
        "the exact-rational power fallback must remain multiprecision-only",
    )

    validate_cpu_replay(cpu_replay, decisions, cases, timeout_seconds)

    summary_only = run_process(
        gpu_runner, ["--summary-only"], gpu_input, timeout_seconds
    )
    require_success(summary_only, "summary-only Phase 2B power-bisector GPU replay")
    summary_records = parse_jsonl(
        summary_only.stdout, "summary-only power-bisector replay"
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
        context="summary-only power-bisector GPU summary",
    )

    validate_invalid_inputs(gpu_runner, cases, timeout_seconds)
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
            f"Phase 2B power-bisector qualification failed: {error}",
            file=sys.stderr,
        )
        return 1
    print(
        json.dumps(
            {
                "cases": counters["gpu_inputs"],
                "exact_zeros": counters["exact_zeros"],
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
