"""HDBSCAN() aux valeurs par defaut de sklearn (mcs=5, ms=None->5) contre mcs=20 sur le plan n<=8000 ; lecture seule."""
import sys, warnings, statistics as st, collections
warnings.filterwarnings('ignore')
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import bench_datasets as d, plan as P
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score as ari
a5 = collections.defaultdict(list); a20 = collections.defaultdict(list)
lv5 = collections.defaultdict(list); lv20 = collections.defaultdict(list)
for s in P.specifications():
    if s['n'] > 8000: continue
    p, t, _ = d.generate({k: s[k] for k in d.SPEC_KEYS})
    x5 = ari(t, HDBSCAN().fit(p).labels_)
    x20 = ari(t, HDBSCAN(min_cluster_size=20).fit(p).labels_)
    for k in (s['family'], 'all'):
        a5[k].append(x5); a20[k].append(x20)
    if s['n']==2000 and s['groups']==8 and s['noise_fraction']==0.0:
        lv5[s['level']].append(x5); lv20[s['level']].append(x20)
print('runs', len(a5['all']))
for k in sorted(a5): print('%-16s mcs5=%.3f mcs20=%.3f (n=%d)' % (k, st.mean(a5[k]), st.mean(a20[k]), len(a5[k])))
for k in d.LEVELS: print('level %-8s (famille x niveau, n2000 g8) mcs5=%.3f mcs20=%.3f' % (k, st.mean(lv5[k]), st.mean(lv20[k])))
