"""Small synthetic format gates; these are not native/statistical evidence."""
from copy import deepcopy
from pathlib import Path
import tempfile
import unittest

import normalize_full_weighted_publication as normalizer


class NormalizationTests(unittest.TestCase):
    def test_only_trailing_ascii_and_line_endings(self):
        before = 'é  milieu\ttexte \t\r\n  \nfin  '.encode()
        after = 'é  milieu\ttexte\r\n\nfin'.encode()
        self.assertEqual(normalizer.normalize(before), after)
        self.assertEqual(normalizer.normalize(after), after)
        self.assertEqual(normalizer.normalize(b'\r\n\n'), b'\r\n\n')
        self.assertEqual(normalizer.normalize('x\u00a0\n'.encode()), 'x\u00a0\n'.encode())
        self.assertEqual(normalizer.normalize(b'a \rb'), b'a \rb')

    def test_non_utf8_refused(self):
        with self.assertRaises(UnicodeDecodeError):
            normalizer.normalize(b'\xff \n')

    def test_frozen_formatter(self):
        self.assertEqual(normalizer.sha(normalizer.FORMATTER), normalizer.FORMATTER_SHA)

    def test_exact_proof_and_mutants(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-format-synthetic-') as directory:
            source, target = Path(directory) / 'source', Path(directory) / 'target'
            source.mkdir(); target.mkdir()
            for name in ('rows.csv', 'aggregates.csv'):
                (source / name).write_bytes(b'a,b\r\n1,2\r\n')
                (target / name).write_bytes(b'a,b\r\n1,2\r\n')
            (source / 'TABLES.md').write_bytes(b'# heading \n\nunchanged  text\t\r\n')
            expected = normalizer.normalize((source / 'TABLES.md').read_bytes())
            (target / 'TABLES.md').write_bytes(expected)
            proof = normalizer.transformation_proof(source, target)
            self.assertEqual((proof['changed_lines'], proof['removed_bytes']), (2, 2))
            self.assertTrue(all(proof['csv_byte_identical'].values()))
            for bad in (expected.replace(b'unchanged', b'changed'),
                        expected.replace(b'  text', b' text'),
                        expected.replace(b'\r\n', b'\n'), expected + b'\n'):
                (target / 'TABLES.md').write_bytes(bad)
                with self.assertRaises(ValueError):
                    normalizer.transformation_proof(source, target)
            (target / 'TABLES.md').write_bytes(expected)
            (target / 'rows.csv').write_bytes(b'a,b\n1,2\n')
            with self.assertRaises(ValueError):
                normalizer.transformation_proof(source, target)

    def test_inventory_and_no_overwrite(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-format-inventory-') as directory:
            folder = Path(directory)
            for name in (*normalizer.PAYLOADS, 'receipt.json'):
                (folder / name).write_bytes(b'{}\n')
            normalizer.inventory(folder)
            with self.assertRaises(ValueError):
                normalizer.publish(folder, '0' * 64, folder)
            (folder / 'extra').write_bytes(b'')
            with self.assertRaises(ValueError):
                normalizer.inventory(folder)

    def test_parent_metadata_and_refusals(self):
        same = ('plan', 'scope', 'root_policy', 'note', 'elapsed_seconds',
                'native_binary', 'native_binary_sha256', 'commands', 'worker_commands',
                'reuse_verification', 'worker_environment', 'baseline_receipt')
        receipt = {name: {'synthetic': name} for name in same}
        receipt.update(sources_after={}, manifest='synthetic')
        qualification, orchestration, pins = {'synthetic': 'qualification'}, {'synthetic': 'orchestration'}, {}
        post = dict(schema='synthetic', capture='synthetic', receipt_sha256='synthetic',
                    pins_sha256={}, exact_ARI={}, status='passed')
        parent = dict(receipt)
        parent.update(sources={}, input_manifest='synthetic',
                      baseline_receipt_sha256=normalizer.common.BASELINE_SHA,
                      input_manifest_sha256=normalizer.common.MANIFEST_SHA,
                      qualification=qualification, orchestration=orchestration,
                      post_audit=dict(schema='synthetic', status='passed',
                                      private_receipt='synthetic', receipt_sha256='synthetic'),
                      live_pins_checked=0, live_pin_map_sha256=normalizer.common.map_digest({}),
                      row_count=364, aggregate_count=140, command_count=26,
                      approximate_postprocessing=True, selected_subset_after_previous_scores=True,
                      live_before_and_after=True)
        flags = ('original_failure_promoted', 'raw_clouds_or_native_arrays_published',
                 'fits_or_geometry_rerun', 'EOM_or_metrics_rerun', 'serial_speedup_claimed',
                 'memory_bound_claimed', 'GCP_used', 'GPU_used')
        parent.update({key: False for key in flags})
        normalizer.validate_metadata(parent, receipt, post, qualification, pins, orchestration)
        mutations = [lambda d: d.update(row_count=363), lambda d: d.update(aggregate_count=139),
                     lambda d: d.update(qualification={}), lambda d: d.update(orchestration={}),
                     lambda d: d.update(live_pins_checked=1), lambda d: d.update(scope='changed'),
                     lambda d: d.update(sources={'different': 'pin'}),
                     lambda d: d['post_audit'].update(status='failed')]
        mutations.extend(lambda d, key=key: d.update({key: True}) for key in flags)
        for mutate in mutations:
            changed = deepcopy(parent)
            mutate(changed)
            with self.assertRaises(ValueError):
                normalizer.validate_metadata(changed, receipt, post, qualification, pins, orchestration)


if __name__ == '__main__':
    unittest.main()
