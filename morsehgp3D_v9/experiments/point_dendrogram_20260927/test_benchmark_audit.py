"""Synthetic orchestration gates; never scientific or geometry evidence.

All estimator/routing replacements below are confined to unittest mocks.
The production runner and sealed inherited artifacts are not modified.
"""
from contextlib import ExitStack, redirect_stdout
from copy import deepcopy
from fractions import Fraction
import io
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import benchmark_point_dendrogram as runner


def rows_fixture():
    rows = []
    for case in runner.CASES:
        for m in (20, 50):
            for z in (1, 2):
                methods = (*runner.inherited.formatting.METHODS, runner.METHOD)
                if z == 1:
                    methods += ('hdbscan_standard',)
                for method in methods:
                    rows.append(dict(case=case, k=5, min_cluster_size=m, exp_z=z,
                                     method=method, metrics={'ari_all': 0.0, 'clusters': 0}))
    return rows


class BenchmarkAudit(unittest.TestCase):
    def test_complete_grid_counts(self):
        rows = rows_fixture()
        saved = deepcopy(rows)
        runner.validate_grid(rows)
        self.assertEqual(rows, saved)
        self.assertEqual(len(rows), 234)
        self.assertEqual(sum(row['method'] == runner.METHOD for row in rows), 52)
        self.assertEqual(sum(row['method'] != runner.METHOD for row in rows), 182)
        self.assertEqual(sum(row['method'] == 'hdbscan_standard' for row in rows), 26)

    def test_grid_mutants(self):
        mutations = [lambda rows: rows.pop(),
                     lambda rows: rows.append(deepcopy(rows[0])),
                     lambda rows: rows.__setitem__(-1, deepcopy(rows[0])),
                     lambda rows: rows[0].update(case='unknown'),
                     lambda rows: rows[0].update(k=10),
                     lambda rows: rows[0].update(min_cluster_size=21),
                     lambda rows: rows[0].update(exp_z=3),
                     lambda rows: rows[0].update(method='unknown')]
        for mutate in mutations:
            rows = rows_fixture()
            mutate(rows)
            with self.assertRaises(ValueError):
                runner.validate_grid(rows)

    def test_json_duplicates_and_integer_keys_refused(self):
        self.assertEqual(runner.integer_map({'0': [], '12': [0]}), {0: [], 12: [0]})
        for key in ('00', '+1', '-1', '1.0', '\u0661', 1):
            with self.assertRaises(ValueError):
                runner.integer_map({key: []})
        with self.assertRaises(ValueError):
            runner.unique_object([('n', 1), ('n', 2)])
        with tempfile.TemporaryDirectory(prefix='mhgp9-point-runner-json-') as directory:
            path = Path(directory) / 'duplicate.json'
            path.write_text('{"n":1,"n":2}\n')
            with self.assertRaises(ValueError):
                runner.read(path)

    def test_source_inventory_excludes_this_additional_test(self):
        pins = runner.source_inventory()
        self.assertNotIn(str(Path(__file__).resolve()), pins)
        for name in ('benchmark_point_dendrogram.py', 'point_tree.py', 'point_eom.py',
                     'test_point_tree.py', 'test_point_eom_audit.py', 'PLAN.md'):
            path = runner.HERE / name
            self.assertEqual(pins[str(path)], runner.sha(path))

    def test_live_pin_changes_refused(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-point-runner-pin-') as directory:
            path = Path(directory) / 'pin'
            path.write_text('initial')
            pins = {str(path): runner.sha(path)}
            runner.check_pins(pins)
            path.write_text('changed')
            with self.assertRaises(ValueError):
                runner.check_pins(pins)

    def test_no_overwrite(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-point-runner-existing-') as directory:
            folder = Path(directory)
            marker = folder / 'keep'
            marker.write_text('existing capture')
            args = SimpleNamespace(output=folder, capture=folder, post_audit=folder/'post',
                                   qualification=folder/'qualification')
            with self.assertRaises(ValueError):
                runner.run(args)
            self.assertEqual(marker.read_text(), 'existing capture')

    def test_truth_read_after_projection_and_both_selections(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-point-runner-order-') as directory:
            folder = Path(directory)
            events = []
            truth_path, measure_path = folder/'truth.json', folder/'measure.json'
            case = dict(id='synthetic', n=1200, labels_json=str(truth_path), regime='synthetic',
                        communities=1, separation=1, seed=0)
            measure = dict(facets=[[0,1,2,3,4],[5,6,7,8,9]], scores=[0.1,0.3],
                           children={'2':[0,1]}, squared_levels={'2':{'num':'4','den':'1'}},
                           leaf_birth_betas=[{'num':'0','den':'1'}]*2)
            tree = dict(squared_levels={1200:Fraction(4)}, statistics={'synthetic': True})

            def read(path):
                if str(path) == str(measure_path):
                    events.append('measure')
                    return deepcopy(measure)
                self.assertEqual(str(path), str(truth_path))
                self.assertIn('select20', events)
                self.assertIn('select50', events)
                events.append('truth')
                return [0]*1200

            def route(*args):
                self.assertEqual(len(args), 7)
                self.assertEqual(args[0], 1200)
                self.assertEqual(args[2], [Fraction.from_float(0.1), Fraction.from_float(0.3)])
                events.append('route')
                return dict(statistics={'synthetic': True}, attachments=[{'reason':'synthetic'}])

            def select(raw, *, min_cluster_size, exp_z):
                self.assertNotIn('truth', events)
                self.assertIs(raw, tree)
                self.assertEqual(exp_z, 2)
                events.append('select'+str(min_cluster_size))
                return {'result': {'selection': {'labels': [0]*1200}}}

            def evaluate(*args, **kwargs):
                self.assertEqual(events[-1], 'truth')
                return {'synthetic': True}

            with ExitStack() as stack:
                stack.enter_context(patch.object(runner, 'read', side_effect=read))
                stack.enter_context(patch.object(runner, 'route_points', side_effect=route))
                stack.enter_context(patch.object(runner, 'build_point_tree', return_value=tree))
                stack.enter_context(patch.object(runner, 'validate_point_tree'))
                stack.enter_context(patch.object(runner, 'cut', return_value=[[0,1]]))
                stack.enter_context(patch.object(runner, 'routing_cut', return_value=[[0,1]]))
                stack.enter_context(patch.object(runner, 'cluster_point_tree', side_effect=select))
                stack.enter_context(patch.object(runner, 'payload'))
                stack.enter_context(patch.object(runner, 'sha', return_value='0'*64))
                stack.enter_context(patch.object(runner, 'metrics', side_effect=evaluate))
                stack.enter_context(patch.object(runner, 'evaluate_labels', side_effect=evaluate))
                unit = runner.build_unit(case, 2, measure_path, folder)
            self.assertEqual(events, ['measure','route','select20','select50','truth'])
            self.assertEqual(len(unit['rows']), 2)
            self.assertEqual(len(unit['checks']), 6)

    def test_invalid_source_score_never_reaches_routing(self):
        measure = dict(facets=[[0,1,2,3,4]], scores=[1.0])
        for bad in (0.0, -1.0, float('nan'), float('inf'), 1):
            changed = deepcopy(measure); changed['scores'] = [bad]
            with patch.object(runner, 'read', return_value=changed), \
                 patch.object(runner, 'route_points') as route:
                with self.assertRaises(ValueError):
                    runner.build_unit({'n':1200}, 1, Path('/synthetic'), Path('/synthetic'))
                route.assert_not_called()

    def synthetic_run(self, error=None, wrong_parent=False):
        """Exercise only the runner ledger; all scientific/provenance boundaries are mocked."""
        with tempfile.TemporaryDirectory(prefix='mhgp9-point-runner-synthetic-') as directory:
            folder = Path(directory)
            capture = folder/'inherited'; capture.mkdir()
            original = capture/'receipt.json'; original.write_text('{"synthetic_only":true}\n')
            post = folder/'post.json'; post.write_text('{"synthetic_only":true}\n')
            qualification = folder/'qualification.json'
            qualification.write_text('{"synthetic_only":true}\n')
            manifest = folder/'manifest.json'
            manifest.write_text(json.dumps({'cases':[{'id':case,'n':1200} for case in runner.CASES]}))
            artifacts = {}
            for case in runner.CASES:
                unit_dir = capture/f'{case}_k5'; unit_dir.mkdir()
                for z in (1,2):
                    path = unit_dir/f'measure_z{z}.json.gz'
                    path.write_bytes(b'synthetic, never decompressed by these ledger tests')
                    artifacts[str(path)] = runner.sha(path)
            all_rows = rows_fixture()
            old = dict(rows=[r for r in all_rows if r['method'] != runner.METHOD],
                       manifest=str(manifest), artifacts=artifacts)
            old_saved = deepcopy(old)
            original_sha = runner.sha(original)
            args = SimpleNamespace(output=folder/'new', capture=capture, post_audit=post,
                                   qualification=qualification)

            def unit(case, exponent, source, target):
                (target/'synthetic_partial.txt').write_text('kept if interrupted')
                if error is not None:
                    raise error
                rows = [r for r in all_rows if r['method'] == runner.METHOD and
                        r['case'] == case['id'] and r['exp_z'] == exponent]
                return dict(case=case['id'], k=5, exp_z=exponent, status='completed', rows=rows,
                            times_ms={'routing_reference':0.0}, synthetic_only=True)

            with ExitStack() as stack, redirect_stdout(io.StringIO()):
                stack.enter_context(patch.object(runner, 'CAPTURE_SHA', 'f'*64 if wrong_parent else original_sha))
                stack.enter_context(patch.object(runner, 'POST_SHA', runner.sha(post)))
                stack.enter_context(patch.object(runner, 'MANIFEST_SHA', runner.sha(manifest)))
                stack.enter_context(patch.object(runner, 'QUALIFICATION_SHA', runner.sha(qualification)))
                stack.enter_context(patch.object(runner, 'validate_qualification',
                    return_value={str(qualification):runner.sha(qualification)}))
                stack.enter_context(patch.object(runner.inherited, 'validated_inputs',
                    return_value=(old, None, None, {str(original):original_sha}, None)))
                stack.enter_context(patch.object(runner, 'source_inventory',
                    return_value={str(Path(__file__).resolve()):runner.sha(__file__)}))
                build = stack.enter_context(patch.object(runner, 'build_unit', side_effect=unit))
                if error is not None or wrong_parent:
                    with self.assertRaises(ValueError if wrong_parent else type(error)):
                        runner.run(args)
                else:
                    runner.run(args)
            receipt = runner.read(args.output/'receipt.json')
            self.assertEqual(runner.sha(original), original_sha)
            self.assertEqual(old, old_saved)
            self.assertEqual(receipt['sources_before'], receipt['sources_after'])
            return receipt, build.call_count, list(args.output.glob('*/synthetic_partial.txt')) != []

    def test_synthetic_complete_ledger_keeps_inherited_rows(self):
        receipt, calls, _ = self.synthetic_run()
        self.assertEqual(receipt['status'], 'completed')
        self.assertEqual((calls, len(receipt['units']), len(receipt['rows'])), (26,26,234))
        expected = [row for row in rows_fixture() if row['method'] != runner.METHOD]
        self.assertEqual(receipt['rows'][:182], expected)
        self.assertFalse(receipt['geometry_rerun'])
        self.assertFalse(receipt['hdbscan_fits_rerun'])

    def test_error_and_keyboard_interrupt_are_preserved(self):
        for error in (RuntimeError('synthetic failure'), KeyboardInterrupt('synthetic interruption')):
            receipt, calls, partial_exists = self.synthetic_run(error=error)
            self.assertEqual(receipt['status'], 'failed')
            self.assertEqual((calls, len(receipt['units']), len(receipt['rows'])), (1,0,182))
            self.assertIn(type(error).__name__, receipt['error'])
            self.assertIn(str(error), receipt['traceback'])
            self.assertTrue(partial_exists)

    def test_wrong_inherited_hash_refused_before_science(self):
        receipt, calls, partial_exists = self.synthetic_run(wrong_parent=True)
        self.assertEqual(receipt['status'], 'failed')
        self.assertEqual(calls, 0)
        self.assertFalse(partial_exists)


if __name__ == '__main__':
    unittest.main()
