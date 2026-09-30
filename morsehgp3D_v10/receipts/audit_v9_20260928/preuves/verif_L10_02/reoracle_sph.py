# Verification independante du constat L10-02 (oracle HDBSCAN 1D vs 2D).
import sys, json, time, hashlib
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import numpy as np
import plan as P
import bench_datasets as D
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score as ari

GRID = (5, 10, 15, 20, 30, 50, 75, 100)
MS = (None, 1, 3, 5, 10)
SCENES = sys.argv[1].split(',')
res = {}
for spec in P.specifications():
    if spec['scene'] not in SCENES:
        continue
    pts, truth, meta = D.generate({k: spec[k] for k in D.SPEC_KEYS})
    pts = np.asarray(pts, dtype=np.float64); truth = np.asarray(truth)
    cells = {}
    t0 = time.time()
    for ms in MS:
        for sel in ('eom', 'leaf'):
            for m in GRID:
                lab = HDBSCAN(min_cluster_size=m, min_samples=ms, cluster_selection_method=sel, copy=True).fit(pts).labels_
                cells['%s|%s|%d' % (ms, sel, m)] = float(ari(truth, lab))
    res.setdefault(spec['scene'], []).append(dict(seed=spec['seed'], digest=meta['digest'], cells=cells))
    print(spec['scene'], spec['seed'], meta['digest'][:12], round(time.time() - t0, 1), 's', flush=True)
out = {}
for scene, runs in res.items():
    def best(filter_fn):
        vals = []
        for r in runs:
            vals.append(max(v for k, v in r['cells'].items() if filter_fn(k)))
        return float(np.mean(vals))
    oracle1d = best(lambda k: k.startswith('None|eom|'))
    per = {}
    for ms in MS:
        for sel in ('eom', 'leaf'):
            per['%s/%s' % (ms, sel)] = round(best(lambda k, p='%s|%s|' % (ms, sel): k.startswith(p)), 4)
    oracle2d = best(lambda k: True)
    # oracle 2D *par scene* : meilleure cellule sur toutes les combinaisons, par graine
    # "global" : un seul (ms,sel,mcs) fixe pour les 5 graines (moins optimiste)
    keys = runs[0]['cells'].keys()
    glob = max(keys, key=lambda k: np.mean([r['cells'][k] for r in runs]))
    out[scene] = dict(oracle1d=round(oracle1d, 4), oracle2d=round(oracle2d, 4), gain=round(oracle2d - oracle1d, 4),
                      per_ms_sel=per, best_fixed_cell=glob,
                      best_fixed_cell_mean=round(float(np.mean([r['cells'][glob] for r in runs])), 4))
print(json.dumps(out, indent=1))
json.dump(dict(raw=res, summary=out), open(sys.argv[2], 'w'), indent=1)
