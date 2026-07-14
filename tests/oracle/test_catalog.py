from __future__ import annotations

import math
import unittest
from fractions import Fraction
from itertools import permutations

from reference.morsehgp3d_oracle.catalog import (
    CriticalSphere,
    build_critical_catalog,
    enumerate_critical_spheres,
)
from reference.morsehgp3d_oracle.exact import DuplicatePointError


def _event_with_shell(
    catalog: object, shell_ids: tuple[int, ...]
) -> CriticalSphere | None:
    return next(
        (
            event
            for event in getattr(catalog, "events")
            if event.shell_ids == shell_ids
        ),
        None,
    )


class CriticalCatalogTests(unittest.TestCase):
    def test_singleton_and_pair_events_have_exact_shared_roles(self) -> None:
        catalog = build_critical_catalog([(2, 0, 0), (0, 0, 0)], 10)
        self.assertEqual(catalog.k_eff, 2)
        self.assertEqual(catalog.s_max, 2)
        self.assertEqual(catalog.points[0], (Fraction(0), Fraction(0), Fraction(0)))
        self.assertEqual(len(catalog.by_rank(1)), 2)

        pair = _event_with_shell(catalog, (0, 1))
        self.assertIsNotNone(pair)
        assert pair is not None
        self.assertEqual(pair.center, (Fraction(1), Fraction(0), Fraction(0)))
        self.assertEqual(pair.squared_radius, 1)
        self.assertEqual(pair.closed_rank, 2)
        self.assertEqual(pair.birth_order, 2)
        self.assertEqual(pair.saddle_order, 1)
        self.assertEqual(pair.morse_index(2), 0)
        self.assertEqual(pair.morse_index(1), 1)
        self.assertEqual(pair.local_multiplicity(2), 1)
        self.assertEqual(pair.local_multiplicity(1), 1)
        self.assertEqual(pair.arms, ((1,), (0,)))

    def test_acute_triangle_and_well_centred_tetrahedron_are_catalogued(self) -> None:
        triangle = build_critical_catalog(
            [(0, 0, 0), (4, 0, 0), (2, 3, 0)], 3
        )
        triangle_event = _event_with_shell(triangle, (0, 1, 2))
        self.assertIsNotNone(triangle_event)
        assert triangle_event is not None
        self.assertEqual(triangle_event.squared_radius, Fraction(169, 36))
        self.assertEqual(
            triangle_event.barycentric_coordinates,
            (Fraction(13, 36), Fraction(5, 18), Fraction(13, 36)),
        )
        self.assertEqual(triangle_event.local_multiplicity(2), 2)
        self.assertEqual(
            triangle_event.arms,
            ((1, 2), (0, 2), (0, 1)),
        )

        tetrahedron = build_critical_catalog(
            [
                (1, 1, 1),
                (1, -1, -1),
                (-1, 1, -1),
                (-1, -1, 1),
            ],
            4,
        )
        tetra_event = _event_with_shell(tetrahedron, (0, 1, 2, 3))
        self.assertIsNotNone(tetra_event)
        assert tetra_event is not None
        self.assertEqual(tetra_event.center, (Fraction(0),) * 3)
        self.assertEqual(tetra_event.squared_radius, 3)
        self.assertEqual(tetra_event.barycentric_coordinates, (Fraction(1, 4),) * 4)
        self.assertEqual(tetra_event.local_multiplicity(3), 3)

    def test_obtuse_triangle_uses_its_smaller_minimal_support(self) -> None:
        catalog = build_critical_catalog([(0, 0, 0), (4, 0, 0), (1, 1, 0)], 3)
        rank_three = catalog.by_rank(3)
        self.assertEqual(len(rank_three), 1)
        event = rank_three[0]
        self.assertEqual(event.minimal_support_ids, (0, 2))
        self.assertEqual(event.interior_ids, (1,))
        self.assertEqual(event.shell_ids, (0, 2))
        self.assertEqual(event.center, (Fraction(2), Fraction(0), Fraction(0)))
        self.assertEqual(event.squared_radius, 4)
        statistics = catalog.statistics
        self.assertIsNotNone(statistics)
        assert statistics is not None
        self.assertEqual(statistics.support_candidates, 7)
        self.assertEqual(statistics.non_well_centred_candidates, 1)
        self.assertEqual(statistics.accepted_events, 6)
        self.assertEqual(statistics.rejected_candidates, 1)
        self.assertEqual(statistics.point_classifications, 18)

    def test_rank_bound_is_effective_not_requested_k_alone(self) -> None:
        points = [(0, 0, 0), (4, 0, 0), (2, 3, 0)]
        catalog = build_critical_catalog(points, 1)
        self.assertEqual(catalog.k_eff, 1)
        self.assertEqual(catalog.s_max, 2)
        self.assertTrue(catalog.events)
        self.assertTrue(all(event.closed_rank <= 2 for event in catalog.events))
        self.assertFalse(catalog.by_rank(3))

        terminal = build_critical_catalog([(0, 0, 0), (2, 0, 0)], 10)
        self.assertEqual(terminal.k_eff, 2)
        self.assertEqual(terminal.s_max, 2)

    def test_permutations_produce_identical_canonical_catalogues(self) -> None:
        points = [(0, 0, 0), (4, 0, 0), (2, 3, 0), (2, 1, 1)]
        expected = build_critical_catalog(points, 4)
        for permutation in permutations(points):
            with self.subTest(permutation=permutation):
                self.assertEqual(build_critical_catalog(permutation, 4), expected)

    def test_distinct_events_at_an_equal_level_remain_distinct_and_sorted(self) -> None:
        catalog = build_critical_catalog(
            [(0, 0, 0), (2, 0, 0), (10, 0, 0), (12, 0, 0)], 4
        )
        level_one = [event for event in catalog.events if event.squared_radius == 1]
        self.assertEqual([event.shell_ids for event in level_one], [(0, 1), (2, 3)])

    def test_exact_shell_degeneracy_is_reported_not_perturbed(self) -> None:
        right_triangle = [(0, 0, 0), (2, 0, 0), (0, 2, 0)]
        catalog = build_critical_catalog(right_triangle, 3)
        self.assertFalse(catalog.relevant_gp_complete)
        self.assertEqual(len(catalog.degenerate_candidates), 1)
        degeneracy = catalog.degenerate_candidates[0]
        self.assertEqual(degeneracy.reason, "extra_shell")
        self.assertEqual(degeneracy.shell_ids, (0, 1, 2))
        self.assertEqual(degeneracy.ball.support_ids, (1, 2))
        self.assertIsNone(_event_with_shell(catalog, (0, 1, 2)))

    def test_one_ulp_around_degeneracy_changes_the_exact_combinatorics(self) -> None:
        below = math.nextafter(1.0, 0.0)
        above = math.nextafter(1.0, math.inf)
        common = [(-1, 0, 0), (1, 0, 0)]

        inside = build_critical_catalog(common + [(0, below, 0)], 3)
        exact = build_critical_catalog(common + [(0, 1.0, 0)], 3)
        outside = build_critical_catalog(common + [(0, above, 0)], 3)

        self.assertTrue(inside.relevant_gp_complete)
        inside_rank_three = inside.by_rank(3)
        self.assertEqual(len(inside_rank_three), 1)
        self.assertEqual(inside_rank_three[0].interior_ids, (1,))
        self.assertEqual(inside_rank_three[0].shell_ids, (0, 2))

        self.assertFalse(exact.relevant_gp_complete)
        self.assertFalse(exact.by_rank(3))

        self.assertTrue(outside.relevant_gp_complete)
        outside_rank_three = outside.by_rank(3)
        self.assertEqual(len(outside_rank_three), 1)
        self.assertEqual(outside_rank_three[0].interior_ids, ())
        self.assertEqual(outside_rank_three[0].shell_ids, (0, 1, 2))
        self.assertGreater(outside_rank_three[0].squared_radius, 1)

    def test_catalogue_rejects_invalid_set_inputs_and_kmax(self) -> None:
        with self.assertRaises(ValueError):
            build_critical_catalog([], 1)
        with self.assertRaises(DuplicatePointError):
            build_critical_catalog([(0, 0, 0), (-0.0, 0, 0)], 1)
        for invalid in (0, 11):
            with self.subTest(k_max=invalid):
                with self.assertRaises(ValueError):
                    build_critical_catalog([(0, 0, 0)], invalid)
        with self.assertRaises(TypeError):
            build_critical_catalog([(0, 0, 0)], True)

    def test_convenience_enumerator_matches_catalog_events(self) -> None:
        points = [(0, 0, 0), (2, 0, 0), (1, 1, 0)]
        self.assertEqual(
            enumerate_critical_spheres(points, 3),
            build_critical_catalog(points, 3).events,
        )


if __name__ == "__main__":
    unittest.main()
