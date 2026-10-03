#!/usr/bin/env python3
"""Fixture d'echelle melangee du rapport (section 7), recalculee par mon code : x=(0,0,0), y=(-2,0,0), z=(2s,0,0),
w1=(2s+2,0,0), w2=(2s+1,2,0), K = 2."""
import sys, json
sys.dont_write_bytecode = True
from fractions import Fraction as Fr
from math import sqrt
import indep_full as I
out = []
for s in (10, 100, 1000, 10000):
    P = [(0, 0, 0), (-2, 0, 0), (2 * s, 0, 0), (2 * s + 2, 0, 0), (2 * s + 1, 2, 0)]
    T = I.Full(P, 2)
    h1 = I.rule_margin_sq(T, 1); h1r = I.rule_margin_r(T, 1); h3 = I.rule_margin_sq(T, 3)
    row = {'s': s, 'x_H1_niveau': str(h1[0][0]), 'x_H1_rayon': sqrt(h1[0][0]), 'attendu_2s+2': 2 * s + 2,
           'x_H1r_rayon': float(h1r[0][0]), 'y_H1_rayon': sqrt(h1[1][0]), 'y_H1r': float(h1r[1][0]),
           'x_H3_rayon': sqrt(h3[0][0]), 'y_H3_rayon': sqrt(h3[1][0]), 'racine_rayon': sqrt(T.h[T.root]),
           'noeuds': [(str(h), sorted(T.covered(v, h))) for v, h in enumerate(T.h)]}
    out.append(row)
    print({k: v for k, v in row.items() if k != 'noeuds'})
json.dump(out, open('recus/echelle.json', 'w'), indent=1, default=str)
