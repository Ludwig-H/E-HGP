#!/usr/bin/env python3
"""Implantation independante (verificateur adverse) de FULL_k et des regles de pendaison.

Aucun import du rapport `axiomes` ni du depot : MEB exactes (Fraction) par enumeration des supports, graphe Gamma_k
(sommets = k-parties au niveau beta, aretes = (k+1)-parties), arbre de Kruskal binaire (les plateaux exacts sont
traites par la coupe fermee : ancetre le plus haut de niveau <= a). Rayons en mpmath (60 chiffres), niveaux carres
en Fraction exacte.

Regles (rayon r, niveau a = r^2) :
  P(kappa, m) : e = max(r(t), max_F [ r(meet) - kappa (r(a_F) - r(t)) ])   (ancrage persistant, en rayon)
  Q(kappa, m) : e^2 = max(t, max_F [ meet - kappa (a_F - t) ])             (meme formule en niveau carre)
  core, cover (LCA des ex aequo), first (LCA des ex aequo sur le profil qualifie), closure (fermeture qualifiee).
"""
from fractions import Fraction
from itertools import combinations
import mpmath

mpmath.mp.dps = 60
TOL = mpmath.mpf(10) ** -40


def _sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def _dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def _gauss(A, b):
    n = len(A)
    M = [list(A[i]) + [b[i]] for i in range(n)]
    for c in range(n):
        piv = None
        for r in range(c, n):
            if M[r][c] != 0:
                piv = r
                break
        if piv is None:
            return None
        M[c], M[piv] = M[piv], M[c]
        inv = 1 / M[c][c]
        M[c] = [v * inv for v in M[c]]
        for r in range(n):
            if r != c and M[r][c] != 0:
                f = M[r][c]
                M[r] = [x - f * y for x, y in zip(M[r], M[c])]
    return [M[i][n] for i in range(n)]


def circumball(pts):
    """(centre, rayon carre) de la sphere circonscrite a centre dans l'enveloppe affine ; None si degenere."""
    p0 = pts[0]
    if len(pts) == 1:
        return tuple(Fraction(c) for c in p0), Fraction(0)
    V = [_sub(p, p0) for p in pts[1:]]
    A = [[Fraction(_dot(V[i], V[j])) for j in range(len(V))] for i in range(len(V))]
    b = [Fraction(_dot(v, v), 2) for v in V]
    lam = _gauss(A, b)
    if lam is None:
        return None
    c = tuple(Fraction(p0[d]) + sum(l * v[d] for l, v in zip(lam, V)) for d in range(3))
    return c, sum((c[d] - p0[d]) ** 2 for d in range(3))


class Cloud(object):
    def __init__(self, pts):
        self.pts = [tuple(int(c) for c in p) for p in pts]
        if len(set(self.pts)) != len(self.pts):
            raise ValueError('points non distincts')
        self.n = len(self.pts)
        self._meb = {}

    def meb2(self, S):
        S = tuple(sorted(S))
        if S in self._meb:
            return self._meb[S]
        P = [self.pts[i] for i in S]
        best = None
        for q in range(1, min(4, len(P)) + 1):
            for T in combinations(range(len(P)), q):
                cb = circumball([P[i] for i in T])
                if cb is None:
                    continue
                c, r2 = cb
                if best is not None and r2 >= best:
                    continue
                if all(sum((p[d] - c[d]) ** 2 for d in range(3)) <= r2 for p in P):
                    best = r2
        self._meb[S] = best
        return best

    def dk2(self, x, k):
        d = sorted(sum((a - b) ** 2 for a, b in zip(self.pts[x], p)) for p in self.pts)
        return Fraction(d[k - 1])

    def knn_part(self, x, k):
        d = sorted((sum((a - b) ** 2 for a, b in zip(self.pts[x], p)), y) for y, p in enumerate(self.pts))
        return tuple(sorted(y for _, y in d[:k]))


class Full(object):
    """FULL_k par Gamma_k ; arbre de Kruskal binaire avec niveaux et masques d'amas (sites couverts)."""

    def __init__(self, cloud, k):
        self.cloud, self.k, n = cloud, k, cloud.n
        self.parts = list(combinations(range(n), k))
        self.leaf = {F: i for i, F in enumerate(self.parts)}
        self.level, self.parent, self.mask = [], [], []
        for F in self.parts:
            self.level.append(cloud.meb2(F))
            self.parent.append(-1)
            m = 0
            for s in F:
                m |= 1 << s
            self.mask.append(m)
        edges = []
        if k < n:
            for G in combinations(range(n), k + 1):
                edges.append((cloud.meb2(G), G))
        edges.sort()
        uf = list(range(len(self.parts)))
        top = list(range(len(self.parts)))  # racine uf -> noeud de l'arbre

        def find(i):
            while uf[i] != i:
                uf[i] = uf[uf[i]]
                i = uf[i]
            return i
        for L, G in edges:
            subs = [tuple(G[:j] + G[j + 1:]) for j in range(k + 1)]
            r0 = find(self.leaf[subs[0]])
            for F in subs[1:]:
                r1 = find(self.leaf[F])
                if r1 == r0:
                    continue
                a, b = top[r0], top[r1]
                v = len(self.level)
                self.level.append(L)
                self.parent.append(-1)
                self.mask.append(self.mask[a] | self.mask[b])
                self.parent[a] = v
                self.parent[b] = v
                uf[r1] = r0
                top[r0] = v
        self.root = top[find(0)]
        self._chain = {}

    def chain(self, v):
        if v not in self._chain:
            out = [v]
            while self.parent[out[-1]] >= 0:
                out.append(self.parent[out[-1]])
            self._chain[v] = out
        return self._chain[v]

    def conn(self, F, G):
        """Niveau de rencontre (coupe fermee) des lignees de deux k-parties."""
        a, b = self.leaf[F], self.leaf[G]
        ca = self.chain(a)
        sa = set(ca)
        for w in self.chain(b):
            if w in sa:
                return max(self.level[w], self.level[a], self.level[b])
        raise RuntimeError('non connexe')

    def comp_at(self, F, a):
        """Noeud (ancetre le plus haut de niveau <= a) de la composante contenant F a la coupe fermee a."""
        v = self.leaf[F]
        if self.level[v] > a:
            return None
        best = v
        for w in self.chain(v)[1:]:
            if self.level[w] <= a:
                best = w
            else:
                break
        return best

    def amas_at(self, F, a):
        v = self.comp_at(F, a)
        return 0 if v is None else self.mask[v]

    def qual_level(self, F, m):
        """Premier niveau a >= beta(F) ou la composante de F couvre au moins m sites."""
        b = self.level[self.leaf[F]]
        cands = [b] + [self.level[w] for w in self.chain(self.leaf[F])[1:] if self.level[w] > b]
        for a in sorted(set(cands)):
            if bin(self.amas_at(F, a)).count('1') >= m:
                return a
        return None

    def entries(self, x, m):
        out = []
        for F in self.parts:
            if x in F:
                a = self.qual_level(F, m)
                if a is not None:
                    out.append((a, F))
        return out


def R(level):
    return mpmath.sqrt(mpmath.mpf(level.numerator) / level.denominator)


class Rule(object):
    """Resultat d'une regle ancree : par site, date (rayon mpf), date carree exacte si dispo, k-partie ancre."""

    def __init__(self, name, e, anchor, t, full, e2=None):
        self.name, self.e, self.anchor, self.t, self.full, self.e2 = name, e, anchor, t, full, e2

    def u(self, i, j):
        if i == j:
            return self.e[i]
        c = max(self.t[i], self.t[j], self.full.conn(self.anchor[i], self.anchor[j]))
        return max(self.e[i], self.e[j], R(c))

    def u2(self, i, j):
        """Version exacte en niveau (seulement si e2 est connu)."""
        if i == j:
            return self.e2[i]
        c = max(self.t[i], self.t[j], self.full.conn(self.anchor[i], self.anchor[j]))
        return max(self.e2[i], self.e2[j], c)

    def owner_at(self, i, r):
        """Noeud vivant du proprietaire a la coupe fermee de rayon r (None si pas entre)."""
        if self.e[i] > r + TOL:
            return None
        return self.full.comp_at(self.anchor[i], Fraction(0) + _level_floor(self.full, r))

    def blocks(self, r):
        n = len(self.e)
        act = [i for i in range(n) if self.e[i] <= r + TOL]
        out, seen = [], set()
        for i in act:
            if i in seen:
                continue
            b = frozenset(j for j in act if self.u(i, j) <= r + TOL)
            seen |= b
            out.append(b)
        return sorted(out, key=lambda b: sorted(b))


def _level_floor(full, r):
    """Plus grand niveau d'evenement <= r^2 (avec tolerance) : la coupe fermee de rayon r."""
    best = None
    for L in set(full.level):
        if R(L) <= r + TOL and (best is None or L > best):
            best = L
    return best


def rule_P(full, kappa, m, name=None):
    n = full.cloud.n
    e, anc, ts = [], [], []
    for x in range(n):
        ent = full.entries(x, m)
        t = min(a for a, _ in ent)
        Fs = min(F for a, F in ent if a == t)
        best = R(t)
        for a, F in ent:
            meet = max(t, a, full.conn(Fs, F))
            if kappa == 'inf':
                if a == t:
                    best = max(best, R(meet))
            else:
                best = max(best, R(meet) - kappa * (R(a) - R(t)))
        e.append(best)
        anc.append(Fs)
        ts.append(t)
    return Rule(name or 'P%s_m%d' % (kappa, m), e, anc, ts, full)


def rule_Q(full, kappa, m, name=None):
    n = full.cloud.n
    e, e2, anc, ts = [], [], [], []
    for x in range(n):
        ent = full.entries(x, m)
        t = min(a for a, _ in ent)
        Fs = min(F for a, F in ent if a == t)
        best = t
        for a, F in ent:
            meet = max(t, a, full.conn(Fs, F))
            best = max(best, meet - Fraction(kappa) * (a - t))
        e2.append(best)
        e.append(R(best))
        anc.append(Fs)
        ts.append(t)
    return Rule(name or 'Q%s_m%d' % (kappa, m), e, anc, ts, full, e2)


def rule_core(full):
    cl, k = full.cloud, full.k
    e, e2, anc, ts = [], [], [], []
    for x in range(cl.n):
        D = cl.dk2(x, k)
        F0 = cl.knn_part(x, k)
        e2.append(D)
        e.append(R(D))
        anc.append(F0)
        ts.append(full.level[full.leaf[F0]])
    return Rule('core', e, anc, ts, full, e2)


def rule_lca(full, m, name):
    """Premiere couverture (qualifiee si m > 1) avec LCA des ex aequo (cover pour m = 1, first sinon)."""
    e, e2, anc, ts = [], [], [], []
    for x in range(full.cloud.n):
        ent = full.entries(x, m)
        t = min(a for a, _ in ent)
        ties = sorted(F for a, F in ent if a == t)
        L = t
        for F in ties[1:]:
            L = max(L, full.conn(ties[0], F))
        e2.append(L)
        e.append(R(L))
        anc.append(ties[0])
        ts.append(t)
    return Rule(name, e, anc, ts, full, e2)


class Closure(object):
    """Fermeture qualifiee de l'auditeur : w(i,j) = premier niveau ou une meme composante qualifiee couvre i et j,
    puis minimax (liaison simple) ; e_i = premiere hyperarete qualifiee contenant i."""

    def __init__(self, full, m):
        n = full.cloud.n
        self.n = n
        ent = [full.entries(x, m) for x in range(n)]
        self.e2 = [min(a for a, _ in ent[x]) for x in range(n)]
        w = [[None] * n for _ in range(n)]
        for i in range(n):
            w[i][i] = self.e2[i]
            for j in range(i + 1, n):
                best = None
                for a, F in ent[i]:
                    for b, G in ent[j]:
                        c = max(a, b, full.conn(F, G))
                        if best is None or c < best:
                            best = c
                w[i][j] = w[j][i] = best
        u = [row[:] for row in w]
        for kk in range(n):
            for i in range(n):
                for j in range(n):
                    if i != j:
                        c = max(u[i][kk], u[kk][j]) if (i != kk and j != kk) else u[i][j]
                        if c < u[i][j]:
                            u[i][j] = c
        self.uu2 = u
        self.e = [R(v) for v in self.e2]

    def u(self, i, j):
        return R(self.uu2[i][j])

    def blocks(self, r):
        act = [i for i in range(self.n) if self.e[i] <= r + TOL]
        out, seen = [], set()
        for i in act:
            if i in seen:
                continue
            b = frozenset(j for j in act if self.u(i, j) <= r + TOL)
            seen |= b
            out.append(b)
        return sorted(out, key=lambda b: sorted(b))


def fmt(x, nd=4):
    return float(mpmath.nstr(x, 20)) if not isinstance(x, Fraction) else float(x)
