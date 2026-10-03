#!/usr/bin/env python3
"""Axiomes FULL_k -> points : regles H_kappa (niveau carre ou rayon), temoins, ultrametriques et blocs.

Cadre : exploration_v11_hors_registre / cpu_reference / quantized_u18_input_only / not_claimed. GCP non utilise.
Lecture seule de l'oracle de la definition (morsehgp3D_v11/reference/hgp11_ref, bench/points_reference.py) ;
aucun bytecode ecrit (sys.dont_write_bytecode) ; aucune ecriture hors de build/v11-points-math/axiomes/.

Regles (toutes a k fixe, sur Definition(points).order(k)) :
  core          e = D_k(x), noeud du coeur (P1) ;
  cover         premiere couverture, LCA des ex aequo, date max(alpha, naissance du LCA) (P2/P4) ;
  H[kappa, m, echelle]
                t = premier niveau de R_i^(m) ; p = un point le plus bas ;
                e = sup_q [ m(p, q) - kappa (h(q) - t) ] ;  proprietaire = ancetre de p vivant a e ;
                echelle 'sq' : niveaux = rayons carres (regle du developpeur pour kappa = 1) ;
                echelle 'rad' : niveaux = rayons (racines exactes approchees en Decimal a 80 chiffres).
Le sup sur un rayon de noeud est atteint au premier niveau ou ce noeud couvre le site (m(p, q) - h(q) decroit le
long d'une remontee), donc on ne garde que ces premiers instants.
"""
from decimal import Decimal, getcontext
from fractions import Fraction
import os
import sys

sys.dont_write_bytecode = True
BENCH = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench'
sys.path.insert(0, BENCH)
import points_reference as pr  # noqa: E402
from hgp11_ref import Definition  # noqa: E402

getcontext().prec = 80
TIE = Decimal(10) ** -50
NEAR_TIES = [0]


def popcount(x):
    return bin(x).count('1')


def dsqrt(fr):
    """Racine d'un rationnel positif en Decimal (80 chiffres)."""
    if fr == 0:
        return Decimal(0)
    return (Decimal(fr.numerator) / Decimal(fr.denominator)).sqrt()


class Scale(object):
    """Echelle des niveaux : 'sq' (Fraction exacte) ou 'rad' (Decimal)."""

    def __init__(self, name):
        self.name = name
        self.cache = {}

    def __call__(self, level):
        if self.name == 'sq':
            return level
        got = self.cache.get(level)
        if got is None:
            got = dsqrt(level)
            self.cache[level] = got
        return got

    def le(self, a, b):
        """a <= b, avec signalement des quasi-egalites en echelle rad (traitees comme egalites)."""
        if self.name == 'sq':
            return a <= b
        d = a - b
        if abs(d) < TIE:
            if d != 0:
                NEAR_TIES[0] += 1
            return True
        return d < 0


class Tree(object):
    def __init__(self, res, scale):
        self.nodes = res.nodes
        self.parent = pr.parents_of(res.nodes)
        self.scale = scale
        self.h = [scale(node.level) for node in res.nodes]
        self._chain = {}

    def chain(self, v):
        got = self._chain.get(v)
        if got is None:
            out = [v]
            while self.parent[out[-1]] >= 0:
                out.append(self.parent[out[-1]])
            got = self._chain[v] = out
        return got

    def lca(self, a, b):
        seen = set(self.chain(a))
        for w in self.chain(b):
            if w in seen:
                return w
        raise RuntimeError('foret')

    def meet(self, a, la, b, lb):
        w = self.lca(a, b)
        if w in (a, b):
            return max(la, lb)
        return max(la, lb, self.h[w])

    def alive(self, v, level):
        """Ancetre de v vivant a level (coupe fermee) : h(w) <= level < h(parent(w))."""
        for w in self.chain(v):
            p = self.parent[w]
            if p < 0 or not self.scale.le(self.h[p], level):
                return w
        raise RuntimeError('chaine')


def first_cover(res, n, m):
    """Pour chaque site : {noeud: premier niveau (Fraction) ou ce noeud, qualifie (>= m sites), couvre le site}."""
    out = [dict() for _ in range(n)]
    for cut in res.cuts:
        for v, coverage, _core in cut.closed:
            if popcount(coverage) < m:
                continue
            for i in range(n):
                if coverage >> i & 1 and v not in out[i]:
                    out[i][v] = cut.level
    return out


def rule_H(res, n, kappa=1, m=1, scale='sq'):
    """Regle H_kappa a qualification m : liste par site de (date, proprietaire) dans l'echelle demandee, et arbre."""
    sc = Scale(scale)
    tree = Tree(res, sc)
    kap = Fraction(kappa) if scale == 'sq' else Decimal(Fraction(kappa).numerator) / Decimal(Fraction(kappa).denominator)
    prof = first_cover(res, n, m)
    out = []
    for i in range(n):
        pts = [(sc(level), v) for v, level in prof[i].items()]
        if not pts:
            out.append(None)  # site jamais qualifie (impossible a la racine si m <= n)
            continue
        t = min(a for a, _v in pts)
        v1 = min(v for a, v in pts if a == t)
        e = t
        for a, v in pts:
            val = tree.meet(v1, t, v, a) - kap * (a - t)
            if val > e:
                e = val
        out.append((e, tree.alive(v1, e)))
    return out, tree


def rule_reference(res, n, m, scale='sq'):
    """core et cover de l'oracle (points_reference), transportes dans l'echelle demandee."""
    ref, _t = pr.reference_rules(res, n, m)
    sc = Scale(scale)
    tree = Tree(res, sc)
    out = {}
    for rule in ('core', 'cover', 'first', 'margin1', 'margin'):
        out[rule] = [(sc(level), v) for level, v in ref[rule]]
    return out, tree


def ultrametric(entries, tree):
    n = len(entries)
    u = [[None] * n for _ in range(n)]
    for i in range(n):
        if entries[i] is None:
            continue
        ei, oi = entries[i]
        u[i][i] = ei
        for j in range(i + 1, n):
            if entries[j] is None:
                continue
            ej, oj = entries[j]
            u[i][j] = u[j][i] = tree.meet(oi, ei, oj, ej)
    return u


def blocks_at(u, level, le=None):
    """Blocs (sites entres) a la coupe fermee level."""
    if le is None:
        def le(a, b):
            return a <= b
    n = len(u)
    active = [i for i in range(n) if u[i][i] is not None and le(u[i][i], level)]
    out, seen = [], set()
    for i in active:
        if i in seen:
            continue
        block = frozenset(j for j in active if le(u[i][j], level))
        seen |= block
        out.append(block)
    return sorted([sorted(b) for b in out])


def fmt(x, digits=6):
    if isinstance(x, Fraction):
        return '%s (~%.*f)' % (x, digits, float(x))
    if isinstance(x, Decimal):
        return '%.*f' % (digits, float(x))
    return str(x)


def to_float(x):
    return float(x)


def oracle(points, k):
    return Definition([tuple(p) for p in points]).order(k)


def rule_closure(res, n, m):
    """Fermeture qualifiee de l'auditeur (echelle carree) : w(i, j) = premier niveau ou une meme composante d'au
    moins m sites couvre i et j ; u = fermeture minimax ; diagonale = premiere hyperarete qualifiee."""
    w = [[None] * n for _ in range(n)]
    for cut in res.cuts:
        for _v, coverage, _core in cut.closed:
            if popcount(coverage) < m:
                continue
            members = [i for i in range(n) if coverage >> i & 1]
            for a in members:
                for b in members:
                    if w[a][b] is None:
                        w[a][b] = cut.level
    u = [row[:] for row in w]
    for c in range(n):
        for a in range(n):
            if u[a][c] is None:
                continue
            for b in range(n):
                if u[c][b] is None:
                    continue
                val = max(u[a][c], u[c][b])
                if u[a][b] is None or val < u[a][b]:
                    u[a][b] = val
    return u
