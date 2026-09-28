"""Portes de la reference : tour (catalogue + morceaux + descente) == oracle Gamma_k, coupes ouvertes et fermees,
hierarchie de points C n X, sur nuages generiques et degeneres (grilles)."""
import random
import sys
import unittest
from fractions import Fraction as Fr

import hgp10_ref as R

E5 = [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)]


def check_cloud(P, kmax, points=True):
    cat, tower = R.build_tower(P, kmax)
    for k, forest in tower.items():
        levels, closed, opened = R.gamma_cuts(P, k)
        tl = set(forest.levels())
        for a, cl, op in zip(levels, closed, opened):
            if forest.cut(a, True) != cl:
                return 'closed cut k=%d a=%s' % (k, a)
            if forest.cut(a, False) != op:
                return 'open cut k=%d a=%s' % (k, a)
        if points:
            for a in levels:
                for closed_ in (True, False):
                    if R.point_partition_tower(P, forest, a, cat, closed_) != R.point_partition_gamma(P, k, a, closed_):
                        return 'points k=%d a=%s closed=%s' % (k, a, closed_)
    return None


class RefGates(unittest.TestCase):
    def test_e5(self):
        self.assertIsNone(check_cloud(E5, 4))

    def test_generic_random(self):
        for s in range(25):
            rnd = random.Random(s)
            n = rnd.randint(4, 8)
            P = [tuple(rnd.randint(-500, 500) for _ in range(3)) for _ in range(n)]
            if len(set(P)) < n:
                continue
            err = check_cloud(P, 4)
            if err is not None:
                self.fail('seed %d: %s %s' % (s, err, P))

    def test_degenerate_grids(self):
        for s in range(40):
            rnd = random.Random(1000 + s)
            n = rnd.randint(4, 8)
            pts = set()
            while len(pts) < n:
                pts.add(tuple(rnd.randint(0, 2) for _ in range(3)))
            P = sorted(pts)
            rnd.shuffle(P)
            err = check_cloud(P, 4)
            if err is not None:
                self.fail('seed %d: %s %s' % (s, err, P))

    def test_square_and_cube(self):
        sq = [(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)]
        self.assertIsNone(check_cloud(sq, 3))
        cube = [(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, 2)]
        self.assertIsNone(check_cloud(cube, 3, points=False))


if __name__ == '__main__':
    unittest.main()
