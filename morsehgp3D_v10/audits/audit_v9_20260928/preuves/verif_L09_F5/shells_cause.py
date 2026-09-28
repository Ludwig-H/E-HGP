import sys, warnings
warnings.filterwarnings('ignore')
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import bench_datasets as d, baselines as b
for n in (2000, 8000):
    p,t,_ = d.generate(dict(family='shells', n=n, groups=8, level='medium', noise_fraction=0.0, seed=2026092800))
    r = b.hdbscan_default(p,t); o = b.hdbscan_oracle(p,t)
    print(n, 'default', {k: r[k] for k in ('ari','clusters','noise','coverage')}, 'oracle', {k: o[k] for k in ('ari','clusters','noise','parameter')})
