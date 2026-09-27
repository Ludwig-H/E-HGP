#!/usr/bin/env python3
"""Offline EOM gates. Synthetic points are not geometry/GPU evidence.

Run with python3 -B [or -B -O] test_eom.py. sklearn 1.9.1 is required;
no download, fit subprocess, external dataset, or evidence file is produced.
"""
from copy import deepcopy
import json
import math
import unittest

import numpy as np
from sklearn.cluster._hdbscan._tree import HIERARCHY_dtype, tree_to_labels

from eom import (condense_eom, equivalent_labels, fit_hdbscan,
                 hdbscan_min_samples, sklearn_tree_to_points)


def two_lobes(pair_radius=1.25):
    children = {8: [0, 1], 9: [2, 3], 10: [8, 9], 11: [4, 5],
                12: [6, 7], 13: [11, 12], 14: [10, 13]}
    heights = {8: pair_radius, 9: pair_radius, 10: 2.0, 11: pair_radius,
               12: pair_radius, 13: 2.0, 14: 10.0}
    return children, heights


class EomTests(unittest.TestCase):
    def test_analytic_z_ablation(self):
        children, heights = two_lobes()
        one = condense_eom(8, children, heights, min_cluster_size=2, exp_z=1)
        two = condense_eom(8, children, heights, min_cluster_size=2, exp_z=2)
        self.assertEqual(one["labels"], [0] * 4 + [1] * 4)
        self.assertEqual(two["labels"], [0, 0, 1, 1, 2, 2, 3, 3])
        self.assertAlmostEqual(one["stabilities"][9], 4 * (0.5 - 0.1))
        self.assertAlmostEqual(two["stabilities"][9], 4 * (0.25 - 0.01))
        topology = lambda result: [(e["parent"], e["child"], e["size"])
                                   for e in result["condensed_tree"]]
        self.assertEqual(topology(one), topology(two))
        self.assertEqual(one, condense_eom(8, children, heights, min_cluster_size=2))

    def test_single_cluster_disallowed_and_small_domain(self):
        for n, children, heights in ((1, {}, {}), (4, {4: [0, 1, 2, 3]}, {4: 1.0})):
            for mcs in (2, 5, 50):
                result = condense_eom(n, children, heights, min_cluster_size=mcs)
                self.assertEqual(result["labels"], [-1] * n)
                self.assertEqual(result["selected"], [])

    def test_small_branch_fallout(self):
        # One child below mcs: it drops out, while the large child retains
        # its parent's condensed identity. This is not a new cluster birth.
        children = {5: [0, 1], 6: [2, 3], 7: [5, 6], 8: [7, 4]}
        heights = {5: 1., 6: 1., 7: 3., 8: 10.}
        result = condense_eom(5, children, heights, min_cluster_size=2)
        self.assertEqual(result["labels"], [0, 0, 1, 1, -1])
        self.assertEqual(result["point_exit_lambda"][4], .1)

    def test_binary_plateau_diagnostic_is_not_hidden(self):
        tree = np.array([(0, 1, 1., 2), (2, 3, 1., 2), (4, 5, 1., 4)],
                        dtype=HIERARCHY_dtype)
        native, _ = tree_to_labels(tree, 2)
        n, children, heights = sklearn_tree_to_points(tree)
        kept = condense_eom(n, children, heights, min_cluster_size=2, atomize_ties=False)
        atomic = condense_eom(n, children, heights, min_cluster_size=2, atomize_ties=True)
        self.assertTrue(equivalent_labels(kept["labels"], native))
        self.assertFalse(equivalent_labels(atomic["labels"], native))
        self.assertEqual(atomic["labels"], [-1] * 4)
        self.assertEqual(atomic["contracted_internal_ties"], 2)

    def test_zero_plateau_never_inf_minus_inf(self):
        tree = np.array([(0, 1, 0., 2), (2, 3, 0., 2), (4, 5, 0., 4)],
                        dtype=HIERARCHY_dtype)
        native, _ = tree_to_labels(tree, 2)
        n, children, heights = sklearn_tree_to_points(tree)
        for z in (1, 2):
            for atomic in (True, False):
                result = condense_eom(n, children, heights, min_cluster_size=2,
                                      exp_z=z, atomize_ties=atomic)
                self.assertFalse(any(math.isnan(v) for v in result["stabilities"].values()))
                if atomic:
                    self.assertEqual(result["labels"], [-1] * n)
                else:
                    self.assertTrue(equivalent_labels(result["labels"], native))
                    self.assertEqual(result["stabilities"][n + 1], 0.0)

    def test_duplicate_groups_infinite_stability(self):
        children = {6: [0, 1, 2], 7: [3, 4, 5], 8: [6, 7]}
        heights = {6: 0., 7: 0., 8: 10.}
        for z in (1, 2):
            result = condense_eom(6, children, heights, min_cluster_size=2, exp_z=z)
            self.assertEqual(result["labels"], [0] * 3 + [1] * 3)
            self.assertTrue(all(math.isinf(result["stabilities"][c]) for c in result["selected"]))

    def test_multifurcation_permutation_and_scale_invariance(self):
        children = {6: [0, 1], 7: [2, 3], 8: [4, 5], 9: [6, 7, 8]}
        heights = {6: 1., 7: 1., 8: 1., 9: 10.}
        for z in (1, 2):
            result = condense_eom(6, children, heights, min_cluster_size=2, exp_z=z)
            permuted = {node: list(reversed(row)) for node, row in reversed(list(children.items()))}
            other = condense_eom(6, permuted, heights, min_cluster_size=2, exp_z=z)
            self.assertEqual(result, other)
            scaled = condense_eom(6, children, {node: 2*r for node, r in heights.items()},
                                  min_cluster_size=2, exp_z=z)
            self.assertEqual(result["labels"], scaled["labels"])
            for c in result["stabilities"]:
                self.assertAlmostEqual(scaled["stabilities"][c], result["stabilities"][c] / 2**z)

    def test_input_immutability_and_sequence_api(self):
        children, heights = two_lobes()
        saved = deepcopy((children, heights))
        expected = condense_eom(8, children, heights, min_cluster_size=2)
        rows = [[] for _ in range(15)]
        levels = [0.] * 15
        for node, row in children.items():
            rows[node] = row
            levels[node] = heights[node]
        self.assertEqual(expected, condense_eom(8, rows, levels, min_cluster_size=2))
        self.assertEqual((children, heights), saved)

    def test_refusals(self):
        children, heights = two_lobes()
        for kwargs in (dict(min_cluster_size=1), dict(min_cluster_size=True),
                       dict(min_cluster_size=2, exp_z=3),
                       dict(min_cluster_size=2, allow_single_cluster=True),
                       dict(min_cluster_size=2, cluster_selection_epsilon=.1)):
            with self.assertRaises(ValueError):
                condense_eom(8, children, heights, **kwargs)
        bad = [({2: [0, 0]}, {2: 1}), ({2: [0, 4]}, {2: 1}),
               ({2: [0, 1], 3: [0, 2]}, {2: 1, 3: 2}),
               ({2: [0, 1]}, {2: float("nan")}),
               ({2: [0, 1]}, {2: float("inf")}),
               ({2: [0, 1]}, {2: -1.}),
               ({2: [0, 1]}, {2: 1e-300})]
        for rows, levels in bad:
            with self.assertRaises(ValueError):
                condense_eom(2, rows, levels, min_cluster_size=2, exp_z=2)
        with self.assertRaises(ValueError):
            condense_eom(8, children, dict(heights, **{} ) | {14: .1}, min_cluster_size=2)
        self.assertEqual(hdbscan_min_samples(5), 5)
        self.assertEqual(hdbscan_min_samples(5, "contrib"), 4)
        with self.assertRaises(ValueError):
            hdbscan_min_samples(1, "contrib")
        with self.assertRaises(ValueError):
            fit_hdbscan([[0.], [1.], [2.]], k=2, min_cluster_size=2, max_points=2)
        with self.assertRaises(ValueError):
            fit_hdbscan([[0.], [float("nan")]], k=2, min_cluster_size=2)

    def test_real_library_grid_and_replay(self):
        rng = np.random.default_rng(20260927)
        fixtures = {
            "separated_gaussians": np.concatenate([rng.normal(loc=[x, 0., 0.], scale=.25, size=(30, 3))
                                                    for x in (-8., 0., 8.)]),
            "continuous_uniform": rng.uniform(-2., 2., size=(75, 3)),
            "integer_ties": np.array([[x, y, 0.] for x in range(9) for y in range(9)]),
            "duplicates": np.repeat(np.array([[0., 0.], [1., 0.], [10., 0.], [11., 0.]]), 20, axis=0),
            "all_identical": np.zeros((60, 2)),
        }
        fits, atomic_differences = 0, []
        for name, values in fixtures.items():
            for k in (1, 2, 5, 10):
                for mcs in (2, 5, 20, 50):
                    with self.subTest(fixture=name, k=k, mcs=mcs):
                        result = fit_hdbscan(values, k=k, min_cluster_size=mcs)
                        fits += 1
                        self.assertTrue(result["preserved_tree_z1_matches_standard"])
                        if not result["common_z1_matches_standard"]:
                            atomic_differences.append([name, k, mcs])
                            self.assertTrue(result["warnings"])
                        args = result["tree"]
                        again = condense_eom(**args, min_cluster_size=mcs, exp_z=1)
                        self.assertEqual(again, result["common"])
                        two = condense_eom(**args, min_cluster_size=mcs, exp_z=2)
                        self.assertFalse(any(math.isnan(v) for v in two["stabilities"].values()))
                        shape = lambda r: [(e["parent"], e["child"], e["size"])
                                           for e in r["condensed_tree"]]
                        self.assertEqual(shape(again), shape(two))
                        # K1: HGP uses radius d/2, HDBSCAN distance d.
                        if k == 1:
                            half = dict(args, heights={node: radius / 2 for node, radius in args["heights"].items()})
                            for z, reference in ((1, again), (2, two)):
                                scaled = condense_eom(**half, min_cluster_size=mcs, exp_z=z)
                                self.assertEqual(scaled["labels"], reference["labels"])
        print(json.dumps(dict(synthetic_library_fits=fits, preserved_z1_agreement=fits,
                              atomic_z1_differences=atomic_differences,
                              GPU_evidence=False, cloud_calls=0), sort_keys=True))


if __name__ == "__main__":
    unittest.main(verbosity=2)
