from __future__ import annotations

import hashlib
import importlib
import json
import math
import unittest
from fractions import Fraction
from pathlib import Path

from reference.morsehgp3d_oracle.catalog import build_critical_catalog
from reference.morsehgp3d_oracle.exact import affine_dimension
from reference.morsehgp3d_oracle.gamma import build_gamma_filtration
from reference.morsehgp3d_oracle.generators import generate_affine_cloud
from reference.morsehgp3d_oracle.geometry import (
    BallRelation,
    circumball,
    classify,
)
from reference.morsehgp3d_oracle.hierarchy import build_merge_forest


FIXTURE_ROOT = Path(__file__).parents[1] / "fixtures"
REPOSITORY_ROOT = Path(__file__).parents[2]
Surd3 = tuple[Fraction, Fraction]
SurdPoint3 = tuple[Surd3, Surd3, Surd3]


def _load_fixture(relative_path: str) -> dict[str, object]:
    with (FIXTURE_ROOT / relative_path).open(encoding="utf-8") as fixture_file:
        return json.load(fixture_file)


def _compact_sha256(payload: object, *, sort_keys: bool = False) -> str:
    encoded = json.dumps(
        payload,
        ensure_ascii=True,
        separators=(",", ":"),
        sort_keys=sort_keys,
    ).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def _fraction(value: object) -> Fraction:
    return Fraction(str(value))


def _number(value: object) -> Fraction | float:
    token = str(value)
    if "0x" in token.lower():
        return float.fromhex(token)
    return Fraction(token)


def _surd(raw_value: object) -> Surd3:
    rational, sqrt3_coefficient = raw_value  # type: ignore[misc]
    return _fraction(rational), _fraction(sqrt3_coefficient)


def _surd_add(left: Surd3, right: Surd3) -> Surd3:
    return left[0] + right[0], left[1] + right[1]


def _surd_subtract(left: Surd3, right: Surd3) -> Surd3:
    return left[0] - right[0], left[1] - right[1]


def _surd_multiply(left: Surd3, right: Surd3) -> Surd3:
    return (
        left[0] * right[0] + 3 * left[1] * right[1],
        left[0] * right[1] + left[1] * right[0],
    )


def _surd_scale(value: Fraction, term: Surd3) -> Surd3:
    return value * term[0], value * term[1]


def _surd_squared_distance(left: SurdPoint3, right: SurdPoint3) -> Surd3:
    total = (Fraction(0), Fraction(0))
    for left_coordinate, right_coordinate in zip(left, right):
        delta = _surd_subtract(left_coordinate, right_coordinate)
        total = _surd_add(total, _surd_multiply(delta, delta))
    return total


def _surd_centroid(points: tuple[SurdPoint3, ...]) -> SurdPoint3:
    coordinates = []
    for axis in range(3):
        total = (Fraction(0), Fraction(0))
        for point in points:
            total = _surd_add(total, point[axis])
        coordinates.append(_surd_scale(Fraction(1, len(points)), total))
    return tuple(coordinates)  # type: ignore[return-value]


def _squared_distance(left: list[int], right: list[int]) -> int:
    return sum((left_coordinate - right_coordinate) ** 2 for left_coordinate, right_coordinate in zip(left, right))


class Phase1FixtureTests(unittest.TestCase):
    def test_chapter6_exact_metric_and_rational_surrogate(self) -> None:
        fixture = _load_fixture("exact/chapter6_six_points.json")
        exact = fixture["exact_coordinate_model"]
        assert isinstance(exact, dict)
        labels = exact["labels"]
        raw_points = exact["points"]
        expected = exact["expected"]
        assert isinstance(labels, list)
        assert isinstance(raw_points, dict)
        assert isinstance(expected, dict)

        exact_payload = [raw_points[label] for label in labels]
        self.assertEqual(
            _compact_sha256(exact_payload),
            exact["coordinate_hash_sha256"],
        )
        points: dict[str, SurdPoint3] = {
            label: tuple(_surd(coordinate) for coordinate in raw_points[label])  # type: ignore[misc]
            for label in labels
        }
        side_squared_distance = _surd(expected["side_squared_distance"])
        for pair in expected["side_and_bridge_pairs"]:  # type: ignore[union-attr]
            self.assertEqual(
                _surd_squared_distance(points[pair[0]], points[pair[1]]),
                side_squared_distance,
            )

        triangle_squared_level = _surd(expected["triangle_squared_level"])
        for simplex in expected["triangle_simplices"]:  # type: ignore[union-attr]
            simplex_points = tuple(points[label] for label in simplex)
            center = _surd_centroid(simplex_points)
            self.assertEqual(
                tuple(_surd_squared_distance(point, center) for point in simplex_points),
                (triangle_squared_level,) * 3,
            )

        long_squared_distance = _surd(expected["final_long_squared_distance"])
        self.assertNotEqual(long_squared_distance[1], 0)
        for pair in expected["final_long_pairs"]:  # type: ignore[union-attr]
            self.assertEqual(
                _surd_squared_distance(points[pair[0]], points[pair[1]]),
                long_squared_distance,
            )
        self.assertEqual(
            _surd_scale(Fraction(1, 4), long_squared_distance),
            _surd(expected["final_squared_level"]),
        )
        diagnostic = exact["rational_coordinate_diagnostic"]
        assert isinstance(diagnostic, dict)
        self.assertFalse(diagnostic["congruent_rational_copy_exists"])

        surrogate = fixture["rational_3d_surrogate"]
        assert isinstance(surrogate, dict)
        surrogate_labels = surrogate["labels"]
        surrogate_raw_points = surrogate["points"]
        surrogate_expected = surrogate["expected"]
        assert isinstance(surrogate_labels, list)
        assert isinstance(surrogate_raw_points, dict)
        assert isinstance(surrogate_expected, dict)
        surrogate_payload = [surrogate_raw_points[label] for label in surrogate_labels]
        self.assertEqual(
            _compact_sha256(surrogate_payload),
            surrogate["coordinate_hash_sha256"],
        )
        rational_points = [
            tuple(_fraction(value) for value in surrogate_raw_points[label])
            for label in surrogate_labels
        ]
        label_to_id = {label: point_id for point_id, label in enumerate(surrogate_labels)}

        def point_ids(label: str) -> tuple[int, ...]:
            return tuple(sorted(label_to_id[point_label] for point_label in label))

        filtration = build_gamma_filtration(rational_points, 2)
        expected_levels = {
            point_ids(simplex): (
                _fraction(surrogate_expected["triangle_squared_level"])
                if simplex in ("ABC", "DEF")
                else _fraction(surrogate_expected["final_squared_level"])
            )
            for simplex in surrogate_expected["gabriel_simplices"]  # type: ignore[union-attr]
        }
        self.assertEqual(
            {
                edge.simplex_point_ids: edge.squared_level
                for edge in filtration.gabriel_hyperedges
            },
            expected_levels,
        )

        for raw_level, raw_components in surrogate_expected[
            "component_labels_by_level"
        ].items():  # type: ignore[union-attr]
            actual_components = tuple(
                sorted(
                    component.facet_point_ids
                    for component in filtration.cut(_fraction(raw_level)).components
                )
            )
            expected_components = tuple(
                sorted(
                    tuple(sorted(point_ids(facet) for facet in component))
                    for component in raw_components
                )
            )
            self.assertEqual(actual_components, expected_components)

        triangle_level = _fraction(surrogate_expected["triangle_squared_level"])
        final_level = _fraction(surrogate_expected["final_squared_level"])
        bridge = point_ids("CD")
        self.assertIn(
            (bridge,),
            tuple(
                component.facet_point_ids
                for component in filtration.cut(triangle_level).components
            ),
        )
        self.assertFalse(
            any(
                component.facet_point_ids == (bridge,)
                for component in filtration.cut(final_level).components
            )
        )

        forest = build_merge_forest(filtration, "full_pi0")
        merges = tuple(node for node in forest.nodes if node.kind == "merge")
        simultaneous_triangle_merges = tuple(
            node for node in merges if node.squared_level == triangle_level
        )
        self.assertEqual(len(simultaneous_triangle_merges), 2)
        self.assertTrue(
            all(len(node.child_ids) == 3 for node in simultaneous_triangle_merges)
        )
        final_merge = tuple(node for node in merges if node.squared_level == final_level)
        self.assertEqual(len(final_merge), 1)
        self.assertEqual(len(final_merge[0].child_ids), 3)

    def test_gabriel_pair_is_absent_from_fixed_small_lnn(self) -> None:
        fixture = _load_fixture("regressions/gabriel_pair_missing_lnn.json")
        points = fixture["points"]
        expected = fixture["expected"]
        assert isinstance(points, list)
        assert isinstance(expected, dict)
        self.assertEqual(
            _compact_sha256(sorted(points)),
            fixture["coordinate_hash_sha256"],
        )
        left_id, right_id = expected["target_pair_ids"]
        target_pair = tuple(sorted((left_id, right_id)))
        target_squared_distance = int(expected["target_squared_distance"])

        for endpoint_id, counterpart_id in ((left_id, right_id), (right_id, left_id)):
            neighbor_order = sorted(
                (
                    (_squared_distance(points[endpoint_id], point), point_id)
                    for point_id, point in enumerate(points)
                    if point_id != endpoint_id
                )
            )
            closer_count = sum(
                squared_distance < target_squared_distance
                for squared_distance, _ in neighbor_order
            )
            self.assertEqual(
                closer_count,
                expected["strictly_closer_points_per_endpoint"],
            )
            selected_ids = {
                point_id
                for _, point_id in neighbor_order[
                    : expected["absent_from_directed_lnn_limit"]
                ]
            }
            self.assertNotIn(counterpart_id, selected_ids)

        filtration = build_gamma_filtration(points, 1)
        edge_by_pair = {
            edge.simplex_point_ids: edge for edge in filtration.gabriel_hyperedges
        }
        self.assertIn(target_pair, edge_by_pair)
        self.assertEqual(
            edge_by_pair[target_pair].squared_level,
            _fraction(expected["target_squared_level"]),
        )
        center = tuple(
            Fraction(points[left_id][axis] + points[right_id][axis], 2)
            for axis in range(3)
        )
        target_squared_level = _fraction(expected["target_squared_level"])
        for point_id, point in enumerate(points):
            if point_id in target_pair:
                continue
            squared_radius = sum(
                (Fraction(coordinate) - center[axis]) ** 2
                for axis, coordinate in enumerate(point)
            )
            self.assertGreater(squared_radius, target_squared_level)

    def test_one_ulp_coplanar_and_cospherical_boundaries(self) -> None:
        fixture = _load_fixture("degeneracies/one_ulp_boundaries.json")
        coplanar = fixture["quasi_coplanar"]
        cospherical = fixture["quasi_cospherical"]
        assert isinstance(coplanar, dict)
        assert isinstance(cospherical, dict)
        hash_payload = {
            "quasi_coplanar": {
                "fixed_points": coplanar["fixed_points"],
                "moving_xy": coplanar["moving_xy"],
                "variant_z": [variant["z"] for variant in coplanar["variants"]],
            },
            "quasi_cospherical": {
                "support_points": cospherical["support_points"],
                "moving_xy": cospherical["moving_xy"],
                "variant_z": [variant["z"] for variant in cospherical["variants"]],
            },
        }
        self.assertEqual(
            _compact_sha256(hash_payload, sort_keys=True),
            fixture["coordinate_hash_sha256"],
        )

        fixed_points = [
            tuple(_number(value) for value in point)
            for point in coplanar["fixed_points"]
        ]
        moving_xy = tuple(_number(value) for value in coplanar["moving_xy"])
        for variant in coplanar["variants"]:
            moving_point = moving_xy + (_number(variant["z"]),)
            self.assertEqual(
                affine_dimension(fixed_points + [moving_point]),
                variant["expected_affine_dimension"],
            )
        coplanar_z = [
            _number(variant["z"]) for variant in coplanar["variants"]
        ]
        self.assertEqual(coplanar_z[0], math.nextafter(1.0, 0.0))
        self.assertEqual(coplanar_z[1], 1.0)
        self.assertEqual(coplanar_z[2], math.nextafter(1.0, math.inf))

        support_points = [
            tuple(_number(value) for value in point)
            for point in cospherical["support_points"]
        ]
        ball = circumball(support_points, range(4))
        self.assertEqual(
            ball.center,
            tuple(_fraction(value) for value in cospherical["expected_center"]),
        )
        self.assertEqual(
            ball.squared_radius,
            _fraction(cospherical["expected_squared_radius"]),
        )
        expected_relations = {
            "interior": BallRelation.INTERIOR,
            "shell": BallRelation.SHELL,
            "exterior": BallRelation.EXTERIOR,
        }
        moving_xy = tuple(_number(value) for value in cospherical["moving_xy"])
        for variant in cospherical["variants"]:
            moving_point = moving_xy + (_number(variant["z"]),)
            self.assertIs(
                classify(moving_point, ball),
                expected_relations[variant["expected_relation"]],
            )
        cospherical_z = [
            _number(variant["z"]) for variant in cospherical["variants"]
        ]
        self.assertEqual(cospherical_z[0], math.nextafter(-1.0, 0.0))
        self.assertEqual(cospherical_z[1], -1.0)
        self.assertEqual(cospherical_z[2], math.nextafter(-1.0, -math.inf))

    def test_selected_n12_n14_catalogs_and_declared_scope(self) -> None:
        fixture = _load_fixture("oracle/selected_n12_n14.json")
        generator = fixture["generator"]
        nightly_scope = fixture["nightly_scope"]
        assert isinstance(generator, dict)
        assert isinstance(nightly_scope, dict)
        self.assertEqual(nightly_scope["n12_orders"], list(range(1, 11)))
        self.assertEqual(nightly_scope["n14_orders"], list(range(1, 11)))

        for case in fixture["cases"]:
            with self.subTest(case=case["name"]):
                point_count = case["point_count"]
                points = generate_affine_cloud(
                    generator["parameters"]["affine_dimension"],
                    point_count,
                    generator["seed"],
                    coordinate_bound=generator["parameters"]["coordinate_bound"],
                )
                self.assertEqual(
                    _compact_sha256(sorted(map(list, points))),
                    case["coordinate_hash_sha256"],
                )
                catalog = build_critical_catalog(points, fixture["k_max"])
                expected_catalog = case["expected_catalog"]
                self.assertEqual(len(catalog.events), expected_catalog["event_count"])
                self.assertEqual(catalog.s_max, expected_catalog["s_max"])
                self.assertIs(
                    catalog.relevant_gp_complete,
                    expected_catalog["relevant_gp_complete"],
                )
                self.assertEqual(
                    [cost["order"] for cost in case["order_costs"]],
                    list(range(1, 11)),
                )
                for cost in case["order_costs"]:
                    order = cost["order"]
                    self.assertEqual(cost["facet_count"], math.comb(point_count, order))
                    self.assertEqual(
                        cost["coface_count"],
                        math.comb(point_count, order + 1),
                    )

    def test_minimum_fixture_matrix_resolves_to_tests(self) -> None:
        matrix = _load_fixture("oracle/phase1_fixture_matrix.json")
        requirements = matrix["requirements"]
        requirement_ids = [requirement["id"] for requirement in requirements]
        self.assertEqual(len(requirement_ids), len(set(requirement_ids)))
        self.assertEqual(
            set(requirement_ids),
            {
                "chapter6_six_points",
                "well_centered_support_2",
                "well_centered_support_3",
                "well_centered_support_4",
                "obtuse_triangle",
                "non_well_centered_tetrahedron",
                "isolated_facet_absorbed_later",
                "two_same_level_fusions",
                "multifusion_one_center",
                "overlapping_k_polyhedra",
                "gabriel_pair_missing_small_lnn",
                "gabriel_point_set_counterexample",
                "morse_rank_window",
                "gamma_q1_coverage_delta",
                "vertical_q1_growth_target",
                "one_ulp_quasi_coplanar",
                "one_ulp_quasi_cospherical",
                "selected_n12",
                "selected_n14",
            },
        )
        for requirement in requirements:
            with self.subTest(requirement=requirement["id"]):
                fixture_path = requirement["fixture"]
                if fixture_path is not None:
                    self.assertTrue((REPOSITORY_ROOT / fixture_path).is_file())
                module_name, class_name, method_name = requirement["test"].rsplit(
                    ".", 2
                )
                test_class = getattr(importlib.import_module(module_name), class_name)
                self.assertTrue(callable(getattr(test_class, method_name)))


if __name__ == "__main__":
    unittest.main()
