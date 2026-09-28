"""Plafond de modele connu : GMM (k vrai, init aux moyennes vraies) -> ARI ; et
HDBSCAN matched. Point de calibration n=2000, g=8, seeds du plan. Lecture seule."""
import sys, warnings, statistics as st
warnings.filterwarnings('ignore')
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import numpy as np
import bench_datasets as d
from sklearn.mixture import GaussianMixture
from sklearn.metrics import adjusted_rand_score as ari
from sklearn.cluster import HDBSCAN
fams = sys.argv[1].split(',')
for f in fams:
    for l in d.LEVELS:
        g, m = [], []
        for s in range(2026092800, 2026092805):
            p, t, _ = d.generate(dict(family=f, n=2000, groups=8, level=l, noise_fraction=0.0, seed=s))
            keep = t >= 0
            k = len(set(t[keep].tolist()))
            means = np.array([p[t == c].mean(axis=0) for c in sorted(set(t[keep].tolist()))])
            gm = GaussianMixture(n_components=k, means_init=means, covariance_type='full', random_state=0).fit(p[keep])
            pred = np.full(len(t), -1); pred[keep] = gm.predict(p[keep])
            g.append(ari(t, pred))
            lab = HDBSCAN(min_cluster_size=45, min_samples=3).fit(p).labels_
            m.append(ari(t, lab))
        print('%-15s %-8s GMM-oracle %.3f (min %.2f)  matched_eom %.3f' % (f, l, st.mean(g), min(g), st.mean(m)), flush=True)
