#!/usr/bin/env python3
"""Small exact fixtures for a future q2 consumer, independent of its index."""
from __future__ import annotations

import hashlib
import itertools
import json
from pathlib import Path
import sys

Point = tuple[int, int, int]
Box = tuple[Point, Point]
Ball = tuple[Point, int]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def h(a: Point, b: Point, z: Point) -> int:
    return sum((zi - ai) * (bi - zi) for ai, bi, zi in zip(a, b, z))


def ball(a: Point, b: Point) -> Ball:
    center_twice = tuple(ai + bi for ai, bi in zip(a, b))
    radius_four = sum((ai - bi) ** 2 for ai, bi in zip(a, b))
    return center_twice, radius_four  # type: ignore[return-value]


def distance_four(center_twice: Point, z: Point) -> int:
    return sum((2 * zi - ci) ** 2 for zi, ci in zip(z, center_twice))


def bounds_h_four(a: Point, b: Point, box: Box) -> tuple[int, int]:
    center, radius_four = ball(a, b)
    low, high = box
    minimum_distance = 0
    maximum_distance = 0
    for ci, lo, hi in zip(center, low, high):
        nearest = min(max(ci, 2 * lo), 2 * hi)
        minimum_distance += (nearest - ci) ** 2
        maximum_distance += max((2 * lo - ci) ** 2, (2 * hi - ci) ** 2)
    return radius_four - maximum_distance, radius_four - minimum_distance


def corners(box: Box) -> list[Point]:
    low, high = box
    return [tuple(high[d] if bits[d] else low[d] for d in range(3))
            for bits in itertools.product((0, 1), repeat=3)]  # type: ignore[misc]


def transform(z: Point, permutation: tuple[int, ...], reflection: int) -> Point:
    return tuple(65535 - z[d] if reflection & (1 << i) else z[d]
                 for i, d in enumerate(permutation))  # type: ignore[return-value]


def run(mutant: str | None) -> dict[str, object]:
    points: list[Point] = [(0, 1, 1), (3, 2, 2), (1, 0, 1),
                          (2, 3, 2), (1, 1, 1), (1, 1, 4)]
    supports = [(0, 1), (2, 3)]
    center, radius_four = ball(points[0], points[1])
    require((center, radius_four) == ((3, 3, 3), 11), 'fractional-center key')
    require(ball(points[2], points[3]) == (center, radius_four), 'shared ball')
    geometries = {ball(points[a], points[b]) for a, b in supports}
    without_z = [i for i in range(len(points)) if i != 4]
    require(not any(h(points[0], points[1], points[i]) > 0 for i in without_z),
            'removing z leaves an empty open ball')
    require([i for i in without_z if h(points[0], points[1], points[i]) == 0] == [0, 1, 2, 3],
            'removing z preserves the four-site shell')
    if mutant == 'pair_ids_count_distinct_balls':
        require(len(set(supports)) == len(geometries), 'two pair IDs represent only one ball')
    queries = 0
    tested_sites = 0
    threshold_decisions = 0
    for permutation in itertools.permutations(range(3)):
        for reflection in range(8):
            cloud = [transform(z, permutation, reflection) for z in points]
            for left, right in supports:
                for a_id, b_id in [(left, right), (right, left)]:
                    a, b = cloud[a_id], cloud[b_id]
                    c2, r4 = ball(a, b)
                    signed = [h(a, b, z) for z in cloud]
                    require(signed == [0, 0, 0, 0, 2, -4], 'exact H transformed fixture')
                    for z, value in zip(cloud, signed):
                        require(4 * value == r4 - distance_four(c2, z), 'doubled-center identity')
                        tested_sites += 1
                    if mutant == 'floor_center':
                        c2 = tuple(2 * (coordinate // 2) for coordinate in c2)
                    interior = [i for i, z in enumerate(cloud) if distance_four(c2, z) < r4]
                    shell = [i for i, z in enumerate(cloud) if distance_four(c2, z) == r4]
                    require(interior == [4] and shell == [0, 1, 2, 3], 'interior/shell IDs differ')
                    for kmax in (1, 2, 3):
                        require((len(interior) < kmax) == (1 + 2 <= kmax + 1),
                                'q2 support window')
                        threshold_decisions += 1
                    queries += 1
    a, b = (0, 2, 2), (4, 2, 2)
    node_points = [(2, 0, 0), (2, 4, 4), (2, 2, 2)]
    node_box: Box = ((2, 0, 0), (2, 4, 4))
    corner_values = [h(a, b, z) for z in corners(node_box)]
    interval = bounds_h_four(a, b, node_box)
    require(corner_values == [-4] * 8 and interval == (-16, 16), 'corner fixture bounds')
    actual_positive = sum(h(a, b, z) > 0 for z in node_points)
    require(actual_positive == 1, 'corner fixture nonvacuity')
    if mutant == 'outside_from_maximum_corners':
        retained = 0 if max(corner_values) <= 0 else actual_positive
        require(retained == actual_positive, 'outside corners do not exclude interior sites')
    a, b, tangent = points[0], points[1], points[2]
    tangent_interval = bounds_h_four(a, b, (tangent, tangent))
    require(tangent_interval == (0, 0), 'tangent box exact bound')
    if mutant == 'strict_pruning_delivers_full_shell':
        retained_shell = [] if tangent_interval[1] <= 0 else [tangent]
        require(retained_shell == [tangent], 'strict-depth pruning loses a shell site')
    require(queries == 192 and tested_sites == 1152 and threshold_decisions == 576,
            'nonvacuity floor')
    return {
        'status': 'passed',
        'scope': 'exact_math_fixtures_no_production_consumer_or_full_qualification',
        'queries': queries,
        'tested_sites': tested_sites,
        'threshold_decisions': threshold_decisions,
        'transforms': 48,
        'fractional_shared_ball': {
            'points': points,
            'support_pairs': supports,
            'center_twice': center,
            'radius_squared_times_four': radius_four,
            'h_by_id': [0, 0, 0, 0, 2, -4],
            'interior_ids': [4],
            'shell_ids': [0, 1, 2, 3],
            'candidate_kmax': [2, 3],
            'rejected_kmax': [1],
            'distinct_support_pairs': 2,
            'distinct_geometric_balls': 1,
            'without_id_4': {'interior_ids': [], 'shell_ids': [0, 1, 2, 3],
                             'candidate_kmax_1': True, 'strict_gabriel_pair': False},
        },
        'outside_corners_inside_site': {
            'a': (0, 2, 2),
            'b': (4, 2, 2),
            'node_points': node_points,
            'node_box': node_box,
            'corner_h': corner_values,
            'continuous_h_times_four_bounds': interval,
            'point_h': [-4, -4, 4],
        },
        'tangent_shell': {'pair': supports[0], 'point_id': 2,
                          'h_times_four_bounds': tangent_interval},
        'mutants': ['floor_center', 'pair_ids_count_distinct_balls',
                    'outside_from_maximum_corners', 'strict_pruning_delivers_full_shell'],
    }


def main() -> int:
    allowed = ['floor_center', 'pair_ids_count_distinct_balls',
               'outside_from_maximum_corners', 'strict_pruning_delivers_full_shell']
    args = sys.argv[1:]
    if args == ['--selftest']:
        mutant = None
    elif len(args) == 2 and args[0] == '--mutant' and args[1] in allowed:
        mutant = args[1]
    else:
        print(json.dumps({'status': 'invalid_cli'}))
        return 2
    try:
        result = run(mutant)
        result['source_sha256'] = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
        print(json.dumps(result, sort_keys=True))
        return 0
    except ValueError as error:
        print(json.dumps({'status': 'rejected', 'mutant': mutant, 'reason': str(error)}, sort_keys=True))
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
