#!/usr/bin/env python3
"""T0_P1 : la variante D-1 (deplacement d'une unite) donne a C la lentille CD comme premiere couverture unique ; H3
(|du| <= 5 delta, delta = 2 eps sqrt(Lambda) + eps^2) force alors u(C, A) dans la base pres de la racine."""
import json
import sys
from fractions import Fraction
from math import isqrt
sys.dont_write_bytecode = True
import adaptateur as AD  # noqa: E402
CL, PR = AD.CL, AD.PR
v2 = json.load(open(CL.V2))
e = next(x for x in v2['fixtures'] if x['name'] == 'T0_P1_aretes_courtes__mcs2-3')
out = {}
for vn, pts in [('base', e['points'])] + [(v['name'], v['points']) for v in e['variants']]:
    noms = list(pts)
    P = [tuple(pts[x]) for x in noms]
    res = AD.definition(P).order(2)
    ref, tree = PR.reference_rules(res, len(P), 1)
    u = PR.reference_ultrametric(ref['margin1'], tree)
    iA, iC = noms.index('A'), noms.index('C')
    racine = max(nd.level for nd in res.nodes)
    out[vn] = {'points_modifies': {k: pts[k] for k in pts if pts[k] != e['points'][k]},
               'premiere_couverture_C': [str(res.cover[iC].level), sorted(res.cover[iC].nodes)],
               'u_CA': str(u[iC][iA]), 'r_u_CA': round(float(u[iC][iA]) ** 0.5, 3), 'racine': str(racine),
               'r_racine': round(float(racine) ** 0.5, 3)}
lam = max(Fraction(v['racine']) for v in out.values())
eps = 1
delta = 2 * eps * (float(lam) ** 0.5) + eps * eps
uY = Fraction(out['D-1']['u_CA'])
borne = float(uY) - 5 * delta
out['H3'] = {'Lambda': str(lam), 'delta': round(delta, 3), 'u_CA_D-1': str(uY),
             'minorant_u_CA_base_par_H3': round(borne, 1), 'en_rayon': round(borne ** 0.5, 3),
             'fenetre_cible_rayon': [1300, 1700]}
print(json.dumps(out, indent=1))
json.dump(out, open('recus/propagation_h3_T0_P1.json', 'w'), indent=1)
