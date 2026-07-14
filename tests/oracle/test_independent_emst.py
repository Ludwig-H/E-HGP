from __future__ import annotations

import ast
import unittest
from fractions import Fraction
from pathlib import Path

from reference.morsehgp3d_oracle.generators import (
    generate_affine_cloud,
)
from reference.morsehgp3d_oracle.gamma import GammaCut, build_gamma_filtration
from reference.morsehgp3d_oracle.hierarchy import ForestBatch, MergeForest, build_merge_forest
from reference.morsehgp3d_oracle.oracle import run_oracle
from tests.reference_emst import build_exhaustive_emst, exact_affine_dimension


Component = tuple[int, ...]
Partition = tuple[Component, ...]
FusionSignature = tuple[tuple[Component, ...], Component]

APPROVED_EMST_IMPORT_ROOTS = frozenset(
    {"__future__", "dataclasses", "fractions", "itertools", "math", "typing"}
)
DYNAMIC_IMPORT_NAMES = frozenset({"__import__", "import_module", "load_module"})


def _audit_emst_source_imports(source: str) -> tuple[str, ...]:
    """Return deterministic violations for one prospective EMST source."""

    tree = ast.parse(source)
    violations = []
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            for alias in node.names:
                root = alias.name.partition(".")[0]
                if root not in APPROVED_EMST_IMPORT_ROOTS:
                    violations.append(
                        f"line {node.lineno}: forbidden absolute import {alias.name}"
                    )
        elif isinstance(node, ast.ImportFrom):
            if node.level:
                violations.append(
                    f"line {node.lineno}: relative imports are forbidden"
                )
            else:
                module = node.module or ""
                root = module.partition(".")[0]
                if root not in APPROVED_EMST_IMPORT_ROOTS:
                    violations.append(
                        f"line {node.lineno}: forbidden import from {module}"
                    )
        elif isinstance(node, ast.Call):
            called_name = None
            if isinstance(node.func, ast.Name):
                called_name = node.func.id
            elif isinstance(node.func, ast.Attribute):
                called_name = node.func.attr
            if called_name in DYNAMIC_IMPORT_NAMES:
                violations.append(
                    f"line {node.lineno}: forbidden dynamic import {called_name}"
                )
    return tuple(sorted(violations))


def _gamma_partition(cut: GammaCut) -> Partition:
    return tuple(sorted(component.covered_point_ids for component in cut.components))


def _forest_partition(forest: MergeForest, level: Fraction, *, closed: bool) -> Partition:
    cut = forest.cut(level, closed=closed)
    return tuple(sorted(component.covered_point_ids for component in cut.components))


def _semantic_fusions(before: Partition, after: Partition) -> tuple[FusionSignature, ...]:
    signatures = []
    for merged_component in after:
        merged_points = set(merged_component)
        children = tuple(
            sorted(component for component in before if set(component) <= merged_points)
        )
        if len(children) >= 2:
            signatures.append((children, merged_component))
    return tuple(sorted(signatures))


def _forest_fusions(
    batch: ForestBatch | None,
    forest: MergeForest,
) -> tuple[FusionSignature, ...]:
    if batch is None:
        return ()
    nodes_by_id = {node.node_id: node for node in forest.nodes}
    pre_by_root = {
        component.root_id: component.covered_point_ids
        for component in batch.pre_components
    }
    post_by_root = {
        component.root_id: component.covered_point_ids
        for component in batch.post_components
    }
    signatures = []
    for node_id in batch.created_node_ids:
        node = nodes_by_id[node_id]
        if node.kind != "merge":
            continue
        children = tuple(sorted(pre_by_root[child_id] for child_id in node.child_ids))
        signatures.append((children, post_by_root[node_id]))
    return tuple(sorted(signatures))


def _partition_drop_weight(filtration, *, graph_kind: str) -> Fraction:
    total = Fraction(0)
    for level in filtration.critical_levels:
        if level == 0:
            continue
        before = len(
            filtration.cut(level, closed=False, graph_kind=graph_kind).components
        )
        after = len(
            filtration.cut(level, closed=True, graph_kind=graph_kind).components
        )
        if after > before:
            raise AssertionError("a k=1 graph cut gained components at an edge level")
        total += 4 * level * (before - after)
    return total


class IndependentEMSTTests(unittest.TestCase):
    maxDiff = None

    def assert_emst_matches_k1(self, points) -> None:
        emst = build_exhaustive_emst(points)
        filtration = build_gamma_filtration(points, 1)
        forest = build_merge_forest(filtration, "hgp_reduced")
        levels = tuple(
            sorted(
                set(emst.replay_levels)
                | set(filtration.critical_levels)
                | set(forest.critical_levels)
            )
        )

        forest_batches = {batch.squared_level: batch for batch in forest.batches}
        emst_batches = {batch.squared_level: batch for batch in emst.batches}
        for level in levels:
            with self.subTest(level=level):
                for closed in (False, True):
                    tree_partition = emst.cut(level, closed=closed).components
                    complete_partition = emst.cut(
                        level,
                        closed=closed,
                        edge_source="complete_graph",
                    ).components
                    gamma_partition = _gamma_partition(
                        filtration.cut(level, closed=closed, graph_kind="gamma")
                    )
                    gabriel_partition = _gamma_partition(
                        filtration.cut(level, closed=closed, graph_kind="gabriel")
                    )
                    forest_partition = _forest_partition(
                        forest, level, closed=closed
                    )
                    self.assertEqual(tree_partition, complete_partition)
                    self.assertEqual(tree_partition, gamma_partition)
                    self.assertEqual(tree_partition, gabriel_partition)
                    self.assertEqual(tree_partition, forest_partition)

                before = emst.cut(level, closed=False).components
                after = emst.cut(level, closed=True).components
                emst_fusions = _semantic_fusions(before, after)
                gamma_fusions = _semantic_fusions(
                    _gamma_partition(
                        filtration.cut(level, closed=False, graph_kind="gamma")
                    ),
                    _gamma_partition(
                        filtration.cut(level, closed=True, graph_kind="gamma")
                    ),
                )
                gabriel_fusions = _semantic_fusions(
                    _gamma_partition(
                        filtration.cut(level, closed=False, graph_kind="gabriel")
                    ),
                    _gamma_partition(
                        filtration.cut(level, closed=True, graph_kind="gabriel")
                    ),
                )
                forest_fusions = _forest_fusions(forest_batches.get(level), forest)
                self.assertEqual(emst_fusions, gamma_fusions)
                self.assertEqual(emst_fusions, gabriel_fusions)
                self.assertEqual(emst_fusions, forest_fusions)
                if level in emst_batches:
                    recorded = tuple(
                        (fusion.child_components, fusion.merged_component)
                        for fusion in emst_batches[level].multifusions
                    )
                    self.assertEqual(recorded, emst_fusions)

        forest_total = sum(
            (
                4 * node.squared_level * (len(node.child_ids) - 1)
                for node in forest.nodes
                if node.kind == "merge"
            ),
            Fraction(0),
        )
        self.assertEqual(emst.total_squared_weight, forest_total)
        self.assertEqual(
            emst.total_squared_weight,
            _partition_drop_weight(filtration, graph_kind="gamma"),
        )
        self.assertEqual(
            emst.total_squared_weight,
            _partition_drop_weight(filtration, graph_kind="gabriel"),
        )

        coverage_records = tuple(
            sorted(
                (
                    delta.squared_level,
                    delta.added_facet_point_ids,
                    delta.added_point_ids,
                )
                for delta in forest.coverage_log
            )
        )
        expected_coverage = tuple(
            (Fraction(0), ((point_id,),), (point_id,))
            for point_id in range(len(points))
        )
        self.assertEqual(coverage_records, expected_coverage)

        counters = emst.counters
        expected_edges = len(points) * (len(points) - 1) // 2
        self.assertEqual(counters.point_count, len(points))
        self.assertEqual(counters.distance_evaluations, expected_edges)
        self.assertEqual(counters.complete_edge_count, expected_edges)
        self.assertEqual(counters.selected_edge_count, max(0, len(points) - 1))
        self.assertEqual(
            counters.redundant_edge_count,
            counters.complete_edge_count - counters.selected_edge_count,
        )
        self.assertEqual(counters.distinct_edge_weight_count, len(emst.batches))
        self.assertEqual(counters.replay_level_count, 1 + len(emst.batches))

    def test_reference_source_has_only_stdlib_imports(self) -> None:
        source_path = (
            Path(__file__).resolve().parents[1]
            / "reference_emst"
            / "exhaustive_emst.py"
        )
        violations = _audit_emst_source_imports(
            source_path.read_text(encoding="utf-8")
        )
        self.assertEqual(violations, ())

    def test_reference_source_audit_rejects_external_and_dynamic_imports(self) -> None:
        bad_sources = {
            "absolute": "import reference.morsehgp3d_oracle\n",
            "importlib": "import importlib\n",
            "from_importlib": "from importlib import import_module\n",
            "dunder": "module = __import__('reference.morsehgp3d_oracle')\n",
            "import_module": "module = import_module('reference.morsehgp3d_oracle')\n",
            "load_module": "loader.load_module('reference.morsehgp3d_oracle')\n",
            "relative": "from ..oracle import exact\n",
        }
        for label, source in bad_sources.items():
            with self.subTest(label=label):
                self.assertTrue(_audit_emst_source_imports(source))

    def test_independent_affine_dimension_and_invalid_inputs(self) -> None:
        self.assertEqual(exact_affine_dimension([(0, 0, 0)]), 0)
        self.assertEqual(
            exact_affine_dimension([(0, 0, 0), (1, 2, 3), (2, 4, 6)]),
            1,
        )
        self.assertEqual(
            exact_affine_dimension(
                [(0, 0, 0), (1, 0, 1), (0, 1, 2), (2, 3, 8)]
            ),
            2,
        )
        self.assertEqual(
            exact_affine_dimension(
                [(0, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1)]
            ),
            3,
        )
        for bad_points in ((), ((0, 0),), ((0, 0, float("inf")),)):
            with self.subTest(bad_points=bad_points):
                with self.assertRaises((TypeError, ValueError)):
                    exact_affine_dimension(bad_points)

    def test_seeded_affine_clouds_n4_through_n12(self) -> None:
        for dimension in (1, 2, 3):
            for point_count in range(4, 13):
                with self.subTest(dimension=dimension, point_count=point_count):
                    points = generate_affine_cloud(
                        dimension,
                        point_count,
                        seed=10_000 * dimension + point_count,
                        coordinate_bound=31,
                    )
                    self.assertEqual(exact_affine_dimension(points), dimension)
                    self.assert_emst_matches_k1(points)

    def test_equal_weight_batches_preserve_canonical_multifusions(self) -> None:
        clouds = (
            (tuple((index, 0, 0) for index in range(6)), Fraction(5), 6),
            (
                ((-1, -1, 0), (1, -1, 0), (1, 1, 0), (-1, 1, 0)),
                Fraction(12),
                4,
            ),
            (
                tuple(
                    (x, y, z)
                    for x in (-1, 1)
                    for y in (-1, 1)
                    for z in (-1, 1)
                ),
                Fraction(28),
                8,
            ),
        )
        for points, expected_weight, expected_arity in clouds:
            with self.subTest(point_count=len(points)):
                emst = build_exhaustive_emst(points)
                self.assertEqual(emst.total_squared_weight, expected_weight)
                self.assertGreater(emst.counters.equal_weight_batch_count, 0)
                self.assertGreater(emst.counters.max_equal_weight_batch_size, 1)
                self.assertGreater(emst.counters.multifusion_count, 0)
                self.assertEqual(emst.counters.max_merge_arity, expected_arity)
                self.assertEqual(emst.cut(0, closed=False).components, ())
                self.assertEqual(
                    emst.cut(0, closed=True).components,
                    tuple((point_id,) for point_id in range(len(points))),
                )
                self.assert_emst_matches_k1(points)

    def test_small_n_run_oracle_smoke_for_both_profiles(self) -> None:
        clouds = {
            1: ((2, -1, 4),),
            2: ((0, 0, 0), (3, 1, 0)),
            3: ((0, 0, 0), (4, 0, 0), (1, 3, 0)),
        }
        for point_count, points in clouds.items():
            with self.subTest(point_count=point_count, comparison="emst"):
                self.assert_emst_matches_k1(points)
            for profile in ("full_pi0", "hgp_reduced"):
                with self.subTest(point_count=point_count, profile=profile):
                    result = run_oracle(points, 10, profile=profile)
                    self.assertEqual(result.k_max, 10)
                    self.assertEqual(result.k_eff, point_count)
                    self.assertEqual(result.s_max, point_count)
                    self.assertEqual(
                        tuple(order.order for order in result.orders),
                        tuple(range(1, point_count + 1)),
                    )
                    self.assertEqual(len(result.forests), point_count)
                    self.assertEqual(len(result.vertical_maps), point_count - 1)
                    self.assertEqual(result.profile, profile)


if __name__ == "__main__":
    unittest.main()
