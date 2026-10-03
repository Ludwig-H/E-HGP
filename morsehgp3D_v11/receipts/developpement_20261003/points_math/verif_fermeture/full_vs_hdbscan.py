#!/usr/bin/env python3
"""Corollaire T2 o T3 : les couvertures FULL_k (sans qualification) et la liaison simple de l'atteignabilite
mutuelle (min_samples = k, site compte) sont 2-entrelacees en rayon :
  (i)  chaque couverture E_C(a) tient dans un bloc de SL_mr au niveau 4a ;
  (ii) chaque bloc de SL_mr au niveau a (sites actifs D_k <= a) tient dans UNE couverture FULL au niveau 4a.
On mesure aussi le plus petit facteur c^2 qui suffit (atteinte de 4 ?). k >= 2, vf_oracle seul.

    PYTHONDONTWRITEBYTECODE=1 python3 full_vs_hdbscan.py --clouds 300 --seed 5150
"""
import argparse
import random
import sys
import time
from fractions import Fraction
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-points-math/verif_fermeture')
import vf_oracle as vo
from verif_theoremes import gen_ties, gen_generic, gen_blobs


def sl_blocks(umr, a):
    return vo.blocks(umr, a)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--clouds', type=int, default=300)
    ap.add_argument('--seed', type=int, default=5150)
    a = ap.parse_args()
    rng = random.Random(a.seed)
    t0 = time.time()
    chk = fail1 = fail2 = 0
    worst1 = worst2 = Fraction(0)
    ex = None
    for c in range(a.clouds):
        pts = (gen_ties, gen_generic, gen_blobs)[c % 3](rng)
        n = len(pts)
        cl = vo.Cloud(pts)
        for k in range(2, min(3, n - 1) + 1):
            F = vo.Full(cl, k)
            Dk = [cl.Dk(i, k) for i in range(n)]
            mr = [[max(Dk[i], Dk[j], cl.d2(i, j)) if i != j else Dk[i] for j in range(n)] for i in range(n)]
            umr = vo.minmax(mr, n)
            # (i) couverture a -> bloc SL_mr ; facteur necessaire = max sur paires couvertes de umr(i,j)/a
            for lev, snap in F.snaps:
                for _v, cov in snap:
                    s = vo.bits(cov)
                    need = max(umr[i][j] for i in s for j in s) / lev if lev > 0 else Fraction(0)
                    worst1 = max(worst1, need)
                    chk += 1
                    if need > 4:
                        fail1 += 1
            # (ii) bloc SL_mr au niveau L -> une couverture FULL : facteur necessaire = premier niveau ou une
            #      couverture contient le bloc, divise par L
            levels = sorted(set(umr[i][j] for i in range(n) for j in range(n)))
            for L in levels:
                if L == 0:
                    continue
                for b in vo.blocks(umr, L):
                    bm = sum(1 << i for i in b)
                    first = None
                    for lev, snap in F.snaps:
                        if any(bm & ~cov == 0 for _v, cov in snap):
                            first = lev
                            break
                    need = first / L
                    chk += 1
                    if need > worst2:
                        worst2 = need
                        ex = (pts, k, sorted(b), str(L), str(first))
                    if need > 4:
                        fail2 += 1
    print('controles %d en %.1f s ; (i) couverture FULL au niveau a dans un bloc SL_mr a 4a : echecs %d, facteur'
          ' carre max necessaire %.4f ; (ii) bloc SL_mr au niveau a dans une couverture FULL a 4a : echecs %d,'
          ' facteur carre max necessaire %.4f' % (chk, time.time() - t0, fail1, float(worst1), fail2, float(worst2)))
    print('pire cas (ii) :', ex)


if __name__ == '__main__':
    main()
