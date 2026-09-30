"""Verification exacte : triangle de Gabriel isole dans G_2^Gab (convention du bord), scene bridge."""
import sys, json
from fractions import Fraction as F
sys.path.insert(0, '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code')
import probe as P
import numpy as np
spec = dict(family='bridge', n=2000, groups=8, level='medium', noise_fraction=0.0, seed=P.bench_plan.BASE_SEED)
points, truth, meta = P.data.generate(spec)
grid, scale = P.data.quantize(points)
X = [tuple(int(c) for c in row) for row in np.asarray(grid)]
tri = (1070, 1484, 1944)
def sub(a, b): return tuple(x - y for x, y in zip(a, b))
def dot(a, b): return sum(x * y for x, y in zip(a, b))
def cross(a, b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def meb2(ids):
    # returns (center, r2) exact for 2 or 3 points (MEB)
    pts = [X[i] for i in ids]
    if len(pts) == 2:
        c = tuple(F(p + q, 2) for p, q in zip(*pts)); r2 = sum((F(p) - cc) ** 2 for p, cc in zip(pts[0], c)); return c, r2
    a, b, c = pts
    # obtuse check
    for i in range(3):
        p, q, r = pts[i], pts[(i+1)%3], pts[(i+2)%3]
        if dot(sub(q, p), sub(r, p)) <= 0:  # angle at p >= 90 -> MEB = diametral of qr
            return meb2([ids[(i+1)%3], ids[(i+2)%3]])
    ab, ac = sub(b, a), sub(c, a)
    n = cross(ab, ac); nn = dot(n, n)
    t1 = cross(n, ab); t2 = cross(ac, n)
    num = tuple(F(dot(ac, ac)) * t1[k] + F(dot(ab, ab)) * t2[k] for k in range(3))
    center = tuple(F(a[k]) + num[k] / (2 * nn) for k in range(3))
    r2 = sum((F(a[k]) - center[k]) ** 2 for k in range(3))
    return center, r2
def inside(center, r2, ids_excl, strict=True):
    out = []
    for i, p in enumerate(X):
        if i in ids_excl: continue
        d2 = sum((F(p[k]) - center[k]) ** 2 for k in range(3))
        if d2 < r2 or (not strict and d2 == r2): out.append(i)
    return out
c, r2 = meb2(tri)
print('triangle', tri, 'coords', [X[i] for i in tri], 'MEB r2', float(r2))
print('  strict intruders in MEB(sigma):', inside(c, r2, set(tri)), ' on sphere:', [i for i in inside(c, r2, set(tri), strict=False)])
for e in [(1070, 1484), (1070, 1944), (1484, 1944)]:
    ce, re2 = meb2(e)
    intr = inside(ce, re2, set(e))
    print('  edge', e, 'diam r2', float(re2), 'intruders', intr[:8], 'count', len(intr))
rep = json.load(open('/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code/exports/bridge_medium_2000_8_0.0_s2026092800_k2.json'))
cof = [tuple(sorted(e['vertices'])) for e in rep['cofaces']]
for e in [(1070, 1484), (1070, 1944), (1484, 1944)]:
    print('  Gabriel triangles containing', e, [t for t in cof if set(e) <= set(t)])
# exhaustive oracle for triangles containing each edge: which are Gabriel (MEB with no strict intruder)
for e in [(1070, 1484), (1070, 1944), (1484, 1944)]:
    gab = []
    for z in range(len(X)):
        if z in e: continue
        t = tuple(sorted(e + (z,)))
        cc, rr = meb2(t)
        # quick reject by bounding
        if rr > 4 * r2: continue
        if not inside(cc, rr, set(t)): gab.append(t)
    print('  oracle Gabriel triangles containing', e, gab)
# general position check: min over other points of |d2 - r2| relative
