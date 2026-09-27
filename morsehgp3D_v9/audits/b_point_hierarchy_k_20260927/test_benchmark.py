#!/usr/bin/env python3
"""Independent, read-only arithmetic gates for benchmark scoring.

No dataset download, native process, source mutation, or result file. Truth
labels below are hand-made test fixtures, not parameter-selection feedback.
"""
from fractions import Fraction
import math
from pathlib import Path
import unittest
from unittest.mock import patch

import numpy as np

from benchmark import clean, dendrogram_purity, metrics, validate_case


def pairwise_purity_oracle(tree, truth):
    """Enumerate same-class pairs and each LCA; do not reuse production DP.

    A binary equal-height chain represents one event: climb through all its
    equal-height parents before counting leaves. Noise is removed from both
    the pair universe and LCA population, as declared by this experiment.
    """
    n, children, heights = tree["n"], tree["children"], tree["heights"]
    parent = {child: node for node, row in children.items() for child in row}

    def ancestry(node):
        result = [node]
        while node in parent:
            node = parent[node]
            result.append(node)
        return result

    def members(node):
        if node < n:
            return [node]
        return [leaf for child in children[node] for leaf in members(child)]

    values = []
    for first in range(n):
        if truth[first] < 0:
            continue
        lineage = set(ancestry(first))
        for second in range(first + 1, n):
            if truth[first] != truth[second]:
                continue
            lca = next(node for node in ancestry(second) if node in lineage)
            while lca in parent and heights[lca] == heights[parent[lca]]:
                lca = parent[lca]
            inliers = [leaf for leaf in members(lca) if truth[leaf] >= 0]
            same = sum(truth[leaf] == truth[first] for leaf in inliers)
            values.append(Fraction(same, len(inliers)))
    return sum(values, Fraction()) / len(values) if values else None


def quartet(equal=False, zero=False):
    return dict(n=4, children={4: [0, 1], 5: [2, 3], 6: [4, 5]},
                heights={4: 0. if zero else 1., 5: 0. if zero else 1.,
                         6: 0. if zero else (1. if equal else 3.)})


class MetricTests(unittest.TestCase):
    def test_perfect_partition_and_label_permutation(self):
        result = metrics([0, 0, 1, 1], [99, 99, 3, 3])
        self.assertEqual(result["ari_all"], 1.)
        self.assertEqual(result["nmi_all"], 1.)
        self.assertEqual(result["coverage"], 1.)
        self.assertEqual(result["clusters"], 2)
        self.assertEqual(result["noise_count"], 0)
        self.assertIsNone(result["noise_f1"])

    def test_predicted_noise_is_one_block_not_a_free_accuracy_claim(self):
        result = metrics([0, 0, 1, 1], [0, 0, -1, -1])
        # Crucial interpretation gate: ARI/NMI alone cannot penalize this
        # perfectly aligned NOISE block. Coverage must always accompany them.
        self.assertEqual(result["ari_all"], 1.)
        self.assertEqual(result["nmi_all"], 1.)
        self.assertEqual(result["ari_true_inliers"], 1.)
        self.assertEqual(result["ari_classified"], 1.)
        self.assertAlmostEqual(result["ari_inliers_noise_singletons"], 4 / 7)
        self.assertEqual(result["coverage"], .5)
        self.assertEqual(result["clusters"], 1)
        self.assertEqual(result["noise_count"], 2)

    def test_noise_precision_recall_and_distinct_ari_universes(self):
        result = metrics([0, 0, 1, 1, -1, -1], [0, -1, 1, 1, -1, 2])
        self.assertEqual(result["noise_precision"], .5)
        self.assertEqual(result["noise_recall"], .5)
        self.assertEqual(result["noise_f1"], .5)
        self.assertAlmostEqual(result["ari_all"], 2 / 7)
        self.assertAlmostEqual(result["ari_true_inliers"], 4 / 7)
        self.assertAlmostEqual(result["ari_inliers_noise_singletons"], 4 / 7)
        self.assertEqual(result["ari_classified"], 1.)
        self.assertAlmostEqual(result["coverage"], 2 / 3)
        self.assertEqual(result["noise_count"], 2)

    def test_all_predicted_noise_and_all_true_noise(self):
        result = metrics([0, 0, 1, 1], [-1] * 4)
        self.assertEqual(result["ari_all"], 0.)
        self.assertEqual(result["nmi_all"], 0.)
        self.assertEqual(result["clusters"], 0)
        self.assertEqual(result["coverage"], 0.)
        self.assertEqual(result["ari_inliers_noise_singletons"], 0.)
        self.assertIsNone(result["ari_classified"])
        all_noise = metrics([-1] * 4, [-1] * 4)
        self.assertEqual(all_noise["ari_all"], 1.)
        self.assertEqual(all_noise["noise_f1"], 1.)
        self.assertIsNone(all_noise["ari_true_inliers"])
        self.assertIsNone(all_noise["ari_classified"])
        self.assertIsNone(all_noise["ari_inliers_noise_singletons"])

    def test_singleton_noise_score_ignores_true_noise_not_predicted_noise(self):
        # Last two points are excluded by the true-inlier universe; changing
        # their predictions must not change this score. Existing labels need
        # not be dense or start at zero, so noise singleton IDs must not clash.
        first = metrics([0, 0, 1, 1, -1, -1], [100, 100, -1, -1, 8, 9])
        second = metrics([7, 7, 3, 3, -1, -1], [100, 100, -1, -1, -1, -1])
        self.assertAlmostEqual(first["ari_inliers_noise_singletons"], 4 / 7)
        self.assertEqual(first["ari_inliers_noise_singletons"], second["ari_inliers_noise_singletons"])
        self.assertEqual(metrics([0, 1], [-1, -1])["ari_inliers_noise_singletons"], 1.)
        self.assertIsNone(metrics([0, -1], [-1, 0])["ari_inliers_noise_singletons"])

    def test_no_predicted_noise_and_small_classified_set(self):
        result = metrics([0, 0, -1, -1], [0, 0, 1, 1])
        self.assertEqual(result["noise_precision"], 0.)
        self.assertEqual(result["noise_recall"], 0.)
        self.assertEqual(result["noise_f1"], 0.)
        singleton = metrics([0, 0, 1], [-1, -1, 0])
        self.assertIsNone(singleton["ari_classified"])
        self.assertAlmostEqual(singleton["coverage"], 1 / 3)

    def test_shape_refusals(self):
        for truth, labels in (([], []), ([0], [0, 1]), ([[0], [1]], [[0], [1]])):
            with self.assertRaises(ValueError):
                metrics(truth, labels)


class PurityTests(unittest.TestCase):
    def check_oracle(self, tree, truth):
        wanted, actual = pairwise_purity_oracle(tree, truth), dendrogram_purity(tree, truth)
        if wanted is None:
            self.assertIsNone(actual)
        else:
            self.assertAlmostEqual(actual, float(wanted), places=14)
            self.assertGreaterEqual(actual, 0.)
            self.assertLessEqual(actual, 1.)
        return actual

    def test_pure_and_crossed_tree(self):
        self.assertEqual(self.check_oracle(quartet(), [0, 0, 1, 1]), 1.)
        self.assertEqual(self.check_oracle(quartet(), [0, 1, 0, 1]), .5)

    def test_equal_height_atomization_removes_fake_purity(self):
        truth = [0, 0, 1, 1]
        star = dict(n=4, children={4: [0, 1, 2, 3]}, heights={4: 1.})
        self.assertEqual(self.check_oracle(quartet(equal=True), truth), .5)
        self.assertEqual(self.check_oracle(star, truth), .5)
        self.assertEqual(self.check_oracle(quartet(zero=True), truth), .5)

    def test_noise_excluded_also_from_lca_mass(self):
        star = dict(n=4, children={4: [0, 1, 2, 3]}, heights={4: 1.})
        # Purity deliberately does NOT penalize absorbing two true-noise
        # points; the benchmark publishes separate noise/coverage scores.
        self.assertEqual(self.check_oracle(star, [0, 0, -1, -1]), 1.)
        self.assertIsNone(self.check_oracle(star, [-1, -1, -1, -1]))

    def test_singleton_classes_and_no_pair_universe(self):
        star = dict(n=3, children={3: [0, 1, 2]}, heights={3: 1.})
        self.assertAlmostEqual(self.check_oracle(star, [0, 0, 1]), 2 / 3)
        self.assertIsNone(self.check_oracle(star, [0, 1, 2]))
        self.assertEqual(self.check_oracle(star, [9, 9, 9]), 1.)
        self.assertIsNone(self.check_oracle(dict(n=1, children={}, heights={}), [0]))

    def test_label_permutation_child_order_and_radius_scale(self):
        tree = quartet()
        first = self.check_oracle(tree, [0, 0, 1, 1])
        other = dict(n=4, children={k: list(reversed(v)) for k, v in tree["children"].items()},
                     heights={k: v * 7 for k, v in tree["heights"].items()})
        self.assertEqual(self.check_oracle(other, [9, 9, 3, 3]), first)

    def test_independent_pair_oracle_random_trees(self):
        rng = np.random.default_rng(2741)
        for n in range(2, 13):
            for repeat in range(12):
                roots, children, heights = list(range(n)), {}, {i: 0. for i in range(n)}
                for node in range(n, 2 * n - 1):
                    positions = sorted(rng.choice(len(roots), size=2, replace=False).tolist(), reverse=True)
                    pair = [roots.pop(index) for index in positions]
                    children[node] = pair
                    heights[node] = max(heights[x] for x in pair) + float(rng.integers(0, 3))
                    roots.append(node)
                truth = rng.integers(-1, 4, size=n).tolist()
                with self.subTest(n=n, repeat=repeat):
                    self.check_oracle(dict(n=n, children=children, heights=heights), truth)

    def test_bad_truth_length(self):
        with self.assertRaises(ValueError):
            dendrogram_purity(quartet(), [0, 1])


class PlumbingTests(unittest.TestCase):
    def test_strict_json_infinity_encoding(self):
        self.assertEqual(clean({1: [math.inf, -math.inf, .5]}),
                         {"1": ["+infinity", "-infinity", .5]})
        with self.assertRaises(ValueError):
            clean({"bad": math.nan})

    def test_same_input_gate_without_files(self):
        points = np.array([[0, 0, 0], [1, 2, 3]], dtype=np.uint32)
        case = dict(points_npy="unused.npy", points_u32le="unused.bin", labels_json="unused.json", n=2)
        with patch("benchmark.np.load", return_value=points), \
             patch("benchmark.np.fromfile", return_value=points.ravel()), \
             patch.object(Path, "read_text", return_value="[0, 1]"):
            actual, truth = validate_case(case)
            self.assertTrue(np.array_equal(actual, points))
            self.assertEqual(truth.tolist(), [0, 1])
        with patch("benchmark.np.load", return_value=points + 1), \
             patch("benchmark.np.fromfile", return_value=points.ravel()), \
             patch.object(Path, "read_text", return_value="[0, 1]"):
            with self.assertRaises(ValueError):
                validate_case(case)
        repeated = np.zeros((2, 3), dtype=np.uint32)
        with patch("benchmark.np.load", return_value=repeated), \
             patch("benchmark.np.fromfile", return_value=repeated.ravel()), \
             patch.object(Path, "read_text", return_value="[0, 1]"):
            with self.assertRaises(ValueError):
                validate_case(case)


if __name__ == "__main__":
    unittest.main(verbosity=2)
