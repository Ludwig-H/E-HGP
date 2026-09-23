#!/usr/bin/env python3
"""Small independent integer check of proposed box prefilters, not a product test."""

from itertools import product
import random


def check(ok, message):
    if not ok:
        raise RuntimeError(message)


def values(interval):
    return range(interval[0], interval[1] + 1)


def ends(interval):
    return sorted(set(interval))


def h_component(a, b, g, h):
    return 4 * ((g - a) * (b - g) + (h - a) * (b - h))


def cross_component(aj, ak, bj, bk, uj, uk):
    return 2 * ((bj - aj) * (uk - ak - bk) -
                (bk - ak) * (uj - aj - bj))


def h_box_min(boxes):
    return min(h_component(*corner) for corner in product(*(ends(x) for x in boxes)))


def cross_box_ends(boxes):
    return (min(cross_component(*corner)
                for corner in product(*(ends(x) for x in boxes))),
            max(cross_component(*corner)
                for corner in product(*(ends(x) for x in boxes))))


def cross_box_ends_optimized(boxes):
    aj_box, ak_box, bj_box, bk_box, uj_box, uk_box = boxes
    lows, highs = [], []
    for aj, ak, bj, bk in product(*(ends(x) for x in boxes[:4])):
        dj, dk = bj - aj, bk - ak
        uj_min = uj_box[0] if dk <= 0 else uj_box[1]
        uj_max = uj_box[1] if dk <= 0 else uj_box[0]
        uk_min = uk_box[0] if dj >= 0 else uk_box[1]
        uk_max = uk_box[1] if dj >= 0 else uk_box[0]
        lows.append(cross_component(aj, ak, bj, bk, uj_min, uk_min))
        highs.append(cross_component(aj, ak, bj, bk, uj_max, uk_max))
    return min(lows), max(highs)


def pair_test(a, b, g, h):
    d = [b[i] - a[i] for i in range(3)]
    D = sum(x * x for x in d)
    w = [2 * (g[i] + h[i] - a[i] - b[i]) for i in range(3)]
    H = sum(h_component(a[i], b[i], g[i], h[i]) for i in range(3))
    X = sum((d[(i + 1) % 3] * w[(i + 2) % 3] -
             d[(i + 2) % 3] * w[(i + 1) % 3]) ** 2 for i in range(3))
    check(H == 2 * D - sum((2 * g[i] - a[i] - b[i]) ** 2 +
                          (2 * h[i] - a[i] - b[i]) ** 2 for i in range(3)),
          "H identity")
    return H, X, H > 0 and 3 * H * H > 4 * X, H > 0 and H * H > 2 * X


def main():
    rng = random.Random(230923)
    for _ in range(30):
        boxes = [(start, start + rng.randrange(3))
                 for start in (rng.randrange(-5, 6) for _ in range(6))]
        h_exact = min(h_component(*p)
                      for p in product(*(values(x) for x in boxes[:4])))
        check(h_box_min(boxes[:4]) == h_exact, "H scalar box")
        x_exact = [cross_component(*p)
                   for p in product(*(values(x) for x in boxes))]
        check(cross_box_ends(boxes) == (min(x_exact), max(x_exact)),
              "X scalar box 64 corners")
        check(cross_box_ends_optimized(boxes) ==
              (min(x_exact), max(x_exact)), "X scalar box 16 corners")

    a, b = (0, 0, 0), (4, 0, 0)
    g, h = (1, -1, -1), (1, 0, 0)
    H, X, q3, q4 = pair_test(a, b, g, h)
    check((H, X, q3, q4) == (16, 128, True, False), "strict q4 equality")
    # q4 is equality: a non-strict port would incorrectly credit this pair.
    check(H * H == 2 * X, "q4 equality value")
    g, h = (2, 1, 0), (2, -1, 0)
    H, X, q3, q4 = pair_test(a, b, g, h)
    check(H > 0 and X == 0 and q3 and q4, "positive q3/q4")
    print("PASS: 30 exhaustive scalar boxes, 3 X components, q4 equality and strict positive")


if __name__ == "__main__":
    main()
