#!/usr/bin/env python3
"""Bounded integer model of P0 tube credits; no FULL engine or timing claim.

The proposed path sorts cell/projection records and counts suffixes with two
pointers. Quadratic loops occur only in the independent bounded judges below.
The model repeats grid/sort/range preparation for each lane; runtime sharing
between lanes is not exercised. Residual enumeration is a bounded judge, not
an implementation of the proposed output-sensitive credit-class selector.
No compiled v7/v8 producer is called. Python integers have unbounded precision.
"""

import itertools
import json
import sys
from collections import Counter
from fractions import Fraction
from typing import Any

Point = tuple[int, int, int]
Sites = list[tuple[int, Point]]
Model = dict[str, Any]


class GateFailure(Exception):
    """A scientific comparison or non-vacuity requirement failed."""


def require(condition: bool, cause: str) -> None:
    if not condition:
        raise GateFailure(cause)


def sub(a: Point, b: Point) -> Point:
    return tuple(x - y for x, y in zip(a, b))


def dot(a: Point, b: Point) -> int:
    return sum(x * y for x, y in zip(a, b))


def cross(a: Point, b: Point) -> Point:
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def box(points: Sites) -> tuple[Point, Point]:
    return (tuple(min(p[j] for _, p in points) for j in range(3)),
            tuple(max(p[j] for _, p in points) for j in range(3)))


def corners(points: Sites) -> tuple[Point, ...]:
    lo, hi = box(points)
    return tuple(itertools.product(*(sorted({lo[j], hi[j]})
                                    for j in range(3))))


def axis_and_separation(a_points: Sites, b_points: Sites, separation: int) -> Point:
    alo, ahi = box(a_points)
    blo, bhi = box(b_points)
    axis = tuple(blo[j] + bhi[j] - alo[j] - ahi[j] for j in range(3))
    diameter_a2 = dot(sub(ahi, alo), sub(ahi, alo))
    diameter_b2 = dot(sub(bhi, blo), sub(bhi, blo))
    require(separation >= 8, "separation.profile")
    require(dot(axis, axis) >= (separation + 2) ** 2 *
            max(diameter_a2, diameter_b2), "separation.failed")
    require(dot(axis, axis) > 0, "separation.zero_axis")
    return axis


def spindle(q: int, a: Point, b: Point, z: Point) -> bool:
    # Direct point predicate, independently of the tube bound.
    u = sub(z, a)
    v = sub(b, z)
    h = dot(u, v)
    if h <= 0:
        return False
    if q == 2:
        return True
    uv = cross(u, v)
    return (3 if q == 3 else 2) * h * h > dot(uv, uv)


def coefficients(q: int) -> tuple[int, int]:
    return {2: (1, 9), 3: (1, 1), 4: (16, 9)}[q]


def tube_credits(points: Sites, axis: Point, q: int, cap: int,
                 width_factor: int = 4, mutant: str | None = None) -> Model:
    """One paid grid, one sort, and one monotone suffix sweep per cell.

    Certificates are (cell index, suffix start), not expanded witness lists.
    Width is an explicitly supplied constant times max(abs(axis)); its choice
    performs no search. Cell origins and actual transverse ranges are scanned.
    """
    width = width_factor * max(abs(x) for x in axis)
    require(width > 0 and cap > 0, "tube.parameters")
    transformed = [(pid, dot(axis, p), cross(axis, p)) for pid, p in points]
    origin = tuple(min(u[j] for _, _, u in transformed) for j in range(3))
    records = []
    for pid, projection, transverse in transformed:
        cell = tuple((transverse[j] - origin[j]) // width for j in range(3))
        records.append((cell, projection, pid, transverse))
    records.sort(key=lambda record: record[:3])
    counts, certificates, cells = {}, {}, []
    left_coefficient, right_coefficient = coefficients(q)
    comparisons = 0
    for _, iterator in itertools.groupby(records, key=lambda record: record[0]):
        rows = list(iterator)
        spans = [max(row[3][j] for row in rows) -
                 min(row[3][j] for row in rows) for j in range(3)]
        bound = sum(span * span for span in spans)
        cell_id = len(cells)
        cells.append((bound, rows))
        first = 0
        for _, projection, pid, _ in rows:
            while first < len(rows):
                gap = rows[first][1] - projection
                comparisons += 1
                positive = gap >= 0 if mutant == "allow_self" else gap > 0
                effective_bound = 0 if mutant == "ignore_transverse" else bound
                if (positive and left_coefficient * effective_bound <=
                        right_coefficient * gap * gap):
                    break
                first += 1
            counts[pid] = min(cap, len(rows) - first)
            certificates[pid] = (cell_id, first)
    return {"counts": counts, "certificates": certificates,
            "cells": cells, "comparisons": comparisons}


def check_credits(model: Model, points: Sites, opposite: Sites, axis: Point,
                  q: int, cap: int, stats: Counter) -> None:
    """Brute suffix-set comparison plus actual ALL-corners spindle judge."""
    coordinates = dict(points)
    c, d = coefficients(q)
    for aid, a in points:
        cell_id, start = model["certificates"][aid]
        bound, rows = model["cells"][cell_id]
        actual = {row[2] for row in rows[start:]}
        expected = set()
        for row in rows:
            gap = dot(axis, sub(coordinates[row[2]], a))
            if gap > 0 and c * bound <= d * gap * gap:
                expected.add(row[2])
            stats["brute_tube_comparisons"] += 1
        require(actual == expected, "tube.suffix_set")
        require(model["counts"][aid] == min(cap, len(expected)), "tube.count")
        for zid in actual:
            z = coordinates[zid]
            require(zid != aid, "tube.self_witness")
            gap = dot(axis, sub(z, a))
            actual_cross = cross(axis, sub(z, a))
            require(gap > 0 and c * dot(actual_cross, actual_cross) <=
                    d * gap * gap, "tube.actual_cone")
            stats["credited_identities"] += 1
            for b in corners(opposite):
                require(spindle(q, a, b, z), "tube.unsound_credit")
                stats["corner_predicates"] += 1


def residual(a_points: Sites, b_points: Sites, a_model: Model, b_model: Model,
             cap: int, mutant: str | None = None) -> set[tuple[int, int]]:
    # Bounded test enumeration: no claim of an implemented class selector.
    if mutant == "empty_if_indecisive":
        return set()
    multiplier = 2 if mutant == "add_identical_grids" else 1
    return {(aid, bid) for aid, _ in a_points for bid, _ in b_points
            if multiplier * (a_model["counts"][aid] +
                             b_model["counts"][bid]) < cap}


def exact_q2(a_points: Sites, b_points: Sites, cap: int,
             stats: Counter) -> set[tuple[int, int]]:
    all_points = a_points + b_points
    survivors = set()
    for aid, a in a_points:
        for bid, b in b_points:
            inside = 0
            for zid, z in all_points:
                if zid != aid and zid != bid:
                    inside += spindle(2, a, b, z)
                    stats["exact_q2_site_tests"] += 1
            if inside < cap:
                survivors.add((aid, bid))
    return survivors


def final_q2_from_residual(a_points: Sites, b_points: Sites,
                           candidates: set[tuple[int, int]], cap: int,
                           stats: Counter) -> set[tuple[int, int]]:
    points = dict(a_points + b_points)
    result = set()
    for aid, bid in candidates:
        a, b = points[aid], points[bid]
        count = sum(spindle(2, a, b, z) for zid, z in points.items()
                    if zid not in (aid, bid))
        stats["fallback_q2_pairs"] += 1
        if count < cap:
            result.add((aid, bid))
    return result


def tube_fixture(m: int, separation: int, orientation: int) -> list[Sites]:
    transforms = (
        lambda x, y, z: (x, y, z),
        lambda x, y, z: (y, x, z),
        lambda x, y, z: (-x, y, z),
        lambda x, y, z: (x + y, x - y, z),
        lambda x, y, z: (z, y, x),
    )
    transform = transforms[orientation]
    result = []
    for side in range(2):
        points = []
        for i in range(m):
            raw = transform(2 * i + side * (separation + 2) * m,
                            i % 2, (i // 2) % 2)
            p = tuple(value + 1000 for value in raw)
            require(all(0 <= value <= 65535 for value in p), "input.u16")
            points.append((side * m + i, p))
        result.append(points)
    return result


def expect_mutant_failure(cause: str, function: Any, stats: Counter) -> None:
    try:
        function()
    except GateFailure as error:
        require(str(error) == cause, "mutant.wrong_rejection")
        stats["physical_mutants_rejected"] += 1
        return
    raise GateFailure("mutant.survived")


def minimax_h4(a_points: Sites, b_points: Sites, z_points: Sites) -> int:
    """Independent integer transcription of the bounded negative-box lemma."""
    alo, ahi = box(a_points)
    blo, bhi = box(b_points)
    zlo, zhi = box(z_points)
    result = 0
    for j in range(3):
        values = []
        for a in (alo[j], ahi[j]):
            for b in (blo[j], bhi[j]):
                projected = min(max(a + b, 2 * zlo[j]), 2 * zhi[j])
                values.append((b - a) ** 2 - (projected - a - b) ** 2)
        result += min(values)
    return result


def selftest() -> dict[str, Any]:
    stats = Counter()
    beta = Fraction(1, 4)
    for q, rho in ((2, Fraction(3)), (3, Fraction(1)), (4, Fraction(3, 4))):
        require(1 - rho * beta > 0, "proof.positive_h")
        if q != 2:
            margin = (3 if q == 3 else 2) * (1 - rho * beta) ** 2 - (rho + beta) ** 2
            require(margin == (Fraction(1, 8) if q == 3 else Fraction(41, 128)),
                    "proof.margin")
        stats["exact_coefficient_checks"] += 1
    canonical = {}
    for separation in (8, 10, 12):
        for orientation in range(5):
            for permuted in (False, True):
                a_points, b_points = tube_fixture(12, separation, orientation)
                if permuted:
                    a_points = list(reversed(a_points))
                    b_points = b_points[5:] + b_points[:5]
                axis = axis_and_separation(a_points, b_points, separation)
                stats["separated_fixtures"] += 1
                # Verify the connection-cone premise directly at all box corners.
                for a in corners(a_points):
                    for b in corners(b_points):
                        v = sub(b, a)
                        axial = dot(axis, v)
                        transverse = cross(axis, v)
                        require(axial > 0 and 16 * dot(transverse, transverse) <=
                                axial * axial, "proof.connection_cone")
                        stats["connection_corner_checks"] += 1
                for q in (2, 3, 4):
                    cap = 12 - q
                    am = tube_credits(a_points, axis, q, cap)
                    reverse = tuple(-x for x in axis)
                    bm = tube_credits(b_points, reverse, q, cap)
                    check_credits(am, a_points, b_points, axis, q, cap, stats)
                    check_credits(bm, b_points, a_points, reverse, q, cap, stats)
                    candidates = residual(a_points, b_points, am, bm, cap)
                    require(len(candidates) == cap * (cap + 1) // 2,
                            "tube.expected_residual")
                    signature = (am["counts"], bm["counts"], candidates)
                    key = (separation, orientation, q)
                    if permuted:
                        require(signature == canonical[key], "tube.permutation")
                        stats["permutation_comparisons"] += 1
                    else:
                        canonical[key] = signature
                    stats["model_comparisons"] += am["comparisons"] + bm["comparisons"]
                    require(am["comparisons"] <= 2 * len(a_points) and
                            bm["comparisons"] <= 2 * len(b_points), "tube.linear_sweep")
                    stats["lane_runs"] += 1
                    stats["residual_pairs"] += len(candidates)
                    if q == 2:
                        exact = exact_q2(a_points, b_points, cap, stats)
                        final = final_q2_from_residual(a_points, b_points, candidates,
                                                      cap, stats)
                        require(exact == final == candidates, "tube.q2_final")
                        stats["q2_final_comparisons"] += 1
    # An intentionally unhelpful wide transverse cell: all ordinary credits zero.
    a_points = [(0, (0, 0, 0)), (1, (1, 100, 0))]
    b_points = [(2, (1000, 0, 0)), (3, (1001, 100, 0))]
    axis = axis_and_separation(a_points, b_points, 12)
    reverse = tuple(-x for x in axis)
    for q in (2, 3, 4):
        am = tube_credits(a_points, axis, q, 1, width_factor=1024)
        bm = tube_credits(b_points, reverse, q, 1, width_factor=1024)
        check_credits(am, a_points, b_points, axis, q, 1, stats)
        check_credits(bm, b_points, a_points, reverse, q, 1, stats)
        require(sum(am["counts"].values()) + sum(bm["counts"].values()) == 0,
                "indecisive.expected")
        stats["indecisive_lane_runs"] += 1
    exact = exact_q2(a_points, b_points, 1, stats)
    require(len(exact) > 0, "indecisive.nonvacuity")
    am = tube_credits(a_points, axis, 2, 1, width_factor=1024)
    bm = tube_credits(b_points, reverse, 2, 1, width_factor=1024)
    candidates = residual(a_points, b_points, am, bm, 1)
    require(final_q2_from_residual(a_points, b_points, candidates, 1, stats) == exact,
            "indecisive.fallback")
    bad = tube_credits(a_points, axis, 2, 1, width_factor=1024,
                       mutant="ignore_transverse")
    require(sum(bad["counts"].values()) > 0, "mutant.rank_not_exercised")
    # A direct geometric failure, rather than only disagreement with our bound.
    def reject_rank() -> None:
        cell_id, start = bad["certificates"][0]
        rows = bad["cells"][cell_id][1]
        for row in rows[start:]:
            require(all(spindle(2, dict(a_points)[0], b, dict(a_points)[row[2]])
                        for b in corners(b_points)), "mutant.rank_unsound")
    expect_mutant_failure("mutant.rank_unsound", reject_rank, stats)
    def reject_empty() -> None:
        bad_candidates = residual(a_points, b_points, am, bm, 1,
                                  mutant="empty_if_indecisive")
        require(final_q2_from_residual(a_points, b_points, bad_candidates, 1, stats)
                == exact, "mutant.lost_q2")
    expect_mutant_failure("mutant.lost_q2", reject_empty, stats)
    # The same geometric population cannot be added through two identical grids.
    a_points, b_points = tube_fixture(12, 8, 0)
    axis = axis_and_separation(a_points, b_points, 8)
    am = tube_credits(a_points, axis, 2, 10)
    bm = tube_credits(b_points, tuple(-x for x in axis), 2, 10)
    exact = exact_q2(a_points, b_points, 10, stats)
    require(final_q2_from_residual(a_points, b_points,
                                  residual(a_points, b_points, am, bm, 10),
                                  10, stats) == exact, "mutant.control")
    def reject_double() -> None:
        bad_candidates = residual(a_points, b_points, am, bm, 10,
                                  mutant="add_identical_grids")
        require(len(bad_candidates) < len(exact), "mutant.double_not_exercised")
        require(final_q2_from_residual(a_points, b_points, bad_candidates, 10, stats)
                == exact, "mutant.double_lost_q2")
    expect_mutant_failure("mutant.double_lost_q2", reject_double, stats)
    a_points, b_points = [(0, (0, 0, 0))], [(1, (10, 0, 0))]
    axis = axis_and_separation(a_points, b_points, 8)
    normal = tube_credits(a_points, axis, 2, 1)
    require(normal["counts"][0] == 0, "self.nominal")
    bad = tube_credits(a_points, axis, 2, 1, mutant="allow_self")
    require(bad["counts"][0] == 1, "mutant.self_not_exercised")
    def reject_self() -> None:
        cell_id, start = bad["certificates"][0]
        zid = bad["cells"][cell_id][1][start][2]
        require(spindle(2, a_points[0][1], b_points[0][1], dict(a_points)[zid]),
                "mutant.self_boundary")
    expect_mutant_failure("mutant.self_boundary", reject_self, stats)
    # A full-dimensional, isotropic box can hide genuine local witnesses from
    # this deliberately wide grid. Failure to credit does not mean absence.
    a_points = list(enumerate(itertools.product((0, 10), repeat=3)))
    b_points = [(8 + pid, (p[0] + 200, p[1], p[2])) for pid, p in a_points]
    axis = axis_and_separation(a_points, b_points, 12)
    am = tube_credits(a_points, axis, 4, 8, width_factor=1024)
    check_credits(am, a_points, b_points, axis, 4, 8, stats)
    require(sum(am["counts"].values()) == 0, "isotropic.expected_indecisive")
    actual_witnesses = sum(all(spindle(4, a, b, z) for b in corners(b_points))
                           for aid, a in a_points for zid, z in a_points
                           if aid != zid)
    require(actual_witnesses > 0, "isotropic.missed_witness_nonvacuity")
    stats["isotropic_uncredited_witnesses"] = actual_witnesses
    # A universal-negative result on a parent is not hereditary under restriction.
    a_points = [(0, (4, 100, 0)), (1, (4, 102, 0))]
    b_points = [(2, (4, 0, 0))]
    z_points = [(3, (3, 101, 0))]
    axis_and_separation(a_points, b_points, 12)
    parent_negative = minimax_h4(a_points, b_points, z_points)
    require(parent_negative == -408, "negative.parent_minimax")
    child = [a_points[1]]
    z = z_points[0][1]
    require(not all(spindle(4, a, b, z) for a in corners(a_points)
                    for b in corners(b_points)), "negative.parent_control")
    require(all(spindle(4, a, b, z) for a in corners(child)
                for b in corners(b_points)), "negative.child_positive")
    def reject_negative_transfer() -> None:
        inherited = [] if parent_negative <= 0 else z_points
        require(sum(all(spindle(4, a, b, p) for a in corners(child)
                        for b in corners(b_points)) for _, p in inherited) == 1,
                "mutant.negative_transfer")
    expect_mutant_failure("mutant.negative_transfer", reject_negative_transfer, stats)
    stats["negative_restriction_fixtures"] += 1
    require(stats["lane_runs"] == 90 and stats["credited_identities"] > 1000 and
            stats["corner_predicates"] > 10000 and
            stats["physical_mutants_rejected"] == 5 and
            stats["q2_final_comparisons"] == 30, "gate.nonvacuity")
    return {"status": "passed_bounded_tube_model", **dict(stats),
            "product_executed": False, "full_qualification": False,
            "performance_claim": False, "gcp_used": False,
            "scope": "integer structural filter model; bounded geometric judges; no FULL engine"}


def main() -> int:
    if sys.argv[1:] != ["--selftest"]:
        return 2
    try:
        result = selftest()
    except GateFailure as error:
        print(json.dumps({"status": "failed", "cause": str(error)}, sort_keys=True))
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
