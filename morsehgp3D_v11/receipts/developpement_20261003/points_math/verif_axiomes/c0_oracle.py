#!/usr/bin/env python3
"""C0 : recoupement de l'implantation independante (vfull) contre l'oracle du depot.

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B c0_oracle.py SEED NCLOUDS
Compare exactement (Fraction) les ultrametriques en niveau carre : Q1 o Pi_1 contre `margin1`, Q1 o Pi_m contre
`margin` (m = k + 1), core, cover, first ; et en rayon P1 o Pi_m contre `reference_radius_rules` (tolerance 1e-30).
Lecture seule du depot (aucun bytecode ecrit).
"""
import json
import random
import sys
import time
from fractions import Fraction

sys.dont_write_bytecode = True
BENCH = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench'
REF = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/reference'
sys.path.insert(0, REF)
sys.path.insert(0, BENCH)
import points_reference as pr  # noqa: E402
from hgp11_ref import Definition  # noqa: E402

import vfull as V  # noqa: E402
import mpmath  # noqa: E402


def cloud(rng):
    n = rng.randint(4, 8)
    k = rng.randint(2, min(4, n - 1))
    box = rng.choice([3, 5, 8, 40, 1000])
    pts = set()
    while len(pts) < n:
        pts.add((rng.randint(0, box), rng.randint(0, box), rng.randint(0, box) if rng.random() < 0.7 else 0))
    return sorted(pts), k


def main():
    seed, count = int(sys.argv[1]), int(sys.argv[2])
    rng = random.Random(seed)
    stats = {'clouds': 0, 'pairs': 0, 'mismatch': {}, 'radius_pairs': 0, 'radius_mismatch': 0, 'examples': []}
    t0 = time.time()
    for _ in range(count):
        pts, k = cloud(rng)
        n = len(pts)
        m = k + 1
        res = Definition(pts).order(k)
        ref, tree = pr.reference_rules(res, n, m)
        full = V.Full(V.Cloud(pts), k)
        mine = {'margin1': V.rule_Q(full, 1, 1), 'margin': V.rule_Q(full, 1, m), 'core': V.rule_core(full),
                'cover': V.rule_lca(full, 1, 'cover'), 'first': V.rule_lca(full, m, 'first')}
        for name, rule in mine.items():
            u = pr.reference_ultrametric(ref[name], tree)
            for i in range(n):
                for j in range(i, n):
                    stats['pairs'] += 1
                    if u[i][j] != rule.u2(i, j):
                        stats['mismatch'][name] = stats['mismatch'].get(name, 0) + 1
                        if len(stats['examples']) < 5:
                            stats['examples'].append({'pts': pts, 'k': k, 'rule': name, 'i': i, 'j': j,
                                                      'oracle': str(u[i][j]), 'mine': str(rule.u2(i, j))})
        rad, rtree = pr.reference_radius_rules(res, n, m)
        for name, mm in (('margin_r1', 1), ('margin_r', m)):
            urad = pr.reference_radius_ultrametric(rad[name], rtree)
            p = V.rule_P(full, 1, mm)
            for i in range(n):
                for j in range(i, n):
                    stats['radius_pairs'] += 1
                    if abs(mpmath.mpf(str(urad[i][j])) - p.u(i, j)) > mpmath.mpf(10) ** -25:
                        stats['radius_mismatch'] += 1
        stats['clouds'] += 1
    stats['seconds'] = round(time.time() - t0, 1)
    print(json.dumps(stats, indent=1))
    with open('recus_c0_oracle.json', 'w') as f:
        json.dump(stats, f, indent=1)


if __name__ == '__main__':
    main()
