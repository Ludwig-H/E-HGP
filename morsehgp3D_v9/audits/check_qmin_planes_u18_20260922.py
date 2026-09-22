#!/usr/bin/env python3
"""Independent small-shell oracle for the factorized u18 q_min=3 plane test.

This does not implement the full great-circle arrangement or certify a v9
engine. It checks the proposed plane quotient against exact barycentric
triangles, including coplanar and non-coplanar shell points.
"""

from __future__ import annotations

import json
import random
import sys
from collections import defaultdict
from fractions import Fraction
from functools import cmp_to_key
from itertools import combinations, permutations, product
from math import gcd


M = (1 << 18) - 1


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def sub(a: tuple[int, ...], b: tuple[int, ...]) -> tuple[int, ...]:
    return tuple(x - y for x, y in zip(a, b, strict=True))


def cross(a: tuple[int, ...], b: tuple[int, ...]) -> tuple[int, int, int]:
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def normal(w: tuple[int, ...], p: tuple[int, ...], q: tuple[int, ...]) -> tuple[int, int, int]:
    # Factorized: w_p x (q-p), not w_p x w_q with much wider products.
    n = cross(w, sub(q, p))
    require(all(abs(v) < 1 << 116 for v in n), "normal exceeds the published u18 bound")
    return n


def primitive_unoriented(n: tuple[int, int, int]) -> tuple[int, int, int]:
    g = gcd(gcd(abs(n[0]), abs(n[1])), abs(n[2]))
    require(g != 0, "antipodal or repeated sites must be handled before planes")
    out = tuple(v // g for v in n)
    if next(v for v in out if v != 0) < 0:
        out = tuple(-v for v in out)
    return out


def has_positive_triangle_by_planes(points: list[tuple[int, int, int]],
                                    center: tuple[Fraction | int, Fraction | int, Fraction | int],
                                    a: int = 1) -> bool:
    require(a > 0, "A must be positive")
    scaled = tuple(-2 * a * c for c in center)
    require(all(x.denominator == 1 for x in map(Fraction, scaled)),
            "the chosen A must make B integral")
    b = tuple(int(x) for x in scaled)
    w = [tuple(2 * a * p[r] + b[r] for r in range(3)) for p in points]
    groups: dict[tuple[int, int, int], set[int]] = defaultdict(set)
    for i, j in combinations(range(len(points)), 2):
        n = normal(w[i], points[i], points[j])
        if n == (0, 0, 0):
            continue  # Antipodes belong to q_min=2; no plane is needed.
        groups[primitive_unoriented(n)].update((i, j))

    for plane, members in groups.items():
        if len(members) < 3:
            continue
        axis = next(r for r in range(3) if plane[r] != 0)
        dims = tuple(r for r in range(3) if r != axis)
        orientation = -1 if axis == 1 else 1
        ids = list(members)

        def half(i: int) -> int:
            x, y = w[i][dims[0]], w[i][dims[1]]
            return 0 if y > 0 or (y == 0 and x >= 0) else 1

        def signed_det(i: int, j: int) -> int:
            # det(project(w_i),project(w_j)) = +/-2A*N_ij[axis].
            return orientation * normal(w[i], points[i], points[j])[axis]

        def compare(i: int, j: int) -> int:
            hi, hj = half(i), half(j)
            if hi != hj:
                return -1 if hi < hj else 1
            d = signed_det(i, j)
            require(d != 0, "duplicate or antipodal projected directions")
            return -1 if d > 0 else 1

        ids.sort(key=cmp_to_key(compare))
        if all(signed_det(ids[k], ids[(k + 1) % len(ids)]) > 0
               for k in range(len(ids))):
            return True  # Every circular gap is strictly less than pi.
    return False


def brute_positive_triangle(points: list[tuple[int, int, int]],
                            center: tuple[Fraction | int, Fraction | int, Fraction | int]) -> bool:
    v = [sub(p, center) for p in points]
    for p, q, r in combinations(v, 3):
        d = sub(p, r)
        e = sub(q, r)
        for i, j in combinations(range(3), 2):
            determinant = d[i] * e[j] - d[j] * e[i]
            if determinant == 0:
                continue
            alpha = Fraction(-r[i] * e[j] + r[j] * e[i], determinant)
            beta = Fraction(-d[i] * r[j] + d[j] * r[i], determinant)
            gamma = 1 - alpha - beta
            if (alpha > 0 and beta > 0 and gamma > 0 and
                    all(alpha * p[k] + beta * q[k] + gamma * r[k] == 0 for k in range(3))):
                return True
    return False


def positive_tetrahedron(points: list[tuple[int, int, int]],
                         center: tuple[int, int, int]) -> bool:
    require(len(points) == 4, "tetrahedron needs four points")
    matrix = [[Fraction(points[col][row]) for col in range(4)] for row in range(3)]
    matrix.append([Fraction(1) for _ in range(4)])
    rhs = [Fraction(c) for c in center] + [Fraction(1)]
    for col in range(4):
        pivot = next((row for row in range(col, 4) if matrix[row][col] != 0), None)
        if pivot is None:
            return False
        matrix[col], matrix[pivot] = matrix[pivot], matrix[col]
        rhs[col], rhs[pivot] = rhs[pivot], rhs[col]
        scale = matrix[col][col]
        matrix[col] = [x / scale for x in matrix[col]]
        rhs[col] /= scale
        for row in range(4):
            if row == col:
                continue
            scale = matrix[row][col]
            matrix[row] = [matrix[row][k] - scale * matrix[col][k] for k in range(4)]
            rhs[row] -= scale * rhs[col]
    return all(weight > 0 for weight in rhs)


def main() -> None:
    require(len(sys.argv) == 1, "usage: check_qmin_planes_u18_20260922.py")
    require(84 * M**5 < 1 << 97, "w bound")
    require(168 * M**6 < 1 << 116, "normal bound")
    require(504 * M**7 < 1 << 135, "direct coplanarity bound")
    require(3 * (1 << 76) * M**2 + 3 * (1 << 96) * M + (1 << 116) < 1 << 117,
            "BallKey power bound")

    c = (5, 5, 5)
    antipodal = [(10, 5, 5), (0, 5, 5)]
    require(sum(antipodal[0][i] + antipodal[1][i] == 2 * c[i] for i in range(3)) == 3,
            "q_min=2 fixture")
    triangle = [(10, 5, 5), (2, 9, 5), (2, 1, 5), (5, 5, 10)]
    require(brute_positive_triangle(triangle, c), "q_min=3 oracle fixture")
    require(has_positive_triangle_by_planes(triangle, c), "q_min=3 plane fixture")
    half_center = (Fraction(7, 2), Fraction(7, 2), Fraction(5))
    half_triangle = [(3, 5, 5), (2, 3, 5), (5, 3, 5)]
    require(brute_positive_triangle(half_triangle, half_center),
            "rational-center q_min=3 oracle fixture")
    require(has_positive_triangle_by_planes(half_triangle, half_center),
            "rational-center q_min=3 plane fixture")
    c4 = (1, 1, 1)
    tetra = [(2, 2, 2), (2, 0, 0), (0, 2, 0), (0, 0, 2)]
    require(positive_tetrahedron(tetra, c4), "q_min=4 tetra fixture")
    require(not brute_positive_triangle(tetra, c4), "q_min=4 triangle oracle")
    require(not has_positive_triangle_by_planes(tetra, c4), "q_min=4 plane fixture")

    sphere = set()
    for coordinates in permutations((3, 4, 0)):
        for signs in product((-1, 1), repeat=3):
            sphere.add(tuple(coordinates[i] * signs[i] for i in range(3)))
    for axis in range(3):
        for sign in (-5, 5):
            p = [0, 0, 0]
            p[axis] = sign
            sphere.add(tuple(p))
    shell = [tuple(c[i] + p[i] for i in range(3)) for p in sorted(sphere)]
    rng = random.Random(20260922)
    checked = 0
    for u in range(3, 10):
        for _ in range(200):
            sample = rng.sample(shell, u)
            directions = {sub(p, c) for p in sample}
            if any(tuple(-x for x in d) in directions for d in directions):
                continue
            expected = brute_positive_triangle(sample, c)
            actual = has_positive_triangle_by_planes(sample, c)
            require(actual == expected, f"plane quotient differs for {sample}")
            checked += 1
    print(json.dumps({"status": "PASS", "u18_M": M, "sphere_sites": len(shell),
                      "random_shells_without_antipodes": checked, "fixtures": 4},
                     sort_keys=True))


if __name__ == "__main__":
    main()
