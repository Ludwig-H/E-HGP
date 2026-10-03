#!/usr/bin/env python3
"""Rival lointain pour H_{k+1} (k = 2, m = 3) : le site x = (0,0,0) forme un triangle aigu avec deux voisins a
gauche (couverture qualifiee a r = 1,25) ; un triangle eloigne a droite (abscisse D) ne le couvre qu'a r ~ D/2 et
fusionne vers r ~ (D+2)/2. Entree de x selon H_3, EC_3, first_3 et la fermeture m = 3."""
import math
import sys
sys.dont_write_bytecode = True
import lib_fermeture as lf  # noqa: E402

print('# x=(0,0,0), gauche (-2,+-1,0), droite (D,+-1,0),(D+2,0,0) ; k=2, m=3 ; rayons')
for D in (4, 10, 30, 100, 300):
    pts = [(0, 0, 0), (-2, 1, 0), (-2, -1, 0), (D, 1, 0), (D, -1, 0), (D + 2, 0, 0)]
    res = lf.Definition(pts).order(2)
    ref, tree = lf.faithful_rules(res, 6, 3)
    cl = lf.closure(res, 6, 3)
    ec = lf.ec_hanging(res, 6, 3)
    root = max(nd.level for nd in res.nodes)
    t = cl['entries'][0]
    print('D=%3d : premiere couverture qualifiee r=%.3f ; fusion FULL r=%.3f | entree de x : H_3 r=%.3f (x%.2f)'
          ' ; EC_3 r=%.3f ; first_3 r=%.3f ; fermeture r=%.3f' % (
              D, math.sqrt(t), math.sqrt(root), math.sqrt(ref['margin'][0][0]),
              math.sqrt(ref['margin'][0][0] / t), math.sqrt(ec[0][0]), math.sqrt(ref['first'][0][0]),
              math.sqrt(cl['u'][0][0])))
