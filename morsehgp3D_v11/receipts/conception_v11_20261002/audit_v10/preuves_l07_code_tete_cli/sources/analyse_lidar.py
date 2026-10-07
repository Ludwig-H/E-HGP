"""Audit L07 : depouille les sorties de la sonde sur les trames LiDAR du contrat (pas de verite terrain ici : on
mesure l'ecart ENTRE la tete publiee V et la tete a cohortes N)."""
import json
import os
import sys

import numpy as np
from sklearn.metrics import adjusted_rand_score

CFG = [l.split() for l in open(sys.argv[1] + '/cfg.txt')]


def singletons(l):
    l = l.copy()
    noise = l < 0
    l[noise] = l.max() + 1 + np.arange(noise.sum())
    return l


print('%-6s %-5s %2s %-14s %7s %6s %6s %6s %6s %6s %8s %8s %7s %5s %9s %9s %8s %8s' % (
    'trame', 'src', 'K', 'tete', 'noeuds', 'clust', 'selV', 'selN', 'decl', 'stabX', 'pts_chg', 'ARI(V,N)', 'bruitV', 'prof', 'marche_pt', 'marche_cl', 't_dendro', 't_tete'))
for frame in sys.argv[2:]:
    for line in open(os.path.join(sys.argv[1], frame + '.jsonl')):
        if not line.startswith('{"source"'):
            if line.startswith('{'):
                print(frame, line.strip())
            continue
        r = json.loads(line)
        if 'cfg' not in r:
            print(frame, line.strip())
            continue
        c = CFG[r['cfg']]
        ari, chg = 1.0, 0
        base = os.path.join(sys.argv[1], '%s.%s.k%d.%d' % (frame, r['source'], r['k'], r['cfg']))
        if r['lab_vn'] > 0:
            V = np.fromfile(base + '.v', dtype='<i4').astype(np.int64)
            N = np.fromfile(base + '.n', dtype='<i4').astype(np.int64)
            ari = adjusted_rand_score(singletons(V), singletons(N))
            # points dont le groupe change : appariement des etiquettes par recouvrement maximal
            pairs = {}
            for a, b in zip(V, N):
                pairs[(a, b)] = pairs.get((a, b), 0) + 1
            best = {}
            for (a, b), m in pairs.items():
                if a >= 0 and (a not in best or m > best[a][1]):
                    best[a] = (b, m)
            chg = int(sum(m for (a, b), m in pairs.items() if (a < 0) != (b < 0) or (a >= 0 and best[a][0] != b)))
        print('%-6s %-5s %2d %-14s %7d %6d %6d %6d %6d %6d %8d %8.4f %7d %5d %9d %9d %8.4f %8.5f' % (
            frame, r['source'], r['k'], '%s mcs=%s z=%s' % (c[2], c[0], c[1][:1]), r['nodes'], r['clusters'], r['sel_v'], r['sel_n'],
            r['trig_c'], r['stab_changed'], chg, ari, r['noise_v'], r['depth'], r['walk_plab'], r['walk_clab'], r['t_dendro'], r['t_head_v']))
