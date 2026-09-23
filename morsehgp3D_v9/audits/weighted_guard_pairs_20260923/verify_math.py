#!/usr/bin/env python3
"""Exact integer/Fraction checks for weighted guard-pair certificates."""

import itertools
import json
from fractions import Fraction


def check(condition, message):
    if not condition:
        raise RuntimeError(message)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def guard(a, b, g):
    d = tuple(y - x for x, y in zip(a, b))
    d2 = dot(d, d)
    w = tuple(2 * z - x - y for x, y, z in zip(a, b, g))
    return d2 - dot(w, w), cross(d, w)


def margin(a, b, g, h, lam, mu, lane):
    hg, cg = guard(a, b, g)
    hh, ch = guard(a, b, h)
    H = lam * hg + mu * hh
    C = tuple(lam * x + mu * y for x, y in zip(cg, ch))
    multiplier, cross_multiplier = ((3, 4) if lane == 3 else (1, 2))
    F = multiplier * H * H - cross_multiplier * dot(C, C)
    return H, F


def optimum_fixed_edge(a, b, g, h, lane):
    """Maximize F over normalized weight t in [0,1] with H>=0.

    A strict positive maximum is equivalent to a rational t in (0,1)
    proving the lane. H=0 cannot cause a false positive because F<=0.
    """
    hg, cg = guard(a, b, g)
    hh, ch = guard(a, b, h)
    dh = hg - hh
    dc = tuple(x - y for x, y in zip(cg, ch))
    k, m = ((3, 4) if lane == 3 else (1, 2))
    qa = k * dh * dh - m * dot(dc, dc)
    qb = 2 * (k * hh * dh - m * dot(ch, dc))
    qc = k * hh * hh - m * dot(ch, ch)
    candidates = [Fraction(0), Fraction(1)]
    if dh:
        candidates.append(Fraction(-hh, dh))
    candidates = [t for t in candidates if 0 <= t <= 1 and hh + dh * t >= 0]
    if not candidates:
        return None
    low, high = min(candidates), max(candidates)
    if qa < 0:
        vertex = Fraction(-qb, 2 * qa)
        if low < vertex < high:
            candidates.append(vertex)
    return max((qa * t * t + qb * t + qc, t) for t in candidates)


def corners(lo, hi):
    return tuple(itertools.product(*((x, y) for x, y in zip(lo, hi))))


def main():
    a, b = (3, 10, 10), (17, 10, 10)
    transverse = ((16, 10), (7, 15), (7, 5))
    shifts = ((0, 0, 0), (1, 2, -3), (2, 3, -5), (3, 1, -4))
    guards = [(10 + row[j], *transverse[j]) for row in shifts for j in range(3)]
    result = {"case": "B fixed-edge 12 guards, 66 unordered pairs"}
    for lane in (3, 4):
        values = []
        for i, j in itertools.combinations(range(12), 2):
            best = optimum_fixed_edge(a, b, guards[i], guards[j], lane)
            if best is not None:
                values.append((best[0], i, j, best[1]))
        check(len(values) == 65, "B H>=0 pair count")
        maximum, i, j, ratio = max(values)
        check(maximum < 0, "B weighted pair unexpectedly certified")
        result[f"q{lane}"] = {"H_nonnegative_pairs": len(values),
                               "strict_successes": sum(v[0] > 0 for v in values),
                               "largest_F": str(maximum),
                               "pair_indices": [i, j],
                               "best_t": str(ratio)}

    box_a = corners((3, 10, 10), (4, 10, 10))
    box_b = corners((17, 10, 10), (18, 10, 10))
    g, h = (10, 5, 13), (15, 14, 8)
    weighted = [margin(x, y, g, h, 2, 3, 4) for x, y in itertools.product(box_a, box_b)]
    equal = [margin(x, y, g, h, 1, 1, 4) for x, y in itertools.product(box_a, box_b)]
    check(len(weighted) == 64 and len(set(itertools.product(box_a, box_b))) == 4,
          "fixture corner count")
    check(all(H > 0 and F > 0 for H, F in weighted), "weighted box certificate")
    check(sum(H <= 0 or F <= 0 for H, F in equal) == 16, "equal box failure")
    check(all(margin(x, y, g, h, 2, 3, 3)[1] > 0
              for x, y in itertools.product(box_a, box_b)), "weighted q3")
    check(all(margin(x, y, g, g, 1, 0, 3)[1] < 0
              and margin(x, y, h, h, 1, 0, 3)[1] < 0
              for x, y in itertools.product(box_a, box_b)), "singleton failure")
    result["rectangle_fixture"] = {
        "a_box": [[3, 10, 10], [4, 10, 10]],
        "b_box": [[17, 10, 10], [18, 10, 10]],
        "guards": [list(g), list(h)], "ratio": "2:3",
        "corner_pairs": 64, "distinct_corner_pairs": 4,
        "weighted_min_H": min(H for H, _ in weighted),
        "weighted_min_F4": min(F for _, F in weighted),
        "equal_fail_F4_corners": sum(H <= 0 or F <= 0 for H, F in equal),
    }
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
