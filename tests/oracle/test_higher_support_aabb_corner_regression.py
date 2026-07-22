from __future__ import annotations

import json
import unittest
from fractions import Fraction
from pathlib import Path


FIXTURE = (
    Path(__file__).resolve().parents[1]
    / "fixtures"
    / "regressions"
    / "higher_support_aabb_corner_regression.json"
)


def q(value: int | str) -> Fraction:
    return Fraction(value)


def dot(left: tuple[Fraction, Fraction], right: tuple[Fraction, Fraction]) -> Fraction:
    return left[0] * right[0] + left[1] * right[1]


def subtract(
    left: tuple[Fraction, Fraction], right: tuple[Fraction, Fraction]
) -> tuple[Fraction, Fraction]:
    return left[0] - right[0], left[1] - right[1]


def triangle_is_well_centered(t: Fraction) -> bool:
    points = ((t, Fraction(2)), (Fraction(-1), Fraction(0)), (Fraction(1), Fraction(0)))
    for index in range(3):
        first = points[(index + 1) % 3]
        second = points[(index + 2) % 3]
        if dot(subtract(first, points[index]), subtract(second, points[index])) <= 0:
            return False
    return True


def triangle_circumcenter(
    points: tuple[
        tuple[Fraction, Fraction],
        tuple[Fraction, Fraction],
        tuple[Fraction, Fraction],
    ],
) -> tuple[Fraction, Fraction]:
    anchor, first, second = points
    first_edge = subtract(first, anchor)
    second_edge = subtract(second, anchor)
    first_rhs = dot(first, first) - dot(anchor, anchor)
    second_rhs = dot(second, second) - dot(anchor, anchor)
    determinant = 2 * (
        first_edge[0] * second_edge[1]
        - first_edge[1] * second_edge[0]
    )
    if determinant == 0:
        raise ValueError("the regression triangle must be affinely independent")
    return (
        (first_rhs * second_edge[1] - first_edge[1] * second_rhs)
        / determinant,
        (first_edge[0] * second_rhs - first_rhs * second_edge[0])
        / determinant,
    )


def squared_distance(
    left: tuple[Fraction, Fraction], right: tuple[Fraction, Fraction]
) -> Fraction:
    difference = subtract(left, right)
    return dot(difference, difference)


def query_power(t: Fraction, query: tuple[Fraction, Fraction]) -> Fraction:
    points = (
        (t, Fraction(2)),
        (Fraction(-1), Fraction(0)),
        (Fraction(1), Fraction(0)),
    )
    center = triangle_circumcenter(points)
    return squared_distance(query, center) - squared_distance(points[0], center)


class HigherSupportAabbCornerRegressionTests(unittest.TestCase):
    def test_support_corners_do_not_decide_universal_polynomials(self) -> None:
        data = json.loads(FIXTURE.read_text(encoding="utf-8"))
        self.assertEqual(data["fixture_id"], "higher-support-aabb-corner-regression-v1")

        well = data["well_centering_case"]
        endpoints = tuple(q(value) for value in well["t_interval"])
        self.assertEqual(
            [triangle_is_well_centered(value) for value in endpoints],
            well["corner_well_centered"],
        )
        self.assertTrue(triangle_is_well_centered(q(well["interior_t"])))

        power = data["query_power_case"]
        power_endpoints = tuple(q(value) for value in power["t_interval"])
        query = tuple(q(value) for value in power["query_point"][:2])
        observed_corner_powers = [
            query_power(value, query) for value in power_endpoints
        ]
        self.assertEqual(
            observed_corner_powers,
            [q(value) for value in power["corner_powers"]],
        )
        self.assertTrue(all(value < 0 for value in observed_corner_powers))
        interior_power = query_power(q(power["interior_t"]), query)
        self.assertEqual(interior_power, q(power["interior_power"]))
        self.assertGreater(interior_power, 0)


if __name__ == "__main__":
    unittest.main()
