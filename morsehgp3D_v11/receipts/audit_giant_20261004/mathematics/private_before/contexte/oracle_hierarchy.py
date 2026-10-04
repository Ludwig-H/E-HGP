#!/usr/bin/env python3
"""Hiérarchie de points v11 H^r_{k+1} sur un PETIT nuage, depuis l'oracle exact de la définition (n <= 12-14).

    import sys; sys.path.insert(0, '/workspaces/E-HGP/build/v11-points-select/contexte')
    import oracle_hierarchy as oh
    h = oh.hierarchy([(0, 0, 0), (3, 0, 0), ...], k=2)      # m = k + 1 par défaut (m = 1 à k = 1)

Champs rendus (sites dans l'ordre d'entrée, coordonnées entières positives ou nulles) :
    h.entry[i]          rayon d'entrée du site i (float) ; h.entry_exact[i] = RValue exacte sqrt(t)+sqrt(m)-sqrt(q)
    h.owner[i]          nœud propriétaire (indice de nœud FULL_k)
    h.parent[v]         parent du nœud v (-1 à la racine) ; h.birth[v] = rayon de naissance (float),
                        h.birth_level[v] = niveau carré exact (Fraction)
    h.core[i]           temps de cœur d_k(x_i) en rayon (float)
    h.covered(v)        sites couverts par le nœud v à sa naissance (amas discret de la définition 8)
    h.blocks(level)     blocs exacts à la coupe fermée de niveau CARRÉ `level` (Fraction) : liste triée de listes
    h.u                 ultrametrique de réunion en rayon (floats ; diagonale = entrée)
    h.order, h.hang     objets du banc (bench/points_hierarchy.Order, bench/points_radius.RadiusHanging)

Toutes les décisions (date, propriétaire, blocs) sont exactes ; les floats ne servent qu'à la lecture.
"""
from fractions import Fraction
import os
import sys

import numpy as np

V11 = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11'
sys.path.insert(0, os.path.join(V11, 'reference'))
sys.path.insert(0, os.path.join(V11, 'bench'))

from hgp11_ref import Definition  # noqa: E402
import points_reference as pr  # noqa: E402
import points_radius as prad  # noqa: E402


class Hierarchy(object):
    pass


def hierarchy(points, k, m=None):
    points = [tuple(int(c) for c in p) for p in points]
    n = len(points)
    if m is None:
        m = 1 if k == 1 else k + 1
    res = Definition(points).order(k)
    order = pr.order_from_definition(res, n, k)
    hang = prad.hang_margin_radius(order, m)
    h = Hierarchy()
    h.points, h.k, h.m, h.n, h.res, h.order, h.hang = points, k, m, n, res, order, hang
    h.entry_exact = list(hang.values)
    h.entry = [v.approx() for v in hang.values]
    h.owner = [int(x) for x in hang.owner]
    h.parent = [int(x) for x in order.parent]
    h.birth_level = [Fraction(*order.levels.exact(int(r))) for r in order.rank]
    h.birth = [float(np.sqrt(float(x))) for x in h.birth_level]
    h.core = [float(np.sqrt(float(d))) for d in order.core_d]

    def covered(v):
        level = res.nodes[v].level
        for cut in res.cuts:
            if cut.level == level:
                for w, coverage, _core in cut.closed:
                    if w == v:
                        return [i for i in range(n) if coverage >> i & 1]
        return []

    def blocks(level):
        target = prad.RValue(Fraction(level))
        groups = {}
        for i in range(n):
            if hang.values[i].cmp(target) <= 0:
                top = prad.ancestor_at_radius(order, h.owner[i], target)
                groups.setdefault(top, []).append(i)
        return sorted(sorted(b) for b in groups.values())

    h.covered, h.blocks = covered, blocks
    h.u = prad.ultrametric_radius(hang)
    return h


if __name__ == '__main__':
    # Contrôle : deux triangles équilatéraux exacts de la thèse (§ 6.1), k = 2 : ABC | DEF au rayon 1.
    tri = [tuple(c + 2 for c in p) for p in ((-1, -1, 0), (-1, 0, -1), (0, 0, 0), (1, 1, 0), (2, 2, 0), (2, 1, 1))]
    h = hierarchy(tri, 2)
    print('entrées', [round(x, 4) for x in h.entry])
    print('blocs au rayon 1', h.blocks(Fraction(1)))
    print('nœuds', len(h.parent), 'naissances', [round(b, 4) for b in h.birth])
