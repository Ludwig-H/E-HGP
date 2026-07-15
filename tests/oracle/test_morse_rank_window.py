from __future__ import annotations

import json
import unittest
from fractions import Fraction
from pathlib import Path

from reference.morsehgp3d_oracle.catalog import build_critical_catalog
from reference.morsehgp3d_oracle.exact import affine_dimension, squared_distance
from reference.morsehgp3d_oracle.geometry import (
    classify_points,
    minimum_enclosing_ball,
)


FIXTURE = (
    Path(__file__).parents[1]
    / "fixtures"
    / "regressions"
    / "morse_rank_window.json"
)


def _fraction(value: object) -> Fraction:
    return Fraction(value)


class MorseRankWindowRegressionTests(unittest.TestCase):
    def setUp(self) -> None:
        with FIXTURE.open(encoding="utf-8") as fixture_file:
            self.fixture = json.load(fixture_file)
        self.points = tuple(
            tuple(_fraction(coordinate) for coordinate in point)
            for point in self.fixture["points"]
        )
        self.expected = self.fixture["expected"]

    def test_well_centred_ball_is_critical_only_in_its_rank_window(self) -> None:
        self.assertEqual(
            affine_dimension(self.points), self.expected["affine_dimension"]
        )
        ball = minimum_enclosing_ball(
            self.points, self.expected["target_subset_source_ids"]
        )
        classification = classify_points(self.points, ball)
        self.assertEqual(ball.center, tuple(map(Fraction, self.expected["center"])))
        self.assertEqual(ball.squared_radius, _fraction(self.expected["squared_level"]))
        self.assertEqual(
            ball.support_ids,
            tuple(self.expected["minimal_support_source_ids"]),
        )
        self.assertEqual(
            classification.interior_ids,
            tuple(self.expected["interior_source_ids"]),
        )
        self.assertEqual(
            classification.shell_ids,
            tuple(self.expected["shell_source_ids"]),
        )
        self.assertEqual(
            classification.exterior_ids,
            tuple(self.expected["exterior_source_ids"]),
        )
        self.assertEqual(classification.closed_rank, self.expected["closed_rank"])

        critical_orders = tuple(
            order
            for order in range(1, len(self.points) + 1)
            if len(classification.interior_ids) < order <= classification.closed_rank
        )
        self.assertEqual(
            critical_orders,
            tuple(record["order"] for record in self.expected["critical_orders"]),
        )

        distances = tuple(
            sorted(squared_distance(point, ball.center) for point in self.points)
        )
        for record in self.expected["noncritical_descending_orders"]:
            order = record["order"]
            self.assertLessEqual(order, len(classification.interior_ids))
            self.assertLess(distances[order - 1], ball.squared_radius)
            self.assertEqual(distances[order - 1], _fraction(record["d_k_at_center"]))

        for record in self.expected["noncritical_upper_orders"]:
            order = record["order"]
            self.assertGreater(order, classification.closed_rank)
            self.assertGreater(distances[order - 1], ball.squared_radius)
            self.assertEqual(distances[order - 1], _fraction(record["d_k_at_center"]))

        catalog = build_critical_catalog(self.points, k_max=5)
        matching = tuple(
            event
            for event in catalog.events
            if event.center == ball.center
            and event.squared_radius == ball.squared_radius
            and event.closed_rank == classification.closed_rank
            and len(event.interior_ids) == len(classification.interior_ids)
            and len(event.shell_ids) == len(classification.shell_ids)
        )
        self.assertEqual(len(matching), 1)
        event = matching[0]
        self.assertEqual(event.saddle_order, 3)
        self.assertEqual(event.birth_order, 4)
        for record in self.expected["critical_orders"]:
            order = record["order"]
            self.assertTrue(event.is_critical_at_order(order))
            self.assertEqual(event.morse_index(order), record["morse_index"])
            self.assertEqual(
                event.local_multiplicity(order), record["local_multiplicity"]
            )
        for record in self.expected["noncritical_descending_orders"]:
            order = record["order"]
            self.assertFalse(event.is_critical_at_order(order))
            self.assertEqual(event.morse_index(order), record["formal_index"])
            self.assertEqual(
                event.local_multiplicity(order), record["local_multiplicity"]
            )
        for record in self.expected["noncritical_upper_orders"]:
            order = record["order"]
            self.assertFalse(event.is_critical_at_order(order))
            self.assertEqual(event.morse_index(order), record["formal_index"])
            self.assertEqual(
                event.local_multiplicity(order), record["local_multiplicity"]
            )


if __name__ == "__main__":
    unittest.main()
