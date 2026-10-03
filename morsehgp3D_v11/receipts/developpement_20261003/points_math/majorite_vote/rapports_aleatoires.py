#!/usr/bin/env python3
"""Rapport |Delta u| / delta sur des paires perturbees aleatoires DE MEME COMBINATOIRE (delta = decalage maximal des
niveaux et des debuts de couverture apparies, cf. pente.py), pour ER0h, ER0h a cone relatif, ER0hv, H_1, H_{k+1}.
Nuages : n de 4 a 7, K = 2 ou 3, treillis dilate (pas 1000 a 20000) puis bruit, un site deplace de +-1 sur un axe.
Usage : python3 -B rapports_aleatoires.py graine secondes > recus/rapports_aleatoires.json"""
from fractions import Fraction
import json
import random
import sys
import time

sys.dont_write_bytecode = True
import arbre  # noqa: E402
import regles as RG  # noqa: E402
from pente import match, delta_of, level_diff, MatchError  # noqa: E402

RULES = [('ER0h(1,12)', lambda t, k: RG.rule_er0h(t, 1, 12)),
         ('ER0hr(1,10)', lambda t, k: RG.rule_er0hr(t, 1, 10)),
         ('ER0hv(1,12,1/20)', lambda t, k: RG.rule_er0hv(t, 1, 12, Fraction(1, 20))),
         ('H_1', lambda t, k: RG.rule_hm(t, 1)),
         ('H_k+1', lambda t, k: RG.rule_hm(t, k + 1))]


def main():
    seed, budget = int(sys.argv[1]), float(sys.argv[2])
    rng = random.Random(seed)
    t0 = time.time()
    worst = dict((nm, (0.0, None)) for nm, _f in RULES)
    pairs = skipped = 0
    hist = dict((nm, [0, 0, 0, 0]) for nm, _f in RULES)   # rapports <= 1, <= 5, <= 20, > 20
    while time.time() - t0 < budget:
        n = rng.randint(4, 7)
        step = rng.choice([1000, 3000, 20000])
        pts = set()
        while len(pts) < n:
            pts.add((rng.randrange(4) * step + rng.randrange(-step // 4, step // 4 + 1),
                     rng.randrange(4) * step + rng.randrange(-step // 4, step // 4 + 1),
                     rng.choice([0, rng.randrange(3) * step])))
        X = sorted(pts)
        k = 2 if n < 5 else rng.choice([2, 3])
        j = rng.randrange(n)
        ax = rng.randrange(3)
        sgn = rng.choice([-1, 1])
        Y = [tuple(X[i][c] + (sgn if (i == j and c == ax) else 0) for c in range(3)) for i in range(n)]
        if len(set(Y)) < n:
            continue
        _dx, _rx, tx = arbre.v11_tree(X, k)
        _dy, _ry, ty = arbre.v11_tree(Y, k)
        try:
            mp = match(tx, ty)
            delta = delta_of(tx, ty, mp)
        except MatchError:
            skipped += 1
            continue
        if delta == 0:
            continue
        pairs += 1
        for nm, fn in RULES:
            ux = RG.ultrametric(tx, fn(tx, k))
            uy = RG.ultrametric(ty, fn(ty, k))
            best = Fraction(0)
            for a in range(n):
                for b in range(a, n):
                    lo, _hi = level_diff(ux[a][b], uy[a][b])
                    best = max(best, lo)
            r = float(best / delta)
            h = hist[nm]
            h[0 if r <= 1 else 1 if r <= 5 else 2 if r <= 20 else 3] += 1
            if r > worst[nm][0]:
                worst[nm] = (r, dict(X=X, Y=Y, k=k, delta=str(delta)))
    print(json.dumps(dict(graine=seed, secondes=round(time.time() - t0, 1), paires=pairs,
                          combinatoire_changee=skipped, histogramme_rapports=hist,
                          pires=dict((nm, {'rapport': w[0], 'cas': w[1]}) for nm, w in worst.items())),
                     indent=1, default=str))


if __name__ == '__main__':
    main()
