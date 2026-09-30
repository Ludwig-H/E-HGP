import sys, json, subprocess
sys.path.insert(0,'/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import numpy as np
import measure as M, cluster as C
CAT='/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
FULL='/workspaces/E-HGP/build/v9-weighted-attachments-native-20260927-r1/native_weighted_export'
pts=np.array([[0,0,7],[0,9,6],[1,4,0],[0,0,1],[4,1,2]],dtype='<u4')
open('e5.u32le','wb').write(pts.tobytes())
def run(b,k):
    o=subprocess.run(['nice','-n','19',b,'--input','e5.u32le','--k',str(k),'--workers','1'],capture_output=True,text=True)
    assert o.returncode==0,o.stderr
    return json.loads(o.stdout)
rep=run(CAT,2)
json.dump(rep,open('e5_cat.json','w'))
cof,gab,size=M.read_export(rep)
print('cofaces',[(v,str(b)) for v,b in cof])
print('gabriel facets',{k:str(v) for k,v in gab.items()})
for conv in ('gabriel','boundary'):
    births=M.facet_births(cof,gab,conv); keep=None if conv=='boundary' else gab
    facets,plateaus=C.facet_levels(cof,keep); nodes,roots=C.merge_tree(facets,plateaus,births)
    print(conv,'births',{k:str(v) for k,v in births.items()})
    for n,d in sorted(nodes.items()):
        print('  ',n,str(d['level']),d['children'])
    print('  roots',roots)
full=run(FULL,2)
json.dump(full,open('e5_full.json','w'))
print(list(full.keys()))
