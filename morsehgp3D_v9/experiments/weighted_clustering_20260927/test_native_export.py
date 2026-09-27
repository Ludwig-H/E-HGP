#!/usr/bin/env python3
"""Bounded CLI/schema tests; geometry oracle is qualify_geometry.py.

Run --native /absolute/path/native_weighted_export, normally and with -O.
All inputs live in a fresh temporary directory; no source/build is modified.
"""
import argparse
from fractions import Fraction
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

NATIVE = None


class NativeExportTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='weighted-export-cli-')
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)

    def invoke(self, *args):
        return subprocess.run([str(NATIVE), *map(str, args)], capture_output=True, timeout=60, check=False)

    def points(self, points, name='points.u32le'):
        path = self.directory / name
        path.write_bytes(b''.join(struct.pack('<III', *p) for p in points))
        return path

    def test_gate(self):
        result = self.invoke('--gate')
        self.assertEqual(result.returncode, 0, result.stderr.decode())
        gate = json.loads(result.stdout)
        self.assertEqual(gate['status'], 'passed')
        for key in ('cases', 'exhaustive_subsets', 'cofaces', 'rejected_center_masks', 'coverage_queries'):
            self.assertGreater(gate[key], 0)
        self.assertTrue(gate['unsupported_shell_checked'])

    def test_complete_schema_and_parallel_identity(self):
        points = [(2, 2, 0), (0, 0, 0), (0, 2, 0), (2, 0, 0)]
        path = self.points(points)
        outputs = []
        for workers in (1, 2):
            result = self.invoke('--input', path, '--k', 2, '--workers', workers, '--verify-coverage')
            self.assertEqual(result.returncode, 0, result.stderr.decode())
            self.assertEqual(result.stderr, b'')
            data = json.loads(result.stdout)
            self.assertEqual(data['schema'], 'mhgp9_weighted_catalogue_export_v1')
            self.assertEqual(data['catalog_universe'], 'gabriel_complete_boundary')
            self.assertEqual(data['native']['points'], [list(p) for p in points])
            self.assertTrue(data['native']['validation']['all_cut_coverage_replayed'])
            self.assertFalse(data['native']['facet_catalogue_exported'])  # frozen sub-object only
            self.assertEqual(data['stats']['global_combinations_enumerated'], 0)
            self.assertEqual(len(data['cofaces']), 4)
            self.assertEqual(len(data['catalogue']), data['stats']['catalogue_balls'])
            seen = set()
            for coface in data['cofaces']:
                vertices = tuple(coface['vertices'])
                self.assertEqual(vertices, tuple(sorted(set(vertices))))
                self.assertEqual(len(vertices), 3)
                self.assertNotIn(vertices, seen)
                seen.add(vertices)
                ball = data['catalogue'][coface['ball']]
                self.assertEqual(ball['id'], coface['ball'])
                self.assertEqual(ball['beta'], coface['beta'])
                beta = coface['beta']
                self.assertEqual(Fraction(int(beta['num']), int(beta['den'])), 2)
                selected = [p for bit, p in enumerate(ball['shell']) if coface['shell_mask'] & (1 << bit)]
                self.assertEqual(sorted(ball['interior'] + selected), list(vertices))
                self.assertTrue(any(mask & coface['shell_mask'] == mask for mask in ball['minimal_support_masks']))
            outputs.append(data)
        for key in ('catalogue', 'cofaces'):
            self.assertEqual(outputs[0][key], outputs[1][key])
        for key in ('tower_digest', 'catalogue_digest', 'presentation_digest'):
            self.assertEqual(outputs[0]['native'][key], outputs[1]['native'][key])

    def test_refusals_no_success_json(self):
        valid = self.points([(0, 0, 0), (1, 0, 0), (0, 1, 0)])
        duplicate = self.points([(0, 0, 0), (0, 0, 0)], 'duplicate.u32le')
        outside = self.points([(0, 0, 0), (262144, 0, 0)], 'outside.u32le')
        truncated = self.directory / 'truncated.u32le'
        truncated.write_bytes(b'\x00' * 13)
        commands = [[], ['--input', valid], ['--input', valid, '--k', '0'],
                    ['--input', valid, '--k', '11'], ['--input', valid, '--k', '4'],
                    ['--input', valid, '--k', '2', '--k', '2'],
                    ['--input', valid, '--k', '2', '--workers', '0'],
                    ['--input', valid, '--k', '2', '--unknown'],
                    ['--input', duplicate, '--k', '1'], ['--input', outside, '--k', '1'],
                    ['--input', truncated, '--k', '1']]
        for args in commands:
            with self.subTest(args=list(map(str, args))):
                result = self.invoke(*args)
                self.assertEqual(result.returncode, 2)
                self.assertEqual(result.stdout, b'')
                self.assertIn(b'native_weighted_export:', result.stderr)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    args, remainder = parser.parse_known_args()
    NATIVE = args.native.resolve()
    if not NATIVE.is_file():
        raise RuntimeError('native executable missing')
    unittest.main(argv=[__file__, *remainder])
