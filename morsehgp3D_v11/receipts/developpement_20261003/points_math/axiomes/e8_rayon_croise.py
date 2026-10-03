#!/usr/bin/env python3
"""E8 : recoupe de P_1 o Pi_m (hk.py, rayon) avec reference_radius_rules du developpeur (ajoutee a 21 h 48 UTC,
bench/points_reference.py) : memes dates et memes hauteurs u (a 1e-50 pres), m in {1, k+1} ; les proprietaires
ne different qu'aux egalites exactes e = naissance d'un ancetre (diagnostic e8b_diag.py).

    python3 -B e8_rayon_croise.py GRAINE N
"""
import json
import random
import sys
import time

import hk
from e1_cibles import FIXTURES
from e2_e5 import random_cloud


def main():
    seed, count = int(sys.argv[1]), int(sys.argv[2])
    rng = random.Random(seed)
    clouds = [spec['points'] for spec in FIXTURES.values() if len(spec['points']) <= 9]
    clouds += [random_cloud(rng, rng.randint(4, 8)) for _ in range(count)]
    stats = dict(comparisons=0, sites=0, date_diff=0, owner_diff=0, u_diff=0, pairs=0, examples=[])
    t0 = time.time()
    for pts in clouds:
        n = len(pts)
        d = hk.Definition(pts)
        for k in range(1, min(4, n - 1) + 1):
            res = d.order(k)
            for m in sorted(set([1, k + 1])):
                ref, _t = hk.pr.reference_radius_rules(res, n, m)
                mine, _t2 = hk.rule_H(res, n, 1, m, 'rad')
                key = 'margin_r1' if m == 1 else 'margin_r'
                stats['comparisons'] += 1
                for i in range(n):
                    stats['sites'] += 1
                    (e1, o1), (e2, o2) = ref[key][i], mine[i]
                    if abs(e1 - e2) > hk.TIE:
                        stats['date_diff'] += 1
                        if len(stats['examples']) < 5:
                            stats['examples'].append(dict(points=pts, k=k, m=m, site=i, ref=str(e1), mine=str(e2)))
                    if o1 != o2:
                        stats['owner_diff'] += 1
                ur = hk.pr.reference_radius_ultrametric(ref[key], _t)
                um = hk.ultrametric(mine, _t2)
                for i in range(n):
                    for j in range(i + 1, n):
                        stats['pairs'] += 1
                        if abs(ur[i][j] - um[i][j]) > hk.TIE:
                            stats['u_diff'] += 1
    stats.update(seed=seed, clouds=len(clouds), seconds=round(time.time() - t0, 1))
    print(json.dumps(stats, indent=1, default=str))
    return 1 if stats['date_diff'] or stats['u_diff'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
