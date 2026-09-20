#!/usr/bin/env python3
"""Independent exact mathematical gate; no v8 product import/qualification.

API: forms(a,b,x,z)->(P,B), bounds(a,b,x)->(Uold,Unew).
D means squared owner-edge length. Face abc must be strictly acute and ab
maximal. The refined continuous bound is mu^2 <= D*S^2/T, S=2G-EX,
T=4G-EX. The implementable outward bound uses q=ceil(D*S/T), Unew =
min(Uold,ceil_sqrt(q*S)); it never forms the potentially wide D*S*S.
"""

from fractions import Fraction
from itertools import permutations, product
from math import isqrt
import json
import random

M = 65535
I128_MAX = (1 << 127) - 1


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def subtract(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def square_distance(a, b):
    d = subtract(a, b)
    return dot(d, d)


def ceil_sqrt(n):
    if n < 0:
        raise ValueError("negative square-root argument")
    r = isqrt(n)
    return r + (r * r < n)


def ceil_div(n, d):
    if n < 0 or d <= 0:
        raise ValueError("ceil_div domain")
    q, r = divmod(n, d)
    return q + (r != 0)


def face_data(a, b, x):
    d, u = subtract(b, a), subtract(x, a)
    D, E, X, F = dot(d, d), dot(u, u), square_distance(b, x), dot(d, u)
    G = D * E - F * F
    if not (G > 0 and 0 < F < min(D, E) and E <= D and X <= D):
        raise ValueError("requires a strictly acute face with maximal edge ab")
    C = E * X
    J, S, T = D * (3 * G - 2 * C), 2 * G - C, 4 * G - C
    require(J > 0 and S > 0 and T > 0, "positive face lost positive chord coefficients")
    return D, E, F, X, G, J, S, T


def forms(a, b, x, z):
    d, u, v = subtract(b, a), subtract(x, a), subtract(z, a)
    D, E, F = dot(d, d), dot(u, u), dot(d, u)
    G = D * E - F * F
    if G <= 0:
        raise ValueError("flat family seed")
    W = tuple(E * (D - F) * d[i] + D * (E - F) * u[i] for i in range(3))
    return G * dot(v, v) - dot(W, v), dot(cross(d, u), v)


def bounds(a, b, x):
    D, _, _, _, _, J, S, T = face_data(a, b, x)
    old = ceil_sqrt(ceil_div(J, 2))
    q = ceil_div(D * S, T)
    return old, min(old, ceil_sqrt(q * S))


def solve(matrix, rhs):
    """Rational Gaussian elimination, independent of closed-form P/B."""
    n = len(rhs)
    rows = [[Fraction(value) for value in row] + [Fraction(rhs[i])]
            for i, row in enumerate(matrix)]
    for col in range(n):
        pivot = next((r for r in range(col, n) if rows[r][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        den = rows[col][col]
        rows[col] = [value / den for value in rows[col]]
        for r in range(n):
            if r != col:
                multiple = rows[r][col]
                rows[r] = [value - multiple * entry for value, entry in zip(rows[r], rows[col])]
    return tuple(row[-1] for row in rows)


def gram_ball(points):
    """Return centre, radius squared and barycentric weights; None if flat."""
    base = points[0]
    edges = [subtract(p, base) for p in points[1:]]
    matrix = [[dot(v, w) for w in edges] for v in edges]
    coefficients = solve(matrix, [Fraction(dot(v, v), 2) for v in edges])
    if coefficients is None:
        return None
    shift = tuple(sum(coefficients[i] * edges[i][d] for i in range(len(edges))) for d in range(3))
    center = tuple(base[d] + shift[d] for d in range(3))
    return center, dot(shift, shift), (1 - sum(coefficients),) + coefficients


def verify_face(a, b, x, counters):
    D, E, F, X, G, J, S, T = face_data(a, b, x)
    center, radius, weights = gram_ball((a, b, x))
    require(all(weight > 0 for weight in weights), "acute face is not positive in Gram oracle")
    require(radius == Fraction(D * E * X, 4 * G), "face radius identity")
    require(Fraction(D, 4) <= radius <= Fraction(D, 3), "owner face radius range")
    exact = Fraction(D * S * S, T)  # Bigint oracle ONLY; not the i128 path.
    require(exact <= Fraction(J, 2), "refined chord exceeds the old chord")
    require(exact == 4 * G * (Fraction(D * D, 1) / (4 * (D - radius)) - radius),
            "refined chord/radius derivation")
    old, new = bounds(a, b, x)
    q = ceil_div(D * S, T)
    require(Fraction(new * new) >= exact and new <= old, "outward implementable chord failed")
    require(Fraction(old * old) >= Fraction(J, 2), "old chord rounding inward")
    require(3 * J >= D * G, "positive J lower bound")
    # Values actually formed in the proposed i128 implementation, not D*S*S.
    intermediates = (D, E, F, X, D * E, F * F, G, E * X, 3 * G, 2 * E * X,
                     J, 2 * G, 4 * G, S, T, D * S, q, q * S,
                     ceil_div(J, 2), old * old, new * new)
    require(all(0 <= value <= I128_MAX for value in intermediates), "i128 chord path exceeded")
    require(J <= 81 * M**6 and D * S <= 54 * M**6 and q * S <= 54 * M**6,
            "declared u16 bounds failed")
    counters["faces"] += 1
    counters["strictly_smaller_integer_bounds"] += new < old
    counters["max_i128_intermediate_bits"] = max(counters["max_i128_intermediate_bits"],
                                                   max(value.bit_length() for value in intermediates))
    # The original forms are checked against an independently solved centre.
    for z in (a, b, x, (0, 0, 0), (M, M, M), (M, 0, M), (0, M, 0)):
        P, B = forms(a, b, x, z)
        relative = subtract(z, center)
        require(P == G * (dot(relative, relative) - radius), "P differs from rational Gram power")
        require(abs(P) <= 135 * M**6 and abs(B) <= 6 * M**3,
                "u16 P/B bound failed")
        require(abs(P) + old * abs(B) <= 177 * M**6 <= I128_MAX, "i128 witness test exceeded")
        counters["gram_power_checks"] += 1


def verify_tetra(points, counters):
    ball = gram_ball(points)
    if ball is None or any(weight <= 0 for weight in ball[2]):
        return
    center, radius, _ = ball
    # IDs break maximal-length ties independently of permutation ranks.
    i, j = min(((i, j) for i in range(4) for j in range(i + 1, 4)),
               key=lambda pair: (-square_distance(points[pair[0]], points[pair[1]]), pair))
    a, b = points[i], points[j]
    other = [r for r in range(4) if r not in (i, j)]
    seen = 0
    for third, fourth in (other, other[::-1]):
        x, y = points[third], points[fourth]
        try:
            D, _, _, _, G, J, S, T = face_data(a, b, x)
        except ValueError:
            continue
        seen += 1
        P, B = forms(a, b, x, y)
        require(B != 0 and P > 0, "positive tetra lost the strict face-power condition")
        mu = Fraction(P, B)
        c0, r0, _ = gram_ball((a, b, x))
        normal = cross(subtract(b, a), subtract(x, a))
        require(center == tuple(c0[d] + mu * normal[d] / (2 * G) for d in range(3)),
                "family parameter does not reconstruct the Gram centre")
        require(radius == r0 + mu * mu / (4 * G), "family radius identity")
        require(radius <= Fraction(D * D, 1) / (4 * (D - r0)), "refined positive-tetra radius failed")
        require(mu * mu <= Fraction(D * S * S, T) <= Fraction(J, 2), "positive tetra escaped a chord")
        old, new = bounds(a, b, x)
        require(abs(mu) <= new <= old, "positive tetra escaped the rounded chord")
        for z in points:
            power, side = forms(a, b, x, z)
            require(power - mu * side == 0, "support not on reconstructed sphere")
        counters["positive_owned_seed_presentations"] += 1
    require(seen > 0, "positive tetra has no acute face on its maximal edge")
    counters["positive_owned_tetrahedra"] += 1


def must_refuse(a, b, x):
    try:
        bounds(a, b, x)
    except ValueError:
        return
    raise RuntimeError("invalid face accepted by the mathematical gate")


def main():
    counters = dict(faces=0, strictly_smaller_integer_bounds=0, max_i128_intermediate_bits=0,
                    gram_power_checks=0, positive_owned_tetrahedra=0, positive_owned_seed_presentations=0)
    base_faces = [((0, 0, 0), (4, 0, 0), (2, 3, 0)),
                  ((0, 0, 0), (3, 0, 0), (1, 2, 0)),
                  ((0, 0, 0), (0, 1, 1), (1, 0, 1)),
                  ((0, 0, 0), (12, 0, 0), (5, 6, 0)),
                  ((0, 0, 0), (M, M, 0), (M, 0, M))]
    for face in base_faces:
        largest = max(max(p) for p in face)
        for scale in sorted({1, max(1, M // largest)}):
            for perm in permutations(range(3)):
                transformed = tuple(tuple(scale * p[d] for d in perm) for p in face)
                for a, b, x in (transformed, (transformed[1], transformed[0], transformed[2])):
                    verify_face(a, b, x, counters)

    bad_faces = [((0, 0, 0), (0, 0, 0), (1, 1, 0)),
                 ((0, 0, 0), (4, 0, 0), (2, 0, 0)),
                 ((0, 0, 0), (4, 0, 0), (2, 2, 0)),  # Right at x.
                 ((0, 0, 0), (4, 0, 0), (2, 1, 0)),  # Obtuse at x.
                 ((0, 0, 0), (2, 3, 0), (4, 0, 0))]  # ab is not maximal.
    for face in bad_faces:
        must_refuse(*face)

    # Dense, small and deterministic: actual fourth vertices, not arbitrary mu.
    for face in base_faces[:4]:
        for y in product(range(8), repeat=3):
            verify_tetra(face + (y,), counters)
    rng = random.Random(20260920)
    for _ in range(1200):
        points = tuple(tuple(rng.randrange(17) for _ in range(3)) for _ in range(4))
        verify_tetra(points, counters)
    regular = ((0, 0, 0), (0, 1, 1), (1, 0, 1), (1, 1, 0))
    for scale in (1, 17, M):
        for ordering in permutations(regular):
            verify_tetra(tuple(tuple(scale * v for v in p) for p in ordering), counters)

    # Three unsafe roundings each violate the exact real chord they purport to cover.
    a, b, x = base_faces[1]
    D, _, _, _, _, J, S, T = face_data(a, b, x)
    require(isqrt(J // 2) ** 2 < Fraction(J, 2), "floor old square-root mutant survived")
    require(isqrt(ceil_div(D * S, T) * S) ** 2 < Fraction(D * S * S, T),
            "floor refined square-root mutant survived")
    a, b, x, y = regular
    D, _, _, _, _, _, S, T = face_data(a, b, x)
    wrong = ceil_sqrt((D * S // T) * S)
    P, B = forms(a, b, x, y)
    require(abs(Fraction(P, B)) > wrong, "floor quotient mutant did not lose the regular tetra")

    # Two witnesses that change role across a real family, neither universal.
    a, b, x = ((0, 0, 2), (4, 0, 2), (2, 3, 2))
    witnesses = ((2, 1, 0), (2, 1, 4))
    values = [forms(a, b, x, z) for z in witnesses]
    require(values == [(-96, -24), (-96, 24)], "collective fixture forms changed")
    old, new = bounds(a, b, x)
    require((old, new) == (28, 25), "collective fixture bounds changed")
    require(all(P + old * abs(B) >= 0 for P, B in values), "fixture accidentally has a universal witness")
    test_mu = (Fraction(-old), Fraction(-4), Fraction(0), Fraction(4), Fraction(old))
    require(min(sum(P - mu * B < 0 for P, B in values) for mu in test_mu) == 1,
            "collective witness fixture lost its strict depth")
    verify_tetra((a, b, x, (2, 1, 5)), counters)
    require(counters["strictly_smaller_integer_bounds"] > 0 and counters["positive_owned_tetrahedra"] > 0,
            "vacant refinement/tetra gate")
    print(json.dumps({"schema": "mhgp8_audit_q34_chord_math_v1", "status": "PASS",
                      "scope": "independent integer/rational mathematical gate; no product qualification",
                      **counters, "invalid_faces_refused": len(bad_faces), "rounding_mutants_killed": 3,
                      "collective_fixture": {"forms": values, "Uold": old, "Unew": new,
                                             "individual_universal_witnesses": 0, "minimum_pool_depth": 1}},
                     sort_keys=True))


if __name__ == "__main__":
    main()
