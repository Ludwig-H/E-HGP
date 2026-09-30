"""Temoin : HDBSCAN(min_samples=1, min_cluster_size=ceil(sqrt n)) -- la chaine v9 a K=1 -- sur les scenes du banc."""
import sys, csv, math, json, collections
import numpy as np
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import bench_datasets as data, plan as bench_plan, baselines
from sklearn.cluster import HDBSCAN
fams = set(sys.argv[1].split(',')) if len(sys.argv) > 1 else None
specs = [s for s in bench_plan.specifications(heavy=False) if fams is None or s['family'] in fams]
w = csv.writer(sys.stdout)
w.writerow(['scene', 'seed', 'family', 'n', 'level', 'ari_ms1_sqrt', 'clusters', 'coverage'])
for s in specs:
    pts, truth, meta = data.generate({k: s[k] for k in data.SPEC_KEYS})
    n = len(pts)
    lab = HDBSCAN(min_cluster_size=math.ceil(math.sqrt(n)), min_samples=1).fit(np.asarray(pts, dtype=np.float64)).labels_
    sc = baselines.scores(truth, lab)
    w.writerow([s['scene'], s['seed'], s['family'], s['n'], s['level'], round(sc['ari'], 4), sc['clusters'], round(sc['coverage'], 3)])
    sys.stdout.flush()
