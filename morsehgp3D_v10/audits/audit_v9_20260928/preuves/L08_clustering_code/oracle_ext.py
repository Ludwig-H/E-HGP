"""Oracle HDBSCAN etendu : grille min_cluster_size x min_samples {1, 5, = mcs} x {eom, leaf}."""
import sys, csv, math
import numpy as np
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import bench_datasets as data, plan as bench_plan, baselines
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score as ari
fams = set(sys.argv[1].split(','))
maxn = int(sys.argv[2])
specs = [s for s in bench_plan.specifications(heavy=False) if s['family'] in fams and s['n'] <= maxn]
w = csv.writer(sys.stdout); w.writerow(['scene', 'seed', 'n', 'oracle_restricted', 'oracle_ext', 'best'])
for s in specs:
    pts, truth, meta = data.generate({k: s[k] for k in data.SPEC_KEYS})
    X = np.asarray(pts, dtype=np.float64)
    best, arg, restricted = -1, None, -1
    for mcs in baselines.ORACLE_GRID:
        if mcs * 2 > len(X): continue
        for ms in (1, 5, None):
            for sel in ('eom', 'leaf'):
                lab = HDBSCAN(min_cluster_size=mcs, min_samples=ms, cluster_selection_method=sel).fit(X).labels_
                a = ari(truth, lab)
                if ms is None and sel == 'eom': restricted = max(restricted, a)
                if a > best: best, arg = a, (mcs, ms, sel)
    w.writerow([s['scene'], s['seed'], s['n'], round(restricted, 4), round(best, 4), arg]); sys.stdout.flush()
