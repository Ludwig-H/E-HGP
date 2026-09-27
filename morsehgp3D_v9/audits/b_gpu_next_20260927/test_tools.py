#!/usr/bin/env python3
"""Offline reader tests. Synthetic mutations, no new GPU evidence."""
from copy import deepcopy
from pathlib import Path
import unittest

import readback as analysis
import publish_closed as publication


class ReaderTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cases = analysis.read(analysis.HERE / 'plan.json')['cases']
        cls.source = analysis.read(analysis.ROOT /
            'morsehgp3D_v9/receipts/g4_q3_payload_20260926/vm/probe_1.stdout')

    def fixtures(self):
        # These are synthetic post-protocol fixtures only. They exercise the
        # additional direct comparisons, not full protocol/geometry validity.
        probes = {i: deepcopy(self.source) for i in range(6)}
        for i, probe in probes.items():
            probe['frames']['chain_total_ms'] = [910.0, 900.0, 890.0, 905.0]
            if i in (1, 2, 5):
                for key in analysis.worker.CERTIFICATE_WORK_KEYS:
                    if 'core' in key:
                        probe['ledger'][key] = 0
        return probes

    def test_plan(self):
        plan = analysis.read(analysis.HERE / 'plan.json')
        manifest = {row['file']: row['sha256'] for row in analysis.worker.INPUTS.values()}
        self.assertEqual(analysis.worker.validate_plan(plan, manifest), self.cases)

    def test_direct_twins_accept_different_work_between_arms(self):
        pairs = analysis.compare_completed(self.cases, self.fixtures())
        self.assertEqual([(p['on'], p['off']) for p in pairs], [(0, 1), (3, 2)])

    def test_off_gpu_work_mutant(self):
        probes = self.fixtures()
        probes[1]['ledger']['dead_uniform_tests'] += 1
        with self.assertRaisesRegex(ValueError, 'direct same-core'):
            analysis.compare_completed(self.cases, probes)

    def test_off_engine_work_mutant(self):
        probes = self.fixtures()
        probes[5]['ledger']['dead_uniform_tests'] += 1
        with self.assertRaisesRegex(ValueError, 'direct same-core'):
            analysis.compare_completed(self.cases, probes)

    def test_object_mutant(self):
        probes = self.fixtures()
        probes[2]['tower_digest'] = '0000000000000000'
        with self.assertRaisesRegex(ValueError, 'object differs'):
            analysis.compare_completed(self.cases, probes)

    def test_incomplete_probes(self):
        probes = self.fixtures()
        del probes[5]
        with self.assertRaisesRegex(ValueError, 'six completed'):
            analysis.compare_completed(self.cases, probes)

    def test_late_frame_failure_is_rejected(self):
        value = deepcopy(self.source)
        for key in analysis.worker.FRAME_LISTS:
            value['frames'][key] *= 4
        value['frames']['results'] = [deepcopy(value['frames']['results'][0]) for _ in range(4)]
        value['frames']['count'] = 4
        self.assertEqual(analysis.worker.validate_probe(value, self.cases[0], 0), 'complete_relative')
        value['frames']['results'][2]['catalogue_digest'] = '0000000000000000'
        self.assertEqual(value['frames']['results'][0]['catalogue_digest'], value['catalogue_digest'])
        with self.assertRaises(ValueError):
            analysis.worker.validate_probe(value, self.cases[0], 0)

    def test_incomplete_outcomes(self):
        outcomes = [{'outcome': 'complete_relative'} for _ in range(6)]
        outcomes[5]['outcome'] = 'skipped_budget'
        with self.assertRaisesRegex(ValueError, 'incomplete experiment'):
            analysis.analyze(self.cases, outcomes, {}, {})

    def test_other_plan_refused(self):
        cases = deepcopy(self.cases)
        cases[0]['s'] = 10
        with self.assertRaisesRegex(ValueError, 'unexpected experiment plan'):
            analysis.analyze(cases, [], {}, {})

    def test_closed_receipt_required(self):
        for status, stopped in [('completed', False), ('capture_failed', True), ('failed', True)]:
            with self.assertRaises(ValueError):
                analysis.evidence.closed_host(dict(status=status, targeted_shutdown_certified=stopped,
                                                  GPU_executed=True, FULL_executed=True))

    def test_publisher_import_is_inert(self):
        self.assertTrue(callable(publication.publish))
        self.assertEqual(publication.copy_helpers.analysis, analysis)


if __name__ == '__main__':
    unittest.main()
