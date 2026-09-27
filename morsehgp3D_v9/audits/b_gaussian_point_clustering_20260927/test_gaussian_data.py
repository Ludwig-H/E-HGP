#!/usr/bin/env python3
"""Offline Gaussian generator/MAP/preparation tests; no clustering scores."""
import copy
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock

import numpy as np

import gaussian_data as gd


class GaussianDataTests(unittest.TestCase):
    def test_predeclared_48_cases(self):
        cases = gd.PLAN["cases"]
        self.assertEqual(len(cases), 48)
        self.assertEqual(len({c["id"] for c in cases}), 48)
        self.assertEqual(sum(c["regime"] == "spherical" for c in cases), 36)
        self.assertEqual(sum(c["regime"] == "anisotropic" for c in cases), 6)
        self.assertEqual(sum(c["regime"] == "unbalanced" for c in cases), 6)
        self.assertTrue(all(c["n"] == 1200 and c["dimension"] == 3 for c in cases))
        self.assertEqual(gd.PLAN["k"], [5, 10])
        self.assertEqual(gd.PLAN["min_cluster_size"], [10, 20, 50, 100])
        self.assertEqual(gd.PLAN["exp_z"], [1, 2])
        self.assertEqual(gd.PLAN["primary"], dict(k=5, min_cluster_size=20, exp_z=1))

    def test_farthest_first_layout_normalization(self):
        for g in gd.COMMUNITIES:
            layout = gd.unit_layout(g)
            self.assertEqual(layout.shape, (g, 3))
            np.testing.assert_array_equal(layout, gd.unit_layout(g))
            np.testing.assert_allclose(layout.mean(axis=0), 0, atol=1e-15)
            dmin = min(np.linalg.norm(x-y) for i,x in enumerate(layout) for y in layout[:i])
            self.assertAlmostEqual(dmin, 1.)
        with self.assertRaises(ValueError):
            gd.unit_layout(3)

    def test_seeded_proper_rotation(self):
        for g in gd.COMMUNITIES:
            for seed in gd.SEEDS:
                rotation = gd.proper_rotation(seed, g, 1)
                np.testing.assert_allclose(rotation.T @ rotation, np.eye(3), atol=1e-14)
                self.assertAlmostEqual(np.linalg.det(rotation), 1.)
                np.testing.assert_array_equal(rotation, gd.proper_rotation(seed, g, 1))

    def test_all_shapes_positive_truth_and_exact_counts(self):
        for case in gd.PLAN["cases"]:
            points, labels, parameters = gd.generate(case)
            g = case["communities"]
            self.assertEqual(points.shape, (1200, 3))
            self.assertTrue(np.isfinite(points).all())
            self.assertEqual(set(labels), set(range(1, g+1)))
            counts = np.bincount(labels, minlength=g+1)[1:]
            expected = [240, 60]*4 if case["regime"] == "unbalanced" else [1200//g]*g
            np.testing.assert_array_equal(counts, expected)
            self.assertEqual(parameters["true_counts"], expected)
            self.assertEqual(parameters["injected_noise_count"], 0)
            self.assertAlmostEqual(sum(parameters["priors"]), 1.)
            self.assertAlmostEqual(parameters["minimum_mean_distance_observed"], case["separation"])

    def test_covariances_and_component_rotations(self):
        for regime in ("spherical", "anisotropic", "unbalanced"):
            case = next(c for c in gd.PLAN["cases"] if c["regime"] == regime)
            _, _, parameters = gd.generate(case)
            expected = [.25, 1., 4.] if regime == "anisotropic" else [1., 1., 1.]
            for covariance in parameters["covariances"]:
                np.testing.assert_allclose(np.linalg.eigvalsh(covariance), expected, atol=1e-13)
            if regime == "anisotropic":
                rotations = parameters["component_rotations"]
                self.assertFalse(np.array_equal(rotations[0], rotations[1]))

    def test_pairing_across_separations(self):
        for regime in ("spherical", "anisotropic", "unbalanced"):
            cases = [c for c in gd.PLAN["cases"] if c["regime"] == regime
                     and c["communities"] == 8 and c["seed_index"] == 1]
            reference = None
            for case in cases:
                points, labels, parameters = gd.generate(case)
                residual = points - np.asarray(parameters["means"])[labels-1]
                current = (residual, labels, parameters)
                if reference is not None:
                    np.testing.assert_array_equal(labels, reference[1])
                    np.testing.assert_allclose(residual, reference[0], atol=3e-14)
                    self.assertEqual(parameters["unit_centers"], reference[2]["unit_centers"])
                    self.assertEqual(parameters["covariances"], reference[2]["covariances"])
                    self.assertEqual(parameters["standardized_draws_sha256"], reference[2]["standardized_draws_sha256"])
                reference = current

    def test_reproducibility_and_distinct_seed(self):
        a, b = gd.PLAN["cases"][:2]
        x, labels, parameters = gd.generate(a)
        repeated, repeated_labels, repeated_parameters = gd.generate(a)
        other, _, _ = gd.generate(b)
        np.testing.assert_array_equal(x, repeated)
        np.testing.assert_array_equal(labels, repeated_labels)
        self.assertEqual(parameters, repeated_parameters)
        self.assertFalse(np.array_equal(x, other))

    def test_bayes_map_known_spherical_boundary_and_tie(self):
        parameters = dict(means=[[-1, 0, 0], [1, 0, 0]],
                          covariances=[np.eye(3).tolist()]*2, priors=[.5, .5])
        points = np.asarray([[-1, 0, 0], [1, 0, 0], [0, 0, 0], [.1, 3, -2]])
        np.testing.assert_array_equal(gd.bayes_map(points, parameters), [1, 2, 1, 2])
        parameters["priors"] = [.8, .2]
        np.testing.assert_array_equal(gd.bayes_map(points, parameters), [1, 2, 1, 1])

    def test_bayes_covariance_density_not_only_nearest_mean(self):
        parameters = dict(means=[[0, 0, 0], [0, 0, 0]],
                          covariances=[np.eye(3).tolist(), (4*np.eye(3)).tolist()], priors=[.5, .5])
        np.testing.assert_array_equal(gd.bayes_map(np.asarray([[0, 0, 0], [5, 0, 0]]), parameters), [1, 2])

    def test_refuse_changed_quantizer(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "changed.py"
            with path.open("xb") as out:
                out.write(b"raise RuntimeError('must not execute')\n")
            with mock.patch.object(gd, "QUANTIZER", path), self.assertRaisesRegex(ValueError, "source changed"):
                gd._quantizer()

    def test_prepared_format_map_and_no_overwrite(self):
        small_plan = copy.deepcopy(gd.PLAN)
        small_plan["cases"] = gd.PLAN["cases"][:1]
        with tempfile.TemporaryDirectory() as directory, mock.patch.object(gd, "PLAN", small_plan):
            result = gd.prepare(directory)
            self.assertEqual(result, gd.prepare(directory))
            case = result["cases"][0]
            q = np.fromfile(case["points_u32le"], dtype="<u4").reshape(-1, 3)
            np.testing.assert_array_equal(q, np.load(case["points_npy"], allow_pickle=False))
            labels = json.loads(Path(case["labels_json"]).read_text())
            self.assertEqual(len(labels), 1200)
            self.assertTrue(all(x > 0 for x in labels))
            diagnostic = json.loads(Path(case["bayes_map_json"]).read_text())
            self.assertEqual(len(diagnostic["predictions_original"]), 1200)
            self.assertEqual(len(diagnostic["predictions_reconstructed_grid"]), 1200)
            self.assertFalse(diagnostic["fitted"])
            self.assertEqual(sum(map(sum, diagnostic["confusion_rows_truth_columns_map"])), 1200)
            self.assertEqual(case["noise_count"], 0)
            self.assertEqual(case["quantization"]["merged_rows"], 0)
            for key, expected in {**case["prepared_sha256"], **case["diagnostic_sha256"]}.items():
                self.assertEqual(hashlib.sha256(Path(case[key]).read_bytes()).hexdigest(), expected)
            with mock.patch.object(gd, "N", 1201), self.assertRaises(ValueError):
                gd.prepare(directory)


if __name__ == "__main__":
    unittest.main()
