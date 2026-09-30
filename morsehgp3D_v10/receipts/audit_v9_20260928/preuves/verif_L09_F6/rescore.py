"""Lecture seule : rejoue tour (4 variantes) + HDBSCAN defaut/oracle sur le banc du 28,
et score chaque labellisation sous plusieurs conventions de bruit."""
import csv, math, os, sys, tempfile, time, warnings
warnings.filterwarnings('ignore')
W = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0, W + '/synthetic_bench_20260928')
sys.path.insert(0, W + '/tower_clustering_20260928')
import numpy as np
import baselines, bench_datasets as data, plan as bench_plan
import run_tower as RT
from sklearn.metrics import adjusted_rand_score as ARI

def sing(pred):
    p = np.asarray(pred).copy(); m = p < 0
    p[m] = (p.max() + 1 if (~m).any() else 0) + np.arange(m.sum())
    return p

def metrics(truth, pred):
    truth = np.asarray(truth); pred = np.asarray(pred)
    inl = truth >= 0
    return dict(ari=ARI(truth, pred),
                ari_sing27=ARI(truth[inl], sing(pred)[inl]) if inl.sum() > 1 else float('nan'),
                ari_sing_both=ARI(sing(truth), sing(pred)),
                coverage=float((pred >= 0).mean()))

binary, out, maxn, limit = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
class A: pass
args = A(); args.export = binary; args.k = 2; args.z = 1; args.convention = 'gabriel'
args.lambda_mode = 'radius'; args.min_cluster_mass = 0.0; args.workers = 2
specs = [s for s in bench_plan.specifications(seeds=5, heavy=False) if s['n'] <= maxn][:limit]
cols = ['scene', 'family', 'level', 'n', 'noise_fraction', 'seed', 'method', 'parameter', 'ari', 'ari_sing27', 'ari_sing_both', 'coverage']
with tempfile.TemporaryDirectory() as keep, open(out, 'w', newline='') as h:
    w = csv.DictWriter(h, fieldnames=cols); w.writeheader()
    for i, s in enumerate(specs):
        pts, truth, meta = data.generate({k: s[k] for k in data.SPEC_KEYS})
        base = dict(scene=s['scene'], family=s['family'], level=s['level'], n=s['n'], noise_fraction=s['noise_fraction'], seed=s['seed'])
        grid, _ = data.quantize(pts)
        rep = RT.export(binary, grid, 2, 2, keep)
        mass = math.sqrt(len(pts))
        variants, _ = RT.tower_variants(rep, len(pts), 2, 1, 'gabriel', mass, 'radius')
        for v, (lab, _) in sorted(variants.items()):
            w.writerow(dict(base, method='tower_' + v, parameter=round(mass, 3), **metrics(truth, lab)))
        lab = baselines.hdbscan_labels(pts, baselines.DEFAULT_MIN_CLUSTER_SIZE)
        w.writerow(dict(base, method='hdbscan_default', parameter=20, **metrics(truth, lab)))
        cand = []
        for size in baselines.ORACLE_GRID:
            if size * 2 > len(pts): continue
            cand.append((size, metrics(truth, baselines.hdbscan_labels(pts, size))))
        # oracle du banc : premier maximum strict de l'ARI tous points (meme regle que baselines.py:62)
        best = None
        for size, m in cand:
            if best is None or m['ari'] > best[1]['ari']: best = (size, m)
        w.writerow(dict(base, method='hdbscan_oracle_ari', parameter=best[0], **best[1]))
        best = None
        for size, m in cand:
            if best is None or m['ari_sing27'] > best[1]['ari_sing27']: best = (size, m)
        w.writerow(dict(base, method='hdbscan_oracle_sing27', parameter=best[0], **best[1]))
        h.flush(); print(i, s['scene'], s['seed'], flush=True)
