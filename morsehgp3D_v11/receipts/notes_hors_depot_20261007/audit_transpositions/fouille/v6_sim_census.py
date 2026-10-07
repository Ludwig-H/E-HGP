#!/usr/bin/env python3
"""Fouille v6 -> v11, idee v6-I1 : ESTIMATION (pas une mesure du moteur) du parcours census de l'index global v11
sur les sites reels sans sol d'une trame du contrat, avec deux bornes de la puissance sur une boite :

  * separee  = celle de la v11 : extrema du terme quadratique et du terme lineaire pris separement par axe
               (morsehgp3D_v11/src/num/predicates.cpp l. 104-118 bound_terms, l. 263-275 power_bound_signs ;
               commentaire l. 144 : « Separer les extrema peut elargir l'intervalle ») ;
  * exacte   = celle de la v6 : min de la parabole convexe de chaque axe au sommet ecrete, max aux bornes
               (morsehgp3D_v6/src/pipeline/census.hpp l. 61-84, AxisBounds) ; forme de reseau proposee pour la v11
               par receipts/audit_independant_20261002/index_lattice_bounds_review_10/README.md.

Index : tri Morton des sites, mediane des rangs, feuilles <= 8, noeuds en ordre prefixe avec echappement
(v11 src/index/build.cpp l. 54-71 ; parcours src/index/census_workspace.cpp l. 47-65 ; IndexParams{} = 8).
Requetes : MEB (flottant, enumeration des sous-ensembles de 2..4 points) d'une partie de k sites tiree parmi les m plus
proches voisins d'un site, seuil k (locate.cpp l. 86 : query(..., meb.sphere(), k, ...)). Proxy des parties des
descentes : ce ne sont PAS les vraies parties. Le binaire64 sert a compter noeuds et tests, jamais a decider.

Usage : python3 -B v6_sim_census.py <trame ng00|ng01|ng02> <k> <m> <requetes>
Resultats graves dans fouille/v6.md, annexe A (graine 3 ; 4 octobre 2026, codespace, un coeur, quelques secondes).
"""
import itertools
import random
import sys

import numpy as np
from scipy.spatial import cKDTree

DATA = '/workspaces/E-HGP/build/v11-full-data-20261002/lidar_{}.u32le'
LEAF = 8


def part1by2(x):
    x = x.astype(np.uint64) & np.uint64(0x1fffff)
    x = (x | (x << np.uint64(32))) & np.uint64(0x1f00000000ffff)
    x = (x | (x << np.uint64(16))) & np.uint64(0x1f0000ff0000ff)
    x = (x | (x << np.uint64(8))) & np.uint64(0x100f00f00f00f00f)
    x = (x | (x << np.uint64(4))) & np.uint64(0x10c30c30c30c30c3)
    x = (x | (x << np.uint64(2))) & np.uint64(0x1249249249249249)
    return x


def build(pts):
    lo, hi, beg, end, esc = [], [], [], [], []

    def visit(b, e):
        here = len(lo)
        lo.append(None); hi.append(None); beg.append(b); end.append(e); esc.append(None)
        if e - b <= LEAF:
            sub = pts[b:e]
            lo[here] = tuple(int(v) for v in sub.min(0)); hi[here] = tuple(int(v) for v in sub.max(0))
            esc[here] = here + 1
            return
        mid = b + (e - b) // 2
        visit(b, mid)
        right = len(lo)
        visit(mid, e)
        lo[here] = tuple(min(lo[here + 1][j], lo[right][j]) for j in range(3))
        hi[here] = tuple(max(hi[here + 1][j], hi[right][j]) for j in range(3))
        esc[here] = len(lo)

    sys.setrecursionlimit(10000)
    visit(0, len(pts))
    return lo, hi, beg, end, esc


def meb(P):
    best = None
    k = len(P)
    for s in range(2, min(4, k) + 1):
        for sub in itertools.combinations(range(k), s):
            Q = P[list(sub)]
            a = Q[0]
            if s == 2:
                c = (Q[0] + Q[1]) / 2.0
            else:
                A = Q[1:] - a
                try:
                    y = np.linalg.solve(A @ A.T, (A * A).sum(1) / 2.0)
                except np.linalg.LinAlgError:
                    continue
                c = a + A.T @ y
            r2 = ((Q - c) ** 2).sum(1).max()
            if (((P - c) ** 2).sum(1) <= r2 * (1 + 1e-12) + 1e-9).all():
                if best is None or r2 < best[1]:
                    best = (c, r2, list(sub))
    return best


def walk(tree, X, Y, Z, c, r2, anchor, thr, exact):
    lo, hi, beg, end, esc = tree
    w = (c[0] - anchor[0], c[1] - anchor[1], c[2] - anchor[2])
    tol = 1e-7 * max(r2, 1.0)
    p = nodes = tests = cur = 0
    while cur < len(lo) and p < thr:
        nodes += 1
        L, H = lo[cur], hi[cur]
        low = up = 0.0
        for j in range(3):
            l, h, wj = L[j] - anchor[j], H[j] - anchor[j], w[j]
            if exact:  # v6 AxisBounds : parabole v^2 - 2 w v, minimum au sommet ecrete, maximum aux bornes
                t = min(max(wj, l), h)
                low += t * t - 2 * wj * t
                up += max(l * l - 2 * wj * l, h * h - 2 * wj * h)
            else:      # v11 bound_terms : quadratique et lineaire separes
                l2, h2 = l * l, h * h
                low += (l2 if l > 0 else (h2 if h < 0 else 0.0)) - 2 * wj * (h if wj >= 0 else l)
                up += (l2 if l2 > h2 else h2) - 2 * wj * (l if wj >= 0 else h)
        if low > tol:
            cur = esc[cur]
        elif up < -tol:
            p += min(end[cur] - beg[cur], thr - p)
            cur = esc[cur]
        elif end[cur] - beg[cur] <= LEAF:
            for i in range(beg[cur], end[cur]):
                if p >= thr:
                    break
                tests += 1
                if (X[i] - c[0]) ** 2 + (Y[i] - c[1]) ** 2 + (Z[i] - c[2]) ** 2 - r2 < -tol:
                    p += 1
            cur = esc[cur]
        else:
            cur += 1
    return nodes, tests, p >= thr


def main():
    frame, k, m, nq = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4])
    raw = np.fromfile(DATA.format(frame), dtype='<u4').reshape(-1, 3).astype(np.int64)
    key = (part1by2(raw[:, 0]) << np.uint64(2)) | (part1by2(raw[:, 1]) << np.uint64(1)) | part1by2(raw[:, 2])
    pts = raw[np.argsort(key, kind='stable')]
    tree = build(pts)
    X, Y, Z = ([float(v) for v in pts[:, j]] for j in range(3))
    kd = cKDTree(pts.astype(np.float64))
    rng = random.Random(3)
    tot = {False: [0, 0], True: [0, 0]}
    cat = {}
    done = 0
    for _ in range(nq):
        s = rng.randrange(len(pts))
        _, nb = kd.query(pts[s].astype(np.float64), k=m)
        part = [s] + rng.sample([q for q in nb if q != s], k - 1)
        P = pts[part].astype(np.float64)
        got = meb(P)
        if got is None:
            continue
        c, r2, sup = got
        anchor = tuple(float(v) for v in P[sup[0]])
        out = {e: walk(tree, X, Y, Z, c, r2, anchor, k, e) for e in (False, True)}
        for e in (False, True):
            tot[e][0] += out[e][0]; tot[e][1] += out[e][1]
        acc = cat.setdefault('saturee' if out[False][2] else 'complete', [0, 0, 0, 0, 0])
        acc[0] += 1; acc[1] += out[False][0]; acc[2] += out[False][1]; acc[3] += out[True][0]; acc[4] += out[True][1]
        done += 1
    print(f'trame={frame} k={k} m={m} requetes={done} noeuds_index={len(tree[0])}')
    for e in (False, True):
        print(f"{'exacte ' if e else 'separee'} : noeuds/req={tot[e][0] / done:7.1f} tests/req={tot[e][1] / done:7.1f}")
    for name, acc in sorted(cat.items()):
        n = acc[0]
        print(f'  {name:8s} n={n:4d} separee noeuds={acc[1] / n:6.1f} tests={acc[2] / n:6.1f}'
              f' | exacte noeuds={acc[3] / n:6.1f} tests={acc[4] / n:6.1f}')
    print(f'rapports exacte/separee : noeuds {tot[True][0] / tot[False][0]:.3f}'
          f' tests {tot[True][1] / tot[False][1]:.3f}')


if __name__ == '__main__':
    main()
