#!/usr/bin/env python3
"""Oracle exact independant de l'audit L03 (v11-audit) : tour FULL_K par Gamma_K exhaustif.

Ecrit depuis les definitions seules, sans import du depot ni des dossiers prives :
  - L_K(r) = { y : au moins K sites dans la boule fermee B(y, r) } ;
  - L_K(r) = reunion des regions temoins T_r(F) = intersection des B(f, r), f dans F, F parcourant les K-parties ;
    T_r(F) est convexe, non vide ssi rayon(MEB(F)) <= r ; T_r(F) et T_r(F') se coupent ssi rayon(MEB(F u F')) <= r ;
  - donc (nerf de convexes) les composantes de L_K(r) sont celles du graphe sur les K-parties actives, deux K-parties
    etant reliees quand leur reunion a un rayon <= r ; en passant d'une K-partie a l'autre par echanges d'un site,
    les aretes portees par les (K+1)-parties suffisent (Gamma_K).
Tout est exact (Fraction). Bornes : n <= ~14, K <= 5 (oracle borne, jamais un chemin produit).
Niveaux : rayons CARRES. Coupes fermees.
"""
from fractions import Fraction as Fr
from itertools import combinations
import math


def _sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def _dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def d2(a, b):
    v = _sub(a, b)
    return _dot(v, v)


def circum(S):
    """Sphere circonscrite minimale (dans l'enveloppe affine) de points affinement independants ; None sinon."""
    p0 = S[0]
    if len(S) == 1:
        return tuple(Fr(c) for c in p0), Fr(0)
    D = [_sub(p, p0) for p in S[1:]]
    m = len(D)
    A = [[Fr(2 * _dot(D[i], D[j])) for j in range(m)] + [Fr(_dot(D[i], D[i]))] for i in range(m)]
    for c in range(m):
        piv = next((i for i in range(c, m) if A[i][c] != 0), None)
        if piv is None:
            return None
        A[c], A[piv] = A[piv], A[c]
        for i in range(m):
            if i != c and A[i][c] != 0:
                f = A[i][c] / A[c][c]
                A[i] = [a - f * b for a, b in zip(A[i], A[c])]
    lam = [A[i][m] / A[i][i] for i in range(m)]
    cen = tuple(Fr(p0[t]) + sum(lam[j] * D[j][t] for j in range(m)) for t in range(len(p0)))
    return cen, d2(cen, p0)


def meb(P):
    """Plus petite boule fermee englobante : minimum des spheres circonscrites de <= 4 points qui contiennent tout."""
    best = None
    for q in range(1, min(4, len(P)) + 1):
        for S in combinations(P, q):
            cb = circum(S)
            if cb is None:
                continue
            cen, r2 = cb
            if all(d2(cen, p) <= r2 for p in P):
                if best is None or r2 < best[1]:
                    best = (cen, r2)
    return best


class Tour:
    """FULL_K par Gamma_K exhaustif."""

    def __init__(self, P, K):
        self.P = [tuple(int(c) for c in p) for p in P]
        self.n = len(self.P)
        self.K = K
        idx = range(self.n)
        self.V = {}
        self.cen = {}
        for F in combinations(idx, K):
            c, r2 = meb([self.P[i] for i in F])
            self.V[F] = r2
            self.cen[F] = c
        self.E = {}
        if K + 1 <= self.n:
            for S in combinations(idx, K + 1):
                self.E[S] = meb([self.P[i] for i in S])[1]
        self.levels = sorted(set(self.V.values()) | set(self.E.values()))
        self._cache = {}

    def comps(self, a):
        """Composantes de Gamma_K a la coupe fermee a : dict K-partie active -> representant (frozenset de K-parties)."""
        if a in self._cache:
            return self._cache[a]
        act = [F for F, b in self.V.items() if b <= a]
        par = {F: F for F in act}

        def find(x):
            while par[x] != x:
                par[x] = par[par[x]]
                x = par[x]
            return x

        for S, b in self.E.items():
            if b <= a:
                fs = [F for F in combinations(S, self.K)]
                r0 = find(fs[0])
                for F in fs[1:]:
                    r1 = find(F)
                    if r1 != r0:
                        par[r1] = r0
        groups = {}
        for F in act:
            groups.setdefault(find(F), []).append(F)
        out = {}
        for g in groups.values():
            fs = frozenset(g)
            for F in g:
                out[F] = fs
        self._cache[a] = out
        return out

    def amas_discrets(self, a):
        """Amas discrets X n delta_r(C) a la coupe fermee a : liste d'ensembles de sites (peuvent se recouvrir)."""
        cs = self.comps(a)
        res = []
        for comp in set(cs.values()):
            res.append(frozenset(i for F in comp for i in F))
        return sorted(res, key=lambda s: (min(s), len(s), sorted(s)))

    # ---- entrees de points
    def core_entry(self, x):
        ds = sorted((d2(self.P[x], self.P[j]), j) for j in range(self.n))
        F = tuple(sorted(j for _, j in ds[:self.K]))
        return Fr(ds[self.K - 1][0]), F

    def cover_entries(self, x):
        """alpha_K(x)^2 et TOUTES les K-parties contenant x qui l'atteignent (premieres couvertures a egalite)."""
        best = min(b for F, b in self.V.items() if x in F)
        return best, [F for F, b in self.V.items() if x in F and b == best]

    def cover_tie_components(self, x):
        """Composantes distinctes de Gamma_K(alpha^2) portant une premiere couverture de x (ambiguite si > 1)."""
        a, Fs = self.cover_entries(x)
        cs = self.comps(a)
        return a, sorted({cs[F] for F in Fs}, key=lambda c: sorted(c))

    def partition(self, a, entry):
        """Partition a la coupe fermee a pour une projection entry : x -> (niveau d'entree, K-partie temoin)."""
        cs = self.comps(a)
        blocks = {}
        single = []
        for x in range(self.n):
            e, F = entry[x]
            if e <= a:
                blocks.setdefault(cs[F], []).append(x)
            else:
                single.append(x)
        return sorted([tuple(sorted(b)) for b in blocks.values()] + [(x,) for x in single])

    def hauteurs(self, entry):
        """Hauteur de reunion u(x, y) (niveau = rayon carre) pour une projection : premiere coupe fermee commune."""
        levels = sorted(set(self.levels) | {entry[x][0] for x in range(self.n)})
        u = {}
        for a in levels:
            for b in self.partition(a, entry):
                for i in range(len(b)):
                    for j in range(i + 1, len(b)):
                        u.setdefault((b[i], b[j]), a)
        return u

    def suite(self, entry, mcs=1):
        """Suite des partitions (blocs d'au moins mcs points) aux coupes ou elle change."""
        levels = sorted(set(self.levels) | {entry[x][0] for x in range(self.n)})
        out = []
        last = None
        for a in levels:
            p = tuple(b for b in self.partition(a, entry) if len(b) >= mcs)
            if p != last:
                out.append((a, p))
                last = p
        return out


def core_projection(T):
    return {x: T.core_entry(x) for x in range(T.n)}


def cover_projection(T, choix='min'):
    """Premiere couverture ; aux egalites, choix 'min' = plus petite K-partie (lexicographique), 'max' = plus grande."""
    pr = {}
    for x in range(T.n):
        a, Fs = T.cover_entries(x)
        pr[x] = (a, min(Fs) if choix == 'min' else max(Fs))
    return pr


def hdbscan_partition(P, K, rho2):
    """Partition de l'atteignabilite mutuelle (point compte) dans la convention de la these : niveau rho = distance/2.
    rho2 = rho^2 ; seuil de distance carree 4 rho2. Actifs : D_K(x) <= 4 rho2. Exact (entiers)."""
    n = len(P)
    D = []
    for x in range(n):
        ds = sorted(d2(P[x], P[j]) for j in range(n))
        D.append(ds[K - 1])
    thr = 4 * rho2
    par = list(range(n))

    def find(x):
        while par[x] != x:
            par[x] = par[par[x]]
            x = par[x]
        return x

    act = [x for x in range(n) if D[x] <= thr]
    for i in act:
        for j in act:
            if i < j and max(D[i], D[j], d2(P[i], P[j])) <= thr:
                par[find(i)] = find(j)
    g = {}
    for x in act:
        g.setdefault(find(x), []).append(x)
    return sorted([tuple(sorted(b)) for b in g.values()] + [(x,) for x in range(n) if D[x] > thr])


def hdbscan_suite(P, K):
    n = len(P)
    D = []
    for x in range(n):
        ds = sorted(d2(P[x], P[j]) for j in range(n))
        D.append(ds[K - 1])
    lv = sorted({Fr(max(D[i], D[j], d2(P[i], P[j])), 4) for i in range(n) for j in range(i + 1, n)} | {Fr(d, 4) for d in D})
    out = []
    last = None
    for a in lv:
        p = tuple(b for b in hdbscan_partition(P, K, a) if len(b) >= 2)
        if p != last:
            out.append((a, p))
            last = p
    return out


def r(a):
    return math.sqrt(float(a))


def fmt_suite(S, names=None):
    lines = []
    for a, p in S:
        if names:
            ps = ' | '.join(''.join(names[i] for i in b) for b in p)
        else:
            ps = ' | '.join(str(b) for b in p)
        lines.append('  r = %10.4f  (r^2 = %s) : %s' % (r(a), a, ps if ps else '(aucun bloc)'))
    return '\n'.join(lines)
