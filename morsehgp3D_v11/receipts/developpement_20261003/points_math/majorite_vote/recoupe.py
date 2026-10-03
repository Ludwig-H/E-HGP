#!/usr/bin/env python3
"""Recoupes : (1) arbre v11 (hgp11_ref) = arbre v10 (vfull) ; (2) H_m ecrit ici = oracle du developpeur
(points_reference 'margin' et 'margin1') ; (3) ER0h ecrit ici (credits explicites, arbre v11) = ER-h du verificateur
v10 (ver_var mode 'h', lambda infini, mcs 1 ; arbre vfull). Comparaison par ultrametriques exactes (independantes
de la numerotation des noeuds).

Usage : python3 -B recoupe.py [nombre_de_nuages_aleatoires] [graine] > recus/recoupe.json
"""
from fractions import Fraction
import json
import random
import sys
import time

sys.dont_write_bytecode = True
import arbre  # noqa: E402
import fixtures as FX  # noqa: E402
import regles as RG  # noqa: E402
from surd import Surd  # noqa: E402

sys.path.insert(0, arbre.V10)
import points_reference as pr  # noqa: E402
import ver_var  # noqa: E402
from ver import R as VR  # noqa: E402


def to_surd(r):
    """Rayon vrad.R -> Surd (meme representation sum c sqrt(q))."""
    return Surd(dict(r.t))


def u_level_from_pr(entries, tree_pr):
    return pr.reference_ultrametric(entries, tree_pr)


def u_surd_equal(u1, u2):
    n = len(u1)
    return all(u1[i][j].cmp(u2[i][j]) == 0 for i in range(n) for j in range(n))


def one_cloud(points, k, stats, eta=1, kappa=12):
    n = len(points)
    d, res, t11 = arbre.v11_tree(points, k)
    T10, _info, t10 = arbre.v10_tree(points, k)
    stats['clouds'] += 1
    if arbre.signature(t11) != arbre.signature(t10):
        stats['tree_mismatch'].append((points, k))
        return
    # H_m contre l'oracle du developpeur
    for m in sorted(set([1, k + 1])):
        if m > n:
            continue
        ref, tree_pr = pr.reference_rules(res, n, m)
        rule = 'margin' if m > 1 else 'margin1'
        u_ref = pr.reference_ultrametric(ref[rule], tree_pr)
        mine = RG.rule_hm(t11, m)
        u_mine = RG.ultrametric(t11, mine)
        ok = all(u_mine[i][j].cmp(Surd.sqrt(u_ref[i][j])) == 0 for i in range(n) for j in range(n))
        stats['hm_comparisons'] += 1
        if not ok:
            stats['hm_mismatch'].append((points, k, m))
    # ER0h contre ver_var (v10)
    if k >= 2:
        mine = RG.rule_er0h(t11, eta, kappa)
        u_mine = RG.ultrametric(t11, mine)
        h, _res = ver_var.regle_var(T10, Fraction(10 ** 12), Fraction(eta), Fraction(kappa), 1, 'h')
        hang10 = [(to_surd(h.dates[x]), h.owners[x]) for x in range(n)]
        u10 = RG.ultrametric(t10, hang10)
        stats['er0h_comparisons'] += 1
        if not u_surd_equal(u_mine, u10):
            stats['er0h_mismatch'].append((points, k))


def random_cloud(rng):
    n = rng.randint(4, 8)
    side = rng.choice([3, 4, 6, 10, 40])
    scale = rng.choice([1, 7, 1000])
    pts = set()
    while len(pts) < n:
        pts.add((rng.randrange(side) * scale, rng.randrange(side) * scale, rng.choice([0, rng.randrange(side) * scale])))
    pts = sorted(pts)
    rng.shuffle(pts)
    return pts


def main():
    count = int(sys.argv[1]) if len(sys.argv) > 1 else 60
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 20261003
    stats = dict(clouds=0, hm_comparisons=0, er0h_comparisons=0, tree_mismatch=[], hm_mismatch=[], er0h_mismatch=[])
    t0 = time.time()
    fixtures = [(FX.EQUILATERAL, 2), (FX.FIVE, 2), ([(0, 0, 0), (2000, 0, 0), (4000, 0, 0)], 2),
                ([(0, 0, 0), (2000, 0, 0), (4001, 0, 0)], 2), (FX.Q1['points'], 2), (FX.Q2['points'], 2),
                (FX.Q4['points'], 3)]
    for pts, k in fixtures:
        one_cloud(pts, k, stats)
    rng = random.Random(seed)
    for _ in range(count):
        pts = random_cloud(rng)
        for k in range(2, min(4, len(pts) - 1) + 1):
            one_cloud(pts, k, stats)
    stats['seconds'] = round(time.time() - t0, 1)
    stats['seed'] = seed
    stats['random_clouds'] = count
    print(json.dumps(stats, indent=1, default=str))
    return 0 if not (stats['tree_mismatch'] or stats['hm_mismatch'] or stats['er0h_mismatch']) else 1


if __name__ == '__main__':
    raise SystemExit(main())
