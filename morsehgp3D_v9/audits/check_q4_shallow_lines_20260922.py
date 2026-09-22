#!/usr/bin/env python3
"""Oracle indépendant de la sélection d'événements peu profonds sur une droite.

Une forme affine L_z restreinte à une droite de graine est alpha*t+beta.
Le contrôle compare la sélection top/bottom O(Km) à TOUTES les intersections,
avec arithmétique rationnelle exacte. Il ne construit ni cover, ni atlas,
ni catalogue, ni tour HGP.
"""
from __future__ import annotations

import json
import random
from fractions import Fraction

M = (1 << 18) - 1

def require(ok: bool, reason: str) -> None:
    if not ok:
        raise RuntimeError(reason)


def candidates(forms: list[tuple[int, int]], budget: int) -> set[Fraction]:
    """Au plus 2(budget+1) seuils, sans trier toutes les intersections."""
    if budget < 0:
        return set()
    upper: list[Fraction] = []  # plus grandes tau des pentes positives
    lower: list[Fraction] = []  # plus petites tau des pentes négatives
    quota = budget + 1
    for alpha, beta in forms:
        if alpha == 0:
            continue
        tau = Fraction(-beta, alpha)
        target = upper if alpha > 0 else lower
        if tau in target:
            continue
        target.append(tau)
        if len(target) > quota:
            target.remove(min(target) if alpha > 0 else max(target))
    return set(upper) | set(lower)


def depth(forms: list[tuple[int, int]], t: Fraction) -> int:
    return sum(alpha * t + beta < 0 for alpha, beta in forms)


def brute_shallow(forms: list[tuple[int, int]], budget: int) -> set[Fraction]:
    if budget < 0:
        return set()
    all_events = {Fraction(-beta, alpha) for alpha, beta in forms if alpha != 0}
    return {t for t in all_events if depth(forms, t) <= budget}


def restricted(seed: tuple[int, int, int],
               other: tuple[int, int, int]) -> tuple[int, int, int]:
    """(alpha, beta, positive_denominator) for another u18-certified form."""
    c0, x0, y0 = seed
    c, x, y = other
    require(x0 != 0 or y0 != 0, "seed must define a line")
    if y0 != 0:  # t=u, v=-(c0+x0*t)/y0
        alpha, beta, den = x * y0 - y * x0, c * y0 - y * c0, y0
    else:  # t=v, u=-(c0+y0*t)/x0
        alpha, beta, den = y * x0 - x * y0, c * x0 - x * c0, x0
    if den < 0:
        alpha, beta, den = -alpha, -beta, -den
    require(abs(alpha) < 1 << 79 and abs(beta) < 1 << 80,
            "u18 restricted-form bound")
    return alpha, beta, den


def det3(seed: tuple[int, int, int], f: tuple[int, int, int],
         g: tuple[int, int, int]) -> int:
    c0, x0, y0 = seed
    c, x, y = f
    cc, xx, yy = g
    value = c0 * (x * yy - y * xx) - x0 * (c * yy - y * cc) + y0 * (c * xx - x * cc)
    require(abs(value) < 1 << 121, "factorized determinant exceeds i128 u18 bound")
    return value


def sign(value: int) -> int:
    return (value > 0) - (value < 0)


def check_u18_restriction_bounds() -> int:
    rng = random.Random(180922)
    samples = 0
    for i in range(500):
        def form() -> tuple[int, int, int]:
            return (rng.randint(-15 * M * M, 15 * M * M),
                    rng.randint(-8 * M * M, 8 * M * M),
                    rng.randint(-8 * M * M, 8 * M * M))
        seed = form()
        if i % 5 == 0:
            seed = (seed[0], seed[1] or 1, 0)  # alternate pivot
        if i % 5 == 1:
            seed = (seed[0], seed[1], -(abs(seed[2]) or 1))
        f, g = form(), form()
        a, b, den = restricted(seed, f)
        aa, bb, dden = restricted(seed, g)
        t = Fraction(rng.randint(-9, 9), rng.randint(1, 9))
        c0, x0, y0 = seed
        if y0 != 0:
            u, v = t, -(Fraction(c0) + x0 * t) / y0
        else:
            u, v = -(Fraction(c0) + y0 * t) / x0, t
        require(Fraction(a * t + b, den) == f[0] + f[1] * u + f[2] * v,
                "restricted affine form differs from direct evaluation")
        require(Fraction(aa * t + bb, dden) == g[0] + g[1] * u + g[2] * v,
                "second restricted affine form differs")
        if a and aa:
            n1, d1, n2, d2 = -b, a, -bb, aa
            if d1 < 0:
                n1, d1 = -n1, -d1
            if d2 < 0:
                n2, d2 = -n2, -d2
            determinant = n1 * d2 - n2 * d1
            require(abs(determinant) < 1 << 160,
                    "u18 rational-root comparator bound")
            require((determinant > 0) - (determinant < 0) ==
                    (Fraction(n1, d1) > Fraction(n2, d2)) -
                    (Fraction(n1, d1) < Fraction(n2, d2)),
                    "rational-root comparator sign")
            # The 160-bit cross-product difference factors by the seed
            # pivot. Its sign needs only a 3x3 determinant <2^121.
            d3 = det3(seed, f, g)
            raw_cross = b * aa - bb * a
            pivot = y0 if y0 != 0 else -x0
            require(raw_cross == pivot * d3,
                    "cross-product / 3x3 determinant identity")
            factorized_sign = -sign(pivot) * sign(d3) * sign(a) * sign(aa)
            require(factorized_sign == sign(determinant),
                    "i128 factorized comparator differs from Fraction")
        samples += 1
    return samples


def dot(a: tuple[int, int, int], b: tuple[int, int, int]) -> int:
    return sum(x * y for x, y in zip(a, b, strict=True))


def sub(a: tuple[int, int, int], b: tuple[int, int, int]) -> tuple[int, int, int]:
    return tuple(x - y for x, y in zip(a, b, strict=True))


def cross(a: tuple[int, int, int], b: tuple[int, int, int]) -> tuple[int, int, int]:
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def norm2(a: tuple[int, int, int]) -> int:
    return dot(a, a)


def owner_false_vertex() -> None:
    a, b = (8, 5, 1), (9, 5, 8)
    x, y = (8, 1, 5), (9, 2, 5)
    center = (5, 5, 5)
    points = (a, b, x, y)
    require(all(norm2(sub(p, center)) == 25 for p in points),
            "not one common radius-five sphere")
    pairs = [(norm2(sub(points[i], points[j])), i, j)
             for i in range(4) for j in range(i + 1, 4)]
    require(max(pairs)[1:] == (0, 1) and max(pairs)[0] == 50,
            "ab is not the strict longest edge")
    require(dot(sub(a, x), sub(b, x)) == 4,
            "x should be an acute owned seed")
    require(dot(sub(a, y), sub(b, y)) == -3,
            "y should be an obtuse completion")
    direction = cross(sub(b, a), sub(x, a))
    require(direction != (0, 0, 0) and dot(direction, sub(y, a)) != 0,
            "the two contact lines are not independent")
    forms = []
    for z in points:
        za = sub(z, a)
        # Power_z(center+t*direction) - Power_a(center+t*direction).
        alpha = -2 * dot(za, direction)
        beta = norm2(z) - norm2(a) - 2 * dot(za, center)
        forms.append((alpha, beta))
    require(depth(forms, Fraction(0)) == 0,
            "the contact vertex should be shallow")
    require(Fraction(0) in candidates(forms, 0),
            "selection lost the contact vertex")
    require(all(p[0] >= 8 > center[0] for p in points),
            "center should be outside convex hull of contacts")


def main() -> None:
    require(5760 * M**6 < 1 << 121,
            "u18 six-term determinant bound does not fit signed i128")
    rng = random.Random(20260922)
    cases = 0
    selected_total = 0
    brute_total = 0
    for _ in range(900):
        n = rng.randrange(2, 31)
        forms = [(rng.randrange(-4, 5), rng.randrange(-9, 10))
                 for _ in range(n)]
        # Exact equality, concurrent events, opposite signs, coincident forms.
        if n >= 4 and rng.randrange(2) == 0:
            forms[-1] = forms[0]
            forms[-2] = tuple(-x for x in forms[0])
        for budget in range(8):
            selected = candidates(forms, budget)
            shallow = brute_shallow(forms, budget)
            filtered = {t for t in selected if depth(forms, t) <= budget}
            require(len(selected) <= 2 * (budget + 1),
                    "candidate budget exceeded")
            require(filtered == shallow,
                    f"shallow event omitted: {forms}, budget={budget}")
            selected_total += len(selected)
            brute_total += len(shallow)
            cases += 1
    # Multiplicities at a single vertex and parallel/zero forms.
    adversarial = [(1, 0)] * 100 + [(-1, 0)] * 100
    adversarial += [(0, -1)] * 7 + [(0, 0)] * 9
    for budget in range(10):
        selected = candidates(adversarial, budget)
        require({t for t in selected if depth(adversarial, t) <= budget} ==
                brute_shallow(adversarial, budget),
                "multiplicity/constant adversarial case")
        cases += 1
    owner_false_vertex()
    numeric_samples = check_u18_restriction_bounds()
    print(json.dumps({"status": "PASS", "cases": cases,
                      "selected_events": selected_total,
                      "shallow_events": brute_total,
                      "owner_false_vertex": 1,
                      "u18_restriction_samples": numeric_samples,
                      "factorized_compare_bits": 121,
                      "max_candidates_formula": "2*(budget+1)"},
                     sort_keys=True))


if __name__ == "__main__":
    main()
