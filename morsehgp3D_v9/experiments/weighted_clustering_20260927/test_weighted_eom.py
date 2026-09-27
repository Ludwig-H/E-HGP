"""Small offline gates: exact bottom-up Fraction oracle and pinned unit EOM.

The oracle constructs eligible components in INCREASING radius, independently
of the implementation's descending condensed traversal. It maintains explicit
leaf join dates and compares selected sets and every non-root stability.
No geometry, benchmark, third-party estimator fitting, cloud or file writes.
"""
from copy import deepcopy
from fractions import Fraction as Q
import hashlib
import importlib.util
import math
from pathlib import Path
import random
import unittest

from weighted_eom import PORTED_SHA256, weighted_condense_eom


def exact_oracle(n, children, heights, masses, threshold, z):
    """Independent bottom-up condensation; None denotes symbolic +infinity."""
    masses, threshold = list(map(Q, masses)), Q(threshold)
    children = {node: list(row) for node, row in children.items() if row}
    heights = {node: Q(value) for node, value in heights.items()}
    root = (set(range(n)) | set(children)) - {c for row in children.values() for c in row}
    root = next(iter(root))
    clusters = []

    def same_level_children(node):
        result = []
        for child in children[node]:
            if child in children and heights[child] == heights[node]:
                result.extend(same_level_children(child))
            else:
                result.append(child)
        return result

    def new_cluster(members, at, child_clusters=()):
        cid = len(clusters)
        clusters.append(dict(members=set(members), joins={p: at for p in members},
                             children=list(child_clusters), stability=None))
        return cid

    def close(cid, at):
        terms = []
        for p, joined in clusters[cid]['joins'].items():
            if joined == at:
                terms.append(Q(0))
            elif joined is None:
                terms.append(None)
            else:
                if at is None or joined < at:
                    raise ValueError("oracle lifetime reversed")
                terms.append(masses[p]*(joined-at))
        clusters[cid]['stability'] = None if None in terms else sum(terms, Q(0))

    def visit(node):
        if node < n:
            members = {node}
            cid = new_cluster(members, None) if masses[node] >= threshold else None
            return members, cid
        states = [visit(child) for child in same_level_children(node)]
        members = set().union(*(row[0] for row in states))
        at = None if heights[node] == 0 else Q(1)/heights[node]**z
        qualified = [cid for _, cid in states if cid is not None]
        if len(qualified) >= 2:
            for cid in qualified:
                close(cid, at)
            return members, new_cluster(members, at, qualified)
        if len(qualified) == 1:
            cid = qualified[0]
            for p in members-clusters[cid]['members']:
                clusters[cid]['joins'][p] = at
            clusters[cid]['members'] = members
            return members, cid
        total = sum((masses[p] for p in members), Q(0))
        return members, new_cluster(members, at) if total >= threshold else None

    _, root_cid = visit(root)
    if root_cid is None:
        return {}, []
    close(root_cid, Q(0))

    def choose(cid):
        descendants = [choose(c) for c in clusters[cid]['children']]
        total = None if any(value is None for value, _ in descendants) else sum((value for value, _ in descendants), Q(0))
        own = clusters[cid]['stability']
        better_children = own is not None and (total is None or total > own)
        if cid != root_cid and not better_children:
            return own, [frozenset(clusters[cid]['members'])]
        return total, [group for _, groups in descendants for group in groups]

    _, selected = choose(root_cid)
    values = {frozenset(c['members']): c['stability'] for i, c in enumerate(clusters) if i != root_cid}
    return values, selected


def result_birth_sets(result):
    sets = {c: set() for c in result['births']}
    for leaf, parent in enumerate(result['leaf_parent']):
        sets[parent].add(leaf)
    for cluster in reversed(list(sets)):
        if cluster in result['cluster_parent']:
            sets[result['cluster_parent'][cluster]].update(sets[cluster])
    return sets


def random_tree(rng, n):
    active, children, heights = list(range(n)), {}, {}
    for step in range(n-1):
        first = active.pop(rng.randrange(len(active)))
        second = active.pop(rng.randrange(len(active)))
        node = n+step
        children[node] = [first, second]
        heights[node] = 2**(step//2)  # deliberate atomic ties, exactly dyadic lambda
        active.append(node)
    return children, heights


class WeightedEOMTests(unittest.TestCase):
    exact_cases = 0
    unit_cases = 0
    refusals = 0

    def run_case(self, n, children, heights, masses, threshold, z):
        original = deepcopy((children, heights, masses))
        result = weighted_condense_eom(n, children, heights, masses,
                                      min_cluster_size=threshold, exp_z=z)
        self.assertEqual((children, heights, masses), original)
        expected, selection = exact_oracle(n, children, heights, masses, threshold, z)
        sets = result_birth_sets(result)
        root = n
        actual = {frozenset(sets[c]): score for c, score in result['stabilities'].items() if c != root}
        self.assertEqual(set(actual), set(expected))
        for members, value in expected.items():
            self.assertEqual(actual[members], math.inf if value is None else float(value))
        self.assertEqual({frozenset(sets[c]) for c in result['selected']}, set(selection))
        self.assertEqual(result['leaf_labels'], result['labels'])
        self.assertNotIn(root, result['selected'])
        self.assertEqual(len([e for e in result['condensed_tree'] if e['child'] < n]), n)
        self.assertEqual(set(sets[root]), set(range(n)))
        for label, cluster in enumerate(result['selected']):
            self.assertEqual(sets[cluster], {p for p, value in enumerate(result['labels']) if value == label})
        for cluster, members in sets.items():
            self.assertEqual(result['cluster_mass_at_birth'][cluster], float(sum((Q(masses[p]) for p in members), Q(0))))
        for edge in result['condensed_tree']:
            self.assertFalse(math.isnan(edge['duration']))
            self.assertGreaterEqual(edge['duration'], 0)
        type(self).exact_cases += 1
        return result

    def test_fraction_oracle_random(self):
        rng = random.Random(27092026)
        for n in range(2, 13):
            for _ in range(12):
                children, heights = random_tree(rng, n)
                masses = [Q(rng.randint(1, 8), 4) for _ in range(n)]
                threshold = Q(rng.randint(1, 16), 4)
                for z in (1, 2):
                    self.run_case(n, children, heights, masses, threshold, z)

    def test_zero_one_two_large_children_and_multifurcation(self):
        children = {8:[0,1], 9:[2,3], 10:[4,5], 11:[8,9,10,6,7]}
        heights = {8:1, 9:2, 10:4, 11:8}
        masses = [Q(1,2),Q(1,2),1,1,2,2,Q(1,4),Q(1,4)]
        for threshold in (Q(1,2), 1, 2, 3, 5, 20):
            for z in (1,2):
                self.run_case(8,children,heights,masses,threshold,z)

    def test_atomic_plateaux(self):
        binary = {4:[0,1],5:[2,3],6:[4,5]}
        star = {6:[3,1,0,2]}
        masses = [Q(1,2)]*4
        for z in (1,2):
            first = self.run_case(4,binary,{4:2,5:2,6:2},masses,1,z)
            second = self.run_case(4,star,{6:2},masses,1,z)
            for key in ('labels','births','stabilities','condensed_tree','leaf_exit_lambda'):
                self.assertEqual(first[key],second[key])
            self.assertEqual(first['contracted_internal_ties'],2)
            self.assertEqual(first['selected'],[])

    def test_infinity_leaf_and_zero_duration(self):
        for z in (1,2):
            positive = self.run_case(2,{2:[0,1]},{2:2},[Q(3,2),Q(3,2)],1,z)
            self.assertEqual(positive['labels'],[0,1])
            self.assertEqual(positive['leaf_exit_lambda'],[math.inf,math.inf])
            self.assertTrue(all(math.isinf(positive['stabilities'][c]) for c in positive['selected']))
            zero = self.run_case(2,{2:[0,1]},{2:0},[2,2],1,z)
            self.assertTrue(math.isinf(zero['stabilities'][2]))
            self.assertTrue(all(zero['stabilities'][c] == 0 for c in zero['selected']))
            self.assertTrue(all(e['duration']==0 for e in zero['condensed_tree'] if e['child']<2))
        singleton = self.run_case(1,{}, {},[5],1,2)
        self.assertEqual(singleton['selected'],[])
        self.assertEqual(singleton['labels'],[-1])

    def test_pinned_unit_mass_equivalence(self):
        old_path = Path(__file__).resolve().parents[2]/'audits/b_point_hierarchy_k_20260927/eom.py'
        self.assertEqual(hashlib.sha256(old_path.read_bytes()).hexdigest(),PORTED_SHA256)
        spec = importlib.util.spec_from_file_location('weighted_unit_reference',old_path)
        old = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(old)
        rng = random.Random(517)
        for n in range(1,16):
            children, heights = random_tree(rng,n)
            for threshold in (2,3,5,20):
                for z in (1,2):
                    actual = weighted_condense_eom(n,children,heights,[1]*n,min_cluster_size=threshold,exp_z=z)
                    expected = old.condense_eom(n,children,heights,min_cluster_size=threshold,exp_z=z)
                    for key in ('labels','selected','selected_sources','births','stabilities','eom_comparisons'):
                        self.assertEqual(actual[key],expected[key])
                    self.assertEqual(actual['leaf_parent'],expected['point_parent'])
                    self.assertEqual(actual['leaf_exit_lambda'],expected['point_exit_lambda'])
                    edges = [dict(parent=e['parent'],child=e['child'],lambda_value=e['lambda_value'],size=e['mass']) for e in actual['condensed_tree']]
                    self.assertEqual(edges,expected['condensed_tree'])
                    type(self).unit_cases += 1

    def test_fraction_input_and_near_threshold_warning(self):
        result = weighted_condense_eom(3,{3:[0,1],4:[3,2]},{3:1,4:2},
                                      [Q(1,3),Q(2,3),Q(3,2)],min_cluster_size=1,exp_z=2)
        self.assertIn(3,result['near_threshold_nodes'])
        self.assertIn('near_or_equal_float_mass_threshold:3',result['warnings'])
        self.assertEqual(result['certification'],'not_claimed')
        self.assertEqual(result['leaf_masses'],[float(Q(1,3)),float(Q(2,3)),1.5])
        self.assertEqual(result['conservation']['leaf_exits'],3)

    def test_deep_comb_exact_dyadic_mass(self):
        n=10001
        children={n:[0,1]}
        children.update({n+i-1:[n+i-2,i] for i in range(2,n)})
        heights={node:float(node-n+1) for node in children}
        masses=[1.0]+[2.0**-53]*(n-1)
        exact=Q(1)+(n-1)*Q(1,2**53)
        for z in (1,2):
            result=weighted_condense_eom(n,children,heights,masses,min_cluster_size=2,exp_z=z)
            self.assertEqual(result['total_mass'],float(exact))
            self.assertEqual(result['conservation']['exit_mass'],float(exact))
            self.assertTrue(result['conservation']['exact_dyadic_input_mass_conserved'])
            self.assertEqual(result['conservation']['cluster_mass_residuals'],{n:0.0})
            self.assertEqual(result['labels'],[-1]*n)
            self.assertEqual(result['leaf_masses'],masses)
            self.assertEqual(result['conservation']['leaf_exits'],n)

    def test_atomic_mass_rounding_independence(self):
        masses=[1.0,2.0**-53,2.0**-53,2.0]
        threshold=1.0+2.0**-52
        binary={4:[0,1],5:[4,2],6:[5,3]}
        star={5:[0,1,2],6:[5,3]}
        for z in (1,2):
            first=weighted_condense_eom(4,binary,{4:1,5:1,6:2},masses,min_cluster_size=threshold,exp_z=z)
            second=weighted_condense_eom(4,star,{5:1,6:2},masses,min_cluster_size=threshold,exp_z=z)
            for key in ('labels','births','stabilities','cluster_mass_at_birth','condensed_tree','leaf_exit_lambda'):
                self.assertEqual(first[key],second[key])
            self.assertEqual(first['labels'],[0,0,0,1])

    def test_mass_and_radius_scaling(self):
        children={6:[0,1],7:[2,3],8:[4,5],9:[6,7,8]}
        heights={6:1,7:2,8:4,9:8}; masses=[Q(1,2)]*6
        for z in (1,2):
            original=weighted_condense_eom(6,children,heights,masses,min_cluster_size=1,exp_z=z)
            mass_scaled=weighted_condense_eom(6,children,heights,[4*x for x in masses],min_cluster_size=4,exp_z=z)
            radius_scaled=weighted_condense_eom(6,children,{k:2*v for k,v in heights.items()},masses,min_cluster_size=1,exp_z=z)
            self.assertEqual(original['labels'],mass_scaled['labels'])
            self.assertEqual(original['labels'],radius_scaled['labels'])
            for c,value in original['stabilities'].items():
                self.assertEqual(mass_scaled['stabilities'][c],4*value)
                self.assertEqual(radius_scaled['stabilities'][c],value/2**z)

    def test_list_schema_and_child_permutation(self):
        mappings={4:[0,1],5:[2,3],6:[4,5]}; heights={4:1,5:2,6:4}
        expected=weighted_condense_eom(4,mappings,heights,[Q(3,4)]*4,min_cluster_size=1)
        actual=weighted_condense_eom(4,[[],[],[],[],[1,0],[3,2],[5,4]],
            [0,0,0,0,1,2,4],{i:Q(3,4) for i in range(4)},min_cluster_size=1)
        self.assertEqual(actual,expected)

    def test_refusals(self):
        base=dict(n_leaves=2,children={2:[0,1]},heights={2:1},masses=[1,1],min_cluster_size=1)
        mutations=[dict(n_leaves=True),dict(n_leaves=0),dict(masses=[True,1]),dict(masses=[0,1]),
          dict(masses=[-1,1]),dict(masses=[math.inf,1]),dict(masses=[math.nan,1]),dict(masses=[1]),
          dict(masses={False:1,1:1}),dict(masses=[Q(1,10**400),1]),dict(masses=[1e308,1e308]),
          dict(min_cluster_size=True),dict(min_cluster_size=0),dict(min_cluster_size=math.nan),
          dict(exp_z=True),dict(exp_z=3),dict(exp_z=1.0),dict(allow_single_cluster=True),
          dict(allow_single_cluster=0),dict(children={2:[0,0]}),dict(children={2:[0]}),
          dict(children={2:[0,3]}),dict(children={}),dict(children={0:[1],2:[0,1]}),
          dict(heights={2:True}),dict(heights={2:-1}),dict(heights={0:1,2:2}),
          dict(heights={2:math.inf}),dict(heights={2:1e-320}),dict(heights={2:1e308},exp_z=2),
          dict(heights={2:1e300},masses=[1e-100,1e-100],min_cluster_size=1),
          dict(heights={2:1e-100},masses=[1e300,1e300],min_cluster_size=1e301)]
        for mutation in mutations:
            with self.subTest(mutation=mutation),self.assertRaises(ValueError):
                weighted_condense_eom(**dict(base,**mutation))
            type(self).refusals += 1


if __name__=='__main__':
    suite=unittest.defaultTestLoader.loadTestsFromTestCase(WeightedEOMTests)
    outcome=unittest.TextTestRunner(verbosity=2).run(suite)
    print(f"weighted_eom gates={outcome.testsRun} exact_fraction_cases={WeightedEOMTests.exact_cases} "
          f"pinned_unit_cases={WeightedEOMTests.unit_cases} refusals={WeightedEOMTests.refusals} "
          f"status={'PASS' if outcome.wasSuccessful() else 'FAIL'}")
    raise SystemExit(0 if outcome.wasSuccessful() else 1)
