#!/usr/bin/env python3
"""Tiny independent interval oracle and SQLite fixture; no GPU or cloud."""
from contextlib import redirect_stdout
import io
import json
from pathlib import Path
import random
import sqlite3
import tempfile
import unittest

from analyze_trace import Intervals, analyze, family, main, summarize


class AnalysisTests(unittest.TestCase):
    def test_nested_overlap_adjacency_and_gap(self):
        intervals = Intervals()
        for pair in [(0, 100), (10, 20), (30, 40), (100, 110), (120, 130)]:
            intervals.add(*pair)
        result = intervals.result()
        self.assertEqual((result['summed_ns'], result['union_ns'], result['internal_gaps_ns']), (140, 120, 10))
        self.assertEqual(result['largest_internal_gaps'], [dict(duration_ns=10, start_ns=110, end_ns=120)])

    def test_independent_integer_cell_oracle(self):
        rng = random.Random(20260927)
        for _ in range(100):
            pairs = sorted((x, x+rng.randrange(12)) for x in (rng.randrange(-5, 30) for _ in range(20)))
            covered = {x for start, end in pairs for x in range(start, end)}
            intervals = Intervals()
            for pair in pairs:
                intervals.add(*pair)
            result = intervals.result()
            self.assertEqual(result['union_ns'], len(covered))
            self.assertEqual(result['span_ns'], result['union_ns']+result['internal_gaps_ns'])

    def test_reject_invalid_and_unsorted(self):
        intervals = Intervals()
        for values in [(2, 1), (True, 2), (1, 2, -1)]:
            with self.assertRaises(ValueError):
                intervals.add(*values)
        intervals.add(10, 20)
        with self.assertRaises(ValueError):
            intervals.add(9, 30)

    def test_sqlite_readonly_kinds_devices_and_lazy_table(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-nsys-analysis-') as folder:
            path = Path(folder) / 'trace.sqlite'
            with sqlite3.connect(path) as connection:
                connection.executescript('''
                    CREATE TABLE StringIds(id INTEGER PRIMARY KEY,value TEXT);
                    INSERT INTO StringIds VALUES(1,'mhgp9::pair_kernel<false>'),(2,'mhgp9::certificate_kernel');
                    CREATE TABLE CUPTI_ACTIVITY_KIND_KERNEL(deviceId INTEGER,start INTEGER,end INTEGER,demangledName INTEGER);
                    INSERT INTO CUPTI_ACTIVITY_KIND_KERNEL VALUES(0,0,100,1),(0,10,20,2),(1,0,40,1);
                    CREATE TABLE CUPTI_ACTIVITY_KIND_MEMCPY(deviceId INTEGER,start INTEGER,end INTEGER,bytes INTEGER,copyKind INTEGER);
                    INSERT INTO CUPTI_ACTIVITY_KIND_MEMCPY VALUES(0,90,110,1024,1),(0,120,130,2048,2);
                ''')
            before = path.read_bytes()
            report = analyze(path)
            self.assertEqual(path.read_bytes(), before)
            self.assertEqual(sorted(p.name for p in Path(folder).iterdir()), ['trace.sqlite'])
            device = report['devices']['0']
            self.assertEqual(device['all_activity']['union_ns'], 120)
            self.assertEqual(device['all_activity']['bytes'], 3072)
            self.assertEqual(device['all_activity']['internal_gaps_ns'], 10)
            self.assertEqual(device['kernel_families']['S2_filter']['summed_ns'], 100)
            self.assertEqual(report['devices']['1']['all_activity']['union_ns'], 40)
            self.assertTrue(any('MEMSET absent' in warning for warning in report['warnings']))

    def test_names_and_missing_activity(self):
        self.assertEqual(family('cub::DeviceScanKernel'), 'CUB_unassigned')
        self.assertEqual(family('mhgp9::lanes_task_kernel'), 'S4_lanes')
        self.assertEqual(family('unrelated_pair_kernel'), 'other_kernel')
        with sqlite3.connect(':memory:') as connection:
            with self.assertRaises(ValueError):
                summarize(connection)

    def test_memset_overlap_and_unresolved_kernel(self):
        with sqlite3.connect(':memory:') as connection:
            connection.executescript('''
                CREATE TABLE CUPTI_ACTIVITY_KIND_KERNEL(deviceId INTEGER,start INTEGER,end INTEGER);
                INSERT INTO CUPTI_ACTIVITY_KIND_KERNEL VALUES(0,0,10);
                CREATE TABLE CUPTI_ACTIVITY_KIND_MEMSET(deviceId INTEGER,start INTEGER,end INTEGER,bytes INTEGER);
                INSERT INTO CUPTI_ACTIVITY_KIND_MEMSET VALUES(0,5,20,32);
            ''')
            report = summarize(connection)
            self.assertEqual(report['devices']['0']['all_activity']['union_ns'], 20)
            self.assertEqual(report['devices']['0']['kinds']['memset']['summed_ns'], 15)
            self.assertEqual(report['devices']['0']['all_activity']['bytes'], 32)
            self.assertTrue(any('unresolved kernel' in warning for warning in report['warnings']))

    def test_output_exclusive_and_stdout_default(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-nsys-output-') as folder:
            path = Path(folder) / 'trace.sqlite'
            output = Path(folder) / 'analysis.json'
            with sqlite3.connect(path) as connection:
                connection.executescript('''
                    CREATE TABLE CUPTI_ACTIVITY_KIND_MEMSET(deviceId INTEGER,start INTEGER,end INTEGER,bytes INTEGER);
                    INSERT INTO CUPTI_ACTIVITY_KIND_MEMSET VALUES(0,0,10,32);
                ''')
            stdout = io.StringIO()
            with redirect_stdout(stdout):
                main([str(path), '--output', str(output)])
            self.assertEqual(stdout.getvalue(), '')
            original = output.read_bytes()
            self.assertEqual(json.loads(original)['devices']['0']['all_activity']['union_ns'], 10)
            with self.assertRaises(FileExistsError):
                main([str(path), '--output', str(output)])
            self.assertEqual(output.read_bytes(), original)
            with redirect_stdout(stdout):
                main([str(path)])
            self.assertEqual(stdout.getvalue().encode(), original)


if __name__ == '__main__':
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(AnalysisTests)
    result = unittest.TextTestRunner(verbosity=1).run(suite)
    print(json.dumps(dict(status='PASS' if result.wasSuccessful() else 'FAIL', tests=result.testsRun,
                          geometry_executed=False, GPU_executed=False, GCP_used=False), sort_keys=True))
    raise SystemExit(0 if result.wasSuccessful() else 1)
