#!/usr/bin/env python3
"""Oracle rationnel des centres q3 partagés et de leurs bornes u18.

Contrôle une arête et des paquets de graines aiguës propriétaires, avec
IDs implicites a=0, b=1, x>=2 pour les égalités de plus longue arête.
Ne lance ni le générateur v9, ni son census, ni une trame LiDAR.
"""
from __future__ import annotations

import itertools
import json
import random
from fractions import Fraction

M = (1 << 18) - 1
Point = tuple[int, int, int]


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise RuntimeError(reason)


def sub(a: Point, b: Point) -> Point:
    return tuple(a[i] - b[i] for i in range(3))


def dot(a: Point, b: Point) -> int:
    return sum(a[i] * b[i] for i in range(3))


def norm2(a: Point) -> int:
    return dot(a, a)


def h_for_x(a: Point, b: Point, x: Point) -> Point:
    d, u = sub(b, a), sub(x, a)
    D, E = norm2(d), dot(d, u)
    return tuple(D * u[i] - E * d[i] for i in range(3))


def seed(a: Point, b: Point, x: Point):
    d, u = sub(b, a), sub(x, a)
    D, U = norm2(d), norm2(u)
    E = dot(d, u)
    G = D * U - E * E
    if not D or not G or U <= E or E <= 0 or U > D or norm2(sub(x, b)) > D:
        return None
    H = h_for_x(a, b, x)
    xi = Fraction(D * (U - E), 2 * G)
    require(0 < xi <= Fraction(1, 3), "owned acute xi bound")
    require(3 * D * (U - E) <= 2 * G, "integer xi bound")
    center_num = tuple(G * (a[i] + b[i]) + (U - E) * H[i]
                       for i in range(3))
    require(max(abs(v) for v in center_num) < 54 * M ** 5,
            "reduced center numerator u18 bound")
    c = tuple(Fraction(center_num[i], 2 * G) for i in range(3))
    require(c == tuple(Fraction(a[i] + b[i], 2) + xi * Fraction(H[i], D)
                       for i in range(3)), "center cancellation")
    radius = sum((c[i] - a[i]) ** 2 for i in range(3))
    for p in (b, x):
        require(sum((c[i] - p[i]) ** 2 for i in range(3)) == radius,
                "center is not equidistant")
    return H, c


def center_box(a: Point, b: Point, hs: list[Point]):
    D = norm2(sub(b, a))
    den = 6 * D
    low = tuple(3 * D * (a[i] + b[i]) + 2 * min(0, *(h[i] for h in hs))
                for i in range(3))
    high = tuple(3 * D * (a[i] + b[i]) + 2 * max(0, *(h[i] for h in hs))
                 for i in range(3))
    require(max(abs(v) for v in (*low, *high)) < 30 * M ** 3,
            "center numerator u18 bound")
    return den, low, high


def power(a: Point, z: Point, c: tuple[Fraction, ...]) -> Fraction:
    return sum(z[i] * z[i] - a[i] * a[i] - 2 * c[i] * (z[i] - a[i])
               for i in range(3))


def power_box_bounds(a: Point, cbox, zs: list[Point]):
    den, low, high = cbox
    zlow = tuple(min(z[i] for z in zs) for i in range(3))
    zhigh = tuple(max(z[i] for z in zs) for i in range(3))
    bounds = []
    for cn in itertools.product(*[(low[i], high[i]) for i in range(3)]):
        lows, highs = [], []
        for i in range(3):
            def axis(z: int) -> int:
                return den * (z * z - a[i] * a[i]) - 2 * cn[i] * (z - a[i])

            floor = cn[i] // den
            candidates = {zlow[i], zhigh[i],
                          min(zhigh[i], max(zlow[i], floor)),
                          min(zhigh[i], max(zlow[i], floor + 1))}
            lows.append(min(axis(z) for z in candidates))
            highs.append(max(axis(zlow[i]), axis(zhigh[i])))
        bounds.append((sum(lows), sum(highs)))
    lower = min(v[0] for v in bounds)
    upper = max(v[1] for v in bounds)
    require(lower <= upper, "inverted power bounds")
    require(max(abs(lower), abs(upper)) < 234 * M ** 4,
            "power numerator u18 bound")
    return Fraction(lower, den), Fraction(upper, den)


def main() -> None:
    rng = random.Random(20260922)
    groups = seeds = checks = 0
    for case in range(240):
        def point() -> Point:
            if case % 3:
                return tuple(rng.randint(0, M) for _ in range(3))
            return tuple(rng.choice((0, 1, M - 1, M)) for _ in range(3))

        a, b = point(), point()
        found = []
        selected_x = []
        for _ in range(100):
            x = point()
            result = seed(a, b, x)
            if result is not None:
                found.append(result)
                selected_x.append(x)
        if not found:
            continue
        groups += 1
        seeds += len(found)
        xlow = tuple(min(x[i] for x in selected_x) for i in range(3))
        xhigh = tuple(max(x[i] for x in selected_x) for i in range(3))
        xcorners = itertools.product(*[(xlow[i], xhigh[i]) for i in range(3)])
        cbox = center_box(a, b, [h_for_x(a, b, x) for x in xcorners])
        den, low, high = cbox
        for _, c in found:
            for i in range(3):
                require(Fraction(low[i], den) <= c[i] <= Fraction(high[i], den),
                        "q3 center escaped shared box")
        witnesses = [point() for _ in range(4)]
        lo, hi = power_box_bounds(a, cbox, witnesses)
        for _, c in found:
            for z in witnesses:
                value = power(a, z, c)
                require(lo <= value <= hi, "power escaped shared box")
                if hi < 0:
                    require(value < 0, "false shared inside")
                if lo >= 0:
                    require(value >= 0, "contact lost by count exclusion")
                checks += 1
    a, b, x, z = (1, 2, 0), (5, 10, 0), (9, 2, 0), (5, 0, 0)
    result = seed(a, b, x)
    require(result is not None, "contact fixture has no acute owned seed")
    require(result[1] == (5, 5, 0) and power(a, z, result[1]) == 0,
            "contact fixture failed")
    a, b, x = (0, 0, 0), (M, M, 0), (M, 0, M)
    result = seed(a, b, x)
    require(result is not None and result[1] == (Fraction(2 * M, 3),
                                                  Fraction(M, 3),
                                                  Fraction(M, 3)),
            "extreme center fixture failed")
    D, u, d = norm2(sub(b, a)), sub(x, a), sub(b, a)
    E, U = dot(d, u), norm2(u)
    require(D * (U - E) * result[0][2] > (1 << 127) - 1,
            "literal i128 overflow fixture failed")
    a, b, z, x1, x2 = ((20, 20, 20), (40, 20, 20), (30, 31, 20),
                       (30, 32, 20), (30, 37, 20))
    c1, c2 = seed(a, b, x1), seed(a, b, x2)
    require(c1 is not None and c2 is not None, "cursor fixture seeds invalid")
    require(power(a, z, c1[1]) < 0 and power(a, z, c2[1]) < 0 and
            power(a, x1, c1[1]) == 0 and power(a, x1, c2[1]) < 0,
            "cursor fixture contact/interior mismatch")
    require(groups > 100 and seeds > 500, "insufficient seed coverage")
    print(json.dumps({"status": "PASS", "index_box_groups": groups,
                      "groups": groups, "seeds": seeds,
                      "power_checks": checks, "contact_fixtures": 1,
                      "overflow_fixtures": 1, "cursor_fixtures": 1},
                     sort_keys=True))


if __name__ == "__main__":
    main()
