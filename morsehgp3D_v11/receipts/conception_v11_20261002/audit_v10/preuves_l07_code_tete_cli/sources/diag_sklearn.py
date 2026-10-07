import json, os, subprocess, sys, tempfile
import numpy as np
from collections import Counter
from sklearn.cluster import HDBSCAN
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import oracle_condense as oc
from cross_check import CLI, same_partition

rng = np.random.default_rng(5)
shown = 0
with tempfile.TemporaryDirectory() as tmp:
    for t in range(300):
        n = int(rng.integers(12, 61))
        P = np.unique(rng.integers(0, 4000, size=(n, 3)), axis=0)
        src = os.path.join(tmp, 'in.u32le')
        np.ascontiguousarray(P, dtype='<u4').tofile(src)
        for mcs in (2, 3, 5):
            for z in (1, 2):
                if mcs == 2:
                    continue
                out, tree = os.path.join(tmp, 'v'), os.path.join(tmp, 'tree')
                r = subprocess.run([CLI, src, out, '--k=2', '--mcs=%d' % mcs, '--z=%d' % z, '--entry=core', '--threads=1', '--tree=' + tree], capture_output=True, text=True)
                o = oc.run(tree, mcs, z, True, False)
                levels = o['levels']
                D = np.array([[0.0 if i == j else levels[o['D'][i][j]] ** (z / 2.0) for j in range(len(P))] for i in range(len(P))])
                S = HDBSCAN(min_cluster_size=mcs, min_samples=1, metric='precomputed', cluster_selection_method='eom', copy=True).fit(D).labels_
                if same_partition(S, o['labels']):
                    continue
                shown += 1
                print('--- cas', t, 'n', len(P), 'mcs', mcs, 'z', z)
                print('O', o['labels']); print('S', S.tolist())
                # cohortes : rangs d'entree partages par >= 3 points du meme noeud, ou entree == rang d'un noeud
                ent = Counter((p[1], p[2]) for p in o['pts'])
                print('cohortes >= 3 :', {k: v for k, v in ent.items() if v >= 3})
                node_ranks = set(v[0] for v in o['nodes'])
                print('entrees egales a un rang de noeud :', [(p[0], p[1], p[2]) for p in o['pts'] if p[2] in node_ranks and o['nodes'][p[1]][0] != p[2]][:6],
                      '; entrees au rang de creation de leur noeud :', sum(1 for p in o['pts'] if o['nodes'][p[1]][0] == p[2]))
                m = len(o['clusters'])
                kids = [[] for _ in range(m)]
                for c in range(1, m): kids[o['clusters'][c]['parent']].append(c)
                for c in range(m):
                    cl = o['clusters'][c]
                    print('  cluster', c, 'parent', cl['parent'], 'taille', len(cl['members']), 'S', float(o['S'][c]), 'retenu', o['chosen'][c], 'enfants', kids[c])
