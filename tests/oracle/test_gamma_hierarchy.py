from __future__ import annotations

import unittest
from dataclasses import dataclass, fields, replace
from fractions import Fraction
from itertools import combinations

from reference.morsehgp3d_oracle.gamma import GammaFiltration, build_gamma_filtration
from reference.morsehgp3d_oracle.hierarchy import (
    ForestCut,
    MergeForest,
    build_exhaustive_hierarchy,
    build_gabriel_partial_forest,
    build_merge_forest,
    build_profile_vertical_map_family,
    build_vertical_map_family,
)


Point3 = tuple[Fraction, Fraction, Fraction]


@dataclass(frozen=True)
class _Ball:
    center: Point3
    squared_radius: Fraction
    support_ids: tuple[int, ...]


def _point(raw_point) -> Point3:
    return tuple(Fraction(value) for value in raw_point)  # type: ignore[return-value]


def _squared_distance(left: Point3, right: Point3) -> Fraction:
    return sum((x - y) ** 2 for x, y in zip(left, right))


def _circumcircle_xy(
    points: tuple[Point3, ...], support: tuple[int, int, int]
) -> _Ball | None:
    first, second, third = (points[index] for index in support)
    ax, ay, _ = first
    bx, by, _ = second
    cx, cy, _ = third
    denominator = 2 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by))
    if denominator == 0:
        return None
    first_norm = ax * ax + ay * ay
    second_norm = bx * bx + by * by
    third_norm = cx * cx + cy * cy
    center = (
        (
            first_norm * (by - cy)
            + second_norm * (cy - ay)
            + third_norm * (ay - by)
        )
        / denominator,
        (
            first_norm * (cx - bx)
            + second_norm * (ax - cx)
            + third_norm * (bx - ax)
        )
        / denominator,
        Fraction(0),
    )
    return _Ball(center, _squared_distance(first, center), support)


def _independent_planar_miniball(points, subset_ids) -> _Ball:
    """Small brute-force planar oracle kept independent from geometry.py."""

    exact_points = tuple(_point(point) for point in points)
    subset = tuple(sorted(subset_ids))
    if not subset:
        raise ValueError("a miniball needs a non-empty subset")
    candidates: list[_Ball] = []
    for point_id in subset:
        candidates.append(_Ball(exact_points[point_id], Fraction(0), (point_id,)))
    for left_id, right_id in combinations(subset, 2):
        left = exact_points[left_id]
        right = exact_points[right_id]
        center = tuple((x + y) / 2 for x, y in zip(left, right))
        candidates.append(
            _Ball(
                center,  # type: ignore[arg-type]
                _squared_distance(left, center),  # type: ignore[arg-type]
                (left_id, right_id),
            )
        )
    for support in combinations(subset, 3):
        candidate = _circumcircle_xy(exact_points, support)
        if candidate is not None:
            candidates.append(candidate)
    containing = [
        candidate
        for candidate in candidates
        if all(
            _squared_distance(exact_points[point_id], candidate.center)
            <= candidate.squared_radius
            for point_id in subset
        )
    ]
    if not containing:
        raise AssertionError("the independent planar miniball found no candidate")
    return min(
        containing,
        key=lambda candidate: (
            candidate.squared_radius,
            len(candidate.support_ids),
            candidate.support_ids,
        ),
    )


def _component_facets(cut) -> tuple[tuple[tuple[int, ...], ...], ...]:
    return tuple(sorted(component.facet_point_ids for component in cut.components))


def _covered_sets(cut) -> tuple[tuple[int, ...], ...]:
    return tuple(sorted(component.covered_point_ids for component in cut.components))


def _unchecked_reordering(filtration: GammaFiltration) -> GammaFiltration:
    """Bypass the storage-order guard solely to stress simultaneous reduction."""

    reordered = object.__new__(GammaFiltration)
    values = {
        "facets": tuple(reversed(filtration.facets)),
        "cofaces": tuple(reversed(filtration.cofaces)),
        "gabriel_hyperedges": tuple(reversed(filtration.gabriel_hyperedges)),
    }
    for field in fields(GammaFiltration):
        object.__setattr__(
            reordered,
            field.name,
            values.get(field.name, getattr(filtration, field.name)),
        )
    return reordered


class GammaHierarchyTests(unittest.TestCase):
    def test_legacy_forest_constructors_default_to_gamma_keyword_only(self) -> None:
        forest = MergeForest(1, "hgp_reduced", (), (), (), ())
        cut = ForestCut(1, "hgp_reduced", Fraction(0), True, ())
        self.assertEqual(forest.graph_kind, "gamma")
        self.assertEqual(cut.graph_kind, "gamma")

    def test_default_geometry_matches_independent_planar_miniballs(self) -> None:
        cases = (
            ([(0, 0, 0), (2, 0, 0), (8, 0, 0)], 1),
            ([(-2, 0, 0), (0, 0, 0), (2, 0, 0)], 2),
            ([(0, 0, 0), (1, 3, 0), (4, 0, 0)], 2),
            (
                [
                    (-2, -1, 0),
                    (-2, 1, 0),
                    (0, 0, 0),
                    (3, 2, 0),
                    (4, -1, 0),
                ],
                2,
            ),
        )
        for points, order in cases:
            with self.subTest(points=points, order=order):
                self.assertEqual(
                    build_gamma_filtration(points, order),
                    build_gamma_filtration(
                        points, order, ball_fn=_independent_planar_miniball
                    ),
                )

    def test_k1_matches_emst_levels_and_rejects_non_gabriel_pair(self) -> None:
        points = [(0, 0, 0), (2, 0, 0), (8, 0, 0)]
        filtration = build_gamma_filtration(
            points, 1, ball_fn=_independent_planar_miniball
        )

        self.assertEqual(
            tuple(edge.simplex_point_ids for edge in filtration.gabriel_hyperedges),
            ((0, 1), (1, 2)),
        )
        self.assertEqual(filtration.cut(Fraction(1)).components[0].covered_point_ids, (0, 1))
        reduced = build_merge_forest(filtration, "hgp_reduced")
        merge_nodes = tuple(node for node in reduced.nodes if node.kind == "merge")
        self.assertEqual(tuple(node.squared_level for node in merge_nodes), (Fraction(1), Fraction(9)))
        self.assertEqual(_covered_sets(reduced.cut(Fraction(9))), ((0, 1, 2),))

    def test_full_forest_replays_every_open_and_closed_gamma_cut(self) -> None:
        points = [(-2, -1, 0), (-2, 1, 0), (0, 0, 0), (3, 2, 0), (4, -1, 0)]
        filtration = build_gamma_filtration(
            points, 2, ball_fn=_independent_planar_miniball
        )
        forest = build_merge_forest(filtration, "full_pi0")

        for level in filtration.critical_levels:
            for closed in (False, True):
                with self.subTest(level=level, closed=closed):
                    self.assertEqual(
                        _component_facets(forest.cut(level, closed=closed)),
                        _component_facets(filtration.cut(level, closed=closed)),
                    )

    def test_reduced_forest_replays_exact_gamma_reduction_at_every_cut(self) -> None:
        points = [(-2, -1, 0), (-2, 1, 0), (0, 0, 0), (3, 2, 0), (4, -1, 0)]
        for order in (1, 2, 3):
            filtration = build_gamma_filtration(
                points, order, ball_fn=_independent_planar_miniball
            )
            forest = build_merge_forest(filtration, "hgp_reduced")

            critical_levels = tuple(filtration.critical_levels)
            interval_levels = tuple(
                (left + right) / 2
                for left, right in zip(critical_levels, critical_levels[1:])
            )
            queried_levels = (
                (critical_levels[0] - 1,)
                + critical_levels
                + interval_levels
                + (critical_levels[-1] + 1,)
            )

            for level in queried_levels:
                for closed in (False, True):
                    with self.subTest(order=order, level=level, closed=closed):
                        gamma_cut = filtration.cut(level, closed=closed)
                        expected = tuple(
                            component
                            for component in gamma_cut.components
                            if order == 1 or component.nontrivial
                        )
                        reduced_cut = forest.cut(level, closed=closed)
                        self.assertEqual(
                            sorted(
                                component.facet_point_ids
                                for component in reduced_cut.components
                            ),
                            sorted(component.facet_point_ids for component in expected),
                        )
                        self.assertEqual(
                            {
                                component.facet_point_ids: component.covered_point_ids
                                for component in reduced_cut.components
                            },
                            {
                                component.facet_point_ids: component.covered_point_ids
                                for component in expected
                            },
                        )

    def test_binary_merge_keeps_simultaneous_facet_as_coverage_only(self) -> None:
        points = [(-2, 0, 0), (0, 0, 0), (2, 0, 0)]
        filtration = build_gamma_filtration(
            points, 2, ball_fn=_independent_planar_miniball
        )
        full = build_merge_forest(filtration, "full_pi0")
        batch = next(batch for batch in full.batches if batch.squared_level == 4)
        merge = next(node for node in full.nodes if node.kind == "merge")

        self.assertEqual(len(merge.child_ids), 2)
        self.assertEqual(batch.activated_facet_point_ids, ((0, 2),))
        self.assertEqual(len(batch.coverage_deltas), 1)
        self.assertEqual(batch.coverage_deltas[0].added_facet_point_ids, ((0, 2),))
        self.assertEqual(batch.coverage_deltas[0].added_point_ids, ())
        self.assertEqual(
            _component_facets(full.cut(Fraction(4))),
            (((0, 1), (0, 2), (1, 2)),),
        )

        reduced = build_merge_forest(filtration, "hgp_reduced")
        self.assertEqual(tuple(node.kind for node in reduced.nodes), ("birth",))
        self.assertEqual(reduced.nodes[0].squared_level, Fraction(4))
        self.assertEqual(reduced.nodes[0].covered_point_ids, (0, 1, 2))

    def test_non_well_centered_tetrahedron_batches_its_simultaneous_facet(self) -> None:
        points = [(0, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1)]
        filtration = build_gamma_filtration(points, 3)
        full = build_merge_forest(filtration, "full_pi0")
        merge = next(node for node in full.nodes if node.kind == "merge")
        batch = next(batch for batch in full.batches if batch.squared_level == Fraction(2, 3))

        self.assertEqual(
            tuple(facet.squared_level for facet in filtration.facets),
            (Fraction(1, 2), Fraction(1, 2), Fraction(1, 2), Fraction(2, 3)),
        )
        self.assertEqual(filtration.cofaces[0].support_ids, (1, 2, 3))
        self.assertEqual(filtration.cofaces[0].squared_level, Fraction(2, 3))
        self.assertEqual(len(merge.child_ids), 3)
        self.assertEqual(batch.activated_facet_point_ids, ((1, 2, 3),))
        self.assertEqual(batch.coverage_deltas[0].added_point_ids, ())
        self.assertEqual(
            _component_facets(full.cut(Fraction(2, 3))),
            (((0, 1, 2), (0, 1, 3), (0, 2, 3), (1, 2, 3)),),
        )
        lower_filtration = build_gamma_filtration(points, 2)
        self.assertTrue(
            build_vertical_map_family(filtration, lower_filtration).is_natural
        )

    def test_equal_level_k1_event_is_one_ternary_multifusion(self) -> None:
        points = [(-2, 0, 0), (0, 0, 0), (2, 0, 0)]
        filtration = build_gamma_filtration(
            points, 1, ball_fn=_independent_planar_miniball
        )
        forest = build_merge_forest(filtration, "hgp_reduced")
        merges = tuple(node for node in forest.nodes if node.kind == "merge")

        self.assertEqual(len(merges), 1)
        self.assertEqual(merges[0].squared_level, Fraction(1))
        self.assertEqual(len(merges[0].child_ids), 3)

    def test_acute_triangle_is_one_ternary_merge_and_order_independent(self) -> None:
        points = [(0, 0, 0), (1, 3, 0), (4, 0, 0)]
        filtration = build_gamma_filtration(
            points, 2, ball_fn=_independent_planar_miniball
        )
        forest = build_merge_forest(filtration, "full_pi0")
        merges = tuple(node for node in forest.nodes if node.kind == "merge")

        self.assertEqual(
            tuple(facet.squared_level for facet in filtration.facets),
            (Fraction(5, 2), Fraction(4), Fraction(9, 2)),
        )
        self.assertEqual(len(merges), 1)
        self.assertEqual(merges[0].squared_level, Fraction(5))
        self.assertEqual(len(merges[0].child_ids), 3)
        self.assertEqual(
            build_merge_forest(_unchecked_reordering(filtration), "full_pi0"),
            forest,
        )

    def test_reduced_profile_preserves_overlapping_point_unions(self) -> None:
        points = [(-2, -1, 0), (-2, 1, 0), (0, 0, 0), (3, 2, 0), (4, -1, 0)]
        filtration = build_gamma_filtration(
            points, 2, ball_fn=_independent_planar_miniball
        )
        reduced = build_merge_forest(filtration, "hgp_reduced")
        second_birth = Fraction(1105, 242)

        reduced_cut = reduced.cut(second_birth)
        gamma_cut = filtration.cut(second_birth, graph_kind="gamma")
        nontrivial_gamma = tuple(
            component.facet_point_ids
            for component in gamma_cut.components
            if component.nontrivial
        )
        self.assertEqual(_component_facets(reduced_cut), tuple(sorted(nontrivial_gamma)))
        self.assertEqual(_covered_sets(reduced_cut), ((0, 1, 2), (2, 3, 4)))
        self.assertEqual(
            set(reduced_cut.components[0].covered_point_ids)
            & set(reduced_cut.components[1].covered_point_ids),
            {2},
        )
        self.assertEqual(_covered_sets(reduced.cut(Fraction(13, 2))), ((0, 1, 2, 3, 4),))

    def test_reduced_profile_keeps_a_fully_redundant_batch_delta(self) -> None:
        points = [(0, 0, 0), (4, 0, 0), (1, 3, 0), (1, 1, 0)]
        filtration = build_gamma_filtration(
            points, 2, ball_fn=_independent_planar_miniball
        )
        reduced = build_merge_forest(filtration, "hgp_reduced")
        batch = next(batch for batch in reduced.batches if batch.squared_level == 5)

        self.assertEqual(batch.relation_simplex_point_ids, ((0, 1, 2),))
        self.assertFalse(batch.created_node_ids)
        self.assertEqual(len(batch.pre_components), 1)
        self.assertEqual(batch.pre_components, batch.post_components)
        self.assertEqual(len(batch.coverage_deltas), 1)
        self.assertEqual(batch.coverage_deltas[0].added_facet_point_ids, ())
        self.assertEqual(batch.coverage_deltas[0].added_point_ids, ())
        self.assertTrue(batch.coverage_deltas[0].fully_redundant)
        self.assertIn(batch.coverage_deltas[0], reduced.coverage_log)

    def test_gabriel_partial_forest_is_separate_from_exact_gamma(self) -> None:
        points = [
            (0, 0, 0),
            (0, 0, 4),
            (0, 3, 1),
            (2, 3, 2),
            (3, 1, 2),
        ]
        filtration = build_gamma_filtration(points, 2)
        exact = build_merge_forest(filtration, "hgp_reduced")
        partial = build_gabriel_partial_forest(filtration)

        self.assertEqual(exact.graph_kind, "gamma")
        self.assertEqual(partial.graph_kind, "gabriel")
        self.assertEqual(exact.cut(Fraction(24)).graph_kind, "gamma")
        self.assertEqual(partial.cut(Fraction(24)).graph_kind, "gabriel")
        self.assertNotEqual(exact, partial)

        target = build_gamma_filtration(points, 1)
        target_exact = build_merge_forest(target, "hgp_reduced")
        with self.assertRaisesRegex(ValueError, "exhaustive Gamma forests"):
            build_profile_vertical_map_family(
                filtration,
                target,
                partial,
                target_exact,
                "hgp_reduced",
            )

    def test_terminal_order_is_present_only_in_full_profile(self) -> None:
        points = [(-1, 0, 0), (1, 0, 0)]
        filtration = build_gamma_filtration(
            points, 2, ball_fn=_independent_planar_miniball
        )

        full = build_merge_forest(filtration, "full_pi0")
        reduced = build_merge_forest(filtration, "hgp_reduced")
        self.assertEqual(len(full.nodes), 1)
        self.assertEqual(full.nodes[0].kind, "birth")
        self.assertEqual(full.nodes[0].squared_level, Fraction(1))
        self.assertEqual(full.nodes[0].covered_point_ids, (0, 1))
        self.assertFalse(reduced.nodes)
        self.assertFalse(reduced.batches)

        target = build_gamma_filtration(
            points, 1, ball_fn=_independent_planar_miniball
        )
        family = build_vertical_map_family(filtration, target)
        at_birth = next(
            mapping
            for mapping in family.maps
            if mapping.squared_level == 1 and mapping.closed
        )
        self.assertEqual(len(at_birth.assignments), 1)
        self.assertEqual(at_birth.assignments[0].source_component_facets, ((0, 1),))
        self.assertEqual(at_birth.assignments[0].target_component_facets, ((0,), (1,)))

    def test_vertical_maps_are_unique_and_natural_at_all_exact_cuts(self) -> None:
        points = [(-2, -1, 0), (-2, 1, 0), (0, 0, 0), (3, 2, 0), (4, -1, 0)]
        target = build_gamma_filtration(
            points, 1, ball_fn=_independent_planar_miniball
        )
        source = build_gamma_filtration(
            points, 2, ball_fn=_independent_planar_miniball
        )
        family = build_vertical_map_family(source, target)

        self.assertTrue(family.is_natural)
        self.assertGreater(family.naturality_squares_checked, 0)
        selected = next(
            mapping
            for mapping in family.maps
            if mapping.squared_level == Fraction(1105, 242) and mapping.closed
        )
        self.assertEqual(len(selected.assignments), 2)
        self.assertEqual(
            len({assignment.target_component_facets for assignment in selected.assignments}),
            1,
        )

    def test_profile_vertical_maps_are_total_and_natural_for_both_profiles(self) -> None:
        points = [(-2, -1, 0), (-2, 1, 0), (0, 0, 0), (3, 2, 0), (4, -1, 0)]
        filtrations = tuple(build_gamma_filtration(points, order) for order in (1, 2, 3))

        for profile in ("full_pi0", "hgp_reduced"):
            forests = tuple(
                build_merge_forest(filtration, profile) for filtration in filtrations
            )
            for source_index in (1, 2):
                with self.subTest(profile=profile, source_order=source_index + 1):
                    family = build_profile_vertical_map_family(
                        filtrations[source_index],
                        filtrations[source_index - 1],
                        forests[source_index],
                        forests[source_index - 1],
                        profile,
                    )
                    self.assertTrue(family.is_natural)
                    self.assertGreater(family.naturality_squares_checked, 0)
                    for mapping in family.maps:
                        source_cut = forests[source_index].cut(
                            mapping.squared_level, closed=mapping.closed
                        )
                        target_cut = forests[source_index - 1].cut(
                            mapping.squared_level, closed=mapping.closed
                        )
                        self.assertEqual(
                            set(mapping.assignment_by_source),
                            {
                                component.facet_point_ids
                                for component in source_cut.components
                            },
                        )
                        self.assertTrue(
                            all(
                                assignment.target_component_facets
                                in {
                                    component.facet_point_ids
                                    for component in target_cut.components
                                }
                                for assignment in mapping.assignments
                            )
                        )

    def test_profile_vertical_projection_fails_closed_on_missing_target(self) -> None:
        points = [(0, 0, 0), (2, 0, 0), (5, 1, 0)]
        target = build_gamma_filtration(points, 1)
        source = build_gamma_filtration(points, 2)
        source_forest = build_merge_forest(source, "hgp_reduced")
        target_forest = build_merge_forest(target, "hgp_reduced")
        incomplete_target = replace(target_forest, batches=(), root_ids=())

        with self.assertRaisesRegex(ValueError, "bijective"):
            build_profile_vertical_map_family(
                source,
                target,
                source_forest,
                incomplete_target,
                "hgp_reduced",
            )

    def test_hierarchy_validates_bounds_and_builds_every_effective_order(self) -> None:
        points = [(0, 0, 0), (1, 0, 0), (0, 1, 0)]
        hierarchy = build_exhaustive_hierarchy(
            points,
            10,
            "full_pi0",
            ball_fn=_independent_planar_miniball,
        )
        self.assertEqual(hierarchy.k_eff, 3)
        self.assertEqual(tuple(filtration.order for filtration in hierarchy.filtrations), (1, 2, 3))
        self.assertEqual(len(hierarchy.vertical_maps), 2)
        with self.assertRaises(ValueError):
            build_exhaustive_hierarchy(
                points, 0, "full_pi0", ball_fn=_independent_planar_miniball
            )
        with self.assertRaises(ValueError):
            build_exhaustive_hierarchy(
                points, 11, "full_pi0", ball_fn=_independent_planar_miniball
            )
        with self.assertRaises(ValueError):
            build_exhaustive_hierarchy(
                points,
                2,
                "unknown",  # type: ignore[arg-type]
                ball_fn=_independent_planar_miniball,
            )
        with self.assertRaises(ValueError):
            build_gamma_filtration(points, 4, ball_fn=_independent_planar_miniball)

        unrelated_source = build_gamma_filtration(
            [(0, 0, 0), (2, 0, 0), (0, 2, 0)], 2
        )
        with self.assertRaises(ValueError):
            build_vertical_map_family(unrelated_source, hierarchy.filtrations[0])


if __name__ == "__main__":
    unittest.main()
