from __future__ import annotations

import itertools
import json
import unittest
from fractions import Fraction
from pathlib import Path


FIXTURE = (
    Path(__file__).parents[1]
    / "fixtures"
    / "regressions"
    / "pair_aabb_corner_correlation.json"
)


def _point(values: list[object]) -> tuple[Fraction, Fraction, Fraction]:
    return tuple(Fraction(value) for value in values)  # type: ignore[return-value]


def _phi(
    query: tuple[Fraction, Fraction, Fraction],
    first: tuple[Fraction, Fraction, Fraction],
    second: tuple[Fraction, Fraction, Fraction],
) -> Fraction:
    return sum(
        (query[axis] - first[axis]) * (query[axis] - second[axis])
        for axis in range(3)
    )


def _corners(
    bounds: list[list[object]],
) -> tuple[tuple[Fraction, Fraction, Fraction], ...]:
    lower = _point(bounds[0])
    upper = _point(bounds[1])
    return tuple(
        tuple((lower[axis], upper[axis])[selector[axis]] for axis in range(3))
        for selector in itertools.product((0, 1), repeat=3)
    )  # type: ignore[return-value]


class PairAabbCornerCorrelationRegressionTests(unittest.TestCase):
    def test_artificial_box_corners_do_not_exclude_real_universal_witness(self) -> None:
        with FIXTURE.open(encoding="utf-8") as fixture_file:
            fixture = json.load(fixture_file)
        query = _point(fixture["query_point"])
        first_points = tuple(map(_point, fixture["first_support_points"]))
        second_points = tuple(map(_point, fixture["second_support_points"]))
        expected = fixture["expected"]

        real_values = tuple(
            _phi(query, first, second)
            for first in first_points
            for second in second_points
        )
        self.assertEqual(
            real_values,
            tuple(Fraction(value) for value in expected["real_pair_phi_values"]),
        )
        self.assertTrue(all(value < 0 for value in real_values))

        artificial_values = tuple(
            _phi(query, first, second)
            for first in _corners(expected["first_support_aabb"])
            for second in _corners(expected["second_support_aabb"])
        )
        self.assertEqual(
            max(artificial_values),
            Fraction(expected["artificial_corner_phi_maximum"]),
        )
        self.assertGreaterEqual(max(artificial_values), 0)


if __name__ == "__main__":
    unittest.main()
