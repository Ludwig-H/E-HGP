#!/usr/bin/env python3
"""Small exact-integer oracle for the multisite rectangle-corner lemma."""

from itertools import product
from random import Random


def dot(x, y):
    return sum(a * b for a, b in zip(x, y))


def cross(x, y):
    return (
        x[1] * y[2] - x[2] * y[1],
        x[2] * y[0] - x[0] * y[2],
        x[0] * y[1] - x[1] * y[0],
    )


def corners(box):
    return set(product(*((lo, hi) for lo, hi in box)))


def grid(box):
    return product(*(range(lo, hi + 1) for lo, hi in box))


def test_pair(a, b, guards, threshold, lane):
    n = len(guards)
    zsum = tuple(sum(z[i] for z in guards) for i in range(3))
    qsum = sum(dot(z, z) for z in guards)
    d = tuple(b[i] - a[i] for i in range(3))
    s = tuple(a[i] + b[i] for i in range(3))
    dsq = dot(d, d)
    assert dsq > 0
    h = 4 * (dot(s, zsum) - qsum - n * dot(a, b))
    w = tuple(2 * zsum[i] - n * s[i] for i in range(3))
    c = cross(d, w)
    alternate_c = tuple(
        2 * (cross(d, zsum)[i] + n * cross(a, b)[i]) for i in range(3)
    )
    assert c == alternate_c
    assert h == sum(
        dsq - dot(tuple(2 * z[i] - s[i] for i in range(3)),
                  tuple(2 * z[i] - s[i] for i in range(3)))
        for z in guards
    )
    if lane == 3:
        aval, coefficient = 3 * h - 4 * (threshold - 1) * dsq, 12
    else:
        assert lane == 4
        aval, coefficient = 2 * h - 3 * (threshold - 1) * dsq, 8
    return aval > 0 and aval * aval > coefficient * dot(c, c)


def check_box(a_box, b_box, guards, k):
    assert a_box[0][1] < b_box[0][0]
    assert len(guards) == len(set(guards))
    results = []
    for lane, threshold in ((3, k - 1), (4, k - 2)):
        assert threshold > 0
        corner_ok = all(
            test_pair(a, b, guards, threshold, lane)
            for a in corners(a_box)
            for b in corners(b_box)
        )
        if corner_ok:
            assert all(
                test_pair(a, b, guards, threshold, lane)
                for a in grid(a_box)
                for b in grid(b_box)
            )
        results.append(corner_ok)
    return tuple(results)


def main():
    guards = tuple(product((9, 10, 11, 12), (9, 10), (9, 10)))
    a_box = ((0, 1), (9, 10), (9, 10))
    b_box = ((19, 20), (9, 10), (9, 10))
    planted = check_box(a_box, b_box, guards, 5)
    assert planted == (True, True), planted

    rng = Random(20260924)
    corner_certifications = 2
    for _ in range(12):
        scale = rng.randrange(1, 4)
        shift = tuple(rng.randrange(0, 1000) for _ in range(3))
        moved_a = tuple((scale * lo + shift[i], scale * hi + shift[i])
                        for i, (lo, hi) in enumerate(a_box))
        moved_b = tuple((scale * lo + shift[i], scale * hi + shift[i])
                        for i, (lo, hi) in enumerate(b_box))
        moved_guards = tuple(tuple(scale * z[i] + shift[i] for i in range(3))
                             for z in guards)
        assert check_box(moved_a, moved_b, moved_guards, 5) == (True, True)
        corner_certifications += 2
    for _ in range(160):
        a_box = tuple((base := rng.randrange(0, 4), base + rng.randrange(0, 2))
                      for _ in range(3))
        b_box = (
            (base := rng.randrange(8, 12), base + rng.randrange(0, 2)),
            (base := rng.randrange(0, 4), base + rng.randrange(0, 2)),
            (base := rng.randrange(0, 4), base + rng.randrange(0, 2)),
        )
        guards = tuple(rng.sample(list(product(range(2, 10), repeat=3)),
                                  rng.randrange(1, 16)))
        corner_certifications += sum(check_box(a_box, b_box, guards,
                                                rng.choice((5, 10))))
    print(f"OK: 12 translated/scaled and 160 random boxes, "
          f"planted two-lane box, "
          f"{corner_certifications} corner certifications")


if __name__ == "__main__":
    main()
