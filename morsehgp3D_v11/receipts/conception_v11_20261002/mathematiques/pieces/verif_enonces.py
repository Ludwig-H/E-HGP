#!/usr/bin/env python3
"""Piece de MATHEMATIQUES.md (v11) : confrontation directe des enonces a la definition, par force brute exacte.

Aucun import du depot : boules minimales, enveloppes convexes et graphes Gamma_k sont recalcules ici en fractions.
Ce script ne prouve rien : il cherche un contre-exemple aux enonces CAT-2, LOC-2, LOC-3, LOC-W, TOUR-B, TOUR-C,
TOUR-D, TOUR-G, LOC-4, LOC-5, PTS-CORE, PTS-COVER, PTS-ENC, PTS-MR et INV-EULER sur de petits nuages, generiques et degeneres.

Usage : python3 -B verif_enonces.py [graine] [nombre de nuages aleatoires] [n maximal] [copies]
Avec « copies », les nuages portent des positions repetees : chaque copie est un point (section 10).
Code de sortie : 0 aucun ecart, 1 au moins un ecart.
"""
import random
import sys
from fractions import Fraction
from itertools import combinations
from math import comb


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def solve(rows, rhs):
    size = len(rows)
    m = [[Fraction(v) for v in rows[i]] + [Fraction(rhs[i])] for i in range(size)]
    for col in range(size):
        piv = next((r for r in range(col, size) if m[r][col] != 0), None)
        if piv is None:
            return None
        m[col], m[piv] = m[piv], m[col]
        for r in range(size):
            if r != col and m[r][col] != 0:
                f = m[r][col] / m[col][col]
                m[r] = [x - f * y for x, y in zip(m[r], m[col])]
    return [m[i][size] / m[i][i] for i in range(size)]


class Cloud(object):
    def __init__(self, pts):
        self.p = [tuple(Fraction(c) for c in q) for q in pts]
        self.n = len(pts)
        self._circ = {}
        self._meb = {}
        self._bary = {}

    def d2(self, c, i):
        return sum((a - b) ** 2 for a, b in zip(self.p[i], c))

    def circum(self, s):
        """Centre equidistant dans l'enveloppe affine de s (tuple d'indices), ou None si s est affinement lie."""
        if s not in self._circ:
            p0 = self.p[s[0]]
            if len(s) == 1:
                self._circ[s] = (p0, Fraction(0))
            else:
                d = [sub(self.p[i], p0) for i in s[1:]]
                lam = solve([[2 * dot(a, b) for b in d] for a in d], [dot(a, a) for a in d])
                if lam is None:
                    self._circ[s] = None
                else:
                    c = tuple(p0[j] + sum(w * v[j] for w, v in zip(lam, d)) for j in range(3))
                    self._circ[s] = (c, sum((c[j] - p0[j]) ** 2 for j in range(3)))
        return self._circ[s]

    def meb(self, part):
        """(niveau, centre) de la plus petite boule englobante : plus petite sphere circonscrite d'un support de
        1 a 4 points de la partie qui contient la partie (CAD-1)."""
        part = tuple(sorted(part))
        if part not in self._meb:
            best = None
            for q in range(1, min(4, len(part)) + 1):
                for s in combinations(part, q):
                    cs = self.circum(s)
                    if cs is None or (best is not None and cs[1] >= best[0]):
                        continue
                    if all(self.d2(cs[0], i) <= cs[1] for i in part):
                        best = (cs[1], cs[0])
            self._meb[part] = best
        return self._meb[part]

    def bary(self, c, s):
        """Coordonnees barycentriques de c dans s (affinement independant), ou None si c n'est pas dans aff(s)
        ou si s est affinement lie."""
        key = (c, s)
        if key not in self._bary:
            p0 = self.p[s[0]]
            res = None
            if len(s) == 1:
                res = [Fraction(1)] if p0 == c else None
            else:
                d = [sub(self.p[i], p0) for i in s[1:]]
                rhs = sub(c, p0)
                lam = solve([[dot(a, b) for b in d] for a in d], [dot(a, rhs) for a in d])
                if lam is not None:
                    back = tuple(sum(w * v[j] for w, v in zip(lam, d)) for j in range(3))
                    if back == rhs:
                        res = [1 - sum(lam)] + lam
            self._bary[key] = res
        return self._bary[key]

    def in_hull(self, c, a):
        """c dans l'enveloppe convexe fermee de la partie a (Caratheodory : un simplexe d'au plus 4 points)."""
        a = tuple(sorted(a))
        for q in range(1, min(4, len(a)) + 1):
            for s in combinations(a, q):
                lam = self.bary(c, s)
                if lam is not None and all(x >= 0 for x in lam):
                    return True
        return False

    def qmin(self, c, shell):
        for q in range(2, min(4, len(shell)) + 1):
            for s in combinations(shell, q):
                lam = self.bary(c, s)
                if lam is not None and all(x > 0 for x in lam):
                    return q
        return None


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def covering_family(X, c, shell):
    """LOC-5 : parties U+(v0) u W(v0, l), v0 = +-(u_i x u_j), l dans U0(v0). Rend des ensembles d'indices.
    Avec des copies, deux indices de meme position ont le meme u : ils entrent et sortent ensemble."""
    u = dict((x, sub(X.p[x], c)) for x in shell)
    positions = sorted(set(u.values()))
    fam = set()
    for i, ui in enumerate(positions):
        for uj in positions[i + 1:]:
            n = cross(ui, uj)
            if n == (0, 0, 0):
                continue
            for sg in (1, -1):
                v0 = tuple(sg * t for t in n)
                plus = frozenset(x for x in shell if dot(v0, u[x]) > 0)
                zero = [x for x in shell if dot(v0, u[x]) == 0]
                for l in zero:
                    w = frozenset(y for y in zero if u[y] == u[l] or dot(v0, cross(u[l], u[y])) > 0)
                    fam.add(plus | w)
    if not fam:  # paire antipodale : les parties separables maximales sont les positions
        for pos in positions:
            fam.add(frozenset(x for x in shell if u[x] == pos))
    return fam


def components(vertices, edges):
    par = dict((v, v) for v in vertices)

    def find(v):
        while par[v] != v:
            par[v] = par[par[v]]
            v = par[v]
        return v
    for faces in edges:
        r0 = find(faces[0])
        for f in faces[1:]:
            r = find(f)
            if r != r0:
                par[r] = r0
    return dict((v, find(v)) for v in vertices)


class Check(object):
    def __init__(self):
        self.fails = []
        self.count = {}

    def ok(self, name, cond, detail=''):
        self.count[name] = self.count.get(name, 0) + 1
        if not cond:
            self.fails.append((name, detail))

    def tally(self, name, k=1):
        self.count[name] = self.count.get(name, 0) + k


def check_cloud(pts, chk, rnd, label):
    X = Cloud(pts)
    n = X.n
    idx = tuple(range(n))
    # toutes les boules critiques : boules minimales des parties d'au moins deux points
    balls = {}
    for size in range(2, n + 1):
        for g in combinations(idx, size):
            lv, c = X.meb(g)
            balls.setdefault((c, lv), None)
    for i in idx:
        balls.setdefault((X.p[i], Fraction(0)), None)
    info = {}
    for (c, lv) in balls:
        inner = tuple(i for i in idx if X.d2(c, i) < lv)
        shell = tuple(i for i in idx if X.d2(c, i) == lv)
        q = 1 if lv == 0 else X.qmin(c, shell)
        chk.ok('CAT-1 support present', q is not None, '%s %s' % (label, (c, lv)))
        info[(c, lv)] = (inner, shell, q)
        # CAT-2 : MEB(F) = b ssi c dans conv(F n U), sur un echantillon de parties de la boule fermee
        pop = inner + shell
        cand = [f for size in range(1, len(pop) + 1) for f in combinations(pop, size)]
        if len(cand) > 40:
            cand = rnd.sample(cand, 40)
        for f in cand:
            trace = tuple(i for i in f if i in shell)
            same = X.meb(f) == (lv, c)
            chk.ok('CAT-2 boule minimale', same == X.in_hull(c, trace) and (same or X.meb(f)[0] < lv),
                   '%s %s %s' % (label, (c, lv), f))
    dk_all = [sorted(sum((a - b) ** 2 for a, b in zip(X.p[x], X.p[y])) for y in idx) for x in idx]
    for k in range(1, n + 1):
        beta_k = dict((f, X.meb(f)[0]) for f in combinations(idx, k))
        beta_k1 = dict((g, X.meb(g)[0]) for g in combinations(idx, k + 1)) if k < n else {}
        levels = sorted(set(beta_k.values()) | set(beta_k1.values()))

        def graph(level, strict):
            keep = (lambda b: b < level) if strict else (lambda b: b <= level)
            vs = [f for f in beta_k if keep(beta_k[f])]
            es = [[g[:i] + g[i + 1:] for i in range(k + 1)] for g in beta_k1 if keep(beta_k1[g])]
            return components(vs, es)
        # INV-EULER : toutes les boules critiques
        total = 0
        for (c, lv), (inner, shell, q) in info.items():
            p, m = len(inner), len(shell)
            t = k - p
            if lv == 0:
                total += 1 if m >= k else 0
            elif 1 <= t <= m:
                for size in range(max(t, 2), m + 1):
                    for b in combinations(shell, size):
                        if X.in_hull(c, b):
                            total += (-1) ** (size - t) * comb(size - 1, t - 1)
        chk.ok('INV-EULER', total == 1, '%s k=%d somme=%d' % (label, k, total))
        # PTS-ENC et PTS-COVER
        for x in idx:
            alpha = min(beta_k[f] for f in beta_k if x in f)
            dk = dk_all[x][k - 1]
            chk.ok('PTS-ENC', dk <= 4 * alpha and alpha <= dk, '%s k=%d x=%d' % (label, k, x))
            first = [key for key, (inner, shell, q) in info.items()
                     if key[1] <= alpha and x in inner + shell and len(inner) + len(shell) >= k]
            chk.ok('PTS-COVER niveau', bool(first) and all(key[1] == alpha for key in first),
                   '%s k=%d x=%d' % (label, k, x))
            chk.ok('PTS-COVER admission p+q<=k', all(len(info[key][0]) + info[key][2] <= k for key in first),
                   '%s k=%d x=%d' % (label, k, x))
            # PTS-CORE : toutes les k-parties de la boule fermee de rayon carre D_k(x) centree en x sont dans
            # une meme composante de Gamma_k(D_k(x)) : la composante de x ne depend d'aucun depart des ex aequo
            near = tuple(y for y in idx if sum((a - b) ** 2 for a, b in zip(X.p[x], X.p[y])) <= dk)
            if len(near) <= 9:
                keep = [f for f in beta_k if beta_k[f] <= dk]
                es = [[g[:i] + g[i + 1:] for i in range(k + 1)] for g in beta_k1 if beta_k1[g] <= dk]
                comp = components(keep, es)
                chk.ok('PTS-CORE composante independante du depart',
                       len(set(comp[f] for f in combinations(near, k))) == 1, '%s k=%d x=%d' % (label, k, x))
        # PTS-MR : blocs core au niveau a dans les blocs de M_k(2 sqrt(a)) ; blocs de M_k(rho) dans ceux de core a 9 rho^2 / 4
        dist2 = lambda x, y: sum((a - b) ** 2 for a, b in zip(X.p[x], X.p[y]))  # noqa: E731

        def core_blocks(level):
            comp = graph(level, False)
            blocks = {}
            for x in idx:
                if dk_all[x][k - 1] <= level:
                    near = tuple(sorted(sorted(idx, key=lambda y: (dist2(x, y), y))[:k]))
                    blocks.setdefault(comp[near], set()).add(x)
            return list(blocks.values())

        def mr_blocks(rho2):
            verts = [x for x in idx if dk_all[x][k - 1] <= rho2]
            comp = components(verts, [[x, y] for x in verts for y in verts if x < y and dist2(x, y) <= rho2])
            blocks = {}
            for x in verts:
                blocks.setdefault(comp[x], set()).add(x)
            return list(blocks.values())
        if n <= 7:
            for level in levels:
                big = mr_blocks(4 * level)
                chk.ok('PTS-MR core(a) dans M(2 sqrt a)', all(any(blk <= b2 for b2 in big) for blk in core_blocks(level)),
                       '%s k=%d niveau %s' % (label, k, level))
            for rho2 in sorted(set(dist2(x, y) for x in idx for y in idx if x < y))[:12]:
                big = core_blocks(Fraction(9 * rho2, 4))
                chk.ok('PTS-MR M(rho) dans core(9 rho^2 / 4)', all(any(blk <= b2 for b2 in big) for blk in mr_blocks(rho2)),
                       '%s k=%d rho2 %s' % (label, k, rho2))
        prev = {}
        for level in levels:
            if level == 0:
                prev = graph(level, False)
                zero = [key for key, (inner, shell, q) in info.items() if key[1] == 0 and len(shell) >= k]
                chk.ok('TOUR-B niveau nul : une naissance par position de poids >= k',
                       len(set(prev.values())) == len(zero) and
                       all(len(set(prev[f] for f in combinations(info[key][1], k))) == 1 for key in zero), label)
                continue
            old = graph(level, True)
            new = graph(level, False)
            chk.ok('coupe ouverte = coupe fermee precedente', set(old) == set(prev) and
                   len(set(old.values())) == len(set(prev.values())), '%s k=%d' % (label, k))
            comp_old_in_new = {}
            for f, r in old.items():
                comp_old_in_new.setdefault(new[f], set()).add(r)
            # graphe biparti H de TOUR-C : boules du cas 4 contre composantes anciennes
            hpar = dict((r, r) for r in set(old.values()))

            def hfind(v):
                while hpar[v] != v:
                    hpar[v] = hpar[hpar[v]]
                    v = hpar[v]
                return v
            births = set()
            for (c, lv), (inner, shell, q) in info.items():
                if lv != level:
                    continue
                p, m = len(inner), len(shell)
                pop = inner + shell
                if p + m < k:
                    continue
                t = k - p
                kparts = [tuple(sorted(f)) for f in combinations(pop, k)]
                sep = lambda f: not X.in_hull(c, tuple(i for i in f if i in shell))  # noqa: E731
                vlt = [f for f in kparts if sep(f)]
                veq = [f for f in kparts if not sep(f)]
                chk.ok('CAT-2 niveau des k-parties', all(beta_k[f] < level for f in vlt) and
                       all(beta_k[f] == level for f in veq), '%s k=%d' % (label, k))
                # LOC-2 / Johnson : toutes les k-parties de P_b dans une composante de Gamma_k(a) si p + m >= k + 1
                chk.ok('LOC-1 Johnson ferme', len(set(new[f] for f in kparts)) == 1, '%s k=%d' % (label, k))
                if not vlt:
                    # naissance : composante = toutes les k-parties de P_b, aucun sommet ancien
                    comp = [f for f in new if new[f] == new[kparts[0]]]
                    chk.ok('TOUR-B naissance', sorted(comp) == sorted(kparts) and new[kparts[0]] not in comp_old_in_new
                           and p < k and t >= q, '%s k=%d %s' % (label, k, (c, lv)))
                    births.add(new[kparts[0]])
                    if t >= 1:
                        tsep = [a for a in combinations(shell, t) if not X.in_hull(c, a)]
                        chk.ok('TOUR-B naissance ssi aucune t-partie separable', not tsep, label)
                    chk.tally('naissances')
                    continue
                met = set(old[f] for f in vlt)
                r0 = hfind(next(iter(met)))
                for r in met:
                    hpar[hfind(r)] = r0
                if p >= k or k <= p + q - 2:
                    chk.ok('LOC-W boule inerte : une composante', len(met) == 1, '%s k=%d %s' % (label, k, (c, lv)))
                    chk.tally('inertes')
                    if p < k:
                        chk.ok('LOC-W toute t-partie separable', all(not X.in_hull(c, a) for a in
                                                                    combinations(shell, t)), label)
                else:
                    # fenetre : morceaux, representants
                    tsep = [a for a in combinations(shell, t) if not X.in_hull(c, a)]
                    mpar = dict((a, a) for a in tsep)

                    def mfind(v):
                        while mpar[v] != v:
                            mpar[v] = mpar[mpar[v]]
                            v = mpar[v]
                        return v
                    for i, a in enumerate(tsep):
                        for b2 in tsep[i + 1:]:
                            if not X.in_hull(c, tuple(sorted(set(a) | set(b2)))):
                                mpar[mfind(a)] = mfind(b2)
                    pieces = {}
                    for a in tsep:
                        pieces.setdefault(mfind(a), []).append(a)
                    reps = [tuple(sorted(inner + rnd.choice(v))) for v in pieces.values()]
                    chk.ok('TOUR-B composantes rencontrees = celles des representants',
                           set(old[f] for f in reps) == met, '%s k=%d %s' % (label, k, (c, lv)))
                    # LOC-3 : deux t-parties du meme morceau donnent la meme composante ouverte
                    for v in pieces.values():
                        chk.ok('LOC-2c meme morceau, meme composante',
                               len(set(old[tuple(sorted(inner + a))] for a in v)) == 1, label)
                    # critere par faces d'une (t+1)-partie separable : meme cloture transitive
                    fpar = dict((a, a) for a in tsep)

                    def ffind(v):
                        while fpar[v] != v:
                            fpar[v] = fpar[fpar[v]]
                            v = fpar[v]
                        return v
                    if t + 1 <= m:
                        for g in combinations(shell, t + 1):
                            if not X.in_hull(c, g):
                                faces = [g[:i] + g[i + 1:] for i in range(t + 1)]
                                for f2 in faces[1:]:
                                    fpar[ffind(f2)] = ffind(faces[0])
                    same = all((mfind(a) == mfind(b2)) == (ffind(a) == ffind(b2)) for a in tsep for b2 in tsep)
                    chk.ok('LOC-3 critere par paires = critere par faces', same, label)
                    # LOC-3 (2) : les morceaux sont les composantes du graphe strict induit sur P_b
                    ind = components(vlt, [[g[:i] + g[i + 1:] for i in range(k + 1)]
                                           for g in (tuple(sorted(g)) for g in combinations(pop, k + 1))
                                           if sep(g)] if len(pop) > k else [])
                    chk.ok('LOC-3 morceaux = composantes induites sur P_b', len(set(ind.values())) == len(pieces) and
                           all(len(set(ind[tuple(sorted(inner + a))] for a in v)) == 1 for v in pieces.values()),
                           '%s k=%d %s' % (label, k, (c, lv)))
                    # LOC-4 et LOC-5 : quotient par la famille couvrante des demi-espaces
                    fam = covering_family(X, c, shell)
                    alive = [mm for mm in fam if len(mm) >= t]
                    apar = list(range(len(alive)))

                    def afind(v):
                        while apar[v] != v:
                            apar[v] = apar[apar[v]]
                            v = apar[v]
                        return v
                    for i in range(len(alive)):
                        for j in range(i + 1, len(alive)):
                            if len(alive[i] & alive[j]) >= t:
                                apar[afind(i)] = afind(j)
                    chk.ok('LOC-5 membres separables', all(not X.in_hull(c, tuple(sorted(mm))) for mm in fam), label)
                    chk.ok('LOC-5 famille couvrante', all(any(set(a) <= mm for mm in alive) for a in tsep), label)
                    chk.ok('LOC-4 nombre de morceaux', len(set(afind(i) for i in range(len(alive)))) == len(pieces),
                           '%s k=%d %s' % (label, k, (c, lv)))
                    cls = {}
                    okc = True
                    for a in tsep:
                        key = afind(next(i for i, mm in enumerate(alive) if set(a) <= mm))
                        if cls.setdefault(key, mfind(a)) != mfind(a):
                            okc = False
                    chk.ok('LOC-4 meme partition des parties separables', okc and len(cls) == len(pieces), label)
                    regular = m == q and len(set(X.p[i] for i in shell)) == m
                    if regular:
                        chk.ok('LOC-R coquille reguliere : m morceaux a t = m - 1',
                               t == m - 1 and len(pieces) == m, '%s k=%d' % (label, k))
                    chk.tally('cellules en fenetre a %s morceaux' % ('1' if len(pieces) == 1 else '>=2'))
                    # gain de couverture sans fusion (TOUR-G)
                    cover_before = set(i for f in old if old[f] in met for i in f)
                    if len(met) == 1 and not set(pop) <= cover_before:
                        chk.tally('gains de couverture sans fusion')
                        chk.ok('TOUR-G gain seulement si coquille etendue et t >= q', (not regular) and t >= q, label)
            # TOUR-C : composantes de Gamma_k(a) contenant un sommet ancien = composantes de H
            groups = {}
            for r in hpar:
                groups.setdefault(hfind(r), set()).add(r)
            want = sorted(sorted(map(str, s)) for s in comp_old_in_new.values())
            got = sorted(sorted(map(str, s)) for s in groups.values())
            chk.ok('TOUR-C plateaux', want == got, '%s k=%d niveau %s' % (label, k, level))
            # composantes sans sommet ancien = naissances
            newborn = set(r for r in set(new.values()) if r not in comp_old_in_new)
            chk.ok('TOUR-B naissances = composantes sans sommet ancien', newborn == births, '%s k=%d' % (label, k))
            # TOUR-G (c) : couverture d'une composante = union des P_b, p + q <= k <= p + m, niveau <= a, attachees
            cov = {}
            for f, r in new.items():
                cov.setdefault(r, set()).update(f)
            cov2 = {}
            for (c, lv), (inner, shell, q) in info.items():
                p, m = len(inner), len(shell)
                if lv <= level and p + q <= k <= p + m:
                    pop = inner + shell
                    r = new[tuple(sorted(pop[:k]))]
                    cov2.setdefault(r, set()).update(pop)
            chk.ok('TOUR-G couverture par les boules p+q<=k', cov == cov2, '%s k=%d niveau %s' % (label, k, level))
            prev = new
        # TOUR-D : descentes valides au hasard
        final = graph(levels[-1], False)
        chk.ok('INV-RACINE', len(set(final.values())) == 1, '%s k=%d' % (label, k))
        if k >= 1:
            starts = list(beta_k)
            if len(starts) > 12:
                starts = rnd.sample(starts, 12)
            for f0 in starts:
                f = f0
                at_start = graph(beta_k[f0], False)
                steps = 0
                while True:
                    lv, c = X.meb(f)
                    if lv == 0:
                        break
                    inner, shell, q = info[(c, lv)]
                    p = len(inner)
                    if p >= k:
                        nxt = tuple(sorted(rnd.sample(inner, k)))
                    else:
                        tsep = [a for a in combinations(shell, k - p) if not X.in_hull(c, a)]
                        if not tsep:
                            break
                        nxt = tuple(sorted(inner + rnd.choice(tsep)))
                    chk.ok('TOUR-D decroissance stricte', beta_k[nxt] < lv, label)
                    chk.ok('TOUR-D meme composante au niveau de depart', at_start[nxt] == at_start[f0], label)
                    f = nxt
                    steps += 1
                chk.tally('descentes')
                chk.tally('pas de descente', steps)


def families(rnd, count, nmax):
    out = []
    circle25 = [(x + 5, y + 5, 3) for x in range(-5, 6) for y in range(-5, 6) if x * x + y * y == 25]
    sphere9 = [(x + 3, y + 3, z + 3) for x in range(-3, 4) for y in range(-3, 4) for z in range(-3, 4)
               if x * x + y * y + z * z == 9]
    sphere14 = [(x + 4, y + 4, z + 4) for x in range(-4, 5) for y in range(-4, 5) for z in range(-4, 5)
                if x * x + y * y + z * z == 14]
    kinds = ('generic', 'grid3', 'grid4', 'coplanar', 'collinear', 'cocircular', 'cospherical9', 'cospherical14',
             'clusters')
    for i in range(count):
        kind = kinds[i % len(kinds)]
        n = rnd.randint(4, nmax)
        pts = set()
        guard = 0
        while len(pts) < n and guard < 10000:
            guard += 1
            if kind == 'generic':
                pts.add((rnd.randint(0, 30), rnd.randint(0, 30), rnd.randint(0, 30)))
            elif kind == 'grid3':
                pts.add((rnd.randint(0, 2), rnd.randint(0, 2), rnd.randint(0, 2)))
            elif kind == 'grid4':
                pts.add((rnd.randint(0, 3), rnd.randint(0, 3), rnd.randint(0, 3)))
            elif kind == 'coplanar':
                pts.add((rnd.randint(0, 5), rnd.randint(0, 5), 2))
            elif kind == 'collinear':
                pts.add((rnd.randint(0, 12), 0, 0))
            elif kind == 'cocircular':
                pts.add(rnd.choice(circle25) if rnd.random() < 0.85 else (rnd.randint(0, 10), rnd.randint(0, 10), 3))
            elif kind == 'cospherical9':
                pts.add(rnd.choice(sphere9) if rnd.random() < 0.85 else
                        (rnd.randint(0, 6), rnd.randint(0, 6), rnd.randint(0, 6)))
            elif kind == 'cospherical14':
                pts.add(rnd.choice(sphere14) if rnd.random() < 0.9 else
                        (rnd.randint(0, 8), rnd.randint(0, 8), rnd.randint(0, 8)))
            else:
                pts.add(tuple(rnd.choice((0, 3, 6)) + rnd.randint(0, 1) for _ in range(3)))
        out.append(('%s_%d' % (kind, i), sorted(pts)))
    return out


def weighted_families(rnd, count, nmax):
    circle25 = [(x + 5, y + 5, 3) for x in range(-5, 6) for y in range(-5, 6) if x * x + y * y == 25]
    kinds = ('w_generic', 'w_grid3', 'w_collinear', 'w_cocircular', 'w_coplanar')
    out = []
    for i in range(count):
        kind = kinds[i % len(kinds)]
        ns = rnd.randint(2, max(2, nmax - 2))
        sites = set()
        while len(sites) < ns:
            if kind == 'w_generic':
                sites.add((rnd.randint(0, 12), rnd.randint(0, 12), rnd.randint(0, 12)))
            elif kind == 'w_grid3':
                sites.add((rnd.randint(0, 2), rnd.randint(0, 2), rnd.randint(0, 2)))
            elif kind == 'w_collinear':
                sites.add((rnd.randint(0, 9), 0, 0))
            elif kind == 'w_cocircular':
                sites.add(rnd.choice(circle25))
            else:
                sites.add((rnd.randint(0, 4), rnd.randint(0, 4), 1))
        pts = sorted(sites)
        while len(pts) < nmax and (len(pts) == len(sites) or rnd.random() < 0.6):
            pts.append(rnd.choice(sorted(sites)))
        out.append(('%s_%d' % (kind, i), sorted(pts)))
    return out


FIXTURES_W = (
    ('paire_31', [(12, 12, 12)] * 3 + [(14, 12, 12)]),
    ('triangle_311', [(0, 0, 0)] * 3 + [(6, 0, 0), (3, 5, 0)]),
    ('triangle_211', [(0, 0, 0)] * 2 + [(6, 0, 0), (3, 5, 0)]),
    ('carre_pondere', [(0, 0, 0), (2, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0), (1, 1, 5), (1, 1, 5)]),
    ('site_de_poids_4', [(5, 5, 5)] * 4),
    ('paire_33', [(1, 1, 1)] * 3 + [(3, 1, 1)] * 3),
)

FIXTURES = (
    ('carre', [(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)]),
    ('triangle_rectangle', [(0, 0, 0), (3, 0, 0), (0, 4, 0)]),
    ('octaedre', [(15, 10, 10), (5, 10, 10), (10, 15, 10), (10, 5, 10), (10, 10, 15), (10, 10, 5)]),
    ('tetraedre_et_centre', [(0, 0, 0), (2, 2, 0), (2, 0, 2), (0, 2, 2), (1, 1, 1)]),
    ('e5', [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)]),
    ('these_th5_plan', [(0, 100, 0), (200, 100, 0), (101, 10, 0), (130, 15, 0), (103, 400, 0)]),
    ('gain_de_couverture', [(8, 9, 0), (5, 10, 0), (2, 9, 0), (5, 0, 0)]),
    ('ligne_024', [(0, 0, 0), (2, 0, 0), (4, 0, 0)]),
    ('ligne_01269', [(0, 0, 0), (1, 0, 0), (2, 0, 0), (6, 0, 0), (9, 0, 0)]),
    ('firstcov_k3_n6', [(2, 4, 4), (2, 8, 5), (2, 9, 1), (3, 7, 0), (6, 9, 6), (8, 10, 7)]),
    ('internal_k3', [(15, 4, 0), (5, 4, 0), (7, 8, 0), (7, 0, 0), (1, 4, 0), (0, 4, 1)]),
    ('cube', [(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, 2)]),
    ('ball_anchors', [(2, 2, 2), (2, 0, 0), (0, 2, 0), (0, 0, 2), (0, 0, 0)]),
)


def main():
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 20261002
    count = int(sys.argv[2]) if len(sys.argv) > 2 else 45
    nmax = int(sys.argv[3]) if len(sys.argv) > 3 else 7
    weighted = len(sys.argv) > 4 and sys.argv[4] == 'copies'
    rnd = random.Random(seed)
    chk = Check()
    if weighted:
        fixed = list(FIXTURES_W)
        clouds = fixed + weighted_families(rnd, count, nmax)
    else:
        fixed = list(FIXTURES)
        clouds = fixed + families(rnd, count, nmax)
    for label, pts in clouds:
        check_cloud(pts, chk, rnd, label)
    print('%s : nuages %d (fixtures %d, aleatoires %d, n <= %d), graine %d'
          % ('copies (positions repetees)' if weighted else 'sites distincts', len(clouds), len(fixed), count,
             max(len(p) for _l, p in clouds), seed))
    for name in sorted(chk.count):
        print('  %-62s %8d' % (name, chk.count[name]))
    print('ecarts : %d' % len(chk.fails))
    for name, detail in chk.fails[:20]:
        print('  ECART %s : %s' % (name, detail))
    return 1 if chk.fails else 0


if __name__ == '__main__':
    sys.exit(main())
