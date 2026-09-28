import sys, json, subprocess, time, collections
sys.path.insert(0,'/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
sys.path.insert(0,'/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import numpy as np
import bench_datasets as data, measure as M, cluster as C
CAT='/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
FULL='/workspaces/E-HGP/build/v9-weighted-attachments-native-20260927-r1/native_weighted_export'
fam,n,k,seed=sys.argv[1],int(sys.argv[2]),int(sys.argv[3]),int(sys.argv[4])
spec=dict(family=fam,n=n,groups=8,level='medium',noise_fraction=0.0,seed=seed)
pts,truth,meta=data.generate(spec); grid,scale=data.quantize(pts)
path='c_%s_%d_%d.u32le'%(fam,n,seed); open(path,'wb').write(np.ascontiguousarray(grid,dtype='<u4').tobytes())
def run(b):
    o=subprocess.run(['nice','-n','19',b,'--input',path,'--k',str(k),'--workers','2'],capture_output=True,text=True)
    assert o.returncode==0,o.stderr[:300]; return json.loads(o.stdout)
rep=run(CAT); cof,gab,size=M.read_export(rep)
out=dict(family=fam,n=len(pts),k=k,seed=seed,digest=meta['digest'],cofaces=len(cof))
for conv in ('gabriel','boundary'):
    births=M.facet_births(cof,gab,conv); keep=None if conv=='boundary' else gab
    facets,plateaus=C.facet_levels(cof,keep); nodes,roots=C.merge_tree(facets,plateaus,births)
    # point coverage of each root
    mem=[nodes[r]['members'] if r in nodes else {r} for r in roots]
    cov=[len({p for f in m for p in f}) for m in mem]
    big=max(cov)
    out[conv]=dict(roots=len(roots),facets=len(facets),root_point_cover_sorted=sorted(cov,reverse=True)[:6],
                   roots_cover_ge_sqrtn=sum(1 for c in cov if c>=n**0.5))
full=run(FULL); nat=full['weighted']['native']
out['full_roots']=len(nat['roots']) if isinstance(nat['roots'],list) else nat['roots']
out['full_cat_same_cofaces']= len(full['weighted']['cofaces'])==len(cof)
print(json.dumps(out))
