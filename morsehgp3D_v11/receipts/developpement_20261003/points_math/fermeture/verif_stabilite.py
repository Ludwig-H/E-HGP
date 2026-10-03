#!/usr/bin/env python3
"""Stabilite sous perturbation appariee : fermeture qualifiee (borne 1 eps en rayon) contre regles fideles.

    python3 verif_stabilite.py --clouds 300 --seed 7 --out resultats/stabilite.json

Chaque site bouge de delta_i dans {-1, 0, 1}^3 (sites restant distincts) ; eps^2 = max |delta_i|^2.
Pour chaque (nuage, k, m) et chaque paire i <= j (diagonale = entree) :
  - fermeture : |sqrt(u_X) - sqrt(u_Y)| <= eps decide exactement (lib_fermeture.within_radius) ;
  - toutes les regles : rapport |sqrt(u_X) - sqrt(u_Y)| / eps (flottant, diagnostic) et, comme le developpeur,
    rapport |u_X - u_Y| / delta avec delta = 2 eps sqrt(Lambda) + eps^2 (Lambda = plus haut niveau des deux arbres).
Regles : fermeture (m), H_m ('margin'), H_1 ('margin1'), first (m), cover, core, fermeture exclusive EC (m).
"""
import argparse
import json
import math
import random
import sys
import time

sys.dont_write_bytecode = True
import lib_fermeture as lf  # noqa: E402

RULES = ('closure', 'margin', 'margin1', 'first', 'cover', 'core', 'ec')


def perturb(rng, pts):
    while True:
        moves = [tuple(rng.choice((-1, 0, 1)) for _ in range(3)) for _ in pts]
        out = [tuple(a + b for a, b in zip(p, d)) for p, d in zip(pts, moves)]
        if len(set(out)) == len(out) and any(any(d) for d in moves):
            return out, max(sum(c * c for c in d) for d in moves)


def ultrametrics(defn, n, k, m):
    res = defn.order(k)
    out = {'closure': lf.closure(res, n, m)['u']}
    ref, tree = lf.faithful_rules(res, n, m)
    for rule in ('margin', 'margin1', 'first', 'cover', 'core'):
        out[rule] = lf.ultrametric_of(ref[rule], tree)
    out['ec'] = lf.ultrametric_of(lf.ec_hanging(res, n, m), tree)
    top = max(nd.level for nd in res.nodes)
    return out, top


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--clouds', type=int, default=300)
    parser.add_argument('--seed', type=int, default=7)
    parser.add_argument('--seconds', type=float, default=240.0)
    parser.add_argument('--out', required=True)
    args = parser.parse_args()
    rng = random.Random(args.seed)
    stats = {r: dict(pairs=0, max_ratio_r=0.0, max_ratio_delta=0.0, over_1eps=0, over_3eps=0, over_5eps=0,
                     over_5delta=0) for r in RULES}
    closure_exact_viol = 0
    worst = {}
    started = time.monotonic()
    clouds = 0
    for c in range(args.clouds):
        if time.monotonic() - started > args.seconds:
            break
        pts = lf.cloud_gate(rng) if c % 2 == 0 else lf.cloud_generic(rng)
        qts, eps2 = perturb(rng, pts)
        n = len(pts)
        dx, dy = lf.Definition(pts), lf.Definition(qts)
        eps = math.sqrt(eps2)
        for k in range(1, min(3, n - 1) + 1):
            for m in sorted(set([1, k + 1, k + 2])):
                if m > n:
                    continue
                ux, topx = ultrametrics(dx, n, k, m)
                uy, topy = ultrametrics(dy, n, k, m)
                lam = max(topx, topy)
                delta = 2 * eps * math.sqrt(lam) + eps2
                for r in RULES:
                    a, b = ux[r], uy[r]
                    st = stats[r]
                    for i in range(n):
                        for j in range(i, n):
                            st['pairs'] += 1
                            gap = lf.radius_gap(a[i][j], b[i][j])
                            ratio = gap / eps
                            dr = abs(float(a[i][j] - b[i][j])) / delta if delta > 0 else 0.0
                            if ratio > st['max_ratio_r']:
                                st['max_ratio_r'] = ratio
                                worst[r] = dict(points=pts, perturbed=qts, k=k, m=m, i=i, j=j, ux=str(a[i][j]),
                                                uy=str(b[i][j]), eps2=eps2)
                            st['max_ratio_delta'] = max(st['max_ratio_delta'], dr)
                            st['over_1eps'] += ratio > 1 + 1e-9
                            st['over_3eps'] += ratio > 3 + 1e-9
                            st['over_5eps'] += ratio > 5 + 1e-9
                            st['over_5delta'] += dr > 5 + 1e-9
                            if r == 'closure' and not lf.within_radius(a[i][j], b[i][j], eps2):
                                closure_exact_viol += 1
        clouds += 1
    out = dict(clouds=clouds, seed=args.seed, seconds=round(time.monotonic() - started, 1), stats=stats,
               closure_exact_violations_1eps=closure_exact_viol, worst=worst)
    with open(args.out, 'w') as fh:
        json.dump(out, fh, indent=1, sort_keys=True, default=str)
    print('nuages perturbes %d en %.1f s' % (clouds, time.monotonic() - started))
    print('%-8s %9s %12s %14s %8s %8s %8s %9s' % ('regle', 'paires', 'max|dr|/eps', 'max|du|/delta', '>1eps',
                                                  '>3eps', '>5eps', '>5delta'))
    for r in RULES:
        st = stats[r]
        print('%-8s %9d %12.3f %14.3f %8d %8d %8d %9d' % (r, st['pairs'], st['max_ratio_r'], st['max_ratio_delta'],
                                                          st['over_1eps'], st['over_3eps'], st['over_5eps'],
                                                          st['over_5delta']))
    print('fermeture : violations exactes de |dsqrt(u)| <= eps : %d' % closure_exact_viol)
    return 1 if closure_exact_viol else 0


if __name__ == '__main__':
    raise SystemExit(main())
