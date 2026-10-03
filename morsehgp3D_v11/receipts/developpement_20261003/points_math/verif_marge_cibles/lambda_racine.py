#!/usr/bin/env python3
"""Contre-exemple a la conjecture du rapport (claim 14) : « prendre Lambda = niveau de la racine suffit dans H3
(aucune premiere couverture au-dessus de la racine) ».

Nuage cocirculaire a K = 3 : x = (0, R, 0), s = (0, -R, 0), p = (-a, -b, 0), q = (a, -b, 0) avec a^2 + b^2 = R^2,
s antipode de x. Les trois 3-parties contenant x ont toutes la MEB du cercle (rayon R) ; cette boule contient la
partie {p, q, s} deja nee : la lentille de x nait collee, FULL_3 n'a qu'un noeud (ne a MEB{p,q,s}^2 = a^2), et x
n'est couvert qu'a R^2 > a^2. Puis paire homothetique X = c P, Y = (c + 1) P (deplacement eps = R pour chaque site) :
violation des bornes 3 delta / 5 delta de H3 si Lambda = max des racines.
Recoupe : mon code (indep_full) ET l'oracle v11 (hgp11_ref + points_reference, regles margin1 et margin).
"""
import sys, json
sys.dont_write_bytecode = True
from fractions import Fraction as Fr
from math import sqrt, isqrt
import indep_full as I
sys.path.insert(0, '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench')
sys.path.insert(0, '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/reference')
import points_reference as PR
from hgp11_ref import Definition

def cloud(c, R=221, a=21, b=220):
    return [(0, c * R, 0), (0, -c * R, 0), (-c * a, -c * b, 0), (c * a, -c * b, 0)]

out = {}
for (R, a, b) in ((25, 7, 24), (221, 21, 220)):
    for c in (1, 10):
        P = cloud(c, R, a, b)
        T = I.Full(P, 3)
        res = Definition(P).order(3)
        first = [min(v for v in T.cov[i] if v is not None) for i in range(4)]
        e1 = I.rule_margin_sq(T, 1); e4 = I.rule_margin_sq(T, 4)
        ref1, tree = PR.reference_rules(res, 4, 1)
        ref4, _ = PR.reference_rules(res, 4, 4)
        u1o = PR.reference_ultrametric(ref1['margin1'], tree); u4o = PR.reference_ultrametric(ref4['margin'], tree)
        u1 = I.ultra_sq(T, e1); u4 = I.ultra_sq(T, e4)
        out['R%d_c%d' % (R, c)] = {
            'points': P, 'noeuds_mon_code': [str(h) for h in T.h], 'noeuds_oracle': [str(nd.level) for nd in res.nodes],
            'racine': str(T.h[T.root]), 'premieres_couvertures': [str(f) for f in first],
            'x_couvert_au_dessus_de_la_racine': first[0] > T.h[T.root],
            'H1_mon_code': [str(e) for e, _o in e1], 'H1_oracle': [str(e) for e, _o in ref1['margin1']],
            'H4_mon_code': [str(e) for e, _o in e4], 'H4_oracle': [str(e) for e, _o in ref4['margin']],
            'u_egales_H1': u1 == u1o, 'u_egales_H4': u4 == u4o, 'u1': [[str(v) for v in row] for row in u1]}
        print('R', R, 'c', c, 'noeuds', [str(h) for h in T.h], 'oracle', [str(nd.level) for nd in res.nodes],
              'premieres couvertures', [str(f) for f in first], 'H1', [str(e) for e, _o in e1], 'H1 oracle',
              [str(e) for e, _o in ref1['margin1']], 'u egales', u1 == u1o, u4 == u4o)

# paire homothetique : X = c P, Y = (c+1) P, eps = R (|P_i| = R pour tous les sites)
R0, a0, b0 = 221, 21, 220
pairs = []
for c in (1, 2, 5, 10, 30):
    X, Y = cloud(c, R0, a0, b0), cloud(c + 1, R0, a0, b0)
    eps = max(sqrt(sum((X[i][k] - Y[i][k]) ** 2 for k in range(3))) for i in range(4))
    TX, TY = I.Full(X, 3), I.Full(Y, 3)
    Lam = max(TX.h[TX.root], TY.h[TY.root])
    delta = 2 * eps * sqrt(Lam) + eps * eps
    row = {'c': c, 'eps': eps, 'racine_X': str(TX.h[TX.root]), 'racine_Y': str(TY.h[TY.root]), 'Lambda': str(Lam), 'delta': delta}
    for m in (1, 4):
        eX, eY = I.rule_margin_sq(TX, m), I.rule_margin_sq(TY, m)
        uX, uY = I.ultra_sq(TX, eX), I.ultra_sq(TY, eY)
        de = max(abs(float(eX[i][0] - eY[i][0])) for i in range(4))
        du = max(abs(float(uX[i][j] - uY[i][j])) for i in range(4) for j in range(4))
        row['m%d' % m] = {'max_de': de, 'max_du': du, 'de/delta': de / delta, 'du/delta': du / delta,
                          'viole_3delta': de > 3 * delta, 'viole_5delta': du > 5 * delta}
    # avec Lambda = toutes les premieres couvertures : la borne doit tenir
    Lam2 = max(Lam, max(min(v for v in TX.cov[i] if v is not None) for i in range(4)),
               max(min(v for v in TY.cov[i] if v is not None) for i in range(4)))
    delta2 = 2 * eps * sqrt(Lam2) + eps * eps
    row['delta_Lambda_premieres_couvertures'] = delta2
    row['du/delta2_m1'] = row['m1']['max_du'] / delta2
    pairs.append(row)
    print(json.dumps(row))
out['paires_homothetiques'] = pairs
json.dump(out, open('recus/lambda_racine.json', 'w'), indent=1, default=str)
