#!/usr/bin/env python3
"""Faits exacts de FULL (oracle v11, hgp11_ref.Definition) pour la preuve d'impossibilite et pour H3.

(1) Q1 (T1_1700, base, K = 2) : la premiere couverture de C est unique (lentille CD) ; le noeud de CD a pour parent la
    racine ; avant la racine, sa couverture est {C, D}.
(2) Q2 (S17, base, K = 2) : tout noeud vivant qui couvre a a un niveau <= 5625 couvre au plus 2 sites.
(3) Q4 (T6, base, K = 3) : avant la racine, la composante de la chaine CmD couvre {C, m, D} seulement.
(4) H3 avec Lambda = niveau de la racine : nuage K = 3 a un seul noeud ; la couverture de x vient apres la racine."""
import json
import sys
from fractions import Fraction
sys.dont_write_bytecode = True
import adaptateur as AD  # noqa: E402
CL = AD.CL
PR = AD.PR

v2 = json.load(open(CL.V2))
by = {e['name']: e for e in v2['fixtures']}


def couverture_par_coupe(res, n):
    """[(niveau, noeud, masque couvert)] des coupes fermees."""
    out = []
    for cut in res.cuts:
        for v, cov, _core in cut.closed:
            out.append((cut.level, v, cov))
    return out


def noms_de(mask, noms):
    return sorted(noms[i] for i in range(len(noms)) if mask >> i & 1)


faits = {}
# (1)
e = by['Q1_T1_1700_K2__mcs3']
noms = list(e['points'])
P = [tuple(e['points'][x]) for x in noms]
res = AD.definition(P).order(2)
tree = PR.Tree(res.nodes)
iC = noms.index('C')
ent = res.cover[iC]
cd = sorted(ent.nodes)
racine = [v for v in range(len(res.nodes)) if tree.parent[v] < 0][0]
cov_cd = sorted(set(tuple(noms_de(c, noms)) for lv, v, c in couverture_par_coupe(res, len(P))
                    if v in cd and lv < res.nodes[racine].level))
faits['Q1'] = {'A2_C': str(ent.level), 'noeuds_premiere_couverture_C': cd,
               'parent_du_noeud': [tree.parent[v] for v in cd], 'racine': racine,
               'niveau_racine': str(res.nodes[racine].level), 'couverture_avant_racine': cov_cd}
# (2)
e = by['Q2_S17_K2__mcs2']
noms = list(e['points'])
P = [tuple(e['points'][x]) for x in noms]
res = AD.definition(P).order(2)
ia = noms.index('a')
mx = 0
lst = []
for lv, v, c in couverture_par_coupe(res, len(P)):
    if lv <= 5625 and c >> ia & 1:
        k = bin(c).count('1')
        mx = max(mx, k)
        lst.append((str(lv), v, noms_de(c, noms)))
faits['Q2'] = {'max_sites_couverts_par_un_noeud_couvrant_a_jusqu_a_5625': mx,
               'noeuds': sorted(set((x[1], tuple(x[2])) for x in lst))}
# (3)
e = by['Q4_T6_chaine_K3__mcs2-3']
noms = list(e['points'])
P = [tuple(e['points'][x]) for x in noms]
res = AD.definition(P).order(3)
tree = PR.Tree(res.nodes)
iC = noms.index('C')
ent = res.cover[iC]
racine = [v for v in range(len(res.nodes)) if tree.parent[v] < 0][0]
ch = sorted(ent.nodes)
cov_ch = sorted(set(tuple(noms_de(c, noms)) for lv, v, c in couverture_par_coupe(res, len(P))
                    if v in ch and lv < res.nodes[racine].level))
faits['Q4'] = {'A3_C': str(ent.level), 'noeuds_premiere_couverture_C': ch,
               'parent_du_noeud': [tree.parent[v] for v in ch], 'racine': racine,
               'niveau_racine': str(res.nodes[racine].level), 'couverture_avant_racine': cov_ch}
# (4)
for xx in (100, 102):
    P = [(0, 0, 0), (1, 0, 0), (0, 1, 0), (1, 1, 0), (xx, 0, 0)]
    res = AD.definition(P).order(3)
    ref, tree = PR.reference_rules(res, len(P), 1)
    faits['H3_lambda_x%d' % xx] = {'noeuds': [(str(nd.level), list(nd.children)) for nd in res.nodes],
                                  'entree_H1_x': str(ref['margin1'][4][0]), 'A3_x': str(res.cover[4].level)}
print(json.dumps(faits, indent=1, default=str))
json.dump(faits, open('recus/faits_full.json', 'w'), indent=1, default=str)
