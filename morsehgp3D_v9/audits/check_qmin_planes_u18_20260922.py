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


def check_arrangement_identity(points: list[tuple[int, int, int]],
                               center: tuple[Fraction | int, Fraction | int, Fraction | int]) -> None:
    require(len(points) >= 3, "arrangement identity needs three sites")
    scaled = tuple(-2 * c for c in center)
    require(all(x.denominator == 1 for x in map(Fraction, scaled)),
            "arrangement test needs an integral BallKey B")
    b = tuple(int(x) for x in scaled)
    w = tuple(2 * points[0][r] + b[r] for r in range(3))
    nij = normal(w, points[0], points[1])
    nik = normal(w, points[0], points[2])
    displacement = sub(points[2], points[0])
    d = sum(nij[r] * displacement[r] for r in range(3))
    require(abs(d) < 1 << 135, "arrangement comparator exceeds the published u18 bound")
    require(cross(nij, nik) == tuple(d * x for x in w),
            "factorized great-circle comparator identity")


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


def check_large_shell_fixture(center: tuple[int, int, int], radius: int,
                              equator: list[tuple[int, int]],
                              expected_pair_supports: int,
                              expected_triangle_supports: int,
                              critical_k: int, expected_strict: int) -> None:
    points = [(center[0] + x, center[1] + y, center[2]) for x, y in equator]
    points.append((center[0], center[1], center[2] + radius))
    require(all(sum((p[r] - center[r]) ** 2 for r in range(3)) == radius ** 2
                for p in points), "large-shell sites must lie on one sphere")
    require(len(set(points)) == len(points), "large-shell positions must be distinct")
    forbidden: list[int] = []
    pairs = 0
    triangles = 0
    for i, j in combinations(range(len(equator)), 2):
        if all(equator[i][r] + equator[j][r] == 0 for r in range(2)):
            forbidden.append((1 << i) | (1 << j))
            pairs += 1
    for i, j, k in combinations(range(len(equator)), 3):
        if brute_positive_triangle([points[i], points[j], points[k]], center):
            forbidden.append((1 << i) | (1 << j) | (1 << k))
            triangles += 1
    require((pairs, triangles) == (expected_pair_supports, expected_triangle_supports),
            "large-shell exact support counts differ")

    # All sites have z >= c_z, so every positive convex support has zero
    # coefficient on the north point. In the equatorial plane, Caratheodory
    # reduces every non-strict subset to one of the exact pairs or triangles.
    def strict_count(t: int) -> int:
        count = 0
        for members in combinations(range(len(points)), t):
            mask = sum(1 << i for i in members)
            if not any(mask & support == support for support in forbidden):
                count += 1
        return count

    require(strict_count(critical_k) == expected_strict,
            "large-shell strict facet count differs")
    require(strict_count(critical_k + 1) == 0,
            "large-shell strict coface should be absent")
    if len(points) == 13:
        require([strict_count(t) for t in range(1, 11)] ==
                [13, 72, 180, 240, 180, 72, 12, 0, 0, 0],
                "13-site rank profile differs")


def check_q4_large_shell() -> None:
    center = (10, 10, 10)
    offsets = [(5, 5, 5), (5, -5, -5), (-5, 5, -5), (-5, -5, 5),
               (7, 5, -1), (7, -1, 5), (-1, 7, 5), (-7, 5, -1),
               (7, 5, 1), (5, -1, 7), (5, -7, 1), (7, -5, -1),
               (1, 5, 7)]
    points = [tuple(center[r] + p[r] for r in range(3)) for p in offsets]
    require(all(sum(x * x for x in p) == 75 for p in offsets),
            "q4 large-shell radius fixture")
    require(len(set(points)) == 13, "q4 large-shell distinct sites")
    for i, j in combinations(range(13), 2):
        require(any(offsets[i][r] + offsets[j][r] != 0 for r in range(3)),
                "q4 large-shell unexpectedly has an antipodal pair")
    for ids in combinations(range(13), 3):
        require(not brute_positive_triangle([points[i] for i in ids], center),
                "q4 large-shell unexpectedly has a positive triangle")
    supports = []
    for ids in combinations(range(13), 4):
        if positive_tetrahedron([points[i] for i in ids], center):
            supports.append(sum(1 << i for i in ids))
    require(len(supports) == 60, "q4 large-shell positive supports")

    # First twelve contacts stay inside the v7/v9 quotient ceiling. This
    # is a positive public-chain fixture, unlike the u13 refusal fixture.
    supports12 = [mask for mask in supports if not mask & (1 << 12)]
    require(len(supports12) == 51, "q4 u12 positive support count")
    def strict12(t: int) -> list[int]:
        result = []
        for members in combinations(range(12), t):
            mask = sum(1 << i for i in members)
            if not any(mask & support == support for support in supports12):
                result.append(mask)
        return result
    require(len(strict12(10)) == 3 and len(strict12(11)) == 0,
            "q4 u12 should have three isolated K10 facets")

    def strict_masks(t: int) -> list[int]:
        result = []
        for members in combinations(range(13), t):
            mask = sum(1 << i for i in members)
            if not any(mask & support == support for support in supports):
                result.append(mask)
        return result

    rank10, rank11 = strict_masks(10), strict_masks(11)
    require(len(rank10) == 32 and len(rank11) == 3,
            f"q4 large-shell rank K10/K11 differs: {len(rank10)}, {len(rank11)}")
    # The three 11-faces have explicit integer separating normals; they are
    # easy to miss when a non-generic arrangement is treated as simple.
    witnesses = {frozenset((3, 10)): (6, 8, -3),
                 frozenset((3, 7)): (8, 5, -4),
                 frozenset((2, 7)): (2, -1, 2)}
    for mask in rank11:
        omitted = frozenset(i for i in range(13) if not mask & (1 << i))
        require(omitted in witnesses, "unexpected strict 11-face")
        n = witnesses[omitted]
        require(all(sum(n[r] * offsets[i][r] for r in range(3)) > 0
                    for i in range(13) if mask & (1 << i)),
                "strict 11-face has no claimed integer witness")

    parent = {mask: mask for mask in rank10}
    def root(mask: int) -> int:
        while parent[mask] != mask:
            mask = parent[mask]
        return mask
    for coface in rank11:
        faces = [coface ^ (1 << i) for i in range(13) if coface & (1 << i)]
        require(all(face in parent for face in faces), "strict face missing")
        for face in faces[1:]:
            parent[root(face)] = root(faces[0])
    sizes: dict[int, int] = defaultdict(int)
    for mask in rank10:
        sizes[root(mask)] += 1
    require(sorted(sizes.values()) == [1, 31],
            f"q4 large-shell K10 components differ: {sorted(sizes.values())}")


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
    check_arrangement_identity(half_triangle, half_center)
    c4 = (1, 1, 1)
    tetra = [(2, 2, 2), (2, 0, 0), (0, 2, 0), (0, 0, 2)]
    require(positive_tetrahedron(tetra, c4), "q_min=4 tetra fixture")
    require(not brute_positive_triangle(tetra, c4), "q_min=4 triangle oracle")
    require(not has_positive_triangle_by_planes(tetra, c4), "q_min=4 plane fixture")

    equator13 = [(5, 0), (-5, 0), (0, 5), (0, -5)]
    equator13 += [(x, y) for x in (-3, 3) for y in (-4, 4)]
    equator13 += [(x, y) for x in (-4, 4) for y in (-3, 3)]
    check_large_shell_fixture((5, 5, 5), 5, equator13, 6, 40, 7, 12)
    equator17 = [(65, 0), (-65, 0), (0, 65), (0, -65)]
    for x, y in ((63, 16), (63, -16), (60, 25), (60, -25),
                 (56, 33), (56, -33)):
        equator17.extend(((x, y), (-x, -y)))
    check_large_shell_fixture((65, 65, 65), 65, equator17, 8, 112, 9, 16)
    check_q4_large_shell()

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
            check_arrangement_identity(sample, c)
            checked += 1
    print(json.dumps({"status": "PASS", "u18_M": M, "sphere_sites": len(shell),
                      "random_shells_without_antipodes": checked,
                      "arrangement_identities": checked + 1,
                      "large_shell_fixtures": ["mixed13", "mask17", "q4_13"],
                      "fixtures": 4, "q4_u12_k10_parents": 3},
                     sort_keys=True))


if __name__ == "__main__":
    main()
