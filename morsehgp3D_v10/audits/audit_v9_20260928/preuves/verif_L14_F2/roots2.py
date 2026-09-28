import sys, json, subprocess, tempfile, os, collections
import numpy as np
W='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0,W+'/synthetic_bench_20260928'); sys.path.insert(0,W+'/tower_clustering_20260928')
import bench_datasets as bd, cluster as C, measure as M
B='/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
fam=sys.argv[1]; K=int(sys.argv[2]); seed=int(sys.argv[3]) if len(sys.argv)>3 else 2026092800
p,t,m=bd.generate(dict(family=fam,n=2000,groups=8,level='medium',noise_fraction=0.0,seed=seed))
g,_=bd.quantize(p)
fd,path=tempfile.mkstemp(suffix='.u32le',dir='.'); os.write(fd,np.ascontiguousarray(g,dtype='<u4').tobytes()); os.close(fd)
r=subprocess.run([B,'--input',path,'--k',str(K),'--workers','2'],capture_output=True,text=True); os.unlink(path)
assert r.returncode==0, r.stderr[:300]
rep=json.loads(r.stdout)
cof,gab,size=M.read_export(rep)
print(fam,'K',K,'points',len(p),'cofaces',len(cof),'gabriel_facets',len(gab), 'stats', rep.get('stats'))
for conv in ('gabriel','boundary'):
    keep=None if conv=='boundary' else gab
    births=M.facet_births(cof,gab,conv)
    facets,plateaus=C.facet_levels(cof,keep)
    nodes,roots=C.merge_tree(facets,plateaus,births)
    def members(r): return nodes[r]['members'] if r in nodes else {r}
    comps=[set(x for f in members(r0) for x in f) for r0 in roots]
    sizes=sorted((len(c) for c in comps), reverse=True)
    big=max(comps,key=len)
    covered=set().union(*comps)
    # disjoint point groups: union-find over components sharing points
    print(' conv',conv,'facets',len(facets),'roots',len(roots),'covered_pts',len(covered),
          'top point-sizes',sizes[:8],'pts_outside_largest',len(covered-big))
