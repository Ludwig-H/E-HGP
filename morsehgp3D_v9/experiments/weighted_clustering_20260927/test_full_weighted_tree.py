"""Exact attachment cuts versus a direct FULL-segment oracle; no geometry run."""
from copy import deepcopy
from fractions import Fraction as Q
import math
import random
import unittest

from full_weighted_tree import build_full_weighted_tree,eom_input,tree_cut
from weighted_eom import weighted_condense_eom


def beta(value):
    value=Q(value)
    return dict(num=str(value.numerator),den=str(value.denominator))


def source(rows):
    parents={child:i for i,(_,kids) in enumerate(rows) for child in kids}
    nodes=[dict(id=i,level=beta(level),children=list(kids),successor=parents.get(i))
           for i,(level,kids) in enumerate(rows)]
    return nodes,[node['id'] for node in nodes if node['successor'] is None]


def direct_cut(nodes,attachments,at,closed,virtual=False):
    """Naive geometry oracle: each facet independently walks original FULL.

    It does not inspect the augmented/pruned tree. A pre-birth virtual facet
    is a distinct singleton, never assigned to a future FULL component early.
    """
    admitted=lambda value:value<=at if closed else value<at
    groups={}; inactive=[]
    for facet,attachment in enumerate(attachments):
        birth=Q(int(attachment['beta']['num']),int(attachment['beta']['den']))
        if not admitted(birth):
            if virtual and admitted(Q(0)):
                groups['isolated-'+str(facet)]=[facet]
            else:
                inactive.append(facet)
            continue
        node=attachment['node']
        successor=nodes[node]['successor']
        while successor is not None:
            level=Q(int(nodes[successor]['level']['num']),int(nodes[successor]['level']['den']))
            if not admitted(level):
                break
            node=successor; successor=nodes[node]['successor']
        groups.setdefault(node,[]).append(facet)
    return dict(blocks=sorted(groups.values()),inactive=inactive,virtual=virtual)


class FullWeightedTreeTests(unittest.TestCase):
    cuts=0
    refusals=0

    def check_cuts(self,nodes,roots,masses,attachments):
        original=deepcopy((nodes,roots,masses,attachments))
        result=build_full_weighted_tree(nodes,roots,masses,attachments)
        self.assertEqual((nodes,roots,masses,attachments),original)
        dates={Q(0)}
        for row in nodes:
            dates.add(Q(int(row['level']['num']),int(row['level']['den'])))
        for row in attachments:
            dates.add(Q(int(row['beta']['num']),int(row['beta']['den'])))
        ordered=sorted(dates); dates.update((a+b)/2 for a,b in zip(ordered,ordered[1:]))
        dates.add(max(dates)+1)
        for at in sorted(dates):
            for closed in (False,True):
                for virtual in (False,True):
                    actual=tree_cut(result,at,closed=closed,virtual=virtual,min_cluster_size=2 if virtual else None)
                    self.assertEqual(actual,direct_cut(nodes,attachments,at,closed,virtual))
                    self.assertEqual(sum(map(len,actual['blocks']))+len(actual['inactive']),len(masses))
                    self.assertEqual(sorted([p for row in actual['blocks'] for p in row]+actual['inactive']),list(range(len(masses))))
                    type(self).cuts+=1
        self.assertFalse(result['artificial_root_added'])
        self.assertEqual(len([row for row in result['geometry']['nodes'] if row['kind']=='FULL_birth']),len(nodes))
        return result

    def test_continuation_and_closed_boundary(self):
        nodes,roots=source([(1,[]),(2,[]),(5,[0,1])])
        attachments=[dict(node=0,beta=beta(1)),dict(node=0,beta=beta(3)),
                     dict(node=1,beta=beta(5)),dict(node=2,beta=beta(7))]
        result=self.check_cuts(nodes,roots,[Q(1,2)]*4,attachments)
        self.assertEqual(result['attachments'][2]['node'],2)
        self.assertTrue(result['attachments'][2]['moved_at_closed_boundary'])
        self.assertEqual(result['statistics']['closed_boundary_moves'],1)
        self.assertEqual(tree_cut(result,Q(3),closed=False)['blocks'],[[0]])
        self.assertEqual(tree_cut(result,Q(3))['blocks'],[[0,1]])
        self.assertEqual(tree_cut(result,Q(5))['blocks'],[[0,1,2]])
        self.assertEqual(tree_cut(result,Q(7))['blocks'],[[0,1,2,3]])

    def test_zero_mass_branches_and_no_false_merger(self):
        nodes,roots=source([(1,[]),(1,[]),(2,[0,1]),(1,[]),(4,[2,3])])
        attachments=[dict(node=0,beta=beta(1)),dict(node=3,beta=beta(3))]
        result=self.check_cuts(nodes,roots,[1,1],attachments)
        self.assertGreater(result['statistics']['zero_mass_events_removed'],0)
        self.assertGreater(result['statistics']['unary_events_contracted'],0)
        self.assertTrue(any(row['positive_facet_count']==0 for row in result['geometry']['nodes']))
        self.assertEqual(list(result['tree']['squared_levels'].values()),[beta(4)])
        self.assertEqual(tree_cut(result,Q(3))['blocks'],[[0],[1]])
        self.assertEqual(tree_cut(result,Q(4))['blocks'],[[0,1]])

    def test_equal_date_atomic_attachment(self):
        nodes,roots=source([(1,[]),(1,[]),(4,[0,1])])
        attachments=[dict(node=0,beta=beta(4)),dict(node=1,beta=beta(4)),dict(node=2,beta=beta(4))]
        result=self.check_cuts(nodes,roots,[Q(1,3)]*3,attachments)
        self.assertEqual(len(result['tree']['children']),1)
        self.assertEqual(next(iter(result['tree']['children'].values())),[0,1,2])
        self.assertEqual(tree_cut(result,Q(4),closed=False)['inactive'],[0,1,2])
        self.assertEqual(tree_cut(result,Q(4))['blocks'],[[0,1,2]])

    def test_forest_empty_and_single_positive_root(self):
        nodes,roots=source([(1,[]),(1,[])])
        attachments=[dict(node=0,beta=beta(1)),dict(node=1,beta=beta(2))]
        result=self.check_cuts(nodes,roots,[1,1],attachments)
        self.assertEqual(len(result['tree']['roots']),2)
        with self.assertRaises(ValueError):
            eom_input(result,min_cluster_size=2)
        single=self.check_cuts(nodes,roots,[1],attachments[:1])
        self.assertEqual(single['statistics']['full_roots'],2)
        self.assertEqual(single['statistics']['positive_roots'],1)
        self.assertEqual(weighted_condense_eom(**eom_input(single,min_cluster_size=2))['labels'],[-1])
        empty=self.check_cuts(nodes,roots,[],[])
        self.assertEqual(empty['tree']['roots'],[])
        with self.assertRaises(ValueError):
            eom_input(empty,min_cluster_size=2)

    def test_actual_birth_before_virtual_threshold_adaptation(self):
        nodes,roots=source([(2,[])])
        result=self.check_cuts(nodes,roots,[Q(3,4),Q(3,4)],
                               [dict(node=0,beta=beta(3)),dict(node=0,beta=beta(5))])
        self.assertEqual(tree_cut(result,Q(1))['inactive'],[0,1])
        self.assertEqual(tree_cut(result,Q(4))['blocks'],[[0]])
        self.assertEqual(tree_cut(result,Q(4),virtual=True,min_cluster_size=2)['blocks'],[[0],[1]])
        for threshold in (Q(1,2),Q(3,4),1,Q(3,2),True):
            with self.subTest(threshold=threshold),self.assertRaises(ValueError):
                eom_input(result,min_cluster_size=threshold)
        heavy=build_full_weighted_tree(nodes,roots,[2], [dict(node=0,beta=beta(3))])
        with self.assertRaises(ValueError):
            eom_input(heavy,min_cluster_size=2)
        eom_input(heavy,min_cluster_size=3)

    def test_m2_nontrivial_eom_and_zero_mass_side(self):
        nodes,roots=source([(1,[]),(1,[]),(1,[]),(16,[0,1,2])])
        attachments=[dict(node=0,beta=beta(1)) for _ in range(2)] + [dict(node=1,beta=beta(1)) for _ in range(2)]
        result=self.check_cuts(nodes,roots,[1]*4,attachments)
        for z in (1,2):
            selected=weighted_condense_eom(**eom_input(result,min_cluster_size=2),exp_z=z)
            self.assertEqual(selected['labels'],[0,0,1,1])
            self.assertEqual(selected['leaf_exit_lambda'],[1.0]*4)

    def test_random_exact_segment_cut_oracle(self):
        rng=random.Random(927105)
        for _ in range(50):
            rows=[(Q(rng.randint(0,2)),[]) for _ in range(rng.randint(2,6))]
            active=list(range(len(rows)))
            while len(active)>1:
                a=active.pop(rng.randrange(len(active))); b=active.pop(rng.randrange(len(active)))
                at=max(rows[a][0],rows[b][0])+rng.randint(1,3)
                active.append(len(rows)); rows.append((at,[a,b]))
            nodes,roots=source(rows); attachments=[]
            for _ in range(rng.randint(1,20)):
                node=rng.randrange(len(nodes)); low=rows[node][0]; successor=nodes[node]['successor']
                high=rows[successor][0] if successor is not None else low+2
                at=rng.choice([low,(low+high)/2,high])
                attachments.append(dict(node=node,beta=beta(at)))
            self.check_cuts(nodes,roots,[Q(1,2)]*len(attachments),attachments)

    def test_rational_collision_refusal_for_float_adapter_only(self):
        nodes,roots=source([(1,[])])
        adjacent=Q(2**54+1,2**54)
        result=build_full_weighted_tree(nodes,roots,[Q(1,2)]*3,
            [dict(node=0,beta=beta(1)),dict(node=0,beta=beta(1)),dict(node=0,beta=beta(adjacent))])
        self.assertEqual(tree_cut(result,Q(1))['blocks'],[[0,1]])
        self.assertEqual(tree_cut(result,adjacent)['blocks'],[[0,1,2]])
        with self.assertRaises(ValueError):
            eom_input(result,min_cluster_size=2)

    def test_refusals(self):
        nodes,roots=source([(1,[]),(1,[]),(4,[0,1])])
        base=dict(full_nodes=nodes,roots=roots,masses=[1],attachments=[dict(node=0,beta=beta(2))])
        mutations=[dict(masses=[0]),dict(masses=[True]),dict(masses=[math.inf]),dict(masses=[Q(1,10**400)]),
                   dict(attachments=[]),dict(attachments=[dict(node=True,beta=beta(2))]),
                   dict(attachments=[dict(node=3,beta=beta(2))]),dict(attachments=[dict(node=0,beta=beta(0))]),
                   dict(attachments=[dict(node=0,beta=beta(5))]),dict(roots=[0]),dict(roots=[2,2]),
                   dict(attachments=[dict(node=0,beta={'num':'02','den':'1'})]),
                   dict(attachments=[dict(node=0,beta={'num':'2','den':'0'})])]
        bad=deepcopy(nodes); bad[0]['successor']=None; mutations.append(dict(full_nodes=bad))
        bad=deepcopy(nodes); bad[2]['children']=[0]; mutations.append(dict(full_nodes=bad))
        bad=deepcopy(nodes); bad[2]['level']=beta(1); mutations.append(dict(full_nodes=bad))
        for change in mutations:
            with self.subTest(change=change),self.assertRaises(ValueError):
                build_full_weighted_tree(**dict(base,**change))
            type(self).refusals+=1


if __name__=='__main__':
    suite=unittest.defaultTestLoader.loadTestsFromTestCase(FullWeightedTreeTests)
    result=unittest.TextTestRunner(verbosity=2).run(suite)
    print(f'full_weighted_tree gates={result.testsRun} exact_cuts={FullWeightedTreeTests.cuts} '
          f'refusals={FullWeightedTreeTests.refusals} status={"PASS" if result.wasSuccessful() else "FAIL"}')
    raise SystemExit(0 if result.wasSuccessful() else 1)
