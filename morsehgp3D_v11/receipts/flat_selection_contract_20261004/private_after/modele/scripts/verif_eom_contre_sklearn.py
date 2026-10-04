#!/usr/bin/env python3
"""Controle d'implementation : la condensation N-aire et l'EOM de modele_lib, appliquees a l'ultrametrique de
HDBSCAN (scikit-learn, min_samples = k), contre les etiquettes de scikit-learn (z = 1, racine exclue, EOM).

Deux classes de cas :
  - arbre du lien simple SANS hauteurs ex aequo : les deux doivent coincider (controle du code) ;
  - AVEC ex aequo (frequents des k >= 2 : une arete d'atteignabilite mutuelle vaut souvent la distance de coeur
    d'un de ses bouts) : scikit-learn binarise le plateau dans l'ordre de np.argsort ; on compte les desaccords et
    les amas de stabilite nulle que scikit-learn selectionne (nes et morts au meme lambda).
C'est un controle de code et une observation sur l'adversaire, pas une mesure de qualite.

    python3 verif_eom_contre_sklearn.py GRAINE NUAGES
"""
import collections
import json
import sys

import numpy as np
from sklearn.cluster import HDBSCAN
from sklearn.cluster._hdbscan._tree import _condense_tree

import modele_lib as ml


def sklearn_zero_stability_selected(X, k, mcs):
    """Nombre d'amas selectionnes par scikit-learn dont tous les points sortent au lambda de naissance."""
    model = HDBSCAN(min_samples=k, min_cluster_size=mcs, algorithm='kd_tree', n_jobs=1, copy=True).fit(X)
    ct = np.asarray(_condense_tree(model._single_linkage_tree_, mcs))
    clus = ct[ct['cluster_size'] > 1]
    birth = {int(r['child']): float(r['value']) for r in clus}
    labels = model.labels_
    zero = 0
    for cid, lam in birth.items():
        rows = ct[ct['parent'] == cid]
        if len(rows) and all(float(v) == lam for v in rows['value']) and not np.any(clus['parent'] == cid):
            # feuille de duree nulle ; est-elle selectionnee ? (ses points portent une etiquette)
            pts = [int(c) for c in rows['child'] if c < len(X)]
            if pts and labels[pts[0]] >= 0:
                zero += 1
    return zero


def main():
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 20261004
    clouds = int(sys.argv[2]) if len(sys.argv) > 2 else 40
    rng = np.random.default_rng(seed)
    out = dict(seed=seed, clouds=0, sans_ex_aequo=dict(checks=0, agree=0, disagreements=[]),
               avec_ex_aequo=dict(checks=0, agree=0, disagreements=0, sklearn_zero_stability_selected=0,
                                  points_differents=0), forced=0)
    for c in range(clouds):
        n = int(rng.integers(40, 160))
        g = int(rng.integers(2, 6))
        centers = rng.uniform(0, 10, size=(g, 3))
        lab = rng.integers(0, g, size=n)
        X = centers[lab] + rng.normal(0, rng.uniform(0.3, 1.2), size=(n, 3))
        k = 1 if c % 3 == 0 else int(rng.integers(2, 8))
        U = ml.hdbscan_ultrametric(X, k)
        model = HDBSCAN(min_samples=k, min_cluster_size=2, algorithm='kd_tree', n_jobs=1, copy=True).fit(X)
        values = np.asarray(model._single_linkage_tree_)['value'].tolist()
        tied = any(v > 1 for v in collections.Counter(values).values())
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
            ok = ml.same_partition(ours, theirs)
            diff = sum(1 for a, b in zip(ours, theirs) if (a == -1) != (b == -1))
            if not tied:
                box = out['sans_ex_aequo']
                box['checks'] += 1
                box['agree'] += ok
                if not ok:
                    box['disagreements'].append(dict(cloud=c, n=n, k=k, mcs=mcs, diff=diff))
            else:
                box = out['avec_ex_aequo']
                box['checks'] += 1
                box['agree'] += ok
                if not ok:
                    box['disagreements'] += 1
                    box['points_differents'] += diff
                    box['sklearn_zero_stability_selected'] += sklearn_zero_stability_selected(X, k, mcs)
    print(json.dumps(out, indent=1))


if __name__ == '__main__':
    main()
