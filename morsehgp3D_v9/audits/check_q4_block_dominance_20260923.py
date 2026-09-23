#!/usr/bin/env python3
"""Independent integer check of a centre-cell guard-block dominance lemma."""

from itertools import product
from random import Random


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def box_of(points):
    return tuple((min(p[i] for p in points), max(p[i] for p in points)) for i in range(3))


def corners(box):
    return product(*(sorted(set(interval)) for interval in box))


def squared(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b))


def farthest_squared(point, box):
    return sum(max((point[i] - lo) ** 2, (point[i] - hi) ** 2)
               for i, (lo, hi) in enumerate(box))


def nearest_squared(point, box):
    return sum((lo - point[i]) ** 2 if point[i] < lo else
               (point[i] - hi) ** 2 if point[i] > hi else 0
               for i, (lo, hi) in enumerate(box))


def paired_certificate(cell, guards_box, witnesses_box):
    return all(farthest_squared(c, guards_box) < nearest_squared(c, witnesses_box)
               for c in corners(cell))


def old_radius_certificate(cell, guards, witnesses_box):
    upper = max(squared(c, g) for c in corners(cell) for g in guards)
    lower = sum((witnesses_box[i][0] - cell[i][1]) ** 2
                if witnesses_box[i][0] > cell[i][1] else
                (cell[i][0] - witnesses_box[i][1]) ** 2
                if cell[i][0] > witnesses_box[i][1] else 0
                for i in range(3))
    return lower > upper


def check_exact(cell, guards, witnesses):
    return all(squared(c, g) < squared(c, z)
               for c in product(*(range(lo, hi + 1) for lo, hi in cell))
               for g in guards for z in witnesses)


def main():
    # Eight distinct prepared sites: Kmax=10 requires T=Kmax-2=8 guards.
    cell = ((0, 10), (0, 1), (0, 1))
    guards = list(product((7, 8), (0, 1), (0, 1)))
    witnesses = [(17, 0, 0)]
    require(len(guards) == 8, "fixture needs eight distinct guards")
    require(not old_radius_certificate(cell, guards, box_of(witnesses)),
            "old independent extrema unexpectedly reject fixture")
    require(paired_certificate(cell, box_of(guards), box_of(witnesses)),
            "paired certificate missed strict integer fixture")
    require(check_exact(cell, guards, witnesses), "fixture not truly dominated")

    # Exact box predicate is sufficient, not necessary. Exhaustive integer
    # centres verify its implication, including boxes with overlapping axes.
    rng = Random(230923)
    tested = accepted = 0
    for _ in range(1200):
        low = tuple(rng.randrange(0, 7) for _ in range(3))
        cell = tuple((x, x + rng.randrange(0, 3)) for x in low)
        guards = rng.sample(list(product(range(0, 9), repeat=3)), rng.randrange(3, 9))
        available = [p for p in product(range(0, 9), repeat=3) if p not in guards]
        witnesses = rng.sample(available, rng.randrange(1, 4))
        certified = paired_certificate(cell, box_of(guards), box_of(witnesses))
        if certified:
            accepted += 1
            require(check_exact(cell, guards, witnesses),
                    f"false-positive domination: {cell}, {guards}, {witnesses}")
        tested += 1

    # A contact or interior site dominated by T guards forces at least T
    # strict interiors of any sphere through the contact (or beyond it).
    c = (10, 10, 10)
    support = (11, 11, 11)
    near = [(10, 10, 10), (10, 10, 11), (10, 11, 10)]
    require(all(squared(c, g) < squared(c, support) for g in near),
            "K5 contact example lost its three strict interiors")
    # Cross-lane counterexample: a q4 K−2 guard threshold cannot delete Z
    # from q3's shared cover. The positive barycentric weights 6,5,5 / 16
    # place c strictly inside an acute q3 support of radius 5.
    c = (0, 0, 0)
    triangle = ((5, 0, 0), (-3, 4, 0), (-3, -4, 0))
    guards = ((0, 0, 0), (1, 0, 0), (0, 1, 0))
    witness = (2, 0, 0)
    require(all(squared(c, p) == 25 for p in triangle), "q3 radius fixture")
    require(all(sum(w * p[i] for w, p in zip((6, 5, 5), triangle)) == 0
                for i in range(3)), "q3 positive barycentric fixture")
    require(paired_certificate(((0, 0),) * 3, box_of(guards), box_of((witness,))),
            "q3 counterexample needs a valid q4 guard certificate")
    require(all(squared(c, p) < 25 for p in (*guards, witness)) and
            len(guards) == 5 - 2 and len((*guards, witness)) == 5 - 1,
            "q3 depth must cross the K5 rejection threshold")
    print({"status": "PASS", "random_boxes": tested,
           "certified_random_boxes": accepted, "strict_improvement_fixtures": 1,
           "contact_depth_fixtures": 1, "q3_shared_cover_counterexamples": 1})


if __name__ == "__main__":
    main()
