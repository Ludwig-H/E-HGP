#!/usr/bin/env python3
"""Contre-oracle B exact : sommet q4 peu profond, mais pas miniballe.

Le petit contrôle combinatoire teste aussi les deux bornes de sommets
sur des droites entières avec coïncidences, orientations opposées et
concurrences. Ce n'est pas un gate du cover ni du générateur v9.
"""

from __future__ import annotations

import json
import random
from fractions import Fraction
from itertools import combinations
from math import gcd


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise RuntimeError(reason)


def sub(a: tuple[int, ...], b: tuple[int, ...]) -> tuple[int, ...]:
    return tuple(x - y for x, y in zip(a, b, strict=True))


def dot(a: tuple[int, ...], b: tuple[int, ...]) -> int:
    return sum(x * y for x, y in zip(a, b, strict=True))


def dist2(a: tuple[int, ...], b: tuple[int, ...]) -> int:
    return dot(sub(a, b), sub(a, b))


def cross(a: tuple[int, int, int],
          b: tuple[int, int, int]) -> tuple[int, int, int]:
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def check_false_vertex() -> dict[str, int]:
    a, b = (25, 13, 18), (1, 13, 18)
    x, y = (17, 25, 16), (16, 25, 17)
    c = (13, 13, 13)
    points = (a, b, x, y)
    radius2, diameter2 = 169, 576
    require(all(dist2(p, c) == radius2 for p in points),
            "four points do not share the claimed sphere")
    distances = [dist2(points[i], points[j])
                 for i, j in combinations(range(4), 2)]
    require(distances == [576, 212, 226, 404, 370, 2],
            "ab is not the strict longest edge")
    require(dot(sub(a, x), sub(b, x)) == 20 and
            dot(sub(a, y), sub(b, y)) == 10,
            "both ab-completion angles must be strictly acute")
    for z in (x, y):
        require(dot(sub(b, a), sub(z, a)) > 0 and
                dot(sub(a, b), sub(z, b)) > 0,
                "the other two triangle angles must be acute")
    volume6 = dot(sub(b, a), cross(sub(x, a), sub(y, a)))
    require(volume6 == -288, "contact lines are not independent")
    midpoint = (13, 13, 18)
    require(8 * dist2(c, midpoint) == 200 < diameter2,
            "center is outside the universal q4 owner disc")
    require(8 * radius2 == 1352 < 3 * diameter2,
            "radius exceeds the universal q4 owner bound")
    require(all(p[2] >= 16 > c[2] for p in points),
            "center should be outside the contact convex hull")
    return {"radius2": radius2, "diameter2": diameter2,
            "center_midpoint_distance2": dist2(c, midpoint),
            "tetra_volume6": volume6}


Form = tuple[int, int, int]  # c + x*u + y*v
Center = tuple[Fraction, Fraction]


def line_key(form: Form) -> Form | None:
    c, x, y = form
    if x == 0 and y == 0:
        return None
    common = gcd(gcd(abs(c), abs(x)), abs(y))
    c, x, y = c // common, x // common, y // common
    if x < 0 or (x == 0 and y < 0):
        c, x, y = -c, -x, -y
    return c, x, y


def intersection(a: Form, b: Form) -> Center | None:
    c, x, y = a
    cc, xx, yy = b
    determinant = x * yy - y * xx
    if determinant == 0:
        return None
    return (Fraction(y * cc - c * yy, determinant),
            Fraction(c * xx - x * cc, determinant))


def value(form: Form, center: Center) -> Fraction:
    c, x, y = form
    return c + x * center[0] + y * center[1]


def restriction(seed: Form, form: Form) -> tuple[int, int]:
    c0, x0, y0 = seed
    c, x, y = form
    if y0 != 0:  # t=u
        alpha, beta, denominator = x * y0 - y * x0, c * y0 - y * c0, y0
    else:  # t=v
        alpha, beta, denominator = y * x0 - x * y0, c * x0 - x * c0, x0
    if denominator < 0:
        alpha, beta = -alpha, -beta
    return alpha, beta


def candidate_roots(forms: list[tuple[int, int]], budget: int) -> set[Fraction]:
    top: list[Fraction] = []
    bottom: list[Fraction] = []
    quota = budget + 1
    for alpha, beta in forms:
        if alpha == 0:
            continue
        root = Fraction(-beta, alpha)
        buffer = top if alpha > 0 else bottom
        if root in buffer:
            continue
        buffer.append(root)
        if len(buffer) > quota:
            buffer.remove(min(buffer) if alpha > 0 else max(buffer))
    return set(top) | set(bottom)


def depth_1d(forms: list[tuple[int, int]], t: Fraction) -> int:
    return sum(alpha * t + beta < 0 for alpha, beta in forms)


def check_arrangements() -> dict[str, int]:
    rng = random.Random(220926)
    cases: list[list[Form]] = [
        [(0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1),
         (0, -2, 0), (-1, 0, 0), (0, 0, 0)],
        [(1, 1, 0), (-1, -1, 0), (2, 1, 0), (0, 1, 0),
         (0, 0, 1), (0, 0, -1)],
        [(0, 1, 0), (0, 0, 1), (0, 1, 1), (0, -1, -1)],
    ]
    for _ in range(400):
        forms: list[Form] = []
        for _ in range(rng.randrange(2, 10)):
            form = (rng.randrange(-4, 5), rng.randrange(-3, 4),
                    rng.randrange(-3, 4))
            forms.append(form)
            if rng.randrange(3) == 0:
                forms.append(tuple(-v for v in form))
        cases.append(forms)

    checked, seed_lines, shallow_vertices = 0, 0, 0
    for forms in cases:
        lines = sorted({key for form in forms
                        if (key := line_key(form)) is not None})
        if len(lines) < 2:
            continue
        seeds = set(rng.sample(lines, rng.randrange(1, len(lines) + 1)))
        vertices = {point for a, b in combinations(lines, 2)
                    if (point := intersection(a, b)) is not None}
        for budget in range(4):
            shallow = {p for p in vertices
                       if sum(value(f, p) < 0 for f in forms) <= budget}
            require(len(shallow) <= len(lines) * (budget + 1),
                    "m(d+1) shallow-center bound failed")
            selected = {p for p in shallow
                        if any(value(seed, p) == 0 for seed in seeds)}
            require(len(selected) <= len(seeds) * (2 * budget + 2),
                    "s(2d+2) selected-center bound failed")
            for seed in seeds:
                on_line = [restriction(seed, f) for f in forms]
                roots = {Fraction(-beta, alpha) for alpha, beta in on_line
                         if alpha != 0}
                brute = {t for t in roots
                         if depth_1d(on_line, t) <= budget}
                candidates = candidate_roots(on_line, budget)
                filtered = {t for t in candidates
                            if depth_1d(on_line, t) <= budget}
                require(filtered == brute,
                        "top/bottom selector omitted a shallow crossing")
                projected = {p[0] if seed[2] else p[1] for p in shallow
                             if value(seed, p) == 0}
                require(projected == brute,
                        "line restriction disagrees with 2D arrangement")
                seed_lines += 1
            shallow_vertices += len(shallow)
            checked += 1
    return {"arrangement_budget_checks": checked,
            "seed_line_checks": seed_lines,
            "shallow_vertex_sum": shallow_vertices}


def main() -> None:
    receipt = {"status": "PASS", "scope": "local exact audit, not v9 gate"}
    receipt.update(check_false_vertex())
    receipt.update(check_arrangements())
    print(json.dumps(receipt, sort_keys=True))


if __name__ == "__main__":
    main()
