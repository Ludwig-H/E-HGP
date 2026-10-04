#!/usr/bin/env python3
"""Validation de la tete commune (condensation N-aire, EOM, feuilles, etiquetage) : appliquee a l'arbre du lien
simple de HDBSCAN avec lambda = 1/r et une fusion binaire par pas (l'ordre de scikit-learn), elle doit rendre les
etiquettes de sklearn.cluster.HDBSCAN (min_samples = k, min_cluster_size = mcs, EOM et feuilles), a permutation pres.
    PYTHONDONTWRITEBYTECODE=1 python3 -B scripts/selftest_tete.py > sorties/selftest_tete.txt"""
import sys, os, warnings
warnings.filterwarnings('ignore')
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
from sklearn.cluster import HDBSCAN
import sklearn
import lidar_lib as L

total = bad = 0
rows = []
for s in range(60):
    rng = np.random.RandomState(1000 + s)
    n0 = rng.randint(30, 120)
    X = np.concatenate([rng.randn(n0 // 3, 3) * rng.uniform(0.2, 0.6) + rng.randn(3) * 3 for _ in range(3)]
                       + [rng.uniform(-6, 6, size=(n0 // 10, 3))])
    n = len(X)
    for k in (1, 2, 3, 5):
        tree = L.hdbscan_tree(X, k)
        hier = L.hier_from_linkage('hdbscan', tree, n, group_ties=False)
        for mcs in (2, 3, 5, 8):
            for method in ('eom', 'leaf'):
                ref = HDBSCAN(min_samples=k, min_cluster_size=mcs, cluster_selection_method=method,
                              copy=True).fit(X).labels_
                got, _ = L.flat(hier, mcs, 1, method)
                total += 1
                ok = L.same_partition(ref, got)
                if not ok:
                    bad += 1
                    rows.append((s, k, mcs, method, int((ref >= 0).sum()), int((got >= 0).sum())))
print('scikit-learn', sklearn.__version__)
print('comparaisons', total, 'ecarts', bad)
for r in rows[:30]:
    print('  ecart graine %d k=%d mcs=%d %s : non-bruit sklearn %d, tete %d' % r)
