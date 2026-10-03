#!/usr/bin/env python3
"""Pendaison fidele a marge mesuree en RAYON (regle H_m^r), decisions exactes.

Meme regle que H_m (bench/points_hierarchy.py, docs/HIERARCHIE_POINTS.md) mais la marge se mesure sur
r = sqrt(a) au lieu du niveau carre a :

    e_i = r(t_i) + max(0, max_q [ r(m(p_i, q)) - r(h(q)) ]),   proprietaire = ancetre de p_i vivant a e_i.

C'est le parametre de l'entrelacement P5 (decalage additif epsilon en rayon) : la stabilite H3 y vaut 3 epsilon
pour les entrees et 5 epsilon pour les hauteurs, uniformement, sans facteur d'echelle. Toute decision de la regle
(rival maximal, proprietaire, rang plancher) compare deux sommes de deux racines de rationnels et se tranche
exactement par elevations au carre controlees (sqrt_cmp2). Seul l'ordre de deux dates de sites differents
strictement entre deux niveaux (3 racines contre 3) passe par des encadrements entiers isqrt de precision
croissante ; une egalite non identique au-dela de 2^-4096 est comptee et traitee comme egalite.
"""
from fractions import Fraction
import math

import numpy as np

import points_hierarchy as ph

ZERO = Fraction(0)


def sign(v):
    return (v > 0) - (v < 0)


def sqrt_diff_cmp(x, y, u):
    """Signe exact de sqrt(x) - sqrt(y) - u, pour x, y >= 0 rationnels et u rationnel."""
    d = sign(x - y)
    if d >= 0 and u <= 0:
        return 0 if d == 0 and u == 0 else 1
    if d <= 0 and u >= 0:
        return 0 if d == 0 and u == 0 else -1
    if d > 0:  # u > 0 : sqrt x - sqrt y - u a le signe de (x - y - u^2) - 2 u sqrt y
        w = x - y - u * u
        if w <= 0:
            return 0 if w == 0 and y == 0 else -1
        return sign(w * w - 4 * u * u * y)
    return -sqrt_diff_cmp(y, x, -u)  # d < 0 et u < 0


def sqrt_cmp2(a, b, c, d):
    """Signe exact de (sqrt a + sqrt b) - (sqrt c + sqrt d) ; les deux sommes sont positives."""
    return sqrt_diff_cmp(a * b, c * d, ((c + d) - (a + b)) / 2)


def sqrt_bounds(f, bits):
    """Encadrement [lo, hi) de sqrt(f) par isqrt a 2^-bits pres (relatif au denominateur)."""
    n, d = f.numerator, f.denominator
    s = math.isqrt((n * d) << (2 * bits))
    scale = d << bits
    return Fraction(s, scale), Fraction(s + 1, scale)


class RValue(object):
    """Valeur exacte sqrt(t) + sqrt(m) - sqrt(q) (rayon), t, m, q rationnels positifs ou nuls."""
    __slots__ = ('t', 'm', 'q')
    undecided = 0  # comparaisons 3 contre 3 non separees a 2^-4096 (comptees, traitees comme egalites)

    def __init__(self, t, m=ZERO, q=ZERO):
        self.t, self.m, self.q = t, m, q

    def approx(self):
        return math.sqrt(self.t) + math.sqrt(self.m) - math.sqrt(self.q)

    def cmp_sqrt(self, a):
        """Signe de self - sqrt(a)."""
        return sqrt_cmp2(self.t, self.m, a, self.q)

    def cmp(self, other):
        left = sorted(x for x in (self.t, self.m, other.q) if x)
        right = sorted(x for x in (other.t, other.m, self.q) if x)
        for x in list(left):  # termes communs : retires exactement
            if x in right:
                left.remove(x)
                right.remove(x)
        if not left and not right:
            return 0
        if len(left) <= 2 and len(right) <= 2:
            return sqrt_cmp2(*(left + [ZERO] * (2 - len(left))), *(right + [ZERO] * (2 - len(right))))
        bits = 96
        while bits <= 4096:
            lo_l = sum((sqrt_bounds(x, bits)[0] for x in left), ZERO)
            hi_l = sum((sqrt_bounds(x, bits)[1] for x in left), ZERO)
            lo_r = sum((sqrt_bounds(x, bits)[0] for x in right), ZERO)
            hi_r = sum((sqrt_bounds(x, bits)[1] for x in right), ZERO)
            if lo_l >= hi_r:
                return 1
            if lo_r >= hi_l:
                return -1
            bits *= 2
        RValue.undecided += 1
        return 0

    def square_approx(self):
        return self.approx() ** 2

    def __str__(self):
        if not self.m and not self.q:
            return 'sqrt(%s)' % self.t
        return 'sqrt(%s)+sqrt(%s)-sqrt(%s)' % (self.t, self.m, self.q)


class RadiusHanging(ph.Hanging):
    """Pendaison dont les dates sont des RValue ; meme interface que ph.Hanging pour l'evaluateur."""

    def __init__(self, order, values, owner, floor, strict, rule, extra=None):
        ph.Hanging.__init__(self, order, None, None, owner, floor, strict, rule, extra)
        self.values = values

    def value(self, i):
        return self.values[i]

    def cmp_entries(self, i, j):
        return self.values[i].cmp(self.values[j])

    def le(self, i, level):
        if isinstance(level, RValue):
            return self.values[i].cmp(level) <= 0
        return self.values[i].cmp_sqrt(Fraction(level)) <= 0

    def square(self, i):
        """Date au carre en flottant (lecture seulement)."""
        return self.values[i].square_approx()


def floor_rank_radius(levels, value):
    target = value.approx() ** 2
    r = int(np.searchsorted(levels.approx, target, side='right')) - 1
    r = max(0, min(r, len(levels) - 1))
    while r + 1 < len(levels) and value.cmp_sqrt(Fraction(*levels.exact(r + 1))) >= 0:
        r += 1
    while r > 0 and value.cmp_sqrt(Fraction(*levels.exact(r))) < 0:
        r -= 1
    return r, value.cmp_sqrt(Fraction(*levels.exact(r))) > 0


def ancestor_at_radius(order, start, value):
    """Ancetre le plus haut de start dont le rayon de naissance est <= value (coupe fermee)."""
    _, table = order.lifting()
    x = int(start)
    for up in reversed(table):
        y = int(up[x])
        if y != x and value.cmp_sqrt(Fraction(*order.levels.exact(int(order.rank[y])))) >= 0:
            x = y
    while order.parent[x] >= 0 and \
            value.cmp_sqrt(Fraction(*order.levels.exact(int(order.rank[order.parent[x]])))) >= 0:
        x = int(order.parent[x])
    return x


def hang_margin_radius(order, m, rule='margin_r'):
    """Regle H_m^r : rival maximal en rayon, decisions exactes."""
    levels = order.levels
    qual = ph.qualify(order, m)
    node, rank = ph.qualified_starts(order, qual)
    t, p1 = ph.first_points(order, node, rank)
    a = p1[order.inc_site]
    w = order.lca(a, node)
    rival = w != node
    meet = order.rank[w]
    root = np.sqrt(levels.approx)
    approx = np.where(rival, root[meet] - root[rank], 0.0)
    best = np.maximum.reduceat(approx, order.offsets[:-1])
    slack = 1e-9 * (root[meet] + root[rank]) + 1e-300
    candidate = np.flatnonzero(rival & (approx + slack >= best[order.inc_site] - 1e-9 * np.abs(best[order.inc_site])))
    chosen = {}
    for j in candidate.tolist():
        s = int(order.inc_site[j])
        mj, qj = Fraction(*levels.exact(int(meet[j]))), Fraction(*levels.exact(int(rank[j])))
        if mj <= qj:
            continue  # terme nul ou negatif : pas un rival effectif
        old = chosen.get(s)
        if old is None or sqrt_cmp2(mj, old[1], old[0], qj) > 0:  # sqrt mj - sqrt qj > sqrt mo - sqrt qo
            chosen[s] = (mj, qj)
    values, owner, delayed = [], p1.copy(), 0
    for i in range(order.n):
        ti = Fraction(*levels.exact(int(t[i])))
        pick = chosen.get(i)
        if pick is None:
            values.append(RValue(ti))
            continue
        delayed += 1
        value = RValue(ti, pick[0], pick[1])
        values.append(value)
        owner[i] = ancestor_at_radius(order, int(p1[i]), value)
    floor = np.zeros(order.n, dtype=np.int64)
    strict = np.zeros(order.n, dtype=bool)
    for i, value in enumerate(values):
        floor[i], strict[i] = floor_rank_radius(levels, value)
    o = owner
    alive = (order.rank[o] <= floor) & ((order.parent[o] < 0) |
                                        (order.rank[np.maximum(order.parent[o], 0)] > floor))
    ph.need(bool(np.all(alive)), 'proprietaire_non_vivant:' + rule)
    return RadiusHanging(order, values, owner, floor, strict, rule, dict(delayed=delayed))


def ultrametric_radius(hanging):
    """Hauteurs de reunion en rayon, flottants de precision double (tests seulement, petits nuages)."""
    order, n = hanging.order, hanging.order.n
    e = [hanging.values[i].approx() for i in range(n)]
    u = [[None] * n for _ in range(n)]
    for i in range(n):
        u[i][i] = e[i]
        for j in range(i + 1, n):
            a, b = int(hanging.owner[i]), int(hanging.owner[j])
            w = int(order.lca(np.array([a]), np.array([b]))[0])
            value = max(e[i], e[j])
            if w not in (a, b):
                value = max(value, math.sqrt(order.levels.approx[int(order.rank[w])]))
            u[i][j] = u[j][i] = value
    return u
