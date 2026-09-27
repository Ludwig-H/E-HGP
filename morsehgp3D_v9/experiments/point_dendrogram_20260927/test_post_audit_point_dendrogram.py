"""Small synthetic reader gates. No real fit, geometry, or pilot rerun."""
from copy import deepcopy
from fractions import Fraction as Q
import hashlib
import json
from pathlib import Path
import tempfile
import unittest

import post_audit_point_dendrogram as audit
from point_tree import build_point_tree, cut, to_jsonable
from point_eom import cluster_point_tree


def fixture(z=1, birth=1):
    route = audit.pilot.route_points(5, [[0],[1],[2],[3],[4]], [1]*5,
                                    {5:[0,1],6:[2,3],7:[5,6,4]},
                                    {5:Q(birth),6:Q(birth),7:Q(16)}, [7], [Q(0)]*5)
    tree = build_point_tree(route)
    payload = audit.pilot.clean(to_jsonable(cluster_point_tree(tree,min_cluster_size=2,exp_z=z)))
    truth = [0,0,1,1,-1]
    prediction = payload['result']['selection']['labels']
    row = dict(n=5,min_cluster_size=2,
               metrics=audit.pilot.metrics(truth,prediction),
               extra=audit.pilot.evaluate_labels(truth,prediction,min_cluster_size=2))
    return tree,payload,truth,row


class ReaderAudit(unittest.TestCase):
    def test_lossless_tree_decoder(self):
        tree,_,_,_ = fixture()
        encoded = to_jsonable(tree); saved = deepcopy(encoded)
        self.assertEqual(audit.decode_tree(encoded), tree)
        self.assertEqual(encoded, saved)

    def test_decoder_mutants(self):
        mutations = [lambda x:x['children'].__setitem__('05',x['children'].pop('5')),
                     lambda x:x['leaf_birth_betas'].__setitem__(0,{'num':'1','den':'1'}),
                     lambda x:x['point_counts'].__setitem__('0',2),
                     lambda x:x['attachments'][0].update(point=1)]
        for mutate in mutations:
            wire = to_jsonable(fixture()[0]); mutate(wire)
            with self.assertRaises(ValueError):
                audit.decode_tree(wire)

    def test_cuts_require_complete_exact_fingerprints(self):
        tree = fixture()[0]; dates = sorted(set(tree['squared_levels'].values()))
        selected = {Q(0),dates[-1]+1}
        selected.update(dates[q*(len(dates)-1)//4] for q in range(5))
        checks = []
        for beta in sorted(selected):
            for closed in (False,True):
                blocks = cut(tree,beta,closed=closed)
                checks.append(dict(beta=to_jsonable(beta),closed=closed,blocks=len(blocks),
                    partition_sha256=hashlib.sha256(json.dumps(blocks,separators=(',',':')).encode()).hexdigest()))
        self.assertEqual(audit.verify_cuts(tree,checks),len(checks))
        for mutated in (checks[:-1],checks[::-1],[dict(checks[0],partition_sha256='0'*64)]+checks[1:]):
            with self.assertRaises(ValueError):
                audit.verify_cuts(tree,mutated)

    def test_both_exponents_and_infinite_terminal_lifetimes(self):
        for z in (1,2):
            for birth in (0,1):
                tree,payload,truth,row = fixture(z,birth)
                prediction = audit.verify_selection(payload,tree,2,z)
                exact = audit.verify_scores(truth,prediction,row,audit.arithmetic())
                self.assertEqual(exact['ari_all'],[1,1])
                self.assertEqual(prediction,[0,0,1,1,-1])

    def test_selection_mutants(self):
        changes = [lambda x:x.update(exp_z=2),
                   lambda x:x.update(mass_policy='facet_mass'),
                   lambda x:x['source_squared_levels'].__setitem__('5',{'num':'2','den':'1'}),
                   lambda x:x['result']['selection']['labels'].__setitem__(0,-1),
                   lambda x:x['result']['selection']['selected'].append(0),
                   lambda x:x['result']['selection'].update(allow_single_cluster=True),
                   lambda x:x['result']['condensed_tree']['mass_at_birth'].__setitem__(0,4),
                   lambda x:x['result']['condensed_tree']['point_exit_ids'].__setitem__(0,1),
                   lambda x:x['result']['condensed_tree']['own_stability'].__setitem__(1,123),
                   lambda x:x['result']['stats'].update(points_removed_from_input=1)]
        for change in changes:
            tree,payload,_,_ = fixture(); change(payload)
            with self.assertRaises(ValueError):
                audit.verify_selection(payload,tree,2,1)

    def test_metrics_noise_singletons_and_hungarian(self):
        independent = audit.arithmetic()
        for prediction in ([0,0,-1,-1],[-1]*4,[0,0,0,0],[0,1,0,1]):
            truth = [0,0,1,1]
            row = dict(n=4,min_cluster_size=2,metrics=audit.pilot.metrics(truth,prediction),
                       extra=audit.pilot.evaluate_labels(truth,prediction,min_cluster_size=2))
            exact = audit.verify_scores(truth,prediction,row,independent)
            if prediction == [0,0,-1,-1]:
                self.assertEqual(exact['ari_all'],[1,1])
                self.assertEqual(exact['ari_inliers_noise_singletons'],[4,7])

    def test_score_mutants_and_cardinality(self):
        for category,key,value in [('metrics','ari_all',0.0),('metrics','coverage',1.0),
                                   ('metrics','clusters',3),('extra','min_cluster_size',None),
                                   ('extra','matched_macro_f1',0.0),('extra','eligible_classes',0)]:
            _,payload,truth,row = fixture(); row[category][key] = value
            with self.assertRaises(ValueError):
                audit.verify_scores(truth,payload['result']['selection']['labels'],row,audit.arithmetic())
        truth,prediction = [0,0,1,1],[0,0,1,-1]
        row = dict(n=4,min_cluster_size=2,metrics=audit.pilot.metrics(truth,prediction),
                   extra=audit.pilot.evaluate_labels(truth,prediction,min_cluster_size=2))
        with self.assertRaises(ValueError):
            audit.verify_scores(truth,prediction,row,audit.arithmetic())

    def test_reject_failed_or_running_capture(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-point-reader-gate-') as directory:
            folder = Path(directory)
            for status in ('running','failed'):
                (folder/'receipt.json').write_text(json.dumps(dict(schema=audit.pilot.SCHEMA,status=status)))
                with self.assertRaisesRegex(ValueError,'completed'):
                    audit.validate_capture(folder)

    def test_conflicting_or_changed_pins(self):
        with self.assertRaises(ValueError):
            audit.add_pins({'a':'1'},{'a':'2'})
        with tempfile.TemporaryDirectory(prefix='mhgp9-point-reader-pin-') as directory:
            path = Path(directory)/'pin'; path.write_text('one')
            pins = {str(path):audit.sha(path)}; audit.check_pins(pins)
            path.write_text('two')
            with self.assertRaises(ValueError):
                audit.check_pins(pins)

    def test_no_overwrite(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-point-reader-output-') as directory:
            with self.assertRaisesRegex(ValueError,'NEW'):
                audit.audit(Path(directory)/'missing',Path(directory))


if __name__ == '__main__':
    unittest.main()
