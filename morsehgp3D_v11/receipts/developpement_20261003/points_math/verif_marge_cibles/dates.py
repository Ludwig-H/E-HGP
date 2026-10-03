#!/usr/bin/env python3
"""Dates d'entree et proprietaires (mon code) pour les fixtures citees dans les tableaux du rapport."""
import sys, json
sys.dont_write_bytecode = True
from math import sqrt
import indep_full as I
v2 = json.load(open('/workspaces/E-HGP/build/v10-verrou-points/juge_final/cibles/fixtures_catalogue_v2.json'))
by = {e['name']: e for e in v2['fixtures']}
FX = ['T0_P1_aretes_courtes__mcs2-3', 'T0_P2_pont_court__mcs2-3', 'T0_S_3D_equilateral__mcs2-3', 'Q1_T1_1700_K2__mcs3',
      'Q2_S17_K2__mcs2', 'Q3_filament_K2__mcs2-6', 'Q4_T6_chaine_K3__mcs2-3']
out = {}
for nm in FX:
    e = by[nm]; pts = e['points']; noms = list(pts); P = [tuple(pts[x]) for x in noms]
    T = I.Full(P, e['K'])
    def cov_names(w, lvl):
        return ''.join(sorted(noms[y] for y in T.covered(w, lvl)))
    row = {}
    for lab, ent, kind in (('H1', I.rule_margin_sq(T, 1), 'sq'), ('Hk1', I.rule_margin_sq(T, e['K'] + 1), 'sq'),
                           ('H9', I.rule_margin_sq(T, 9), 'sq') if e['K'] == 2 and T.n >= 9 else (None, None, None),
                           ('H1r', I.rule_margin_r(T, 1), 'r'), ('Hk1r', I.rule_margin_r(T, e['K'] + 1), 'r'),
                           ('P2r', I.rule_margin_r(T, 1, 2), 'r')):
        if lab is None: continue
        d = {}
        for i, (ev, o) in enumerate(ent):
            r = sqrt(ev) if kind == 'sq' else float(ev)
            lvl = ev if kind == 'sq' else None
            d[noms[i]] = (round(r, 3), 'h=%.3f' % sqrt(T.h[o]), cov_names(o, T.h[o]))
        row[lab] = d
    row['racine'] = round(sqrt(T.h[T.root]), 3)
    out[nm] = row
    print(nm, 'racine', row['racine'])
    for lab in row:
        if lab == 'racine': continue
        print('  ', lab, row[lab])
json.dump(out, open('recus/dates.json', 'w'), indent=1)
