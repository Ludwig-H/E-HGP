import sys, json, subprocess, os
import numpy as np
from fractions import Fraction
W='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0,W+'/tower_clustering_20260928')
import cluster as C, measure as M
B='/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
pts=np.array([[0,0,7],[0,9,6],[1,4,0],[0,0,1],[4,1,2]],dtype='<u4')  # A B C D E -> ids 0..4
open('e5.u32le','wb').write(pts.tobytes())
r=subprocess.run([B,'--input','e5.u32le','--k','2','--workers','1'],capture_output=True,text=True)
print('rc',r.returncode, r.stderr[:200])
rep=json.loads(r.stdout)
cof,gab,size=M.read_export(rep)
name='ABCDE'
print('cofaces',[(''.join(name[v] for v in c),str(b)) for c,b in cof])
print('gabriel facets',[(''.join(name[v] for v in f),str(b)) for f,b in gab.items()])
for conv in ('gabriel','boundary'):
    keep=None if conv=='boundary' else gab
    births=M.facet_births(cof,gab,conv)
    facets,plateaus=C.facet_levels(cof,keep)
    nodes,roots=C.merge_tree(facets,plateaus,births)
    print(conv,'roots',len(roots))
    for nm,nd in sorted(nodes.items(), key=lambda x: x[1]['level']):
        pts_=sorted(set(name[x] for f in nd['members'] for x in f))
        print('  node',nm,'level',nd['level'],float(nd['level']),'points',''.join(pts_))
