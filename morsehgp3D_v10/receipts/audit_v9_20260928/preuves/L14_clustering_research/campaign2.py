"""Campagne L14-b : EOM a l'echelle de densite avec z estime (Levina-Bickel), sans verite.

z_hat = dimension intrinseque MLE globale (moyenne des inverses locaux, k=10, point exclu).
C'est l'usage 'vertical' le plus simple : le rapport des rayons d'ordre k1 < k2 de la tour.
"""
import csv
import math
import sys
import time

import numpy as np
from sklearn.metrics import adjusted_rand_score
from sklearn.neighbors import NearestNeighbors

sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/synthetic_bench_20260928')
import bench_datasets as bd  # noqa: E402
import plan as bench_plan  # noqa: E402
import heads as H  # noqa: E402

GRID = (2, 3, 5, 8, 12, 20, 32)


def mle_dimension(points, k=10):
    d = NearestNeighbors(n_neighbors=k + 1).fit(points).kneighbors(points)[0][:, 1:]
    d = np.maximum(d, 1e-12)
    inv = np.mean(np.log(d[:, -1:] / d[:, :-1]), axis=1)  # (1/(k-1)) sum log(T_k/T_j)
    return float(1.0 / np.mean(inv)), float(np.median(1.0 / np.maximum(inv, 1e-12)))


def main():
    out = sys.argv[1]
    specs = [s for s in bench_plan.specifications(heavy=False) if s['n'] <= 8000]
    fields = ('scene', 'seed', 'family', 'level', 'n', 'groups', 'noise', 'method', 'ari', 'coverage', 'clusters', 'extra')
    with open(out, 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=fields)
        w.writeheader()
        for i, s in enumerate(specs):
            t0 = time.time()
            pts, truth, meta = bd.generate({k: s[k] for k in bd.SPEC_KEYS})
            n = len(pts)
            sq = int(round(math.sqrt(n)))
            zmean, zmed = mle_dimension(pts)
            labs = {}
            for ms in GRID:
                tree = H.slt(pts, ms)
                for tag, mcs in (('sqrt', sq), ('20', 20)):
                    cl = H.condense(tree, n, mcs)
                    lab = H.labels_from(cl, H.select(cl, H.stabilities(cl, zmean), 'eom'), n)
                    labs[(ms, tag)] = lab
                    for fill in ('', '_fill'):
                        L = H.fill_noise(pts, lab) if fill else lab
                        w.writerow(dict(scene=s['scene'], seed=s['seed'], family=s['family'], level=s['level'], n=n,
                                        groups=s['groups'], noise=s['noise_fraction'],
                                        method='eom_zhat_ms%d_mcs%s%s' % (ms, tag, fill),
                                        ari=float(adjusted_rand_score(truth, L)), coverage=float((L >= 0).mean()),
                                        clusters=int(len(set(L.tolist()) - {-1})), extra='%.3f' % zmean))
            for tag in ('sqrt', '20'):
                L0 = [labs[(ms, tag)] for ms in GRID]
                agree = []
                for j in range(len(GRID)):
                    nb = [q for q in (j - 1, j + 1) if 0 <= q < len(GRID)]
                    agree.append(np.mean([adjusted_rand_score(L0[j], L0[q]) for q in nb]))
                j = int(np.argmax(agree))
                for fill in ('', '_fill'):
                    L = H.fill_noise(pts, L0[j]) if fill else L0[j]
                    w.writerow(dict(scene=s['scene'], seed=s['seed'], family=s['family'], level=s['level'], n=n,
                                    groups=s['groups'], noise=s['noise_fraction'],
                                    method='autoK_consensus_zhat_%s%s' % (tag, fill),
                                    ari=float(adjusted_rand_score(truth, L)), coverage=float((L >= 0).mean()),
                                    clusters=int(len(set(L.tolist()) - {-1})), extra='ms%d_z%.2f' % (GRID[j], zmean)))
            h.flush()
            print('%d/%d %s %d z=%.2f/%.2f %.1fs' % (i + 1, len(specs), s['scene'], s['seed'], zmean, zmed,
                                                   time.time() - t0), flush=True)


if __name__ == '__main__':
    main()
