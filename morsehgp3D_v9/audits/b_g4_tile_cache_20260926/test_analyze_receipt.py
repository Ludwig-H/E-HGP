#!/usr/bin/env python3
"""Offline analysis tests only: protocol tools and GCE are explicitly faked."""

import copy
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import analyze_receipt as analysis
import gpu_filter_selftest_v9 as fixture
import gpu_filter_snapshot_v9 as snapshot


class AnalysisTests(unittest.TestCase):
    def setUp(self):
        self.cases = analysis.read(analysis.HERE / 'plan.json')['cases']
        self.probes = [fixture.fake_probe_value(case['n'], analysis.worker.INPUTS[case['scene']]['fnv'],
                                               case['k'], case['workers'], case['repeats'],
                                               tile_cache=case['tile_cache'], s=case['s']) for case in self.cases]
        self.commands = [dict(exit_code=0, elapsed_seconds=0.1) for _ in self.cases]

    def test_arithmetic_and_scope(self):
        rows, comparisons = analysis.analyze_rows(self.cases, self.probes, self.commands)
        self.assertEqual(len(rows), 14)
        self.assertEqual(len(comparisons), 6)
        self.assertEqual(sum(len(row['passes']) for row in rows), 42)
        self.assertEqual(rows[0]['tile_cache_logical_storage_bytes'], 0)
        self.assertEqual(rows[1]['tile_cache_logical_storage_bytes'], 89 * 20 + 16 * 100)
        self.assertEqual(rows[1]['pair_geometry_tests'], 4935)
        self.assertEqual(comparisons[0]['process_count'], dict(reference=2, tile_cache=2))
        self.assertAlmostEqual(comparisons[0]['geometry_reduction_reference_over_cache'], 5035 / 4935)
        self.assertEqual(rows[0]['timings_ms']['all_passes']['pair_ms']['count'], 3)
        self.assertEqual(rows[0]['timings_ms']['warm_passes_1_onward']['pair_ms']['count'], 2)

    def test_cold_and_warm_are_separate(self):
        gpu = self.probes[0]['gpu']
        gpu['passes'][0]['pair_ms'] = 100.0
        gpu['passes'][0]['total_ms'] = gpu['first_total_ms'] = 104.5
        gpu['passes'][1]['pair_ms'], gpu['passes'][1]['total_ms'] = 12.0, 16.5
        rows, _ = analysis.analyze_rows(self.cases, self.probes, self.commands)
        self.assertEqual(rows[0]['timings_ms']['all_passes']['pair_ms']['median'], 12)
        self.assertEqual(rows[0]['timings_ms']['warm_passes_1_onward']['pair_ms']['median'], 11)

    def test_population_mismatch(self):
        self.probes[1]['population']['pair_survivors'] += 1
        with self.assertRaisesRegex(ValueError, 'paired populations'):
            analysis.analyze_rows(self.cases, self.probes, self.commands)

    def test_cpu_work_mismatch(self):
        self.probes[1]['cpu']['cache_searches'] += 1
        with self.assertRaisesRegex(ValueError, 'CPU work differ'):
            analysis.analyze_rows(self.cases, self.probes, self.commands)

    def test_gpu_work_mismatch_across_processes(self):
        self.probes[2]['gpu']['cache_node_tests'] += 1
        with self.assertRaisesRegex(ValueError, 'GPU work differs'):
            analysis.analyze_rows(self.cases, self.probes, self.commands)

    def test_mask_and_repeat_mutants(self):
        for key in ('pair_mismatches', 'repeat_mismatches'):
            probes = copy.deepcopy(self.probes)
            probes[1]['gpu'][key] += 1
            with self.assertRaises(ValueError):
                analysis.analyze_rows(self.cases, probes, self.commands)

    def test_all_passes_required(self):
        self.probes[1]['gpu']['passes'].pop()
        with self.assertRaisesRegex(ValueError, 'all GPU repetitions'):
            analysis.analyze_rows(self.cases, self.probes, self.commands)

    def test_raw_controller_fixture_and_tampered_hash(self):
        # This is the actual controller+worker state machine with FakeCloud,
        # fake compilers and fake probe. It cannot contact GCP.
        with tempfile.TemporaryDirectory(prefix='mhgp9-tile-analysis-test-') as directory:
            directory = Path(directory)
            package_dir = directory / 'package'
            record = snapshot.build('HEAD', package_dir, analysis.HERE / 'plan.json',
                                    allow_uncommitted_protocol=not fixture.protocol_committed())
            manifest_path = package_dir / 'source_manifest.json'
            pkg = dict(directory=directory, record=record, committed=fixture.protocol_committed(),
                       archive=package_dir / 'snapshot.tar.gz', manifest_path=manifest_path,
                       manifest=analysis.read(manifest_path))
            scenario = directory / 'scenario'
            scenario.mkdir()
            with patch.dict(fixture._PACKAGE, {True: pkg}):
                code, host_receipt, fake, host = fixture.run_scenario(scenario, tile_cache=True)
            self.assertEqual(code, 0)
            fixture.expect_certified_stop(host_receipt, fake)
            # FakeCloud checks real argv but does not serialize host command
            # rows. Materialize its actually observed final stop for this
            # reader-only fixture, never claim it is an external GCE receipt.
            stop_name, stop_argv = fake.calls[-1]
            self.assertEqual(stop_name, 'guarded_stop')
            (host / 'guarded_stop.stdout').write_text('[OFFLINE FIXTURE] TERMINATED\n')
            (host / 'guarded_stop.stderr').write_text('')
            host_receipt['commands'] = [dict(name=stop_name, argv=stop_argv, exit_code=0, group_closed=True,
                                             stdout_sha256=analysis.sha(host / 'guarded_stop.stdout'),
                                             stderr_sha256=analysis.sha(host / 'guarded_stop.stderr'))]
            (host / 'receipt.json').write_text(json.dumps(host_receipt))
            with patch.object(analysis.worker, 'CUDA_PATHS', (str(scenario / 'fakebin/nvcc'),)):
                result = analysis.build(host)
                self.assertEqual(result['case_count'], 14)
                self.assertEqual(result['gpu_pass_count'], 42)
                self.assertTrue(result['protocol_replayed'])
                self.assertTrue(result['shutdown_raw_hashes_verified'])
                self.assertFalse(result['FULL_executed'])
                self.assertFalse(result['global_subquadratic_claim'])
                path = host / 'received/output/probe_1.stdout'
                probe = analysis.read(path)
                probe['gpu']['pair_ms'] += 1
                path.write_text(json.dumps(probe))
                with self.assertRaisesRegex(ValueError, 'raw log hash'):
                    analysis.build(host)


if __name__ == '__main__':
    unittest.main()
