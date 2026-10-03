#!/usr/bin/env python3
"""Recherche adverse ciblee (meme principe que sauts.py) pour le vote de la these A TOUTES LES FACES avec cone
(VOTE[p, all, kappa = 12]), p = 0 et 2. Usage : python3 -B sauts_cible.py essais graine > recus/sauts_cible.json"""
from fractions import Fraction
import json
import random
import sys
import time

sys.dont_write_bytecode = True
import regles as RG  # noqa: E402
from sauts import jump  # noqa: E402


def main():
    trials = int(sys.argv[1])
    seed = int(sys.argv[2])
    rng = random.Random(seed)
    t0 = time.time()
    rules = [('VOTE[p2,all,k12]', 2), ('VOTE[p0,all,k12]', 0)]
    counts = dict((nm, [0, 0]) for nm, _p in rules)
    found = {}
    for _ in range(trials):
        n = rng.randint(4, 7)
        pts = set()
        while len(pts) < n:
            pts.add((rng.randrange(4), rng.randrange(4), rng.choice([0, 0, 1])))
        P = sorted(pts)
        k = 2 if n < 6 else rng.choice([2, 3])
        j = rng.randrange(n)
        e = rng.choice([(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1)])
        for nm, p in rules:
            fn = (lambda pp: (lambda d, t: RG.rule_vote(d, t, k, pp, 'all', Fraction(12))))(p)
            js = []
            for D in (1000, 10000):
                X = [tuple(D * c for c in q) for q in P]
                Y = [tuple(D * P[i][c] + (e[c] if i == j else 0) for c in range(3)) for i in range(n)]
                js.append(jump(X, Y, k, fn))
            counts[nm][0] += 1
            if js[0][0] > 1 and js[1][0] >= 9 * js[0][0]:
                counts[nm][1] += 1
                if nm not in found or n < found[nm]['n']:
                    D = 100000
                    X = [tuple(D * c for c in q) for q in P]
                    Y = [tuple(D * P[i][c] + (e[c] if i == j else 0) for c in range(3)) for i in range(n)]
                    found[nm] = dict(n=n, k=k, P=P, site_deplace=j, deplacement=e, saut_D1000=float(js[0][0]),
                                     saut_D10000=float(js[1][0]), saut_D100000=float(jump(X, Y, k, fn)[0]),
                                     paire=js[1][1])
        if time.time() - t0 > 240:
            break
    print(json.dumps(dict(essais=trials, graine=seed, secondes=round(time.time() - t0, 1), comptes=counts,
                          discontinuites_minimales=found), indent=1, default=str))


if __name__ == '__main__':
    main()
