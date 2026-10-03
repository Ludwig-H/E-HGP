#!/usr/bin/env python3
"""Recherche adverse de discontinuites (meme principe que verdict/recherche_sauts.py en v10, code independant) :
une configuration P de treillis, riche en egalites exactes, est dilatee par D, puis un site est deplace d'une unite.
Toutes les regles comparees sont invariantes par homothetie (rapports de niveaux) : si le saut maximal de hauteur
(en rayon, sur toutes les paires) est proportionnel a D, la regle est discontinue en P (dans le nuage normalise, le
deplacement 1/D tend vers 0 et le saut reste constant). On juge a D et 10 D, et on confirme a 100 D.

Usage : python3 -B sauts.py essais graine > recus/sauts.json
"""
from fractions import Fraction
import json
import random
import sys
import time

sys.dont_write_bytecode = True
import arbre  # noqa: E402
import regles as RG  # noqa: E402


def rules(k):
    return [('ER0h(1,12)', lambda d, t: RG.rule_er0h(t, 1, 12)),
            ('H_k+1', lambda d, t: RG.rule_hm(t, k + 1)),
            ('VOTE[p2,gab]', lambda d, t: RG.rule_vote(d, t, k, 2, 'gabriel', None)),
            ('VOTE[p2,gab,k12]', lambda d, t: RG.rule_vote(d, t, k, 2, 'gabriel', Fraction(12))),
            ('VOTE[p2,all]', lambda d, t: RG.rule_vote(d, t, k, 2, 'all', None)),
            ('VOTE[p2,all,k12]', lambda d, t: RG.rule_vote(d, t, k, 2, 'all', Fraction(12)))]


def jump(px, py, k, fn):
    dx, _rx, tx = arbre.v11_tree(px, k)
    dy, _ry, ty = arbre.v11_tree(py, k)
    ux = RG.ultrametric(tx, fn(dx, tx))
    uy = RG.ultrametric(ty, fn(dy, ty))
    n = len(px)
    best = (Fraction(0), None)
    for i in range(n):
        for j in range(i, n):
            lo, hi = (ux[i][j] - uy[i][j]).bounds(120)
            m = max(abs(lo), abs(hi))
            if m > best[0]:
                best = (m, (i, j))
    return best


def main():
    trials = int(sys.argv[1]) if len(sys.argv) > 1 else 100
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 11
    rng = random.Random(seed)
    t0 = time.time()
    counts = {}
    found = {}
    tested = 0
    for _ in range(trials):
        n = rng.randint(4, 6)
        pts = set()
        while len(pts) < n:
            pts.add((rng.randrange(4), rng.randrange(4), rng.choice([0, 0, rng.randrange(3)])))
        P = sorted(pts)
        k = rng.choice([2, 2, 3]) if n >= 5 else 2
        j = rng.randrange(n)
        e = rng.choice([(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1)])
        for name, fn in rules(k):
            js = []
            for D in (1000, 10000):
                X = [tuple(D * c for c in p) for p in P]
                Y = [tuple(D * P[i][c] + (e[c] if i == j else 0) for c in range(3)) for i in range(n)]
                js.append(jump(X, Y, k, fn))
            tested += 1
            c = counts.setdefault(name, [0, 0])
            c[0] += 1
            if js[0][0] > 1 and js[1][0] >= 9 * js[0][0]:
                c[1] += 1
                if name not in found or n < found[name]['n']:
                    D = 100000
                    X = [tuple(D * c2 for c2 in p) for p in P]
                    Y = [tuple(D * P[i][c2] + (e[c2] if i == j else 0) for c2 in range(3)) for i in range(n)]
                    j3 = jump(X, Y, k, fn)
                    found[name] = dict(n=n, k=k, P=P, site_deplace=j, deplacement=e,
                                       saut_D1000=float(js[0][0]), saut_D10000=float(js[1][0]), saut_D100000=float(j3[0]),
                                       paire=js[1][1])
    print(json.dumps(dict(essais=trials, graine=seed, secondes=round(time.time() - t0, 1), comptes=counts,
                          discontinuites_minimales=found), indent=1, default=str))


if __name__ == '__main__':
    main()
