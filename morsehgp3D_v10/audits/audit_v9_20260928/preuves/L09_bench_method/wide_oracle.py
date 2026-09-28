"""Oracle HDBSCAN elargi (mcs x min_samples x eom/leaf) sur les 32 cellules famille x niveau, n=2000, g=8."""
import sys, csv, warnings, itertools
warnings.filterwarnings('ignore')
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import numpy as np
import bench_datasets as d, plan as P
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score as ari
MCS = (5, 10, 20, 30, 45, 75, 100, 150)
MS = (1, 2, 3, 5, 10, 20, None)
specs = [s for s in P.specifications() if s['n'] == 2000 and s['groups'] == 8 and s['noise_fraction'] == 0.0]
with open('wide_oracle.csv', 'w', newline='') as h:
    w = csv.writer(h); w.writerow(['scene','family','level','seed','method','ari','coverage','parameter'])
    for s in specs:
        p, t, _ = d.generate({k: s[k] for k in d.SPEC_KEYS})
        best = (-2, None, None)
        best_eom_mcs_only = None
        for mcs, ms, sel in itertools.product(MCS, MS, ('eom', 'leaf')):
            lab = HDBSCAN(min_cluster_size=mcs, min_samples=ms, cluster_selection_method=sel).fit(p).labels_
            a = ari(t, lab)
            if a > best[0]: best = (a, float((lab >= 0).mean()), '%d/%s/%s' % (mcs, ms, sel))
        w.writerow([s['scene'], s['family'], s['level'], s['seed'], 'hdbscan_wide_oracle', best[0], best[1], best[2]]); h.flush()
        print(s['scene'], s['seed'], round(best[0], 3), best[2], flush=True)
