#!/usr/bin/env python3
"""Run a deterministic small differential corpus against native predicates."""

from __future__ import annotations

import json
import random
import struct
import subprocess
import sys
from fractions import Fraction
from pathlib import Path


SEED = 0x4D4F525345484750
EXPONENTS = (0, 1, 2, 1022, 1023, 1024, 2045, 2046)


def random_word(generator: random.Random) -> str:
    sign = generator.getrandbits(1)
    exponent = generator.choice(EXPONENTS)
    fraction = generator.getrandbits(52)
    bits = (sign << 63) | (exponent << 52) | fraction
    return f"{bits:016x}"


def rational(word: str) -> Fraction:
    return Fraction.from_float(struct.unpack(">d", bytes.fromhex(word))[0])


def point(words: list[str]) -> tuple[Fraction, Fraction, Fraction]:
    return tuple(rational(word) for word in words)  # type: ignore[return-value]


def sign(value: Fraction) -> str:
    if value < 0:
        return "negative"
    return "zero" if value == 0 else "positive"


def invoke(executable: Path, predicate: str, points: list[list[str]]) -> dict:
    completed = subprocess.run(
        [str(executable), predicate, *(word for point_words in points for word in point_words)],
        check=True,
        capture_output=True,
        encoding="utf-8",
        timeout=10,
    )
    return json.loads(completed.stdout)


def distance_difference(points: list[tuple[Fraction, Fraction, Fraction]]) -> Fraction:
    witness, left, right = points
    left_distance = sum(((a - b) ** 2 for a, b in zip(witness, left)), Fraction())
    right_distance = sum(((a - b) ** 2 for a, b in zip(witness, right)), Fraction())
    return left_distance - right_distance


def orientation(points: list[tuple[Fraction, Fraction, Fraction]]) -> Fraction:
    a, b, c, d = points
    u = tuple(bi - ai for ai, bi in zip(a, b))
    v = tuple(ci - ai for ai, ci in zip(a, c))
    w = tuple(di - ai for ai, di in zip(a, d))
    return (
        u[0] * (v[1] * w[2] - v[2] * w[1])
        - u[1] * (v[0] * w[2] - v[2] * w[0])
        + u[2] * (v[0] * w[1] - v[1] * w[0])
    )


def audit(executable: Path, predicate: str, point_count: int, cases: int) -> None:
    generator = random.Random(SEED ^ point_count)
    oracle = distance_difference if predicate == "compare_squared_distances" else orientation
    for case_index in range(cases):
        words = [[random_word(generator) for _ in range(3)] for _ in range(point_count)]
        exact_points = [point(point_words) for point_words in words]
        expected = oracle(exact_points)
        observed = invoke(executable, predicate, words)
        if observed["sign"] != sign(expected):
            raise AssertionError(
                f"{predicate} case {case_index} differs: expected {sign(expected)}, "
                f"observed {observed['sign']}, words={words}"
            )
        counters = observed["counters"]
        expected_counters = {
            "cpu_multiprecision_certified": 1,
            "exact_zeros": 1 if expected == 0 else 0,
            "expansion_certified": 0,
            "fp32_proposals": 0,
            "fp64_filtered_certified": 0,
            "remaining_unknown": 0,
        }
        if observed["certification_stage"] != "cpu_multiprecision" or counters != expected_counters:
            raise AssertionError(f"{predicate} case {case_index} has invalid counters")
        if predicate == "compare_squared_distances":
            witness, left, right = exact_points
            left_level = sum(((a - b) ** 2 for a, b in zip(witness, left)), Fraction())
            right_level = sum(((a - b) ** 2 for a, b in zip(witness, right)), Fraction())
            if observed["left_squared_distance"] != {
                "denominator": str(left_level.denominator),
                "numerator": str(left_level.numerator),
                "schema_version": "2.0.0",
                "unit": "input_coordinate_unit_squared",
            } or observed["right_squared_distance"] != {
                "denominator": str(right_level.denominator),
                "numerator": str(right_level.numerator),
                "schema_version": "2.0.0",
                "unit": "input_coordinate_unit_squared",
            }:
                raise AssertionError(f"{predicate} case {case_index} has invalid exact levels")
        elif observed["determinant_exact"] != {
            "denominator": str(expected.denominator),
            "numerator": str(expected.numerator),
        }:
            raise AssertionError(f"{predicate} case {case_index} has an invalid determinant")


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: check_predicate_random.py NATIVE_REPLAY")
    executable = Path(sys.argv[1])
    audit(executable, "compare_squared_distances", 3, 128)
    audit(executable, "orientation_3d", 4, 64)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
