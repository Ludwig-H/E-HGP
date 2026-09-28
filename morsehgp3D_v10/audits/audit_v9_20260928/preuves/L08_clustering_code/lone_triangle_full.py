import sys
sys.path.insert(0, '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code')
exec(open('/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code/lone_triangle.py').read().split('rep = json.load')[0])
import numpy as np
A = np.asarray(X, dtype=np.float64)
for e in [(1070, 1484), (1070, 1944), (1484, 1944)]:
    gab = []
    for z in range(len(X)):
        if z in e: continue
        t = tuple(sorted(e + (z,)))
        cc, rr = meb2(t)
        ccf = np.array([float(v) for v in cc]); d2 = ((A - ccf) ** 2).sum(1)
        cand = [i for i in np.nonzero(d2 < float(rr) * (1 + 1e-9))[0] if i not in t]
        if any(sum((F(X[i][k]) - cc[k]) ** 2 for k in range(3)) < rr for i in cand):
            continue
        gab.append(t)
    print('full oracle Gabriel triangles containing', e, gab)
