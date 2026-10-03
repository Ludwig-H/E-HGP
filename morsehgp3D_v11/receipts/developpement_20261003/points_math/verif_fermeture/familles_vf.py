#!/usr/bin/env python3
"""Familles exactes du rapport rejouees par vf_oracle : vallee (t=10/30/50, 3D k=2,3), vallee+frange, contact en
un site, cinq points, rival lointain (H_1 formule 4s^2+sD ; H_3 x13,9), discontinuite EC."""
import math
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-points-math/verif_fermeture')
import vf_oracle as vo
from fractions import Fraction


def r(x):
    return '%.3f' % math.sqrt(x)


def valley(d, t, third=False):
    A = [(-d - t, -t, 0), (-d - t, t, 0), (-d, 0, 0)]
    B = [(d, 0, 0), (d + t, -t, 0), (d + t, t, 0)]
    if third:
        A.append((-d - t, 0, t))
        B.append((d + t, 0, t))
    pts = A + [(0, 0, 0)] + B
    na = len(A)
    return pts, list(range(na)), list(range(na + 1, 2 * na + 1))


def lineage_merge(F, ia, ib):
    core = F.hang_core()
    va, vb = core[ia][1], core[ib][1]
    w = F.lca(va, vb)
    return max(F.levels_of_node[w], core[ia][0], core[ib][0])


def report_closure(name, pts, A, B, k, ms):
    cl = vo.Cloud(pts)
    F = vo.Full(cl, k)
    root = F.levels_of_node[F.root]
    # FULL : premiere fusion d'une composante couvrant un site de A avec une couvrant un site de B (tous germes)
    fm = min(lineage_merge(F, a, b) for a in A for b in B)
    out = []
    for m in ms:
        u, w = F.closure(m)
        um = min(u[a][b] for a in A for b in B)
        out.append('m=%d fermeture A-B r=%s' % (m, r(um)))
    print('%-40s k=%d racine FULL r=%s ; premiere fusion FULL lignees A/B r=%s ; %s'
          % (name, k, r(root), r(fm), ' ; '.join(out)))


print('# familles')
for t in (10, 30, 50):
    pts, A, B = valley(100, t)
    report_closure('vallee d=100 t=%d' % t, pts, A, B, 2, [1, 2, 3])
pts, A, B = valley(100, 20, third=True)
report_closure('vallee 3D d=100 t=20', pts, A, B, 2, [3, 4])
report_closure('vallee 3D d=100 t=20', pts, A, B, 3, [3, 4])
five = [(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]
report_closure('cinq points', five, [1, 2], [3, 4], 2, [2, 3])
for z in (0, 2, 5):
    s = (0, 0, 0)
    pts = [s, (-12, -2, 0), (-12, 2, 0), (12, -2, z), (12, 2, z)]
    report_closure('contact un site z=%d' % z, pts, [1, 2], [3, 4], 2, [2, 3])

# vallee + frange (fermeture moins bonne selon le rapport)
d, t, f = 100, 10, 150
A = [(-d - t, -t, 0), (-d - t, t, 0), (-d, 0, 0), (-d - t - f, 0, 0)]
B = [(d, 0, 0), (d + t, -t, 0), (d + t, t, 0), (d + t + f, 0, 0)]
pts = A + [(0, 0, 0)] + B
cl = vo.Cloud(pts)
F = vo.Full(cl, 2)
u, w = F.closure(3)
n = len(pts)
Aidx = [0, 1, 2, 3]
for sa in Aidx:
    merge = u[sa][5]
    blk = [j for j in range(n) if u[sa][j] < merge and u[j][j] < merge]
    print('vallee+frange fermeture m=3 germe %d : reunion r=%s bloc avant %s fraction %s' % (
        sa, r(merge), blk, Fraction(len([j for j in blk if j in Aidx]), 4)))
core = F.hang_core()
print('vallee+frange : D_2 des sites de A', [cl.Dk(i, 2) for i in Aidx], ' racine FULL r=%s' % r(F.levels_of_node[F.root]))
for sa in Aidx:
    va = core[sa][1]
    merge = lineage_merge(F, sa, 5)
    best = 0
    for a, snap in F.snaps:
        if a >= merge or a < core[sa][0]:
            continue
        anc = F.alive(va, a)
        for v, cov in snap:
            if v == anc:
                best = max(best, vo.popcount(cov & 15))
    print('vallee+frange FULL germe %d : fusion lignee r=%s fraction %s' % (sa, r(merge), Fraction(best, 4)))

# rival lointain H_1 : (0, 2s, 4s + D)
print('# rival lointain H_1, k=2, m=1')
for s in (1, 1000):
    for D in ((0, 1, 10, 96) if s == 1 else (0, 1, 1000, 96000)):
        pts = [(0, 0, 0), (2 * s, 0, 0), (4 * s + D, 0, 0)]
        F = vo.Full(vo.Cloud(pts), 2)
        e = F.hang_margin(1)[1][0]
        ec = F.hang_ec(1)[1][0]
        fi = F.hang_first(1)[1][0]
        u, _ = F.closure(1)
        form = Fraction(4 * s * s + s * D) if D > 0 else Fraction(4 * s * s)
        print('s=%d D=%d : H_1 niveau %s (formule %s : %s) r=%s ; EC r=%s ; first(LCA) r=%s ; fermeture r=%s' % (
            s, D, e, form, 'OK' if e == form else 'ECART', r(e), r(ec), r(fi), r(u[1][1])))
print('# rival lointain H_3, k=2, m=3')
for D in (4, 10, 30, 100, 300, 1000):
    pts = [(0, 0, 0), (-2, 1, 0), (-2, -1, 0), (D, 1, 0), (D, -1, 0), (D + 2, 0, 0)]
    F = vo.Full(vo.Cloud(pts), 2)
    e = F.hang_margin(3)[0][0]
    u, _ = F.closure(3)
    t0 = u[0][0]
    print('D=%d : premiere couverture qualifiee r=%s ; racine r=%s ; H_3 r=%s (x%.2f) ; EC_3 r=%s ; first_3 r=%s'
          % (D, r(t0), r(F.levels_of_node[F.root]), r(e), math.sqrt(e / t0), r(F.hang_ec(3)[0][0]),
             r(F.hang_first(3)[0][0])))
