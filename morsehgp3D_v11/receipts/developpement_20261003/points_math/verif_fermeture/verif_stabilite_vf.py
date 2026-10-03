#!/usr/bin/env python3
"""Stabilite sous perturbation, oracle independant vf_oracle.

    PYTHONDONTWRITEBYTECODE=1 python3 verif_stabilite_vf.py --clouds 500 --seed 777 --seconds 240

Nuage X entier (petites coordonnees pour que la perturbation compte), Y = X + d_i, d_i dans {-1,0,1}^3, sites
distincts des deux cotes ; eps^2 = max |d_i|^2. Pour k in {1,2,3}, m in {1, k+1, k+2} :
  - test EXACT de |sqrt(u^X(i,j)) - sqrt(u^Y(i,j))| <= eps pour la fermeture (diagonale comprise) ;
  - rapports maximaux |Delta sqrt(u)| / eps (flottant) pour fermeture, H_m, H_1, core, first, cover, EC ;
  - rapport |Delta u| / delta, delta = 2 eps sqrt(Lambda) + eps^2 (Lambda = plus grand niveau racine), pour H_m.
"""
import argparse
import json
import math
import random
import sys
import time

sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-points-math/verif_fermeture')
import vf_oracle as vo  # noqa: E402


def gen(rng):
    kind = rng.choice(['grid', 'box', 'two'])
    n = rng.randint(4, 8)
    s = set()
    if kind == 'grid':
        side, sc = rng.choice([3, 4, 5]), rng.choice([2, 3, 5])
        while len(s) < n:
            s.add((rng.randrange(side) * sc, rng.randrange(side) * sc, rng.choice([0, rng.randrange(side) * sc])))
    elif kind == 'box':
        b = rng.choice([8, 15, 30])
        while len(s) < n:
            s.add((rng.randrange(b), rng.randrange(b), rng.randrange(b)))
    else:
        t, L = rng.randint(3, 8), rng.randint(15, 40)
        while len(s) < n:
            if rng.random() < 0.45:
                s.add((rng.randrange(t), rng.randrange(t), rng.randrange(t)))
            elif rng.random() < 0.8:
                s.add((L + rng.randrange(t), rng.randrange(t), rng.randrange(t)))
            else:
                s.add((rng.randrange(t, L), rng.randrange(-t, 2 * t), rng.randrange(-t, 2 * t)))
    pts = sorted(s)
    rng.shuffle(pts)
    return pts


def perturb(rng, pts):
    for _ in range(50):
        d = [(rng.choice([-1, 0, 1]), rng.choice([-1, 0, 1]), rng.choice([-1, 0, 1])) for _ in pts]
        q = [tuple(a + b for a, b in zip(p, dd)) for p, dd in zip(pts, d)]
        if len(set(q)) == len(q) and any(any(dd) for dd in d):
            return q, max(sum(x * x for x in dd) for dd in d)
    return None, None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--clouds', type=int, default=500)
    ap.add_argument('--seed', type=int, default=777)
    ap.add_argument('--seconds', type=float, default=240)
    ap.add_argument('--out', default='resultats/stabilite_vf.json')
    a = ap.parse_args()
    rng = random.Random(a.seed)
    t0 = time.time()
    comps = 0
    viol = 0
    worst = {}
    worst_delta = {}
    viol_examples = []
    pairs = 0
    for c in range(a.clouds):
        if time.time() - t0 > a.seconds:
            break
        X = gen(rng)
        Y, e2 = perturb(rng, X)
        if Y is None:
            continue
        pairs += 1
        n = len(X)
        cx, cy = vo.Cloud(X), vo.Cloud(Y)
        eps = math.sqrt(e2)
        for k in (1, 2, 3):
            if k >= n:
                continue
            FX, FY = vo.Full(cx, k), vo.Full(cy, k)
            Lam = max(FX.levels_of_node[FX.root], FY.levels_of_node[FY.root])
            delta = 2 * eps * math.sqrt(Lam) + e2
            for m in sorted(set([1, k + 1, k + 2])):
                if m > n:
                    continue
                ux, _ = FX.closure(m)
                uy, _ = FY.closure(m)
                for i in range(n):
                    for j in range(n):
                        comps += 1
                        if not vo.sqrt_gap_le(ux[i][j], uy[i][j], e2):
                            viol += 1
                            if len(viol_examples) < 5:
                                viol_examples.append((X, Y, k, m, i, j, str(ux[i][j]), str(uy[i][j])))
                rules = dict(fermeture=(ux, uy))
                for name, fx, fy in (('Hm', FX.hang_margin(m), FY.hang_margin(m)),
                                     ('H1', FX.hang_margin(1), FY.hang_margin(1)),
                                     ('core', FX.hang_core(), FY.hang_core()),
                                     ('first', FX.hang_first(m), FY.hang_first(m)),
                                     ('cover', FX.hang_first(1), FY.hang_first(1)),
                                     ('EC', FX.hang_ec(m), FY.hang_ec(m))):
                    rules[name] = (FX.ultra(fx), FY.ultra(fy))
                for name, (p, q) in rules.items():
                    r = max(abs(math.sqrt(p[i][j]) - math.sqrt(q[i][j])) for i in range(n) for j in range(n)) / eps
                    if r > worst.get(name, (0,))[0]:
                        worst[name] = (r, X, Y, k, m)
                    rd = max(abs(float(p[i][j] - q[i][j])) for i in range(n) for j in range(n)) / delta
                    worst_delta[name] = max(worst_delta.get(name, 0), rd)
    el = time.time() - t0
    print('paires de nuages %d, comparaisons exactes fermeture %d, violations de 1 eps %d, %.1f s'
          % (pairs, comps, viol, el))
    for name in sorted(worst):
        print('%-10s max |Delta sqrt u|/eps = %.4f   max |Delta u|/delta = %.4f' % (
            name, worst[name][0], worst_delta[name]))
    print('pire cas fermeture :', worst['fermeture'][1:])
    print('pire cas Hm :', worst['Hm'][1:])
    for v in viol_examples:
        print('VIOLATION', v)
    with open(a.out, 'w') as fh:
        json.dump(dict(pairs=pairs, comparisons=comps, violations=viol, seconds=round(el, 1),
                       worst={k: v[0] for k, v in worst.items()}, worst_delta=worst_delta,
                       worst_cases={k: [str(x) for x in v[1:]] for k, v in worst.items()}), fh, indent=1)


if __name__ == '__main__':
    main()
