"""Verification adverse L09-F3 : reproduit HDBSCAN apparie avec ms=2 (FAIRNESS: ms=K) et ms=3 (HGP-old: K+1),
et variantes isolant mcs vs ms. Lecture seule du worktree."""
import csv, math, sys, time, warnings
warnings.filterwarnings('ignore')
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import numpy as np
import bench_datasets as data
import plan as bench_plan
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score

def run(points, mcs, ms):
    return HDBSCAN(min_cluster_size=int(mcs), min_samples=ms, copy=True).fit(points).labels_

out = sys.argv[1]
specs = [s for s in bench_plan.specifications() if s['n'] <= 8000]
with open(out, 'w', newline='') as h:
    w = csv.DictWriter(h, fieldnames=['scene','family','n','seed','method','ari']); w.writeheader()
    for i, s in enumerate(specs):
        pts, truth, meta = data.generate({k: s[k] for k in data.SPEC_KEYS})
        n = len(pts); r = max(2, int(round(math.sqrt(n))))
        confs = [('m_sqrt_ms2', r, 2), ('m_sqrt_ms3', r, 3), ('m20_ms3', 20, 3), ('m20_ms20', 20, None)]
        for name, mcs, ms in confs:
            lab = run(pts, mcs, ms)
            w.writerow(dict(scene=s['scene'], family=s['family'], n=s['n'], seed=s['seed'], method=name,
                            ari=adjusted_rand_score(truth, lab)))
        # oracle mcs grid with ms=3 (and ms=2)
        for ms in (2, 3):
            best = -2
            for size in (5, 10, 15, 20, 30, 50, 75, 100):
                if size * 2 > n: continue
                best = max(best, adjusted_rand_score(truth, run(pts, size, ms)))
            w.writerow(dict(scene=s['scene'], family=s['family'], n=s['n'], seed=s['seed'], method='oracle_ms%d' % ms, ari=best))
        h.flush()
        print(i, s['scene'], s['seed'], flush=True)
