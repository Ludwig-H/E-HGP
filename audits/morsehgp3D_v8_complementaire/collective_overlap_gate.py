#!/usr/bin/env python3
"""Exact overlapping-group credits and bounded certificate compression model.

This independent audit model imports no producer or other auditor code.
"""

from __future__ import annotations

from fractions import Fraction as F
import hashlib
from itertools import combinations
import json
from pathlib import Path
import sys


Point = tuple[int | F, int | F, int | F]


def require(condition: bool, cause: str) -> None:
    if not condition:
        raise RuntimeError(cause)


def dot(a: Point, b: Point) -> F:
    return sum((F(x) * F(y) for x, y in zip(a, b, strict=True)), F(0))


def subtract(a: Point, b: Point) -> Point:
    return tuple(F(x) - F(y) for x, y in zip(a, b, strict=True))


def power(point: Point, center: Point, radius2: F) -> F:
    delta = subtract(point, center)
    return dot(delta, delta) - radius2


def group_gap(a: Point, b: Point, sites: list[Point], weights: list[F]) -> F:
    require(len(sites) == len(weights) and bool(sites), "invalid group lengths")
    require(all(weight >= 0 for weight in weights) and sum(weights) == 1,
            "invalid convex weights")
    mean: Point = tuple(sum((weight * F(site[axis])
                            for site, weight in zip(sites, weights, strict=True)), F(0))
                        for axis in range(3))
    edge = subtract(b, a)
    lam = dot(subtract(mean, a), edge) / dot(edge, edge)
    require(0 < lam < 1 and all(mean[axis] == (1 - lam) * F(a[axis]) +
                              lam * F(b[axis]) for axis in range(3)),
            "group mean is not on the open chord")
    gap = sum((weight * dot(site, site)
               for site, weight in zip(sites, weights, strict=True)), F(0))
    gap -= (1 - lam) * dot(a, a) + lam * dot(b, b)
    require(gap < 0, "group margin is not strict")
    return gap


def fractional_credit(groups: list[tuple[int, ...]], weights: list[F]) -> int:
    require(len(groups) == len(weights), "packing lengths differ")
    loads: dict[int, F] = {}
    for group, weight in zip(groups, weights, strict=True):
        require(weight >= 0 and bool(group) and len(set(group)) == len(group),
                "invalid group IDs or packing weight")
        for identity in group:
            loads[identity] = loads.get(identity, F(0)) + weight
    require(all(value <= 1 for value in loads.values()), "site capacity exceeded")
    total = sum(weights, F(0))
    return (total.numerator + total.denominator - 1) // total.denominator


def null_vector(rows: list[list[F]]) -> list[F]:
    matrix = [row[:] for row in rows]
    columns = len(matrix[0])
    pivots: list[int] = []
    for column in range(columns):
        first = len(pivots)
        pivot = next((i for i in range(first, len(matrix)) if matrix[i][column]), None)
        if pivot is None:
            continue
        matrix[first], matrix[pivot] = matrix[pivot], matrix[first]
        scale = matrix[first][column]
        matrix[first] = [entry / scale for entry in matrix[first]]
        for i in range(len(matrix)):
            if i != first:
                scale = matrix[i][column]
                matrix[i] = [entry - scale * basis for entry, basis
                             in zip(matrix[i], matrix[first], strict=True)]
        pivots.append(column)
        if len(pivots) == len(matrix):
            break
    free = next(column for column in range(columns) if column not in pivots)
    result = [F(0)] * columns
    result[free] = F(1)
    for row, pivot in enumerate(pivots):
        result[pivot] = -matrix[row][free]
    require(all(sum((entry * value for entry, value in zip(row, result, strict=True)),
                    F(0)) == 0 for row in rows), "incorrect affine dependence")
    return result


def compress(a: Point, b: Point, sites: list[Point], weights: list[F]) -> tuple[list[F], int]:
    # Bounded constructive elimination, with this fixture's x-aligned chord.
    # The proof is coordinate invariant; this model is not a fast LP solver.
    require(a[1:] == b[1:], "compression fixture chord is not x aligned")
    current = weights[:]
    gap = group_gap(a, b, sites, current)
    steps = 0
    while sum(weight > 0 for weight in current) > 3:
        active = [i for i, weight in enumerate(current) if weight > 0]
        rows = [[F(1) for _ in active],
                [F(sites[i][1]) - F(a[1]) for i in active],
                [F(sites[i][2]) - F(a[2]) for i in active]]
        delta = null_vector(rows)
        edge = subtract(b, a)
        costs = []
        for i in active:
            lam = dot(subtract(sites[i], a), edge) / dot(edge, edge)
            costs.append(dot(sites[i], sites[i]) - (1 - lam) * dot(a, a) - lam * dot(b, b))
        derivative = sum((step * cost for step, cost in zip(delta, costs, strict=True)), F(0))
        if derivative > 0:
            delta = [-value for value in delta]
        scale = min(current[i] / -change for i, change in zip(active, delta, strict=True)
                    if change < 0)
        for i, change in zip(active, delta, strict=True):
            current[i] += scale * change
        next_gap = group_gap(a, b, sites, current)
        require(next_gap <= gap and sum(current[i] > 0 for i in active) < len(active),
                "compression did not preserve and shorten certificate")
        gap = next_gap
        steps += 1
    return current, steps


def run() -> dict[str, object]:
    a, b = (0, 17, 10), (20, 17, 10)
    sites: list[Point] = [(10, 26, 10), (10, 21, 18), (10, 10, 16),
                         (10, 10, 4), (10, 21, 2)]
    groups = [(0, 1, 3), (0, 2, 3), (0, 2, 4), (1, 2, 4), (1, 3, 4)]
    convex_weights = [[F(16, 79), F(27, 79), F(36, 79)],
                      [F(7, 16), F(9, 32), F(9, 32)],
                      [F(16, 79), F(36, 79), F(27, 79)],
                      [F(2, 11), F(4, 11), F(5, 11)],
                      [F(5, 11), F(4, 11), F(2, 11)]]
    gaps = [group_gap(a, b, [sites[i] for i in group], weights)
            for group, weights in zip(groups, convex_weights, strict=True)]
    require(gaps == [F(-1384, 79), F(-67, 4), F(-1384, 79),
                     F(-200, 11), F(-200, 11)], "collective margins changed")
    degrees = [sum(i in group for group in groups) for i in range(len(sites))]
    require(degrees == [3] * 5, "incidence degree changed")
    packed = fractional_credit(groups, [F(1, 3)] * 5)
    require(packed == 2, "fractional credit did not reach two")
    integral_maximum = 0
    for mask in range(1 << len(groups)):
        selected = [group for i, group in enumerate(groups) if mask & (1 << i)]
        flat = [identity for group in selected for identity in group]
        if len(flat) == len(set(flat)):
            integral_maximum = max(integral_maximum, len(selected))
    require(integral_maximum == 1, "disjoint packing comparison changed")
    hitting_sizes = [mask.bit_count() for mask in range(1 << len(sites))
                    if all(any(mask & (1 << identity) for identity in group)
                           for group in groups)]
    require(min(hitting_sizes) == packed, "independent hitting-set oracle")

    # A positive, noncoplanar q4 support, with a-b the unique longest edge.
    c, d, center, radius2 = (10, 4, 7), (10, 4, 13), (10, 14, 10), F(109)
    support = [a, b, c, d]
    bary = [F(5, 13), F(5, 13), F(3, 26), F(3, 26)]
    require(all(power(point, center, radius2) == 0 for point in support), "support sphere")
    require(all(weight > 0 for weight in bary) and sum(bary) == 1 and
            all(sum(weight * point[axis] for weight, point
                    in zip(bary, support, strict=True)) == center[axis]
                for axis in range(3)), "positive q4 support")
    require(20 * ((4 - 17) * (13 - 10) - (7 - 10) * (4 - 17)) == -1560,
            "noncoplanar q4 determinant")
    require(all(dot(subtract(support[i], support[j]), subtract(support[i], support[j])) < 400
                for i, j in combinations(range(4), 2) if (i, j) != (0, 1)),
            "a-b is not unique maximum edge")
    powers = [power(site, center, radius2) for site in sites]
    require(powers == [35, 4, -57, -57, 4], "tight positive q4 depth")
    require(sum(value < 0 for value in powers) == packed, "fractional bound not attained")
    for site in sites:
        u, v = subtract(site, a), subtract(b, site)
        h = dot(u, v)
        xi = dot(u, u) * dot(v, v) - h * h
        require(h > 0 and 3 * h * h < xi and 2 * h * h < xi,
                "individual W3/W4 failure not exercised")

    centers_y = [F(i) for i in range(-6, 7)] + [F(-5, 2), F(-19, 18), F(15, 14)]
    sphere_checks = boundary_members = weighted_identities = 0
    seen_outside = [False] * len(sites)
    for dy in centers_y:
        for dz in range(-6, 7):
            trial_center: Point = (10, 17 + dy, 10 + dz)
            trial_radius2 = F(100) + dy * dy + dz * dz
            values = [power(site, trial_center, trial_radius2) for site in sites]
            require(sum(value < 0 for value in values) >= packed, "sampled sphere loses credit")
            for group, weights, gap in zip(groups, convex_weights, gaps, strict=True):
                require(sum(weight * values[i] for i, weight in zip(group, weights, strict=True))
                        == gap, "independent power identity failed")
                weighted_identities += 1
            for i, value in enumerate(values):
                boundary_members += int(value == 0)
                seen_outside[i] |= value > 0
            sphere_checks += 1
    require(all(seen_outside), "individual nonuniversality floor")
    require(sphere_checks == 208 and weighted_identities == 1040 and boundary_members >= 3,
            "sphere nonvacuity")

    # Mix five valid certificates to produce a genuine five-positive-ID one.
    mixture = [F(0)] * len(sites)
    for group, weights in zip(groups, convex_weights, strict=True):
        for identity, weight in zip(group, weights, strict=True):
            mixture[identity] += weight / len(groups)
    require(all(weight > 0 for weight in mixture), "compression input not full support")
    original_gap = group_gap(a, b, sites, mixture)
    reduced, steps = compress(a, b, sites, mixture)
    reduced_gap = group_gap(a, b, sites, reduced)
    require(sum(weight > 0 for weight in reduced) == 3 and steps == 2 and
            reduced_gap <= original_gap, "three-ID compression not exercised")
    # No one or two of the transverse pentagon sites can barycenter the chord.
    transverse = [(F(site[1]) - 17, F(site[2]) - 10) for site in sites]
    require(all(y != 0 or z != 0 for y, z in transverse), "one-ID certificate exists")
    require(all(p[0] * q[1] - p[1] * q[0] != 0
                for p, q in combinations(transverse, 2)), "two-ID certificate exists")

    rejected = 0
    try:
        fractional_credit(groups, [F(1, 2)] * 5)
    except RuntimeError:
        rejected += 1
    require(rejected == 1, "overloaded capacity accepted")
    # Omitting capacities would ceil(5/2)=3 despite actual depth two.
    require((5 + 2 - 1) // 2 > sum(value < 0 for value in powers), "capacity mutant inactive")
    require(len(groups) > sum(value < 0 for value in powers), "raw group count mutant inactive")
    require(5 // 3 < packed, "floor loses guaranteed integer credit")
    require(fractional_credit([(0, 1), (1, 2)], [F(1, 2)] * 2) == 1,
            "two overlapping groups received two credits")
    require(fractional_credit([(0, 1), (2, 3)], [F(1)] * 2) == 2,
            "disjoint-group special case regressed")
    return {"status": "passed", "scope": "independent_collective_model_not_v8_producer",
            "public_status": "not_claimed", "sites": sites, "groups": groups,
            "group_margins": [str(gap) for gap in gaps], "id_degrees": degrees,
            "incidences": sum(map(len, groups)), "fractional_packing_credit": packed,
            "maximum_disjoint_packing": integral_maximum, "hitting_set_minimum": min(hitting_sizes),
            "positive_q4_power_values": [str(value) for value in powers],
            "individual_w3_w4_failures": len(sites), "sphere_checks": sphere_checks,
            "weighted_power_identities": weighted_identities, "boundary_members": boundary_members,
            "compression_input_ids": 5, "compression_output_ids": 3, "compression_steps": steps,
            "compression_weights": [str(weight) for weight in reduced],
            "compression_initial_gap": str(original_gap), "compression_final_gap": str(reduced_gap),
            "refuted_unsafe_rules": ["omit_id_capacity", "raw_group_count"],
            "refuted_weakening": "floor_instead_of_ceiling_is_safe_but_loses_second_credit",
            "source_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}


def main() -> int:
    if sys.argv[1:] != ["--selftest"]:
        print("usage: collective_overlap_gate.py --selftest", file=sys.stderr)
        return 2
    try:
        result = run()
    except (OSError, RuntimeError, ValueError, ZeroDivisionError) as error:
        print(f"collective overlap gate failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
