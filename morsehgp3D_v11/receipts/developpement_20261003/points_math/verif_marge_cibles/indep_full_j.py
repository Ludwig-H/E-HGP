#!/usr/bin/env python3
"""Variante rapide de indep_full.Full : aretes de Johnson (unions de k+1 sites) au lieu de toutes les paires de
K-parties. Meme nerf : si MEB(F u G) <= r, les K-parties intermediaires (echange d'un site a la fois) restent dans la
meme boule, donc F et G sont relies par des aretes de Johnson de niveau <= r^2 ; la reciproque est immediate.
Meme couverture : MEB(F u {x}) <= r. Sert au catalogue n <= 16 ; recoupe contre indep_full.Full (test_j)."""
import sys
sys.dont_write_bytecode = True
from itertools import combinations
import indep_full as I


class FullJ(I.Full):
    def __init__(self, P, K):
        self.P, self.K, self.n = [tuple(p) for p in P], K, len(P)
        I.exiger(len(set(self.P)) == self.n, 'sites non distincts')
        self._meb = {}
        n = self.n
        self.Ks = list(combinations(range(n), K))
        index = {F: i for i, F in enumerate(self.Ks)}
        nk = len(self.Ks)
        self.b = [self.meb(F) for F in self.Ks]
        at = {}
        for f in range(nk):
            at.setdefault(self.b[f], ([], []))[0].append(f)
        if K < n:
            for U in combinations(range(n), K + 1):
                subs = [index[U[:j] + U[j + 1:]] for j in range(K + 1)]
                at.setdefault(self.meb(U), ([], []))[1].append(subs)
        uf = {}

        def find(x):
            r = x
            while uf[r] != r:
                r = uf[r]
            while uf[x] != r:
                uf[x], x = r, uf[x]
            return r
        node_of = {}
        members = {}
        self.first_node = [None] * nk
        self.h, self.children = [], []
        for L in sorted(at):
            news, edges = at[L]
            dirty = {}
            for f in news:
                uf[f] = f
                node_of[f] = None
                members[f] = [f]
                dirty[f] = set()
            for subs in edges:
                r0 = find(subs[0])
                for g in subs[1:]:
                    r1 = find(g)
                    if r1 == r0:
                        continue
                    o0 = dirty.pop(r0) if r0 in dirty else set([node_of[r0]])
                    o1 = dirty.pop(r1) if r1 in dirty else set([node_of[r1]])
                    if len(members[r0]) < len(members[r1]):
                        r0, r1 = r1, r0
                    uf[r1] = r0
                    members[r0].extend(members.pop(r1))
                    node_of.pop(r1, None)
                    dirty[r0] = o0 | o1
            for r, olds in dirty.items():
                olds.discard(None)
                if not olds:
                    nd = len(self.h)
                    self.h.append(L)
                    self.children.append([])
                elif len(olds) == 1:
                    nd = next(iter(olds))
                else:
                    nd = len(self.h)
                    self.h.append(L)
                    self.children.append(sorted(olds))
                node_of[r] = nd
            for f in news:
                self.first_node[f] = node_of[find(f)]
        nn = len(self.h)
        self.parent = [-1] * nn
        for v in range(nn):
            for c in self.children[v]:
                self.parent[c] = v
        roots = [v for v in range(nn) if self.parent[v] < 0]
        I.exiger(len(roots) == 1, 'foret non connexe')
        self.root = roots[0]
        self.death = [self.h[self.parent[v]] if self.parent[v] >= 0 else None for v in range(nn)]
        self.sub = [[] for _ in range(nn)]
        for f in range(nk):
            v = self.first_node[f]
            while v >= 0:
                self.sub[v].append(f)
                v = self.parent[v]
        self.cov = [[None] * nn for _ in range(n)]
        for x in range(n):
            Mx = [self.b[f] if x in self.Ks[f] else self.meb(tuple(sorted(self.Ks[f] + (x,)))) for f in range(nk)]
            for v in range(nn):
                if not self.sub[v]:
                    continue
                c = max(self.h[v], min(Mx[f] for f in self.sub[v]))
                if self.death[v] is None or c < self.death[v]:
                    self.cov[x][v] = c
        self._qual = {}
