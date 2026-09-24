#!/usr/bin/env python3
"""Exact small-box checks for the multisite q3/q4 rectangle certificate.

All checks use explicit exceptions so that ``python3 -O`` runs the same gate.
This is an audit oracle, not a generator or a proof of the corner lemma.
"""

from __future__ import annotations

from dataclasses import dataclass
from itertools import product

Point = tuple[int, int, int]
Box = tuple[tuple[int, int], tuple[int, int], tuple[int, int]]
Guard = tuple[int, Point]


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def dot(a: Point, b: Point) -> int:
    return sum(x * y for x, y in zip(a, b))


def cross(a: Point, b: Point) -> Point:
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def corners(box: Box) -> tuple[Point, ...]:
    # Keep repeated corners for a degenerate box: this still executes all
    # 8 x 8 crossed corner tests, never just matched or representative corners.
    return tuple(product(*((low, high) for low, high in box)))


def integer_points(box: Box) -> tuple[Point, ...]:
    return tuple(product(*(range(low, high + 1) for low, high in box)))


@dataclass(frozen=True)
class Terms:
    d: Point
    D: int
    H: int
    W: Point
    X: int
    A3: int
    A4: int

    def lane(self, q: int) -> bool:
        if q == 3:
            return self.D > 0 and self.A3 > 0 and self.A3 * self.A3 > 12 * self.X
        need(q == 4, "unknown lane")
        return self.D > 0 and self.A4 > 0 and self.A4 * self.A4 > 8 * self.X


def terms(a: Point, b: Point, guards: tuple[Guard, ...], k: int) -> Terms:
    need(k >= 3, "both thresholds must be positive")
    n = len(guards)
    z = tuple(sum(point[i] for _, point in guards) for i in range(3))
    q = sum(dot(point, point) for _, point in guards)
    d = tuple(b[i] - a[i] for i in range(3))
    s = tuple(a[i] + b[i] for i in range(3))
    D = dot(d, d)
    W = tuple(2 * z[i] - n * s[i] for i in range(3))
    H = 4 * (dot(s, z) - q - n * dot(a, b))
    C = cross(d, W)
    X = dot(C, C)

    # Independent integer expansions of both moment identities. In particular,
    # these checks still run for coincident virtual endpoints (D = 0).
    direct_h = 0
    for _, point in guards:
        w = tuple(2 * point[i] - s[i] for i in range(3))
        direct_h += D - dot(w, w)
    alternate_c = tuple(2 * (cross(d, z)[i] + n * cross(a, b)[i])
                        for i in range(3))
    need(H == direct_h and C == alternate_c,
         f"moment identity failed at a={a}, b={b}")
    A3 = 3 * H - 4 * (k - 2) * D
    A4 = 2 * H - 3 * (k - 3) * D
    result = Terms(d, D, H, W, X, A3, A4)
    if D == 0:
        need(not result.lane(3) and not result.lane(4),
             "coincident virtual endpoints certified a lane")
    return result


@dataclass(frozen=True)
class Case:
    name: str
    a_box: Box
    b_box: Box
    guards: tuple[Guard, ...]
    expected: tuple[bool, bool]
    endpoint_guard: int | None = None
    boundary: tuple[int, str] | None = None
    positive_pairs_without_rectangle: bool = False
    require_distinct_corners: bool = False


def check_case(case: Case) -> None:
    ids = [site_id for site_id, _ in case.guards]
    points = [point for _, point in case.guards]
    need(len(ids) == len(set(ids)) == len(points) == len(set(points)),
         f"{case.name}: guards must have distinct IDs and coordinates")
    for box in (case.a_box, case.b_box):
        need(len(box) == 3 and all(0 <= low <= high < (1 << 18)
                                   for low, high in box),
             f"{case.name}: box outside u18")
    need(all(all(0 <= coordinate < (1 << 18) for coordinate in point)
             for point in points), f"{case.name}: guard outside u18")

    if case.endpoint_guard is not None:
        first_a = tuple(low for low, _ in case.a_box)
        first_b = tuple(low for low, _ in case.b_box)
        guard = dict(case.guards).get(case.endpoint_guard)
        need(guard == first_a, f"{case.name}: endpoint ID is not reused")
        t = terms(first_a, first_b, case.guards, 5)
        s = tuple(first_a[i] + first_b[i] for i in range(3))
        w = tuple(2 * guard[i] - s[i] for i in range(3))
        need(w == tuple(-x for x in t.d) and t.D - dot(w, w) == 0,
             f"{case.name}: reused endpoint has nonzero margin")

    if case.boundary is not None:
        lane, kind = case.boundary
        a = tuple(low for low, _ in case.a_box)
        b = tuple(low for low, _ in case.b_box)
        t = terms(a, b, case.guards, 5)
        A, factor = (t.A3, 12) if lane == 3 else (t.A4, 8)
        if kind == "A=0":
            need(A == 0, f"{case.name}: expected A=0, got {A}")
        else:
            need(kind == "square" and A > 0 and A * A == factor * t.X,
                 f"{case.name}: expected positive-A squared equality")
        need(not t.lane(lane), f"{case.name}: equality incorrectly certified")

    corner_a, corner_b = corners(case.a_box), corners(case.b_box)
    need(len(corner_a) * len(corner_b) == 64, "not all crossed corners tested")
    if case.require_distinct_corners:
        need(len(set(corner_a)) == len(set(corner_b)) == 8,
             f"{case.name}: expected 64 distinct crossed corners")
    corner_terms = [terms(a, b, case.guards, 5)
                    for a, b in product(corner_a, corner_b)]
    corner_result = (all(t.lane(3) for t in corner_terms),
                     all(t.lane(4) for t in corner_terms))
    need(corner_result == case.expected,
         f"{case.name}: corner result {corner_result} != {case.expected}")

    integer_a, integer_b = integer_points(case.a_box), integer_points(case.b_box)
    integer_terms = [terms(a, b, case.guards, 5)
                     for a, b in product(integer_a, integer_b)]
    exhaustive_result = (all(t.lane(3) for t in integer_terms),
                         all(t.lane(4) for t in integer_terms))
    need(exhaustive_result == corner_result,
         f"{case.name}: corner/exhaustive disagreement")
    positives = (sum(t.lane(3) for t in integer_terms),
                 sum(t.lane(4) for t in integer_terms))
    if case.positive_pairs_without_rectangle:
        need(not any(corner_result) and all(positives),
             f"{case.name}: expected positive edges but no rectangle proof")
    degenerate = sum(t.D == 0 for t in integer_terms)
    if case.name.startswith("overlap"):
        need(degenerate > 0, f"{case.name}: overlap did not exercise D=0")
    print(f"{case.name}: corners=64 integer_pairs={len(integer_terms)} "
          f"D0={degenerate} rectangle=q3:{int(corner_result[0])},q4:{int(corner_result[1])} "
          f"positive_pairs=q3:{positives[0]},q4:{positives[1]}")


def symmetric_guards(scale: int) -> tuple[Guard, ...]:
    middle = 10 * scale
    points = tuple((middle, middle + sign_y * y, middle + sign_z * z)
                   for y, z in ((scale, 3 * scale), (3 * scale, scale))
                   for sign_y, sign_z in product((-1, 1), repeat=2))
    return tuple((i + 2, point) for i, point in enumerate(points))


def centered_guards(middle: Point, vectors: tuple[Point, ...],
                    extra: Point | None = None) -> tuple[Guard, ...]:
    points = []
    for v in vectors:
        points.extend((tuple(middle[i] + sign * v[i] for i in range(3))
                       for sign in (-1, 1)))
    if extra is not None:
        points.append(tuple(middle[i] + extra[i] for i in range(3)))
    return tuple((i + 2, point) for i, point in enumerate(points))


def cases() -> tuple[Case, ...]:
    scaled = symmetric_guards(20)
    endpoint = (0, (100, 200, 200))
    q4_scaled = symmetric_guards(30)[:-1]
    q3_only = tuple((i + 2, (500, 1274, 950 + i)) for i in range(100))
    overlap_guards = symmetric_guards(1)
    return (
        Case("endpoint_guard_both", ((100, 102), (200, 202), (200, 202)),
             ((300, 302), (200, 202), (200, 202)), scaled + (endpoint,),
             (True, True), endpoint_guard=0, require_distinct_corners=True),
        Case("q4_only", ((150, 152), (300, 302), (300, 302)),
             ((450, 452), (300, 302), (300, 302)), q4_scaled,
             (False, True), require_distinct_corners=True),
        Case("q3_only", ((0, 2), (1000, 1002), (1000, 1002)),
             ((1000, 1002), (1000, 1002), (1000, 1002)), q3_only,
             (True, False), require_distinct_corners=True),
        Case("overlap_corner_D0", ((5, 15), (10, 10), (10, 10)),
             ((5, 15), (10, 10), (10, 10)), overlap_guards,
             (False, False), positive_pairs_without_rectangle=True),
        Case("overlap_interior_D0", ((5, 15), (10, 10), (10, 10)),
             ((4, 16), (10, 10), (10, 10)), overlap_guards,
             (False, False), positive_pairs_without_rectangle=True),
        Case("q3_A_zero", ((5, 5), (10, 10), (10, 10)),
             ((15, 15), (10, 10), (10, 10)),
             centered_guards((10, 10, 10), ((0, 3, 0), (0, 1, 3),
                                            (0, 2, 3), (0, 3, 3))),
             (False, True), boundary=(3, "A=0")),
        Case("q4_A_zero", ((5, 5), (10, 10), (10, 10)),
             ((15, 15), (10, 10), (10, 10)),
             centered_guards((10, 10, 10), ((0, 3, 0), (0, 0, 4)),
                             (0, 0, 0)),
             (False, False), boundary=(4, "A=0")),
        Case("q3_square_equality", ((5, 5), (5, 5), (10, 10)),
             ((15, 15), (15, 15), (10, 10)),
             centered_guards((10, 10, 10), ((0, 3, 0), (0, 1, 3)),
                             (0, 1, 1)),
             (False, True), boundary=(3, "square")),
        Case("q4_square_equality", ((5, 5), (10, 10), (10, 10)),
             ((15, 15), (10, 10), (10, 10)),
             centered_guards((10, 10, 10), ((0, 3, 0), (0, 1, 3)),
                             (0, 1, 1)),
             (False, False), boundary=(4, "square")),
    )


def main() -> None:
    for case in cases():
        check_case(case)
    print("OK: 9 exact rectangle fixtures; checks active under python -O")


if __name__ == "__main__":
    main()
