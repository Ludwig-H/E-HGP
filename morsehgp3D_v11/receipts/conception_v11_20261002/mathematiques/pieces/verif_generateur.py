#!/usr/bin/env python3
"""Piece de MATHEMATIQUES.md (v11), section 3 : modele executable du generateur par boites de centres, en fractions.

Le modele applique a la lettre les enonces GEN-D, GEN-DLOC, GEN-L, GEN-C, GEN-A, GEN-P, GEN-M, GEN-Z, GEN-S, GEN-E :
arbre binaire de paves demi-ouverts ajustes, listes filtrees par dominance (Y tire au hasard a chaque noeud),
feuilles enumerees par masques de dominance et seuils theta_q = K + 1 - q, recensement sur la liste avec arret
anticipe, table des supports canoniques deja emis. Il est compare au catalogue Cat_K calcule par la definition
(tous les supports de 2 a 4 sites, recensement sur tout le nuage). Poids permis (positions repetees).

Il verifie aussi, a part : GEN-DLOC contre les huit coins du pave ; GEN-Z contre l'intersection exacte de la droite
des equidistants et du pave ferme, cas d'egalite compris (droite par une arete ou un sommet).

Trois reglages par nuage et par K : feuille de 2 sites et profondeur 5 (toutes les feuilles sont arretees par la
profondeur : la regle d'arret n'entre pas dans l'exactitude), feuille de K + 3, feuille de 12 ; profondeur 9 au plus.
Une feuille plus petite que K + 3 a profondeur libre fait exploser l'arbre (GEN-COUT) : ce n'est pas essaye ici.

Usage : python3 -B verif_generateur.py [graine] [nombre de nuages]
Code de sortie : 0 aucun ecart, 1 au moins un ecart.
"""
import random
import sys
from fractions import Fraction
from itertools import combinations

F = Fraction


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def center2(a, b):
    return tuple(F(x + y, 2) for x, y in zip(a, b))


def center3(a, b, c):
    u, v = sub(b, a), sub(c, a)
    w = cross(u, v)
    ww = dot(w, w)
    if ww == 0:
        return None
    t = tuple(dot(u, u) * v[i] - dot(v, v) * u[i] for i in range(3))
    n = cross(t, w)
    return tuple(a[i] + F(n[i], 2 * ww) for i in range(3))


def center4(a, b, c, d):
    u, v, s = sub(b, a), sub(c, a), sub(d, a)
    det = dot(u, cross(v, s))
    if det == 0:
        return None
    vs, su, uv = cross(v, s), cross(s, u), cross(u, v)
    n = tuple(dot(u, u) * vs[i] + dot(v, v) * su[i] + dot(s, s) * uv[i] for i in range(3))
    return tuple(a[i] + F(n[i], 2 * det) for i in range(3))


def d2(c, x):
    return sum((F(a) - b) ** 2 for a, b in zip(x, c))


def acute(a, b, c):
    return dot(sub(b, a), sub(c, a)) > 0 and dot(sub(a, b), sub(c, b)) > 0 and dot(sub(a, c), sub(b, c)) > 0


def inside_tetra(t, c):
    """Centre strictement interieur au tetraedre non degenere t."""
    for f in range(4):
        p0, p1, p2 = t[(f + 1) % 4], t[(f + 2) % 4], t[(f + 3) % 4]
        w = cross(sub(p1, p0), sub(p2, p0))
        so = dot(w, sub(t[f], p0))
        sc = dot(w, sub(c, p0))
        if sc == 0 or (sc > 0) != (so > 0):
            return False
    return True


def support_center(pts):
    """Centre du support (2 a 4 sites) si c'en est un (affinement independant, centre dans l'interieur relatif)."""
    if len(pts) == 2:
        return center2(*pts)
    if len(pts) == 3:
        if cross(sub(pts[1], pts[0]), sub(pts[2], pts[0])) == (0, 0, 0) or not acute(*pts):
            return None
        return center3(*pts)
    c = center4(*pts)
    if c is None or not inside_tetra(pts, c):
        return None
    return c


def canonical(sites, shell, c):
    """q_min et support canonique : plus petit cardinal, puis premier dans l'ordre lexicographique des indices."""
    for q in (2, 3, 4):
        for s in combinations(shell, q):
            if support_center([sites[i] for i in s]) == c:
                return q, s
    return None


def brute_catalogue(sites, w, kmax):
    found = {}
    n = len(sites)
    for q in (2, 3, 4):
        for s in combinations(range(n), q):
            c = support_center([sites[i] for i in s])
            if c is None:
                continue
            lv = d2(c, sites[s[0]])
            if (c, lv) in found:
                continue
            inner = tuple(i for i in range(n) if d2(c, sites[i]) < lv)
            shell = tuple(i for i in range(n) if d2(c, sites[i]) == lv)
            qm, sup = canonical(sites, shell, c)
            found[(c, lv)] = (sup, qm, sum(w[i] for i in inner), inner, shell, lv)
    return sorted(rec for rec in found.values() if rec[2] + rec[1] <= kmax + 1)


def dominates(z, x, lo, hi):
    """GEN-DLOC : z domine x sur le pave ferme [lo, hi]."""
    xs, zs = sub(x, lo), sub(z, lo)
    rhs = sum(max(0, 2 * (hi[i] - lo[i]) * (xs[i] - zs[i])) for i in range(3))
    return dot(xs, xs) - dot(zs, zs) > rhs


def line_meets(a1, a2, a3, lo, hi):
    """GEN-Z : la droite des equidistants de trois sites non alignes rencontre le pave ferme (critere du zonogone)."""
    u, v = sub(a1, a2), sub(a1, a3)
    y0 = tuple((lo[i] + hi[i]) / F(2) for i in range(3))
    f0 = (d2(y0, a1) - d2(y0, a2), d2(y0, a1) - d2(y0, a3))
    g = [((hi[i] - lo[i]) * u[i], (hi[i] - lo[i]) * v[i]) for i in range(3)]
    for i in range(3):
        if u[i] == 0 and v[i] == 0:
            continue
        n = (v[i], -u[i])
        if abs(n[0] * f0[0] + n[1] * f0[1]) > sum(abs(n[0] * g[j][0] + n[1] * g[j][1]) for j in range(3)):
            return False
    return True


def line_meets_exact(a1, a2, a3, lo, hi):
    """La meme question par decoupage exact du parametre de la droite c0 + s w dans les trois tranches."""
    c0 = center3(a1, a2, a3)
    w = cross(sub(a2, a1), sub(a3, a1))
    smin, smax = None, None
    for i in range(3):
        if w[i] == 0:
            if not lo[i] <= c0[i] <= hi[i]:
                return False
            continue
        s1, s2 = (lo[i] - c0[i]) / w[i], (hi[i] - c0[i]) / w[i]
        if s1 > s2:
            s1, s2 = s2, s1
        smin = s1 if smin is None else max(smin, s1)
        smax = s2 if smax is None else min(smax, s2)
    return smin is None or smin <= smax


class Model(object):
    """Generateur abstrait. leaf : taille de feuille M ; depth : profondeur maximale (regle d'arret)."""

    def __init__(self, sites, w, kmax, leaf, depth, rnd):
        self.sites, self.w, self.k, self.leaf, self.depth, self.rnd = sites, w, kmax, leaf, depth, rnd
        self.out = []
        self.nodes = self.leaves = self.judged = 0
        self.centres = {}

    def run(self):
        n = len(self.sites)
        lo = tuple(F(min(s[i] for s in self.sites)) for i in range(3))
        ext = max(max(s[i] for s in self.sites) - lo[i] for i in range(3))
        side = 1
        while side <= ext:
            side *= 2
        hi = tuple(lo[i] + side for i in range(3))
        self.node(lo, hi, list(range(n)), 0)
        return sorted(self.out)

    def node(self, lo, hi, parent, depth):
        self.nodes += 1
        # GEN-L : Y quelconque dans la liste parente
        y = [z for z in parent if self.rnd.random() < 0.8] if depth else []
        cand = [x for x in parent
                if sum(self.w[z] for z in y if z != x and dominates(self.sites[z], self.sites[x], lo, hi)) < self.k]
        if not cand:
            return
        # GEN-A : pave ajuste Q inter [e-, e+ + 1)
        alo = tuple(max(lo[i], F(min(self.sites[s][i] for s in cand))) for i in range(3))
        ahi = tuple(min(hi[i], F(max(self.sites[s][i] for s in cand)) + 1) for i in range(3))
        if any(alo[i] >= ahi[i] for i in range(3)):
            return
        if len(cand) <= self.leaf or depth >= self.depth:
            self.leaves += 1
            self.enumerate(cand, alo, ahi)
            return
        ax = max(range(3), key=lambda i: ahi[i] - alo[i])
        mid = (alo[ax] + ahi[ax]) / 2
        left_hi = tuple(mid if i == ax else ahi[i] for i in range(3))
        right_lo = tuple(mid if i == ax else alo[i] for i in range(3))
        self.node(alo, left_hi, cand, depth + 1)   # GEN-P : deux moities demi-ouvertes
        self.node(right_lo, ahi, cand, depth + 1)

    def in_box(self, c, lo, hi):
        return all(lo[i] <= c[i] < hi[i] for i in range(3))

    def enumerate(self, cand, lo, hi):
        sites, w, k = self.sites, self.w, self.k
        m = len(cand)
        th = {2: k - 1, 3: k - 2, 4: k - 3}
        dom = [set(j for j in range(m) if j != i and dominates(sites[cand[j]], sites[cand[i]], lo, hi))
               for i in range(m)]
        wt = lambda s: sum(w[cand[j]] for j in s)  # noqa: E731
        memo = set()
        live2, live3 = set(), set()
        for i, j in combinations(range(m), 2):
            if i in dom[j] or j in dom[i]:
                continue
            d = wt(dom[i] | dom[j])
            if d > th[2]:
                continue
            if d <= th[3]:
                live2.add((i, j))
            c = center2(sites[cand[i]], sites[cand[j]])
            if self.in_box(c, lo, hi):
                self.judge(cand, (i, j), c, th[2], memo, lo, hi)
        if th[3] < 0:
            return
        for i, j, l in combinations(range(m), 3):
            if (i, j) not in live2 or (i, l) not in live2 or (j, l) not in live2:
                continue
            d = wt(dom[i] | dom[j] | dom[l])
            if d > th[3]:
                continue
            a, b, e = sites[cand[i]], sites[cand[j]], sites[cand[l]]
            if cross(sub(b, a), sub(e, a)) == (0, 0, 0):
                continue
            if not line_meets(a, b, e, lo, hi):
                continue
            if d <= th[4]:
                live3.add((i, j, l))
            if acute(a, b, e):
                c = center3(a, b, e)
                if self.in_box(c, lo, hi):
                    self.judge(cand, (i, j, l), c, th[3], memo, lo, hi)
        if th[4] < 0:
            return
        for quad in combinations(range(m), 4):
            if any(t3 not in live3 for t3 in combinations(quad, 3)):
                continue
            d = wt(set().union(*(dom[i] for i in quad)))
            if d > th[4]:
                continue
            pts = [sites[cand[i]] for i in quad]
            c = center4(*pts)
            if c is None or not self.in_box(c, lo, hi) or not inside_tetra(pts, c):
                continue
            self.judge(cand, quad, c, th[4], memo, lo, hi)

    def judge(self, cand, pres, c, theta, memo, lo, hi):
        """GEN-C : recensement sur la liste de la feuille, arret des que le poids interieur depasse theta."""
        self.judged += 1
        sites, w = self.sites, self.w
        lv = d2(c, sites[cand[pres[0]]])
        p = 0
        inner, shell = [], []
        for s in cand:
            dd = d2(c, sites[s])
            if dd < lv:
                p += w[s]
                if p > theta:
                    return
                inner.append(s)
            elif dd == lv:
                shell.append(s)
        gen = tuple(cand[i] for i in pres)
        if len(shell) == len(pres):
            q, sup = len(pres), gen
        else:
            q, sup = canonical(sites, shell, c)   # GEN-E : parties de la coquille, cardinal croissant
            if sup in memo:
                return
            memo.add(sup)
        if p + q <= self.k + 1:
            self.out.append((sup, q, p, tuple(inner), tuple(shell), lv))


def unit_checks(rnd, chk):
    # GEN-DLOC contre les huit coins (le maximum d'une forme affine sur un pave est atteint en un coin)
    for _ in range(2000):
        lo = tuple(F(rnd.randint(-8, 8)) for _ in range(3))
        hi = tuple(lo[i] + rnd.randint(1, 6) for i in range(3))
        x = tuple(rnd.randint(-10, 14) for _ in range(3))
        z = tuple(rnd.randint(-10, 14) for _ in range(3))
        corners = [tuple((lo, hi)[b >> i & 1][i] for i in range(3)) for b in range(8)]
        want = all(d2(c, z) < d2(c, x) for c in corners)
        chk('GEN-DLOC = huit coins', dominates(z, x, lo, hi) == want)
        if any(d2(c, z) == d2(c, x) for c in corners) and all(d2(c, z) <= d2(c, x) for c in corners):
            chk('GEN-DLOC egalite en un coin : pas de dominance', not dominates(z, x, lo, hi))
    # GEN-Z contre l'intersection exacte, puis paves construits pour que la droite touche une arete ou un sommet
    done = 0
    while done < 1500:
        a = [tuple(rnd.randint(0, 9) for _ in range(3)) for _ in range(3)]
        if cross(sub(a[1], a[0]), sub(a[2], a[0])) == (0, 0, 0):
            continue
        lo = tuple(F(rnd.randint(-4, 10)) for _ in range(3))
        hi = tuple(lo[i] + rnd.randint(1, 5) for i in range(3))
        chk('GEN-Z = intersection exacte', line_meets(a[0], a[1], a[2], lo, hi) == line_meets_exact(a[0], a[1], a[2], lo, hi))
        # pave dont un sommet est un point de la droite, pave place au-dela : contact en un sommet au plus
        c0 = center3(*a)
        w = cross(sub(a[1], a[0]), sub(a[2], a[0]))
        step = F(rnd.randint(-3, 3), 2)
        pt = tuple(c0[i] + step * w[i] for i in range(3))
        size = tuple(F(rnd.randint(1, 3)) for _ in range(3))
        sgn = tuple(rnd.choice((0, 1)) for _ in range(3))
        lo2 = tuple(pt[i] - sgn[i] * size[i] for i in range(3))
        hi2 = tuple(lo2[i] + size[i] for i in range(3))
        chk('GEN-Z contact au sommet : rencontre', line_meets(a[0], a[1], a[2], lo2, hi2) and
            line_meets_exact(a[0], a[1], a[2], lo2, hi2))
        done += 1


def clouds(rnd, count):
    out = []
    circle = [(x + 5, y + 5, 2) for x in range(-5, 6) for y in range(-5, 6) if x * x + y * y == 25]
    sphere = [(x + 3, y + 3, z + 3) for x in range(-3, 4) for y in range(-3, 4) for z in range(-3, 4)
              if x * x + y * y + z * z == 9]
    kinds = ('generic', 'grid3', 'grid4', 'coplanar', 'cocircular', 'cospherical', 'collinear', 'clusters', 'weights')
    for i in range(count):
        kind = kinds[i % len(kinds)]
        n = rnd.randint(5, 9)
        pts = set()
        while len(pts) < n:
            if kind in ('generic', 'weights'):
                pts.add((rnd.randint(0, 40), rnd.randint(0, 40), rnd.randint(0, 40)))
            elif kind == 'grid3':
                pts.add((rnd.randint(0, 2), rnd.randint(0, 2), rnd.randint(0, 2)))
            elif kind == 'grid4':
                pts.add((rnd.randint(0, 3), rnd.randint(0, 3), rnd.randint(0, 3)))
            elif kind == 'coplanar':
                pts.add((rnd.randint(0, 6), rnd.randint(0, 6), 3))
            elif kind == 'cocircular':
                pts.add(rnd.choice(circle) if rnd.random() < 0.85 else (rnd.randint(0, 10), rnd.randint(0, 10), 2))
            elif kind == 'cospherical':
                pts.add(rnd.choice(sphere) if rnd.random() < 0.85 else
                        (rnd.randint(0, 6), rnd.randint(0, 6), rnd.randint(0, 6)))
            elif kind == 'collinear':
                pts.add((rnd.randint(0, 20), 0, 0))
            else:
                pts.add(tuple(rnd.choice((0, 5, 30)) + rnd.randint(0, 1) for _ in range(3)))
        pts = sorted(pts)
        w = [rnd.choice((1, 1, 2, 3)) for _ in pts] if kind == 'weights' else [1] * len(pts)
        out.append(('%s_%d' % (kind, i), pts, w))
    return out


def main():
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 20261002
    count = int(sys.argv[2]) if len(sys.argv) > 2 else 36
    rnd = random.Random(seed)
    counts, fails = {}, []

    def chk(name, cond, detail=''):
        counts[name] = counts.get(name, 0) + 1
        if not cond:
            fails.append((name, detail))
    unit_checks(rnd, chk)
    fixed = [
        ('carre', [(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)], [1] * 4),
        ('cube', [(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, 2)], [1] * 8),
        ('octaedre', [(15, 10, 10), (5, 10, 10), (10, 15, 10), (10, 5, 10), (10, 10, 15), (10, 10, 5)], [1] * 6),
        ('triangle_rectangle', [(0, 0, 0), (3, 0, 0), (0, 4, 0)], [1] * 3),
        ('paire_31', [(12, 12, 12), (14, 12, 12)], [3, 1]),
        ('triangle_311', [(0, 0, 0), (6, 0, 0), (3, 5, 0)], [3, 1, 1]),
    ]
    balls = nodes = leaves = judged = runs = 0
    for label, pts, w in fixed + clouds(rnd, count):
        for kmax in (1, 2, 3, 5):
            want = brute_catalogue(pts, w, kmax)
            for leaf, depth in ((2, 5), (kmax + 3, 9), (12, 9)):
                model = Model(pts, w, kmax, leaf, depth, rnd)
                got = model.run()
                chk('GEN-G catalogue = definition', got == want, '%s K=%d M=%d : %d contre %d' % (
                    label, kmax, leaf, len(got), len(want)))
                chk('GEN-E aucune boule emise deux fois', len(set(r[0] for r in got)) == len(got), label)
                runs += 1
                nodes += model.nodes
                leaves += model.leaves
                judged += model.judged
            balls += len(want)
    print('graine %d ; %d nuages ; %d executions du modele ; %d boules attendues (somme sur K) ; %d noeuds, %d feuilles, '
          '%d recensements' % (seed, len(fixed) + count, runs, balls, nodes, leaves, judged))
    for name in sorted(counts):
        print('  %-52s %8d' % (name, counts[name]))
    print('ecarts : %d' % len(fails))
    for name, detail in fails[:15]:
        print('  ECART %s : %s' % (name, detail))
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
