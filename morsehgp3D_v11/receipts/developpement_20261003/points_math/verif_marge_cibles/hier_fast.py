#!/usr/bin/env python3
"""Hier plus rapide pour le catalogue : niveaux carres en Fraction (comparaison exacte a r^2 quand r = sqrt(q)),
rayons en R avec filtre flottant et repli exact ; partitions mises en cache. Meme semantique que indep_full.Hier
(coupe fermee : i, j actifs ensemble ssi u(i, j) <= r ; site non entre = singleton)."""
import sys
sys.dont_write_bytecode = True
from fractions import Fraction as Fr
from indep_full import R


class HierFast(object):
    def __init__(self, U, kind):
        self.n, self.kind, self.U = len(U), kind, U
        n = self.n
        if kind == 'sq':
            vals = sorted(set(U[i][j] for i in range(n) for j in range(i, n)))
            self._chg = [R.rac(v) for v in vals]
            self.F = None
        else:
            self.F = [[float(U[i][j]) for j in range(n)] for i in range(n)]
            vals = []
            for i in range(n):
                for j in range(i, n):
                    v = U[i][j]
                    if not any(abs(float(v) - float(w)) < 1e-6 * (1 + abs(float(w))) and v.cmp(w) == 0 for w in vals):
                        vals.append(v)
            out = []
            for v in sorted(vals, key=float):
                k = len(out)
                while k > 0 and out[k - 1].cmp(v) > 0:
                    k -= 1
                out.insert(k, v)
            self._chg = out
        self._cache = {}

    def rayons_changement(self):
        return self._chg

    def _le(self, r):
        if self.kind == 'sq':
            q = r.carre_si_racine()
            if q is not None:
                return lambda i, j: self.U[i][j] <= q
            return lambda i, j: R.rac(self.U[i][j]).cmp(r) <= 0
        fr = float(r)
        tol = 1e-9 * (1 + abs(fr))

        def le(i, j):
            f = self.F[i][j]
            if f < fr - tol:
                return True
            if f > fr + tol:
                return False
            return self.U[i][j].cmp(r) <= 0
        return le

    def partition(self, r):
        key = r.texte()
        if key in self._cache:
            return self._cache[key]
        n = self.n
        le = self._le(r)
        act = [le(i, i) for i in range(n)]
        lab = list(range(n))
        for i in range(n):
            if not act[i]:
                continue
            for j in range(i + 1, n):
                if act[j] and lab[i] != lab[j] and le(i, j):
                    a, b = lab[i], lab[j]
                    lab = [a if z == b else z for z in lab]
        g = {}
        for i in range(n):
            g.setdefault(('c', lab[i]) if act[i] else ('s', i), []).append(i)
        p = tuple(sorted(tuple(sorted(v)) for v in g.values()))
        self._cache[key] = p
        return p
