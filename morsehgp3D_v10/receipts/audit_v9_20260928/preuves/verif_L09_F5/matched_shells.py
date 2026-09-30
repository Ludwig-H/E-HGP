import sys, warnings, math, statistics as st
warnings.filterwarnings('ignore')
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import bench_datasets as d, baselines as b
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score as ari
for n in (500, 8000, 32000):
    mv=[]; ov=[]
    for s in range(2026092800, 2026092805):
        p,t,_ = d.generate(dict(family='shells', n=n, groups=8, level='medium', noise_fraction=0.0, seed=s))
        mv.append(ari(t, HDBSCAN(min_cluster_size=round(math.sqrt(n)), min_samples=3).fit(p).labels_))
        if n == 32000:
            ov.append(ari(t, HDBSCAN(min_cluster_size=100).fit(p).labels_))
    print('shells n=%d matched(sqrt n, ms=3) %.3f %s  mcs100(default ms) %s' % (n, st.mean(mv), [round(x,3) for x in mv], [round(x,3) for x in ov]), flush=True)
