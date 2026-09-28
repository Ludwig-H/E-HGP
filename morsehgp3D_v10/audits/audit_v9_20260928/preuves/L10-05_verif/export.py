import sys, os, json
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
sys.dont_write_bytecode = True
import numpy as np
import bench_datasets as data
out = []
for family, level, n in [('spherical','easy',8000), ('spherical','medium',8000), ('hierarchical','medium',8000), ('spherical','easy',16000)]:
    spec = dict(family=family, n=n, groups=8, level=level, noise_fraction=0.0, seed=2026092800)
    pts, truth, meta = data.generate(spec)
    grid, _ = data.quantize(pts)
    name = f'{family}_{level}_{n}.u32le'
    with open(name, 'wb') as f:
        f.write(np.ascontiguousarray(grid, dtype='<u4').tobytes())
    sizes = np.bincount(truth[truth >= 0])
    inter = (n*n - int((sizes.astype(np.int64)**2).sum()))//2
    out.append(dict(file=name, n=n, span=int(grid.max()), inter_pairs=inter, sizes=sizes.tolist()))
print(json.dumps(out, indent=1))
