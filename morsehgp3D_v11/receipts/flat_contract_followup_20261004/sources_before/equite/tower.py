#!/usr/bin/env python3
"""Dendrogramme N-aire exact de H^r_{k+1} (v11) sur un petit nuage, depuis l'oracle de la definition
(contexte/oracle_hierarchy.py, n <= 12-14) ou depuis l'etage constructif B (quelques dizaines de sites).

u(i, j) = max(e_i, e_j, naissance de LCA(o_i, o_j)) si le LCA n'est ni o_i ni o_j, sinon max(e_i, e_j) ; toutes
les comparaisons sont exactes (RValue.cmp : sommes de radicaux, classes de carres, refus explicite au-dela du
budget). Les plateaux (egalites exactes) forment un seul noeud N-aire. La diagonale e_i n'entre pas dans le
dendrogramme (lemme D du rapport : sans effet pour mcs >= 2) ; elle est rendue a part.
"""
from fractions import Fraction
import os
import sys

import numpy as np

CONTEXTE = '/workspaces/E-HGP/build/v11-points-select/contexte'
V11 = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11'
sys.path.insert(0, CONTEXTE)
sys.path.insert(0, os.path.join(V11, 'reference'))
sys.path.insert(0, os.path.join(V11, 'bench'))

import oracle_hierarchy as oh  # noqa: E402
import points_radius as prad  # noqa: E402
import points_reference as pr  # noqa: E402

import nary_head as nh  # noqa: E402


def rcmp(a, b):
    return a.cmp(b)


def rmax(a, b):
    return a if rcmp(a, b) >= 0 else b


def hierarchy_stage_b(points, k, kmax=None):
    """Meme objet que oracle_hierarchy.hierarchy, par l'etage constructif B (n jusqu'a quelques dizaines)."""
    from hgp11_ref import Reference
    points = [tuple(int(c) for c in p) for p in points]
    n = len(points)
    m = 1 if k == 1 else k + 1
    ref = Reference(points, kmax if kmax is not None else k + 1)
    res = ref.order(k)
    order = pr.order_from_definition(res, n, k)
    hang = prad.hang_margin_radius(order, m)
    h = oh.Hierarchy()
    h.points, h.k, h.m, h.n, h.res, h.order, h.hang = points, k, m, n, res, order, hang
    h.entry_exact = list(hang.values)
    h.entry = [v.approx() for v in hang.values]
    h.owner = [int(x) for x in hang.owner]
    h.parent = [int(x) for x in order.parent]
    h.birth_level = [Fraction(*order.levels.exact(int(r))) for r in order.rank]
    h.birth = [float(np.sqrt(float(x))) for x in h.birth_level]
    h.core = [float(np.sqrt(float(d))) for d in order.core_d]

    def blocks(level):  # meme code que contexte/oracle_hierarchy.py
        target = prad.RValue(Fraction(level))
        groups = {}
        for i in range(n):
            if hang.values[i].cmp(target) <= 0:
                top = prad.ancestor_at_radius(order, h.owner[i], target)
                groups.setdefault(top, []).append(i)
        return sorted(sorted(b) for b in groups.values())
    h.blocks = blocks
    return h


def hierarchy(points, k, stage='auto'):
    if stage == 'A' or (stage == 'auto' and len(points) <= 12):
        return oh.hierarchy(points, k)
    return hierarchy_stage_b(points, k)


def exact_pairs(h):
    """Paires (i, j) -> RValue exacte de u(i, j)."""
    n = h.n
    e = h.entry_exact
    owner = np.array(h.owner, dtype=np.int64)
    births = [prad.RValue(Fraction(b)) for b in h.birth_level]
    pairs = {}
    for i in range(n):
        for j in range(i + 1, n):
            a, b = int(owner[i]), int(owner[j])
            w = int(h.order.lca(np.array([a]), np.array([b]))[0])
            val = rmax(e[i], e[j])
            if w not in (a, b):
                val = rmax(val, births[w])
            pairs[(i, j)] = val
    return pairs


def dendrogram(h):
    return nh.from_ultrametric(h.n, exact_pairs(h), rcmp)


def check_blocks(h, d):
    """Controle par une route independante : a chaque niveau de naissance FULL, les blocs d'au moins deux sites de
    l'oracle (h.blocks : ancetre vivant du proprietaire, coupe fermee, diagonale comprise) sont (1) les composantes de
    u <= niveau calculees ici et (2) les noeuds du dendrogramme N-aire vivants a ce niveau (lemme D).
    Rend le nombre de niveaux controles."""
    pairs = exact_pairs(h)
    levels = sorted(set(Fraction(b) for b in h.birth_level))
    checked = 0
    for level in levels:
        ob = sorted(b for b in h.blocks(level) if len(b) >= 2)
        target = prad.RValue(Fraction(level))
        parent = list(range(h.n))

        def find(x):
            while parent[x] != x:
                parent[x] = parent[parent[x]]
                x = parent[x]
            return x
        for (i, j), val in pairs.items():
            if val.cmp(target) <= 0:
                parent[find(i)] = find(j)
        groups = {}
        for i in range(h.n):
            groups.setdefault(find(i), []).append(i)
        mine = sorted(sorted(g) for g in groups.values() if len(g) >= 2)
        dend = []
        for v in range(d.n, d.nodes):
            if d.exact[v].cmp(target) <= 0 and (d.parent[v] < 0 or d.exact[d.parent[v]].cmp(target) > 0):
                dend.append(d.leaves(v))
        dend = sorted(dend)
        if not (mine == ob == dend):
            raise AssertionError('blocs differents au niveau %s : %s / %s / %s' % (level, mine, ob, dend))
        checked += 1
    return checked
