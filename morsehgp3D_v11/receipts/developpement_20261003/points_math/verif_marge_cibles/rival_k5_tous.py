import sys, json
sys.dont_write_bytecode = True
from math import sqrt
import indep_full as I, indep_full_j as J
v2 = json.load(open('/workspaces/E-HGP/build/v10-verrou-points/juge_final/cibles/fixtures_catalogue_v2.json'))
e = {x['name']: x for x in v2['fixtures']}['k5_spheres_face_a_face__mcs2-7']
pts = e['points']; noms = list(pts); T = J.FullJ([tuple(pts[x]) for x in noms], 5)
out = {}
for i, nm in enumerate(noms):
    S = I.qualified_points(T, i, 6); t = min(S.values())
    ent = I.rule_margin_sq(T, 6)[i]
    out[nm] = {'t_r': round(sqrt(t), 3), 'entree_r': round(sqrt(ent[0]), 3), 'retard_nul': ent[0] == t}
print(json.dumps(out))
json.dump(out, open('recus/rival_k5_tous.json', 'w'), indent=1)
