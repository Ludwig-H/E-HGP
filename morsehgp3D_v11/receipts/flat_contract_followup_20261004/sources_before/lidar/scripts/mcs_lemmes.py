#!/usr/bin/env python3
"""Consequences des lemmes L1/L2 (RAPPORT § 4.3) sur les instances evaluables du criblage (>= 50 points, sans sol).
L1 : si n_g <= mcs/2, aucun cluster de l'arbre condense a mcs ne peut apparier g (IoU > 1/2), pour un bloc sans void.
L2 : si mcs <= floor(n_g/2) + 1, tout bloc qui apparie g a au moins mcs sites : la condensation n'en retire aucun.
Pour chaque mcs (et mcs = round(sqrt(n)) par trame, n = sites sans sol), part des instances >= 50 points rendues
inappariables (L1) et part hors de la zone sure (L2 non garanti).
    PYTHONDONTWRITEBYTECODE=1 python3 -B scripts/mcs_lemmes.py > sorties/mcs_lemmes.txt"""
import json, math
CRIBLAGE = '/workspaces/E-HGP/Zoltan/demos/recherche/criblage_08.jsonl'
SMALL = ('bicycle', 'person', 'motorcycle', 'bicyclist', 'motorcyclist')
rows = []
for line in open(CRIBLAGE):
    f = json.loads(line)
    if f.get('origin') != 'criblage_1_sur_8' or 'instances' not in f:
        continue
    for i in f['instances']:
        if i['points'] >= 50:
            rows.append((f['n_without_ground'], i['points'], i['cls']))
print('instances >= 50 points : %d (dont petites classes %d)' % (len(rows), sum(1 for r in rows if r[2] in SMALL)))
print('%-12s %-26s %-26s' % ('mcs', 'inappariables (L1) tout/petits', 'hors zone sure (L2) tout/petits'))
for label, f in [('%d' % m, (lambda n, m=m: m)) for m in (5, 10, 20, 26, 30, 40, 50, 100)] + [('round(sqrt n)', lambda n: round(math.sqrt(n)))]:
    a = [r for r in rows]
    s = [r for r in rows if r[2] in SMALL]
    l1 = lambda g: sum(1 for n, p, c in g if p <= f(n) / 2) / len(g)
    l2 = lambda g: sum(1 for n, p, c in g if f(n) > p // 2 + 1) / len(g)
    print('%-12s %5.1f %% / %5.1f %%            %5.1f %% / %5.1f %%' % (label, 100 * l1(a), 100 * l1(s), 100 * l2(a), 100 * l2(s)))
ns = sorted(set(r[0] for r in rows))
print('sqrt(n) par trame : de %d a %d' % (round(math.sqrt(ns[0])), round(math.sqrt(ns[-1]))))
