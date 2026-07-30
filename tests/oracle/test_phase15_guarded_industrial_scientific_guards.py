from __future__ import annotations

import copy
import hashlib
import json
import struct
import subprocess
import tempfile
import unittest
from itertools import combinations
from pathlib import Path
from unittest import mock

import numpy as np

import tools.check_phase15_guarded_industrial_scientific_guards as checker


ROOT = Path(__file__).resolve().parents[2]
GIT_SHA = subprocess.check_output(
    ["git", "-C", str(ROOT), "rev-parse", "HEAD"], text=True
).strip()
RUNNER_BINARY_BYTES = b"fixture runner binary"
RUNNER_BINARY_SHA256 = hashlib.sha256(RUNNER_BINARY_BYTES).hexdigest()
CUDA_BANNER = """\
==========
== CUDA ==
==========
CUDA Version 12.9.0
Container image Copyright (c) 2016-2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

This container image and its contents are governed by the NVIDIA Deep Learning Container License.
By pulling and using the container, you accept the terms and conditions of this license:
https://developer.nvidia.com/ngc/nvidia-deep-learning-container-license
A copy of this license is made available in this container at /NGC-DL-CONTAINER-LICENSE for your convenience.

"""


def _tree(order: int, source: checker.ExportedTree) -> checker.ExportedTree:
    return checker.ExportedTree(
        order,
        source.first,
        source.second,
        source.squared_levels,
        source.squared_level_bits,
    )


def _write_export(
    path: Path,
    point_count: int,
    seed: int,
    trees: list[checker.ExportedTree],
) -> None:
    with path.open("wb") as output:
        output.write(checker.EXPORT_MAGIC)
        output.write(
            struct.pack(
                "<IIQQQ", 1, checker.ENDIAN_MARKER, point_count, seed, len(trees)
            )
        )
        for tree in trees:
            output.write(checker.TREE_MAGIC)
            output.write(
                struct.pack(
                    "<IIQQQQ",
                    1,
                    0,
                    point_count,
                    tree.order,
                    seed,
                    len(tree.first),
                )
            )
            for left, right, bits in zip(
                tree.first, tree.second, tree.squared_level_bits
            ):
                output.write(struct.pack("<QQQ", int(left), int(right), int(bits)))


def _complete_graph_trees(
    points: np.ndarray,
) -> tuple[checker.ExportedTree, checker.ExportedTree]:
    edges = np.asarray(list(combinations(range(len(points)), 2)), dtype=np.uint64)
    distances = checker.squared_distances(
        points, edges[:, 0].astype(np.int64), edges[:, 1].astype(np.int64)
    )
    k1 = _tree(1, checker.canonical_kruskal(len(points), edges, distances))
    distance_matrix = np.full((len(points), len(points)), np.inf)
    distance_matrix[edges[:, 0], edges[:, 1]] = distances
    distance_matrix[edges[:, 1], edges[:, 0]] = distances
    core_two = np.partition(distance_matrix, 1, axis=1)[:, 1]
    mutual = np.maximum(
        distances,
        np.maximum(core_two[edges[:, 0]], core_two[edges[:, 1]]),
    )
    k2 = _tree(2, checker.canonical_kruskal(len(points), edges, mutual))
    return k1, k2


def _star_trees(point_count: int = 5) -> list[checker.ExportedTree]:
    first = np.zeros(point_count - 1, dtype=np.uint64)
    second = np.arange(1, point_count, dtype=np.uint64)
    levels = np.arange(1, point_count, dtype=np.float64)
    return [
        checker.ExportedTree(
            order,
            first.copy(),
            second.copy(),
            levels.copy(),
            levels.view(np.uint64).copy(),
        )
        for order in (1, 2)
    ]


def _runner_report(
    *,
    family: str,
    point_count: int,
    seed: int,
    trees: list[checker.ExportedTree],
    git_sha: str = GIT_SHA,
    runner_binary_sha256: str = RUNNER_BINARY_SHA256,
    runner_binary_size_bytes: int = len(RUNNER_BINARY_BYTES),
) -> dict[str, object]:
    return {
        "schema": checker.RUNNER_SCHEMA,
        "git_sha": git_sha,
        "runner_binary_sha256": runner_binary_sha256,
        "runner_binary_size_bytes": runner_binary_size_bytes,
        "family": family,
        "point_count": point_count,
        "maximum_order": len(trees),
        "runs": [
            {
                "run_index": 0,
                "measured_index": 0,
                "warmup": False,
                "seed": seed,
                "oracle_export_ns": 1,
                "oracle_export_path": "/runner/export.bin",
                "hierarchies": [
                    {
                        "order": tree.order,
                        "merge_record_count": point_count - 1,
                        "digest": checker.runner_hierarchy_digest(tree),
                        "root_squared_level": float(tree.squared_levels[-1]),
                    }
                    for tree in trees
                ],
            }
        ],
    }


def _runner_report_v5(
    *,
    family: str,
    point_count: int,
    seed: int,
    trees: list[checker.ExportedTree],
    git_sha: str = GIT_SHA,
    runner_binary_sha256: str = RUNNER_BINARY_SHA256,
    runner_binary_size_bytes: int = len(RUNNER_BINARY_BYTES),
) -> dict[str, object]:
    metadata_trees = [
        *trees,
        *[_tree(order, trees[-1]) for order in range(len(trees) + 1, 11)],
    ]
    value = _runner_report(
        family=family,
        point_count=point_count,
        seed=seed,
        trees=metadata_trees,
        git_sha=git_sha,
        runner_binary_sha256=runner_binary_sha256,
        runner_binary_size_bytes=runner_binary_size_bytes,
    )
    value.update(
        {
            "schema": checker.RUNNER_SCHEMA_V5,
            "maximum_order": 10,
            "export_maximum_order": len(trees),
            "mode": (
                "warm_fresh_cloud_lbvh_top10_capped_"
                "component_bridges_parallel_h0"
            ),
            "component_bridge_policy": (
                "top10_components_capped_centroid_knn_top8_"
                "projection_cross_min"
            ),
            "component_bridge_maximum_component_count": 256,
            "component_bridge_maximum_centroid_distance_evaluation_count": 65_280,
            "component_bridge_member_projection_budget_factor": 16,
            "component_bridge_maximum_pair_count": 1_536,
            "component_bridge_maximum_cross_distance_evaluation_count": 98_304,
        }
    )
    run = value["runs"][0]
    run.update(
        {
            "top_k_component_count": 1,
            "component_bridge_pair_count": 0,
            "component_bridge_support_evaluation_count": 0,
            "component_bridge_centroid_distance_evaluation_count": 0,
            "component_bridge_member_projection_evaluation_budget": 16
            * point_count,
            "component_bridge_member_projection_evaluation_count": 0,
            "component_bridge_cross_distance_evaluation_budget": 98_304,
            "component_bridge_cross_distance_evaluation_count": 0,
            "component_bridge_budget_satisfied": True,
            "component_bridge_budget_stop_reason": "none",
            "materialized_merge_record_count": 10 * (point_count - 1),
            "released_merge_record_count": 10 * (point_count - 1),
            "retained_merge_record_count": 0,
            "merge_records_released_after_digest_and_export": True,
            "exported_order_count": len(trees),
            "exported_merge_record_count": len(trees) * (point_count - 1),
        }
    )
    return value


def _write_runner_report(
    path: Path, report: dict[str, object], *, cuda_banner: bool = False
) -> None:
    prefix = CUDA_BANNER if cuda_banner else ""
    path.write_text(
        prefix + json.dumps(report, sort_keys=True, separators=(",", ":")) + "\n",
        encoding="utf-8",
    )


class GuardedIndustrialScientificGuardsTests(unittest.TestCase):
    def test_binary_export_is_fail_closed(self) -> None:
        levels = np.asarray([1.0, 2.0, 3.0, 4.0], dtype=np.float64)
        first = np.asarray([0, 0, 0, 0], dtype=np.uint64)
        second = np.asarray([1, 2, 3, 4], dtype=np.uint64)
        k1 = checker.ExportedTree(1, first, second, levels, levels.view(np.uint64))
        k2 = checker.ExportedTree(2, first, second, levels, levels.view(np.uint64))
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trees.bin"
            _write_export(path, 5, 7, [k1, k2])
            parsed = checker.read_industrial_export(
                path, expected_point_count=5, expected_tree_count=2
            )
            self.assertEqual(parsed.seed, 7)
            self.assertEqual(set(parsed.trees), {1, 2})
            self.assertEqual(
                parsed.artifact_sha256, hashlib.sha256(path.read_bytes()).hexdigest()
            )
            path.write_bytes(path.read_bytes() + b"x")
            with self.assertRaisesRegex(checker.ScientificGuardError, "trailing"):
                checker.read_industrial_export(
                    path, expected_point_count=5, expected_tree_count=2
                )

    def test_cloud_reconstruction_is_canonical_and_deterministic(self) -> None:
        expected_first_rows = {
            "affine_uniform_binary64": [
                4607182418800017408,
                4610731455014650155,
                4607203797096847135,
            ],
            "jittered_dyadic_grid3d": [
                4608308318677499904,
                4609434218575953920,
                4609434218567565312,
            ],
            "balanced_multiscale_clusters": [
                4607463783091547673,
                4608026589857909762,
                4607464125355339700,
            ],
        }
        for family, expected in expected_first_rows.items():
            with self.subTest(family=family):
                points, bits = checker.reconstruct_canonical_cloud(family, 17, 123)
                self.assertEqual(bits[0].tolist(), expected)
                self.assertTrue(np.all(bits[:-1, 0] <= bits[1:, 0]))
                self.assertEqual(len(np.unique(bits, axis=0)), 17)
                self.assertTrue(np.array_equal(points.view(np.uint64), bits))

    def test_owned_wedges_are_unique(self) -> None:
        edges = np.asarray(list(combinations(range(4), 2)), dtype=np.uint64)
        graph, batches = checker.iter_owned_two_edge_triangles(
            4, edges, batch_size=2
        )
        triangles = np.concatenate(list(batches), axis=0)
        self.assertEqual(graph.raw_wedge_count, 12)
        self.assertEqual(
            {tuple(row) for row in triangles},
            set(combinations(range(4), 3)),
        )

    def test_delaunay_fallback_uses_exact_translation_without_joggle(self) -> None:
        class FixtureQhullError(Exception):
            pass

        points = np.asarray(
            [
                [1.0, 1.0, 1.0],
                [1.5, 1.0, 1.0],
                [1.0, 1.5, 1.0],
                [1.0, 1.0, 1.5],
            ],
            dtype=np.float64,
        )
        calls: list[tuple[np.ndarray, tuple[object, ...], dict[str, object]]] = []

        def delaunay(
            coordinates: np.ndarray, *args: object, **kwargs: object
        ) -> object:
            calls.append((coordinates.copy(), args, kwargs))
            if len(calls) == 1:
                raise FixtureQhullError("primary fixture failure")
            return type(
                "FixtureTriangulation",
                (),
                {"simplices": np.asarray([[0, 1, 2, 3]], dtype=np.int64)},
            )()

        with mock.patch.object(
            checker,
            "_load_spatial",
            return_value=(delaunay, object(), FixtureQhullError),
        ):
            oracle = checker.build_delaunay_oracle(points)

        self.assertEqual(len(calls), 2)
        self.assertTrue(np.array_equal(calls[0][0], points))
        self.assertTrue(np.array_equal(calls[1][0], points - 1.0))
        self.assertEqual(calls[0][1:], ((), {}))
        self.assertEqual(calls[1][1:], ((), {}))
        self.assertTrue(oracle.exact_translation_used)
        self.assertFalse(oracle.deterministic_joggle_used)
        self.assertEqual(oracle.primary_failure, "primary fixture failure")

    def test_delaunay_double_failure_is_fail_closed(self) -> None:
        class FixtureQhullError(Exception):
            pass

        points = np.asarray(
            [
                [1.0, 1.0, 1.0],
                [1.5, 1.0, 1.0],
                [1.0, 1.5, 1.0],
                [1.0, 1.0, 1.5],
            ],
            dtype=np.float64,
        )
        calls: list[tuple[np.ndarray, tuple[object, ...], dict[str, object]]] = []

        def delaunay(
            coordinates: np.ndarray, *args: object, **kwargs: object
        ) -> object:
            calls.append((coordinates.copy(), args, kwargs))
            raise FixtureQhullError(f"fixture failure {len(calls)}")

        with mock.patch.object(
            checker,
            "_load_spatial",
            return_value=(delaunay, object(), FixtureQhullError),
        ):
            with self.assertRaisesRegex(
                checker.ScientificGuardError,
                "both the original input and its exact subtract-one translation",
            ):
                checker.build_delaunay_oracle(points)

        self.assertEqual(len(calls), 2)
        self.assertTrue(np.array_equal(calls[1][0], points - 1.0))
        self.assertTrue(all(args == () and kwargs == {} for _, args, kwargs in calls))

    def test_k1_deadline_guard_accepts_an_alternate_tied_emst(self) -> None:
        points = np.asarray(
            [
                [1.0, 1.0, 1.0],
                [1.0, -1.0, -1.0],
                [-1.0, 1.0, -1.0],
                [-1.0, -1.0, 1.0],
            ]
        )
        edges = np.asarray(list(combinations(range(4), 2)), dtype=np.uint64)
        oracle = checker.DelaunayOracle(
            simplex_count=1,
            edges=edges,
            qhull_options="fixture",
            exact_translation_used=False,
            deterministic_joggle_used=False,
            primary_failure=None,
        )
        first = np.asarray([0, 1, 2], dtype=np.uint64)
        second = np.asarray([1, 2, 3], dtype=np.uint64)
        levels = checker.squared_distances(
            points, first.astype(np.int64), second.astype(np.int64)
        )
        candidate = checker.ExportedTree(
            1, first, second, levels, levels.view(np.uint64)
        )
        report = checker.evaluate_k1(points, oracle, candidate)
        self.assertFalse(report["canonical_edge_identity_match"])
        self.assertEqual(report["late_count"], 0)
        self.assertTrue(report["guard_passed"])

    def test_k2_uses_exact_divide_by_four_and_closed_plateau(self) -> None:
        first = np.asarray([0, 1, 2], dtype=np.uint64)
        second = np.asarray([1, 2, 3], dtype=np.uint64)
        levels = np.asarray([4.0, 4.0, 4.0], dtype=np.float64)
        tree = checker.ExportedTree(2, first, second, levels, levels.view(np.uint64))
        paths = checker.WeightedTreePaths(4, tree)
        triangles = np.asarray([[0, 1, 2]], dtype=np.int64)
        partition, _ = checker._deadline_partition(
            triangles, np.asarray([1.0]), paths
        )
        self.assertEqual(partition["connected_at_count"], 1)
        self.assertEqual(partition["late_count"], 0)
        self.assertEqual(partition["maximum_connection_to_deadline_ratio"], 1.0)

        late_levels = np.nextafter(levels, np.inf)
        late_tree = checker.ExportedTree(
            2, first, second, late_levels, late_levels.view(np.uint64)
        )
        late, _ = checker._deadline_partition(
            triangles,
            np.asarray([1.0]),
            checker.WeightedTreePaths(4, late_tree),
        )
        self.assertEqual(late["late_count"], 1)

    def test_adaptive_k2_rejects_overrounded_valid_miniball_radius(self) -> None:
        # This acute, non-degenerate triangle has a valid binary64 miniball,
        # but that recipe rounds its radius upward.  Using the rounded radius
        # as the deadline therefore accepts an apparent plateau that is
        # strictly late against the exact dyadic triangle miniball.
        point_bits = np.asarray(
            [
                [0x3FFBBE2B7531DB10, 0x3FF385AC3B2C6370, 0x3FF14E361B7B7090],
                [0x3FF28EEEB09643F6, 0x3FF5710CE43BE778, 0x3FF7716E60579316],
                [0x3FF44342B2379BDC, 0x3FFD0D6B8ED8EDC4, 0x3FF317BBDB87FA8F],
            ],
            dtype=np.uint64,
        )
        points = point_bits.view(np.float64).reshape(3, 3)
        triangles = np.asarray([[0, 1, 2]], dtype=np.int64)
        balls = checker.triangle_miniballs(points, triangles)
        self.assertTrue(balls.valid[0])
        self.assertEqual(int(balls.support_cardinalities[0]), 3)
        exact_ball = checker.exact_triangle_miniball(points, triangles[0])
        self.assertGreater(
            checker.Fraction.from_float(float(balls.squared_radii[0])),
            exact_ball.squared_radius,
        )
        longest_lower = checker._triangle_longest_edge_squared_lower_bounds(
            points, triangles
        )[0]

        def exact_squared_distance(left: int, right: int) -> object:
            differences = (
                checker.Fraction.from_float(float(points[left, axis]))
                - checker.Fraction.from_float(float(points[right, axis]))
                for axis in range(3)
            )
            return sum((value * value for value in differences), checker.Fraction(0))

        exact_longest = max(
            exact_squared_distance(0, 1),
            exact_squared_distance(0, 2),
            exact_squared_distance(1, 2),
        )
        self.assertLessEqual(
            checker.Fraction.from_float(float(longest_lower)), exact_longest
        )

        early_levels = np.zeros(2, dtype=np.float64)
        early_tree = checker.ExportedTree(
            2,
            np.asarray([0, 1], dtype=np.uint64),
            np.asarray([1, 2], dtype=np.uint64),
            early_levels,
            early_levels.view(np.uint64),
        )
        early_partition, _ = checker._deadline_partition(
            triangles,
            balls.squared_radii,
            checker.WeightedTreePaths(3, early_tree),
            balls.support_cardinalities,
            points=points,
        )
        self.assertEqual(early_partition["certified_float_before_source_count"], 1)
        self.assertEqual(early_partition["exact_fraction_decision_source_count"], 0)

        raw_level = np.ldexp(balls.squared_radii[0], 2)
        levels = np.asarray([raw_level, raw_level], dtype=np.float64)
        tree = checker.ExportedTree(
            2,
            np.asarray([0, 1], dtype=np.uint64),
            np.asarray([1, 2], dtype=np.uint64),
            levels,
            levels.view(np.uint64),
        )
        paths = checker.WeightedTreePaths(3, tree)
        rounded_partition, _ = checker._deadline_partition(
            triangles,
            balls.squared_radii,
            paths,
            balls.support_cardinalities,
        )
        self.assertEqual(rounded_partition["connected_at_count"], 1)
        self.assertEqual(rounded_partition["late_count"], 0)

        exact_partition, late = checker._deadline_partition(
            triangles,
            balls.squared_radii,
            paths,
            balls.support_cardinalities,
            points=points,
        )
        self.assertEqual(exact_partition["certified_float_before_source_count"], 0)
        self.assertEqual(exact_partition["exact_fraction_decision_source_count"], 1)
        self.assertTrue(exact_partition["adaptive_decision_partition_closed"])
        self.assertEqual(exact_partition["connected_at_count"], 0)
        self.assertEqual(exact_partition["late_count"], 1)
        self.assertEqual(
            exact_partition["late_with_zero_binary64_recipe_ulp_count"], 1
        )
        self.assertTrue(late[0])
        self.assertEqual(
            exact_partition["first_late_witness"]["decision_certification"],
            "exact_dyadic_Fraction_triangle_miniball",
        )

    def test_triangle_miniball_matches_right_triangle_recipe(self) -> None:
        points = np.asarray(
            [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]]
        )
        balls = checker.triangle_miniballs(
            points, np.asarray([[0, 1, 2]], dtype=np.int64)
        )
        self.assertTrue(balls.valid[0])
        self.assertTrue(balls.ambiguous[0])
        self.assertEqual(int(balls.support_cardinalities[0]), 2)
        self.assertEqual(float(balls.squared_radii[0]), 0.5)

    def test_permanent_affine_two_ulp_deadline_regression(self) -> None:
        fixture = json.loads(
            (
                ROOT
                / "tests"
                / "fixtures"
                / "regressions"
                / "phase15_affine_50k_k2_two_ulp_deadline.json"
            ).read_text(encoding="utf-8")
        )
        point_bits = np.asarray(
            [[int(word, 16) for word in row] for row in fixture["point_binary64_bits"]],
            dtype=np.uint64,
        )
        points = point_bits.view(np.float64).reshape(3, 3)
        triangles = np.asarray([[0, 1, 2]], dtype=np.int64)
        balls = checker.triangle_miniballs(points, triangles)
        radius_bits = int(balls.squared_radii.view(np.uint64)[0])
        self.assertEqual(
            radius_bits, int(fixture["source_squared_radius_bits"], 16)
        )
        self.assertEqual(
            int(balls.support_cardinalities[0]), fixture["support_cardinality"]
        )
        low = checker.squared_distances(
            points, np.asarray([0]), np.asarray([1])
        )[0]
        raw_bits = int(fixture["raw_connection_squared_weight_bits"], 16)
        raw = np.asarray([raw_bits], dtype=np.uint64).view(np.float64)[0]
        levels = np.asarray([low, raw], dtype=np.float64)
        tree = checker.ExportedTree(
            2,
            np.asarray([0, 0], dtype=np.uint64),
            np.asarray([1, 2], dtype=np.uint64),
            levels,
            levels.view(np.uint64),
        )
        partition, _ = checker._deadline_partition(
            triangles,
            balls.squared_radii,
            checker.WeightedTreePaths(3, tree),
            balls.support_cardinalities,
        )
        self.assertEqual(partition["late_count"], 1)
        self.assertEqual(
            partition["maximum_lateness_ulps"], fixture["lateness_ulps"]
        )
        self.assertEqual(partition["late_support_two_count"], 1)

    def test_permanent_balanced_top1_projection_bridge_regression(self) -> None:
        fixture = json.loads(
            (
                ROOT
                / "tests"
                / "fixtures"
                / "regressions"
                / "phase15_balanced_50k_projection_top1_emst_deadlines.json"
            ).read_text(encoding="utf-8")
        )
        direction = np.asarray(
            [int(word, 16) for word in fixture["centroid_direction_bits"]],
            dtype=np.uint64,
        ).view(np.float64)

        def decode_supports(
            records: list[dict[str, object]],
        ) -> list[tuple[int, np.ndarray]]:
            return [
                (
                    int(record["point_id"]),
                    np.asarray(
                        [int(word, 16) for word in record["coordinate_bits"]],
                        dtype=np.uint64,
                    ).view(np.float64),
                )
                for record in records
            ]

        left_supports = decode_supports(fixture["left_top8_projected_supports"])
        right_supports = decode_supports(
            fixture["right_top8_projected_supports"]
        )

        def projection(support: tuple[int, np.ndarray]) -> float:
            coordinate = support[1]
            return float(
                (coordinate[0] * direction[0] + coordinate[1] * direction[1])
                + coordinate[2] * direction[2]
            )

        self.assertEqual(
            [support[0] for support in left_supports],
            [
                support[0]
                for support in sorted(
                    left_supports,
                    key=lambda value: (-projection(value), value[0]),
                )
            ],
        )
        self.assertEqual(
            [support[0] for support in right_supports],
            [
                support[0]
                for support in sorted(
                    right_supports,
                    key=lambda value: (projection(value), value[0]),
                )
            ],
        )

        exchange = fixture["emst_exchange_edge"]
        exchange_ids = [int(exchange["u"]), int(exchange["v"])]
        self.assertEqual(
            [
                next(
                    index + 1
                    for index, support in enumerate(left_supports)
                    if support[0] == exchange_ids[0]
                ),
                next(
                    index + 1
                    for index, support in enumerate(right_supports)
                    if support[0] == exchange_ids[1]
                ),
            ],
            fixture["emst_endpoint_projection_ranks"],
        )
        top1 = fixture["top1_projection_edge"]
        self.assertEqual(
            [left_supports[0][0], right_supports[0][0]],
            [top1["u"], top1["v"]],
        )

        def squared_distance(
            first: tuple[int, np.ndarray], second: tuple[int, np.ndarray]
        ) -> float:
            delta = first[1] - second[1]
            return float(
                (delta[0] * delta[0] + delta[1] * delta[1])
                + delta[2] * delta[2]
            )

        candidates = [
            (squared_distance(first, second), first[0], second[0])
            for first in left_supports
            for second in right_supports
        ]
        selected_squared, selected_first, selected_second = min(candidates)
        top8 = fixture["top8_projection_cross_min_edge"]
        self.assertEqual(
            [selected_first, selected_second], [top8["u"], top8["v"]]
        )
        self.assertEqual(
            int(np.float64(selected_squared).view(np.uint64)),
            int(top8["squared_distance_bits"], 16),
        )
        self.assertEqual(
            [selected_first, selected_second], [exchange["u"], exchange["v"]]
        )
        self.assertEqual(
            int(
                np.float64(
                    squared_distance(left_supports[0], right_supports[0])
                ).view(np.uint64)
            ),
            int(top1["squared_distance_bits"], 16),
        )
        self.assertLess(selected_squared, float(top1["squared_distance"]))
        self.assertGreater(
            int(fixture["candidate_connection_squared_level_bits"], 16),
            int(exchange["squared_distance_bits"], 16),
        )
        self.assertEqual(fixture["observed_k1_late_count"], 37)
        self.assertEqual(fixture["top8_k1_late_count"], 0)

    def test_permanent_balanced_top10_emst_bridge_regression(self) -> None:
        fixture = json.loads(
            (
                ROOT
                / "tests"
                / "fixtures"
                / "regressions"
                / "phase15_balanced_50k_top10_emst_bridge.json"
            ).read_text(encoding="utf-8")
        )
        direct = fixture["direct_exchange_edge"]
        bottleneck = fixture["candidate_tree_path"]["bottleneck"]
        legacy_bits = fixture["legacy_point_binary64_bits"]

        def point(point_id: int) -> np.ndarray:
            words = np.asarray(
                [int(word, 16) for word in legacy_bits[str(point_id)]],
                dtype=np.uint64,
            )
            return words.view(np.float64)

        def distance_bits(edge: dict[str, object]) -> int:
            edge_points = np.asarray(
                [point(int(edge["u"])), point(int(edge["v"]))],
                dtype=np.float64,
            )
            squared = checker.squared_distances(
                edge_points,
                np.asarray([0], dtype=np.int64),
                np.asarray([1], dtype=np.int64),
            )[0]
            return int(np.float64(squared).view(np.uint64))

        direct_bits = distance_bits(direct)
        bottleneck_bits = distance_bits(bottleneck)
        self.assertEqual(direct_bits, int(direct["squared_distance_bits"], 16))
        self.assertEqual(
            bottleneck_bits, int(bottleneck["squared_distance_bits"], 16)
        )
        self.assertLess(direct_bits, bottleneck_bits)
        direct_level = np.asarray([direct_bits], dtype=np.uint64).view(np.float64)[0]
        bottleneck_level = np.asarray(
            [bottleneck_bits], dtype=np.uint64
        ).view(np.float64)[0]
        self.assertEqual(
            bottleneck_level / direct_level,
            fixture["bottleneck_to_direct_ratio"],
        )
        self.assertEqual(fixture["candidate_tree_path"]["edge_count"], 2050)

        recertification = fixture["binary64_miniball_recertification_witness"]
        legacy_triangle_points = np.asarray(
            [point(point_id) for point_id in recertification["point_ids"]],
            dtype=np.float64,
        )
        invalid_triangle = np.asarray([[0, 1, 2]], dtype=np.int64)
        binary64_ball = checker.triangle_miniballs(
            legacy_triangle_points, invalid_triangle
        )
        self.assertFalse(binary64_ball.valid[0])
        exact_ball = checker.exact_triangle_miniball(
            legacy_triangle_points, invalid_triangle[0]
        )
        self.assertEqual(
            checker._fraction_text(exact_ball.squared_radius),
            recertification["exact_squared_radius"],
        )
        self.assertEqual(
            exact_ball.support_cardinality,
            recertification["support_cardinality"],
        )

    def test_v5_runner_binding_accepts_partial_order12_export_and_ten_metadata(
        self,
    ) -> None:
        family = "affine_uniform_binary64"
        point_count = 5
        seed = 7
        trees = _star_trees(point_count)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            export = root / "trees.bin"
            runner_report = root / "runner.json"
            runner_binary = root / "runner"
            _write_export(export, point_count, seed, trees)
            runner_binary.write_bytes(RUNNER_BINARY_BYTES)
            parsed = checker.read_industrial_export(
                export,
                expected_point_count=point_count,
                expected_tree_count=2,
            )
            valid = _runner_report_v5(
                family=family,
                point_count=point_count,
                seed=seed,
                trees=trees,
            )
            _write_runner_report(runner_report, valid)
            binding = checker._bind_runner_report(
                parsed,
                runner_report_path=runner_report,
                family=family,
                expected_git_sha=GIT_SHA,
                runner_binary_path=runner_binary,
                expected_runner_binary_sha256=RUNNER_BINARY_SHA256,
            )
            self.assertEqual(binding["runner_schema"], checker.RUNNER_SCHEMA_V5)
            self.assertEqual(binding["reported_hierarchy_metadata_count"], 10)
            self.assertEqual(binding["exported_hierarchy_count"], 2)
            self.assertEqual([item["order"] for item in binding["hierarchies"]], [1, 2])

            mutations = (
                ("export order", ("report", "export_maximum_order", 3), "export_maximum_order"),
                ("bounded mode", ("report", "mode", "forged"), "mode"),
                (
                    "bounded policy",
                    ("report", "component_bridge_policy", "forged"),
                    "component_bridge_policy",
                ),
                (
                    "top component cap",
                    ("run", "top_k_component_count", 257),
                    "1..256",
                ),
                (
                    "centroid count",
                    (
                        "run",
                        "component_bridge_centroid_distance_evaluation_count",
                        1,
                    ),
                    r"C\(C-1\)",
                ),
                (
                    "projection budget",
                    (
                        "run",
                        "component_bridge_member_projection_evaluation_budget",
                        79,
                    ),
                    "16n",
                ),
                (
                    "cross budget",
                    (
                        "run",
                        "component_bridge_cross_distance_evaluation_budget",
                        98_303,
                    ),
                    r"64\*1536",
                ),
                (
                    "pair cap",
                    ("run", "component_bridge_pair_count", 1_537),
                    "1536",
                ),
                (
                    "support split",
                    ("run", "component_bridge_support_evaluation_count", 1),
                    "projection plus cross",
                ),
                (
                    "budget result",
                    ("run", "component_bridge_budget_satisfied", False),
                    "satisfy the component bridge budget",
                ),
                (
                    "materialized count",
                    ("run", "materialized_merge_record_count", 39),
                    "materialized_merge_record_count",
                ),
                (
                    "retained count",
                    ("run", "retained_merge_record_count", 1),
                    "retained_merge_record_count",
                ),
                (
                    "exported record count",
                    ("run", "exported_merge_record_count", 7),
                    "exported_merge_record_count",
                ),
                (
                    "release claim",
                    (
                        "run",
                        "merge_records_released_after_digest_and_export",
                        False,
                    ),
                    "did not release",
                ),
                (
                    "unexported metadata digest",
                    ("hierarchy", 2, "digest", "forged"),
                    "digest is not canonical",
                ),
            )
            for label, mutation, message in mutations:
                with self.subTest(label=label):
                    changed = copy.deepcopy(valid)
                    if mutation[0] == "report":
                        changed[mutation[1]] = mutation[2]
                    elif mutation[0] == "run":
                        changed["runs"][0][mutation[1]] = mutation[2]
                    else:
                        changed["runs"][0]["hierarchies"][mutation[1]][
                            mutation[2]
                        ] = mutation[3]
                    _write_runner_report(runner_report, changed)
                    with self.assertRaisesRegex(
                        checker.ScientificGuardError, message
                    ):
                        checker._bind_runner_report(
                            parsed,
                            runner_report_path=runner_report,
                            family=family,
                            expected_git_sha=GIT_SHA,
                            runner_binary_path=runner_binary,
                            expected_runner_binary_sha256=RUNNER_BINARY_SHA256,
                        )

    def test_runner_binding_rejects_forged_report_digest_git_and_binary_sha(
        self,
    ) -> None:
        family = "affine_uniform_binary64"
        point_count = 5
        seed = 7
        trees = _star_trees(point_count)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            export = root / "trees.bin"
            runner_report = root / "runner.json"
            runner_binary = root / "runner"
            _write_export(export, point_count, seed, trees)
            runner_binary.write_bytes(RUNNER_BINARY_BYTES)
            parsed = checker.read_industrial_export(
                export,
                expected_point_count=point_count,
                expected_tree_count=len(trees),
            )
            valid = _runner_report(
                family=family,
                point_count=point_count,
                seed=seed,
                trees=trees,
            )

            forged_report = copy.deepcopy(valid)
            forged_report["schema"] = "forged.runner.schema"
            _write_runner_report(runner_report, forged_report)
            with self.assertRaisesRegex(
                checker.ScientificGuardError, "runner report schema"
            ):
                checker._bind_runner_report(
                    parsed,
                    runner_report_path=runner_report,
                    family=family,
                    expected_git_sha=GIT_SHA,
                    runner_binary_path=runner_binary,
                    expected_runner_binary_sha256=RUNNER_BINARY_SHA256,
                )

            forged_digest = copy.deepcopy(valid)
            forged_digest["runs"][0]["hierarchies"][0]["digest"] = (
                "0x0000000000000000"
            )
            _write_runner_report(runner_report, forged_digest)
            with self.assertRaisesRegex(
                checker.ScientificGuardError, "digest disagrees"
            ):
                checker._bind_runner_report(
                    parsed,
                    runner_report_path=runner_report,
                    family=family,
                    expected_git_sha=GIT_SHA,
                    runner_binary_path=runner_binary,
                    expected_runner_binary_sha256=RUNNER_BINARY_SHA256,
                )

            forged_git = copy.deepcopy(valid)
            forged_git["git_sha"] = "0" * 40
            _write_runner_report(runner_report, forged_git)
            with self.assertRaisesRegex(
                checker.ScientificGuardError, "Git SHA disagrees"
            ):
                checker._bind_runner_report(
                    parsed,
                    runner_report_path=runner_report,
                    family=family,
                    expected_git_sha=GIT_SHA,
                    runner_binary_path=runner_binary,
                    expected_runner_binary_sha256=RUNNER_BINARY_SHA256,
                )

            _write_runner_report(runner_report, valid)
            with self.assertRaisesRegex(
                checker.ScientificGuardError, "runner binary SHA-256"
            ):
                checker._bind_runner_report(
                    parsed,
                    runner_report_path=runner_report,
                    family=family,
                    expected_git_sha=GIT_SHA,
                    runner_binary_path=runner_binary,
                    expected_runner_binary_sha256="not-a-sha256",
                )

    def test_cli_requires_and_validates_all_provenance_commitments(self) -> None:
        parser = checker._build_parser()
        base = ["trees.bin", "--family", "affine_uniform_binary64"]
        with mock.patch("sys.stderr"), self.assertRaises(SystemExit):
            parser.parse_args(base)
        with mock.patch("sys.stderr"), self.assertRaises(SystemExit):
            parser.parse_args(
                base
                + [
                    "--runner-report",
                    "runner.json",
                    "--expected-git-sha",
                    "forged",
                    "--runner-binary",
                    "runner",
                    "--expected-runner-binary-sha256",
                    RUNNER_BINARY_SHA256,
                ]
            )
        with mock.patch("sys.stderr"), self.assertRaises(SystemExit):
            parser.parse_args(
                base
                + [
                    "--runner-report",
                    "runner.json",
                    "--expected-git-sha",
                    GIT_SHA,
                    "--runner-binary",
                    "runner",
                    "--expected-runner-binary-sha256",
                    "forged",
                ]
            )
        options = parser.parse_args(
            base
            + [
                "--runner-report",
                "runner.json",
                "--expected-git-sha",
                GIT_SHA,
                "--runner-binary",
                "runner",
                "--expected-runner-binary-sha256",
                RUNNER_BINARY_SHA256,
            ]
        )
        self.assertEqual(options.expected_git_sha, GIT_SHA)
        self.assertEqual(options.runner_binary, Path("runner"))
        self.assertEqual(
            options.expected_runner_binary_sha256, RUNNER_BINARY_SHA256
        )

    def test_runner_binary_binding_hashes_bytes_and_rejects_mutation(self) -> None:
        family = "affine_uniform_binary64"
        point_count = 5
        seed = 7
        trees = _star_trees(point_count)
        first_bytes = b"\x7fELF fixture runner A"
        second_bytes = b"\x7fELF fixture runner B"
        first_sha256 = hashlib.sha256(first_bytes).hexdigest()
        second_sha256 = hashlib.sha256(second_bytes).hexdigest()
        self.assertNotEqual(first_sha256, second_sha256)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            export = root / "trees.bin"
            runner_report = root / "runner.json"
            first_binary = root / "runner-a"
            second_binary = root / "runner-b"
            _write_export(export, point_count, seed, trees)
            _write_runner_report(
                runner_report,
                _runner_report(
                    family=family,
                    point_count=point_count,
                    seed=seed,
                    trees=trees,
                    runner_binary_sha256=first_sha256,
                    runner_binary_size_bytes=len(first_bytes),
                ),
            )
            first_binary.write_bytes(first_bytes)
            second_binary.write_bytes(second_bytes)
            parsed = checker.read_industrial_export(
                export,
                expected_point_count=point_count,
                expected_tree_count=len(trees),
            )

            first_binding = checker._bind_runner_report(
                parsed,
                runner_report_path=runner_report,
                family=family,
                expected_git_sha=GIT_SHA,
                runner_binary_path=first_binary,
                expected_runner_binary_sha256=first_sha256,
            )
            self.assertEqual(first_binding["runner_binary_sha256"], first_sha256)
            self.assertEqual(
                first_binding["runner_binary_size_bytes"], len(first_bytes)
            )
            with self.assertRaisesRegex(
                checker.ScientificGuardError, "self-attestation disagrees"
            ):
                checker._bind_runner_report(
                    parsed,
                    runner_report_path=runner_report,
                    family=family,
                    expected_git_sha=GIT_SHA,
                    runner_binary_path=second_binary,
                    expected_runner_binary_sha256=second_sha256,
                )

            _write_runner_report(
                runner_report,
                _runner_report(
                    family=family,
                    point_count=point_count,
                    seed=seed,
                    trees=trees,
                    runner_binary_sha256=second_sha256,
                    runner_binary_size_bytes=len(second_bytes),
                ),
            )
            second_binding = checker._bind_runner_report(
                parsed,
                runner_report_path=runner_report,
                family=family,
                expected_git_sha=GIT_SHA,
                runner_binary_path=second_binary,
                expected_runner_binary_sha256=second_sha256,
            )
            self.assertEqual(second_binding["runner_binary_sha256"], second_sha256)

            with self.assertRaisesRegex(
                checker.ScientificGuardError, "binary bytes disagree"
            ):
                checker._bind_runner_report(
                    parsed,
                    runner_report_path=runner_report,
                    family=family,
                    expected_git_sha=GIT_SHA,
                    runner_binary_path=second_binary,
                    expected_runner_binary_sha256=first_sha256,
                )
            first_binary.write_bytes(second_bytes)
            with self.assertRaisesRegex(
                checker.ScientificGuardError, "binary bytes disagree"
            ):
                checker._bind_runner_report(
                    parsed,
                    runner_report_path=runner_report,
                    family=family,
                    expected_git_sha=GIT_SHA,
                    runner_binary_path=first_binary,
                    expected_runner_binary_sha256=first_sha256,
                )

    def test_runner_binding_rejects_forged_root_and_nonunique_export(self) -> None:
        family = "affine_uniform_binary64"
        point_count = 5
        seed = 7
        trees = _star_trees(point_count)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            export = root / "trees.bin"
            runner_report = root / "runner.json"
            runner_binary = root / "runner"
            _write_export(export, point_count, seed, trees)
            runner_binary.write_bytes(RUNNER_BINARY_BYTES)
            parsed = checker.read_industrial_export(
                export,
                expected_point_count=point_count,
                expected_tree_count=len(trees),
            )
            report = _runner_report(
                family=family,
                point_count=point_count,
                seed=seed,
                trees=trees,
            )
            report["runs"][0]["hierarchies"][1]["root_squared_level"] = 5.0
            _write_runner_report(runner_report, report)
            with self.assertRaisesRegex(
                checker.ScientificGuardError, "root_squared_level disagrees"
            ):
                checker._bind_runner_report(
                    parsed,
                    runner_report_path=runner_report,
                    family=family,
                    expected_git_sha=GIT_SHA,
                    runner_binary_path=runner_binary,
                    expected_runner_binary_sha256=RUNNER_BINARY_SHA256,
                )

            report = _runner_report(
                family=family,
                point_count=point_count,
                seed=seed,
                trees=trees,
            )
            report["runs"].append(copy.deepcopy(report["runs"][0]))
            report["runs"][1]["run_index"] = 1
            report["runs"][1]["measured_index"] = 1
            _write_runner_report(runner_report, report)
            with self.assertRaisesRegex(
                checker.ScientificGuardError, "exactly one run"
            ):
                checker._bind_runner_report(
                    parsed,
                    runner_report_path=runner_report,
                    family=family,
                    expected_git_sha=GIT_SHA,
                    runner_binary_path=runner_binary,
                    expected_runner_binary_sha256=RUNNER_BINARY_SHA256,
                )

    def test_small_end_to_end_independent_oracle(self) -> None:
        family = "affine_uniform_binary64"
        point_count = 24
        seed = 5101
        points, _ = checker.reconstruct_canonical_cloud(family, point_count, seed)
        k1, k2 = _complete_graph_trees(points)
        with tempfile.TemporaryDirectory() as directory:
            export = Path(directory) / "small.bin"
            runner_report = Path(directory) / "runner.json"
            runner_binary = Path(directory) / "runner"
            _write_export(export, point_count, seed, [k1, k2])
            runner_binary.write_bytes(RUNNER_BINARY_BYTES)
            runner_payload = _runner_report(
                family=family,
                point_count=point_count,
                seed=seed,
                trees=[k1, k2],
            )
            _write_runner_report(runner_report, runner_payload, cuda_banner=True)
            report = checker.analyze_export(
                export,
                family=family,
                runner_report_path=runner_report,
                expected_git_sha=GIT_SHA,
                runner_binary_path=runner_binary,
                expected_runner_binary_sha256=RUNNER_BINARY_SHA256,
                expected_point_count=point_count,
                expected_tree_count=2,
                triangle_batch_size=31,
                workers=1,
            )
            self.assertEqual(
                report["input"]["export_sha256"],
                hashlib.sha256(export.read_bytes()).hexdigest(),
            )
            self.assertEqual(
                report["provenance"]["runner_report_sha256"],
                hashlib.sha256(runner_report.read_bytes()).hexdigest(),
            )
            self.assertEqual(
                report["provenance"]["runner_binary_sha256"],
                hashlib.sha256(runner_binary.read_bytes()).hexdigest(),
            )
        self.assertEqual(report["result"], "scientific_guards_satisfied")
        self.assertEqual(report["provenance"]["git_sha"], GIT_SHA)
        self.assertEqual(
            report["provenance"]["runner_binary_sha256"],
            RUNNER_BINARY_SHA256,
        )
        self.assertTrue(
            report["provenance"][
                "runner_binary_reverified_after_scientific_replay"
            ]
        )
        self.assertTrue(
            report["provenance"]["all_hierarchy_digests_match_export"]
        )
        self.assertTrue(
            report["provenance"]["all_root_squared_levels_match_export"]
        )
        self.assertTrue(report["k1"]["canonical_edge_identity_match"])
        self.assertEqual(report["k1"]["late_count"], 0)
        self.assertEqual(
            report["k2"]["all_two_edge_triangle_deadline"]["late_count"], 0
        )
        self.assertGreater(report["k2"]["unique_two_edge_triangle_count"], 0)
        json.dumps(report, allow_nan=False)


if __name__ == "__main__":
    unittest.main()
