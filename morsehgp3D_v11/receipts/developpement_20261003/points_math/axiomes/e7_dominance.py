#!/usr/bin/env python3
"""E7 : domination de Q_1 (H_1 en niveau carre, regle du developpeur) par P_1 (meme formule en rayon).

Verifie sur nuages aleatoires (n <= 8, k <= 4, m in {1, k+1}) : date_P1 <= sqrt(date_Q1) pour chaque site, meme
lignee (le proprietaire de P_1 est un descendant ou egal du proprietaire de Q_1), et u_P1(i,j) <= sqrt(u_Q1(i,j)).
Compte les sites strictement plus precoces. Aucune decision flottante : comparaisons en Decimal a 80 chiffres,
quasi-egalites signalees (hk.NEAR_TIES).

    python3 -B e7_dominance.py GRAINE N
"""
import json
import random
import sys
import time

import hk
from e2_e5 import random_cloud


def main():
    seed, count = int(sys.argv[1]), int(sys.argv[2])
    rng = random.Random(seed)
    stats = dict(sites=0, strict=0, violations_date=0, violations_lignee=0, violations_u=0, pairs=0)
    t0 = time.time()
    for _ in range(count):
        n = rng.randint(4, 8)
        pts = random_cloud(rng, n)
        d = hk.Definition(pts)
        for k in range(1, min(4, n - 1) + 1):
            res = d.order(k)
            for m in sorted(set([1, k + 1])):
                ep, tp = hk.rule_H(res, n, 1, m, 'rad')
                eq, tq = hk.rule_H(res, n, 1, m, 'sq')
                up, uq = hk.ultrametric(ep, tp), hk.ultrametric(eq, tq)
                for i in range(n):
                    if ep[i] is None:
                        continue
                    stats['sites'] += 1
                    rq = hk.dsqrt(eq[i][0])
                    if ep[i][0] > rq + hk.TIE:
                        stats['violations_date'] += 1
                    elif ep[i][0] < rq - hk.TIE:
                        stats['strict'] += 1
                    if ep[i][1] not in tp.chain(ep[i][1]) or eq[i][1] not in tp.chain(ep[i][1]):
                        stats['violations_lignee'] += 1
                    for j in range(i + 1, n):
                        if up[i][j] is None:
                            continue
                        stats['pairs'] += 1
                        if up[i][j] > hk.dsqrt(uq[i][j]) + hk.TIE:
                            stats['violations_u'] += 1
    stats.update(seed=seed, clouds=count, seconds=round(time.time() - t0, 1), near_ties=hk.NEAR_TIES[0])
    print(json.dumps(stats, indent=1))
    return 1 if stats['violations_date'] or stats['violations_lignee'] or stats['violations_u'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
