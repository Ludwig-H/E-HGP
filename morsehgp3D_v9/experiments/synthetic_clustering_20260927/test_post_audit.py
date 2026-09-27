"""Bounded reader tests: independent scores, lifecycle and saved point trees."""
from copy import deepcopy
from fractions import Fraction
import json
import importlib.util
from pathlib import Path
import random
import sys
import tempfile
import unittest

# Historical weighted dependencies themselves import `post_audit`. Use an
# explicit namespace for this NEW reader, as its CLI uses __main__ naturally.
spec = importlib.util.spec_from_file_location('synthetic_post_audit_tests',Path(__file__).with_name('post_audit.py'))
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
import unit_pipeline as unit


class Reader(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.helper,cls.arithmetic = audit.helpers()

    def row(self, truth, labels, *, weighted=True, minimum=2):
        method = 'hgp_weighted_full_vote' if weighted else 'hgp_exclusive_point_routing'
        threshold = None if weighted else minimum
        return dict(n=len(truth),method=method,min_cluster_size=minimum,
            threshold_semantics='facet_mass_before_vote' if weighted else 'point_cardinality',
            metrics=unit.metrics(truth,labels),extra=unit.evaluate_labels(truth,labels,min_cluster_size=threshold))

    def test_fraction_scores_and_noise(self):
        rng = random.Random(20260928)
        for iteration in range(120):
            n = 11+iteration%31
            truth = [rng.choice([-1,1,2,3]) for _ in range(n)]
            labels = [rng.choice([-1,0,1,2]) for _ in range(n)]
            row = self.row(truth,labels)
            exact = audit.verify_scores(truth,labels,row,self.arithmetic)
            self.assertEqual(Fraction(*exact['ari_all']),self.arithmetic.ari(truth,labels))
        for truth,labels in (([1,1,2,2],[-1]*4),([-1]*4,[-1]*4),
                             ([1,1,2,2],[0,0,1,1]),([-1,1,1,-1],[0,0,-1,-1])):
            audit.verify_scores(truth,labels,self.row(truth,labels),self.arithmetic)

    def test_score_mutants_and_threshold_distinction(self):
        truth = [-1,-1,1,1,1,2,2,2]; labels = [-1,0,-1,0,0,1,1,1]
        row = self.row(truth,labels)
        for key in ('ari_all','ari_true_inliers','ari_inliers_noise_singletons','ari_classified',
                    'coverage','noise_precision','noise_recall','noise_f1'):
            mutant = deepcopy(row); mutant['metrics'][key] += .01
            with self.subTest(key=key),self.assertRaises(ValueError):
                audit.verify_scores(truth,labels,mutant,self.arithmetic)
        for key in ('matched_macro_f1','matched_micro_f1','matched_macro_precision','matched_macro_recall'):
            mutant = deepcopy(row); mutant['extra'][key] += .01
            with self.assertRaises(ValueError):
                audit.verify_scores(truth,labels,mutant,self.arithmetic)
        mutant = deepcopy(row); mutant['extra']['min_cluster_size'] = 2
        with self.assertRaises(ValueError):
            audit.verify_scores(truth,labels,mutant,self.arithmetic)
        small = [0,1,-1,-1]; truth = [1,1,2,2]
        audit.verify_scores(truth,small,self.row(truth,small),self.arithmetic)
        with self.assertRaisesRegex(ValueError,'cardinalities'):
            audit.verify_scores(truth,small,self.row(truth,small,weighted=False),self.arithmetic)
        enough = [0,0,1,1]
        audit.verify_scores(truth,enough,self.row(truth,enough,weighted=False),self.arithmetic)

    def lifecycle(self):
        root = Path('/tmp/mhgp9-synthetic-reader-mock')
        commands = [dict(case=name,k=5,pid=pid,argv=[sys.executable,'-B',str(audit.HERE/'run.py'),
            '--worker',str(root/(name+'.spec.json'))],returncode=0,elapsed_seconds=1,
            stdout_sha256='a',stderr_sha256='b') for name,pid in (('a',100),('b',101))]
        start = lambda c:dict(event='start',**{key:c[key] for key in ('case','k','pid','argv')})
        events = [start(commands[0]),start(commands[1])]+[dict(event='joined',**c) for c in commands]
        clean = [dict(pgid=c['pid'],status='closed',returncode=0,signals=[],residual_group_before_cleanup=False)
                 for c in commands]
        return dict(workers=2,worker_commands=commands,events=events,cleanup=clean),root

    def test_lifecycle_complete_and_mutants(self):
        receipt,root = self.lifecycle()
        self.assertEqual(audit.lifecycle(receipt,['a','b'],root),2)
        mutations = []
        bad = deepcopy(receipt); bad['events'].pop(); mutations.append(bad)
        bad = deepcopy(receipt); bad['events'][2]['pid'] = 999; mutations.append(bad)
        bad = deepcopy(receipt); bad['workers'] = 1; mutations.append(bad)
        bad = deepcopy(receipt); bad['cleanup'][0]['status'] = 'running'; mutations.append(bad)
        bad = deepcopy(receipt); bad['cleanup'][0]['pgid'] = 999; mutations.append(bad)
        bad = deepcopy(receipt); bad['events'].reverse(); mutations.append(bad)
        bad = deepcopy(receipt); bad['events'][0]['argv'][-1] = '/tmp/wrong'; mutations.append(bad)
        bad = deepcopy(receipt); bad['worker_commands'][0]['returncode'] = 2; mutations.append(bad)
        bad = deepcopy(receipt); bad['cleanup_errors'] = ['failure']; mutations.append(bad)
        for changed in mutations:
            with self.assertRaises(ValueError):
                audit.lifecycle(changed,['a','b'],root)

    def tree(self):
        n = 60
        routing = unit.route_points(n,[(p,) for p in range(n)],[Fraction(1)]*n,
            {60:list(range(30)),61:list(range(30,60)),62:[60,61]},
            {60:Fraction(1),61:Fraction(1),62:Fraction(9)},[62],[Fraction(0)]*n)
        tree = unit.build_point_tree(routing)
        wire = json.loads(json.dumps(unit.to_jsonable(tree)))
        return self.helper.decode_tree(wire)

    def test_point_tree_cut_and_condensed_replays(self):
        tree = self.tree()
        dates = sorted(set(tree['squared_levels'].values())); selected = {Fraction(0),dates[-1]+1}
        selected.update(dates[q*(len(dates)-1)//4] for q in range(5))
        checks = [dict(beta=unit.to_jsonable(at),closed=closed,blocks=len(unit.cut(tree,at,closed=closed)))
                  for at in sorted(selected) for closed in (False,True)]
        self.assertEqual(audit.check_point_cuts(tree,checks,self.helper),len(checks))
        bad = deepcopy(checks);bad[-1]['blocks'] += 1
        with self.assertRaises(ValueError):audit.check_point_cuts(tree,bad,self.helper)
        for minimum in (20,50):
            for exponent in (1,2):
                selected = unit.cluster_point_tree(tree,min_cluster_size=minimum,exp_z=exponent)
                wire = unit.clean(unit.to_jsonable(selected))
                labels = self.helper.verify_selection(wire,tree,minimum,exponent)
                self.assertEqual(labels,audit.common_labels(wire['result'],60,minimum,exponent,self.helper))
                bad = deepcopy(wire);bad['result']['selection']['labels'][0] = 987
                with self.assertRaises(ValueError):
                    self.helper.verify_selection(bad,tree,minimum,exponent)
                with self.assertRaises(ValueError):
                    audit.common_labels(bad['result'],60,minimum,exponent,self.helper)

    def test_metadata_and_conflicting_pins(self):
        case = dict(phase='quality',axis='size',replicate=1,n=400,groups=8,
            spec=dict(family='spherical',groups=8,separation=4,seed=1,noise_fraction=0))
        row = dict(case);row.update(case['spec'])
        audit.metadata(row,case)
        bad = dict(row,n=399)
        with self.assertRaises(ValueError):audit.metadata(bad,case)
        pins = {'x':'a'};audit.add_pins(pins,{'x':'a','y':'b'})
        with self.assertRaises(ValueError):audit.add_pins(pins,{'x':'b'})

    def test_output_never_mutates_capture(self):
        with tempfile.TemporaryDirectory(prefix='mhgp9-post-output-test-') as name:
            capture = Path(name)/'capture';capture.mkdir()
            with self.assertRaisesRegex(ValueError,'outside capture'):
                audit.audit(capture/'receipt.json',capture/'audit')
            self.assertEqual(list(capture.iterdir()),[])
            with self.assertRaisesRegex(ValueError,'NEW audit'):
                audit.audit(capture/'receipt.json',capture)


if __name__ == '__main__':
    unittest.main(verbosity=2)
