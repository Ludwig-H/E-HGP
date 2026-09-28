import sys, warnings, statistics as st
warnings.filterwarnings('ignore')
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import bench_datasets as d, baselines as b
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score as ari
import math
for fam in ('shells','hierarchical','spherical','filaments'):
    for n in (500, 2000, 8000, 32000):
        if n == 32000 and fam in ('spherical',): pass
        dv, ov, mv = [], [], []
        for s in list(range(2026092800, 2026092805)):
            p, t, _ = d.generate(dict(family=fam, n=n, groups=8, level='medium', noise_fraction=0.0, seed=s))
            dv.append(b.hdbscan_default(p, t)['ari'])
            if n <= 8000: ov.append(b.hdbscan_oracle(p, t)['ari'])
            mv.append(ari(t, HDBSCAN(min_cluster_size=round(math.sqrt(n)), min_samples=3).fit(p).labels_))
        print('%-12s n=%-6d default %.3f %s  oracle %s  matched(sqrt n, ms=3) %.3f' % (fam, n, st.mean(dv), [round(x,2) for x in dv], ('%.3f' % st.mean(ov)) if ov else '-', st.mean(mv)), flush=True)
