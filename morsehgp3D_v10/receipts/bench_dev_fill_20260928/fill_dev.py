"""Exploration DEV (graines dev seulement) : politiques de bruit none / full / b(rho) appliquees SYMETRIQUEMENT aux
meilleures configurations de la tour et de sklearn HDBSCAN (reçu dev_r2). b(rho) (EVAL_v2 § 2.6, CLUSTER_v2 § 8.1) :
un point de bruit p recoit l'amas c du point classe le plus proche si core(p) <= rho * Q95_c, core = distance au
max(K, 5)-ieme voisin (point compris), Q95_c = quantile 95 % (type 7) de core sur les membres de c.
"""
import argparse
import csv
import math
import os
import sys
from concurrent.futures import ProcessPoolExecutor, as_completed

import numpy as np
from scipy.spatial import cKDTree

SYN = sys.argv[sys.argv.index('--syn') + 1] if '--syn' in sys.argv else None
sys.path.insert(0, SYN)
import methods  # noqa: E402
import metrics  # noqa: E402
import run_campaign  # noqa: E402
import scenes  # noqa: E402

FILLS = ('none', 'full', 'b1', 'b1.25', 'b1.5', 'b2')
COLS = ('family', 'level', 'noise', 'n', 'seed', 'config', 'fill', 'ari_s', 'coverage', 'clusters')


def core(G, k):
    d, _ = cKDTree(np.asarray(G, dtype=np.float64)).query(np.asarray(G, dtype=np.float64), k=k)
    return d[:, -1]


def bounded_fill(G, labels, cr, rho):
    labels = labels.copy()
    noise = labels < 0
    if noise.all() or not noise.any():
        return labels
    q95 = {}
    for c in np.unique(labels[~noise]):
        q95[c] = float(np.quantile(cr[labels == c], 0.95))
    tree = cKDTree(np.asarray(G[~noise], dtype=np.float64))
    _, j = tree.query(np.asarray(G[noise], dtype=np.float64))
    cand = labels[~noise][j]
    idx = np.flatnonzero(noise)
    thr = np.array([q95[c] for c in cand]) * rho
    ok = cr[idx] <= thr
    labels[idx[ok]] = cand[ok]
    return labels


def run_unit(spec, build):
    P, L, _ = scenes.generate(spec)
    G, T, _, _ = scenes.quantize18(P, L)
    n = len(G)
    mcs = int(round(math.sqrt(n)))
    zh = methods.zhat(G)
    cfgs = {
        'tw_K1_zhat_eom': (lambda: methods.tower_labels(build, G, 1, mcs, zh, 'eom', threads=1), 1),
        'tw_K5_leaf': (lambda: methods.tower_labels(build, G, 5, mcs, 1.0, 'leaf', threads=1), 5),
        'hdb_ms20_leaf_a1': (lambda: methods.hdbscan_labels(G, 20, mcs, 'leaf', 1.0), 20),
        'hdb_ms12_leaf_a2': (lambda: methods.hdbscan_labels(G, 12, mcs, 'leaf', 2.0), 12),
        'hdb_ms1_eom_a1': (lambda: methods.hdbscan_labels(G, 1, mcs, 'eom', 1.0), 1),
    }
    cores = {}
    out = []
    for name, (fn, k) in cfgs.items():
        lab = fn()
        kk = max(k, 5)
        if kk not in cores:
            cores[kk] = core(G, kk)
        for f in FILLS:
            if f == 'none':
                l2 = lab
            elif f == 'full':
                l2 = methods.fill_noise(G, lab)
            else:
                l2 = bounded_fill(G, lab, cores[kk], float(f[1:]))
            s = metrics.scores(T, l2)
            out.append(dict(family=spec['family'], level=spec['level'], noise=spec['noise_fraction'], n=n,
                            seed=spec['seed'], config=name, fill=f, ari_s=round(s['ari_s'], 6),
                            coverage=round(s['coverage'], 4), clusters=s['clusters']))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build', required=True)
    ap.add_argument('--syn', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--sizes', default='2000,8000')
    ap.add_argument('--jobs', type=int, default=4)
    args = ap.parse_args()
    specs = run_campaign.plan('dev', [int(s) for s in args.sizes.split(',')], 2)
    print('%d unites' % len(specs), flush=True)
    with open(args.out, 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=COLS)
        w.writeheader()
        with ProcessPoolExecutor(max_workers=args.jobs) as pool:
            futs = {pool.submit(run_unit, s, args.build): s for s in specs}
            done = 0
            for fu in as_completed(futs):
                done += 1
                try:
                    rows = fu.result()
                except Exception as e:
                    print('ECHEC', futs[fu], repr(e), flush=True)
                    continue
                for r in rows:
                    w.writerow(r)
                h.flush()
                print('%d/%d' % (done, len(specs)), flush=True)


if __name__ == '__main__':
    main()
