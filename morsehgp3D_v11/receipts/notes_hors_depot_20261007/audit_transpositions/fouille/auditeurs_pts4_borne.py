#!/usr/bin/env python3
"""Borne haute du contrat (30 000 a 60 000 sites) : travail des trames c08 sans sol de PTS4.

Lecture seule : `git show origin/main:.../pts4_review_20261003/case_metadata.json.gz` (aucune ecriture Git,
aucun moteur, aucun GCP). Compte, pour les trames c08 de claudepts3/4 (K1..10, export natif, 4 fils par
processus, 22 processus simultanes : les durees ne sont PAS des mesures contractuelles), les sites, les boules
du catalogue K10 et les noeuds de l'ordre 5 (identiques entre une tour K5 et une tour K10, porte de prefixe
Kmax). Reference : 08/000000 sans sol (masque Patchwork++ v8), 39 885 sites, 576 371 noeuds d'ordre 5
(receipts/audit_deep_20261004/performance/README.md) et 5 512 670 boules K10 (v10, S4).
Usage : python3 -B auditeurs_pts4_borne.py
"""
import gzip
import io
import json
import math
import statistics
import subprocess

WORKTREE = '/workspaces/E-HGP/build/v11-claude-20261003'
PATH = 'morsehgp3D_v11/receipts/pts4_review_20261003/case_metadata.json.gz'
REF_SITES, REF_NODES5, REF_BALLS10, REF_FULL_MS = 39885, 576371, 5512670, 412.4


def slope(rows, i, j):
    xs = [math.log(r[i]) for r in rows]
    ys = [math.log(r[j]) for r in rows]
    mx, my = statistics.mean(xs), statistics.mean(ys)
    return sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / sum((x - mx) ** 2 for x in xs)


def main():
    raw = subprocess.run(['git', '-C', WORKTREE, 'show', 'origin/main:' + PATH], check=True,
                         capture_output=True).stdout
    data = json.load(gzip.open(io.BytesIO(raw)))
    frames, cases = {}, []
    for session, items in data.items():
        for item in items:
            try:
                case = json.loads(item['json_text'])
            except (KeyError, ValueError):
                continue
            ex, name = case.get('export', {}), case.get('name', '')
            if ex.get('kmax') != 10 or ex.get('status') != 'ok' or not name.startswith('c08_'):
                continue
            orders = {o['k']: o for o in ex['orders']}
            row = (ex['sites'], ex['balls'], orders[5]['nodes'], ex['full_ns'] / 1e9)
            frames.setdefault(name, row)
            cases.append(row)
    rows = sorted(frames.values())
    print('trames c08 distinctes', len(rows), 'sites', rows[0][0], '-', rows[-1][0])
    print('au-dessus de 60 000 sites :', sum(r[0] > 60000 for r in rows))
    print('pente noeuds ordre 5 ~ n^%.3f (trames)' % slope(rows, 0, 2))
    print('pente boules K10 ~ n^%.3f (cas)' % slope(cases, 0, 1))
    print('pente full_ns K10 ~ n^%.3f (cas, sous contention, non contractuel)' % slope(cases, 0, 3))
    for lo, hi in ((30000, 40000), (40000, 50000), (50000, 60001)):
        sel = [r for r in rows if lo <= r[0] < hi]
        nodes = [r[2] for r in sel]
        print('[%d, %d) : %d trames ; noeuds ordre 5 med %d max %d ; med/ref %.2f max/ref %.2f'
              % (lo, hi, len(sel), statistics.median(nodes), max(nodes),
                 statistics.median(nodes) / REF_NODES5, max(nodes) / REF_NODES5))
    worst = max((r for r in rows if r[0] <= 60000), key=lambda r: r[2])
    name = [k for k, v in frames.items() if v == worst][0]
    factor = worst[2] / REF_NODES5
    print('trame la plus lourde <= 60 000 : %s, %d sites, %d noeuds ordre 5 (x%.2f), %d boules K10 (x%.2f)'
          % (name, worst[0], worst[2], factor, worst[1], worst[1] / REF_BALLS10))
    print('cible effective sur 08/000000 si temps ~ noeuds ordre 5 : %.0f ms ; ecart a la mediane %.1f ms : x%.1f'
          % (100 / factor, REF_FULL_MS, REF_FULL_MS * factor / 100))


if __name__ == '__main__':
    main()
