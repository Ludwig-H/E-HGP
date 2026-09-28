import sys, warnings, statistics as st, collections
warnings.filterwarnings('ignore')
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import bench_datasets as d, plan as P
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score as ari
acc = collections.defaultdict(list)
for s in P.specifications():
    if s['n'] > 8000: continue
    p, t, _ = d.generate({k: s[k] for k in d.SPEC_KEYS})
    lab = HDBSCAN().fit(p).labels_
    acc[s['family']].append(ari(t, lab)); acc['all'].append(ari(t, lab))
print({k: round(st.mean(v), 3) for k, v in acc.items()})
