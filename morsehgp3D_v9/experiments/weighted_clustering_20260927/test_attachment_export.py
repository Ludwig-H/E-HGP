#!/usr/bin/env python3
"""Tiny attachment CLI/schema gates. Independent Fraction oracle is separate."""
import argparse
from fractions import Fraction
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

BINARY = None


def beta(row):
    return Fraction(int(row['num']), int(row['den']))


class AttachmentTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='weighted-attachment-cli-')
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name)/'points.u32le'

    def invoke(self, *args):
        return subprocess.run([str(BINARY), *map(str, args)], capture_output=True, timeout=60, check=False)

    def write_points(self, points):
        self.path.write_bytes(b''.join(struct.pack('<III', *p) for p in points))

    def test_native_gate(self):
        result = self.invoke('--gate')
        self.assertEqual(result.returncode, 0, result.stderr.decode())
        data = json.loads(result.stdout)
        self.assertEqual(data['status'], 'passed')
        self.assertTrue(data['square_silent_anchor'])
        self.assertTrue(data['e5_silent_attachment'])
        self.assertGreater(data['exchange_steps'], 0)

    def test_schema_closed_normalization_and_workers(self):
        self.write_points([(0,0,7), (0,9,6), (1,4,0), (0,0,1), (4,1,2)])
        values = []
        for k, workers in ((1,1), (2,1), (2,2)):
            result = self.invoke('--input', self.path, '--k', k, '--workers', workers, '--verify-coverage')
            self.assertEqual(result.returncode, 0, result.stderr.decode())
            self.assertEqual(result.stderr, b'')
            data = json.loads(result.stdout)
            self.assertEqual(data['schema'], 'mhgp9_weighted_full_attachment_export_v1')
            validation = data['validation']
            self.assertTrue(validation['observed_native_every_field_equal'])
            self.assertEqual(validation['native_tower_digest'], validation['observed_tower_digest'])
            native = data['weighted']['native']
            if k == 1:
                self.assertEqual(data['anchors'], [])
                self.assertFalse(validation['anchors_available'])
            else:
                self.assertEqual(len(data['anchors']), len(data['weighted']['catalogue']))
                self.assertTrue(validation['all_capture_slots_admitted'])
            facets = sorted({tuple(v for v in c['vertices'] if v != omitted)
                             for c in data['weighted']['cofaces'] for omitted in c['vertices']})
            self.assertEqual([tuple(r['vertices']) for r in data['attachments']], facets)
            for row in data['attachments']:
                node, cut = row['anchor_node'], beta(row['beta'])
                if k == 1:
                    self.assertIsNone(row['terminal_ball'])
                    self.assertEqual(cut, 0)
                    self.assertEqual(row['node'], row['vertices'][0])
                else:
                    self.assertEqual(node, data['anchors'][row['terminal_ball']])
                    self.assertLessEqual(beta(data['weighted']['catalogue'][row['terminal_ball']]['beta']), cut)
                while native['nodes'][node]['successor'] is not None:
                    successor = native['nodes'][node]['successor']
                    if beta(native['nodes'][successor]['level']) > cut:
                        break
                    node = successor
                self.assertEqual(node, row['node'])
            if k == 2:
                ac = next(r for r in data['attachments'] if r['vertices'] == [0,2])
                self.assertEqual(beta(ac['beta']), Fraction(33,2))
                self.assertGreater(ac['descending_steps'], 0)
                values.append(data)
        self.assertEqual(values[0]['attachments'], values[1]['attachments'])
        self.assertEqual(values[0]['anchors'], values[1]['anchors'])

    def test_refusals(self):
        self.write_points([(0,0,0), (1,0,0), (0,1,0)])
        commands = [[], ['--input', self.path], ['--input', self.path, '--k', 0],
                    ['--input', self.path, '--k', 11], ['--input', self.path, '--k', 4],
                    ['--input', self.path, '--k', 2, '--k', 2],
                    ['--input', self.path, '--k', 2, '--workers', 0],
                    ['--input', self.path, '--k', 2, '--unknown']]
        for command in commands:
            with self.subTest(command=command):
                result = self.invoke(*command)
                self.assertEqual(result.returncode, 2)
                self.assertEqual(result.stdout, b'')
                self.assertIn(b'native_attachment_export:', result.stderr)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    args, extra = parser.parse_known_args()
    BINARY = args.native.resolve()
    if not BINARY.is_file():
        raise RuntimeError('native binary missing')
    unittest.main(argv=[__file__, *extra])
