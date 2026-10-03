#!/usr/bin/env python3
"""Ou la marge en rayon perd contre la marge en niveau carre : tetraedres_sommet_partage_K3__mcs4 (n <= 9)."""
import sys, json
sys.dont_write_bytecode = True
from math import sqrt
import indep_full as I
import cells as C
ver = C.ver
v2 = json.load(open(C.V2))
e = {x['name']: x for x in v2['fixtures']}['tetraedres_sommet_partage_K3__mcs4']
print('K', e['K'], 'mcs', e['mcs'], 'points', e['points'])
for t in e['target']:
    print('cible', {k: t[k] for k in ('r2', 'bounds', 'blocks', 'tolerance', 'libres_vrais', 'bruit') if k in t})
out = []
for vn, pts, cibles in [('base', e['points'], e['target'])] + [(v['name'], v['points'], v['target']) for v in e.get('variants', [])]:
    noms = list(pts); P = [tuple(pts[x]) for x in noms]; T = I.Full(P, e['K']); ix = {x: i for i, x in enumerate(noms)}
    for rule in ('H1', 'H1r', 'Hk1', 'Hk1r'):
        U, kind = C.build(rule, T, 4)
        h = I.Hier(U, kind)
        for mcs in range(e['mcs'][0], e['mcs'][1] + 1):
            for ic, c in enumerate(cibles):
                ok, viol = ver.juger(h, c, ix, mcs)
                if not ok:
                    r, part, why = viol
                    cl, _b = ver.condenser(part, mcs)
                    row = (vn, rule, mcs, ic, round(float(r), 3), why, [''.join(sorted(noms[y] for y in b)) for b in cl])
                    out.append(row); print(row)
    if vn == 'base':
        for rule, ent in (('H1', I.rule_margin_sq(T, 1)), ('H1r', I.rule_margin_r(T, 1))):
            print(' ', rule, {noms[i]: round(sqrt(x[0]) if rule == 'H1' else float(x[0]), 3) for i, x in enumerate(ent)})
json.dump(out, open('recus/partout.json', 'w'), indent=1)
