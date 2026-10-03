#!/usr/bin/env python3
"""Contre-fixture « amas scinde » du rapport (d=200, t=20, g=180 et 140), rejouee par vf_oracle.
Questions : (1) les niveaux 91,24 / 110,11 et fractions 1 / 1/2 ; (2) dependance au germe : FULL fusionne-t-il deja
une partie de A avec B avant 110,11 ? (3) meilleure fraction FULL sur TOUTES les composantes ne couvrant aucun site
de B (et pas seulement la lignee du germe)."""
import math
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-points-math/verif_fermeture')
import vf_oracle as vo
from fractions import Fraction


def split_cluster(d, t, g):
    A1 = [(-d - t, -t // 2, 0), (-d - t, t // 2, 0)]
    v = [(-d // 2, 0, 0)]
    A2 = [(0, 0, 0), (t, -t, 0), (t, t, 0)]
    Bp = [(t + g, -t // 2, 0), (t + g, t // 2, 0)]
    return A1 + v + A2 + Bp, [0, 1, 2, 3, 4, 5], [6, 7]


def r(x):
    return '%.3f' % math.sqrt(x)


for g in (180, 140):
    pts, A, B = split_cluster(200, 20, g)
    cl = vo.Cloud(pts)
    F = vo.Full(cl, 2)
    n = len(pts)
    print('\n## amas scinde g=%d, points %s' % (g, pts))
    print('noeuds FULL_2 (rayon, enfants):', [(r(l), ch) for l, ch in zip(F.levels_of_node, F.children)])
    Am = sum(1 << i for i in A)
    Bm = sum(1 << i for i in B)
    # premiere coupe ou une composante couvre a la fois un site de A et un site de B
    first_mixed = None
    for a, snap in F.snaps:
        if any(cov & Am and cov & Bm for _v, cov in snap):
            first_mixed = a
            break
    print('FULL : premiere composante couvrant des sites de A ET de B a r=%s' % r(first_mixed))
    # premiere fusion FULL entre une composante couvrant >=1 site de A (et aucun de B) et une couvrant B
    best_any = Fraction(0)
    for a, snap in F.snaps:
        if a >= first_mixed:
            break
        for _v, cov in snap:
            if not cov & Bm:
                best_any = max(best_any, Fraction(vo.popcount(cov & Am), len(A)))
    print('FULL : meilleure fraction de A couverte par une composante sans site de B, avant r=%s : %s'
          % (r(first_mixed), best_any))
    # par lignee de germe (comme le rapport : germe = plus petit D_k ; on essaie TOUS les germes de A)
    core = F.hang_core()
    for sa in A:
        sb = 6
        va, vb = core[sa][1], core[sb][1]
        w = F.lca(va, vb)
        merge = max(F.levels_of_node[w], core[sa][0], core[sb][0])
        best = 0
        for a, snap in F.snaps:
            if a >= merge:
                break
            if a < core[sa][0]:
                continue
            anc = F.alive(va, a)
            for v, cov in snap:
                if v == anc:
                    best = max(best, vo.popcount(cov & Am))
        print('  germe A=%d (D_2=%s) : fusion des lignees germe A / germe B a r=%s, fraction %s'
              % (sa, cl.Dk(sa, 2), r(merge), Fraction(best, len(A))))
    for m in (3,):
        u, w = F.closure(m)
        for sa in (0, 3):
            merge = u[sa][6]
            blk = [j for j in range(n) if u[sa][j] < merge and u[j][j] < merge]
            print('  fermeture m=%d germe %d : reunion avec B a r=%s, bloc avant = %s, fraction %s'
                  % (m, sa, r(merge), blk, Fraction(len([j for j in blk if j in A]), len(A))))
        for name, ent in (('H3', F.hang_margin(3)), ('H1', F.hang_margin(1)), ('EC3', F.hang_ec(3)),
                          ('first3', F.hang_first(3)), ('core', F.hang_core())):
            uu = F.ultra(ent)
            for sa in (0, 3):
                merge = uu[sa][6]
                blk = [j for j in range(n) if uu[sa][j] < merge and uu[j][j] < merge]
                print('  %-6s germe %d : reunion avec B a r=%s, bloc avant %s, fraction %s'
                      % (name, sa, r(merge), blk, Fraction(len([j for j in blk if j in A]), len(A))))
