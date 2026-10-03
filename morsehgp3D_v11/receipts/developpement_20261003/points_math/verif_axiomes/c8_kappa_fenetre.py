#!/usr/bin/env python3
"""C8 : fenetre de kappa pour laquelle P_kappa o Pi_1 (rayon) passe Q2, Q3, Q4 en lecture stricte et Q-Pi2.

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B c8_kappa_fenetre.py
Q4 strict : a r = sqrt(750000), blocs {P,Q,R} et {P2,Q2,R2}, aucun bloc ne contient deux sites de {C, m, D} ;
puis bloc {C, m, D} avant la racine. Bornes obtenues par dichotomie sur kappa (Fraction exacte du parametre).
"""
import json
import sys
from fractions import Fraction

sys.dont_write_bytecode = True
import mpmath  # noqa: E402
import vfull as V  # noqa: E402
from c1_fixtures import FIX  # noqa: E402
from c3_seuil import block_of, window, radii  # noqa: E402

FULLS = {name: V.Full(V.Cloud(FIX[name][1]), FIX[name][0]) for name in ('Q2', 'Q3', 'Q4')}


def q2(kap):
    rule = V.rule_P(FULLS['Q2'], kap, 1)
    x, a, b1, b2 = range(4)
    return all(block_of(rule, r, x) == frozenset([x, a]) and block_of(rule, r, b1) == frozenset([b1, b2])
               for r in window(rule, 4, 61, 75))


def q3(kap):
    rule = V.rule_P(FULLS['Q3'], kap, 1)
    cs = set(range(6, 14))
    ok = True
    for r in window(rule, 14, 705, 790):
        b = block_of(rule, r, 0)
        ok = ok and b is not None and 1 in b and not (b & cs) and max(len(c) for c in rule.blocks(r)) < 9
    return ok


def q4(kap):
    rule = V.rule_P(FULLS['Q4'], kap, 1)
    C, P, Q, R_, m_, D, P2, Q2, R2 = range(9)
    r0, root = mpmath.sqrt(750000), mpmath.sqrt(1780000)
    tet = block_of(rule, r0, P) == frozenset([P, Q, R_]) and block_of(rule, r0, P2) == frozenset([P2, Q2, R2])
    sep = all(len(b & {C, m_, D}) <= 1 for b in rule.blocks(r0))
    chain = any(r < root - V.TOL and block_of(rule, r, C) == frozenset([C, m_, D]) for r in radii(rule, 9))
    return tet and sep and chain


def edge(test, lo, hi, want_lo):
    """Dichotomie : test(lo) == want_lo, test(hi) != want_lo."""
    for _ in range(40):
        mid = (lo + hi) / 2
        if test(mid) == want_lo:
            lo = mid
        else:
            hi = mid
    return float(lo), float(hi)


def main():
    grid = [1, 1.2, 1.39, 1.4, 1.5, 2, 3, 3.78, 3.79, 4, 8]
    table = {str(k): {'Q2': q2(k), 'Q3': q3(k), 'Q4': q4(k)} for k in grid}
    out = {'grille': table,
           'Q2_seuil_bas': edge(q2, 1.0, 2.0, False),
           'Q4_seuil_bas': edge(q4, 1.0, 2.0, False),
           'Q4_seuil_haut': edge(q4, 2.0, 8.0, True),
           'formules': {'Q2': '(75.25997 - 61)/(60.20797 - 50)', 'Q4_bas': '(1078.17349 - 866.02540)/(1009.95049 - 816.49658)',
                        'Q4_haut': '(1334.16641 - 866.02540)/(816.49658 - 692.82032)'}}
    print(json.dumps(out, indent=1))
    with open('recus_c8_kappa_fenetre.json', 'w') as fh:
        json.dump(out, fh, indent=1)


if __name__ == '__main__':
    main()
