"""Independent small-tree gates of the point-tree/common-EOM adapter.

The exact increasing-radius Fraction oracle is explicitly reused from the
frozen weighted tests, with UNIT masses. It is independent of the descending
condensation implementation. No geometry, fit, benchmark, cloud or file write.
These gates do not certify binary64 EOM margins on arbitrary real radii.
"""
from copy import deepcopy
from fractions import Fraction
import hashlib
import importlib.util
import math
from pathlib import Path
import random
import sys
import unittest

from point_eom import cluster_point_tree

WEIGHTED = Path(__file__).resolve().parent.parent / 'weighted_clustering_20260927'
ORACLE_SOURCE = WEIGHTED / 'test_weighted_eom.py'
ORACLE_SHA256 = '0cad62cf9690c64307676f2c30d989fe2393a769196be70eec46b51a7684a87d'


def load_oracle():
    if hashlib.sha256(ORACLE_SOURCE.read_bytes()).hexdigest() != ORACLE_SHA256:
        raise ValueError('independent Fraction oracle source changed')
    spec = importlib.util.spec_from_file_location('mhgp9_point_eom_independent_oracle', ORACLE_SOURCE)
    if spec is None or spec.loader is None:
        raise ValueError('oracle import specification')
    module = importlib.util.module_from_spec(spec)
    sys.path.insert(0, str(WEIGHTED))
    try:
        spec.loader.exec_module(module)
    finally:
        sys.path.pop(0)
    return module.exact_oracle, module.random_tree


EXACT_ORACLE, RANDOM_TREE = load_oracle()


def point_tree(n, children, radii):
    nodes = set(range(n)) | set(children)
    roots = sorted(nodes - {child for row in children.values() for child in row})
    return dict(n_points=n, children=deepcopy(children),
                squared_levels={node: Fraction(radius)**2 for node, radius in radii.items()},
                roots=roots)


def birth_sets(condensed):
    groups = [set() for _ in condensed['parent']]
    for point, parent in enumerate(condensed['point_exit_parent']):
        groups[parent].add(point)
    for node in range(len(groups)-1, 0, -1):
        groups[condensed['parent'][node]].update(groups[node])
    return groups


class PointEOMAudit(unittest.TestCase):
    oracle_cases = 0

    def check_case(self, n, children, radii, threshold, z):
        tree = point_tree(n, children, radii)
        saved = deepcopy(tree)
        output = cluster_point_tree(tree, min_cluster_size=threshold, exp_z=z)
        self.assertEqual(tree, saved)
        result = output['result']
        condensed, selection = result['condensed_tree'], result['selection']
        labels, groups = selection['labels'], birth_sets(condensed)
        expected_scores, expected_selected = EXACT_ORACLE(
            n, children, radii, [1]*n, threshold, z)
        actual_scores = {frozenset(group): condensed['own_stability'][node]
                         for node, group in enumerate(groups) if node != 0}
        self.assertEqual(set(actual_scores), set(expected_scores))
        for members, score in expected_scores.items():
            self.assertEqual(actual_scores[members], math.inf if score is None else float(score))
        actual_selected = {frozenset(groups[node]) for node in selection['selected']}
        self.assertEqual(actual_selected, set(expected_selected))
        labelled_sets = {frozenset(p for p, value in enumerate(labels) if value == label)
                        for label in set(labels) if label >= 0}
        self.assertEqual(labelled_sets, actual_selected)
        self.assertTrue(all(len(group) >= threshold for group in labelled_sets))
        covered = set().union(*expected_selected) if expected_selected else set()
        self.assertEqual({p for p, label in enumerate(labels) if label < 0}, set(range(n))-covered)
        self.assertEqual(len(labels), n)
        self.assertNotIn(0, selection['selected'])
        self.assertEqual(groups[0], set(range(n)))
        self.assertEqual(condensed['mass_at_birth'], [len(group) for group in groups])
        self.assertEqual(sorted(condensed['point_exit_ids']), list(range(n)))
        self.assertEqual(result['stats']['points_removed_from_input'], 0)
        self.assertTrue(all(not math.isnan(score) for score in condensed['own_stability']))
        type(self).oracle_cases += 1
        return output

    def test_fraction_oracle_random_unit_trees(self):
        rng = random.Random(2709202601)
        for n in range(2, 11):
            for _ in range(3):
                children, radii = RANDOM_TREE(rng, n)
                for m in sorted({2, 3, 5, n+1}):
                    for z in (1, 2):
                        self.check_case(n, children, radii, m, z)

    def test_multifurcation_exact_cardinality_and_residual_noise(self):
        children = {7:[0,1], 8:[2,3], 9:[4,5], 10:[7,8,9,6]}
        radii = {7:1, 8:1, 9:1, 10:4}
        for z in (1,2):
            output = self.check_case(7, children, radii, 2, z)
            self.assertEqual(output['result']['selection']['labels'], [0,0,1,1,2,2,-1])
            self.assertEqual(output['result']['condensed_tree']['point_exit_lambda'][6], 1/4**z)
            for m in (3, 7, 8):
                self.assertEqual(self.check_case(7, children, radii, m, z)
                                 ['result']['selection']['labels'], [-1]*7)

    def test_one_large_child_continues_root_not_selected(self):
        children = {5:[0,1], 6:[2,3], 7:[5,6], 8:[7,4]}
        radii = {5:1, 6:1, 7:2, 8:4}
        for z in (1,2):
            output = self.check_case(5, children, radii, 2, z)
            self.assertEqual(output['result']['selection']['labels'], [0,0,1,1,-1])
            self.assertEqual(output['result']['stats']['suppressed_continuation_internal_nodes'], 1)
            self.assertEqual(self.check_case(5, children, radii, 3, z)
                             ['result']['selection']['labels'], [-1]*5)

    def test_dated_exits_stay_labelled_if_ancestor_selected(self):
        children = {12:[0,1], 13:[2,3], 14:[6,7,8], 15:[12,13,4,5], 16:[15,14]}
        radii = {12:1, 13:1, 14:1, 15:2, 16:16}
        first = self.check_case(9, children, radii, 2, 1)['result']
        second = self.check_case(9, children, radii, 2, 2)['result']
        self.assertEqual(first['selection']['labels'], [0]*6+[1]*3)
        self.assertEqual(second['selection']['labels'], [0,0,1,1,-1,-1,2,2,2])
        # z changes selection, not the unit-mass condensed topology of a FIXED tree.
        for key in ('parent','mass_at_birth','children_offsets','children','point_exit_parent','point_exit_ids'):
            self.assertEqual(first['condensed_tree'][key], second['condensed_tree'][key])

    def test_atomic_plateau_and_child_order(self):
        binary = {4:[0,1], 5:[2,3], 6:[4,5]}
        for z in (1,2):
            first = self.check_case(4, binary, {4:2,5:2,6:2}, 2, z)
            second = self.check_case(4, {6:[3,1,0,2]}, {6:2}, 2, z)
            self.assertEqual(first['result']['selection']['labels'], [-1]*4)
            self.assertEqual(first['result']['condensed_tree'], second['result']['condensed_tree'])

    def test_zero_radius_and_singleton(self):
        children = {4:[0,1],5:[2,3],6:[4,5]}
        for z in (1,2):
            output = self.check_case(4, children, {4:0,5:0,6:2}, 2, z)
            self.assertEqual(output['result']['selection']['labels'], [0,0,1,1])
            self.assertTrue(all(math.isinf(score) for score in
                                output['result']['condensed_tree']['own_stability'][1:]))
            self.assertEqual(self.check_case(4, children, {4:0,5:0,6:0}, 2, z)
                             ['result']['selection']['labels'], [-1]*4)
            self.assertEqual(self.check_case(1, {}, {}, 2, z)
                             ['result']['selection']['labels'], [-1])

    def test_additional_metadata_does_not_change_labels(self):
        tree = point_tree(4, {4:[0,1],5:[2,3],6:[4,5]}, {4:1,5:1,6:4})
        baseline = cluster_point_tree(tree, min_cluster_size=2)['result']['selection']['labels']
        tree.update(provenance={'diagnostic_only': True}, leaf_birth_betas=[Fraction(0)]*4)
        actual = cluster_point_tree(tree, min_cluster_size=2)['result']['selection']['labels']
        self.assertEqual(actual, baseline)

    def test_explicit_domain_refusals(self):
        good = point_tree(4, {4:[0,1],5:[2,3],6:[4,5]}, {4:1,5:1,6:4})
        for m in (0,1,-1,True,2.0):
            with self.assertRaises(ValueError):
                cluster_point_tree(good, min_cluster_size=m)
        for z in (0,3,True,1.0):
            with self.assertRaises(ValueError):
                cluster_point_tree(good, min_cluster_size=2, exp_z=z)
        for bad in (point_tree(0,{},{}), point_tree(2,{},{})):
            with self.assertRaises(ValueError):
                cluster_point_tree(bad, min_cluster_size=2)

    def test_unrepresentable_positive_beta_and_radius_collision(self):
        for beta in (Fraction(1,10**1000), Fraction(10**1000), Fraction(-1)):
            tree = dict(n_points=2, children={2:[0,1]}, squared_levels={2:beta}, roots=[2])
            with self.assertRaises(ValueError):
                cluster_point_tree(tree, min_cluster_size=2)
        tree = point_tree(4, {4:[0,1],5:[2,3],6:[4,5]}, {4:1,5:1,6:4})
        tree['squared_levels'][5] = Fraction(2**53+1,2**53)
        with self.assertRaises(ValueError):
            cluster_point_tree(tree, min_cluster_size=2)


if __name__ == '__main__':
    unittest.main()
