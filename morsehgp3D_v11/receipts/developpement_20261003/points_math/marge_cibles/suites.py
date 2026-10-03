#!/usr/bin/env python3
"""Suites condensees (base de chaque fixture ancree) et dates d'entree, pour H1, Hk1, Hmcs et les temoins."""
import json
import sys
sys.dont_write_bytecode = True
import cellules_hm as CH  # noqa: E402

AD, CL = CH.AD, CH.CL
FIX = [('T0_P1_aretes_courtes__mcs2-3', [2, 3]), ('T0_P2_pont_court__mcs2-3', [2, 3]),
       ('T0_S_plan_egalites__mcs2-3', [2, 3]), ('T0_S_3D_equilateral__mcs2-3', [2, 3]),
       ('Q1_T1_1700_K2__mcs3', [2, 3]), ('Q2_S17_K2__mcs2', [2]), ('Q3_filament_K2__mcs2-6', [2, 6, 9]),
       ('Q4_T6_chaine_K3__mcs2-3', [2, 3])]
v2 = json.load(open(CL.V2))
by = {e['name']: e for e in v2['fixtures']}
prm = {'lam': None, 'eta': CL.Fraction(1), 'kappa': CL.Fraction(12)}
regles = sys.argv[1].split(',') if len(sys.argv) > 1 else ['H1', 'Hk1', 'Hmcs', 'cover', 'ER0h_U']
out = {}
for nm, mcss in FIX:
    e = by[nm]
    pts = e['points']
    noms = list(pts)
    P = [tuple(pts[x]) for x in noms]
    T, info = CL.full(P, e['K'])
    out[nm] = {}
    for rg in regles:
        out[nm][rg] = {}
        for mcs in mcss:
            h, det = CL.construire(rg, T, info, prm, mcs)
            dates = {noms[x]: round(float(h.dates[x]), 3) for x in range(len(noms))}
            suite = CL.suite_condensee(h, noms, mcs)
            out[nm][rg][mcs] = {'dates': dates, 'suite': suite}
            print('%-30s %-6s mcs%-2d dates %s' % (nm, rg, mcs, json.dumps(dates)))
            print('%-30s %-6s mcs%-2d suite %s' % ('', '', mcs, json.dumps(suite)))
json.dump(out, open('recus/suites_condensees.json', 'w'), indent=1, sort_keys=True)
