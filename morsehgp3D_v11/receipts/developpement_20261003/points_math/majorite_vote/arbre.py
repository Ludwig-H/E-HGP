#!/usr/bin/env python3
"""Arbre abstrait de FULL_k et relation de couverture, lu sur l'oracle v11 (hgp11_ref.Definition, etage A) ou sur
le Gamma_K de la v10 (vfull.full_gamma). Lecture seule des deux depots ; aucune ecriture hors de ce repertoire.

Interface commune (classe Tree) :
  birth[v] (Fraction), parent[v] (-1 a la racine), children[v], death[v] (None a la racine),
  cov[x] = {v : c_x(v)} premier niveau (coupe fermee) ou v couvre x pendant sa vie ; clos vers le haut.
"""
from fractions import Fraction
import os
import sys

sys.dont_write_bytecode = True
V11 = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11'
for p in (os.path.join(V11, 'reference'), os.path.join(V11, 'bench')):
    if p not in sys.path:
        sys.path.insert(0, p)
V10 = '/workspaces/E-HGP/build/v10-verrou-points/juge_final/verif_echelle_relative'


class TreeError(RuntimeError):
    pass


def need(c, m):
    if not c:
        raise TreeError(m)


class Tree(object):
    def __init__(self, birth, parent, cov, n):
        self.birth = list(birth)
        self.parent = list(parent)
        self.children = [[] for _ in self.birth]
        for v, p in enumerate(self.parent):
            if p >= 0:
                self.children[p].append(v)
        self.death = [None if p < 0 else self.birth[p] for p in self.parent]
        roots = [v for v, p in enumerate(self.parent) if p < 0]
        need(len(roots) == 1, 'racines %d' % len(roots))
        self.root = roots[0]
        self.cov = cov
        self.n = n
        for v, p in enumerate(self.parent):
            if p >= 0:
                need(self.birth[p] > self.birth[v], 'parent pas plus haut')
        for x in range(n):
            need(cov[x], 'site jamais couvert')
            for v, c in cov[x].items():
                need(self.birth[v] <= c and (self.death[v] is None or c < self.death[v]), 'c_x hors vie')
                p = self.parent[v]
                if p >= 0:
                    need(cov[x].get(p) == self.birth[p], 'couverture non close vers le haut')

    def __len__(self):
        return len(self.birth)

    def chain(self, v):
        out = []
        while v >= 0:
            out.append(v)
            v = self.parent[v]
        return out

    def anc(self, v, s):
        """Ancetre de v vivant au niveau rationnel s (coupe fermee)."""
        need(self.birth[v] <= s, 'noeud non ne')
        while self.parent[v] >= 0 and self.birth[self.parent[v]] <= s:
            v = self.parent[v]
        return v

    def anc_surd(self, v, t):
        """Ancetre de v vivant au rayon t (Surd) : niveau t^2, comparaisons sqrt(birth) <= t exactes."""
        from surd import Surd
        need(Surd.sqrt(self.birth[v]).cmp(t) <= 0, 'noeud non ne au rayon')
        while self.parent[v] >= 0 and Surd.sqrt(self.birth[self.parent[v]]).cmp(t) <= 0:
            v = self.parent[v]
        return v

    def lca(self, a, b):
        seen = set(self.chain(a))
        for w in self.chain(b):
            if w in seen:
                return w
        raise TreeError('lca')

    def meet(self, a, la, b, lb):
        """Niveau de rencontre des remontees de (a, la) et (b, lb) (comme points_reference.Tree.meet)."""
        w = self.lca(a, b)
        if w in (a, b):
            return max(la, lb)
        return max(la, lb, self.birth[w])


def tree_from_definition(res, n):
    """Arbre de l'etage A v11 : naissances, parents, et c_x(v) lus sur les coupes fermees."""
    parent = [-1] * len(res.nodes)
    for v, node in enumerate(res.nodes):
        for c in node.children:
            parent[c] = v
    birth = [node.level for node in res.nodes]
    cov = [dict() for _ in range(n)]
    for cut in res.cuts:
        for v, coverage, _core in cut.closed:
            for x in range(n):
                if coverage >> x & 1 and v not in cov[x]:
                    cov[x][v] = cut.level
    return Tree(birth, parent, cov, n)


def tree_from_vfull(T):
    return Tree(T.birth, T.parent, [dict(c) for c in T.cov], T.n)


def definition(points):
    from hgp11_ref import Definition
    return Definition([tuple(int(c) for c in p) for p in points])


def v11_tree(points, k):
    d = definition(points)
    res = d.order(k)
    return d, res, tree_from_definition(res, len(points))


def v10_tree(points, k):
    if V10 not in sys.path:
        sys.path.insert(0, V10)
    import vfull
    T, info = vfull.full_gamma([tuple(int(c) for c in p) for p in points], k)
    return T, info, tree_from_vfull(T)


def signature(tree):
    """Signature structurelle de l'arbre (independante de la numerotation) : par noeud, (niveau, enfants) recursifs,
    et par site l'ensemble des (signature de noeud, c_x)."""
    memo = {}

    def sig(v):
        if v not in memo:
            memo[v] = (tree.birth[v], tuple(sorted(sig(c) for c in tree.children[v])))
        return memo[v]
    nodes = sorted(sig(v) for v in range(len(tree)))
    sites = [sorted((sig(v), c) for v, c in tree.cov[x].items()) for x in range(tree.n)]
    return nodes, sites
