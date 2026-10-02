#!/usr/bin/env python3
"""Fixtures exactes de projection : trois points, aucun calcul HDBSCAN.

Source : audit_full_hierarchie_20261002/suivi_verrous/points_review.
Les deux etages de la reference doivent conserver le bloc AB de cover ;
les formules du temoin MR2-bord de la v10 donnent une arrivee simultanee
de C au plateau AB. Cela distingue les familles de blocs avant selection.
"""
from fractions import Fraction
import unittest

from hgp11_ref import Definition, Reference, judge


class ProjectionContracts(unittest.TestCase):
    def test_cover_keeps_ab(self):
        for shift, scale in ((0, 1), (13, 1), (7, 3)):
            points = [(shift + scale * x, 0, 0) for x in (0, 2, 5)]
            square = scale * scale
            expected_levels = [Fraction(square), Fraction(9 * square, 4),
                               Fraction(25 * square, 4)]
            for stage in (Definition(points), Reference(points, 2)):
                result = stage.order(2)
                self.assertIsNone(judge.validate_tree(result.nodes))
                self.assertEqual([node.level for node in result.nodes], expected_levels)
                self.assertEqual([node.children for node in result.nodes], [(), (), (0, 1)])
                self.assertEqual([entry.level for entry in result.cover],
                                 [square, square, Fraction(9 * square, 4)])
                self.assertEqual([entry.nodes for entry in result.cover],
                                 [frozenset([0]), frozenset([0]), frozenset([1])])
                opened, closed = judge.cut_at(result, Fraction(25 * square, 4))
                self.assertEqual(sorted(coverage for _, coverage, _ in opened), [3, 6])
                self.assertEqual([coverage for _, coverage, _ in closed], [7])

    def test_mr_border_has_no_intermediate_ab(self):
        # Definition du temoin v10 : core_i = alpha^2 d_K(i)^2 et
        # e_i = min_j max(core_j, distance(i,j)^2). K2 inclut soi-meme.
        # Sur ces trois sites tous les evenements de bord ont meme date.
        for shift, scale in ((0, 1), (13, 1), (7, 3)):
            points = [shift + scale * x for x in (0, 2, 5)]
            distance = [[(a - b) ** 2 for b in points] for a in points]
            core = [4 * sorted(row)[1] for row in distance]
            entries = [min(max(core[j], row[j]) for j in range(3)) for row in distance]
            # En cas d'egalite, le temoin prefere le site lui-meme.
            carriers = [min(range(3), key=lambda j: (max(core[j], row[j]), j != i, j))
                        for i, row in enumerate(distance)]
            square = scale * scale
            self.assertEqual(core, [16 * square, 16 * square, 36 * square])
            self.assertEqual(entries, [16 * square] * 3)
            self.assertEqual(carriers, [0, 1, 1])
            self.assertEqual(max(core[0], core[1], distance[0][1]), entries[2])
        target = frozenset([0, 1])
        cover_blocks = [target, frozenset(range(3))]
        mr_border_blocks = [frozenset([i]) for i in range(3)] + [frozenset(range(3))]
        def iou(block):
            return Fraction(len(target & block), len(target | block))
        self.assertEqual(max(map(iou, cover_blocks)), 1)
        self.assertEqual(max(map(iou, mr_border_blocks)), Fraction(2, 3))

    def test_memo_is_not_valid_at_open_merge(self):
        points = [(0, 0, 0), (2, 0, 0), (4, 0, 0)]
        for stage in (Definition(points), Reference(points, 2)):
            opened, closed = judge.cut_at(stage.order(2), Fraction(4))
            self.assertEqual(sorted(coverage for _, coverage, _ in opened), [3, 6])
            self.assertEqual([coverage for _, coverage, _ in closed], [7])


if __name__ == '__main__':
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(ProjectionContracts)
    if suite.countTestCases() != 3:
        raise SystemExit(3)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if not result.wasSuccessful():
        raise SystemExit(1)
    print('projection_contracts_ok faits=3')
