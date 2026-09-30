import sys, os, json, subprocess, numpy as np
from fractions import Fraction
sys.path.insert(0, os.path.join(os.getcwd(), 'headclust'))
import cluster as C, measure as M
B='/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
found=None
for n in range(5,11):
    for seed in range(400):
        rng=np.random.default_rng(1000*n+seed); s=set()
        while len(s)<n: s.add(tuple(int(v) for v in rng.integers(0,12,3)))
        pts=np.array(sorted(s)); open('mw.u32le','wb').write(pts.astype('<u4').tobytes())
        out=subprocess.run([B,'--input','mw.u32le','--k','2','--workers','1'],capture_output=True,text=True)
        if out.returncode: continue
        rep=json.loads(out.stdout)
        try:
            cof,gab,size=M.read_export(rep)
            births=M.facet_births(cof,gab,'gabriel'); facets,pl=C.facet_levels(cof,gab); nodes,roots=C.merge_tree(facets,pl,births)
        except ValueError: continue
        if len(roots)!=len(rep['native']['roots']):
            found=(n,seed,pts.tolist(),len(roots),len(rep['native']['roots'])); break
    if found: break
print(found)
