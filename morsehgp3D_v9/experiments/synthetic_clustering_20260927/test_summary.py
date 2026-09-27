"""Synthetic formatter fixtures only, never a benchmark receipt."""
from copy import deepcopy
import csv
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT/'morsehgp3D_v9/experiments/synthetic_clustering_20260927'))
import summarize as s


def save(path, value):
    with path.open('w') as stream:
        json.dump(value, stream)


def fixture(directory):
    cases=[]
    for planned in s.plan.PLAN['cases']:
        case=deepcopy(planned); spec=case['spec']; n=spec['n']; g=spec['groups']
        noise=round(n*spec['noise_fraction']); actual=n-noise
        weights=[4 if i%2==0 else 1 for i in range(g)] if spec['family']=='unbalanced' else [1]*g
        counts={str(i+1):actual*w//sum(weights) for i,w in enumerate(weights)}
        if noise:counts['-1']=noise
        case.update(n=n,groups=g,true_counts=counts)
        cases.append(case)
    manifest=dict(schema='mhgp9_synthetic_cluster_input_manifest_v1',plan=s.plan.PLAN,cases=cases)
    manifest_path=directory/'manifest.json';save(manifest_path,manifest)
    units=[]
    pins={str(Path(s.plan.__file__).resolve()):s.sha(s.plan.__file__)}
    for case in cases:
        if case['phase']!='quality':continue
        n=case['n']; rows=[]
        for key in sorted(key for key in s.expected_grid() if key[0]==case['id']):
            _,k,m,z,method=key
            row={name:case[name] for name in ('phase','axis','replicate','spec','n','groups')}
            row.update({name:case['spec'][name] for name in ('family','groups','separation','seed','noise_fraction')})
            score=.7 if method==s.ROUTED else .6
            metrics={name:score for name in s.METRICS}
            metrics.update(coverage=1.,clusters=case['groups'],noise_count=0,
                noise_precision=None,noise_recall=None,noise_f1=None)
            extra={name:.5 for name in s.EXTRAS}
            row.update(case=case['id'],k=k,min_cluster_size=m,exp_z=z,method=method,
                metrics=metrics,extra=extra,selection_ms=.1,labels_payload_sha256='f'*64,
                threshold_semantics='facet_mass_before_vote' if method=='hgp_weighted_full_vote' else 'point_cardinality')
            rows.append(row)
        parts=[dict(exp_z=z,routing_statistics=dict(incidences=100*n,source_nodes=30*n,virtual_nodes=400*n),
                    point_tree_statistics=dict(internal_nodes=n-1,point_leaves=n),
                    full_tree_statistics=dict(full_nodes=10*n),
                    times_ms=dict(measure_and_naive_graph=n,full_facet_tree=n,routing_reference=2*n,point_tree_and_validate=n)) for z in (1,2)]
        units.append(dict(schema='mhgp9_synthetic_clustering_unit_v1',status='completed',case=case['id'],
            n=n,k=5,sources_before=pins,sources_after=pins,commands=[dict(returncode=0,wall_ms=n)],rows=rows,
            units=parts,native_statistics=dict(cofaces=10*n),attachment_statistics=dict(facets=20*n),
            native_times_ms=dict(native_chain_wall=n,native_chain_reported=n,native_tower=n/2),
            native_read_and_validate_ms=n/10,first_coverage_ms=n/10,elapsed_seconds=n/100,
            hdbscan_fits=[dict(min_cluster_size=m,wall_ms=n/10) for m in (20,50)]))
    receipt=dict(schema=s.CAPTURE_SCHEMA,status='completed',plan=s.plan.PLAN,
        GCP_used=False,GPU_used=False,growth_executed=False,workers=2,
        sources_before=pins,sources_after=pins,manifest=str(manifest_path),manifest_sha256=s.sha(manifest_path),
        units=units,rows=[row for unit in units for row in unit['rows']],elapsed_seconds=10.,timing_scope='fake_fixture_only')
    path=directory/'receipt.json';save(path,receipt)
    audit=dict(schema=s.AUDIT_SCHEMA,status='passed',receipt=str(path),receipt_sha256=s.sha(path),
        rows=612,point_trees=68,point_selection_replays=136,common_condensed_replays=272,
        hdbscan_fits_checked=68,worker_commands=34,native_commands=34,fraction_ari_replays=612,
        NMI_recomputed=False,Hungarian_solver_shared=True)
    audit_path=directory/'audit.json';save(audit_path,audit)
    return path,receipt,manifest_path,manifest,audit_path,audit


class Tests(unittest.TestCase):
    refusals=0
    def setUp(self):
        self.temporary=tempfile.TemporaryDirectory(prefix='synthetic-summary-fixture-')
        self.addCleanup(self.temporary.cleanup)
        self.directory=Path(self.temporary.name)
        self.path,self.receipt,self.manifest_path,self.manifest,self.audit_path,self.audit=fixture(self.directory)

    def test_no_audit_full_output(self):
        output=self.directory/'public'
        result=s.publish(self.path,output)
        self.assertTrue(result['structural_validation_only'])
        self.assertEqual((result['rows'],result['aggregates'],result['comparisons'],result['size_diagnostics']),(612,306,136,12))
        for name,count in [('scores.csv',612),('aggregates.csv',306),('comparisons.csv',136),('size_diagnostics.csv',12)]:
            with (output/name).open() as stream:rows=list(csv.DictReader(stream))
            self.assertEqual(len(rows),count)
            self.assertNotIn('labels',rows[0])
        document=(output/'README.md').read_text()
        self.assertIn('non contre-audité',document)
        for case in self.manifest['cases']:
            if case['phase']=='quality':self.assertIn(case['id'],document)
        self.assertIn('NON exécutées',document)
        sys.path.insert(0,str(ROOT))
        from tools import check_docs
        with patch.object(check_docs,'ROOT',self.directory):
            self.assertEqual(check_docs.validate(output/'README.md'),[])
        for name,digest in result['files'].items():self.assertEqual(s.sha(output/name),digest)
        with self.assertRaisesRegex(ValueError,'NEW summary'):
            s.publish(self.path,output)
        type(self).refusals+=1

    def test_audit_bound_and_no_labels_payload_read(self):
        result=s.publish(self.path,self.directory/'audited',self.audit_path)
        self.assertFalse(result['structural_validation_only'])
        self.assertEqual(result['post_audit']['sha256'],s.sha(self.audit_path))
        self.assertFalse(result['metrics_recomputed'])

    def test_arithmetic_pairing_order_and_nulls(self):
        receipt,cases,_,_=s.validate_inputs(self.path)
        rows=[s.flatten(row,cases[row['case']]) for row in receipt['rows']]
        groups=s.aggregates(rows);pairs=s.comparisons(rows)
        self.assertEqual(groups,s.aggregates(list(reversed(rows))))
        self.assertEqual(pairs,s.comparisons(list(reversed(rows))))
        for row in pairs:
            self.assertAlmostEqual(row['delta_ari_all'],.1)
            self.assertEqual(row['delta_coverage'],0.)
            self.assertIsNone(row['delta_noise_f1'])
        for row in groups:self.assertEqual(row['noise_f1_count'],0)
        small=[row for row in rows if row['groups']==32 and row['min_cluster_size']==50]
        self.assertTrue(all(row['true_classes_below_m']==32 for row in small))
        self.assertFalse(next(row for row in small if row['method']=='hgp_weighted_full_vote')['below_m_is_cardinality_constraint'])

    def test_size_doubling_ratios(self):
        receipt,cases,_,_=s.validate_inputs(self.path)
        rows=s.size_diagnostics(receipt,cases)
        for row in rows:
            expected=None if row['n']==400 else 2.
            self.assertEqual(row['cofaces_ratio_to_previous_n'],expected)
            self.assertEqual(row['routing_ms_ratio_to_previous_n'],expected)
            if row['exp_z']==2:self.assertIsNone(row['hdbscan_standard_m20_selection_ms'])

    def test_capture_mutants_refused(self):
        mutations=[lambda d:d.update(status='running'),lambda d:d.update(growth_executed=True),
            lambda d:d['rows'].pop(),lambda d:d['units'].pop(),
            lambda d:d['units'][0].update(status='failed'),
            lambda d:d['units'][0]['commands'][0].update(returncode=1),
            lambda d:d['rows'][0].update(min_cluster_size=True),
            lambda d:d['rows'][0].update(seed=1),
            lambda d:d.update(manifest_sha256='0'*64),
            lambda d:d.update(sources_after={}),
            lambda d:d['rows'][0]['metrics'].update(coverage=float('nan'))]
        for mutate in mutations:
            changed=deepcopy(self.receipt);mutate(changed);save(self.path,changed)
            with self.assertRaises(ValueError):s.validate_inputs(self.path)
            type(self).refusals+=1

    def test_manifest_mutant_refused(self):
        self.manifest['cases'][0]['true_counts']['1']+=1
        save(self.manifest_path,self.manifest)
        self.receipt['manifest_sha256']=s.sha(self.manifest_path);save(self.path,self.receipt)
        with self.assertRaisesRegex(ValueError,'true counts'):s.validate_inputs(self.path)
        type(self).refusals+=1

    def test_audit_mutants_refused(self):
        for key,value in [('status','failed'),('receipt_sha256','0'*64),('rows',611),('point_trees',67)]:
            changed=deepcopy(self.audit);changed[key]=value;save(self.audit_path,changed)
            with self.assertRaises(ValueError):s.validate_inputs(self.path,self.audit_path)
            type(self).refusals+=1


if __name__=='__main__':
    result=unittest.main(exit=False)
    print(json.dumps(dict(status='PASS' if result.result.wasSuccessful() else 'FAIL',
        gates=result.result.testsRun,refusals=Tests.refusals,fake_fixtures_only=True,native_or_fits_run=False)))
    raise SystemExit(0 if result.result.wasSuccessful() else 1)
