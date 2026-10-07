"""Audit L07 : quand sklearn (metric='precomputed' sur l'ultrametrique des points) peut-il servir d'oracle ?
Regime sans plateau dangereux : K = 2, entree core, grille large (position quasi generale), mcs >= 3 (les cohortes
d'entree sont des paires de plus proches voisins mutuels, de masse 2 < mcs). On compare l'oracle par coupes strictes
(O) et sklearn (S), racine exclue ; puis le meme tirage a mcs = 2, ou sklearn binarise les egalites.
"""
import json
import os
import subprocess
import sys
import tempfile

import numpy as np
from sklearn.cluster import HDBSCAN

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import oracle_condense as oc  # noqa: E402
from cross_check import CLI, same_partition  # noqa: E402


def main():
    rng = np.random.default_rng(int(sys.argv[2]))
    res = {}
    with tempfile.TemporaryDirectory() as tmp:
        for t in range(int(sys.argv[1])):
            n = int(rng.integers(12, 61))
            P = np.unique(rng.integers(0, 4000, size=(n, 3)), axis=0)
            src = os.path.join(tmp, 'in.u32le')
            np.ascontiguousarray(P, dtype='<u4').tofile(src)
            for mcs in (2, 3, 5):
                for z in (1, 2):
                    out, tree = os.path.join(tmp, 'v'), os.path.join(tmp, 'tree')
                    r = subprocess.run([CLI, src, out, '--k=2', '--mcs=%d' % mcs, '--z=%d' % z, '--entry=core',
                                        '--threads=1', '--tree=' + tree], capture_output=True, text=True)
                    if r.returncode != 0:
                        continue
                    V = np.fromfile(out, dtype='<i4')
                    o = oc.run(tree, mcs, z, True, False)
                    levels = o['levels']
                    D = np.array([[0.0 if i == j else levels[o['D'][i][j]] ** (z / 2.0) for j in range(len(P))]
                                  for i in range(len(P))])
                    S = HDBSCAN(min_cluster_size=mcs, min_samples=1, metric='precomputed', cluster_selection_method='eom',
                                copy=True).fit(D).labels_
                    a = res.setdefault('mcs=%d' % mcs, dict(cas=0, S_eq_O=0, V_eq_O=0, V_eq_S=0))
                    a['cas'] += 1
                    a['S_eq_O'] += same_partition(S, o['labels'])
                    a['V_eq_O'] += same_partition(V, o['labels'])
                    a['V_eq_S'] += same_partition(V, S)
    print(json.dumps(res, sort_keys=True))


if __name__ == '__main__':
    main()
