"""Lecture seule : HDBSCAN a reglage apparie a la tour (fixe d'avance, sans verite).

hdbscan_matched_eom  : min_cluster_size = sqrt(n) (comme la masse de la tour), min_samples = 3 (K+1, K=2), EOM
hdbscan_matched_leaf : idem, selection leaf
hdbscan_ms_auto      : min_cluster_size = sqrt(n), min_samples = None (= mcs, defaut sklearn)
Scores : ARI tous points (convention du banc), ARI bruit->singletons, AMI, couverture.
"""
import csv, math, sys, time, os, warnings
warnings.filterwarnings('ignore')
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import numpy as np
import bench_datasets as data
import plan as bench_plan
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score, adjusted_mutual_info_score

def singletons(pred):
    pred = np.asarray(pred).copy()
    m = pred < 0
    pred[m] = pred.max() + 1 + np.arange(m.sum())
    return pred

def run(points, mcs, ms, method):
    return HDBSCAN(min_cluster_size=int(mcs), min_samples=ms, cluster_selection_method=method, copy=True).fit(points).labels_

out = sys.argv[1]
maxn = int(sys.argv[2]) if len(sys.argv) > 2 else 8000
specs = [s for s in bench_plan.specifications() if s['n'] <= maxn]
cols = ['scene','family','n','groups','level','noise_fraction','seed','method','parameter','ari','ari_singletons','ami','coverage','clusters','seconds']
with open(out, 'w', newline='') as h:
    w = csv.DictWriter(h, fieldnames=cols); w.writeheader()
    for i, s in enumerate(specs):
        pts, truth, meta = data.generate({k: s[k] for k in data.SPEC_KEYS})
        n = len(pts); mcs = max(2, int(round(math.sqrt(n))))
        for name, ms, meth in (('hdbscan_matched_eom', 3, 'eom'), ('hdbscan_matched_leaf', 3, 'leaf'), ('hdbscan_ms_auto_sqrtn', None, 'eom')):
            t = time.monotonic(); lab = run(pts, mcs, ms, meth); dt = time.monotonic() - t
            w.writerow(dict(scene=s['scene'], family=s['family'], n=s['n'], groups=s['groups'], level=s['level'],
                            noise_fraction=s['noise_fraction'], seed=s['seed'], method=name, parameter='mcs=%d,ms=%s' % (mcs, ms),
                            ari=adjusted_rand_score(truth, lab), ari_singletons=adjusted_rand_score(truth, singletons(lab)),
                            ami=adjusted_mutual_info_score(truth, lab), coverage=float((lab >= 0).mean()),
                            clusters=len(set(lab.tolist()) - {-1}), seconds=dt))
        h.flush()
        print(i, s['scene'], s['seed'], flush=True)
