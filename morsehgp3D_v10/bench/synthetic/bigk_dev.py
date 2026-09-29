"""Experience de DEVELOPPEMENT (graines dev seulement) : des ordres K plus grands aident-ils le clustering ?

Motif : le moteur exact de la tour est borne a K = 10 (kMaxOrder). Sur les melanges, le bruit de l'estimateur K-NN
(ecart-type relatif ~ 1/sqrt(K)) efface les cols peu profonds : aucune composante n'est retenue au-dela d'un col a
70 % du pic a K = 10 (audits/tete_multik_20260929), et il reste 0,017 d'ecart a la partition de Morse, surtout sur
`anisotropic` (receipts/bench_dev_alloc_20260929). Avant tout chantier moteur pour K > 10, on mesure l'effet de K sur
le temoin MR2-bord, qui egale la tour a meme entree et meme tete (lot C, famille « objet » : ecart <= 0,010).

Methodes, a chaque K de KS (mcs = sqrt(n)) :
  - `mr2b`    : hierarchie d'atteignabilite mutuelle alpha = 2, entree points-bord, tete v10 (EOM, z de ZS) ;
  - `sklearn` : sklearn.cluster.HDBSCAN, feuilles, alpha = 2 (configuration du lot C a K >= 3), min_samples = K ;
  - `tour`    : la tour a K = 10 (entree cover, EOM z = 6), ancre du lot C.
Remplissages (communs) : none ; b2 et b2.5 (bornes, core a max(K, 5)) ; asc20_b2 (montee de densite k = 20, rejet
b2) ; ascK_b2 (montee a k = max(K, 5), rejet b2). Un refus vaut ARI_s = 0 et est compte (EVAL_v2 D8).

  python3 bigk_dev.py --build BUILD --out OUT.csv --sizes 8000,16000,32000 --replicates 2 --jobs 46
Codes : 0 ; 1 si une ligne est un refus (toutes les lignes sont ecrites).
"""
import argparse
import csv
import math
import os
import sys
from concurrent.futures import ProcessPoolExecutor, as_completed

import numpy as np
from scipy.spatial import cKDTree

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import alloc_dev  # noqa: E402
import methods  # noqa: E402
import metrics  # noqa: E402
import run_campaign  # noqa: E402
import scenes  # noqa: E402

KS = (10, 16, 24, 32, 48)
ZS = (3, 6)
FILLS = ('none', 'b2', 'b2.5', 'asc20_b2', 'ascK_b2')
COLS = ('family', 'level', 'noise', 'n', 'seed', 'method', 'k', 'z', 'fill', 'ari_s', 'ami_nc', 'coverage',
        'clusters', 'refused')


def rows_for(spec, n, method, k, z, lab, G, T, knn):
    out = []
    for fill in FILLS:
        if lab is None:
            out.append(dict(family=spec['family'], level=spec['level'], noise=spec['noise_fraction'], n=n,
                            seed=spec['seed'], method=method, k=k, z=z, fill=fill, ari_s=0.0, ami_nc=0.0,
                            coverage=0.0, clusters=0, refused=1))
            continue
        core = knn(max(k, 5))[0]
        if fill == 'none':
            pred = lab
        elif fill[0] == 'b':
            pred = methods.bounded_fill(G, lab, max(k, 5), float(fill[1:]))
        else:
            kk = 20 if fill.startswith('asc20') else max(k, 5)
            pred = alloc_dev.reject(lab, alloc_dev.ascent_fill(lab, knn(kk)[1]), core, 2.0)
        s = metrics.scores(T, pred)
        out.append(dict(family=spec['family'], level=spec['level'], noise=spec['noise_fraction'], n=n,
                        seed=spec['seed'], method=method, k=k, z=z, fill=fill, ari_s=round(s['ari_s'], 6),
                        ami_nc=round(s['ami_nc'], 6), coverage=round(s['coverage'], 4), clusters=s['clusters'],
                        refused=0))
    return out


def run_unit(spec, build):
    P, L, _ = scenes.generate(spec)
    G, T, _, _ = scenes.quantize18(P, L)
    n = len(G)
    mcs = int(round(math.sqrt(n)))
    X = np.asarray(G, dtype=np.float64)
    tree = cKDTree(X)
    memo = {}

    def knn(kk):
        if kk not in memo:
            d, nbr = tree.query(X, k=kk)
            memo[kk] = (d[:, -1], alloc_dev.ascent_parents(d[:, -1], nbr))
        return memo[kk]

    out = []
    for k in KS:
        try:
            labs = methods.mreach_labels(build, G, k, 2, 'border', [(mcs, z, 'eom', False) for z in ZS], threads=1)
        except Exception:  # refus : ARI_s = 0 (D8)
            labs = [None] * len(ZS)
        for z, lab in zip(ZS, labs):
            out += rows_for(spec, n, 'mr2b', k, z, lab, G, T, knn)
        try:
            lab = methods.hdbscan_labels(G, k, mcs, 'leaf', 2.0)
        except Exception:
            lab = None
        out += rows_for(spec, n, 'sklearn', k, 0, lab, G, T, knn)
    try:
        batch = methods.tower_labels_batch(build, G, [10], ['cover'], [(mcs, 6, 'eom', False)], threads=1)
        lab = batch[('cover', 10, 0)]
    except Exception:
        lab = None
    out += rows_for(spec, n, 'tour', 10, 6, lab, G, T, knn)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--sizes', default='8000,16000,32000')
    ap.add_argument('--replicates', type=int, default=2)
    ap.add_argument('--jobs', type=int, default=4)
    a = ap.parse_args()
    specs = run_campaign.plan('dev', [int(s) for s in a.sizes.split(',')], a.replicates)
    specs.sort(key=lambda s: -s['n'])
    refused = 0
    with open(a.out, 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=COLS)
        w.writeheader()
        with ProcessPoolExecutor(max_workers=a.jobs) as pool:
            futs = {pool.submit(run_unit, s, a.build): s for s in specs}
            for i, fu in enumerate(as_completed(futs)):
                try:
                    rows = fu.result()
                except Exception as e:  # generation ou ouvrier en echec : ecrit comme refus (D8)
                    print('ECHEC', futs[fu], repr(e)[:300], flush=True)
                    s = futs[fu]
                    rows = []
                    for k in KS:
                        for z in ZS:
                            rows += rows_for(s, s['n'], 'mr2b', k, z, None, None, None, None)
                        rows += rows_for(s, s['n'], 'sklearn', k, 0, None, None, None, None)
                    rows += rows_for(s, s['n'], 'tour', 10, 6, None, None, None, None)
                refused += sum(r['refused'] for r in rows)
                w.writerows(rows)
                h.flush()
                print('%d/%d' % (i + 1, len(specs)), flush=True)
    print('SCENES %d LIGNES_REFUSEES %d' % (len(specs), refused), flush=True)
    return 1 if refused else 0


if __name__ == '__main__':
    sys.exit(main())
