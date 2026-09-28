"""Count roots of the Gabriel-only fold (cluster.merge_tree, as used by run_tower.py)
at the final level. The exact FULL T_K has exactly ONE root at a=+inf (L_K(inf) is
connected for K<=n), so any count > 1 is a topology that is not the FULL tower."""
import sys, json, subprocess, tempfile, os
import numpy as np
base='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0, base+'/tower_clustering_20260928'); sys.path.insert(0, base+'/synthetic_bench_20260928')
import bench_datasets as data, cluster as C, measure as M
BIN='/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
def run(family, n, k, seed):
    spec=dict(family=family,n=n,groups=8,level='medium',noise_fraction=0.0,seed=seed)
    pts,truth,meta=data.generate(spec); grid,_=data.quantize(pts)
    fd,path=tempfile.mkstemp(suffix='.u32le',dir=os.path.dirname(__file__)); os.close(fd)
    open(path,'wb').write(np.ascontiguousarray(grid,dtype='<u4').tobytes())
    try:
        done=subprocess.run(['nice','-n','19',BIN,'--input',path,'--k',str(k),'--workers','2'],capture_output=True,text=True)
    finally: os.unlink(path)
    if done.returncode!=0: return dict(error=done.stderr[:200])
    rep=json.loads(done.stdout)
    cof,gab,size=M.read_export(rep)
    out=dict(family=family,n=n,k=k,seed=seed,cofaces=len(cof))
    for conv in ('boundary','gabriel'):
        births=M.facet_births(cof,gab,conv)
        keep=None if conv=='boundary' else gab
        facets,plateaus=C.facet_levels(cof,keep)
        nodes,roots=C.merge_tree(facets,plateaus,births)
        out[conv+'_facets']=len(facets); out[conv+'_roots']=len(roots)
    return out
for fam,n,k in [(a,int(b),int(c)) for a,b,c in [s.split(":") for s in sys.argv[1:]]]:
    print(json.dumps(run(fam,n,k,11)),flush=True)
