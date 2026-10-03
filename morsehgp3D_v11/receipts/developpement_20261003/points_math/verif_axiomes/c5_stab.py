#!/usr/bin/env python3
"""C5 : stabilite en rayon (proposition D du rapport : P_kappa o Pi en (1 + 2 kappa) eps, dates et hauteurs).

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B c5_stab.py SEED NRANDOM NCLIMB STEPS
  phase 1 : nuages aleatoires, perturbation de chaque coordonnee dans {-1, 0, 1}, eps = max |x_i - y_i| ;
  phase 2 : montee adverse (recherche locale) maximisant max |du| / eps pour P1 o Pi_1 et P1 o Pi_{k+1}.
Rapport max |du|/eps par regle ; toute valeur > 1 + 2 kappa refuterait la proposition D (ou P5).
"""
import json
import random
import sys
import time

sys.dont_write_bytecode = True
import mpmath  # noqa: E402
import vfull as V  # noqa: E402


def eps_of(a, b):
    return max(mpmath.sqrt(sum((p - q) ** 2 for p, q in zip(x, y))) for x, y in zip(a, b))


def rules(pts, k):
    full = V.Full(V.Cloud(pts), k)
    return {'P1': V.rule_P(full, 1, 1), 'P2': V.rule_P(full, 2, 1), 'P1q': V.rule_P(full, 1, k + 1),
            'Q1': V.rule_Q(full, 1, 1), 'core': V.rule_core(full), 'clos': V.Closure(full, k + 1)}


def ratios(A, B, n, eps):
    out = {}
    for name in A:
        best = mpmath.mpf(0)
        for i in range(n):
            for j in range(i, n):
                d = abs(A[name].u(i, j) - B[name].u(i, j))
                if d > best:
                    best = d
        out[name] = best / eps
    return out


def perturb(rng, pts):
    while True:
        q = [tuple(c + rng.choice((-1, 0, 1)) for c in p) for p in pts]
        if len(set(q)) == len(q) and q != pts:
            return q


def main():
    seed, nrand, nclimb, steps = map(int, sys.argv[1:5])
    rng = random.Random(seed)
    best = {}
    witness = {}
    t0 = time.time()
    pairs = 0
    for _ in range(nrand):
        n = rng.randint(4, 7)
        k = rng.randint(2, min(3, n - 1))
        box = rng.choice([8, 20, 60])
        pts = set()
        while len(pts) < n:
            pts.add((rng.randint(0, box), rng.randint(0, box), rng.randint(0, box)))
        pts = sorted(pts)
        q = perturb(rng, pts)
        eps = eps_of(pts, q)
        r = ratios(rules(pts, k), rules(q, k), n, eps)
        pairs += 1
        for name, v in r.items():
            if v > best.get(name, 0):
                best[name] = v
                witness[name] = {'X': pts, 'Y': q, 'k': k, 'ratio': float(v)}
    phase1 = {k: float(v) for k, v in best.items()}
    # phase 2 : montee adverse sur P1 (et P1q) ; perturbation unite sur un seul point, configuration mobile
    climb_best = {'P1': mpmath.mpf(0), 'P1q': mpmath.mpf(0)}
    climb_w = {}
    for _ in range(nclimb):
        n = rng.randint(3, 6)
        k = 2 if n <= 4 else rng.randint(2, 3)
        box = 40
        pts = set()
        while len(pts) < n:
            pts.add((rng.randint(0, box), rng.randint(0, box), rng.randint(0, 2)))
        pts = sorted(pts)
        target = rng.choice(['P1', 'P1q'])

        def score(cfg):
            out = mpmath.mpf(0)
            for _ in range(3):
                q = perturb(rng, cfg)
                eps = eps_of(cfg, q)
                A = rules(cfg, k)
                B = rules(q, k)
                for i in range(n):
                    for j in range(i, n):
                        d = abs(A[target].u(i, j) - B[target].u(i, j)) / eps
                        if d > out:
                            out = d
                            score.last = (q, d)
            return out
        score.last = None
        cur = score(pts)
        curw = score.last
        for _ in range(steps):
            cand = list(pts)
            i = rng.randrange(n)
            cand[i] = tuple(c + rng.randint(-3, 3) for c in cand[i])
            if len(set(cand)) != n:
                continue
            s = score(cand)
            if s >= cur:
                pts, cur, curw = cand, s, score.last
        if cur > climb_best[target]:
            climb_best[target] = cur
            climb_w[target] = {'X': pts, 'Y': curw[0] if curw else None, 'k': k, 'ratio': float(cur)}
    rec = {'phase1_pairs': pairs, 'phase1_max_ratio': phase1, 'phase1_witness': witness,
           'phase2_max_ratio': {k: float(v) for k, v in climb_best.items()}, 'phase2_witness': climb_w,
           'bounds': {'P1': 3, 'P1q': 3, 'P2': 5, 'core': 2, 'clos': 1, 'Q1': 'aucune uniforme'},
           'seconds': round(time.time() - t0, 1)}
    with open('recus_c5_stab_%d.json' % seed, 'w') as fh:
        json.dump(rec, fh, indent=1)
    print(json.dumps({k: rec[k] for k in ('phase1_pairs', 'phase1_max_ratio', 'phase2_max_ratio', 'seconds')},
                     indent=1))


if __name__ == '__main__':
    main()
