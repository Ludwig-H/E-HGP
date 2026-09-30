"""Campagne L14-c : politique de bruit adaptative (remplissage borne par la densite), sans verite.

Un point rejete p recoit l'etiquette de son plus proche point classe q si
core(p) <= rho * quantile_95(core des membres du cluster de q) ; sinon il reste bruit.
rho = inf redonne le remplissage complet, rho = 0 l'abstention.
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
from campaign2 import mle_dimension  # noqa: E402


def bounded_fill(points, lab, core, rho):
    lab = lab.copy()
    noise = lab < 0
    if noise.all() or not noise.any():
        return lab
    ref = {c: np.quantile(core[lab == c], 0.95) for c in set(lab[~noise].tolist())}
    nn = NearestNeighbors(n_neighbors=1).fit(points[~noise])
    idx = nn.kneighbors(points[noise], return_distance=False)[:, 0]
    cand = lab[~noise][idx]
    ok = np.array([core[p] <= rho * ref[c] for p, c in zip(np.flatnonzero(noise), cand)])
    tgt = np.flatnonzero(noise)[ok]
    lab[tgt] = cand[ok]
    return lab


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
            zhat, _ = mle_dimension(pts)
            for ms in (2, 3, 5):
                tree = H.slt(pts, ms)
                core = H.core_distances(pts, max(ms, 5))
                cl = H.condense(tree, n, sq)
                lab = H.labels_from(cl, H.select(cl, H.stabilities(cl, zhat), 'eom'), n)
                for rho in (1.0, 1.5, 2.0, 3.0):
                    L = bounded_fill(pts, lab, core, rho)
                    w.writerow(dict(scene=s['scene'], seed=s['seed'], family=s['family'], level=s['level'], n=n,
                                    groups=s['groups'], noise=s['noise_fraction'],
                                    method='eom_zhat_ms%d_mcssqrt_bfill%.1f' % (ms, rho),
                                    ari=float(adjusted_rand_score(truth, L)), coverage=float((L >= 0).mean()),
                                    clusters=int(len(set(L.tolist()) - {-1})), extra='%.3f' % zhat))
            h.flush()
            print('%d/%d %s %d %.1fs' % (i + 1, len(specs), s['scene'], s['seed'], time.time() - t0), flush=True)


if __name__ == '__main__':
    main()
