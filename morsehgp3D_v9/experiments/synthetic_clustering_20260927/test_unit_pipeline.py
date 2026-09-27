"""Tiny real-Python integrations, with native transport replayed, never executed.

The two small K5 outputs are already-qualified independent native fixtures.
All measures, FULL attachment trees, routes, point trees, EOM, HDBSCAN and
metrics are recomputed here. These n8 fixtures do not test clustering quality.
"""
from __future__ import annotations

import copy
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import numpy as np
import unit_pipeline as unit

PROOF = Path('/workspaces/E-HGP/build/v9-weighted-full-qualification-20260927-r1')
PROOF_SHA = '35126c2a591a4d410b44d86a80967b50f4574bcdb9cadc5d94d9a982bafaba9c'
FIXTURES = ('random8_seed1_k5', 'random8_seed2_k5')


def proof_paths():
    """Expose every historical fixture receipt/stream consulted to parent pins."""
    return [PROOF/'receipt.json'] + [PROOF/(name+suffix) for name in FIXTURES
        for suffix in ('.command.json', '.stdout', '.stderr', '.u32le')]


def fixture(name):
    if unit.sha(PROOF/'receipt.json') != PROOF_SHA:
        raise ValueError('historical fixture qualification changed')
    proof = json.loads((PROOF/'receipt.json').read_text())
    if proof['status'] != 'passed' or proof['sources_before'] != proof['sources_after']:
        raise ValueError('closed historical qualification required')
    for suffix in ('.command.json', '.stdout', '.stderr', '.u32le'):
        if unit.sha(PROOF/(name+suffix)) != proof['artifacts'][name+suffix]:
            raise ValueError('fixture artifact changed')
    command = json.loads((PROOF/(name+'.command.json')).read_text())
    if command['returncode'] != 0:
        raise ValueError('successful historical fixture command required')
    binary = Path(proof['native_binary'])
    if unit.sha(binary) != proof['native_binary_sha256'] or unit.sha(binary) != unit.NATIVE_SHA:
        raise ValueError('historical native identity')
    return (PROOF/(name+'.stdout')).read_bytes(), binary


class Pipeline(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='mhgp9-unit-pipeline-test-')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)

    def case(self, name=FIXTURES[0]):
        raw, binary = fixture(name)
        export = json.loads(raw)
        points = np.asarray(export['weighted']['native']['points'], dtype='<u4')
        n = len(points)
        paths = dict(points_u32le=self.root/'points.u32le', points_npy=self.root/'points.npy',
                     labels_json=self.root/'labels.json')
        paths['points_u32le'].write_bytes(points.tobytes())
        np.save(paths['points_npy'], points.astype(np.float64), allow_pickle=False)
        paths['labels_json'].write_text(json.dumps([-1, -1]+[1]*3+[2]*(n-5)))
        case = dict(id=name, n=n, groups=2, phase='quality', axis='test', replicate=1,
                    spec=dict(n=n, groups=2, separation=4, seed=1, family='spherical', noise_fraction=.1),
                    **{k:str(v) for k,v in paths.items()},
                    prepared_sha256={k:unit.sha(v) for k,v in paths.items()})
        return case, raw, binary

    def transport(self, case, raw, binary, returncode=0):
        def run(argv, *, stdout, stderr, check):
            self.assertEqual(argv, [str(binary.resolve()), '--input', case['points_u32le'],
                                    '--k', '5', '--workers', '1'])
            self.assertFalse(check)
            stdout.write(raw)
            if returncode:
                stderr.write(b'intentional transport failure\n')
            return subprocess.CompletedProcess(argv, returncode)
        return run

    def test_two_integrations_and_truth_separation(self):
        for index, name in enumerate(FIXTURES):
            case, raw, binary = self.case(name)
            output = self.root/f'capture{index}'
            original_read = Path.read_text
            reads = []
            def guarded_read(path, *args, **kwargs):
                if path == Path(case['labels_json']):
                    predictions = [p for p in output.glob('*.json.gz')
                                   if p.name.startswith((*unit.METHODS, 'hdbscan_standard'))]
                    self.assertEqual(len(predictions), 18, 'truth read before all predictions saved')
                    reads.append(str(path))
                return original_read(path, *args, **kwargs)
            with patch.object(unit.subprocess, 'run', side_effect=self.transport(case, raw, binary)) as call:
                with patch.object(Path, 'read_text', guarded_read):
                    receipt = unit.run_unit(case, output, binary)
            self.assertEqual(call.call_count, 1)
            self.assertEqual(len(reads), 1)
            self.assertEqual(receipt['status'], 'completed')
            self.assertEqual(receipt['sources_before'], receipt['sources_after'])
            self.assertEqual(len(receipt['hdbscan_fits']), 2)
            self.assertEqual(len(receipt['units']), 2)
            self.assertEqual(len(receipt['rows']), 18)
            unit.validate_rows(receipt['rows'], name)
            unit.verify(receipt['artifacts'], 'test saved artifact')
            truth = json.loads(original_read(Path(case['labels_json'])))
            for row in receipt['rows']:
                with gzip.open(row['labels_payload'], 'rt') as stream:
                    labels = json.load(stream)['labels']
                self.assertEqual(len(labels), case['n'])
                self.assertEqual(row['metrics'], unit.metrics(truth, labels))
                minimum = None if row['method'] == 'hgp_weighted_full_vote' else row['min_cluster_size']
                self.assertEqual(row['extra'], unit.evaluate_labels(truth, labels, min_cluster_size=minimum))
                self.assertEqual(row['spec'], case['spec'])
            disk = json.loads((output/'receipt.json').read_text())
            self.assertEqual(disk['status'], 'completed')
            self.assertGreater(disk['elapsed_seconds'], 0)

    def test_geometry_corruption_refused_before_native(self):
        case, _, binary = self.case()
        np.save(case['points_npy'], np.zeros((case['n'], 3), dtype=np.float64))
        # Even with a refreshed declared hash, coordinate equality must fail.
        case['prepared_sha256']['points_npy'] = unit.sha(case['points_npy'])
        with patch.object(unit.subprocess, 'run') as call:
            with self.assertRaisesRegex(ValueError, 'same exact'):
                unit.run_unit(case, self.root/'bad_input', binary)
        call.assert_not_called()
        self.assertEqual(json.loads((self.root/'bad_input/receipt.json').read_text())['status'], 'failed')

    def test_native_failure_is_preserved(self):
        case, raw, binary = self.case()
        output = self.root/'failed'
        with patch.object(unit.subprocess, 'run', side_effect=self.transport(case, raw, binary, 2)):
            with self.assertRaisesRegex(ValueError, 'native exporter failed'):
                unit.run_unit(case, output, binary)
        receipt = json.loads((output/'receipt.json').read_text())
        self.assertEqual(receipt['status'], 'failed')
        self.assertEqual(receipt['commands'][0]['returncode'], 2)
        self.assertEqual((output/'native.json').read_bytes(), raw)
        self.assertIn('intentional', (output/'native.stderr').read_text())

    def test_binding_mutants(self):
        case, raw, binary = self.case()
        payload = json.loads(raw)
        points = np.asarray(payload['weighted']['native']['points'], dtype='<u4')
        mutations = []
        changed = copy.deepcopy(payload); changed['validation']['observed_native_every_field_equal'] = False
        mutations.append(changed)
        changed = copy.deepcopy(payload); changed['weighted']['native']['points'][0][0] += 1
        mutations.append(changed)
        changed = copy.deepcopy(payload); changed['attachments'][0]['node'] = -1
        mutations.append(changed)
        changed = copy.deepcopy(payload); changed['validation']['observed_tower_digest'] = 'wrong'
        mutations.append(changed)
        for changed in mutations:
            with self.assertRaises(ValueError):
                unit.validate_export(changed, points)

    def test_row_grid_and_fresh_directory(self):
        case, _, binary = self.case()
        output = self.root/'exists'; output.mkdir()
        sentinel = output/'keep'; sentinel.write_text('unchanged')
        with self.assertRaises(FileExistsError):
            unit.run_unit(case, output, binary)
        self.assertEqual(sentinel.read_text(), 'unchanged')
        rows = [dict(case=case['id'], k=5, min_cluster_size=m, exp_z=z, method=method)
                for m in unit.SIZES for z in unit.EXPONENTS for method in unit.METHODS]
        rows += [dict(case=case['id'], k=5, min_cluster_size=m, exp_z=1, method='hdbscan_standard')
                 for m in unit.SIZES]
        unit.validate_rows(rows, case['id'])
        rows[-1] = rows[0]
        with self.assertRaises(ValueError):
            unit.validate_rows(rows, case['id'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
