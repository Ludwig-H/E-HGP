#!/usr/bin/env python3
"""Claim 8 du rapport : rival tardif sur k5_spheres_face_a_face (base, K = 5, H_{k+1} = H_6), avec mon FULL rapide."""
import sys, json, time
sys.dont_write_bytecode = True
from math import sqrt
import indep_full as I
import indep_full_j as J
v2 = json.load(open('/workspaces/E-HGP/build/v10-verrou-points/juge_final/cibles/fixtures_catalogue_v2.json'))
e = {x['name']: x for x in v2['fixtures']}['k5_spheres_face_a_face__mcs2-7']
pts = e['points']; noms = list(pts); P = [tuple(pts[x]) for x in noms]
t0 = time.time()
T = J.FullJ(P, e['K'])
ix = {x: i for i, x in enumerate(noms)}
out = {'noeuds': len(T.h), 'racine_r': sqrt(T.h[T.root])}
for site in ('x1', 'x2'):
    for m in (1, 6):
        x = ix[site]
        S = I.qualified_points(T, x, m)
        t = min(S.values()); o = min(v for v, s in S.items() if s == t)
        terms = sorted(((float(T.meet(o, t, v, s) - s), sqrt(s), sqrt(T.meet(o, t, v, s)),
                         ''.join(sorted(noms[y] for y in T.covered(v, s)))) for v, s in S.items()), reverse=True)[:3]
        ent = I.rule_margin_sq(T, m)[x]
        out['%s_m%d' % (site, m)] = {'t_r': sqrt(t), 'premiere_couverture_couvre': ''.join(sorted(noms[y] for y in T.covered(o, t))),
                                     'entree_r': sqrt(ent[0]), 'termes_max': terms}
out['secondes'] = round(time.time() - t0, 1)
json.dump(out, open('recus/rival_k5.json', 'w'), indent=1)
print(json.dumps(out, indent=1))
