"""Root counts: native FULL forest (report['native']['roots']) vs Gabriel-only fold
(cluster.merge_tree as used by run_tower.py). Read-only on the repo."""
import sys, json, subprocess, tempfile, os
import numpy as np
base='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0, base+'/tower_clustering_20260928'); sys.path.insert(0, base+'/synthetic_bench_20260928')
import bench_datasets as data, cluster as C, measure as M
BIN='/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
here=os.path.dirname(os.path.abspath(__file__))
def run(family, n, k, seed):
    spec=dict(family=family,n=n,groups=8,level='medium',noise_fraction=0.0,seed=seed)
    pts,truth,meta=data.generate({key: spec.get(key) for key in data.SPEC_KEYS} if hasattr(data,'SPEC_KEYS') else spec); grid,_=data.quantize(pts)
    fd,path=tempfile.mkstemp(suffix='.u32le',dir=here); os.close(fd)
    open(path,'wb').write(np.ascontiguousarray(grid,dtype='<u4').tobytes())
    try:
        done=subprocess.run(['nice','-n','19',BIN,'--input',path,'--k',str(k),'--workers','2'],capture_output=True,text=True)
    finally: os.unlink(path)
    if done.returncode!=0: return dict(family=family,k=k,error=done.stderr[:200])
    rep=json.loads(done.stdout)
    nat=rep['native']
    cof,gab,size=M.read_export(rep)
    out=dict(family=family,n=len(pts),k=k,seed=seed,cofaces=len(cof),full_roots=len(nat.get('roots',[])),full_nodes=len(nat.get('nodes',[])))
    for conv in ('boundary','gabriel'):
        births=M.facet_births(cof,gab,conv)
        keep=None if conv=='boundary' else gab
        facets,plateaus=C.facet_levels(cof,keep)
        nodes,roots=C.merge_tree(facets,plateaus,births)
        out[conv+'_roots']=len(roots)
    return out
for s in sys.argv[1:]:
    a,b,c,d=s.split(':')
    print(json.dumps(run(a,int(b),int(c),int(d))),flush=True)
