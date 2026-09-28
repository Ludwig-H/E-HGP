import sys, json, time
from fractions import Fraction as F
import numpy as np
W = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0, W + '/tower_clustering_20260928'); sys.path.insert(0, W + '/synthetic_bench_20260928')
import run_tower as R, bench_datasets as data, plan as bench_plan, measure as M, cluster as C
BIN = '/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
OUT = '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_F2_verif'
spec = dict(family='bridge', n=2000, groups=8, level='medium', noise_fraction=0.0, seed=bench_plan.BASE_SEED)
points, truth, meta = data.generate(spec)
grid, scale = data.quantize(points)
report = R.export(BIN, grid, 2, 2, OUT)
X = [tuple(int(c) for c in row) for row in np.asarray(grid)]
A = np.asarray(X, dtype=np.float64)
cofaces, gabriel, size = M.read_export(report)
# roots per convention, print small roots
for conv in ('gabriel', 'boundary'):
    births = M.facet_births(cofaces, gabriel, conv)
    keep = None if conv == 'boundary' else gabriel
    facets, plateaus = C.facet_levels(cofaces, keep)
    nodes, roots = C.merge_tree(facets, plateaus, births)
    sizes = []
    for r in roots:
        mem = nodes[r]['members'] if r in nodes else {r}
        sizes.append((len(mem), sorted(mem)[:4]))
    print(conv, 'roots', len(roots), 'sizes', sorted(sizes)[:6])
tri = (1070, 1484, 1944)
cof = {v: b for v, b in cofaces}
print('triangle in export cofaces:', tri in cof, 'beta', cof.get(tri))
print('edges gabriel in catalogue:', [(e, e in gabriel) for e in [(1070,1484),(1070,1944),(1484,1944)]])
print('other export cofaces containing each edge:', [(e, [t for t in cof if set(e) <= set(t) and t != tri]) for e in [(1070,1484),(1070,1944),(1484,1944)]])
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def cross(a,b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def meb(ids):
    pts = [X[i] for i in ids]
    if len(pts) == 2:
        c = tuple(F(p+q, 2) for p, q in zip(*pts)); return c, sum((F(p)-cc)**2 for p, cc in zip(pts[0], c))
    for i in range(3):
        p, q, r = pts[i], pts[(i+1)%3], pts[(i+2)%3]
        if dot(sub(q,p), sub(r,p)) <= 0: return meb([ids[(i+1)%3], ids[(i+2)%3]])
    a, b, c = pts; ab, ac = sub(b,a), sub(c,a); n = cross(ab, ac); nn = dot(n,n)
    t1 = cross(n, ab); t2 = cross(ac, n)
    center = tuple(F(a[k]) + F(dot(ac,ac)*t1[k] + dot(ab,ab)*t2[k], 2*nn) for k in range(3))
    return center, sum((F(a[k])-center[k])**2 for k in range(3))
def empty(ids):
    c, r2 = meb(ids)
    cf = np.array([float(v) for v in c]); d2 = ((A-cf)**2).sum(1)
    cand = [i for i in np.nonzero(d2 < float(r2)*(1+1e-7)+1)[0] if i not in ids]
    return not any(sum((F(X[i][k])-c[k])**2 for k in range(3)) < r2 for i in cand), c, r2
p = [X[i] for i in tri]
print('angles dot at each vertex:', [dot(sub(p[(i+1)%3],p[i]), sub(p[(i+2)%3],p[i])) for i in range(3)])
print('triangle Gabriel (exact):', empty(tri)[0])
for e in [(1070,1484),(1070,1944),(1484,1944)]:
    print('edge', e, 'Gabriel exact:', empty(e)[0])
    others = []
    for z in range(len(X)):
        if z in e: continue
        t = tuple(sorted(e+(z,)))
        if t == tri: continue
        if empty(t)[0]: others.append(t)
    print('  exhaustive other Gabriel triangles containing it:', others)
