"""Q3 (filament contre amas, K = 2) : respect du coeur. Pour r dans [d_2(x) ; racine[, x est un point coeur
(x dans L_2(r)) ; on compare le bloc de x selon core (composante qui contient x) et selon margin (= H_{k+1}, meme
lignee que H^r_{k+1}) et first (m = 3), a quelques coupes fermees."""
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench')
from fractions import Fraction
import math
import points_reference as pr
from hgp11_ref import Definition
O = (10000, 10000, 10000)
def at(d):
    return (O[0] + d[0], O[1] + d[1], O[2] + d[2])
names = ['x', 'f1', 'f2', 'f3', 'f4', 'f5', 'c0', 'c1', 'c2', 'c3', 'c4', 'c5', 'c6', 'c7']
offs = [(0, 0, 0), (700, 3, 0), (1401, -2, 0), (2100, 4, 0), (2802, 0, 0), (3500, -3, 0), (-900, 0, 0),
        (-880, 200, 0), (-880, -200, 0), (-880, 0, 200), (-880, 0, -200), (-1100, 0, 0), (-1080, 150, 100),
        (-1080, -150, -100)]
pts = [at(d) for d in offs]
res = Definition(pts).order(2)
n = len(pts)
root = max(node.level for node in res.nodes)
print('racine (rayon) %.3f ; D_2(x) (rayon) %.3f' % (math.sqrt(root), math.sqrt(res.core[0].level)))
for m in (1, 3):
    ref, tree = pr.reference_rules(res, n, m)
    rules = ('core', 'cover') if m == 1 else ('first', 'margin')
    for rule in rules:
        u = pr.reference_ultrametric(ref[rule], tree)
        e = ref[rule][0][0]
        print('%-6s m=%d : entree de x au rayon %.3f' % (rule, m, math.sqrt(e)))
        for r in (705, 750, 790):
            L = Fraction(r * r)
            blocks = pr.blocks_at(u, L)
            bx = next((b for b in blocks if 0 in b), frozenset())
            print('    r=%d : bloc de x = %s' % (r, sorted(names[i] for i in bx)))
