#!/usr/bin/env python3
"""Exact bounded model of q2 census reuse; no production index or benchmark."""
from __future__ import annotations

from dataclasses import dataclass, field
import hashlib
import itertools
import json
from pathlib import Path
import sys

Point = tuple[int, int, int]
Pair = tuple[int, int]
Geometry = tuple[Point, int]


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def key(a: Point, b: Point) -> Geometry:
    center = (a[0] + b[0], a[1] + b[1], a[2] + b[2])
    return center, sum((a[d] - b[d]) ** 2 for d in range(3))


def power_four(geometry: Geometry, point: Point) -> int:
    center, radius = geometry
    return sum((2 * point[d] - center[d]) ** 2 for d in range(3)) - radius


def direct(points: tuple[Point, ...], pair: Pair) -> tuple[list[int], list[int]]:
    a, b = (points[i] for i in pair)
    values = [sum((z[d] - a[d]) * (b[d] - z[d]) for d in range(3)) for z in points]
    return [i for i, h in enumerate(values) if h > 0], [i for i, h in enumerate(values) if h == 0]


@dataclass
class State:
    points: tuple[Point, ...]
    geometry: Geometry
    pending: list[list[int]]
    count: int = 0
    interior: list[int] = field(default_factory=list)
    shell: list[int] | None = None
    examined: int = 0
    shell_passes: int = 0
    saturated_before: bool = False

    def query(self, threshold: int, mutant: str | None) -> tuple[bool, list[int] | None, list[int] | None]:
        if mutant == 'rejected_for_all_thresholds' and self.saturated_before:
            return False, None, None
        while self.pending and self.count < threshold:
            block = self.pending.pop(0)
            found = [i for i in block if power_four(self.geometry, self.points[i]) < 0]
            self.examined += len(block)
            if mutant == 'clip_consumed_block_then_resume':
                self.count += min(len(found), threshold - self.count)
            else:
                self.count += len(found)
            self.interior.extend(found)
        if self.count >= threshold:
            self.saturated_before = True
            return False, None, None
        require(not self.pending, 'a surviving depth must be complete')
        if self.shell is None:
            self.shell = [i for i, z in enumerate(self.points) if power_four(self.geometry, z) == 0]
            self.shell_passes += 1
        return True, sorted(self.interior), self.shell


class Cache:
    def __init__(self, mutant: str | None) -> None:
        self.mutant = mutant
        self.entries: dict[object, State] = {}

    def query(self, cloud_id: str, points: tuple[Point, ...], pair: Pair,
              threshold: int) -> tuple[bool, list[int] | None, list[int] | None]:
        geometry = key(points[pair[0]], points[pair[1]])
        cache_key: object = (cloud_id, geometry)
        if self.mutant == 'omit_radius':
            cache_key = (cloud_id, geometry[0])
        elif self.mutant == 'omit_cloud_identity':
            cache_key = geometry
        if cache_key not in self.entries:
            # {4,5} is a certified interior box for the outer ball in X.
            # All other IDs are singletons; this is a fixed partition, no index.
            blocks = [[4, 5]] + [[i] for i in range(len(points)) if i not in (4, 5)]
            self.entries[cache_key] = State(points, geometry, blocks)
        return self.entries[cache_key].query(threshold, self.mutant)


def check_query(cache: Cache, cloud_id: str, points: tuple[Point, ...], pair: Pair,
                threshold: int) -> None:
    inside, shell = direct(points, pair)
    expected = (True, inside, shell) if len(inside) < threshold else (False, None, None)
    require(cache.query(cloud_id, points, pair, threshold) == expected,
            f'census reuse differs: cloud={cloud_id}, pair={pair}, threshold={threshold}')


def run(mutant: str | None) -> dict[str, object]:
    points: tuple[Point, ...] = ((0, 1, 1), (3, 2, 2), (1, 0, 1), (2, 3, 2),
                               (1, 1, 1), (2, 2, 2), (1, 1, 0), (1, 1, 4))
    outer = key(points[0], points[1])
    inner = key(points[4], points[5])
    require(outer == ((3, 3, 3), 11) and inner == ((3, 3, 3), 3), 'fixture keys')
    inside, shell = direct(points, (0, 1))
    require(inside == [4, 5] and shell == [0, 1, 2, 3, 6], 'fixture census')
    require(direct(points, (2, 3)) == (inside, shell), 'second diameter same census')
    for corner in itertools.product((1, 2), repeat=3):
        require(power_four(outer, corner) < 0, 'two-interior block needs a true box certificate')
    lookup = {points[i]: i for i in shell}
    supports = []
    for i in shell:
        antipode = tuple(outer[0][d] - points[i][d] for d in range(3))
        j = lookup.get(antipode)
        if j is not None and i < j:
            supports.append((i, j))
    if mutant == 'all_shell_pairs_are_diameters':
        supports = list(itertools.combinations(shell, 2))
    require(supports == [(0, 1), (2, 3)], 'antipodal support matching')
    require(2 * len(supports) <= len(shell) and 6 not in [i for pair in supports for i in pair],
            'unmatched shell site and support cardinal bound')
    query_count = 0
    permutations = list(itertools.permutations((1, 2, 3)))
    for order in permutations:
        cache = Cache(mutant)
        for threshold, pair in zip(order, ((0, 1), (2, 3), (1, 0))):
            check_query(cache, 'X', points, pair, threshold)
            query_count += 1
        check_query(cache, 'X', points, (2, 3), 3)
        query_count += 1
        require(len(cache.entries) == 1, 'same ball must share its state across supports and thresholds')
        state = next(iter(cache.entries.values()))
        require(state.examined == len(points) and state.shell_passes == 1 and state.count == 2,
                'resume must consume each ID once and collect the shell once')
        check_query(cache, 'X', points, (4, 5), 1)
        query_count += 1
        other = points[:5] + ((10, 10, 10),) + points[6:]
        check_query(cache, 'Y', other, (0, 1), 2)
        query_count += 1
        reindexed = (points[4],) + points[1:4] + (points[0],) + points[5:]
        check_query(cache, 'X_reindexed', reindexed, (4, 1), 3)
        query_count += 1
        require(len(cache.entries) == 4, 'radius/cloud/ID space must qualify cached state')
    require(query_count == 42, 'query nonvacuity')
    return {'status': 'passed', 'scope': 'bounded_reuse_model_no_production_index_or_full',
            'queries': query_count, 'threshold_orders': len(permutations),
            'fixture': {'points': points, 'outer_key': outer, 'inner_key': inner,
                        'outer_interior_ids': inside, 'outer_shell_ids': shell,
                        'diameter_support_pairs': supports, 'unmatched_shell_id': 6,
                        'certified_interior_block': [4, 5],
                        'outer_decisions': {'Kmax1': 'reject', 'Kmax2': 'reject', 'Kmax3': 'keep'}},
            'mutants': ['clip_consumed_block_then_resume', 'rejected_for_all_thresholds',
                        'omit_radius', 'omit_cloud_identity', 'all_shell_pairs_are_diameters']}


def main() -> int:
    modes = ['clip_consumed_block_then_resume', 'rejected_for_all_thresholds',
             'omit_radius', 'omit_cloud_identity', 'all_shell_pairs_are_diameters']
    args = sys.argv[1:]
    mutant = None
    if len(args) == 2 and args[0] == '--mutant' and args[1] in modes:
        mutant = args[1]
    elif args != ['--selftest']:
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
