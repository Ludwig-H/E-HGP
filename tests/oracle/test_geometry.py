from __future__ import annotations

import math
import unittest
from fractions import Fraction

from reference.morsehgp3d_oracle.geometry import (
    AffineDependenceError,
    Ball,
    BallRelation,
    PointOutsideAffineHullError,
    barycentric_coordinates,
    circumball,
    classify,
    classify_points,
    is_relative_interior,
    minimum_enclosing_ball,
)


class ExactGeometryTests(unittest.TestCase):
    def test_circumballs_cover_support_sizes_one_through_four(self) -> None:
        singleton = circumball([(3, -2, 5)], [0])
        self.assertEqual(singleton.center, (Fraction(3), Fraction(-2), Fraction(5)))
        self.assertEqual(singleton.squared_radius, 0)

        segment = circumball([(0, 0, 0), (2, 0, 0)], [1, 0])
        self.assertEqual(segment.center, (Fraction(1), Fraction(0), Fraction(0)))
        self.assertEqual(segment.squared_radius, 1)
        self.assertEqual(segment.support_ids, (0, 1))

        triangle_points = [(0, 0, 0), (4, 0, 0), (2, 3, 0)]
        triangle = circumball(triangle_points, [0, 1, 2])
        self.assertEqual(
            triangle.center,
            (Fraction(2), Fraction(5, 6), Fraction(0)),
        )
        self.assertEqual(triangle.squared_radius, Fraction(169, 36))

        tetra_points = [
            (1, 1, 1),
            (1, -1, -1),
            (-1, 1, -1),
            (-1, -1, 1),
        ]
        tetrahedron = circumball(tetra_points, range(4))
        self.assertEqual(tetrahedron.center, (Fraction(0),) * 3)
        self.assertEqual(tetrahedron.squared_radius, 3)

    def test_affinely_dependent_circumsupports_are_rejected(self) -> None:
        with self.assertRaises(AffineDependenceError):
            circumball([(0, 0, 0), (1, 0, 0), (2, 0, 0)], [0, 1, 2])
        with self.assertRaises(AffineDependenceError):
            circumball(
                [(0, 0, 0), (1, 0, 0), (0, 1, 0), (1, 1, 0)],
                range(4),
            )

    def test_barycentrics_distinguish_acute_obtuse_and_outside_hull(self) -> None:
        acute_points = [(0, 0, 0), (4, 0, 0), (2, 3, 0)]
        acute_ball = circumball(acute_points, range(3))
        acute_coordinates = barycentric_coordinates(acute_ball.center, acute_points)
        self.assertEqual(
            acute_coordinates,
            (Fraction(13, 36), Fraction(13, 36), Fraction(5, 18)),
        )
        self.assertTrue(is_relative_interior(acute_ball.center, acute_points))

        obtuse_points = [(0, 0, 0), (4, 0, 0), (1, 1, 0)]
        obtuse_ball = circumball(obtuse_points, range(3))
        self.assertEqual(obtuse_ball.center, (Fraction(2), Fraction(-1), Fraction(0)))
        self.assertFalse(is_relative_interior(obtuse_ball.center, obtuse_points))
        self.assertTrue(any(value < 0 for value in barycentric_coordinates(obtuse_ball.center, obtuse_points)))

        with self.assertRaises(PointOutsideAffineHullError):
            barycentric_coordinates((0, 1, 0), [(0, 0, 0), (1, 0, 0)])

    def test_exact_classification_returns_complete_disjoint_groups(self) -> None:
        points = [(-1, 0, 0), (1, 0, 0), (0, 0, 0), (2, 0, 0)]
        ball = circumball(points, [0, 1])
        self.assertIs(classify(points[0], ball), BallRelation.SHELL)
        self.assertIs(classify(points[2], ball), BallRelation.INTERIOR)
        self.assertIs(classify(points[3], ball), BallRelation.EXTERIOR)
        classification = classify_points(points, ball)
        self.assertEqual(classification.interior_ids, (2,))
        self.assertEqual(classification.shell_ids, (0, 1))
        self.assertEqual(classification.exterior_ids, (3,))
        self.assertEqual(classification.closed_rank, 3)

    def test_miniball_selects_the_true_minimal_support(self) -> None:
        obtuse = [(0, 0, 0), (4, 0, 0), (1, 1, 0)]
        obtuse_ball = minimum_enclosing_ball(obtuse, [0, 1, 2])
        self.assertEqual(obtuse_ball.support_ids, (0, 1))
        self.assertEqual(obtuse_ball.center, (Fraction(2), Fraction(0), Fraction(0)))
        self.assertEqual(obtuse_ball.squared_radius, 4)

        acute = [(0, 0, 0), (4, 0, 0), (2, 3, 0)]
        acute_ball = minimum_enclosing_ball(acute)
        self.assertEqual(acute_ball.support_ids, (0, 1, 2))
        self.assertEqual(acute_ball.squared_radius, Fraction(169, 36))

        non_well_centred_tetrahedron = [
            (0, 0, 0),
            (2, 0, 0),
            (0, 2, 0),
            (0, 0, 2),
        ]
        tetra_ball = minimum_enclosing_ball(non_well_centred_tetrahedron)
        self.assertEqual(tetra_ball.support_ids, (1, 2, 3))
        self.assertEqual(tetra_ball.center, (Fraction(2, 3),) * 3)
        self.assertEqual(tetra_ball.squared_radius, Fraction(8, 3))

    def test_miniball_subset_ids_are_preserved_and_validated(self) -> None:
        points = [(-100, 0, 0), (0, 0, 0), (2, 0, 0), (100, 0, 0)]
        ball = minimum_enclosing_ball(points, [2, 1])
        self.assertEqual(ball.support_ids, (1, 2))
        self.assertEqual(ball.squared_radius, 1)
        with self.assertRaises(ValueError):
            minimum_enclosing_ball(points, [])
        with self.assertRaises(ValueError):
            minimum_enclosing_ball(points, [1, 1])
        with self.assertRaises(IndexError):
            minimum_enclosing_ball(points, [4])
        with self.assertRaises(ValueError):
            minimum_enclosing_ball([])

    def test_degenerate_tie_has_a_canonical_support(self) -> None:
        square = [(-1, -1, 0), (1, -1, 0), (1, 1, 0), (-1, 1, 0)]
        ball = minimum_enclosing_ball(square)
        self.assertEqual(ball.center, (Fraction(0),) * 3)
        self.assertEqual(ball.squared_radius, 2)
        self.assertEqual(ball.support_ids, (0, 2))

    def test_miniball_rejects_a_smaller_exterior_circumsupport(self) -> None:
        points = [
            (-1, 0, -2),
            (-1, 0, 2),
            (1, -2, 0),
            (1, 2, 0),
            (2, -1, 0),
            (2, 1, 0),
        ]
        exterior_triple = circumball(points, [2, 3, 4])
        self.assertEqual(exterior_triple.center, (Fraction(0),) * 3)
        self.assertEqual(exterior_triple.squared_radius, 5)
        exterior_support_points = [
            points[point_id] for point_id in exterior_triple.support_ids
        ]
        self.assertEqual(
            barycentric_coordinates(
                exterior_triple.center,
                exterior_support_points,
            ),
            (Fraction(5, 4), Fraction(3, 4), Fraction(-1)),
        )
        self.assertFalse(
            is_relative_interior(
                exterior_triple.center,
                exterior_support_points,
            )
        )
        exterior_classification = classify_points(points, exterior_triple)
        self.assertEqual(exterior_classification.interior_ids, ())
        self.assertEqual(exterior_classification.shell_ids, tuple(range(6)))
        self.assertEqual(exterior_classification.exterior_ids, ())

        ball = minimum_enclosing_ball(points)
        self.assertEqual(ball.center, (Fraction(0),) * 3)
        self.assertEqual(ball.squared_radius, 5)
        self.assertEqual(ball.support_ids, (0, 1, 2, 3))

    def test_permutation_changes_only_support_identifiers(self) -> None:
        points = [(0, 0, 0), (4, 0, 0), (2, 3, 0), (2, 1, 0)]
        first = minimum_enclosing_ball(points)
        permutation = (2, 0, 3, 1)
        permuted_points = [points[index] for index in permutation]
        second = minimum_enclosing_ball(permuted_points)
        self.assertEqual(first.center, second.center)
        self.assertEqual(first.squared_radius, second.squared_radius)
        first_support = {points[index] for index in first.support_ids}
        second_support = {permuted_points[index] for index in second.support_ids}
        self.assertEqual(first_support, second_support)

    def test_one_ulp_near_shell_is_classified_without_tolerance(self) -> None:
        endpoints = [(-1, 0, 0), (1, 0, 0)]
        ball = circumball(endpoints, [0, 1])
        below = (0, math.nextafter(1.0, 0.0), 0)
        above = (0, math.nextafter(1.0, math.inf), 0)
        self.assertIs(classify(below, ball), BallRelation.INTERIOR)
        self.assertIs(classify((0, 1, 0), ball), BallRelation.SHELL)
        self.assertIs(classify(above, ball), BallRelation.EXTERIOR)

    def test_ball_rejects_negative_radius_and_noncanonical_support(self) -> None:
        with self.assertRaises(ValueError):
            Ball((0, 0, 0), Fraction(-1), (0,))
        with self.assertRaises(ValueError):
            Ball((0, 0, 0), Fraction(0), (0, 0))


if __name__ == "__main__":
    unittest.main()
