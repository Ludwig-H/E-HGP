#!/usr/bin/env python3
"""Recherche adverse de discontinuites pour la famille ER (meme principe que sauts.py ; arbres calcules une fois par
dilatation). Temoin positif attendu : ER0 (discontinue, v10). Usage : python3 -B sauts_er.py essais graine secondes"""
from fractions import Fraction
import json
import random
import sys
import time

sys.dont_write_bytecode = True
import arbre  # noqa: E402
import regles as RG  # noqa: E402

RULES = [('ER0(1,12)', lambda t: RG.rule_er0(t, 1, 12)),
         ('ER0h(1,12)', lambda t: RG.rule_er0h(t, 1, 12)),
         ('ER0hv(1,12,1/20)', lambda t: RG.rule_er0hv(t, 1, 12, Fraction(1, 20))),
         ('ER0hv(1,12,2/5)', lambda t: RG.rule_er0hv(t, 1, 12, Fraction(2, 5))),
         ('H_k+1', None)]


def max_jump(ux, uy):
    n = len(ux)
    best = (Fraction(0), None)
    for i in range(n):
        for j in range(i, n):
            lo, hi = (ux[i][j] - uy[i][j]).bounds(120)
            m = max(abs(lo), abs(hi))
            if m > best[0]:
                best = (m, (i, j))
    return best


def main():
    trials, seed, budget = int(sys.argv[1]), int(sys.argv[2]), float(sys.argv[3])
    rng = random.Random(seed)
    t0 = time.time()
    counts = dict((nm, [0, 0]) for nm, _f in RULES)
    found = {}
    done = 0
    for _ in range(trials):
        n = rng.randint(4, 7)
        pts = set()
        while len(pts) < n:
            pts.add((rng.randrange(4), rng.randrange(4), rng.choice([0, 0, rng.randrange(3)])))
        P = sorted(pts)
        k = rng.choice([2, 3]) if n >= 5 else 2
        j = rng.randrange(n)
        e = rng.choice([(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)])
        jumps = dict((nm, []) for nm, _f in RULES)
        for D in (1000, 10000):
            X = [tuple(D * c for c in q) for q in P]
            Y = [tuple(D * P[i][c] + (e[c] if i == j else 0) for c in range(3)) for i in range(n)]
            _dx, _rx, tx = arbre.v11_tree(X, k)
            _dy, _ry, ty = arbre.v11_tree(Y, k)
            for nm, fn in RULES:
                f = fn if fn is not None else (lambda t: RG.rule_hm(t, k + 1))
                jumps[nm].append(max_jump(RG.ultrametric(tx, f(tx)), RG.ultrametric(ty, f(ty))))
        for nm, _f in RULES:
            js = jumps[nm]
            counts[nm][0] += 1
            if js[0][0] > 1 and js[1][0] >= 9 * js[0][0]:
                counts[nm][1] += 1
                if nm not in found or n < found[nm]['n']:
                    found[nm] = dict(n=n, k=k, P=P, site_deplace=j, deplacement=e, saut_D1000=float(js[0][0]),
                                     saut_D10000=float(js[1][0]), paire=js[1][1])
        done += 1
        if time.time() - t0 > budget:
            break
    print(json.dumps(dict(essais_demandes=trials, essais_faits=done, graine=seed, secondes=round(time.time() - t0, 1),
                          comptes=counts, discontinuites_minimales=found), indent=1, default=str))


if __name__ == '__main__':
    main()
