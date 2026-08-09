#!/usr/bin/env python3
"""Fixture entière bornée pour la récolte shallow de la source Gate D.

Le script n'est ni un chemin produit, ni un oracle général de miniboules. Il
compte, sur une coquille u16 fixe, les paires diamétrales sans extra-shell avec
au plus neuf intérieurs stricts et les triangles aigus sans extra-shell avec au
plus huit intérieurs stricts. Tous les tests sont des signes entiers.
"""

from collections import Counter
from itertools import combinations
from math import isqrt


Point = tuple[int, int, int]


def sub(a: Point, b: Point) -> Point:
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def dot(a: Point, b: Point) -> int:
    return sum(a[i] * b[i] for i in range(3))


def cross(a: Point, b: Point) -> Point:
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def orient(a: Point, b: Point, c: Point, d: Point) -> int:
    return dot(cross(sub(b, a), sub(c, a)), sub(d, a))


def acute(a: Point, b: Point, c: Point) -> bool:
    return (
        dot(sub(b, a), sub(c, a)) > 0
        and dot(sub(a, b), sub(c, b)) > 0
        and dot(sub(a, c), sub(b, c)) > 0
    )


def fixture() -> tuple[Point, list[Point]]:
    radius = 65
    centre = (radius, radius, radius)
    equator: set[Point] = set()
    for x in range(-radius, radius + 1):
        y2 = radius * radius - x * x
        y = isqrt(y2)
        if y * y == y2:
            equator.add((radius + x, radius + y, radius))
            equator.add((radius + x, radius - y, radius))
    points = sorted(equator | {(radius, radius, 0), (radius, radius, 2 * radius)})
    return centre, points


def main() -> None:
    centre, points = fixture()
    pair_depths: Counter[int] = Counter()
    triangle_depths: Counter[int] = Counter()
    pair_extra_shell = 0

    for i, j in combinations(range(len(points)), 2):
        a, b = points[i], points[j]
        signs = [
            dot(sub(z, a), sub(z, b))
            for point_id, z in enumerate(points)
            if point_id not in (i, j)
        ]
        depth = sum(value < 0 for value in signs)
        if any(value == 0 for value in signs):
            pair_extra_shell += 1
        elif depth <= 9:
            pair_depths[depth] += 1

    for ids in combinations(range(len(points)), 3):
        a, b, c = (points[point_id] for point_id in ids)
        if not acute(a, b, c):
            continue
        centre_side = orient(a, b, c, centre)
        if centre_side == 0:
            continue
        signs = [
            orient(a, b, c, z)
            for point_id, z in enumerate(points)
            if point_id not in ids
        ]
        depth = sum(value * centre_side < 0 for value in signs)
        if depth <= 8 and not any(value == 0 for value in signs):
            triangle_depths[depth] += 1

    assert len(points) == 38
    assert pair_depths == Counter({0: 108, **{depth: 36 for depth in range(1, 10)}})
    assert triangle_depths == Counter({depth: 72 for depth in range(9)})
    assert pair_extra_shell == 19
    assert sum(pair_depths.values()) == 432
    assert sum(triangle_depths.values()) == 648

    print("points=38 q2=432 q3=648")
    print("q2_extra_shell_excluded=19")
    print("q2_by_depth=" + ",".join(f"{depth}:{pair_depths[depth]}" for depth in range(10)))
    print("q3_by_depth=" + ",".join(f"{depth}:{triangle_depths[depth]}" for depth in range(9)))


if __name__ == "__main__":
    main()
