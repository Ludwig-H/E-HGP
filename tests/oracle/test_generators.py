from __future__ import annotations

import unittest

from reference.morsehgp3d_oracle.catalog import build_critical_catalog
from tests.oracle.generators import affine_dimension, generate_affine_cloud


class DeterministicGeneratorTests(unittest.TestCase):
    def test_requested_affine_dimension_is_exact(self) -> None:
        for dimension in (1, 2, 3):
            with self.subTest(dimension=dimension):
                points = generate_affine_cloud(dimension, dimension + 9, 1729)
                self.assertEqual(affine_dimension(points), dimension)
                self.assertEqual(len(points), len(set(points)))

    def test_seed_is_reproducible_and_effective(self) -> None:
        first = generate_affine_cloud(3, 12, 11)
        self.assertEqual(first, generate_affine_cloud(3, 12, 11))
        self.assertNotEqual(first, generate_affine_cloud(3, 12, 12))

    def test_fixed_frames_do_not_force_exact_extra_shells(self) -> None:
        for dimension in (1, 2, 3):
            with self.subTest(dimension=dimension):
                points = generate_affine_cloud(dimension, dimension + 1, 0)
                catalog = build_critical_catalog(points, min(3, len(points)))
                self.assertTrue(catalog.relevant_gp_complete)

    def test_invalid_parameters_fail_closed(self) -> None:
        for dimension, count in ((0, 4), (4, 5), (3, 3)):
            with self.subTest(dimension=dimension, count=count):
                with self.assertRaises(ValueError):
                    generate_affine_cloud(dimension, count, 0)


if __name__ == "__main__":
    unittest.main()
