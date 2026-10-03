#!/usr/bin/env python3
"""Q8 : budgets et comparaison quatre racines, controles entiers bornes.

Ce sont des radicandes scalaires admis par les budgets ; aucun nuage, code
produit, natif ou fit n'est execute. Oracle de signe : intervalles isqrt
independants, plus identites symboliques gravees pour les egalites.
"""
from fractions import Fraction as Q
from hashlib import sha256
import json
from math import isqrt
from pathlib import Path
import random

CHECKS = 0


def require(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise ValueError(message)


def sign(x):
    return (x > 0) - (x < 0)


def comparison(values, A, D):
    L = A+3*D
    product = 1
    for v in values:
        require(0 <= v.numerator < 2**A and 1 <= v.denominator < 2**D, "profile input budget")
        product *= v.denominator
    require(product < 2**(4*D), "common denominator budget")
    n = [v.numerator*(product//v.denominator) for v in values]
    require(all(v < 2**L for v in n), "four unified integer radicands")
    x, y, U = n[0]*n[1], n[2]*n[3], n[2]+n[3]-n[0]-n[1]
    require(x < 2**(2*L) and y < 2**(2*L) and abs(U) < 2**(L+1), "products and signed sum budgets")
    d = sign(x-y)
    if d >= 0 and U <= 0:
        return 0 if d == 0 and U == 0 else 1
    if d <= 0 and U >= 0:
        return 0 if d == 0 and U == 0 else -1
    factor = 1
    if d < 0:
        x, y, U, factor = y, x, -U, -1
    require(x > y and U > 0, "normalised positive signs before squaring")
    W = 4*(x-y)-U*U
    require(abs(W) < 2**(2*L+2), "W magnitude budget")
    if W <= 0:
        return 0 if W == 0 and y == 0 else -factor
    left, right = W*W, 16*U*U*y
    require(left < 2**(4*L+4) and right < 2**(4*L+6), "final product budgets")
    return factor*sign(left-right)


def interval_sign(values):
    lo = hi = Q(0)
    for s, v in zip((1, 1, -1, -1), values):
        denominator = v.denominator << 512
        root = isqrt((v.numerator*v.denominator) << 1024)
        a, b = Q(root, denominator), Q(root+1, denominator)
        lo += s*(a if s > 0 else b)
        hi += s*(b if s > 0 else a)
    require(lo > 0 or hi < 0, "independent intervals certify a nonzero sign")
    return 1 if lo > 0 else -1


def run():
    rng = random.Random(202610038)
    rows = []
    counts = dict(random=0, equality=0, extrema=0)
    for B in (18, 21, 24):
        A, D = 8*B+12, 6*B+8
        L, R, T = A+3*D, (A+D)//2, (A+11*D)//2
        E = 6*D+63*(T+3)
        p = E+3
        require(L == 26*B+36 and R == 7*B+10 and T == 37*B+50, "symbolic budgets")
        require(E == 2367*B+3387 and 6 < 2**(p-E), "certified interval width below norm gap")
        rows.append(dict(profile="u%d" % B, A=A, D=D, unified_bits=L,
                         comparator_bits=4*L+6, square_ratio_bits=A+D,
                         coefficient_numerator_bits=6*R+3, coefficient_denominator_bits=6*R,
                         norm_gap_bits=E, sufficient_precision=p,
                         isqrt_input_bits=A+D+2*p, signed_sum_magnitude_bits=p+T+3))
        for raw in ((0, 0, 0, 0), (8, 18, 2, 32), (2, 32, 8, 18)):
            require(comparison(list(map(Q, raw)), A, D) == 0, "symbolically equal sums")
            counts["equality"] += 1
        top, bottom = Q(2**A-1, 2**D-1), Q(1, 2**D-1)
        for values in ([top, top, bottom, bottom], [bottom, bottom, top, top],
                       [top, bottom, bottom, Q(0)], [bottom, Q(0), top, bottom]):
            require(comparison(values, A, D) == interval_sign(values), "profile extrema exact sign")
            counts["extrema"] += 1
        for _ in range(240):
            values = [Q(rng.randrange(0, 2**A), rng.randrange(1, 2**D)) for _ in range(4)]
            result = comparison(values, A, D)
            require(result == interval_sign(values), "four-root sign versus certified independent intervals")
            require(result == -comparison(values[2:]+values[:2], A, D), "antisymmetry")
            counts["random"] += 1
    require([r["comparator_bits"] for r in rows] == [2022, 2334, 2646], "final comparator table")
    require([r["sufficient_precision"] for r in rows] == [45996, 53097, 60198], "norm separation table")
    require([r["isqrt_input_bits"] for r in rows] == [92264, 106508, 120752], "isqrt scratch table")
    require([r["signed_sum_magnitude_bits"] for r in rows] == [46715, 53927, 61139], "aggregate scratch table")
    return dict(schema="ehgp.v11.point_date_budgets.v1", status="pass", checks=CHECKS,
                profiles=rows, comparisons=counts, precision_of_oracle=512,
                scope="scalar profile budgets and comparator derivation; not native qualification or worst-case attainment",
                sha256=sha256(Path(__file__).read_bytes()).hexdigest(), native_runs=0, fits=0)


if __name__ == "__main__":
    print(json.dumps(run(), sort_keys=True, separators=(",", ":")))
