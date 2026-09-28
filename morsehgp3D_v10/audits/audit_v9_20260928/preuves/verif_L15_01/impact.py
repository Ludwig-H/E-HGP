import sys, json, subprocess, math
sys.path.insert(0,'/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/verif_L15_01/head_src')
sys.path.insert(1,'/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import numpy as np
import bench_datasets as data, baselines, measure as M, cluster as C
assert 'head_src' in C.__file__, C.__file__
CAT='/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
fam,n,k,seed=sys.argv[1],int(sys.argv[2]),int(sys.argv[3]),int(sys.argv[4])
spec=dict(family=fam,n=n,groups=8,level='medium',noise_fraction=0.0,seed=seed)
pts,truth,meta=data.generate(spec); grid,scale=data.quantize(pts)
path='c_%s_%d_%d.u32le'%(fam,n,seed); open(path,'wb').write(np.ascontiguousarray(grid,dtype='<u4').tobytes())
o=subprocess.run(['nice','-n','19',CAT,'--input',path,'--k',str(k),'--workers','2'],capture_output=True,text=True)
rep=json.loads(o.stdout)
cof,gab,size=M.read_export(rep); conv='gabriel'
births=M.facet_births(cof,gab,conv); facets,plateaus=C.facet_levels(cof,gab)
nodes,roots=C.merge_tree(facets,plateaus,births)
sums,totals,masses,covered=M.measure(cof,gab,1,conv)
mass=math.sqrt(len(pts))
def pipeline(rts):
    clusters,order=C.condense(nodes,rts,masses,births,mass,'radius',1)
    sel=C.select_excess_of_mass(clusters,order)
    labels,ties=C.vote(len(pts),C.label_facets(clusters,sel),sums,totals,len(sel))
    return baselines.scores(truth,np.asarray(labels)), len(sel)
cov={r:len({p for f in (nodes[r]['members'] if r in nodes else {r}) for p in f}) for r in roots}
main=max(roots,key=lambda r:cov[r])
print(json.dumps(dict(family=fam,k=k,roots=len(roots),all_roots=pipeline(roots),main_root_only=pipeline([main]))))
