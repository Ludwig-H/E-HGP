"""Small preparation gates; no planned cloud, clustering or native export runs.

The integration fixtures replace only the in-process plan/generator and use
at most four points. Source files, including the historical quantizer, remain
unchanged. Assertions use unittest and therefore remain active under -O.
"""
from collections import Counter, defaultdict
from contextlib import redirect_stdout
from copy import deepcopy
from fractions import Fraction as Q
import inspect
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import numpy as np

import plan
import prepare


def read(path):
    return json.loads(Path(path).read_text())


def nearest_even(value):
    """Independent integer-division oracle, including negative rationals."""
    lo, remainder = divmod(value.numerator, value.denominator)
    twice = 2 * remainder
    return lo + int(twice > value.denominator or
                    (twice == value.denominator and lo % 2 != 0))


def unit_grid():
    return dict(origin_exact=['0', '0', '0'], step_exact='1')


def tiny_plan():
    cases = [plan._case('quality', 'size', 'spherical', n, 2, 4, 0, 77, 1)
             for n in (2, 3, 4)]
    return dict(schema='tiny_test_plan_not_a_campaign', cases=cases)


def tiny_generate(spec, reverse_labels=False):
    points = np.asarray([[1., 2., 3.], [3., 4., 5.], [-2., -1., 0.],
                         [10., 9., 8.]])[:spec['n']].copy()
    labels = np.asarray([1, 2, 1, 2], dtype=np.int64)[:spec['n']].copy()
    if reverse_labels:
        labels = 3-labels
    return points, labels, dict(test_fixture_only=True, n=spec['n'])


class PrepareTests(unittest.TestCase):
    refusals = 0
    exact_coordinates = 0

    def refused(self, points, grid, message=None):
        context = self.assertRaisesRegex(ValueError, message) if message else self.assertRaises(ValueError)
        with context:
            prepare.quantize_on_grid(points, grid)
        type(self).refusals += 1

    def check_exact(self, points, grid):
        points_before = np.asarray(points).copy()
        grid_before = deepcopy(grid)
        sites, metadata = prepare.quantize_on_grid(points, grid)
        origin = list(map(Q, grid['origin_exact']))
        step = Q(grid['step_exact'])
        expected = [[nearest_even((Q(float(x))-base)/step)
                     for x, base in zip(row, origin)] for row in points]
        np.testing.assert_array_equal(sites, expected)
        np.testing.assert_array_equal(points, points_before)
        self.assertEqual(grid, grid_before)
        self.assertEqual(sites.dtype, np.dtype('<u4'))
        self.assertEqual(sites.shape, (len(points), 3))
        maximum = max(abs(Q(float(x))-base-int(q)*step)
                      for row, qs in zip(points, sites)
                      for x, q, base in zip(row, qs, origin))
        self.assertEqual(Q(metadata['max_abs_error_exact']), maximum)
        self.assertLessEqual(maximum, step/2)
        self.assertEqual(metadata['n_before'], len(points))
        self.assertEqual(metadata['n_after'], len(points))
        self.assertEqual(metadata['merged_rows'], 0)
        self.assertEqual(metadata['grid_duplicate_rows'], 0)
        type(self).exact_coordinates += 3*len(points)
        return sites, metadata

    def test_complete_unique_plan_and_rows(self):
        cases = plan.cases()
        self.assertEqual(cases, plan.PLAN['cases'])
        self.assertEqual(len(cases), 43)
        self.assertEqual(len({c['id'] for c in cases}), 43)
        self.assertEqual(Counter(c['phase'] for c in cases), {'quality': 34, 'growth': 9})
        self.assertEqual(plan.QUALITY_SEEDS, (2026092801, 2026092802))
        self.assertEqual(plan.GROWTH_SEED, 2026092891)
        scenarios = defaultdict(list)
        for case in cases:
            spec = case['spec']
            if case['phase'] == 'quality':
                scenarios[tuple(spec[k] for k in ('family', 'n', 'groups', 'separation', 'noise_fraction'))].append(spec['seed'])
            else:
                self.assertEqual(spec['seed'], plan.GROWTH_SEED)
                self.assertIn(spec['n'], (8000, 16000, 32000))
        self.assertEqual(len(scenarios), 17)
        self.assertTrue(all(sorted(seeds) == list(plan.QUALITY_SEEDS) for seeds in scenarios.values()))
        quality = plan.PLAN['quality']
        rows = {(c['id'], method, k, m, z)
                for c in cases if c['phase'] == 'quality'
                for method in quality['methods'] for k in quality['k']
                for m in quality['min_cluster_size']
                for z in (quality['standard_exp_z'] if method == 'hdbscan_standard' else quality['exp_z'])}
        self.assertEqual(len(rows), 612)
        self.assertEqual(set(Counter(row[0] for row in rows).values()), {18})
        self.assertEqual(quality['expected_rows'], len(rows))
        self.assertEqual(quality['primary'], dict(k=5, min_cluster_size=20, exp_z=1))
        cases[0]['spec']['n'] = 1
        self.assertEqual(plan.PLAN['cases'][0]['spec']['n'], 400)

    def test_grid_series_and_size_scope(self):
        series = defaultdict(list)
        for case in plan.PLAN['cases']:
            series[prepare.grid_key(case)].append(case)
        # Two quality size series and three growth series have three sizes.
        self.assertEqual(Counter(len(value) for value in series.values()), {1: 28, 3: 5})
        for cases in series.values():
            if len(cases) == 3:
                expected = [400, 800, 1600] if cases[0]['phase'] == 'quality' else [8000, 16000, 32000]
                self.assertEqual(sorted(c['spec']['n'] for c in cases), expected)
        case = deepcopy(plan.PLAN['cases'][0])
        key = prepare.grid_key(case)
        case['spec']['n'] += 1
        self.assertEqual(prepare.grid_key(case), key)
        for name in ('family', 'groups', 'separation', 'noise_fraction', 'seed'):
            changed = deepcopy(case)
            changed['spec'][name] = 'different' if name == 'family' else changed['spec'][name]+1
            self.assertNotEqual(prepare.grid_key(changed), key)

    def test_ties_even_and_adjacent_binary64(self):
        points = np.asarray([[0.5, 0., 0.], [1.5, 1., 0.], [2.5, 2., 0.],
                             [3.5, 3., 0.], [4.5, 4., 0.],
                             [np.nextafter(0.5, 0.), 5., 0.],
                             [np.nextafter(0.5, 1.), 6., 0.]])
        sites, _ = self.check_exact(points, unit_grid())
        self.assertEqual(sites[:, 0].tolist(), [0, 2, 2, 4, 4, 0, 1])

    def test_rational_grid_binary64_oracle(self):
        grid = dict(origin_exact=['-11/3', '-5/7', '2/11'], step_exact='1/13')
        points = np.asarray([[float(Q(-11, 3)+Q(7*i+1, 29)),
                              float(Q(-5, 7)+Q(11*i+2, 31)),
                              float(Q(2, 11)+Q(13*i+3, 37))] for i in range(80)])
        self.check_exact(points, grid)

    def test_domain_endpoints_not_wrapped(self):
        limit = prepare.LIMIT
        sites, _ = self.check_exact(np.asarray([[0., 0., 0.], [limit, limit, limit]]), unit_grid())
        self.assertEqual(sites.tolist(), [[0, 0, 0], [limit, limit, limit]])
        for x in (-1., limit+1., np.nextafter(-0.5, -np.inf), limit+0.5):
            self.refused([[0., 0., 0.], [x, 1., 1.]], unit_grid(), 'never clipping')

    def test_nonfinite_shape_and_invalid_grid_refusals(self):
        for value in (float('nan'), float('inf'), -float('inf')):
            self.refused([[0., 0., 0.], [value, 1., 1.]], unit_grid(), 'finite')
        for points in ([], [1., 2., 3.], [[0., 0., 0.]], [[0., 0.], [1., 1.]],
                       [[0., 0., 0., 0.], [1., 1., 1., 1.]]):
            self.refused(points, unit_grid(), 'finite')
        valid = [[0., 0., 0.], [1., 1., 1.]]
        for step in ('0', '-1', 'nan', 'inf'):
            grid = unit_grid(); grid['step_exact'] = step
            self.refused(valid, grid)
        for origin in (['0', '0'], ['0']*4, ['0', 'nan', '0']):
            grid = unit_grid(); grid['origin_exact'] = origin
            self.refused(valid, grid)

    def test_duplicates_and_collisions_are_not_merged(self):
        for points in ([[0., 0., 0.], [0., 0., 0.]],
                       [[0.1, 0.1, 0.1], [0.4, 0.4, 0.4]]):
            self.refused(points, unit_grid(), 'never merged')

    def test_quantizer_has_no_label_parameter(self):
        self.assertEqual(list(inspect.signature(prepare.quantize_on_grid).parameters), ['points', 'grid'])
        with self.assertRaises(TypeError):
            prepare.quantize_on_grid([[0., 0., 0.], [1., 1., 1.]], unit_grid(), labels=[1, 2])
        type(self).refusals += 1

    def test_pinned_quantizer_and_wrong_pin_refusal(self):
        self.assertEqual(prepare.sha(prepare.QUANTIZER), prepare.QUANTIZER_SHA)
        self.assertTrue(callable(prepare.frozen_quantizer().quantize))
        with patch.object(prepare, 'QUANTIZER_SHA', '0'*64):
            with self.assertRaisesRegex(ValueError, 'frozen quantizer source'):
                prepare.frozen_quantizer()
        type(self).refusals += 1

    def test_tiny_preparation_common_grid_hashes_and_label_independence(self):
        fixture = tiny_plan()
        with tempfile.TemporaryDirectory(prefix='mhgp9-test-synthetic-preparation-') as temporary:
            outputs = []
            for reverse in (False, True):
                output = Path(temporary)/str(reverse)
                def generate(spec, reverse=reverse):
                    return tiny_generate(spec, reverse)
                with patch.object(prepare, 'PLAN', fixture), patch.object(prepare, 'generate', generate), redirect_stdout(io.StringIO()):
                    prepare.prepare(output)
                receipt = read(output/'receipt.json')
                self.assertEqual(receipt['status'], 'completed')
                self.assertEqual(receipt['sources_before'], receipt['sources_after'])
                self.assertEqual((receipt['cases'], receipt['quality_cases'], receipt['growth_cases']), (3, 3, 0))
                self.assertFalse(receipt['clustering_executed'])
                self.assertFalse(receipt['geometry_executed'])
                self.assertFalse(receipt['GCP_used'])
                self.assertEqual(receipt['manifest_sha256'], prepare.sha(output/'manifest.json'))
                manifest = read(output/'manifest.json')
                self.assertEqual(manifest['plan_sha256'], prepare.sha(output/'plan.json'))
                self.assertEqual(read(output/'plan.json'), fixture)
                self.assertEqual(manifest['generator_source_sha256'], prepare.sha(prepare.HERE/'synthetic_data.py'))
                grids = []
                all_sites = []
                for case in manifest['cases']:
                    for path, digest in case['files'].items():
                        self.assertEqual(prepare.sha(path), digest)
                    raw = np.load(case['original_npy'], allow_pickle=False)
                    sites = np.fromfile(case['points_u32le'], dtype='<u4').reshape(-1, 3)
                    np.testing.assert_array_equal(raw, tiny_generate(case['spec'])[0])
                    np.testing.assert_array_equal(np.load(case['points_npy'], allow_pickle=False), sites.astype(np.float64))
                    self.assertEqual(case['grid_series_case_ids'], [c['id'] for c in fixture['cases']])
                    self.assertEqual(case['n'], len(raw))
                    self.assertEqual(sum(case['true_counts'].values()), len(raw))
                    grid = case['quantization']
                    self.assertEqual(grid['origin_exact'], ['-2', '-1', '0'])
                    self.assertEqual(Q(grid['step_exact']), Q(12, prepare.LIMIT))
                    grids.append((grid['origin_exact'], grid['step_exact']))
                    all_sites.append(sites)
                self.assertTrue(all(grid == grids[0] for grid in grids))
                for sites in all_sites[1:]:
                    np.testing.assert_array_equal(sites[:2], all_sites[0])
                outputs.append(manifest)
            for a, b in zip(outputs[0]['cases'], outputs[1]['cases']):
                self.assertEqual(a['quantization'], b['quantization'])
                for field in ('original_npy', 'points_npy', 'points_u32le'):
                    self.assertEqual(Path(a[field]).read_bytes(), Path(b[field]).read_bytes())
                self.assertNotEqual(Path(a['labels_json']).read_bytes(), Path(b['labels_json']).read_bytes())
            receipt_path = Path(temporary)/'False'/'receipt.json'
            before = receipt_path.read_bytes()
            with self.assertRaisesRegex(ValueError, 'NEW prepared-data directory'):
                prepare.prepare(receipt_path.parent)
            self.assertEqual(receipt_path.read_bytes(), before)
            type(self).refusals += 1

    def test_failed_collision_receipt_is_preserved(self):
        fixture = tiny_plan(); fixture['cases'] = fixture['cases'][1:2]
        def colliding(spec):
            return np.asarray([[0., 0., 0.], [0., 0., 0.], [1., 0., 0.]]), np.asarray([1, 2, 1]), {}
        with tempfile.TemporaryDirectory(prefix='mhgp9-test-synthetic-failure-') as temporary:
            output = Path(temporary)/'collision'
            with patch.object(prepare, 'PLAN', fixture), patch.object(prepare, 'generate', colliding):
                with self.assertRaisesRegex(ValueError, 'never merged'):
                    prepare.prepare(output)
            receipt = read(output/'receipt.json')
            self.assertEqual(receipt['status'], 'failed')
            self.assertIn('never merged', receipt['error'])
            self.assertEqual(receipt['sources_before'], receipt['sources_after'])
            self.assertTrue((output/'intent.json').is_file())
            self.assertTrue((output/'plan.json').is_file())
            self.assertFalse((output/'manifest.json').exists())
            type(self).refusals += 1


if __name__ == '__main__':
    result = unittest.main(exit=False)
    print(json.dumps(dict(gates=result.result.testsRun, refusals=PrepareTests.refusals,
                          exact_coordinate_checks=PrepareTests.exact_coordinates,
                          maximum_fixture_points=80, planned_inputs_generated=0,
                          native_executed=False, clustering_executed=False,
                          status='PASS' if result.result.wasSuccessful() else 'FAIL'), sort_keys=True))
    raise SystemExit(0 if result.result.wasSuccessful() else 1)
