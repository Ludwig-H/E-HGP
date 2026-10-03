#!/usr/bin/env python3
"""Outils exacts pour juger la fermeture qualifiee de l'auditeur (label « fermeture », 3 octobre 2026).

Lecture seule du worktree : on importe l'oracle de la definition (reference/hgp11_ref, etage A) et la route
oracle des regles fideles (bench/points_reference.py). Aucun fichier n'est ecrit hors de ce repertoire
(sys.dont_write_bytecode). Niveaux = rayons carres exacts (Fraction), coupes fermees.

Objets calcules ici, tous a partir des coupes fermees de Gamma_k publiees par l'oracle :
  closure(res, n, m)        fermeture des couvertures qualifiees (>= m sites) : ultrametrique u, entrees, echeances w
  closure_fresh(res, n, m)  meme partition recalculee coupe par coupe sans memoire (verifie la monotonie)
  hl_direct(defn, n, kp)    liaison simple de w_kp(x, y) = min{beta(F) : |F| = kp, {x, y} inclus dans F}
  mr2(defn, n, k)           atteignabilite mutuelle de HDBSCAN au carre : max(D_k(x), D_k(y), |x - y|^2)
  ec_hanging(res, n, m)     pendaison « fermeture exclusive » : entree a la premiere couverture qualifiee unique
  parasitic_events(...)     reunions de la fermeture avant la fusion FULL, par noeud de fusion et paire d'enfants
"""
from fractions import Fraction
from itertools import combinations
import math
import sys

sys.dont_write_bytecode = True
REF = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/reference'
BENCH = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench'
for p in (REF, BENCH):
    if p not in sys.path:
        sys.path.insert(0, p)

from hgp11_ref import Definition  # noqa: E402
import points_reference as pr  # noqa: E402


def popcount(x):
    return bin(x).count('1')


def sites_of(mask):
    out, i = [], 0
    while mask:
        if mask & 1:
            out.append(i)
        mask >>= 1
        i += 1
    return out


class DSU(object):
    def __init__(self, n):
        self.p = list(range(n))

    def find(self, x):
        p = self.p
        r = x
        while p[r] != r:
            r = p[r]
        while p[x] != r:
            p[x], x = r, p[x]
        return r

    def union(self, a, b):
        ra, rb = self.find(a), self.find(b)
        if ra != rb:
            self.p[rb] = ra


# ----------------------------------------------------------------------------------------------- fermeture

def closure(res, n, m):
    """Fermeture d'equivalence des couvertures qualifiees, cumulee sur les coupes fermees.

    Rend dict(u, w, entries, levels) : u[i][j] premier niveau ou i et j sont dans un meme bloc (u[i][i] = entree),
    w[i][j] premier niveau ou une meme couverture qualifiee contient i et j (w[i][i] = entree)."""
    dsu = DSU(n)
    active = [False] * n
    u = [[None] * n for _ in range(n)]
    w = [[None] * n for _ in range(n)]
    levels = []
    for cut in res.cuts:
        a = cut.level
        levels.append(a)
        edges = [cov for (_v, cov, _c) in cut.closed if popcount(cov) >= m]
        for cov in edges:
            s = sites_of(cov)
            for i in s:
                active[i] = True
            for i in s[1:]:
                dsu.union(s[0], i)
            for x in range(len(s)):
                for y in range(x, len(s)):
                    i, j = s[x], s[y]
                    if w[i][j] is None:
                        w[i][j] = w[j][i] = a
        for i in range(n):
            if not active[i]:
                continue
            if u[i][i] is None:
                u[i][i] = a
            ri = dsu.find(i)
            for j in range(i + 1, n):
                if u[i][j] is None and active[j] and dsu.find(j) == ri:
                    u[i][j] = u[j][i] = a
    for i in range(n):
        for j in range(n):
            if u[i][j] is None or w[i][j] is None:
                raise RuntimeError('fermeture incomplete (racine sans couverture totale ?)')
    return dict(u=u, w=w, entries=[u[i][i] for i in range(n)], levels=levels)


def partition_of_cut(cut, n, m):
    """Partition (blocs actifs) d'une seule coupe, sans memoire des coupes precedentes."""
    dsu = DSU(n)
    active = set()
    for (_v, cov, _c) in cut.closed:
        if popcount(cov) < m:
            continue
        s = sites_of(cov)
        active.update(s)
        for i in s[1:]:
            dsu.union(s[0], i)
    blocks = {}
    for i in sorted(active):
        blocks.setdefault(dsu.find(i), []).append(i)
    return frozenset(frozenset(b) for b in blocks.values())


def refines(p, q):
    return all(any(b <= c for c in q) for b in p)


def blocks_from_u(u, level):
    return frozenset(pr.blocks_at(u, level))


def minmax_closure(w, n):
    """Ultrametrique sous-dominante (liaison simple) de w hors diagonale ; diagonale = min des aretes."""
    u = [row[:] for row in w]
    for z in range(n):
        for i in range(n):
            uiz = u[i][z]
            for j in range(n):
                v = uiz if uiz > u[z][j] else u[z][j]
                if v < u[i][j]:
                    u[i][j] = v
    return u


def offdiag_min(w, n):
    return [min(w[i][j] for j in range(n) if j != i) for i in range(n)]


# ----------------------------------------------------------------------------------------- formes directes

def w_direct(defn, n, kp):
    """w_kp(i, j) = min des beta(F) sur les kp-parties F contenant i et j (sans la connexite de FULL)."""
    w = [[None] * n for _ in range(n)]
    for i in range(n):
        for j in range(i + 1, n):
            others = [x for x in range(n) if x not in (i, j)]
            best = None
            for extra in combinations(others, kp - 2):
                b = defn.beta((i, j) + extra)
                if best is None or b < best:
                    best = b
            w[i][j] = w[j][i] = best
    for i in range(n):
        w[i][i] = min(w[i][j] for j in range(n) if j != i)
    return w


def hl_direct(defn, n, kp):
    """Liaison simple de w_kp, entrees A_kp(x) = min_j w_kp(x, j)."""
    w = w_direct(defn, n, kp)
    u = minmax_closure(w, n)
    for i in range(n):
        u[i][i] = w[i][i]
    return u, w


def mr2(defn, n, k):
    """Atteignabilite mutuelle au carre (sklearn, min_samples = k, le point compte) : max(D_k(x), D_k(y), d^2)."""
    dk = [Fraction(defn.nearest(x)[k - 1][0]) for x in range(n)]
    out = [[None] * n for _ in range(n)]
    pts = defn.points
    for i in range(n):
        out[i][i] = dk[i]
        for j in range(i + 1, n):
            d2 = sum((a - b) ** 2 for a, b in zip(pts[i], pts[j]))
            out[i][j] = out[j][i] = max(dk[i], dk[j], Fraction(d2))
    return out, dk


# --------------------------------------------------------------------------------------------- pendaisons

def faithful_rules(res, n, m):
    """Regles fideles de la route oracle du developpeur : core, cover, first, margin1 (H_1), margin (H_m)."""
    ref, tree = pr.reference_rules(res, n, m)
    return ref, tree


def ec_hanging(res, n, m):
    """Fermeture exclusive : le site entre au premier niveau ou exactement UNE composante vivante qualifiee le
    couvre, dans cette composante ; puis il suit ses ancetres (pendaison fidele, sans choix)."""
    entries = [None] * n
    for cut in res.cuts:
        cov_by_site = [[] for _ in range(n)]
        for (v, cov, _c) in cut.closed:
            if popcount(cov) < m:
                continue
            for i in sites_of(cov):
                cov_by_site[i].append(v)
        for i in range(n):
            if entries[i] is None and len(cov_by_site[i]) == 1:
                entries[i] = (cut.level, cov_by_site[i][0])
    if any(e is None for e in entries):
        raise RuntimeError('ec : site jamais couvert de facon exclusive')
    return entries


def ultrametric_of(entries, tree):
    return pr.reference_ultrametric(entries, tree)


# ------------------------------------------------------------------------------------- stabilite exacte

def within_radius(a, b, eps2, factor=1):
    """|sqrt(a) - sqrt(b)| <= factor * sqrt(eps2), exactement (a, b, eps2 >= 0 rationnels)."""
    e2 = Fraction(eps2) * factor * factor
    s = a + b - e2
    if s <= 0:
        return True
    return s * s <= 4 * a * b


def radius_gap(a, b):
    return abs(math.sqrt(a) - math.sqrt(b))


# -------------------------------------------------------------------------------- reunions parasites

def subtree_sets(nodes):
    """Pour chaque noeud, l'ensemble de ses descendants (lui compris)."""
    children = [list(nd.children) for nd in nodes]
    desc = [None] * len(nodes)

    def rec(v):
        if desc[v] is None:
            s = {v}
            for c in children[v]:
                s |= rec(c)
            desc[v] = s
        return desc[v]
    for v in range(len(nodes)):
        rec(v)
    return desc


def parasitic_events(res, n, m, k):
    """Reunions de la fermeture (seuil m) entre deux lignees d'enfants d'une fusion FULL, AVANT cette fusion.

    Pour chaque noeud de fusion nu (niveau h) et chaque paire d'enfants (s, t) : U = premier niveau de coupe < h ou
    deux noeuds vivants qualifies, l'un dans le sous-arbre de s, l'autre dans celui de t, ont leurs couvertures dans
    un meme bloc de la fermeture. Couverture de lignee = reunion des couvertures de TOUS les noeuds vivants du
    sous-arbre. Rend une liste d'evenements (dict)."""
    nodes = res.nodes
    desc = subtree_sets(nodes)
    merges = [(v, nd.level, list(nd.children)) for v, nd in enumerate(nodes) if nd.children]
    dsu = DSU(n)
    state = {}
    for (v, h, kids) in merges:
        for x in range(len(kids)):
            for y in range(x + 1, len(kids)):
                state[(v, kids[x], kids[y])] = dict(node=v, h=h, s=kids[x], t=kids[y], U=None)
    for cut in res.cuts:
        a = cut.level
        alive = list(cut.closed)
        for (_v, cov, _c) in alive:
            if popcount(cov) >= m:
                s = sites_of(cov)
                for i in s[1:]:
                    dsu.union(s[0], i)
        for key, st in state.items():
            v, s, t = key
            if not a < st['h']:
                continue
            cs = ct = 0
            bs, bt = set(), set()
            for (x, cov, _c) in alive:
                if x in desc[s]:
                    cs |= cov
                    if popcount(cov) >= m:
                        bs.add(dsu.find(sites_of(cov)[0]))
                elif x in desc[t]:
                    ct |= cov
                    if popcount(cov) >= m:
                        bt.add(dsu.find(sites_of(cov)[0]))
            if not cs or not ct:
                continue
            xs, xt = popcount(cs & ~ct), popcount(ct & ~cs)
            if st['U'] is None and bs & bt:
                st['U'] = a
                st['direct'] = bool(cs & ct)
                st['excl_at_U'] = (xs, xt)
                st['shared_at_U'] = popcount(cs & ct)
                st['maxmin_excl'] = min(xs, xt)
                st['sizes_at_U'] = (popcount(cs), popcount(ct))
            elif st['U'] is not None:
                st['maxmin_excl'] = max(st['maxmin_excl'], min(xs, xt))
    events = []
    thr = max(m, k + 1)
    for st in state.values():
        if st['U'] is None:
            continue
        st['ratio_sq'] = st['U'] / st['h']
        st['ratio_r'] = math.sqrt(st['U'] / st['h'])
        st['two_whole'] = st['maxmin_excl'] >= thr
        st['two_whole_at_U'] = min(st['excl_at_U']) >= thr
        if st['maxmin_excl'] == 0:
            st['kind'] = 'absorption'
        elif st['two_whole']:
            st['kind'] = 'deux_amas_entiers'
        else:
            st['kind'] = 'site_partage'
        events.append(st)
    return events


# ------------------------------------------------------------------------------------- fraction ch. 7

def fraction_before_merge_laminar(u, seed_a, seed_b, group_a):
    """Fraction de group_a dans le bloc de seed_a juste avant sa reunion avec seed_b (coupe ouverte)."""
    merge = u[seed_a][seed_b]
    block = [j for j in range(len(u)) if u[seed_a][j] < merge and u[j][j] < merge]
    return merge, Fraction(len([j for j in block if j in group_a]), len(group_a))


def full_fraction_before_merge(res, n, seed_a, seed_b, group_a, k):
    """FULL (semantique de couverture Theta^poly) : couverture de la composante du coeur de seed_a juste avant sa
    fusion avec celle du coeur de seed_b ; rend (niveau de fusion, fraction de group_a couverte)."""
    tree = pr.Tree(res.nodes)
    va, vb = res.core[seed_a].nodes, res.core[seed_b].nodes
    w = tree.lca(va, vb)
    merge = max(res.nodes[w].level, res.core[seed_a].level, res.core[seed_b].level)
    best = 0
    for cut in res.cuts:
        if cut.level >= merge:
            break
        if cut.level < res.core[seed_a].level:
            continue
        anc = tree.alive_ancestor(va, cut.level)
        for (x, cov, _c) in cut.closed:
            if x == anc:
                best = max(best, len([j for j in group_a if cov >> j & 1]))
    return merge, Fraction(best, len(group_a))


# ------------------------------------------------------------------------------------- generateurs

def cloud_gate(rng):
    """Nuages a ex aequo frequents, comme bench/points_gate.py (4 a 9 sites)."""
    n = rng.randint(4, 9)
    side = rng.choice([3, 4, 6, 10, 40])
    scale = rng.choice([1, 7, 1000])
    pts = set()
    while len(pts) < n:
        pts.add((rng.randrange(side) * scale, rng.randrange(side) * scale,
                 rng.choice([0, rng.randrange(side) * scale])))
    pts = sorted(pts)
    rng.shuffle(pts)
    return pts


def cloud_generic(rng, nmin=5, nmax=9, box=1000):
    """Position generale presque sure : coordonnees entieres uniformes dans [0, box)^3."""
    n = rng.randint(nmin, nmax)
    pts = set()
    while len(pts) < n:
        pts.add(tuple(rng.randrange(box) for _ in range(3)))
    pts = sorted(pts)
    rng.shuffle(pts)
    return pts


def cloud_two_blobs(rng):
    """Deux amas serres (3 ou 4 sites, cube de cote t) separes de L, 1 ou 2 sites de vallee entre eux.

    Rend (points, A, B, V) avec A, B, V les indices des amas et de la vallee."""
    na, nb = rng.choice([3, 4]), rng.choice([3, 4])
    nv = rng.choice([1, 2])
    t = rng.randint(80, 200)
    big = rng.randint(900, 1500)
    pts, groups = [], ([], [], [])
    used = set()

    def add(p, g):
        if p in used:
            return False
        used.add(p)
        groups[g].append(len(pts))
        pts.append(p)
        return True
    for g, (cx, cnt) in enumerate(((0, na), (big, nb))):
        while len(groups[g]) < cnt:
            add((cx + rng.randrange(t), rng.randrange(t), rng.randrange(t)), g)
    while len(groups[2]) < nv:
        add((rng.randrange(t, big), rng.randrange(-t, 2 * t), rng.randrange(-t, 2 * t)), 2)
    return pts, groups[0], groups[1], groups[2]
