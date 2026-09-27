#!/usr/bin/env python3
"""Small offline gates of the compact common condensed cluster-tree API."""
from copy import deepcopy
import json
import math
import unittest

from condensed import (condensed_cut, point_clusterer_from_tree,
                       validate_condensed_tree, _legacy)


def groups_tree(sizes, noise=0):
    n = sum(sizes)+noise
    children, heights, cursor, groups = {}, {}, 0, []
    for i,size in enumerate(sizes):
        node = n+i
        children[node] = list(range(cursor,cursor+size))
        heights[node] = 1.0
        cursor += size
        groups.append(node)
    root = n+len(sizes)
    children[root] = groups+list(range(cursor,n))
    heights[root] = 10.0
    return dict(n=n,children=children,heights=heights,root=root)


def one_large_child():
    return dict(n=5,children={5:[0,1],6:[2,3],7:[5,6],8:[7,4]},
                heights={5:1.,6:1.,7:3.,8:10.},root=8)


class CondensedTests(unittest.TestCase):
    def verify_cuts(self, tree):
        events = sorted(set([0.,*tree['birth_lambda'],*tree['death_lambda'],*tree['point_exit_lambda']]))
        cuts = set(events)
        finite = [x for x in events if math.isfinite(x)]
        cuts.update((a+b)/2 for a,b in zip(finite,finite[1:]))
        cuts.add(max(finite)+1)
        previous = None
        for at in sorted(cuts):
            result = condensed_cut(tree,at)
            groups = [set(row['points']) for row in result['clusters']]
            groups += [{point} for point in result['inactive_points']]
            self.assertEqual(sum(map(len,groups)),tree['n_points'])
            self.assertEqual(set().union(*groups),set(range(tree['n_points'])))
            for i,first in enumerate(groups):
                for second in groups[:i]:
                    self.assertFalse(first & second)
            if previous is not None:
                self.assertTrue(all(any(group <= old for old in previous) for group in groups))
            previous = groups
            for row in result['clusters']:
                c = row['cluster']
                self.assertLessEqual(tree['birth_lambda'][c],at)
                self.assertLess(at,tree['death_lambda'][c])
                if c != 0:
                    self.assertGreaterEqual(len(row['points']),tree['min_cluster_size'])

    def test_zero_large_children_and_root_below_threshold(self):
        raw = dict(n=3,children={3:[0,1,2]},heights={3:2.},root=3)
        result = point_clusterer_from_tree(raw,10,1)
        tree = result['condensed_tree']
        self.assertEqual(tree['parent'],[-1])
        self.assertEqual(tree['mass_at_birth'],[3])
        self.assertEqual(tree['point_exit_parent'],[0]*3)
        self.assertEqual(tree['point_exit_lambda'],[.5]*3)
        self.assertEqual(result['selection']['labels'],[-1]*3)
        self.assertEqual(result['selection']['selected'],[])
        self.assertEqual(tree['own_stability'],[1.5])
        self.assertEqual(condensed_cut(tree,.5)['inactive_points'],[0,1,2])
        self.assertEqual(tree['root'],0)  # Structural root survives its empty cut.
        self.verify_cuts(tree)

    def test_one_large_child_continues_without_new_cluster(self):
        result = point_clusterer_from_tree(one_large_child(),2,1)
        tree = result['condensed_tree']
        self.assertEqual(tree['parent'],[-1,0,0])
        self.assertEqual(tree['mass_at_birth'],[5,2,2])
        self.assertEqual(tree['birth_lambda'],[0.,1/3,1/3])
        self.assertEqual(tree['point_exit_parent'],[1,1,2,2,0])
        self.assertEqual(tree['point_exit_lambda'],[1.,1.,1.,1.,.1])
        self.assertAlmostEqual(tree['own_stability'][0],.1+4/3)
        self.assertEqual(result['stats']['suppressed_continuation_internal_nodes'],1)
        self.assertEqual(result['selection']['labels'],[0,0,1,1,-1])
        at_exit = condensed_cut(tree,.1)
        self.assertEqual(at_exit['clusters'],[dict(cluster=0,points=[0,1,2,3])])
        self.assertEqual(at_exit['inactive_points'],[4])
        self.verify_cuts(tree)

    def test_two_large_children_bifurcate_and_multifusion(self):
        for sizes in ([3,3],[2,3,4]):
            result = point_clusterer_from_tree(groups_tree(sizes),2,1)
            tree = result['condensed_tree']
            self.assertEqual(tree['parent'],[-1]+[0]*len(sizes))
            self.assertEqual(tree['mass_at_birth'],[sum(sizes),*sizes])
            self.assertEqual(tree['birth_lambda'],[0.]+[.1]*len(sizes))
            self.assertEqual(tree['death_lambda'],[.1]+[1.]*len(sizes))
            self.assertEqual(len(result['selection']['selected']),len(sizes))
            self.assertEqual(result['stats']['noise_points'],0)
            self.verify_cuts(tree)

    def test_thresholds_10_20_50_100_remove_clusters_not_points(self):
        raw = groups_tree([12,25,60,110],noise=3)
        for m,clusters,selected,noise in [(10,5,4,3),(20,4,3,15),(50,3,2,40),(100,1,0,210)]:
            result = point_clusterer_from_tree(raw,m,1)
            tree, stats = result['condensed_tree'],result['stats']
            self.assertEqual(stats['condensed_clusters'],clusters)
            self.assertEqual(stats['selected_clusters'],selected)
            self.assertEqual(stats['noise_points'],noise)
            self.assertEqual(tree['n_points'],210)
            self.assertEqual(sorted(tree['point_exit_ids']),list(range(210)))
            self.assertEqual(stats['points_removed_from_input'],0)
            self.assertEqual(stats['raw_internal_nodes'],stats['retained_source_internal_nodes']+
                             stats['contracted_equal_height_nodes']+stats['suppressed_small_internal_nodes']+
                             stats['suppressed_continuation_internal_nodes'])
            self.verify_cuts(tree)

    def test_common_eom_identity_z_topology_and_input_immutable(self):
        raw = one_large_child(); saved = deepcopy(raw)
        previous, previous_tree = None, None
        for z in (1,2):
            result = point_clusterer_from_tree(raw,2,z)
            legacy = _legacy().condense_eom(raw['n'],raw['children'],raw['heights'],min_cluster_size=2,exp_z=z)
            self.assertEqual(result['selection']['labels'],legacy['labels'])
            self.assertEqual(result['selection']['selected_legacy_ids'],legacy['selected'])
            self.assertEqual(result['condensed_tree']['own_stability'],list(legacy['stabilities'].values()))
            signature = [result['condensed_tree'][key] for key in
                         ('parent','mass_at_birth','children_offsets','children','point_exit_parent','point_exit_ids')]
            if previous is not None:
                self.assertEqual(signature,previous)
                for key in ('birth_lambda','death_lambda','point_exit_lambda'):
                    self.assertEqual(result['condensed_tree'][key],[x*x for x in previous_tree[key]])
            previous = signature
            previous_tree = result['condensed_tree']
            self.verify_cuts(result['condensed_tree'])
        self.assertEqual(raw,saved)

    def test_increasing_threshold_does_not_increase_cluster_tree_count(self):
        raw = groups_tree([2,3,5,10],noise=1)
        for z in (1,2):
            counts = []
            for m in (2,3,5,10):
                result = point_clusterer_from_tree(raw,m,z)
                counts.append(result['stats']['condensed_clusters'])
                self.assertEqual(sorted(result['condensed_tree']['point_exit_ids']),list(range(raw['n'])))
            self.assertEqual(counts,[5,4,3,1])
            self.assertTrue(all(a>=b for a,b in zip(counts,counts[1:])))
        # This check concerns cluster-tree count, never monotonic EOM labels.

    def test_plateau_atomization_and_zero_radius_infinity(self):
        raw = dict(n=4,children={4:[0,1],5:[2,3],6:[4,5]},heights={4:1.,5:1.,6:1.})
        result = point_clusterer_from_tree(raw,2)
        self.assertEqual(result['stats']['contracted_equal_height_nodes'],2)
        self.assertEqual(result['stats']['condensed_clusters'],1)
        self.assertEqual(result['selection']['labels'],[-1]*4)
        raw = groups_tree([3,3]); raw['heights'][6] = raw['heights'][7] = 0.
        for z in (1,2):
            result = point_clusterer_from_tree(raw,2,z)
            tree = result['condensed_tree']
            self.assertTrue(all(math.isinf(x) for x in tree['own_stability'][1:]))
            self.assertFalse(any(math.isnan(x) for x in tree['own_stability']))
            self.assertEqual(result['stats']['suppressed_continuation_internal_nodes'],0)
            self.assertEqual(condensed_cut(tree,math.inf)['inactive_points'],list(range(6)))
            self.assertEqual(len(condensed_cut(tree,1e100)['clusters']),2)
            self.verify_cuts(tree)

    def test_singleton_structural_root_and_csr_roundtrip(self):
        for z in (1,2):
            result = point_clusterer_from_tree(dict(n=1,children={},heights={}),100,z)
            self.assertEqual(result['stats']['structural_root_added'],1)
            for key in ('raw_internal_nodes','retained_source_internal_nodes','contracted_equal_height_nodes',
                        'suppressed_small_internal_nodes','suppressed_continuation_internal_nodes','suppressed_internal_nodes'):
                self.assertEqual(result['stats'][key],0)
            self.assertEqual(result['condensed_tree']['mass_at_birth'],[1])
            self.assertEqual(result['condensed_tree']['point_exit_ids'],[0])
            self.assertEqual(result['selection']['labels'],[-1])
            self.verify_cuts(result['condensed_tree'])
        finite = point_clusterer_from_tree(one_large_child(),2)['condensed_tree']
        self.assertEqual(validate_condensed_tree(json.loads(json.dumps(finite,allow_nan=False))),
                         validate_condensed_tree(finite))

    def test_condensed_corruptions_rejected(self):
        good = point_clusterer_from_tree(one_large_child(),2)['condensed_tree']
        mutations = [lambda x:x.update(schema='bad'),lambda x:x.update(root=1),
                     lambda x:x.update(min_cluster_size=3),lambda x:x.update(exp_z=3),
                     lambda x:x['parent'].__setitem__(1,1),lambda x:x['children'].__setitem__(0,2),
                     lambda x:x['children_offsets'].__setitem__(1,1),
                     lambda x:x['mass_at_birth'].__setitem__(1,3),
                     lambda x:x['birth_lambda'].__setitem__(1,-1.),
                     lambda x:x['birth_lambda'].__setitem__(1,.5),
                     lambda x:x['death_lambda'].__setitem__(0,.5),
                     lambda x:x['own_stability'].__setitem__(1,0.),
                     lambda x:x['point_exit_ids'].__setitem__(0,0),
                     lambda x:x['point_exit_parent'].__setitem__(0,0),
                     lambda x:x['point_exit_lambda'].__setitem__(0,.1),
                     lambda x:x['point_exit_lambda'].__setitem__(0,float('nan')),
                     lambda x:x['point_exit_lambda'].pop(),
                     lambda x:x['point_exit_offsets'].__setitem__(1,2),
                     lambda x:x['legacy_cluster_ids'].__setitem__(1,99)]
        for mutate in mutations:
            bad = deepcopy(good); mutate(bad)
            with self.assertRaises(ValueError):
                validate_condensed_tree(bad)

    def test_raw_refusals_and_cut_refusals(self):
        for m,z in ((1,1),(True,1),(2,3),(2,True)):
            with self.assertRaises(ValueError):
                point_clusterer_from_tree(one_large_child(),m,z)
        raw = one_large_child(); raw['root'] = 7
        with self.assertRaises(ValueError):
            point_clusterer_from_tree(raw,2)
        good = point_clusterer_from_tree(one_large_child(),2)['condensed_tree']
        for cut in (-1.,float('nan'),True):
            with self.assertRaises(ValueError):
                condensed_cut(good,cut)


if __name__ == '__main__':
    unittest.main(verbosity=2)
