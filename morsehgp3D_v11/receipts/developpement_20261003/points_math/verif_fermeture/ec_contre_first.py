#!/usr/bin/env python3
"""EC (premier niveau ou une seule composante vivante qualifiee couvre le site) contre first (LCA des ex aequo a la
premiere couverture qualifiee) : sont-elles la meme regle ? Recherche de differences sur nuages a ex aequo."""
import random
import sys
import math
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-points-math/verif_fermeture')
import vf_oracle as vo
from verif_theoremes import gen_ties, gen_generic, gen_blobs

rng = random.Random(31337)
diff = {}
tot = {}
ex = {}
for c in range(900):
    pts = (gen_ties, gen_ties, gen_generic, gen_blobs)[c % 4](rng)
    n = len(pts)
    cl = vo.Cloud(pts)
    for k in (1, 2, 3):
        if k >= n:
            continue
        F = vo.Full(cl, k)
        for m in sorted(set([1, k + 1, k + 2])):
            if m > n:
                continue
            ue = F.ultra(F.hang_ec(m))
            uf = F.ultra(F.hang_first(m))
            key = (k, 'k+1' if m == k + 1 else ('1' if m == 1 else 'k+2'))
            tot[key] = tot.get(key, 0) + 1
            if ue != uf:
                diff[key] = diff.get(key, 0) + 1
                if key not in ex:
                    i = next(i for i in range(n) if any(ue[i][j] != uf[i][j] for j in range(n)))
                    ex[key] = (pts, k, m, i, [str(x) for x in F.hang_ec(m)[i]], [str(x) for x in F.hang_first(m)[i]])
for key in sorted(tot):
    print('k=%d m=%s : triplets %d, EC != first (ultrametrique) %d' % (key[0], key[1], tot[key], diff.get(key, 0)))
for key in sorted(ex):
    print('exemple', key, ex[key])
