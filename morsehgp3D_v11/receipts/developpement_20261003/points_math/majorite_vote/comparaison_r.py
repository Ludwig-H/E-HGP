#!/usr/bin/env python3
"""Complement de comparaison.py : ER0h a cone relatif (kappa' = 10) et ER0hv (theta = 1/20) sur les memes fixtures.
Usage : python3 -B comparaison_r.py > recus/comparaison_relatif.json"""
from fractions import Fraction
import json
import sys

sys.dont_write_bytecode = True
import arbre  # noqa: E402
import fixtures as FX  # noqa: E402
import regles as RG  # noqa: E402
from surd import Surd  # noqa: E402


def main():
    cases = [('EQUILATERAL_v11', dict(K=2, names=['A', 'B', 'C', 'D', 'E', 'F'], points=FX.EQUILATERAL))]
    cases += list(FX.t0_bases().items())
    cases += [('FIVE', dict(K=2, names=['0', '1', '2', '3', '4'], points=FX.FIVE)),
              ('ligne_0_2s_4s', dict(K=2, names=['g', 'x', 'd'], points=[(0, 0, 0), (2000, 0, 0), (4000, 0, 0)])),
              ('ligne_0_2s_4s+1', dict(K=2, names=['g', 'x', 'd'], points=[(0, 0, 0), (2000, 0, 0), (4001, 0, 0)])),
              ('Q1_T1_1700', FX.Q1), ('Q2_S17', FX.Q2), ('Q3_filament', FX.Q3), ('Q4_T6', FX.Q4)]
    out = {}
    for nm, fx in cases:
        _d, _res, tree = arbre.v11_tree(fx['points'], fx['K'])
        tab = {}
        for rule, hang in (('ER0hr(1,10)', RG.rule_er0hr(tree, 1, 10)),
                           ('ER0hv(1,12,1/20)', RG.rule_er0hv(tree, 1, 12, Fraction(1, 20)))):
            u = RG.ultrametric(tree, hang)
            tab[rule] = dict(hierarchie=RG.hierarchy_text(u, fx['names'], 2),
                             entrees=dict((fx['names'][i], round(float(hang[i][0]), 3)) for i in range(len(hang))))
        out[nm] = tab
    print(json.dumps(out, indent=1, ensure_ascii=False))


if __name__ == '__main__':
    main()
