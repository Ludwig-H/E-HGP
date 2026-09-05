#!/usr/bin/env python3
"""Small arithmetic certificates only; this does not execute C++ product code."""

from __future__ import annotations

from fractions import Fraction
import itertools
import json
import math
from pathlib import Path


def check(condition: bool, reason: str) -> None:
    if not condition:
        raise RuntimeError(reason)


def dot(a: tuple[int, ...], b: tuple[int, ...]) -> int:
    return sum(x * y for x, y in zip(a, b))


def cross(a: tuple[int, ...], b: tuple[int, ...]) -> tuple[int, ...]:
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def main() -> None:
    maximum = 65535
    vectors = list(itertools.product(range(-3, 4), repeat=3))
    vectors += list(itertools.product((-maximum, 0, maximum), repeat=3))
    tested = 0
    worst = Fraction(1)
    for d in vectors:
        length = dot(d, d)
        if not length:
            continue
        candidates = [(0, d[2], -d[1]), (-d[2], 0, d[0]), (d[1], -d[0], 0)]
        # Independent selection by the two smallest squared axis coordinates.
        axes = sorted(range(3), key=lambda axis: (d[axis] ** 2, axis))
        u, v = (candidates[axis] for axis in axes[:2])
        remaining = axes[2]
        cross2 = dot(cross(u, v), cross(u, v))
        longest = max(sum((x + sign * y) ** 2 for x, y in zip(u, v))
                      for sign in (-1, 1))
        check(cross2 == d[remaining] ** 2 * length, "cross.identity")
        check(3 * cross2 >= length ** 2, "cross.lower_bound")
        check(longest <= 2 * length, "edge.upper_bound")
        check(all(den * cross2 >= length * longest for den in (6, 8, 12)),
              "first_iteration.containment")
        worst = min(worst, Fraction(cross2, length * longest))
        tested += 1
    check(tested == 368 and worst == Fraction(1, 6), "basis.nonvacuity")
    check(3 * 1 < 2 * 2, "comment_counterexample.must_remain_false")
    check(cross((0, 0, -1), (0, 0, 1)) == (0, 0, 0), "wrong_axes.counterexample")
    values = {0, 1, 2, (1 << 120) - 1, 81 * maximum ** 6 // 2}
    for exponent in (1, 2, 10, 30, 50, 59):
        for offset in (-1, 0, 1):
            root = (1 << exponent) + offset
            for delta in (-1, 0, 1):
                value = root * root + delta
                if 0 <= value < 1 << 120:
                    values.add(value)
    corrections = 0
    nonzero_corrections = 0
    for value in sorted(values):
        estimate = int(math.sqrt(float(value)))
        count = 0
        while estimate > 0 and estimate * estimate > value:
            estimate -= 1
            count += 1
        while (estimate + 1) ** 2 <= value:
            estimate += 1
            count += 1
        check(estimate == math.isqrt(value), "corrected_sqrt.floor")
        check((estimate + 1) ** 2 > value, "corrected_sqrt.strict_outer")
        corrections += count
        nonzero_corrections += count > 0
    check(nonzero_corrections > 0 and len(values) >= 40, "sqrt.nonvacuity")
    unit = Fraction(1, 1 << 53)
    guard = Fraction(1, 1 << 40)
    check(4 * unit / (1 - 8 * unit) < 5 * unit, "chord.product_roundoff")
    check(guard > 7 * unit, "chord.guard_strict_margin")
    limits = {"basis_cross2": 12 * maximum ** 4,
              "basis_test": 144 * maximum ** 4,
              "sector_4dot": 48 * maximum ** 2,
              "B": 6 * maximum ** 3, "J": 81 * maximum ** 6,
              "mu_hat": 8 * maximum ** 3, "L": 216 * maximum ** 6,
              "chord_v": 408 * maximum ** 6}
    bits = {"basis_cross2": 68, "basis_test": 72, "sector_4dot": 38,
            "B": 51, "J": 103, "mu_hat": 51, "L": 104, "chord_v": 105}
    for label, value in limits.items():
        check(value < 1 << bits[label], "integer_bound." + label)
    result = {"status": "passed", "mode": "small_arithmetic_models_no_product_execution",
              "python_optimized": not __debug__, "basis_vectors": tested,
              "tight_inradius_squared_over_D2": "1/6", "basis_initial_scale": [1, 1],
              "sqrt_cases": len(values), "sqrt_corrections": corrections,
              "sqrt_cases_with_correction": nonzero_corrections,
              "old_comment_counterexample": {"d": [1, 1, 0], "D2": 2,
                                              "second_norm2": 1, "claimed_lower_bound": "4/3"},
              "integer_bounds": limits, "strict_power_of_two_bounds": bits,
              "product_tests_run": 0, "gcp": "not_used"}
    folder = Path(__file__).resolve().parent / "receipts_front_20260905"
    folder.mkdir(exist_ok=True)
    path = folder / "secteur_corde_arithmetic.json"
    path.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
