import sys, os, hashlib
sys.dont_write_bytecode=True
W='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928'
sys.path.insert(0,W)
import numpy as np
import bench_datasets as data
fam, n, g, lvl, seed = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4], int(sys.argv[5])
spec=dict(family=fam,n=n,groups=g,level=lvl,noise_fraction=0.0,seed=seed)
pts,truth,meta=data.generate(spec)
grid,scale=data.quantize(pts)
out='%s_n%d_g%d_%s_s%d.u32le'%(fam,n,g,lvl,seed)
open(out,'wb').write(np.ascontiguousarray(grid,dtype='<u4').tobytes())
print(out, len(pts), meta['digest'], hashlib.sha256(open(out,'rb').read()).hexdigest()[:16])
