#!/usr/bin/env python3
"""Exact scalar proposal only: dyadic center comparisons, no product port."""
from fractions import Fraction
import json
from itertools import product
checks = 0

def need(ok, message):
    global checks
    checks += 1
    if not ok:
        raise RuntimeError(message)

def compare_to_boundary(a, n, d, boundary, q):
    whole, remainder = divmod(boundary, q)
    e = n + d * (a - whole)
    if e < 0:
        return -1, e, None
    if e >= d:
        return 1, e, None
    left, right = q * e, d * remainder
    return (left > right) - (left < right), e, max(left, right)

profiles = []
for bits, t in product((18, 21, 24), (0, 1, 5, 6)):
    m, q = 1 << bits, 1 << t
    n_bits, d_bits = 5 * bits + 5, 4 * bits + 5
    ds = (1, 3, (1 << d_bits) - 1)
    ns = (-(1 << n_bits) + 1, -1, 0, 1, (1 << n_bits) - 1)
    boundaries = sorted({0, 1, q - 1, q, q + 1, q * (m - 1), q * m - 1, q * m})
    for a, n, d, boundary in product((0, m - 1), ns, ds, boundaries):
        got, e, multiplied = compare_to_boundary(a, n, d, boundary, q)
        delta = Fraction(a) + Fraction(n, d) - Fraction(boundary, q)
        need(got == (delta > 0) - (delta < 0), "Fraction comparison")
        need(abs(e) < 1 << (5 * bits + 6), "unscaled center bound")
        if multiplied is not None:
            need(multiplied < 1 << (d_bits + t), "small residual product bound")
    # Force equality and +/- one residual unit, including upper/lower contacts.
    d = q * 3
    for remainder in range(q):
        n = d * remainder // q
        for shift in (-1, 0, 1):
            got, e, multiplied = compare_to_boundary(0, n + shift, d, remainder, q)
            delta = Fraction(n + shift, d) - Fraction(remainder, q)
            need(got == (delta > 0) - (delta < 0), "contact and neighboring sign")
    profiles.append({"bits": bits, "T": t, "unscaled_center_bits": 5 * bits + 6,
                     "residual_product_bits": d_bits + t,
                     "scaled_reservoir_bits": 2 * (bits + t) + 5,
                     "unscaled_g1_bound_bits": 2 * bits + t + 5,
                     "unscaled_j2_affine_bits": 2 * bits + t + 4,
                     "unscaled_j2_sat_bits": 3 * bits + t + 5})
need(5 * 24 + 6 <= 127 and 4 * 24 + 5 + 6 <= 127, "u24 T6 center i128 sufficient with residual branch")
need(2 * 24 + 6 + 5 <= 63 and 2 * 24 + 6 + 4 <= 63 and 3 * 24 + 6 + 5 <= 127,
     "u24 T6 unscaled G1/J2 bounds")
need(2 * (24 + 6) + 5 > 63, "reservoir still needs a wider accumulator")
print(json.dumps({"status": "PASS", "checks": checks, "profiles": profiles,
                  "scope": "Fraction comparison of an abstract bounded-coefficient proposal; no catalogue, factory, subgrid implementation, native run or performance measurement",
                  "native_runs": 0, "fits": 0, "gcp_actions": 0}, sort_keys=True))
