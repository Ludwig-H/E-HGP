from __future__ import annotations

import math
import unittest
from fractions import Fraction

from reference.morsehgp3d_oracle.exact import (
    DuplicatePointError,
    NonFiniteCoordinateError,
    affine_dimension,
    canonicalize_points,
    normalize_point,
    normalize_points,
    normalize_scalar,
    squared_distance,
)


class ExactNormalizationTests(unittest.TestCase):
    def test_int_fraction_and_float_are_interpreted_exactly(self) -> None:
        self.assertEqual(normalize_scalar(7), Fraction(7))
        self.assertEqual(normalize_scalar(Fraction(2, 3)), Fraction(2, 3))
        self.assertEqual(normalize_scalar(0.5), Fraction(1, 2))
        self.assertEqual(normalize_scalar(0.1), Fraction.from_float(0.1))
        self.assertNotEqual(normalize_scalar(0.1), Fraction(1, 10))
        self.assertEqual(normalize_scalar(-0.0), Fraction(0))

    def test_invalid_scalar_and_point_inputs_fail_closed(self) -> None:
        for value in (float("nan"), float("inf"), float("-inf")):
            with self.subTest(value=value):
                with self.assertRaises(NonFiniteCoordinateError):
                    normalize_scalar(value)
        with self.assertRaises(TypeError):
            normalize_scalar(True)
        with self.assertRaises(TypeError):
            normalize_scalar("1")  # type: ignore[arg-type]
        with self.assertRaises(ValueError):
            normalize_point((1, 2))
        with self.assertRaises(ValueError):
            normalize_point((1, 2, 3, 4))

    def test_normalization_preserves_order_until_canonicalization(self) -> None:
        raw = [(2, 0, 0), (-1, 0, 0), (Fraction(1, 2), 0, 0)]
        self.assertEqual(
            normalize_points(raw),
            (
                (Fraction(2), Fraction(0), Fraction(0)),
                (Fraction(-1), Fraction(0), Fraction(0)),
                (Fraction(1, 2), Fraction(0), Fraction(0)),
            ),
        )
        canonical = canonicalize_points(reversed(raw))
        self.assertEqual(canonical, canonicalize_points(raw))
        self.assertEqual(
            canonical,
            (
                (Fraction(-1), Fraction(0), Fraction(0)),
                (Fraction(1, 2), Fraction(0), Fraction(0)),
                (Fraction(2), Fraction(0), Fraction(0)),
            ),
        )

    def test_duplicate_coordinates_are_rejected_without_jitter(self) -> None:
        with self.assertRaises(DuplicatePointError):
            canonicalize_points([(0, 0, 0), (-0.0, 0, 0)])
        self.assertEqual(
            canonicalize_points([(0, 0, 0), (0, 0, 0)], reject_duplicates=False),
            ((Fraction(0),) * 3, (Fraction(0),) * 3),
        )

    def test_affine_dimensions_zero_through_three(self) -> None:
        self.assertEqual(affine_dimension([(0, 0, 0)]), 0)
        self.assertEqual(affine_dimension([(0, 0, 0), (2, 0, 0), (3, 0, 0)]), 1)
        self.assertEqual(
            affine_dimension([(0, 0, 0), (1, 0, 0), (0, 1, 0), (2, 3, 0)]),
            2,
        )
        self.assertEqual(
            affine_dimension([(0, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1)]),
            3,
        )
        with self.assertRaises(ValueError):
            affine_dimension([])

    def test_one_ulp_distance_is_not_rounded_away(self) -> None:
        below = math.nextafter(1.0, 0.0)
        above = math.nextafter(1.0, math.inf)
        origin = (0, 0, 0)
        self.assertLess(squared_distance(origin, (below, 0, 0)), Fraction(1))
        self.assertGreater(squared_distance(origin, (above, 0, 0)), Fraction(1))


if __name__ == "__main__":
    unittest.main()
