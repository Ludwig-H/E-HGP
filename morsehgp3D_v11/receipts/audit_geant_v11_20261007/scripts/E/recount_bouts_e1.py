"""Recomptes en lecture seule (aucune ecriture) pour le rapport E.

1. Zoltan/demos/bouts_evalues.json : categories, issues par ordre, bouts a gain ET perte.
2. Recu e1_sortie_plate/etude : tableau du critere ecrit d'avance (k=all et par k), oracle d'antichaine.
3. (hors depot, si present) build/v11-persist/e1_s0/study_all.json : decomposition voitures / velos.
"""
import collections
import json
from pathlib import Path

ROOT = Path('/workspaces/E-HGP')


def bouts():
    d = json.load(open(ROOT / 'Zoltan/demos/bouts_evalues.json'))
    B = d['bouts']
    print('bouts', len(B), collections.Counter(b['category'] for b in B))
    per = collections.defaultdict(collections.Counter)
    for b in B:
        for k, o in b['outcomes'].items():
            per[k][o] += 1
    for k in sorted(per, key=int):
        print('  k=%s' % k, dict(per[k]))
    mixed = [b['name'] for b in B if 'win' in b['outcomes'].values() and 'loss' in b['outcomes'].values()]
    print('  bouts a gain et perte', mixed)


def etude():
    base = ROOT / 'morsehgp3D_v11/receipts/developpement_20261004/e1_sortie_plate/etude'
    t = json.load(open(base / 'etude_table.json'))
    for k in ('2', '3', '5', '10', 'all'):
        print('  k=%-3s' % k, ' | '.join('%s %.3f' % (line, t['%s_mcs20|k=%s' % (line, k)]['all_found'])
                                     for line in ('T_eom1', 'A_eom1', 'R0_eom', 'T_eom2')))
    o = json.load(open(base / 'oracle_lot2.json'))
    for mcs in (10, 20):
        rs = [r for r in o if r['mcs'] == mcs and not r['name'].startswith('zoltan')]
        print('  oracle mcs%d objets %d hier %d oracle %d eom1 %d eom3 %d feuilles %d' % (
            mcs, sum(r['objects'] for r in rs), sum(r['hier'] for r in rs), sum(r['oracle'] for r in rs),
            sum(r['eom1'] for r in rs), sum(r['eom3'] for r in rs), sum(r['leaf'] for r in rs)))


def etude_hors_depot():
    p = ROOT / 'build/v11-persist/e1_s0/study_all.json'
    if not p.is_file():
        print('  study_all.json absent')
        return
    R = json.load(open(p))['results']
    agg = collections.defaultdict(collections.Counter)
    for r in R:
        if not all(x > 0.5 for x in r['level_b_tower']):
            continue
        kind = 'voitures' if 'voitures' in r['name'] else 'velos(+pietons)'
        for line in ('T_eom1_mcs20', 'R0_eom_mcs20'):
            rows = r['lines'][line]['rows']
            c = agg[(kind, r['k'], line)]
            c['n'] += 1
            c['all'] += all(x['found'] for x in rows)
            c['merged'] += sum(x['merged'] for x in rows)
    for key in sorted(agg):
        c = agg[key]
        print('  %-16s k=%-2d %-13s n %4d tous %4d (%.3f) fusions %d' % (
            key[0], key[1], key[2], c['n'], c['all'], c['all'] / c['n'], c['merged']))


if __name__ == '__main__':
    bouts()
    etude()
    etude_hors_depot()
