#!/usr/bin/env python3
"""Fouille v2/v3 -> v11, idee v2_v3-01 : recalcul EXACT (Fraction, aucun flottant) des fixtures de non-heredite
et de seuil de la v2 et de la v3, avant de les proposer comme portes du catalogue v11.

Sources :
  R3v2  : morsehgp3D_v2/WARNING_AUDIT_IMPLEMENTATION_2.md § 2 (tetraedre regulier de rang ferme 4, faces de rang >= 12)
  F64   : morsehgp3D_v3/audits/AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md § 4.4 (q4 rang 4, 6 aretes et 4 faces
          de rang 12)
  Q2X   : morsehgp3D_v3/PROPOSITION.md § 10 (dix temoins q2 dans la boule diametrale qui ne ferment pas q4)
  F16   : morsehgp3D_v3/prototype/q4seed_axis_topr4_probe.cpp l. 276-285 (fixture_16 : 19 points, 16 q4 reguliers,
          deux a chaque profondeur 0..7)
  CRUX  : morsehgp3D_v3/audits/NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md § 4 (q4 positif a deux
          faces aigues seulement, borne optimale du probleme Crux 3653)

Pour chaque support S affinement independant de cardinal 2..4, on calcule la plus petite boule (miniboule) de S,
puis p = interieurs stricts et m = coquille sur TOUT le nuage. Code 0 si chaque assertion tient, 1 sinon.
Usage : python3 -B v2_v3_fixtures_check.py
"""
import itertools
import sys
from fractions import Fraction as F


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def solve(m, r):
    """Gauss exact sur Fraction ; m carree."""
    n = len(m)
    a = [[F(v) for v in row] + [F(r[i])] for i, row in enumerate(m)]
    for c in range(n):
        p = next(i for i in range(c, n) if a[i][c] != 0)
        a[c], a[p] = a[p], a[c]
        for i in range(n):
            if i != c and a[i][c] != 0:
                f = a[i][c] / a[c][c]
                a[i] = [x - f * y for x, y in zip(a[i], a[c])]
    return [a[i][n] / a[i][i] for i in range(n)]


def circum(points):
    """Centre du cercle/sphere circonscrit dans l'enveloppe affine, et poids barycentriques (Gram)."""
    a = points[0]
    vs = [sub(p, a) for p in points[1:]]
    g = [[dot(u, v) for v in vs] for u in vs]
    rhs = [F(dot(u, u), 2) for u in vs]
    lam = solve(g, rhs)
    c = tuple(F(a[j]) + sum(lam[i] * vs[i][j] for i in range(len(vs))) for j in range(3))
    weights = [1 - sum(lam)] + lam
    return c, weights


def miniball(points):
    """Plus petite boule d'au plus quatre points : support minimal positif."""
    best = None
    for r in range(1, len(points) + 1):
        for sub_s in itertools.combinations(points, r):
            if r == 1:
                c, w = tuple(F(v) for v in sub_s[0]), [F(1)]
            else:
                vs = [sub(p, sub_s[0]) for p in sub_s[1:]]
                if r == 3 and cross(vs[0], vs[1]) == (0, 0, 0):
                    continue
                if r == 4 and dot(cross(vs[0], vs[1]), vs[2]) == 0:
                    continue
                c, w = circum(list(sub_s))
            if any(x <= 0 for x in w):
                continue
            rad = sum((F(x) - y) ** 2 for x, y in zip(sub_s[0], c))
            if all(sum((F(x) - y) ** 2 for x, y in zip(p, c)) <= rad for p in points):
                if best is None or rad < best[1]:
                    best = (c, rad, sub_s, w)
    return best


def census(cloud, c, rad):
    p = m = 0
    for x in cloud:
        d = sum((F(v) - y) ** 2 for v, y in zip(x, c))
        if d < rad:
            p += 1
        elif d == rad:
            m += 1
    return p, m


def acute(a, b, c):
    return dot(sub(b, a), sub(c, a)) > 0 and dot(sub(a, b), sub(c, b)) > 0 and dot(sub(a, c), sub(b, c)) > 0


FAILS = []


def check(cond, label):
    print(('ok    ' if cond else 'ECHEC ') + label)
    if not cond:
        FAILS.append(label)


def sub_ranks(cloud, tetra):
    """Rangs fermes du q4 et de ses dix sous-supports (aretes, faces), et q_min de leur miniboule."""
    c, w = circum(list(tetra))
    rad = sum((F(x) - y) ** 2 for x, y in zip(tetra[0], c))
    p4, m4 = census(cloud, c, rad)
    edges, faces = [], []
    for e in itertools.combinations(tetra, 2):
        mb = miniball(list(e))
        edges.append(census(cloud, mb[0], mb[1]))
    for f in itertools.combinations(tetra, 3):
        mb = miniball(list(f))
        faces.append((census(cloud, mb[0], mb[1]), len(mb[2]), acute(*f)))
    return (c, rad, w, p4, m4), edges, faces


def r3v2():
    tetra = [(160, 160, 160), (160, 40, 40), (40, 160, 40), (40, 40, 160)]
    sigma = [(1, 1, -1), (1, -1, 1), (-1, 1, 1), (-1, -1, -1)]
    delta = [(0, 0, 0), (1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1), (1, 1, 0), (-1, -1, 0)]
    cloud = list(tetra) + [tuple(100 + 70 * s[j] + d[j] for j in range(3)) for s in sigma for d in delta]
    (c, rad, w, p4, m4), edges, faces = sub_ranks(cloud, tetra)
    print(f'R3v2 n={len(cloud)} centre={c} R2={rad} poids={w} p={p4} m={m4}')
    print(f'  aretes (p,m) : {edges}')
    print(f'  faces ((p,m), qmin, aigue) : {faces}')
    check(len(set(cloud)) == 40 and rad == 10800 and p4 == 0 and m4 == 4 and all(x > 0 for x in w),
          'R3v2 : q4 regulier de rang ferme 4 (p=0, m=4, poids > 0)')
    check(all(pm[0] + pm[1] >= 12 for pm, _, _ in faces), 'R3v2 : chaque face de rang ferme >= 12')


def f64():
    T = [(20, 20, 20), (60, 60, 20), (60, 20, 60), (20, 60, 60)]
    G01 = [(21, 55, 65), (21, 57, 64), (21, 58, 63), (21, 59, 62), (22, 53, 67), (22, 55, 66), (22, 56, 65),
           (22, 57, 65), (22, 58, 64)]
    G23 = [(21, 21, 18), (21, 22, 17), (21, 23, 16), (21, 25, 15), (22, 21, 17), (22, 22, 16), (22, 23, 15),
           (22, 24, 15), (22, 25, 14)]
    W = [(40, 40, 0), (40, 40, 80)]
    ring = []
    for i in range(-4, 6):
        ring += [(40 + i, 0, 40), (40 + i, 80, 40), (0, 40 + i, 40), (80, 40 + i, 40)]
    cloud = T + G01 + G23 + W + ring
    (c, rad, w, p4, m4), edges, faces = sub_ranks(cloud, T)
    print(f'F64 n={len(cloud)} centre={c} R2={rad} poids={w} p={p4} m={m4}')
    print(f'  aretes (p,m) : {edges}')
    print(f'  faces ((p,m), qmin, aigue) : {faces}')
    check(len(set(cloud)) == 64 and c == (40, 40, 40) and rad == 1200 and p4 == 0 and m4 == 4,
          'F64 : q4 de centre (40,40,40), R2=1200, p=0, m=4')
    check(all(pm == (10, 2) for pm in edges), 'F64 : six aretes p=10, m=2 (rang 12)')
    check(all(pm == (9, 3) and q == 3 for pm, q, _ in faces), 'F64 : quatre faces p=9, m=3 (rang 12)')


def q2x():
    a, b, x, y = (100, 100, 100), (200, 100, 100), (150, 30, 120), (150, 30, 80)
    zs = [(150 + i, 140, 100) for i in range(-4, 6)]
    cloud = [a, b, x, y] + zs
    tetra = [a, b, x, y]
    c, w = circum(tetra)
    rad = sum((F(v) - u) ** 2 for v, u in zip(a, c))
    p4, m4 = census(cloud, c, rad)
    lens = {e: dot(sub(*e), sub(*e)) for e in itertools.combinations(tetra, 2)}
    mid = tuple(F(u + v, 2) for u, v in zip(a, b))
    pab, mab = census(cloud, mid, F(lens[(a, b)], 4))
    print(f'Q2X centre={c} R2={rad} poids={w} p={p4} m={m4} ; boule diametrale ab : p={pab} m={mab}')
    check(c == (150, 80, 100) and rad == 2900 and w == [F(5, 14), F(5, 14), F(1, 7), F(1, 7)] and p4 == 0
          and m4 == 4, 'Q2X : q4 (a,b,x,y) vide, centre (150,80,100), poids (5/14,5/14,1/7,1/7)')
    check(max(lens.values()) == lens[(a, b)] and list(lens.values()).count(lens[(a, b)]) == 1,
          'Q2X : ab unique arete maximale')
    check(pab == 10, 'Q2X : dix interieurs stricts dans la boule diametrale de ab')


def f16():
    seed = [(125, 100, 100), (93, 124, 100), (93, 76, 100)]
    apex = [(100, 100, 100 + h) for h in list(range(-33, -25)) + list(range(26, 34))]
    cloud = seed + apex
    hist = {}
    for d in apex:
        tetra = seed + [d]
        c, w = circum(tetra)
        rad = sum((F(v) - u) ** 2 for v, u in zip(seed[0], c))
        p, m = census(cloud, c, rad)
        if all(x > 0 for x in w) and m == 4:
            hist[p] = hist.get(p, 0) + 1
    print(f'F16 n={len(cloud)} histogramme des q4 (seed + apex) par profondeur p : {dict(sorted(hist.items()))}')
    check(len(cloud) == 19 and hist == {p: 2 for p in range(8)},
          'F16 : seize q4 positifs a coquille 4, deux a chaque profondeur 0..7')
    counts = {K: sum(v for p, v in hist.items() if p + 4 <= K + 1) for K in range(3, 11)}
    print(f'  q4 de cette famille admis dans Cat_K (p+4 <= K+1) : {counts}')
    check(all(counts[K] == 2 * (K - 2) for K in range(3, 11)), 'F16 : 2(K-2) de ces q4 dans Cat_K pour K=3..10')


def crux():
    P = [(5, 8, 9), (5, 8, 11), (9, 12, 5), (15, 11, 12)]
    c, w = circum(P)
    rad = sum((F(v) - u) ** 2 for v, u in zip(P[0], c))
    opp_acute = [acute(*[P[j] for j in range(4) if j != i]) for i in range(4)]
    print(f'CRUX centre={c} R2={rad} poids={w} faces opposees aigues={opp_acute}')
    check(c == (10, 10, 10) and rad == 30 and w == [F(5, 28), F(6, 28), F(5, 28), F(12, 28)],
          'CRUX : centre (10,10,10), R2=30, poids (5,6,5,12)/28')
    check(opp_acute == [True, True, False, False], 'CRUX : seules les faces opposees a P0 et P1 sont aigues')


def main():
    r3v2()
    f64()
    q2x()
    f16()
    crux()
    print('BILAN', 'ECHECS=' + ','.join(FAILS) if FAILS else 'tout vert')
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
