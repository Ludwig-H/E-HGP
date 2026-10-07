import os, subprocess, sys, tempfile
import numpy as np
from sklearn.cluster import HDBSCAN
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import oracle_condense as oc
from cross_check import CLI, same_partition
rng = np.random.default_rng(5)
with tempfile.TemporaryDirectory() as tmp:
    for t in range(120):
        n = int(rng.integers(12, 61))
        P = np.unique(rng.integers(0, 4000, size=(n, 3)), axis=0)
        if t != 119:
            continue
        src = os.path.join(tmp, 'in.u32le')
        np.ascontiguousarray(P, dtype='<u4').tofile(src)
        mcs, z = 3, 2
        out, tree = os.path.join(tmp, 'v'), os.path.join(tmp, 'tree')
        subprocess.run([CLI, src, out, '--k=2', '--mcs=%d' % mcs, '--z=%d' % z, '--entry=core', '--threads=1', '--tree=' + tree], capture_output=True, text=True)
        o = oc.run(tree, mcs, z, True, False)
        levels, nodes, pts, D = o['levels'], o['nodes'], o['pts'], o['D']
        S = HDBSCAN(min_cluster_size=mcs, min_samples=1, metric='precomputed', cluster_selection_method='eom', copy=True).fit(
            np.array([[0.0 if i == j else levels[D[i][j]] ** (z / 2.0) for j in range(len(P))] for i in range(len(P))])).labels_
        c2 = [i for i, l in enumerate(o['labels']) if l == 2]
        s2 = [i for i, l in enumerate(S) if l == 2]
        print('O cluster 2', c2, ' S cluster 2', s2)
        for i in sorted(set(c2) | set(s2)):
            print(' point', i, 'noeud', pts[i][1], 'rang entree', pts[i][2], 'niveau', levels[pts[i][2]], 'rang du noeud', nodes[pts[i][1]][0], 'parent', nodes[pts[i][1]][1],
                  'sortie oracle (cluster, rang)', o['out'][i])
        ids = sorted(set(c2) | set(s2))
        print('rangs d(x,y) :')
        for i in ids:
            print('  ', i, [D[i][j] for j in ids])
        # voisinage : plus petite distance de 36 et 46 a tout autre point
        for i in (36, 46):
            row = sorted((D[i][j], j) for j in range(len(P)) if j != i)[:5]
            print(' plus proches (rang, point) de', i, row)
        for c, cl in enumerate(o['clusters']):
            print('cluster', c, 'parent', cl['parent'], 'naissance rang', cl['birth'], 'membres', cl['members'], 'sorties', cl['exits'])
