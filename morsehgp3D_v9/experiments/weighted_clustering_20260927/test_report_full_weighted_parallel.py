"""Synthetic formatter/provenance tests, never evidence of a native campaign."""
from copy import deepcopy
from contextlib import redirect_stdout
import csv
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import report_full_weighted_parallel as report


def fixture():
    common = report.formatting
    rows = []
    for case in common.CASES:
        regime, raw_g, raw_d, raw_seed = case.rsplit('_', 3)
        g, d, seed = int(raw_g[1:]), int(raw_d[1:]), int(raw_seed[1:])
        for k in (5, 10):
            for m in (20, 50):
                for z in (1, 2):
                    methods = (*common.METHODS, 'hdbscan_standard') if z == 1 else common.METHODS
                    for method in methods:
                        metrics = {name: seed/10 for name in common.METRICS}
                        metrics.update(ari_classified=None if seed == 1 else seed/10,
                                       coverage=0.5, clusters=g, noise_count=600)
                        extra = {name: 0.5 for name in common.EXTRA}
                        extra.update(truth_classes=g, predicted_clusters=g,
                                     eligible_classes=g, classes_below_min_cluster_size=0)
                        row = dict(case=case, regime=regime, communities=g, separation=d,
                                   seed=2026092710+seed, n=1200, k=k, min_cluster_size=m,
                                   exp_z=z, method=method, metrics=metrics, extra=extra)
                        if method == common.METHODS[0]:
                            row.update(final_point_sizes=[600], near_threshold_nodes=0,
                                       vote_ties=1, naive_gabriel_components=2,
                                       final_point_clusters_below_mass_threshold=0)
                        if case == common.CASES[0] and k == 5:
                            row['reused_from'] = dict(synthetic=True)
                        rows.append(row)
    return dict(schema=report.SCHEMA, status='completed', rows=rows,
                plan=dict(cases=list(common.CASES), k=[5, 10], sizes=[20, 50], exp_z=[1, 2]),
                commands=[dict(case=c, k=k) for c in common.CASES for k in (5, 10)])


def orchestration():
    return dict(reused_units=1, newly_executed_units=25, native_commands=26,
                worker_commands=25, maximum_observed_workers=2,
                original_failure_preserved=True, original_source_closure_missing=True,
                owned_groups_closed=25)


def post_fixture(capture, pins):
    return dict(schema=report.AUDIT_SCHEMA, status='passed', capture=str(capture),
                receipt_sha256='b'*64, audit_source_sha256=report.sha(report.AUDIT_SOURCE),
                arithmetic_source_sha256=report.ARITHMETIC_SHA, weighted_rows=104,
                comparator_rows=260, total_rows=364, measures=52, pins_sha256=pins,
                pins_checked=len(pins), orchestration=orchestration(),
                Hungarian_solver_shared=True, NMI_recomputed=False,
                geometry_or_EOM_rerun=False, GCP_used=False)


class ParallelReportTests(unittest.TestCase):
    def test_pinned_formatting_sources(self):
        self.assertEqual(report.sha(report.FORMAT_SOURCE), report.FORMAT_SHA)
        self.assertEqual(report.sha(report.ARITHMETIC_SOURCE), report.ARITHMETIC_SHA)

    def test_whole_grid_and_refusals(self):
        receipt = fixture()
        before = deepcopy(receipt)
        report.validate_grid(receipt)
        self.assertEqual(receipt, before)
        mutations = [lambda d: d.update(status='failed'),
                     lambda d: d.update(schema='mhgp9_weighted_full_gaussian_pilot_v2'),
                     lambda d: d['rows'].pop(),
                     lambda d: d['rows'].__setitem__(-1, deepcopy(d['rows'][0])),
                     lambda d: d['rows'][0].update(exp_z=3),
                     lambda d: d['commands'].pop(),
                     lambda d: d['commands'].__setitem__(-1, deepcopy(d['commands'][0])),
                     lambda d: d['plan'].update(sizes=[20])]
        for mutate in mutations:
            changed = deepcopy(receipt)
            mutate(changed)
            with self.assertRaises(ValueError):
                report.validate_grid(changed)

    def test_aggregates_and_undefined_values(self):
        rows = [report.flatten(row) for row in fixture()['rows']]
        groups = report.formatting.aggregates(rows)
        self.assertEqual(len(rows), 364)
        self.assertEqual(len(groups), 140)
        chosen = next(g for g in groups if g['regime'] == 'spherical' and g['communities'] == 2
                      and g['k'] == 5 and g['min_cluster_size'] == 20 and g['exp_z'] == 1
                      and g['method'] == 'hgp_weighted_full_vote')
        self.assertAlmostEqual(chosen['ari_all_mean'], 0.2)
        self.assertAlmostEqual(chosen['ari_all_sd'], 0.1)
        self.assertEqual(chosen['ari_classified_defined'], 2)
        self.assertAlmostEqual(chosen['ari_classified_mean'], 0.25)
        self.assertEqual(sum(row['unit_origin'] == 'reused_interrupted_serial_unit' for row in rows), 14)
        for row in rows:
            self.assertEqual(row['inherited_comparator'], row['method'] != 'hgp_weighted_full_vote')
        broken = deepcopy(rows)
        broken[1]['seed'] = broken[0]['seed']
        # Duplicate one entire row in place of a different seed of the same group.
        index = next(i for i,r in enumerate(broken) if r['case'] == 'spherical_g2_d8_s2'
                     and r['k'] == 5 and r['min_cluster_size'] == 20 and r['exp_z'] == 1
                     and r['method'] == 'hgp_weighted_full_vote')
        broken[index]['seed'] = rows[0]['seed']
        with self.assertRaises(ValueError):
            report.formatting.aggregates(broken)

    def test_nonfinite_published_values_refused(self):
        row = fixture()['rows'][0]
        row['metrics']['ari_all'] = float('nan')
        with self.assertRaises(ValueError):
            report.flatten(row)

    def test_post_binding_and_refusals(self):
        capture = Path('/tmp/synthetic-parallel-report-only').resolve()
        pins = {'synthetic': 'c'*64}
        post = post_fixture(capture, pins)
        report.validate_post(post, capture, 'b'*64, pins, orchestration())
        changes = [lambda d: d.update(status='failed'),
                   lambda d: d.update(schema='mhgp9_weighted_post_capture_score_audit_v1'),
                   lambda d: d.update(capture='/tmp/not-the-synthetic-capture'),
                   lambda d: d.update(receipt_sha256='d'*64),
                   lambda d: d.update(weighted_rows=103),
                   lambda d: d.update(measures=51),
                   lambda d: d.update(audit_source_sha256='e'*64),
                   lambda d: d.update(arithmetic_source_sha256='f'*64),
                   lambda d: d.update(pins_sha256={}),
                   lambda d: d.update(pins_checked=0),
                   lambda d: d['orchestration'].update(reused_units=2),
                   lambda d: d.update(Hungarian_solver_shared=False),
                   lambda d: d.update(NMI_recomputed=True),
                   lambda d: d.update(geometry_or_EOM_rerun=True),
                   lambda d: d.update(GCP_used=True)]
        for mutate in changes:
            changed = deepcopy(post)
            mutate(changed)
            with self.assertRaises(ValueError):
                report.validate_post(changed, capture, 'b'*64, pins, orchestration())

    def test_tables_disclose_reuse_and_scope(self):
        groups = report.formatting.aggregates([report.flatten(r) for r in fixture()['rows']])
        table = report.tables(groups, orchestration())
        for phrase in ('364 lignes', '140 agrégats', 'HDBSCAN standard', 'failed',
                       'ne la recrée pas rétroactivement', 'borne mémoire', 'sans nouveau fit'):
            self.assertIn(phrase, table)
        self.assertEqual(table.count('## expZ='), 2)

    def test_synthetic_publication_only_and_no_overwrite(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-formatter-synthetic-') as folder:
            root = Path(folder)
            capture = root/'capture'; capture.mkdir()
            original = capture/'receipt.json'
            original.write_text('{"synthetic":true}\n')
            post_path = root/'post.json'; post_path.write_text('{"synthetic":true}\n')
            before = report.sha(original)
            receipt = fixture()
            receipt.update(scope='synthetic_only', root_policy='excluded_for_both',
                           note='synthetic formatter test; no native evidence', elapsed_seconds=1,
                           native_binary='/synthetic/native', native_binary_sha256='a'*64,
                           sources_after={}, worker_commands=[], reuse_verification={'synthetic': True},
                           worker_environment={}, baseline_receipt='/synthetic/baseline',
                           manifest='/synthetic/manifest')
            post = post_fixture(capture, {})
            bundle = receipt, post, {'synthetic': True}, {}, orchestration()
            output = root/'published'
            with patch.object(report, 'validated_inputs', return_value=bundle), redirect_stdout(io.StringIO()):
                report.publish(capture, post_path, output)
            summary = json.loads((output/'receipt.json').read_text())
            self.assertEqual((summary['row_count'], summary['aggregate_count'], summary['command_count']), (364,140,26))
            self.assertFalse(summary['original_failure_promoted'])
            self.assertFalse(summary['serial_speedup_claimed'])
            self.assertEqual(report.sha(original), before)
            with (output/'rows.csv').open() as stream:
                self.assertEqual(len(list(csv.DictReader(stream))),364)
            with self.assertRaises(ValueError):
                report.publish(capture, post_path, output)


if __name__ == '__main__':
    unittest.main()
