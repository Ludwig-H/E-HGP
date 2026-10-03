#!/usr/bin/env python3
"""(1) Equivariance de la fermeture : permutation des identifiants et isometrie entiere (rotation d'axes, reflexion,
translation), vf_oracle. (2) Rival lointain : H_3 avec la marge en niveau carre (regle du developpeur) contre la meme
marge en RAYON, e_r = sqrt(t) + sup_q [sqrt(m(p,q)) - sqrt(h(q))] (calcul flottant double, illustratif)."""
import math
import random
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-points-math/verif_fermeture')
import vf_oracle as vo
from verif_theoremes import gen_ties, gen_generic, gen_blobs

rng = random.Random(2718)
checks = bad = 0
for c in range(150):
    pts = (gen_ties, gen_generic, gen_blobs)[c % 3](rng)
    n = len(pts)
    perm = list(range(n))
    rng.shuffle(perm)
    pp = [pts[perm[j]] for j in range(n)]
    iso = [(-y + 5000, z + 7, -x + 3000) for (x, y, z) in pts]
    c0, cp, ci = vo.Cloud(pts), vo.Cloud(pp), vo.Cloud(iso)
    for k in range(1, min(3, n - 1) + 1):
        F0, Fp, Fi = vo.Full(c0, k), vo.Full(cp, k), vo.Full(ci, k)
        for m in sorted(set([1, k + 1, k + 2])):
            if m > n:
                continue
            u0 = F0.closure(m)[0]
            up = Fp.closure(m)[0]
            ui = Fi.closure(m)[0]
            checks += 1
            ok = all(up[a][b] == u0[perm[a]][perm[b]] for a in range(n) for b in range(n)) and ui == u0
            bad += not ok
print('equivariance fermeture : %d controles, %d ecarts' % (checks, bad))


def margin_radius(F, i, m):
    pts = F.cover_points(i, m)
    t = min(a for a, _ in pts)
    v1 = [v for a, v in pts if a == t][0]
    D = max(math.sqrt(F.meet(v1, t, v, a)) - math.sqrt(a) for a, v in pts)
    return math.sqrt(t) + max(D, 0.0)


print('rival lointain, k=2, m=3 : entree de x en rayon')
for D in (4, 10, 30, 100, 300, 1000):
    pts = [(0, 0, 0), (-2, 1, 0), (-2, -1, 0), (D, 1, 0), (D, -1, 0), (D + 2, 0, 0)]
    F = vo.Full(vo.Cloud(pts), 2)
    eh = math.sqrt(F.hang_margin(3)[0][0])
    er = margin_radius(F, 0, 3)
    print('D=%5d : H_3 (marge en niveau carre) r=%.3f ; marge en rayon r=%.3f ; premiere couverture 1.250' % (D, eh, er))
