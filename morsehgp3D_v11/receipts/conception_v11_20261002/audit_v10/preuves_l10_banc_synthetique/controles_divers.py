"""Audit L10 : controles divers du banc synthetique v10 (lecture seule ; rien n'est ecrit).

Usage : python3 controles_divers.py <bench/synthetic epingle (lot C)> <dossier receipts de la v10> <results.csv du lot C>
  1. niveaux de difficulte = echec de HDBSCAN(min_cluster_size=20) au point de calibration (scenes.py:21-58) ;
  2. espaces de graines dev / test / test_v10b disjoints ; CSV dev des recus sans graine de test ;
  3. sklearn : min_samples = 1 et 2 donnent les memes lignes a alpha = 1 (lot C) ;
  4. fixtures de metrics.py ; etiquettes des ponts ; geometrie des centres selon le nombre de groupes ;
  5. sklearn 1.9.1 + numpy 2.5.3 : cluster_selection_epsilon > 0 en selection feuilles leve TypeError.
"""
import csv
import glob
import gzip
import itertools
import os
import sys
import warnings

import numpy as np

warnings.filterwarnings('ignore')
SRC, RECEIPTS, LOTC = sys.argv[1], sys.argv[2], sys.argv[3]
sys.path.insert(0, SRC)
import metrics  # noqa: E402
import run_campaign  # noqa: E402
import scenes  # noqa: E402
from sklearn.cluster import HDBSCAN  # noqa: E402
from sklearn.metrics import adjusted_rand_score  # noqa: E402

print('== 1. calibration des niveaux (ARI, bruit en classe, HDBSCAN(min_cluster_size=20), n=2000, g=8, 3 graines) : mesure [publie]')
pt = scenes.CALIBRATION_POINT
for fam in scenes.FAMILIES:
    out = []
    for lev in scenes.LEVELS:
        s = []
        for seed in pt['seeds'][:3]:
            P, L, _ = scenes.generate(dict(family=fam, n=pt['n'], groups=pt['groups'], level=lev, noise_fraction=0.0, seed=seed))
            s.append(adjusted_rand_score(L, HDBSCAN(min_cluster_size=20, copy=True).fit(P).labels_))
        out.append('%.2f [%.2f]' % (sum(s) / len(s), scenes.CALIBRATION[fam][lev]))
    print('   %-16s %s' % (fam, '  '.join(out)))

print('== 2. graines')
seeds = lambda split, sizes, reps: {s['seed'] for s in run_campaign.plan(split, sizes, reps)}
dev = seeds('dev', [2000, 8000, 16000, 32000], 8)
ta = seeds('test', [8000, 16000, 32000], 5)
tc = seeds('test_v10b', [8000, 16000, 32000], 5)
print('   dev %d, test %d, test_v10b %d ; intersections %d / %d / %d' % (len(dev), len(ta), len(tc), len(dev & ta), len(dev & tc), len(ta & tc)))
bad = files = 0
for p in sorted(glob.glob(os.path.join(RECEIPTS, 'bench_dev_*', '*.csv.gz')) + glob.glob(os.path.join(RECEIPTS, 'bench_dev_*', '*.csv'))):
    op = gzip.open if p.endswith('.gz') else open
    with op(p, 'rt', newline='') as f:
        rd = csv.DictReader(f)
        if 'seed' not in (rd.fieldnames or []):
            continue
        sd = {int(r['seed']) for r in rd if r['seed']}
    files += 1
    bad += len(sd & ta) + len(sd & tc)
print('   CSV dev lus : %d ; graines de test trouvees : %d' % (files, bad))

print('== 3. lot C : hdb_ms1 contre hdb_ms2 (EOM, alpha = 1, meme remplissage)')
d = {}
for r in csv.DictReader(open(LOTC)):
    d[(r['unit'], r['method'])] = r
units = sorted({u for u, _ in d})
for a, b in (('hdb_ms1', 'hdb_ms2'), ('hdb_ms1_nf', 'hdb_ms2_nf')):
    same = sum(all(d[(u, a)][c] == d[(u, b)][c] for c in ('ari_s', 'ari_nc', 'ami_nc', 'coverage', 'clusters')) for u in units)
    print('   %s = %s sur %d scenes sur %d' % (a, b, same, len(units)))

print('== 4. metriques et generateur')
print('   ARI_s([0,0,1,1], [0,0,-1,-1]) = %.6f (attendu 4/7 = %.6f)' % (metrics.scores(np.array([0, 0, 1, 1]), np.array([0, 0, -1, -1]))['ari_s'], 4 / 7))
P, L, _ = scenes.generate(dict(family='bridge', n=2000, groups=8, level='medium', noise_fraction=0.0, seed=1))
print('   bridge : points de pont etiquetes -1 : %d ; etiquetes -2 : %d' % (int((L == -1).sum()), int((L == -2).sum())))
for g in (2, 3, 5, 8, 12, 20):
    c = scenes.centres(g)
    deg = [sum(1 for j in range(g) if j != i and abs(np.linalg.norm(c[i] - c[j]) - 1) < 1e-9) for i in range(g)]
    print('   centres(%2d) : voisins a la distance minimale, en moyenne %.2f' % (g, sum(deg) / g))

print('== 5. sklearn cluster_selection_epsilon')
rng = np.random.default_rng(0)
X = np.floor(np.vstack([rng.normal(0, 1, (300, 3)), rng.normal(6, 1, (300, 3))]) * 1000)
for sel in ('eom', 'leaf'):
    for eps in (0.0, 500.0):
        try:
            m = HDBSCAN(min_cluster_size=10, min_samples=5, cluster_selection_method=sel, cluster_selection_epsilon=eps, algorithm='kd_tree', copy=True).fit(X)
            print('   %s epsilon %g : ok, %d amas' % (sel, eps, len(set(m.labels_.tolist()) - {-1})))
        except Exception as e:
            print('   %s epsilon %g : ECHEC %s : %s' % (sel, eps, type(e).__name__, str(e)[:120]))
