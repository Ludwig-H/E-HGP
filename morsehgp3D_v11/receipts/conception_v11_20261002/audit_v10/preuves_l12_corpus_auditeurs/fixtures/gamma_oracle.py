#!/usr/bin/env python3
"""Oracle exhaustif borne (audit L12) : Gamma_K exact en Fraction pour de tres petits nuages 3D.

Ecrit independamment des scripts des auditeurs et du moteur : sert a RECALCULER les attendus des fixtures
qu'ils ont publiees (coordonnees exactes -> niveaux exacts). Ne repose pas sur assert (tient sous python -O).
Niveaux = rayons carres (beta). Coupes fermees, plateaux atomiques.
"""
from fractions import Fraction as F
from itertools import combinations


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def solve(M, rhs):
    """Elimination de Gauss exacte ; None si singuliere."""
    n = len(M)
    A = [[F(v) for v in row] + [F(r)] for row, r in zip(M, rhs)]
    for c in range(n):
        piv = next((r for r in range(c, n) if A[r][c] != 0), None)
        if piv is None:
            return None
        A[c], A[piv] = A[piv], A[c]
        p = A[c][c]
        A[c] = [v / p for v in A[c]]
        for r in range(n):
            if r != c and A[r][c] != 0:
                f = A[r][c]
                A[r] = [a - f * b for a, b in zip(A[r], A[c])]
    return [A[r][n] for r in range(n)]


def circum(T):
    """Centre (dans l'enveloppe affine) et rayon carre de la sphere par les points de T ; None si dependants."""
    t0 = T[0]
    if len(T) == 1:
        return tuple(F(v) for v in t0), F(0), [F(1)]
    E = [sub(t, t0) for t in T[1:]]
    G = [[2 * dot(a, b) for b in E] for a in E]
    lam = solve(G, [dot(a, a) for a in E])
    if lam is None:
        return None
    c = tuple(F(t0[i]) + sum(l * e[i] for l, e in zip(lam, E)) for i in range(3))
    r2 = dot(sub(c, t0), sub(c, t0))
    bary = [1 - sum(lam)] + lam
    return c, r2, bary


def meb(S):
    """Plus petite boule englobante exacte d'un ensemble fini de points 3D : (centre, rayon carre)."""
    S = list(S)
    best = None
    for q in range(1, min(4, len(S)) + 1):
        for T in combinations(S, q):
            cb = circum(T)
            if cb is None:
                continue
            c, r2, _ = cb
            if best is not None and r2 >= best[1]:
                continue
            if all(dot(sub(p, c), sub(p, c)) <= r2 for p in S):
                best = (c, r2)
    return best


def census(X, c, r2):
    I = [x for x in X if dot(sub(x, c), sub(x, c)) < r2]
    U = [x for x in X if dot(sub(x, c), sub(x, c)) == r2]
    return I, U


def qmin(U, c):
    """Taille minimale d'un sous-ensemble de la coquille dont l'enveloppe convexe contient le centre."""
    for q in range(1, 5):
        for T in combinations(U, q):
            cb = circum(T)
            if cb is None:
                continue
            cc, _, bary = cb
            if cc == c and all(b >= 0 for b in bary):
                return q
    return None


class Gamma:
    def __init__(self, X, K):
        self.X = [tuple(p) for p in X]
        self.K = K
        n = len(self.X)
        self.verts = list(combinations(range(n), K))
        self.vlevel = {v: meb([self.X[i] for i in v])[1] for v in self.verts}
        self.edges = []
        if n > K:
            for g in combinations(range(n), K + 1):
                self.edges.append((meb([self.X[i] for i in g])[1], g))
        self.levels = sorted(set(self.vlevel.values()) | {l for l, _ in self.edges})

    def components(self, beta, closed=True):
        """Composantes (ensembles de K-parties) a la coupe beta (fermee par defaut, ouverte sinon)."""
        ok = (lambda l: l <= beta) if closed else (lambda l: l < beta)
        alive = [v for v in self.verts if ok(self.vlevel[v])]
        parent = {v: v for v in alive}

        def find(v):
            while parent[v] != v:
                parent[v] = parent[parent[v]]
                v = parent[v]
            return v

        for l, g in self.edges:
            if ok(l):
                sub_v = list(combinations(g, self.K))
                for w in sub_v[1:]:
                    parent[find(w)] = find(sub_v[0])
        comps = {}
        for v in alive:
            comps.setdefault(find(v), []).append(v)
        return sorted(comps.values())

    def covers(self, beta, closed=True):
        """Amas discrets (couvertures) : union des K-parties de chaque composante, en indices de sites."""
        return sorted(sorted(set(i for v in comp for i in v)) for comp in self.components(beta, closed))

    def history(self):
        """Liste (niveau, nombre de composantes fermees, naissances au niveau, couvertures fermees)."""
        out = []
        prev = 0
        for l in self.levels:
            born = [v for v in self.verts if self.vlevel[v] == l]
            comps = self.components(l, True)
            out.append((l, len(comps), len(born), self.covers(l, True)))
            prev = len(comps)
        return out

    def alpha(self, i):
        """Premiere date de couverture du site i : plus petit niveau d'une K-partie le contenant."""
        return min(self.vlevel[v] for v in self.verts if i in v)

    def dk2(self, i):
        """Distance K-NN carree (le site compte pour 1)."""
        d = sorted(dot(sub(self.X[i], x), sub(self.X[i], x)) for x in self.X)
        return d[self.K - 1]

    def merge_level_cover_sets(self, a, b):
        """Premier niveau ferme ou une meme composante couvre les sites a et b."""
        for l in self.levels:
            if any(a in c and b in c for c in self.covers(l, True)):
                return l
        return None


def line(*xs):
    return [(x, 0, 0) for x in xs]
