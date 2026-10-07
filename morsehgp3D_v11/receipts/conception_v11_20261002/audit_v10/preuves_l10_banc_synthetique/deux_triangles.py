"""Audit L10 : fixture des deux triangles equilateraux (these § 6.1). Oracle exhaustif Gamma_2 de la reference v10
(Fraction, 6 points) contre la hierarchie de sklearn.cluster.HDBSCAN a min_samples = 2 (point compte).
Usage : python3 deux_triangles.py <dossier morsehgp3D_v10/reference>"""
import math
import sys
import warnings

import numpy as np

warnings.filterwarnings('ignore')
sys.path.insert(0, sys.argv[1])
import hgp10_ref as R  # noqa: E402

names = 'ABCDEF'
P = [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (4000, 2000, 0), (5732, 3000, 0), (5732, 1000, 0)]
r = 1000.0
for k in (1, 2, 3):
    levels, closed, opened = R.gamma_cuts(P, k)
    print('--- tour exacte (oracle Gamma_%d), coupes fermees : niveau en unites de r = 1000 -> couvertures des composantes' % k)
    last = None
    for a, comps in zip(levels, closed):
        txt = ' | '.join(''.join(names[i] for i in sorted(c)) for c in comps)
        if txt != last:
            print('  r = %.4f : %s' % (math.sqrt(float(a)) / r, txt))
            last = txt
from sklearn.cluster import HDBSCAN
X = np.asarray(P, dtype=np.float64)
for ms in (1, 2, 3):
    m = HDBSCAN(min_cluster_size=2, min_samples=ms, algorithm='kd_tree', copy=True, allow_single_cluster=True).fit(X)
    t = m._single_linkage_tree_
    print('--- sklearn HDBSCAN min_samples = %d : fusions (distance de lien / (2 r), taille) :' % ms,
          ', '.join('%.4f (%d)' % (v / (2 * r), s) for v, s in zip(t['value'], t['cluster_size'])))
    for mcs in (2, 3):
        lab = HDBSCAN(min_cluster_size=mcs, min_samples=ms, algorithm='kd_tree', copy=True).fit(X).labels_
        print('     etiquettes plates, mcs = %d, EOM : %s' % (mcs, dict(zip(names, lab.tolist()))))
