#!/usr/bin/env python3
"""Pilote local (n = 2 000, petits nuages) : variance des differences appariees au niveau C (partition plate) contre
le niveau B (meilleur bloc), sur des paires de methodes HDBSCAN (sklearn tel quel), pour dimensionner le protocole.

    python3 -B pilote_niveau_C.py > sorties/pilote_niveau_C.txt   (ecrit aussi sorties/pilote_niveau_C.json)

Ce pilote ne mesure NI la tour NI une pente : n = 2 000 est hors des tailles d'interet. Il sert a une seule chose :
estimer le rapport sd(niveau C) / sd(niveau B) des differences par scene entre deux hierarchies voisines
(HDBSCAN a deux min_samples) sous une meme selection, rapport qu'on applique ensuite aux ecarts-types mesures sur
G4 (tour contre HDBSCAN, niveau B, n = 8 000) pour une premiere estimation du nombre de scenes. Le protocole impose
de remesurer sd(niveau C) sur G4 (scenes dev) avant de figer le nombre de scenes du test.

Scenes : generateur v10 epingle, 8 familles x {medium, hard} x {3, 8} groupes x 4 graines dev (7001..7004, hors
des graines 9341/9342 des sessions G4 et de tout espace de test), bruit 5 %, quantification 18 bits du banc v11.
"""
import json
import math
import os
import random
import sys
import time
import warnings
from collections import defaultdict

import numpy as np
from sklearn.cluster import HDBSCAN
import sklearn.cluster._hdbscan.hdbscan as skh

import carte_map as cm
import metriques as mt
import vendor_scenes_v10_pin as vs

warnings.filterwarnings('ignore', category=FutureWarning)
HERE = os.path.dirname(os.path.abspath(__file__))
ORDERS = (2, 3, 5, 10)
SEEDS = (7001, 7002, 7003, 7004)


def best_block(tree, n, truth):
    """Meilleur IoU de chaque groupe parmi les blocs de l'arbre du lien simple, plateaux fermes (fusions ex aequo
    lues ensemble), toutes les feuilles presentes au niveau 0 (meme semantique que points_hierarchy.Evaluator)."""
    groups = int(truth.max()) + 1 if (truth >= 0).any() else 0
    total = np.bincount(truth[truth >= 0], minlength=groups)
    parent = list(range(2 * n - 1))
    size = [1] * n + [0] * (n - 1)
    counts = [({int(t): 1} if t >= 0 else {}) for t in truth] + [None] * (n - 1)
    best = [0.0] * groups
    for i in range(n):  # singletons
        t = int(truth[i])
        if t >= 0:
            best[t] = max(best[t], 1.0 / total[t])

    def find(x):
        root = x
        while parent[root] != root:
            root = parent[root]
        while parent[x] != root:
            parent[x], x = root, parent[x]
        return root

    left, right, value = tree['left_node'].tolist(), tree['right_node'].tolist(), tree['value'].tolist()
    j = 0
    while j < len(value):
        level = value[j]
        touched = set()
        while j < len(value) and value[j] == level:
            a, b = find(left[j]), find(right[j])
            node = n + j
            if len(counts[a] or ()) < len(counts[b] or ()):
                a, b = b, a
            ca = counts[a] if counts[a] is not None else {}
            for g, c in (counts[b] or {}).items():
                ca[g] = ca.get(g, 0) + c
            parent[a] = node
            parent[b] = node
            parent[node] = node
            size[node] = size[a] + size[b]
            counts[node] = ca
            counts[a] = counts[b] = None
            touched.discard(a)
            touched.discard(b)
            touched.add(node)
            j += 1
        for r in touched:
            r = find(r)
            for g, c in (counts[r] or {}).items():
                iou = c / (size[r] + total[g] - c)
                if iou > best[g]:
                    best[g] = iou
    return best


def scene_specs():
    for family in vs.FAMILIES:
        for level in ('medium', 'hard'):
            for groups in (3, 8):
                for seed in SEEDS:
                    yield dict(family=family, level=level, groups=groups, n=2000, noise_fraction=0.05, seed=seed)


def run():
    rows = []
    t_start = time.time()
    for spec in scene_specs():
        sc = cm.scene(spec)
        X = sc['grid'].astype(np.float64)
        truth = sc['truth']
        n = len(X)
        mcs_list = lambda k: sorted({k, 10, 20, int(round(math.sqrt(n)))})
        rec = dict(spec=spec, n=n, map=mt.scores(truth, sc['map'], with_ami=False), orders={})
        for k in ORDERS:
            model = HDBSCAN(min_samples=k, min_cluster_size=2, algorithm='kd_tree', n_jobs=1, copy=True).fit(X)
            tree = model._single_linkage_tree_
            bb = best_block(tree, n, truth)
            row = dict(level_b=float(np.mean(bb)), flat={})
            for mcs in mcs_list(k):
                for sel in ('eom', 'leaf'):
                    labels = skh.tree_to_labels(tree, mcs, sel, False, 0.0, None)[0]
                    s = mt.scores(truth, labels, seg_threshold=mcs, with_ami=False)
                    s_map = mt.scores(sc['map'], labels, with_ami=False)  # MAP comme reference
                    row['flat']['%d_%s' % (mcs, sel)] = {key: s[key] for key in (
                        'miou_h', 'miou_best', 'pq', 'rq', 'sq', 'ari_s', 'ari_nc', 'f_pur', 'clusters',
                        'noise_pred', 'coverage', 'absorbed', 'over_seg', 'under_seg')}
                    row['flat']['%d_%s' % (mcs, sel)]['miou_h_vs_map'] = s_map['miou_h']
            rec['orders'][str(k)] = row
        rows.append(rec)
    return rows, time.time() - t_start


def mean(x):
    return sum(x) / len(x)


def sd(x):
    m = mean(x)
    return math.sqrt(sum((v - m) ** 2 for v in x) / (len(x) - 1))


def within_sd(pairs):
    """sd intra-cellule (cellule = famille x niveau x groupes ; graines repetees)."""
    cells = defaultdict(list)
    for key, v in pairs:
        cells[key].append(v)
    ss = sum(sum((v - mean(g)) ** 2 for v in g) for g in cells.values() if len(g) > 1)
    dof = sum(len(g) - 1 for g in cells.values() if len(g) > 1)
    return math.sqrt(ss / dof)


def report(rows, seconds, out):
    print('pilote : %d scenes n = 2000, %.1f s' % (len(rows), seconds), file=out)
    print('\n== moyennes par configuration (HDBSCAN sklearn tel quel ; niveau B = meilleur bloc de l arbre)', file=out)
    print('k   config     miou_h  miou_best  pq     ari_s  f_pur  clusters  bruit  miou_vs_MAP | niveau B', file=out)
    for k in ORDERS:
        cfgs = sorted(rows[0]['orders'][str(k)]['flat'], key=lambda c: (int(c.split('_')[0]), c.split('_')[1]))
        for c in cfgs:
            v = [r['orders'][str(k)]['flat'][c] for r in rows]
            print('%-3d %-10s %.4f  %.4f     %.4f %.4f %.4f %8.1f  %.3f  %.4f      | %.4f' % (
                k, c, mean([x['miou_h'] for x in v]), mean([x['miou_best'] for x in v]), mean([x['pq'] for x in v]),
                mean([x['ari_s'] for x in v]), mean([x['f_pur'] for x in v]), mean([x['clusters'] for x in v]),
                mean([x['noise_pred'] for x in v]), mean([x['miou_h_vs_map'] for x in v]),
                mean([r['orders'][str(k)]['level_b'] for r in rows])), file=out)
    print('MAP contre verite : miou_h %.4f pq %.4f ari_s %.4f' % (
        mean([r['map']['miou_h'] for r in rows]), mean([r['map']['pq'] for r in rows]),
        mean([r['map']['ari_s'] for r in rows])), file=out)
    print('\n== paires de methodes : ecart-type des differences par scene, niveau B contre niveau C (miou_h)', file=out)
    print('paire (k_a - k_b, config)   moy_B    sd_B    sdintra_B | moy_C    sd_C    sdintra_C | sd_C/sd_B  '
          'sdintra_C/sdintra_B', file=out)
    ratios = []
    for ka, kb in ((3, 2), (5, 2), (5, 3), (10, 5), (10, 2)):
        for c in ('10_eom', '20_eom', '45_eom', '10_leaf', '20_leaf'):
            dB, dC = [], []
            for r in rows:
                cell = (r['spec']['family'], r['spec']['level'], r['spec']['groups'])
                dB.append((cell, r['orders'][str(ka)]['level_b'] - r['orders'][str(kb)]['level_b']))
                dC.append((cell, r['orders'][str(ka)]['flat'][c]['miou_h'] - r['orders'][str(kb)]['flat'][c]['miou_h']))
            vb, vc = [v for _, v in dB], [v for _, v in dC]
            sb, sc_, wb, wc = sd(vb), sd(vc), within_sd(dB), within_sd(dC)
            ratios.append((sc_ / sb, wc / wb))
            print('%2d - %-2d %-9s            %+.4f  %.4f  %.4f    | %+.4f  %.4f  %.4f    | %5.2f      %5.2f' % (
                ka, kb, c, mean(vb), sb, wb, mean(vc), sc_, wc, sc_ / sb, wc / wb), file=out)
    rs = sorted(r[0] for r in ratios)
    rw = sorted(r[1] for r in ratios)
    print('rapport sd_C/sd_B : mediane %.2f, min %.2f, max %.2f ; intra-cellule : mediane %.2f, min %.2f, max %.2f' % (
        rs[len(rs) // 2], rs[0], rs[-1], rw[len(rw) // 2], rw[0], rw[-1]), file=out)
    print('\n== selection seule (meme arbre) : EOM contre feuilles, mcs 20 ; mcs 10 contre 20 (EOM)', file=out)
    for k in ORDERS:
        for a, b in (('20_eom', '20_leaf'), ('10_eom', '20_eom'), ('%d_eom' % k, '20_eom')):
            if a not in rows[0]['orders'][str(k)]['flat']:
                continue
            d = [(((r['spec']['family'], r['spec']['level'], r['spec']['groups'])),
                  r['orders'][str(k)]['flat'][a]['miou_h'] - r['orders'][str(k)]['flat'][b]['miou_h']) for r in rows]
            v = [x for _, x in d]
            print('k=%-2d %-8s - %-8s moy %+.4f sd %.4f sd_intra %.4f' % (k, a, b, mean(v), sd(v), within_sd(d)),
                  file=out)
    print('\n== distribution des differences niveau C (k=5 - k=2, 20_eom) : quantiles', file=out)
    v = sorted(r['orders']['5']['flat']['20_eom']['miou_h'] - r['orders']['2']['flat']['20_eom']['miou_h'] for r in rows)
    print('  ' + ' '.join('q%02d=%+.3f' % (int(100 * p), v[min(len(v) - 1, int(p * len(v)))])
                          for p in (0.0, 0.05, 0.1, 0.25, 0.5, 0.75, 0.9, 0.95, 0.99)), file=out)
    zero = sum(1 for x in v if abs(x) < 1e-12)
    print('  differences exactement nulles : %d / %d' % (zero, len(v)), file=out)


def main():
    rows, seconds = run()
    with open(os.path.join(HERE, 'sorties', 'pilote_niveau_C.json'), 'w') as f:
        json.dump(rows, f, sort_keys=True)
    report(rows, seconds, sys.stdout)


if __name__ == '__main__':
    main()
