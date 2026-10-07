"""Audit L10 : batterie A/B, K = 5, par facteur : IoU moyen de la tour (par groupe) et ecart apparie par scene
cover - HDBSCAN (moyenne des scenes). Lecture seule de groupes_A_B.csv.gz."""
import csv
import gzip
import sys
from collections import defaultdict

rows = {}
with gzip.open(sys.argv[1], 'rt', newline='') as f:
    for r in csv.DictReader(f):
        if r['mode'] == 'lidar' or r['resolution'] != 'base' or r['plan_block'].startswith('iid'):
            continue
        if r['source'] not in ('ceiling_def8', 'cover', 'hdbscan'):
            continue
        scene = (r['family'], r['level'], r['noise'], r['n'], r['plan_groups'], r['replicate'])
        rows[(scene, r['k'], r['source'], r['group'])] = int(r['iou_num']) / int(r['iou_den'])
per = defaultdict(lambda: defaultdict(list))
for (scene, k, source, group), v in rows.items():
    per[(scene, k)][source].append(v)
print('scenes x K :', len(per))
for k in ('2', '3', '5', '10'):
    d = [sum(s['cover']) / len(s['cover']) - sum(s['hdbscan']) / len(s['hdbscan']) for (sc, kk), s in per.items() if kk == k]
    print('K=%-2s cover - HDBSCAN, moyenne des scenes : %+.4f (%d scenes ; gagnees %d, egales %d, perdues %d)' % (
        k, sum(d) / len(d), len(d), sum(x > 1e-12 for x in d), sum(abs(x) <= 1e-12 for x in d), sum(x < -1e-12 for x in d)))
names = ('famille', 'niveau', 'bruit', 'n', 'groupes')
for idx, name in enumerate(names):
    print('\nK = 5, par %s : IoU moyen de la tour (groupes) | cover - HDBSCAN (scenes)' % name)
    vals = sorted({sc[idx] for (sc, kk) in per}, key=lambda x: (len(x), x))
    for v in vals:
        tw = [x for (sc, kk), s in per.items() if kk == '5' and sc[idx] == v for x in s['ceiling_def8']]
        d = [sum(s['cover']) / len(s['cover']) - sum(s['hdbscan']) / len(s['hdbscan']) for (sc, kk), s in per.items() if kk == '5' and sc[idx] == v]
        print('   %-16s %.3f (%5d groupes) | %+.3f (%4d scenes)' % (v, sum(tw) / len(tw), len(tw), sum(d) / len(d), len(d)))
