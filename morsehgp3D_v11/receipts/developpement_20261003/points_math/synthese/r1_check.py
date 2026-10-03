"""Fixture R1 (verif_axiomes) : x point coeur de la composante qui fusionne avec le fond a F = 12 ; dates de x."""
import sys, math
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench')
from fractions import Fraction
import points_reference as pr
from hgp11_ref import Definition
names = ['x', 'y', 's2', 's3', 'b1', 'b2', 'w1', 'w2']
pts = [(0, 0, 0), (10, 0, 0), (20, 0, 0), (30, 0, 0), (44, 0, 0), (54, 0, 0), (-20, 10, 0), (-20, -10, 0)]
res = Definition(pts).order(2)
n = len(pts)
print('noeuds (rayon de naissance) :', sorted(round(math.sqrt(nd.level), 3) for nd in res.nodes))
print('D_2(x) rayon %.3f' % math.sqrt(res.core[0].level))
L = Fraction(144)
for cut in res.cuts:
    if cut.level == L:
        print('coupe 144 :', [sorted(names[i] for i in range(n) if cov >> i & 1) for v, cov, core in cut.closed])
for m in (1, 3):
    ref, tree = pr.reference_rules(res, n, m)
    rad = pr.reference_radius_rules(res, n, m)
    rad = rad[0] if isinstance(rad, tuple) else rad
    for rule in (('margin1',) if m == 1 else ('margin', 'first')):
        u = pr.reference_ultrametric(ref[rule], tree)
        bx = next((b for b in pr.blocks_at(u, L) if 0 in b), frozenset())
        print('%-7s m=%d : x entre au rayon %.3f ; bloc de x a F=12 : %s' % (rule, m, math.sqrt(ref[rule][0][0]), sorted(names[i] for i in bx)))
    key = 'margin_r1' if m == 1 else 'margin_r'
    print('%-7s m=%d : x entre au rayon %.3f (marge en rayon)' % (key, m, float(rad[key][0][0])))
