"""Verification adverse L14-F3 : sklearn seul (pas la reimplementation heads.py).

Pour chaque scene du banc (heavy=False, 250 executions) :
- HDBSCAN(min_samples=2|3, min_cluster_size=round(sqrt n)) par fit direct sklearn ;
- arbre de liaison simple sklearn par ms, puis sklearn tree_to_labels(eom) pour chaque mcs :
  oracle 1D recalcule (ms = mcs, grille du banc) et oracle 2D (ms x mcs) ;
- comparaison au recu r1 (hdbscan_oracle) et controle du digest.
"""
import csv, math, sys, time
import numpy as np
from sklearn.cluster import HDBSCAN
from sklearn.cluster._hdbscan._tree import tree_to_labels
from sklearn.metrics import adjusted_rand_score
from sklearn.neighbors import NearestNeighbors

B = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928'
sys.path.insert(0, B)
import bench_datasets as bd
import plan as bench_plan
import baselines as bl

MS_GRID = (2, 3, 5, 8, 12, 20, 32)
MCS = (5, 10, 15, 20, 30, 50, 75, 100)


def fill(points, lab):
    lab = lab.copy(); noise = lab < 0
    if noise.all() or not noise.any():
        return lab
    idx = NearestNeighbors(n_neighbors=1).fit(points[~noise]).kneighbors(points[noise], return_distance=False)[:, 0]
    lab[noise] = lab[~noise][idx]
    return lab


def main(out):
    specs = bench_plan.specifications(heavy=False)
    fields = ['scene', 'seed', 'family', 'level', 'n', 'digest', 'direct_ms2_sq', 'direct_ms3_sq', 'direct_ms2_sq_fill',
              'oracle1d', 'oracle1d_par', 'oracle2d', 'oracle2d_par', 'oracle2d_ms_eq_mcs_only']
    with open(out, 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=fields); w.writeheader()
        for i, s in enumerate(specs):
            t0 = time.time()
            pts, truth, meta = bd.generate({k: s[k] for k in bd.SPEC_KEYS})
            n = len(pts); sq = int(round(math.sqrt(n)))
            row = dict(scene=s['scene'], seed=s['seed'], family=s['family'], level=s['level'], n=n, digest=meta['digest'])
            d2 = HDBSCAN(min_cluster_size=sq, min_samples=2, copy=True).fit(pts).labels_
            d3 = HDBSCAN(min_cluster_size=sq, min_samples=3, copy=True).fit(pts).labels_
            row['direct_ms2_sq'] = adjusted_rand_score(truth, d2)
            row['direct_ms3_sq'] = adjusted_rand_score(truth, d3)
            row['direct_ms2_sq_fill'] = adjusted_rand_score(truth, fill(pts, d2))
            # 1D oracle exactly as baselines.hdbscan_oracle (reuse the bench function)
            o1 = bl.hdbscan_oracle(pts, truth)
            row['oracle1d'] = o1['ari']; row['oracle1d_par'] = o1['parameter']
            best = (-2, None)
            for ms in MS_GRID:
                slt = HDBSCAN(min_cluster_size=5, min_samples=ms, copy=True).fit(pts)._single_linkage_tree_
                for mcs in sorted(set(MCS + (sq,))):
                    if 2 * mcs > n:
                        continue
                    lab, _ = tree_to_labels(slt, mcs, 'eom', False, 0.0, None)
                    a = adjusted_rand_score(truth, lab)
                    if a > best[0]:
                        best = (a, 'ms%d_mcs%d' % (ms, mcs))
            row['oracle2d'] = best[0]; row['oracle2d_par'] = best[1]
            row['oracle2d_ms_eq_mcs_only'] = ''
            w.writerow(row); h.flush()
            print('%d/%d %s %d %.1fs' % (i + 1, len(specs), s['scene'], s['seed'], time.time() - t0), flush=True)


if __name__ == '__main__':
    main(sys.argv[1])
