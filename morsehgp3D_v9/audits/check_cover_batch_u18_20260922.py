#!/usr/bin/env python3
"""Oracle entier du certificat de cover commun à un bloc d'arêtes.

Le cover d'une arête ab est la boule fermée de centre (a+b)/2 et de rayon
|a-b|. Les intervalles ci-dessous certifient un produit A×B×Z ; une
subdivision exacte rend chaque cover individuel. Ce test ne mesure pas le
pipeline WSPD, l'atlas q4 ni le temps de la tour.
"""
from __future__ import annotations

import json
import random
from dataclasses import dataclass

M = (1 << 18) - 1
Point = tuple[int, int, int]


def require(ok: bool, message: str) -> None:
    if not ok:
        raise RuntimeError(message)


@dataclass(frozen=True)
class Box:
    lo: Point
    hi: Point


def box(points: list[Point]) -> Box:
    require(bool(points), "empty point set")
    return Box(tuple(min(p[i] for p in points) for i in range(3)),
               tuple(max(p[i] for p in points) for i in range(3)))


def abs_min_max(low: int, high: int) -> tuple[int, int]:
    require(low <= high, "inverted interval")
    minimum = 0 if low <= 0 <= high else min(abs(low), abs(high))
    return minimum, max(abs(low), abs(high))


def cover_bounds(a: Box, b: Box, z: Box) -> tuple[int, int]:
    nearest = farthest = nearest_edge = farthest_edge = 0
    for i in range(3):
        rz_lo = 2 * z.lo[i] - a.hi[i] - b.hi[i]
        rz_hi = 2 * z.hi[i] - a.lo[i] - b.lo[i]
        mn, mx = abs_min_max(rz_lo, rz_hi)
        nearest += mn * mn
        farthest += mx * mx
        ab_lo, ab_hi = a.lo[i] - b.hi[i], a.hi[i] - b.lo[i]
        mn, mx = abs_min_max(ab_lo, ab_hi)
        nearest_edge += mn * mn
        farthest_edge += mx * mx
    lower, upper = nearest - 4 * farthest_edge, farthest - 4 * nearest_edge
    require(-(12 * M * M) <= lower <= upper <= 12 * M * M,
            "cover bounds exceed certified i64 u18 domain")
    return lower, upper


def cover_form(a: Point, b: Point, z: Point) -> int:
    value = sum((2 * z[i] - a[i] - b[i]) ** 2
                - 4 * (a[i] - b[i]) ** 2 for i in range(3))
    require(abs(value) <= 12 * M * M, "point cover exceeds i64 domain")
    return value


def verify_grouped(edges: list[tuple[Point, Point]],
                   witnesses: list[Point]) -> int:
    """Split edge×Z cells; all terminal tiles are disjoint and exact."""
    covered = [set() for _ in edges]
    assigned = [set() for _ in edges]
    visits = 0

    def visit(edge_ids: list[int], first: int, last: int) -> None:
        nonlocal visits
        visits += 1
        ba = box([edges[i][0] for i in edge_ids])
        bb = box([edges[i][1] for i in edge_ids])
        bz = box(witnesses[first:last])
        lower, upper = cover_bounds(ba, bb, bz)
        if upper <= 0 or lower > 0:
            for i in edge_ids:
                for j in range(first, last):
                    require(j not in assigned[i], "duplicated edge-site tile")
                    assigned[i].add(j)
                    if upper <= 0:
                        covered[i].add(j)
            return
        if len(edge_ids) > 1:
            half = len(edge_ids) // 2
            visit(edge_ids[:half], first, last)
            visit(edge_ids[half:], first, last)
            return
        if last - first > 1:
            middle = (first + last) // 2
            visit(edge_ids, first, middle)
            visit(edge_ids, middle, last)
            return
        i, j = edge_ids[0], first
        require(j not in assigned[i], "duplicated exact leaf")
        assigned[i].add(j)
        if cover_form(*edges[i], witnesses[j]) <= 0:
            covered[i].add(j)

    visit(list(range(len(edges))), 0, len(witnesses))
    for i, (a, b) in enumerate(edges):
        require(assigned[i] == set(range(len(witnesses))),
                "edge-site tile missing")
        expected = {j for j, z in enumerate(witnesses)
                    if cover_form(a, b, z) <= 0}
        require(covered[i] == expected, "grouped cover differs from scalar")
    return visits


def main() -> None:
    require(12 * M * M < (1 << 40), "u18 cover width")
    a, b = (10, 10, 10), (14, 10, 10)
    for z in ((8, 10, 10), (12, 14, 10), (16, 10, 10)):
        require(cover_form(a, b, z) == 0,
                "tangent point is not an equality")
        require(cover_bounds(box([a]), box([b]), box([z])) == (0, 0),
                "tangent box should admit exactly")
    rng = random.Random(20260922)
    box_cases = 0
    grouped_cases = 0
    grouped_visits = 0
    for case in range(1001):
        count_a, count_b, count_z = (rng.randint(1, 5),
                                    rng.randint(1, 5),
                                    rng.randint(1, 9))
        def point() -> Point:
            if case % 3:
                return tuple(rng.randint(0, 25) for _ in range(3))
            return tuple(rng.choice((0, 1, M - 1, M)) for _ in range(3))
        aa = [point() for _ in range(count_a)]
        bb = [point() for _ in range(count_b)]
        zz = [point() for _ in range(count_z)]
        lower, upper = cover_bounds(box(aa), box(bb), box(zz))
        for pa in aa:
            for pb in bb:
                for pz in zz:
                    value = cover_form(pa, pb, pz)
                    require(lower <= value <= upper, "box bound excludes exact value")
                    box_cases += 1
        edges = [(pa, pb) for pa in aa for pb in bb if pa != pb]
        if edges:
            grouped_visits += verify_grouped(edges, zz)
            grouped_cases += 1
    print(json.dumps({"status": "PASS", "box_triplets": box_cases,
                      "grouped_families": grouped_cases,
                      "grouped_visits": grouped_visits,
                      "tangent_fixtures": 3}, sort_keys=True))


if __name__ == "__main__":
    main()
