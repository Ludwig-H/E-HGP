import sys, json, subprocess, tempfile, os, math, collections
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
rep=json.loads(r.stdout)
cof,gab,size=M.read_export(rep)
births=M.facet_births(cof,gab,'gabriel')
facets,plateaus=C.facet_levels(cof,gab)
nodes,roots=C.merge_tree(facets,plateaus,births)
sums,totals,masses,covered=M.measure(cof,gab,1,'gabriel')
def members(r): return nodes[r]['members'] if r in nodes else {r}
rs=[]
for r0 in roots:
    mem=members(r0); pts=set(x for f in mem for x in f)
    rs.append((len(mem), round(float(sum(float(masses.get(f,0)) for f in mem)),2), len(pts)))
rs.sort(reverse=True)
print(fam,'K',K,'cofaces',len(cof),'facets',len(facets),'roots',len(roots),'covered',len(covered))
print('largest roots (facets, mass, points):',rs[:3]); print('others:',rs[3:15])
big=set(x for f in members(max(roots,key=lambda r:len(members(r)))) for x in f)
others=[set(x for f in members(r0) for x in f) for r0 in roots]
print('points outside the largest root component:', len(set().union(*others)-big))
