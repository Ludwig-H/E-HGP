#!/usr/bin/env python3
"""C9 : proprietaire rendu par reference_radius_rules (developpeur) aux egalites exactes, sur T0.

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B c9_owner_bug.py
Compare le proprietaire rendu au noeud vivant a la coupe fermee e (comparaison exacte e^2 = niveau du parent).
"""
import decimal
import json
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/reference')
sys.path.insert(0, '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench')
import points_reference as pr  # noqa: E402
from hgp11_ref import Definition  # noqa: E402

PTS = [(1, 1, 2), (1, 2, 1), (2, 2, 2), (3, 3, 2), (4, 4, 2), (4, 3, 3)]
res = Definition(PTS).order(2)
rad, tree = pr.reference_radius_rules(res, 6, 1)
ctx = decimal.Context(prec=120)
out = {'precision_contexte_global': decimal.getcontext().prec, 'sites': []}
for i, (e, o) in enumerate(rad['margin_r1']):
    p = tree.parent[o]
    lv = res.nodes[p].level
    sq = ctx.sqrt(ctx.divide(decimal.Decimal(lv.numerator), decimal.Decimal(lv.denominator)))
    out['sites'].append({'site': 'ABCDEF'[i], 'e': str(e), 'proprietaire': o, 'niveau_proprietaire': str(res.nodes[o].level),
                         'parent': p, 'niveau_parent': str(lv), 'e_moins_rayon_parent': str(e - sq)})
print(json.dumps(out, indent=1))
with open('recus_c9_owner_bug.json', 'w') as fh:
    json.dump(out, fh, indent=1)
