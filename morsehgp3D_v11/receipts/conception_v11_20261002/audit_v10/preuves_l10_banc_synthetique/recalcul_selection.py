"""Audit L10 : recoupe independante de l'etage selection (niveau C, dev) depuis tvpc.flat.csv.gz (lecture en flux).
mIoU moyen pondere par cellule (bloc, famille, niveau, bruit, n) de quelques configurations, K = 5 et 10 ; puis la
meme lecture par mcs a z fixe (EOM, sans epsilon ni remplissage), toutes les K.
Usage : python3 recalcul_selection.py <tvpc.flat.csv.gz>"""
import csv
import gzip
import sys
from collections import defaultdict

want = set()
for k in (2, 3, 5, 10):
    for m in ('mK', 'm10', 'm20', 'm40', 'msqrt'):
        want.add('hdb|K%d|%s|eom|e0|none' % (k, m))
        want.add('hdb|K%d|%s|eom|e0|b2' % (k, m))
        want.add('hdb|K%d|%s|leaf|e1|b2' % (k, m))
        for z in (1, 2, 3):
            want.add('cover|K%d|%s|eom|z%d|e0|none' % (k, m, z))
            want.add('cover|K%d|%s|eom|z%d|e0|b2' % (k, m, z))
acc = defaultdict(lambda: defaultdict(list))
scenes = set()
with gzip.open(sys.argv[1], 'rt', newline='') as f:
    rd = csv.reader(f)
    head = next(rd)
    ix = {c: i for i, c in enumerate(head)}
    for r in rd:
        if r[ix['kind']] != 'nat' or r[ix['resolution']] != 'base':
            continue
        cfg = r[ix['config']]
        if cfg not in want:
            continue
        cell = (r[ix['plan_block']], r[ix['family']], r[ix['level']], r[ix['noise']], r[ix['n']])
        acc[cfg][cell].append(float(r[ix['miou']]))
        scenes.add(r[ix['unit']])


def J(cfg):
    c = acc.get(cfg)
    if not c:
        return float('nan'), 0
    return sum(sum(v) / len(v) for v in c.values()) / len(c), sum(len(v) for v in c.values())


print('scenes synthetiques lues :', len(scenes))
print('\nmIoU pondere par cellule, memes scenes (768 par K) :')
for k in (2, 3, 5, 10):
    print('K=%-2d hdb sqrt leaf e1 b2 %.4f | hdb sqrt eom e0 b2 %.4f | hdb m40 eom e0 b2 %.4f | cover m40 eom z3 e0 b2 %.4f | cover sqrt eom z3 e0 b2 %.4f  (scenes %d)' % (
        k, J('hdb|K%d|msqrt|leaf|e1|b2' % k)[0], J('hdb|K%d|msqrt|eom|e0|b2' % k)[0], J('hdb|K%d|m40|eom|e0|b2' % k)[0],
        J('cover|K%d|m40|eom|z3|e0|b2' % k)[0], J('cover|K%d|msqrt|eom|z3|e0|b2' % k)[0], J('cover|K%d|m40|eom|z3|e0|b2' % k)[1]))
print('\nEOM, sans epsilon ni remplissage : mIoU de HDBSCAN / cover z=1 / cover z=2 / cover z=3, par mcs')
for k in (2, 3, 5, 10):
    for m in ('mK', 'm10', 'm20', 'm40', 'msqrt'):
        print('K=%-2d %-5s  %.3f / %.3f / %.3f / %.3f' % (k, m, J('hdb|K%d|%s|eom|e0|none' % (k, m))[0],
                                                       J('cover|K%d|%s|eom|z1|e0|none' % (k, m))[0],
                                                       J('cover|K%d|%s|eom|z2|e0|none' % (k, m))[0],
                                                       J('cover|K%d|%s|eom|z3|e0|none' % (k, m))[0]))
