#!/usr/bin/env python3
"""Faits exacts de FULL invoques par le theoreme du rapport (section 4), recalcules par indep_full.py seul."""
import sys, json, time
sys.dont_write_bytecode = True
from fractions import Fraction as Fr
from math import sqrt
import indep_full as I

v2 = json.load(open('/workspaces/E-HGP/build/v10-verrou-points/juge_final/cibles/fixtures_catalogue_v2.json'))
by = {e['name']: e for e in v2['fixtures']}

def cloud(entry, variant='base'):
    pts = entry['points'] if variant == 'base' else [v for v in entry['variants'] if v['name'] == variant][0]['points']
    noms = list(pts)
    return noms, [tuple(pts[x]) for x in noms]

def first_cover_nodes(T, x):
    vals = {v: c for v, c in enumerate(T.cov[x]) if c is not None}
    A = min(vals.values())
    return A, sorted(v for v, c in vals.items() if c == A)

out = {}
t0 = time.time()
# (1) Q1 base
noms, P = cloud(by['Q1_T1_1700_K2__mcs3'])
T = I.Full(P, 2); ix = {x: i for i, x in enumerate(noms)}
A, nds = first_cover_nodes(T, ix['C'])
v = nds[0]
f1 = {'A2_C': str(A), 'r': sqrt(A), 'noeuds_premiere_couverture': len(nds),
      'couverture_a_la_naissance': sorted(noms[y] for y in T.covered(v, T.h[v])),
      'parent_est_racine': T.parent[v] == T.root, 'niveau_parent': str(T.h[T.parent[v]]), 'r_parent': sqrt(T.h[T.parent[v]]),
      'couverture_juste_avant_parent': sorted(noms[y] for y in range(T.n) if T.cov[y][v] is not None),
      'noeuds': [(str(T.h[w]), sorted(noms[y] for y in T.covered(w, T.h[w])), T.children[w]) for w in range(len(T.h))]}
out['fait1_Q1_base'] = f1
# (2) Q2 base : tout noeud qui couvre a a un niveau <= 5625 couvre au plus 2 sites (a ce niveau)
noms, P = cloud(by['Q2_S17_K2__mcs2'])
T = I.Full(P, 2); ix = {x: i for i, x in enumerate(noms)}
a = ix['a']
viol = []
detail = []
for w in range(len(T.h)):
    c = T.cov[a][w]
    if c is None or c > 5625:
        continue
    # couverture maximale de w sur [c, min(5625, mort)) : sites de c_x(w) <= 5625 (c_x valides sont < mort)
    S = sorted(noms[y] for y in range(T.n) if T.cov[y][w] is not None and T.cov[y][w] <= 5625)
    detail.append({'noeud': w, 'h': str(T.h[w]), 'mort': str(T.death[w]), 'c_a': str(c), 'couvre_jusqu_a_5625': S,
                   'c_par_site': {noms[y]: str(T.cov[y][w]) for y in range(T.n) if T.cov[y][w] is not None}})
    if len(S) > 2:
        viol.append(w)
# premier niveau ou un noeud couvre a et au moins 3 sites
q3 = []
for w in range(len(T.h)):
    if T.cov[a][w] is None: continue
    q = T.qual(3)[w]
    if q is None: continue
    s = max(T.cov[a][w], q)
    if T.death[w] is None or s < T.death[w]:
        q3.append(s)
out['fait2_Q2_base'] = {'noeuds_couvrant_a_avant_5625': detail, 'violations': viol,
                        'premier_niveau_qualifie_m3_pour_a': str(min(q3)), 'r': sqrt(min(q3)),
                        'racine': str(T.h[T.root]), 'r_racine': sqrt(T.h[T.root]),
                        'noeuds': [(str(T.h[w]), sqrt(T.h[w]), sorted(noms[y] for y in T.covered(w, T.h[w])), T.children[w]) for w in range(len(T.h))]}
# (3) Q4 base K = 3
noms, P = cloud(by['Q4_T6_chaine_K3__mcs2-3'])
T = I.Full(P, 3); ix = {x: i for i, x in enumerate(noms)}
A, nds = first_cover_nodes(T, ix['C'])
v = nds[0]
out['fait3_Q4_base'] = {'A3_C': str(A), 'r': sqrt(A), 'noeuds_premiere_couverture': len(nds),
                        'couverture_naissance': sorted(noms[y] for y in T.covered(v, T.h[v])),
                        'parent_est_racine': T.parent[v] == T.root, 'niveau_parent': str(T.h[T.parent[v]]),
                        'couverture_avant_parent': sorted(noms[y] for y in range(T.n) if T.cov[y][v] is not None)}
# (4) T0_P1 D-1 et base
e = by['T0_P1_aretes_courtes__mcs2-3']
res4 = {}
for var in ('base', 'D-1'):
    noms, P = cloud(e, var)
    T = I.Full(P, 2); ix = {x: i for i, x in enumerate(noms)}
    A, nds = first_cover_nodes(T, ix['C'])
    v = nds[0]
    # premier niveau ou la lignee de v couvre A
    lvl = None
    for w in T.chain(v):
        c = T.cov[ix['A']][w]
        if c is not None:
            lvl = c if lvl is None else min(lvl, c)
            break
    allfirst = {noms[y]: str(min(c for c in T.cov[y] if c is not None)) for y in range(T.n)}
    maxfirst = max(min(c for c in T.cov[y] if c is not None) for y in range(T.n))
    q3max = max(min(s for s in I.qualified_points(T, y, 3).values()) for y in range(T.n))
    h1 = I.rule_margin_sq(T, 1)
    U = I.ultra_sq(T, h1)
    res4[var] = {'A2_C': str(A), 'noeuds_premiere_couverture_C': len(nds),
                 'couverture_naissance': sorted(noms[y] for y in T.covered(v, T.h[v])),
                 'lignee_couvre_A_des': str(lvl), 'r_lignee_A': sqrt(lvl) if lvl is not None else None,
                 'racine': str(T.h[T.root]), 'r_racine': sqrt(T.h[T.root]),
                 'premieres_couvertures': allfirst, 'max_premiere_couverture': str(maxfirst),
                 'max_premiere_couverture_qualifiee_m3': str(q3max),
                 'H1_entree_C': str(h1[ix['C']][0]), 'r_H1_entree_C': sqrt(h1[ix['C']][0]),
                 'H1_u_CA': str(U[ix['C']][ix['A']]), 'r_H1_u_CA': sqrt(U[ix['C']][ix['A']])}
out['fait4_T0_P1'] = res4
out['secondes'] = round(time.time() - t0, 1)
json.dump(out, open('recus/faits.json', 'w'), indent=1, default=str)
print(json.dumps({k: (v if k != 'fait2_Q2_base' else {kk: vv for kk, vv in v.items() if kk != 'noeuds_couvrant_a_avant_5625'}) for k, v in out.items() if k != 'fait1_Q1_base'}, indent=1, default=str)[:6000])
print(json.dumps({k: v for k, v in f1.items() if k != 'noeuds'}, default=str))
print('Q2 detail:', json.dumps(out['fait2_Q2_base']['noeuds_couvrant_a_avant_5625'], default=str)[:3000])
