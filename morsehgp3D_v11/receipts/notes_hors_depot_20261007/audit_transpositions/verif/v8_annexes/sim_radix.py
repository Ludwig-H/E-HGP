"""Simulation legere (modele, jamais le moteur) du census sature de l'index v11 sur une trame sans sol.

Usage : python3 -B v8_census_bounds_sim.py TRAME.u32le REQUETES VOISINAGE K [graine]

Proxy des census de descente : MEB d'une partie de K sites tiree dans {s} u kNN(s), gardee seulement si
p + q > K + 1 (boule hors CatK : la table rate, la v11 appelle le census au seuil K).
Variantes (borne x index x ordre) :
  A  borne v11 (extrema separes quadratique/lineaire, src/num/predicates.cpp bound_terms), plages Morton, preordre ;
  B  borne exacte sur reseau (v8 PreparedPower : sommet arrondi puis serre, coin le plus eloigne), plages Morton ;
  C  borne exacte, plages Morton, ordre v8 « plus petit minimum d'abord » (pile, deux enfants bornes) ;
  E  borne v11, arbre k-d a coupe mediane sur le plus long cote (feuilles <= 8), preordre ;
  F  borne exacte, arbre k-d a coupe mediane, preordre ;
  G  borne exacte, arbre a coupe au milieu geometrique du plus long cote (index v8), preordre.
Flottant double : estimation de comptes de visites, jamais une decision exacte ; comptes_ok compare p a la
force brute (les rares ecarts viennent de la tolerance flottante).
"""
import itertools
import sys
import time

import numpy as np
from scipy.spatial import cKDTree

PATH = sys.argv[1]
QUERIES = int(sys.argv[2])
NEIGH = int(sys.argv[3])
K = int(sys.argv[4])
SEED = int(sys.argv[5]) if len(sys.argv) > 5 else 20261004
LEAF = 8
rng = np.random.default_rng(SEED)
sys.setrecursionlimit(10000)

raw = np.fromfile(PATH, dtype='<u4').reshape(-1, 3).astype(np.int64)
n = len(raw)


def morton(p):
    key = 0
    for bit in range(21):
        for axis in range(3):
            key |= ((int(p[axis]) >> bit) & 1) << (3 * bit + axis)
    return key


class Tree:
    """Arbre en preordre : boites exactes, plage, echappement, enfants."""

    def __init__(self, order, split):
        self.pts = raw[order]
        self.nodes = []
        self.split = split
        self.build(0, n)
        self.P = self.pts.astype(np.float64)  # apres la construction : les coupes k-d reordonnent pts
        self.LO = np.array([nd[0] for nd in self.nodes], dtype=np.float64)
        self.HI = np.array([nd[1] for nd in self.nodes], dtype=np.float64)
        self.B = [nd[2] for nd in self.nodes]
        self.E = [nd[3] for nd in self.nodes]
        self.ESC = [nd[4] for nd in self.nodes]
        self.L = [nd[5] for nd in self.nodes]
        self.R = [nd[6] for nd in self.nodes]

    def build(self, begin, end):
        here = len(self.nodes)
        self.nodes.append(None)
        sl = self.pts[begin:end]
        lo, hi = sl.min(0), sl.max(0)
        if end - begin <= LEAF:
            self.nodes[here] = [lo, hi, begin, end, here + 1, -1, -1]
            return
        mid = self.split(self, begin, end, lo, hi)
        left = len(self.nodes)
        self.build(begin, mid)
        right = len(self.nodes)
        self.build(mid, end)
        self.nodes[here] = [lo, hi, begin, end, len(self.nodes), left, right]


def split_morton(tree, begin, end, lo, hi):
    return begin + (end - begin) // 2


def split_kd_median(tree, begin, end, lo, hi):
    axis = int(np.argmax(hi - lo))
    seg = tree.pts[begin:end]
    idx = np.argsort(seg[:, axis], kind='stable')
    tree.pts[begin:end] = seg[idx]
    return begin + (end - begin) // 2


def split_kd_middle(tree, begin, end, lo, hi):
    axis = int(np.argmax(hi - lo))
    seg = tree.pts[begin:end]
    idx = np.argsort(seg[:, axis], kind='stable')
    seg = seg[idx]
    tree.pts[begin:end] = seg
    cut = (lo[axis] + hi[axis]) / 2.0
    mid = begin + int(np.searchsorted(seg[:, axis], cut, side='right'))
    if mid <= begin or mid >= end:
        mid = begin + (end - begin) // 2
    return mid


KEYS_ALL = [morton(raw[i]) for i in range(n)]
morton_order = sorted(range(n), key=lambda i: KEYS_ALL[i])
SKEYS = [KEYS_ALL[i] for i in morton_order]


def split_radix(tree, begin, end, lo, hi):
    a, b = SKEYS[begin], SKEYS[end - 1]
    bit = (a ^ b).bit_length() - 1
    lo_i, hi_i = begin, end - 1
    while lo_i < hi_i:
        m = (lo_i + hi_i) // 2
        if (SKEYS[m] >> bit) & 1:
            hi_i = m
        else:
            lo_i = m + 1
    return lo_i


trees = {
    'radix': Tree(morton_order, split_radix),
    'morton': Tree(morton_order, split_morton),
    'kd_median': Tree(list(range(n)), split_kd_median),
    'kd_middle': Tree(list(range(n)), split_kd_middle),
}
P = raw.astype(np.float64)
kdt = cKDTree(P)


def circumcenter(S):
    a = S[0]
    if len(S) == 2:
        return (S[0] + S[1]) / 2
    M = S[1:] - a
    try:
        t = np.linalg.solve(M @ M.T, 0.5 * np.einsum('ij,ij->i', M, M))
    except np.linalg.LinAlgError:
        return None
    return a + M.T @ t


def meb(part):
    best = None
    for size in (2, 3, 4):
        for sub in itertools.combinations(range(len(part)), size):
            S = part[list(sub)]
            c = circumcenter(S)
            if c is None:
                continue
            r2 = float(((S[0] - c) ** 2).sum())
            if np.all(((part - c) ** 2).sum(1) <= r2 * (1 + 1e-12) + 1e-6):
                if best is None or r2 < best[1]:
                    best = (c, r2, sub)
    return best


def bounds_v11(c, r2, anchor, lo, hi):
    w = c - anchor
    vl, vh = lo - anchor, hi - anchor
    lower = upper = 0.0
    for j in range(3):
        l2, h2 = vl[j] * vl[j], vh[j] * vh[j]
        lower += l2 if vl[j] > 0 else (h2 if vh[j] < 0 else 0.0)
        upper += max(l2, h2)
        if w[j] >= 0:
            lower -= 2 * w[j] * vh[j]
            upper -= 2 * w[j] * vl[j]
        else:
            lower -= 2 * w[j] * vl[j]
            upper -= 2 * w[j] * vh[j]
    return lower, upper


def bounds_exact(c, r2, anchor, lo, hi):
    lower = upper = -r2
    for j in range(3):
        x = min(max(np.rint(c[j]), lo[j]), hi[j])
        lower += (x - c[j]) ** 2
        upper += max((lo[j] - c[j]) ** 2, (hi[j] - c[j]) ** 2)
    return lower, upper


EPS = 1e-6


def inside(T, c, r2, i):
    return float(((T.P[i] - c) ** 2).sum()) - r2 < -EPS * r2


def walk_preorder(T, c, r2, anchor, fn):
    p = bounds = tests = 0
    cur, N = 0, len(T.nodes)
    while cur < N and p < K:
        bounds += 1
        lower, upper = fn(c, r2, anchor, T.LO[cur], T.HI[cur])
        if lower > 0:
            cur = T.ESC[cur]
        elif upper < 0:
            p += min(T.E[cur] - T.B[cur], K - p)
            cur = T.ESC[cur]
        elif T.L[cur] < 0:
            for i in range(T.B[cur], T.E[cur]):
                if p >= K:
                    break
                tests += 1
                p += inside(T, c, r2, i)
            cur = T.ESC[cur]
        else:
            cur += 1
    return p, bounds, tests


def walk_minfirst(T, c, r2, anchor, fn):
    p = tests = 0
    lower, upper = fn(c, r2, anchor, T.LO[0], T.HI[0])
    bounds = 1
    stack = [(0, lower, upper)]
    while stack and p < K:
        cur, lower, upper = stack.pop()
        if lower > 0:
            continue
        if upper < 0:
            p += min(T.E[cur] - T.B[cur], K - p)
            continue
        if T.L[cur] < 0:
            for i in range(T.B[cur], T.E[cur]):
                if p >= K:
                    break
                tests += 1
                p += inside(T, c, r2, i)
            continue
        kids = []
        for ch in (T.L[cur], T.R[cur]):
            lo2, up2 = fn(c, r2, anchor, T.LO[ch], T.HI[ch])
            bounds += 1
            kids.append((ch, lo2, up2))
        kids.sort(key=lambda t: t[1], reverse=True)
        stack.extend(kids)
    return p, bounds, tests


VARIANTS = (('A', 'morton', walk_preorder, bounds_v11), ('B', 'morton', walk_preorder, bounds_exact),
            ('R', 'radix', walk_preorder, bounds_v11), ('S', 'radix', walk_preorder, bounds_exact),
            ('C', 'morton', walk_minfirst, bounds_exact), ('E', 'kd_median', walk_preorder, bounds_v11),
            ('F', 'kd_median', walk_preorder, bounds_exact), ('G', 'kd_middle', walk_preorder, bounds_exact))
stats = {(v[0], s): [0, 0, 0, 0] for v in VARIANTS for s in (True, False)}
kept = sat = attempts = 0
t0 = time.time()
while kept < QUERIES and attempts < 50 * QUERIES:
    attempts += 1
    s = int(rng.integers(n))
    _, nn = kdt.query(P[s], k=NEIGH + 1)
    chosen = [s] + list(rng.choice([int(i) for i in nn[1:]], size=K - 1, replace=False))
    part = P[chosen]
    best = meb(part)
    if best is None:
        continue
    c, r2, sub = best
    pin = sum(1 for i in kdt.query_ball_point(c, np.sqrt(r2) * (1 - 1e-9))
              if float(((P[i] - c) ** 2).sum()) - r2 < -EPS * r2)
    if pin + len(sub) <= K + 1:
        continue
    kept += 1
    saturated = pin >= K
    sat += saturated
    anchor = part[sub[0]]
    for name, tree, walk, fn in VARIANTS:
        p, b, t = walk(trees[tree], c, r2, anchor, fn)
        st = stats[(name, saturated)]
        st[0] += b
        st[1] += t
        st[2] += 1
        st[3] += min(p, K) == min(pin, K)
print('noeuds radix', len(trees['radix'].nodes))
print('sites', n, 'noeuds morton/kd_median/kd_middle', len(trees['morton'].nodes), len(trees['kd_median'].nodes),
      len(trees['kd_middle'].nodes), 'requetes', kept, 'saturees', sat, 'voisinage', NEIGH, 'K', K,
      'secondes', round(time.time() - t0, 1))
for name, tree, _, _ in VARIANTS:
    b = sum(stats[(name, s)][0] for s in (True, False))
    t = sum(stats[(name, s)][1] for s in (True, False))
    ok = sum(stats[(name, s)][3] for s in (True, False))
    line = '%s %-9s bornes/requete %6.1f tests/requete %6.1f comptes_ok %d' % (name, tree, b / kept, t / kept, ok)
    for s, label in ((True, 'sature'), (False, 'complet')):
        bb, tt, cc, _ = stats[(name, s)]
        if cc:
            line += ' | %s %d : %.1f / %.1f' % (label, cc, bb / cc, tt / cc)
    print(line)
