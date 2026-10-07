"""Recalcul independant (audit L03) de nombres de tete du rapport de batterie, depuis les tables fusionnees par groupe.
Lecture seule de build/v10-tour-vers-points/batterie_ab/tables/. Aucune coordonnee."""
import gzip, csv, collections, statistics, sys, random
from fractions import Fraction as Fr
B = '/workspaces/E-HGP/build/v10-tour-vers-points/batterie_ab/tables/'
D = collections.defaultdict(dict)   # (scene, k, group) -> source -> Fraction iou
meta = {}
dups = 0; conflicts = 0
with gzip.open(B + 'groupes_A_B.csv.gz', 'rt') as f:
    for r in csv.DictReader(f):
        if r['mode'] != 'sans_map' or r['resolution'] != 'base' or r['level_of_diag'] not in ('A', 'B'):
            continue
        scene = (r['plan_block'], r['family'], r['level'], r['noise'], r['n'], r['plan_groups'], r['replicate'])
        key = (scene, int(r['k']), r['group'])
        iou = Fr(int(r['iou_num']), int(r['iou_den'])) if r['iou_den'] not in ('', '0') else Fr(0)
        if r['source'] in D[key]:
            dups += 1
            if D[key][r['source']] != iou:
                conflicts += 1
        D[key][r['source']] = iou
print('lignes en double :', dups, ' conflits de valeur :', conflicts)
srcs = ['ceiling_def8', 'cover', 'cover1', 'core', 'hdbscan']
for K in (2, 3, 5, 10):
    keys = [k for k in D if k[1] == K]
    scenes = sorted({k[0] for k in keys})
    line = 'K=%-2d groupes=%d scenes=%d :' % (K, len(keys), len(scenes))
    for s in srcs:
        v = [D[k][s] for k in keys]
        line += '  %s exacts=%d >4/5=%.3f IoU=%.3f' % ({'ceiling_def8': 'tour'}.get(s, s), sum(1 for x in v if x == 1), sum(1 for x in v if x > Fr(4, 5)) / len(v), float(sum(v)) / len(v))
    print(line)
    # ecart apparie par scene cover - hdbscan (moyenne par scene de l'IoU moyen des groupes)
    per = collections.defaultdict(lambda: [Fr(0), Fr(0), 0])
    for k in keys:
        p = per[k[0]]
        p[0] += D[k]['cover']; p[1] += D[k]['hdbscan']; p[2] += 1
    diffs = [float((p[0] - p[1]) / p[2]) for p in per.values()]
    m = sum(diffs) / len(diffs)
    rnd = random.Random(20261002)
    bs = []
    for _ in range(2000):
        s = [diffs[rnd.randrange(len(diffs))] for _ in diffs]
        bs.append(sum(s) / len(s))
    bs.sort()
    print('      cover - HDBSCAN apparie par scene : %+0.4f  IC95 bootstrap simple sur les scenes [%+0.4f ; %+0.4f]  scenes +/=/- : %d/%d/%d' % (
        m, bs[50], bs[1949], sum(1 for d in diffs if d > 0), sum(1 for d in diffs if d == 0), sum(1 for d in diffs if d < 0)))
