import sys, json, subprocess, os, time
sys.path.insert(0,'/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
sys.path.insert(0,'/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import numpy as np
import bench_datasets as data
import measure as M, cluster as C
CAT='/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
FULL='/workspaces/E-HGP/build/v9-weighted-attachments-native-20260927-r1/native_weighted_export'
fam, n, k = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
spec=dict(family=fam,n=n,groups=8,level='medium',noise_fraction=0.0,seed=1)
pts,truth,meta=data.generate(spec)
grid,scale=data.quantize(pts)
path='cloud_%s_%d.u32le'%(fam,n)
open(path,'wb').write(np.ascontiguousarray(grid,dtype='<u4').tobytes())
def run(b):
    t=time.time(); out=subprocess.run(['nice','-n','19',b,'--input',path,'--k',str(k),'--workers','2'],capture_output=True,text=True); 
    assert out.returncode==0, out.stderr[:300]
    return json.loads(out.stdout), time.time()-t
rep,t1=run(CAT)
cof,gab,size=M.read_export(rep)
res={}
for conv in ('gabriel','boundary'):
    births=M.facet_births(cof,gab,conv); keep=None if conv=='boundary' else gab
    facets,plateaus=C.facet_levels(cof,keep); nodes,roots=C.merge_tree(facets,plateaus,births)
    res[conv]=len(roots)
full,t2=run(FULL)
nat=full['weighted']['native']
print(json.dumps(dict(family=fam,n=len(pts),k=k,cofaces=len(cof),tower_clustering_roots=res,full_roots=len(nat['roots']),full_roots_list=nat['roots'][:5] if isinstance(nat['roots'],list) else nat['roots'],secs=[round(t1,1),round(t2,1)])))
