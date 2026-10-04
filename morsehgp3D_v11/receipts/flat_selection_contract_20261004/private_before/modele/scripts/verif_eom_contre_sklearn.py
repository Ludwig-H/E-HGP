#!/usr/bin/env python3
"""Controle d'implementation : la condensation N-aire et l'EOM de modele_lib, appliquees a l'ultrametrique de
HDBSCAN (scikit-learn, min_samples = k), rendent les etiquettes de scikit-learn (z = 1, racine exclue, EOM) sur des
nuages flottants sans ex aequo. C'est un controle de code (oracle de correction), pas une mesure.

    python3 verif_eom_contre_sklearn.py GRAINE NUAGES
"""
import json
import sys

import numpy as np

import modele_lib as ml


def main():
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 20261004
    clouds = int(sys.argv[2]) if len(sys.argv) > 2 else 40
    rng = np.random.default_rng(seed)
    out = dict(seed=seed, clouds=0, checks=0, agree=0, disagreements=[], forced=0)
    for c in range(clouds):
        n = int(rng.integers(40, 160))
        g = int(rng.integers(2, 6))
        centers = rng.uniform(0, 10, size=(g, 3))
        lab = rng.integers(0, g, size=n)
        X = centers[lab] + rng.normal(0, rng.uniform(0.3, 1.2), size=(n, 3))
        k = int(rng.integers(2, 8))
        U = ml.hdbscan_ultrametric(X, k)
        out['clouds'] += 1
        for mcs in (2, 3, 5, 10, 20):
            if mcs > n // 2:
                continue
            tg = ml.Treegram(n, U, [U[i][i] for i in range(n)], label='hdbscan')
            clusters, viol = ml.condensed_tree(tg, mcs)
            if viol:
                raise AssertionError('ultrametrique HDBSCAN non monotone ?')
            sel, forced = ml.eom_select(tg, clusters, 1)
            out['forced'] += forced
            ours = ml.labels_from_selection(tg, clusters, sel)
            theirs = ml.hdbscan_labels(X, k, mcs)
            out['checks'] += 1
            if ml.same_partition(ours, theirs):
                out['agree'] += 1
            else:
                out['disagreements'].append(dict(cloud=c, n=n, k=k, mcs=mcs,
                                                 ours=len(set(ours) - {-1}), theirs=len(set(theirs) - {-1}),
                                                 diff=sum(1 for a, b in zip(ours, theirs) if (a == -1) != (b == -1))))
    print(json.dumps(out, indent=1))


if __name__ == '__main__':
    main()
