"""Pure oracle tests. Synthetic JSON below is NOT native/GPU evidence."""
from copy import deepcopy
from fractions import Fraction as F
import unittest

from qualify_geometry import (Oracle, check_export, fixtures, graph_coverage,
                              rational, solve, support_ball)


def q(n, d=1):
    return dict(num=str(n), den=str(d))


def synthetic_pair():
    points = [[0,0,0], [2,0,0]]
    fixture = dict(name='synthetic_pair_not_execution', points=points, k=1)
    native = dict(schema='mhgp9_fixed_k_export_v1', status='completed', points=points,
                  point_count=2, k=1, validation=dict(all_cut_coverage_replayed=True),
                  roots=[2], nodes=[dict(id=0, level=q(0), children=[], successor=2),
                                    dict(id=1, level=q(0), children=[], successor=2),
                                    dict(id=2, level=q(1), children=[0,1], successor=None)],
                  populations=[dict(interior=[0], shell=[]), dict(interior=[1], shell=[])],
                  contributions=[dict(segment=i, level=q(0), population=i,
                                      include_interior=True, shell_mask=0) for i in range(2)])
    data = dict(schema='mhgp9_weighted_catalogue_export_v1', status='completed',
                catalog_universe='gabriel_complete_boundary', beta_unit='squared_radius_grid_units',
                shell_cap=12, native=native,
                catalogue=[dict(id=0, key=dict(a='1', b=['-2','0','0'], c='0'), beta=q(1),
                                q_min=2, interior=[], shell=[0,1], minimal_support_masks=[3])],
                cofaces=[dict(vertices=[0,1], beta=q(1), ball=0, shell_mask=3)],
                stats=dict(cardinality_candidates=1, rejected_center_masks=0))
    return fixture, data


class FractionGeometryTests(unittest.TestCase):
    def test_linear_solver_and_singular(self):
        self.assertEqual(solve([[2,1],[1,2]], [1,0]), [F(2,3), F(-1,3)])
        self.assertIsNone(solve([[1,2],[2,4]], [1,2]))

    def test_equilateral_in_integer_three_space(self):
        oracle = Oracle(((0,0,0), (2,2,0), (2,0,2)))
        self.assertEqual(oracle.meb((0,1,2)), ((F(4,3), F(2,3), F(2,3)), F(8,3)))
        self.assertEqual(oracle.meb((0,1))[1], 2)

    def test_obtuse_and_right_supports(self):
        self.assertIsNone(support_ball(((0,0,0), (4,0,0), (1,1,0))))
        oracle = Oracle(((0,0,0), (4,0,0), (1,1,0)))
        self.assertEqual(oracle.meb((0,1,2)), ((F(2), F(0), F(0)), F(4)))
        center, beta, bary = support_ball(((0,0,0), (2,0,0), (0,2,0)))
        self.assertEqual((center, beta), ((F(1), F(1), F(0)), F(2)))
        self.assertIn(0, bary)

    def test_regular_tetrahedron(self):
        oracle = Oracle(((0,0,0), (2,2,0), (2,0,2), (0,2,2)))
        self.assertEqual(oracle.meb((0,1,2,3)), ((F(1), F(1), F(1)), F(3)))

    def test_degenerate_multiple_supports(self):
        oracle = Oracle(((0,0,0), (2,0,0), (2,2,0), (0,2,0)))
        row = oracle.balls[((F(1), F(1), F(0)), F(2))]
        self.assertEqual(row['supports'], [(0,2), (1,3)])
        self.assertEqual(len(oracle.cofaces(2)[1]), 4)

    def test_mandatory_interior(self):
        oracle = Oracle(((0,0,0), (4,0,0), (1,1,0), (2,0,0)))
        self.assertNotIn((0,1,2), oracle.cofaces(2)[1])
        self.assertEqual(oracle.cofaces(3)[1], {(0,1,2,3): F(4)})

    def test_strict_closed_topology(self):
        oracle = Oracle(((0,0,0), (2,2,0), (2,0,2)))
        rows = oracle.cofaces(2)[1]
        self.assertEqual(graph_coverage(2, rows, F(8,3), False), [])
        self.assertEqual(graph_coverage(2, rows, F(8,3), True), [(0,1,2)])

    def test_synthetic_payload_and_semantic_mutants(self):
        fixture, original = synthetic_pair()
        self.assertEqual(check_export(original, fixture)['gabriel_cofaces'], 1)
        mutations = [lambda d: d['cofaces'].clear(),
                     lambda d: d['cofaces'].append(deepcopy(d['cofaces'][0])),
                     lambda d: d['cofaces'][0].update(beta=q(2)),
                     lambda d: d['cofaces'][0].update(shell_mask=1),
                     lambda d: d['catalogue'][0].update(minimal_support_masks=[]),
                     lambda d: d['catalogue'][0].update(interior=[0]),
                     lambda d: d['catalogue'][0]['key'].update(c='1'),
                     lambda d: d['native']['nodes'][2].update(level=q(2)),
                     lambda d: d['native']['contributions'][0].update(include_interior=False),
                     lambda d: d['native']['nodes'][0].update(successor=None)]
        for mutation in mutations:
            changed = deepcopy(original)
            mutation(changed)
            with self.assertRaises(ValueError):
                check_export(changed, fixture)

    def test_domains_and_fixture_inventory(self):
        self.assertEqual(len(fixtures()), 28)
        self.assertTrue(any(f['k'] == 10 and len(f['points']) == 11 for f in fixtures()))
        for points in ([], [(0,0,0)]*2, [(i,0,0) for i in range(13)]):
            with self.assertRaises(ValueError):
                Oracle(points)
        for value in (q(1,0), q(0), dict(num=True, den=1), dict(num='-1', den='2')):
            with self.assertRaises(ValueError):
                rational(value, True)


if __name__ == '__main__':
    unittest.main()
