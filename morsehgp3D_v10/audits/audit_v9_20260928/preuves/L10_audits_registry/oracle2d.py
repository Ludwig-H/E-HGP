import sys, json, time
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
sys.dont_write_bytecode = True
import numpy as np
import bench_datasets as data
import plan as P
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score
GRID = (5, 10, 15, 20, 30, 50, 75, 100)
MS = (None, 1, 2, 3, 5, 10)
out = {}
specs = [s for s in P.specifications() if s['scene'] in (
    'spherical_n2000_g8_medium_noise0', 'hierarchical_n2000_g8_medium_noise0',
    'filaments_n2000_g8_medium_noise0', 'shells_n2000_g8_medium_noise0',
    'spherical_n2000_g8_hard_noise0')]
for spec in specs:
    gen = data.generate({k: spec[k] for k in data.SPEC_KEYS})
    pts, truth = gen[0], gen[1]
    pts = np.asarray(pts, dtype=np.float64); truth = np.asarray(truth)
    row = {}
    for ms in MS:
        for sel in ('eom', 'leaf'):
            best = -1
            for m in GRID:
                lab = HDBSCAN(min_cluster_size=m, min_samples=ms, cluster_selection_method=sel).fit(pts).labels_
                a = adjusted_rand_score(truth, lab)
                best = max(best, a)
            row['ms=%s/%s' % (ms, sel)] = round(best, 4)
    out.setdefault(spec['scene'], []).append(row)
    print(spec['scene'], spec['seed'], row, flush=True)
agg = {}
for scene, rows in out.items():
    agg[scene] = {k: round(float(np.mean([r[k] for r in rows])), 4) for k in rows[0]}
print(json.dumps(agg, indent=1))
json.dump(dict(per_seed=out, mean=agg), open('oracle2d.json', 'w'), indent=1)
