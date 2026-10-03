#!/usr/bin/env python3
"""E8b : diagnostic des proprietaires differents entre hk.py (rayon) et reference_radius_rules du developpeur.
Hypothese : egalites exactes e = naissance d'un ancetre (ex aequo), tranchees differemment par l'arithmetique
mixte de la reference (sqrt a 120 chiffres, + et - dans le contexte decimal global).

    python3 -B e8b_diag.py GRAINE N
"""
import decimal
import json
import random
import sys

import hk
from e1_cibles import FIXTURES
from e2_e5 import random_cloud


def main():
    seed, count = int(sys.argv[1]), int(sys.argv[2])
    rng = random.Random(seed)
    clouds = [spec['points'] for spec in FIXTURES.values() if len(spec['points']) <= 9]
    clouds += [random_cloud(rng, rng.randint(4, 8)) for _ in range(count)]
    out = dict(owner_diff=0, diff_at_exact_birth=0, other=0, global_prec=decimal.getcontext().prec, example=None)
    for pts in clouds:
        n = len(pts)
        d = hk.Definition(pts)
        for k in range(1, min(4, n - 1) + 1):
            res = d.order(k)
            for m in sorted(set([1, k + 1])):
                ref, tree = hk.pr.reference_radius_rules(res, n, m)
                mine, mt = hk.rule_H(res, n, 1, m, 'rad')
                key = 'margin_r1' if m == 1 else 'margin_r'
                for i in range(n):
                    (e1, o1), (e2, o2) = ref[key][i], mine[i]
                    if o1 == o2:
                        continue
                    out['owner_diff'] += 1
                    # mon proprietaire est-il ne exactement a la date e (egalite de racines) ?
                    exact = hk.dsqrt(res.nodes[o2].level)
                    if abs(exact - e2) < hk.TIE:
                        out['diff_at_exact_birth'] += 1
                        if out['example'] is None:
                            out['example'] = dict(points=pts, k=k, m=m, site=i, e_ref=str(e1), e_mine=str(e2),
                                                  birth_owner_mine=str(exact), owner_ref=o1, owner_mine=o2)
                    else:
                        out['other'] += 1
    print(json.dumps(out, indent=1, default=str))


if __name__ == '__main__':
    main()
