#!/usr/bin/env python3
"""Extension L01 de la porte T2 de la tour : memes controles que tests/oracle/test_tower_oracle.py (composantes et
partition C n X contre Gamma_k, coupes ouvertes et fermees), mais a K = 10 (ordres 1..10) sur des nuages de 11 a 13
points, ce que la porte du depot ne fait pas (elle s'arrete a K = 5, n <= 12).
Usage : tower_oracle_k10.py BUILD_DIR TESTS_ORACLE_DIR [nuages] [graine]"""
import os
import random
import sys
import tempfile

sys.path.insert(0, sys.argv[2])
sys.path.insert(0, os.path.join(sys.argv[2], '..', '..', 'reference'))
import test_tower_oracle as T  # noqa: E402


def clouds(count, rnd):
    for t in range(count):
        n = rnd.randint(11, 13)
        kind = t % 5
        pts = set()
        while len(pts) < n:
            if kind == 0:
                pts.add(tuple(rnd.randint(0, 1000) for _ in range(3)))
            elif kind == 1:
                pts.add(tuple(rnd.randint(0, 2) for _ in range(3)))
            elif kind == 2:
                pts.add((rnd.randint(0, 4), rnd.randint(0, 4), 0))
            elif kind == 3:
                pts.add(tuple(rnd.choice((0, 2, 4)) + rnd.randint(0, 1) for _ in range(3)))
            else:
                pts.add(tuple(rnd.randint(0, 3) for _ in range(3)))
        P = sorted(pts)
        rnd.shuffle(P)
        yield kind, P


def main():
    exe = os.path.join(sys.argv[1], 'mhgp10_tower')
    count = int(sys.argv[3]) if len(sys.argv) > 3 else 10
    seed = int(sys.argv[4]) if len(sys.argv) > 4 else 20261002
    rnd = random.Random(seed)
    checks = fails = cuts = 0
    kinds = {}
    with tempfile.TemporaryDirectory(dir=os.environ.get('L01_TMP')) as tmp:
        for kind, P in clouds(count, rnd):
            err, c = T.check(exe, P, 10, tmp)
            checks += 1
            cuts += c
            kinds[kind] = kinds.get(kind, 0) + 1
            if err:
                fails += 1
                print('ECART/REFUS K=10 n=%d genre=%d : %s\n  %s' % (len(P), kind, err, P), flush=True)
            else:
                print('ok genre=%d n=%d coupes=%d' % (kind, len(P), c), flush=True)
    print('tower_oracle_k10 checks %d fails %d cuts %d genres %s' % (checks, fails, cuts, kinds))
    return 1 if fails else (3 if cuts < 500 else 0)


if __name__ == '__main__':
    sys.exit(main())
