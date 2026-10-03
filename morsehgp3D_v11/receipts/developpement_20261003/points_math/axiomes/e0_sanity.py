#!/usr/bin/env python3
"""E0 : la regle H_kappa de hk.py (kappa = 1, echelle carree) redonne exactement margin1 / margin de l'oracle
points_reference (dates et proprietaires), sur les fixtures et des nuages aleatoires (n <= 8, k <= 3).

    python3 -B e0_sanity.py [graine] [nombre]
"""
import json
import random
import sys
import time

import hk

EQUILATERAL = [(1, 1, 2), (1, 2, 1), (2, 2, 2), (3, 3, 2), (4, 4, 2), (4, 3, 3)]
FIVE = [(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]


def random_cloud(rng):
    n = rng.randint(4, 8)
    side = rng.choice([3, 4, 6, 10, 40])
    pts = set()
    while len(pts) < n:
        pts.add((rng.randrange(side), rng.randrange(side), rng.choice([0, rng.randrange(side)])))
    pts = sorted(pts)
    rng.shuffle(pts)
    return pts


def check(points, stats):
    n = len(points)
    d = hk.Definition(points)
    for k in range(1, min(3, n) + 1):
        res = d.order(k)
        for m in sorted(set([1, k + 1])):
            if m > n:
                continue
            ref, _t = hk.pr.reference_rules(res, n, m)
            got, _tree = hk.rule_H(res, n, 1, m, 'sq')
            want = ref['margin1' if m == 1 else 'margin']
            stats['comparisons'] += 1
            if [tuple(x) for x in got] != [tuple(x) for x in want]:
                stats['disagreements'].append(dict(points=points, k=k, m=m))


def main():
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 20261003
    count = int(sys.argv[2]) if len(sys.argv) > 2 else 300
    rng = random.Random(seed)
    stats = dict(comparisons=0, disagreements=[], clouds=0)
    t0 = time.time()
    for pts in [EQUILATERAL, FIVE, [(0, 0, 0), (2, 0, 0), (4, 0, 0)], [(0, 0, 0), (2, 0, 0), (5, 0, 0)]]:
        check(pts, stats)
        stats['clouds'] += 1
    for _ in range(count):
        check(random_cloud(rng), stats)
        stats['clouds'] += 1
    stats['seconds'] = round(time.time() - t0, 1)
    stats['seed'] = seed
    print(json.dumps(stats, indent=1, default=str))
    return 1 if stats['disagreements'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
