#!/usr/bin/env python3
"""Fast, offline preparation tests; no clustering and no dependency on raw data."""
import json
from pathlib import Path
import tempfile
import unittest

import numpy as np

import datasets as ds


class DatasetTests(unittest.TestCase):
    def test_fixed_plan(self):
        self.assertEqual(len(ds.PLAN["cases"]), 12)
        self.assertEqual(len(set(ds.PLAN["cases"])), 12)
        self.assertEqual(ds.PLAN["k"], [2, 5, 10])
        self.assertEqual(ds.PLAN["min_cluster_size"], [20, 50])
        self.assertEqual(ds.PLAN["z"], [1, 2])
        self.assertNotEqual(ds.SEEDS["development"], ds.SEEDS["evaluation"])

    def test_planar_is_not_fake_3d(self):
        q, labels, info = ds.quantize([[0, 0], [1, 2], [2, 4]], [1, 2, 0])
        self.assertTrue(np.all(q[:, 2] == 0))
        self.assertEqual(info["dimension"], 2)
        self.assertEqual(info["embedding"], "z=0")
        np.testing.assert_array_equal(labels, [1, 2, -1])
        self.assertLessEqual(info["max_abs_error"], info["step"] / 2)

    def test_single_scale_not_per_axis(self):
        q, _, info = ds.quantize([[0, 0, 0], [1, 2, 0]], [1, 2])
        self.assertEqual(int(q[1, 0]), 131072)
        self.assertEqual(int(q[1, 1]), ds.LIMIT)
        self.assertEqual(info["step_exact"], "2/262143")

    def test_exact_ties_to_even(self):
        q, _, info = ds.quantize([[0, 0, 0], [.5, 1, 0], [1.5, 2, 0], [ds.LIMIT, 0, 0]], [1]*4)
        np.testing.assert_array_equal(q[:, 0], [0, 0, 2, ds.LIMIT])
        self.assertEqual(info["step_exact"], "1")
        self.assertEqual(info["max_abs_error_exact"], "1/2")

    def test_geometry_independent_of_truth(self):
        pts, labels = ds.synthetic("varied_density", ds.SEEDS["development"])
        q, _, grid = ds.quantize(pts, labels)
        other, _, other_grid = ds.quantize(pts, np.ones(len(pts)))
        np.testing.assert_array_equal(q, other)
        self.assertEqual(grid, other_grid)

    def test_refuse_exact_duplicate_even_same_label(self):
        with self.assertRaisesRegex(ValueError, "raw=1, grid=1, contradictory_labels=0"):
            ds.quantize([[0, 0], [0, 0], [1, 1]], [1, 1, 2])

    def test_refuse_new_collision_and_conflicting_labels(self):
        with self.assertRaisesRegex(ValueError, "raw=0, grid=1, contradictory_labels=1"):
            ds.quantize([[0, 0], [1e-10, 0], [1, 1]], [1, 2, 3])

    def test_input_guards(self):
        cases = [([[0, 0], [float("nan"), 1]], [1, 2]),
                 ([[0, 0], [0, 0]], [1, 1]),
                 ([[0, 0], [1, 1]], [1]),
                 ([[0, 0], [1, 1]], [1, 1.5]),
                 ([[0, 0], [1, 1]], [1, -2]),
                 ([[0, 0]], [1]),
                 ([[0], [1]], [1, 2])]
        for points, labels in cases:
            with self.subTest(points=points, labels=labels), self.assertRaises(ValueError):
                ds.quantize(points, labels)

    def test_synthetic_reproducible_full_scenes(self):
        for family in ds.FAMILIES:
            a, labels = ds.synthetic(family, ds.SEEDS["development"])
            again, again_labels = ds.synthetic(family, ds.SEEDS["development"])
            different, _ = ds.synthetic(family, ds.SEEDS["evaluation"])
            self.assertEqual(a.shape, (900, 3))
            np.testing.assert_array_equal(a, again)
            np.testing.assert_array_equal(labels, again_labels)
            self.assertFalse(np.array_equal(a, different))
            self.assertEqual(int(np.count_nonzero(labels == 0)), 200 if family == "bridge_noise" else 0)
            _, _, grid = ds.quantize(a, labels)
            self.assertEqual(grid["grid_duplicate_rows"], 0)

    def test_preparation_same_points_both_methods_and_noise(self):
        ident = "synthetic_bridge_noise_development"
        with tempfile.TemporaryDirectory() as directory:
            result = ds.prepare(directory, [ident])
            self.assertFalse(result["complete"])
            self.assertEqual(result, ds.prepare(directory, [ident]))
            case = result["cases"][0]
            q = np.fromfile(case["points_u32le"], dtype="<u4").reshape(-1, 3)
            hdbscan_points = np.load(case["points_npy"], allow_pickle=False)
            np.testing.assert_array_equal(q, hdbscan_points)
            self.assertEqual(hdbscan_points.dtype, np.float64)
            labels = json.loads(Path(case["labels_json"]).read_text())
            self.assertEqual(len(labels), 900)
            self.assertEqual(labels.count(-1), 200)
            self.assertEqual(case["noise_count"], 200)
            self.assertEqual(Path(case["points_u32le"]).stat().st_size, 900 * 12)
            self.assertEqual(case["quantization"]["merged_rows"], 0)
            for key, expected in case["prepared_sha256"].items():
                self.assertEqual(ds.digest(case[key]), expected)

    def test_no_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "witness"
            ds.write_once(path, b"old")
            with self.assertRaisesRegex(ValueError, "existing file differs"):
                ds.write_once(path, b"new")
            self.assertEqual(path.read_bytes(), b"old")

    def test_bad_case_selection(self):
        with tempfile.TemporaryDirectory() as directory:
            for choices in [[], ["missing"], ["fcps_hepta", "fcps_hepta"]]:
                with self.assertRaises(ValueError):
                    ds.prepare(directory, choices)


if __name__ == "__main__":
    unittest.main()
