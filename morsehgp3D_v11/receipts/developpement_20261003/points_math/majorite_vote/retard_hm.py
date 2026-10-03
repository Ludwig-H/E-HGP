#!/usr/bin/env python3
"""Retard d'entree relatif a l'echelle propre alpha^2 = A : paire serree {x, y} (|xy| = 2, A = 1) et paire lointaine
{z1, z2} a distance r, (a) alignee de l'autre cote de x, (b) perpendiculaire. K = 2.
Usage : python3 -B retard_hm.py > recus/retard_hm.json"""
import json
import sys

sys.dont_write_bytecode = True
import arbre  # noqa: E402
import regles as RG  # noqa: E402


def main():
    out = {}
    for label, mk in (('alignee', lambda r: [(0, 0, 0), (2, 0, 0), (-r, 0, 0), (-r - 1, 0, 0)]),
                      ('perpendiculaire', lambda r: [(0, 0, 0), (2, 0, 0), (0, r, 0), (0, r + 1, 0)])):
        rows = []
        for r in (10, 100, 1000, 10000):
            _d, _res, t = arbre.v11_tree(mk(r), 2)
            A = min(t.cov[0].values())
            row = {'r': r, 'A_x': str(A), 'racine': str(t.birth[t.root])}
            for nm, h in (('H_1', RG.rule_hm(t, 1)), ('H_3', RG.rule_hm(t, 3)), ('ER0h(1,12)', RG.rule_er0h(t, 1, 12)),
                          ('ER0hr(1,10)', RG.rule_er0hr(t, 1, 10))):
                e = h[0][0].square()
                row[nm] = str(e.rational()) if e.rational() is not None else float(e)
            rows.append(row)
        out[label] = rows
    print(json.dumps(out, indent=1))


if __name__ == '__main__':
    main()
