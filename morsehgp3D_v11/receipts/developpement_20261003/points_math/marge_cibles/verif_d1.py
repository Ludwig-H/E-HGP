#!/usr/bin/env python3
"""T0_P1, variante D-1 : premier niveau ou un noeud de la lignee de la lentille CD (premiere couverture unique de C)
couvre A. Toute pendaison fidele qui pend C sur cette lignee a donc u(C, A) >= ce niveau."""
import json
import sys
sys.dont_write_bytecode = True
import adaptateur as AD  # noqa: E402
CL, PR = AD.CL, AD.PR
v2 = json.load(open(CL.V2))
e = next(x for x in v2['fixtures'] if x['name'] == 'T0_P1_aretes_courtes__mcs2-3')
pts = next(v['points'] for v in e['variants'] if v['name'] == 'D-1')
noms = list(pts)
P = [tuple(pts[x]) for x in noms]
res = AD.definition(P).order(2)
tree = PR.Tree(res.nodes)
iA, iC = noms.index('A'), noms.index('C')
(cd,) = res.cover[iC].nodes
lignee = set(tree.chain(cd))
premier = min(cut.level for cut in res.cuts for v, cov, _c in cut.closed if v in lignee and cov >> iA & 1)
print(json.dumps({'A2_C': str(res.cover[iC].level), 'noeud_CD': cd, 'lignee': sorted(lignee),
                  'premier_niveau_ou_la_lignee_de_CD_couvre_A': str(premier),
                  'rayon': round(float(premier) ** 0.5, 3)}))
