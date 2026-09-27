"""Exact regression gates for full Cech attachments; no native execution."""
from fractions import Fraction as F
from itertools import combinations
import unittest

from full_attachment_oracle import (E5, build_reference, reference_cut, tree_cut,
                                    virtual_tree_for_threshold)
from qualify_geometry import graph_coverage
from weighted_model import build_facet_model


def coface_rows(reference):
    return [dict(vertices=list(v), beta=dict(num=str(b.numerator), den=str(b.denominator)))
            for v, b in reference['gabriel_cofaces'].items()]


class FullAttachmentTests(unittest.TestCase):
    def test_e5_known_exact_counterexample(self):
        ref = build_reference(E5, 2)
        cut = F(83886, 3563)
        rows = reference_cut(ref, cut)
        nontrivial = [row['coverage'] for row in rows if len(row['gamma_facets']) > 1]
        self.assertEqual(nontrivial, [(0,1,2,3,4)])
        self.assertEqual(graph_coverage(2, ref['gabriel_cofaces'], cut, True),
                         [(0,1,2), (0,2,3,4)])
        ac = ref['facets'].index((0,2))
        self.assertEqual(ref['leaf_birth_betas'][ac], F(33,2))
        self.assertEqual(ref['first_nontrivial_betas'][ac], F(33,2))
        self.assertEqual(ref['first_gabriel_incidence_betas'][ac], cut)
        silent = [event for event in ref['events'] if event['beta'] == F(33,2)]
        self.assertEqual(len(silent), 1)
        self.assertEqual(silent[0]['newly_born_facets'], (ac,))
        self.assertEqual(silent[0]['previous_weighted_components'], 1)

    def test_e5_silent_attachment_changes_mass_not_point_cover(self):
        ref = build_reference(E5, 2)
        before = [row for row in reference_cut(ref, F(33,2), closed=False)
                  if row['coverage'] == (0,2,3,4)][0]
        after = [row for row in reference_cut(ref, F(33,2), closed=True)
                 if row['coverage'] == (0,2,3,4)][0]
        ac = ref['facets'].index((0,2))
        self.assertEqual(set(after['facet_ids'])-set(before['facet_ids']), {ac})
        self.assertEqual(after['mass']-before['mass'], ref['masses'][ac])
        self.assertGreater(ref['masses'][ac], 0)

    def test_birth_not_first_nontrivial_connection(self):
        ref = build_reference(((0,0,0),(2,2,0),(2,0,2)), 2)
        self.assertEqual(ref['leaf_birth_betas'], [F(2)]*3)
        self.assertEqual(ref['first_nontrivial_betas'], [F(8,3)]*3)
        self.assertEqual(ref['first_gabriel_incidence_betas'], [F(8,3)]*3)
        self.assertEqual(tree_cut(ref, F(2), closed=False), [])
        self.assertEqual(tree_cut(ref, F(2)), [(0,), (1,), (2,)])
        self.assertEqual(tree_cut(ref, F(8,3), closed=False), [(0,), (1,), (2,)])
        self.assertEqual(tree_cut(ref, F(8,3)), [(0,1,2)])

    def test_all_critical_cuts_match_direct_cech(self):
        fixtures = [(E5, [1,2,3,4]),
                    (((0,0,0),(2,0,0),(2,2,0),(0,2,0)), [1,2,3]),
                    (((2,1,1),(0,1,1),(1,2,1),(1,0,1),(1,1,2),(1,1,0)), [1,2,3,5]),
                    (((0,0,0),(4,0,0),(1,1,0),(2,0,0)), [1,2,3]),
                    (((0,0,0),(4,0,0),(0,4,0),(0,0,4),(7,3,2),(9,8,5),(2,9,11)), [2,3,5])]
        checks = 0
        for points, orders in fixtures:
            for k in orders:
                ref = build_reference(points, k)
                for beta in ref['critical_betas']:
                    for closed in (False, True):
                        expected = sorted(row['facet_ids'] for row in reference_cut(ref, beta, closed=closed)
                                          if row['facet_ids'])
                        self.assertEqual(tree_cut(ref, beta, closed=closed), expected)
                        # Moving only isolated leaves to zero cannot change
                        # components whose mass is at least two.
                        eligible = lambda groups: [g for g in groups if sum(ref['masses'][i] for i in g) >= 2]
                        self.assertEqual(eligible(tree_cut(ref, beta, closed=closed, virtual=True)), eligible(expected))
                        checks += 1
        self.assertGreater(checks, 200)

    def test_measure_unchanged_for_both_exponents(self):
        for z in (1,2):
            ref = build_reference(E5, 2, exp_z=z)
            old = build_facet_model(len(E5), 2, coface_rows(ref), exp_z=z, rational_z2=z == 2)
            for field in ('facets', 'scores', 'point_totals', 'masses'):
                self.assertEqual(ref[field], old[field], field)
            self.assertEqual(sum(ref['masses']) if z == 2 else round(sum(ref['masses']), 12), 5)

    def test_zero_point_births_at_k1_and_terminal_empty_measure(self):
        ref = build_reference(((0,0,0),(2,0,0),(4,0,0)), 1)
        self.assertEqual(ref['leaf_birth_betas'], [0,0,0])
        self.assertEqual(tree_cut(ref, F(0), closed=False), [])
        self.assertEqual(tree_cut(ref, F(0)), [(0,), (1,), (2,)])
        terminal = build_reference(E5, 5)
        self.assertEqual(terminal['facets'], [])
        self.assertEqual(terminal['roots'], [])
        self.assertEqual(tree_cut(terminal, max(terminal['critical_betas'])), [])
        with self.assertRaises(ValueError):
            virtual_tree_for_threshold(terminal, 2)

    def test_virtual_profile_rejects_heavy_isolated_facets(self):
        ref = build_reference(((0,0,0),(2,2,0),(2,0,2)), 2)
        self.assertEqual(ref['masses'], [1,1,1])
        with self.assertRaises(ValueError):
            virtual_tree_for_threshold(ref, 1)
        args = virtual_tree_for_threshold(ref, 2)
        self.assertEqual(args['children'], ref['children'])
        self.assertEqual(args['n_leaves'], 3)
        self.assertFalse(args['allow_single_cluster'])
        for value in (0, -1, True, float('inf'), float('nan')):
            with self.assertRaises(ValueError):
                virtual_tree_for_threshold(ref, value)

    def test_translation_scale_and_id_permutation(self):
        ref = build_reference(E5, 2)
        transformed = build_reference(tuple(tuple(3*x+7 for x in p) for p in E5), 2)
        self.assertEqual(ref['facets'], transformed['facets'])
        self.assertEqual(ref['masses'], transformed['masses'])
        self.assertEqual(ref['children'], transformed['children'])
        self.assertEqual(transformed['squared_levels'], {node:9*beta for node,beta in ref['squared_levels'].items()})
        permutation = (4,1,3,0,2)
        renamed = build_reference(tuple(E5[i] for i in permutation), 2)
        def point_facets(reference, groups, mapping):
            return sorted(tuple(sorted(tuple(sorted(mapping[x] for x in reference['facets'][i])) for i in group))
                          for group in groups)
        for beta in ref['critical_betas']:
            for closed in (False, True):
                left = point_facets(ref, tree_cut(ref,beta,closed=closed), tuple(range(5)))
                right = point_facets(renamed, tree_cut(renamed,beta,closed=closed), permutation)
                self.assertEqual(left, right)

    def test_scope_and_argument_refusals(self):
        for k in (0, True, 6):
            with self.assertRaises(ValueError):
                build_reference(E5, k)
        with self.assertRaises(ValueError):
            build_reference(tuple((i,0,0) for i in range(13)), 2)
        with self.assertRaises(ValueError):
            build_reference(E5, 2, exp_z=1, rational_z2=True)
        ref = build_reference(E5, 2)
        for beta in (-1, 1.0, True):
            with self.assertRaises(ValueError):
                tree_cut(ref, beta)
        self.assertFalse(ref['provenance']['artificial_root'])
        self.assertFalse(ref['provenance']['GCP_used'])


if __name__ == '__main__':
    unittest.main()
