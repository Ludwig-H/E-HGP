#!/usr/bin/env python3
"""Oracle de la pendaison FULL -> points au niveau de la definition (etage A de reference/hgp11_ref).

Deux routes sans code commun pour les regles :
  - route oracle : regles calculees par force brute sur les coupes fermees de Gamma_k (masques de couverture des
    noeuds vivants a chaque niveau d'evenement), arbre et LCA relus sur les seuls enfants publies ;
  - route banc : bench/points_hierarchy.py applique a un Order construit depuis ces memes coupes (incidences =
    premiers instants de couverture par noeud), ou a l'export natif MHGP11PH sur G4.
Les deux rendent par site une date exacte et un proprietaire ; on compare les ultrametriques exactes u(i, j),
independantes de la numerotation des noeuds.
"""
from fractions import Fraction
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'reference'))
sys.path.insert(0, HERE)

from hgp11_ref import Definition  # noqa: E402
import points_hierarchy as ph  # noqa: E402

RULES = ('core', 'cover', 'first', 'margin1', 'margin')


def parents_of(nodes):
    parent = [-1] * len(nodes)
    for v, node in enumerate(nodes):
        for c in node.children:
            parent[c] = v
    return parent


def popcount(x):
    return bin(x).count('1')


class Tree(object):
    """Arbre de l'etage A relu sur les enfants : ancetre vivant (coupe fermee) et LCA par remontee."""

    def __init__(self, nodes):
        self.nodes, self.parent = nodes, parents_of(nodes)

    def chain(self, v):
        out = [v]
        while self.parent[out[-1]] >= 0:
            out.append(self.parent[out[-1]])
        return out

    def alive_ancestor(self, v, level):
        for w in self.chain(v):
            p = self.parent[w]
            if p < 0 or self.nodes[p].level > level:
                return w
        raise AssertionError('chaine')

    def lca(self, a, b):
        seen = set(self.chain(a))
        for w in self.chain(b):
            if w in seen:
                return w
        raise AssertionError('foret_non_connexe')

    def meet(self, a, la, b, lb):
        """Niveau de rencontre des remontees de (a, la) et (b, lb)."""
        w = self.lca(a, b)
        if w in (a, b):
            return max(la, lb)
        return max(la, lb, self.nodes[w].level)


def reference_rules(res, n, m):
    """Regles par force brute sur les coupes : {regle: [(date, proprietaire)]}."""
    tree = Tree(res.nodes)
    points = [[] for _ in range(n)]  # (niveau, noeud) : noeud vivant qui couvre le site a ce niveau ferme
    qualified = [[] for _ in range(n)]
    for cut in res.cuts:
        for v, coverage, _core in cut.closed:
            for i in range(n):
                if coverage >> i & 1:
                    points[i].append((cut.level, v))
                    if popcount(coverage) >= m:
                        qualified[i].append((cut.level, v))
    out = {rule: [] for rule in RULES}
    for i in range(n):
        out['core'].append((res.core[i].level, res.core[i].nodes))
        for rule, pts in (('cover', points[i]), ('first', qualified[i])):
            t = min(level for level, _ in pts)
            ties = sorted(set(v for level, v in pts if level == t))
            w = ties[0]
            for v in ties[1:]:
                w = tree.lca(w, v)
            out[rule].append((max(t, res.nodes[w].level), w))
        for rule, pts in (('margin1', points[i]), ('margin', qualified[i])):
            t = min(level for level, _ in pts)
            v1 = next(v for level, v in pts if level == t)
            delay = max(tree.meet(v1, t, v, level) - level for level, v in pts)
            e = t + delay
            out[rule].append((e, tree.alive_ancestor(v1, e)))
    return out, tree


def reference_ultrametric(entries, tree):
    n = len(entries)
    u = [[None] * n for _ in range(n)]
    for i in range(n):
        ei, oi = entries[i]
        u[i][i] = ei
        for j in range(i + 1, n):
            ej, oj = entries[j]
            u[i][j] = u[j][i] = tree.meet(oi, ei, oj, ej)
    return u


def order_from_definition(res, n, k):
    """Order du banc construit sur les coupes de l'etage A (incidences = premiers instants de couverture)."""
    values = sorted(set([Fraction(0)] + [node.level for node in res.nodes] + [cut.level for cut in res.cuts]))
    rank_of = {value: r for r, value in enumerate(values)}
    levels = ph.Levels.from_fractions(values)
    parent = np.array(parents_of(res.nodes), dtype=np.int64)
    rank = np.array([rank_of[node.level] for node in res.nodes], dtype=np.int64)
    seen = {}
    incidences = []
    for cut in res.cuts:
        for v, coverage, _core in cut.closed:
            fresh = coverage & ~seen.get(v, 0)
            seen[v] = coverage
            for i in range(n):
                if fresh >> i & 1:
                    incidences.append((i, rank_of[cut.level], v))
    incidences.sort()
    sites = np.array([x[0] for x in incidences], dtype=np.int64)
    offsets = np.searchsorted(sites, np.arange(n + 1))
    core_d = []
    for e in res.core:
        if e.level.denominator != 1:
            raise AssertionError('D_k non entier')
        core_d.append(int(e.level))
    births = sum(1 for node in res.nodes if not node.children)
    root = int(np.flatnonzero(parent < 0)[0])
    return ph.Order(k, parent, rank, births, root, np.array([e.nodes for e in res.core], dtype=np.int64),
                    np.array(core_d, dtype=np.int64), offsets.astype(np.int64),
                    np.array([x[1] for x in incidences], dtype=np.int64),
                    np.array([x[2] for x in incidences], dtype=np.int64), levels)


def bench_hangings(order, m):
    return {'core': ph.hang_core(order), 'cover': ph.hang_first(order, 1, 'cover'),
            'first': ph.hang_first(order, m, 'first'), 'margin1': ph.hang_margin(order, 1, 'margin1'),
            'margin': ph.hang_margin(order, m, 'margin')}


def blocks_at(u, level):
    """Blocs actifs (entres) a la coupe fermee level, comme ensembles de sites."""
    n = len(u)
    active = [i for i in range(n) if u[i][i] <= level]
    out, seen = [], set()
    for i in active:
        if i in seen:
            continue
        block = frozenset(j for j in active if u[i][j] <= level)
        seen |= block
        out.append(block)
    return sorted(out, key=lambda b: sorted(b))


def reference_radius_rules(res, n, m, precision=120):
    """Regles a marge en RAYON par force brute sur les coupes, arithmetique decimal independante du banc.

    Rend {'margin_r1': [(e, proprietaire)], 'margin_r': [...]} ; e en Decimal (rayon)."""
    import decimal
    ctx = decimal.Context(prec=precision)

    def rad(level):
        return ctx.sqrt(ctx.divide(decimal.Decimal(level.numerator), decimal.Decimal(level.denominator)))

    tree = Tree(res.nodes)
    points = [[] for _ in range(n)]
    qualified = [[] for _ in range(n)]
    for cut in res.cuts:
        for v, coverage, _core in cut.closed:
            for i in range(n):
                if coverage >> i & 1:
                    points[i].append((cut.level, v))
                    if popcount(coverage) >= m:
                        qualified[i].append((cut.level, v))
    out = {'margin_r1': [], 'margin_r': []}
    for i in range(n):
        for rule, pts in (('margin_r1', points[i]), ('margin_r', qualified[i])):
            t = min(level for level, _ in pts)
            v1 = next(v for level, v in pts if level == t)
            delay = max(rad(tree.meet(v1, t, v, level)) - rad(level) for level, v in pts)
            e = rad(t) + max(delay, decimal.Decimal(0))
            owner = v1
            for w in tree.chain(v1):  # ancetre le plus haut de rayon de naissance <= e
                if rad(res.nodes[w].level) <= e:
                    owner = w
                else:
                    break
            out[rule].append((e, owner))
    return out, tree


def reference_radius_ultrametric(entries, tree):
    import decimal
    ctx = decimal.Context(prec=120)
    n = len(entries)
    u = [[None] * n for _ in range(n)]
    for i in range(n):
        ei, oi = entries[i]
        u[i][i] = ei
        for j in range(i + 1, n):
            ej, oj = entries[j]
            w = tree.lca(oi, oj)
            value = max(ei, ej)
            if w not in (oi, oj):
                lv = tree.nodes[w].level
                value = max(value, ctx.sqrt(ctx.divide(decimal.Decimal(lv.numerator), decimal.Decimal(lv.denominator))))
            u[i][j] = u[j][i] = value
    return u
