import sys, json
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
sys.dont_write_bytecode = True
import numpy as np
import bench_datasets as data
for family, level, n in [('hierarchical','medium',16000), ('hierarchical','medium',4000)]:
    spec = dict(family=family, n=n, groups=8, level=level, noise_fraction=0.0, seed=2026092800)
    pts, truth, meta = data.generate(spec)
    grid, _ = data.quantize(pts)
    open(f'{family}_{level}_{n}.u32le','wb').write(np.ascontiguousarray(grid, dtype='<u4').tobytes())
    print(family, level, n, int(grid.max()))
