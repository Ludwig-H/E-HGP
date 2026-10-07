"""Audit L10 : recoupe independante des moyennes de tete de la batterie A/B (tour puis hierarchies) depuis la table
fusionnee groupes_A_B.csv.gz (lecture seule). Usage : python3 recalcul_batterie_ab.py <groupes_A_B.csv.gz>"""
import csv
import gzip
import sys
from collections import Counter, defaultdict
from fractions import Fraction

path = sys.argv[1]
seen = {}
combos = Counter()
with gzip.open(path, 'rt', newline='') as f:
    for r in csv.DictReader(f):
        combos[(r['lot'], r['mode'], r['level_of_diag'], r['source'], r['resolution'])] += 1
        if r['mode'] == 'lidar':
            continue
        key = (r['family'], r['level'], r['noise'], r['n'], r['plan_groups'], r['replicate'],
               'iid' if r['plan_block'].startswith('iid') else 'eg', r['k'], r['source'], r['resolution'], r['group'])
        val = (int(r['iou_num']), int(r['iou_den']), r['precision'], r['recall'], r['lot'])
        if key in seen:
            if seen[key][:2] != val[:2]:
                print('DOUBLON DIFFERENT', key, seen[key], val)
            continue
        seen[key] = val
print('combinaisons (lot, mode, niveau, source, resolution) :')
for k, v in sorted(combos.items()):
    print('  ', k, v)
agg = defaultdict(lambda: [0, 0, 0, 0, Fraction(0)])
for key, (num, den, p, rc, lot) in seen.items():
    fam, lev, noise, n, g, rep, blk, k, source, res, grp = key
    if blk != 'eg':
        continue
    a = agg[(res, k, source)]
    a[0] += 1
    a[1] += int(num == den)
    a[2] += int(2 * num > den)
    a[3] += int(5 * num > 4 * den)
    a[4] += Fraction(num, den)
print('\nresolution K source : cibles, exactes, part > 1/2, part > 4/5, IoU moyen')
for (res, k, source), a in sorted(agg.items(), key=lambda kv: (kv[0][0], int(kv[0][1]), kv[0][2])):
    print('%-7s K=%-2s %-14s %6d %6d %.3f %.3f %.4f' % (res, k, source, a[0], a[1], a[2] / a[0], a[3] / a[0],
                                                       float(a[4] / a[0])))
