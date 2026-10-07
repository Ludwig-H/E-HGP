"""Audit L06 : juge independant a l'echelle pour l'ordre K = 1 (prevu par la conception, absent du depot).

L_1(a) = union des boules de rayon sqrt(a) : ses composantes fusionnent quand deux boules se touchent, a = d^2 / 4.
La foret d'ordre 1 est donc le dendrogramme de liaison simple : le multiensemble des niveaux de fusion, compte
(enfants - 1) fois, egale le multiensemble des d^2 / 4 des aretes d'un arbre couvrant minimal euclidien (unique comme
multiensemble de poids). EMST independant : aretes de Delaunay (scipy/Qhull), poids entiers exacts, Kruskal de scipy.
Usage : python3 emst_judge.py IN.u32le DUMP
"""
import sys
from collections import Counter
from fractions import Fraction

import numpy as np
from scipy.sparse import coo_matrix
from scipy.sparse.csgraph import minimum_spanning_tree
from scipy.spatial import Delaunay

pts = np.fromfile(sys.argv[1], dtype='<u4').reshape(-1, 3).astype(np.int64)
n = len(pts)
tri = Delaunay(pts.astype(np.float64))
s = tri.simplices
e = np.vstack([s[:, [0, 1]], s[:, [0, 2]], s[:, [0, 3]], s[:, [1, 2]], s[:, [1, 3]], s[:, [2, 3]]])
e.sort(axis=1)
e = np.unique(e, axis=0)
d = pts[e[:, 0]] - pts[e[:, 1]]
w = (d * d).sum(axis=1)  # entier exact < 2^40
assert w.max() < 2 ** 52
g = coo_matrix((w.astype(np.float64), (e[:, 0], e[:, 1])), shape=(n, n)).tocsr()
mst = minimum_spanning_tree(g).tocoo()
assert mst.nnz == n - 1, (mst.nnz, n)
want = Counter(int(x) for x in mst.data)  # d^2 des aretes de l'EMST

# foret d'ordre 1 du dump
parent, level = [], []
with open(sys.argv[2]) as fh:
    for line in fh:
        t = line.split()
        if t[0] == 'order':
            if int(t[1]) != 1:
                break
        elif t[0] == 'node':
            parent.append(int(t[2]))
            level.append(Fraction(int(t[3]), int(t[4])))
kids = Counter(p for p in parent if p >= 0)
got = Counter()
for v, c in kids.items():
    d2 = level[v] * 4
    assert d2.denominator == 1
    got[int(d2)] += c - 1
print('sites %d ; aretes de Delaunay %d ; aretes EMST %d ; fusions d\'ordre 1 : %d (dont %d a plus de 2 enfants)' %
      (n, len(e), mst.nnz, len(kids), sum(1 for c in kids.values() if c > 2)))
print('multiensemble des niveaux de fusion (x4) == multiensemble des d^2 de l\'EMST : %s' % (got == want))
print('niveaux distincts %d ; somme des multiplicites %d' % (len(got), sum(got.values())))
sys.exit(0 if got == want else 1)
