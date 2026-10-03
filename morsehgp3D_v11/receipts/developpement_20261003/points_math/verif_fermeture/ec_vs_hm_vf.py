#!/usr/bin/env python3
"""Rejoue ec_vs_hm.py (graine 23, 600 nuages, generateurs recopies) avec vf_oracle : sites retardes (entree > premiere
couverture qualifiee) pour H_m (margin, ou margin1 si m=1), EC, first ; EC <,=,> H_m."""
import math
import random
import sys
import time
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-points-math/verif_fermeture')
import vf_oracle as vo
from frequences_vf import cloud_gate, cloud_generic, cloud_two_blobs

rng = random.Random(23)
rows = {}
t0 = time.time()
for c in range(600):
    gen = ('gate', 'generic', 'two_blobs')[c % 3]
    if gen == 'gate':
        pts = cloud_gate(rng)
    elif gen == 'generic':
        pts = cloud_generic(rng)
    else:
        pts = cloud_two_blobs(rng)[0]
    n = len(pts)
    cl = vo.Cloud(pts)
    for k in (2, 3):
        if k >= n:
            continue
        F = vo.Full(cl, k)
        for m in (1, k + 1):
            if m > n:
                continue
            hm = F.hang_margin(m)
            ec = F.hang_ec(m)
            fi = F.hang_first(m)
            u, _ = F.closure(m)
            r = rows.setdefault((k, m), dict(sites=0, dh=0, de=0, df=0, lt=0, eq=0, gt=0, mxh=0.0, mxe=0.0,
                                             sh=0.0, se=0.0, cnt=0, same_ef=0))
            for i in range(n):
                t = u[i][i]
                eh, ee, ef = hm[i][0], ec[i][0], fi[i][0]
                r['sites'] += 1
                r['dh'] += eh > t
                r['de'] += ee > t
                r['df'] += ef > t
                r['same_ef'] += (ee == ef and ec[i][1] == fi[i][1])
                r['lt'] += ee < eh
                r['eq'] += ee == eh
                r['gt'] += ee > eh
                if t > 0:
                    r['cnt'] += 1
                    rh, re_ = math.sqrt(eh / t), math.sqrt(ee / t)
                    r['sh'] += rh
                    r['se'] += re_
                    r['mxh'] = max(r['mxh'], rh)
                    r['mxe'] = max(r['mxe'], re_)
print('600 nuages graine 23, %.1f s' % (time.time() - t0))
for (k, m), r in sorted(rows.items()):
    print('k=%d m=%d sites %d ; retardes H_m %d (%.1f %%) EC %d (%.1f %%) first %d ; EC <,=,> H_m %d,%d,%d ;'
          ' sqrt(e/t) H_m moy %.3f max %.2f ; EC moy %.3f max %.2f ; EC == first (entree et noeud) %d' % (
              k, m, r['sites'], r['dh'], 100.0 * r['dh'] / r['sites'], r['de'], 100.0 * r['de'] / r['sites'],
              r['df'], r['lt'], r['eq'], r['gt'], r['sh'] / r['cnt'], r['mxh'], r['se'] / r['cnt'], r['mxe'],
              r['same_ef']))
