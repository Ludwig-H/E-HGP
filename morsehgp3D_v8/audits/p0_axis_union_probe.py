#!/usr/bin/env python3
"""Independent integer model: union of exact axis columns, q2 only.

This does not execute or qualify the developer's changing C++ implementation.
The large-grid checks count range descriptors without expanding their pairs.
"""

from __future__ import annotations

from bisect import bisect_left, bisect_right
from dataclasses import dataclass
import json
import sys
from typing import Sequence


Point = tuple[int, int, int]


def require(condition: bool, cause: str) -> None:
    if not condition:
        raise RuntimeError(cause)


@dataclass
class Node:
    low: Point
    high: Point
    first: int
    last: int
    left: Node | None = None
    right: Node | None = None


class Model:
    def __init__(self, points: Sequence[Point], a_ids: Sequence[int],
                 b_ids: Sequence[int]) -> None:
        require(len(set(points)) == len(points), "duplicate coordinates")
        require(bool(a_ids) and bool(b_ids), "empty factor")
        require(len(set(a_ids)) == len(a_ids), "duplicate A IDs")
        require(len(set(b_ids)) == len(b_ids), "duplicate B IDs")
        require(set(a_ids).isdisjoint(b_ids), "overlapping factor IDs")
        self.points = points
        self.a_ids = tuple(a_ids)
        self.columns: dict[tuple[int, int], tuple[tuple[int, ...], int]] = {}
        for axis in range(3):
            others = tuple(j for j in range(3) if j != axis)
            groups: dict[tuple[int, int], list[int]] = {}
            for aid in a_ids:
                key = tuple(points[aid][j] for j in others)
                groups.setdefault(key, []).append(aid)
            for ids in groups.values():
                ids.sort(key=lambda aid: points[aid][axis])
                coordinates = tuple(points[aid][axis] for aid in ids)
                for rank, aid in enumerate(ids):
                    self.columns[aid, axis] = coordinates, rank
        self.order: list[int] = []
        self.tree_points = 0
        self.tree_nodes = 0
        self.root = self.build(list(b_ids))

    def build(self, ids: list[int]) -> Node:
        self.tree_nodes += 1
        self.tree_points += len(ids)
        low = tuple(min(self.points[i][j] for i in ids) for j in range(3))
        high = tuple(max(self.points[i][j] for i in ids) for j in range(3))
        first = len(self.order)
        if len(ids) == 1:
            self.order.extend(ids)
            return Node(low, high, first, len(self.order))
        axis = max(range(3), key=lambda j: high[j] - low[j])
        middle = (low[axis] + high[axis]) // 2
        left_ids = [i for i in ids if self.points[i][axis] <= middle]
        right_ids = [i for i in ids if self.points[i][axis] > middle]
        self.tree_points += len(ids)
        require(bool(left_ids) and bool(right_ids), "midpoint split failed")
        left, right = self.build(left_ids), self.build(right_ids)
        return Node(low, high, first, len(self.order), left, right)

    def count(self, aid: int, axis: int, coordinate: int) -> int:
        values, rank = self.columns[aid, axis]
        origin = self.points[aid][axis]
        if coordinate > origin:
            return bisect_left(values, coordinate) - rank - 1
        if coordinate < origin:
            return rank - bisect_right(values, coordinate)
        return 0

    def bounds(self, aid: int, low: Point, high: Point) -> tuple[int, int]:
        lower = upper = 0
        for axis in range(3):
            left = self.count(aid, axis, low[axis])
            right = self.count(aid, axis, high[axis])
            origin = self.points[aid][axis]
            lower += 0 if low[axis] <= origin <= high[axis] else min(left, right)
            upper += max(left, right)
        return lower, upper

    def direct(self, aid: int, bid: int) -> int:
        return sum(self.count(aid, axis, self.points[bid][axis]) for axis in range(3))

    def query(self, need: int) -> tuple[list[tuple[int, int, int]], dict[str, int]]:
        blocks: list[tuple[int, int, int]] = []
        work = {"query_nodes": 0, "rejected_nodes": 0, "emitted_blocks": 0,
                "candidate_pairs": 0}
        if need == 0:
            return blocks, work

        def visit(aid: int, node: Node) -> None:
            work["query_nodes"] += 1
            lower, upper = self.bounds(aid, node.low, node.high)
            if lower >= need:
                work["rejected_nodes"] += 1
                return
            if upper < need:
                blocks.append((aid, node.first, node.last))
                work["emitted_blocks"] += 1
                work["candidate_pairs"] += node.last - node.first
                return
            require(node.left is not None and node.right is not None,
                    "singleton interval bounds differ")
            visit(aid, node.left)
            visit(aid, node.right)

        for aid in self.a_ids:
            visit(aid, self.root)
        return blocks, work


def h_value(a: Point, b: Point, z: Point) -> int:
    return sum((z[j] - a[j]) * (b[j] - z[j]) for j in range(3))


def grid(ny: int, nz: int) -> tuple[list[Point], list[int], list[int]]:
    count = ny * nz
    points = [(x, 1000 + y, 1000 + z) for x in (1000, 60000)
              for z in range(nz) for y in range(ny)]
    return points, list(range(count)), list(range(count, 2 * count))


def small_checks() -> dict[str, int]:
    stats = {"plans": 0, "census_pairs": 0, "rejected_pairs": 0,
             "retained_pairs": 0, "cross_axis_improvements": 0,
             "interval_samples": 0, "mutants": 0}
    fixtures = [grid(3, 4), grid(5, 4),
                ([(1000, 1000, 1000), (1000, 1001, 1000),
                  (1000, 1000, 1001), (60000, 1002, 1002)], [0, 1, 2], [3]),
                ([(1000, 0, 0), (1000, 2, 0), (1000, 100, 0),
                  (60000, 99, 0), (60000, 101, 0)], [0, 1, 2], [3, 4])]
    for points, aids, bids in fixtures:
        for permutation in ((0, 1, 2), (2, 0, 1)):
            rotated = [tuple(p[j] for j in permutation) for p in points]
            for reverse in (False, True):
                aa = aids[::-1] if reverse else aids
                bb = bids[::-1] if reverse else bids
                model = Model(rotated, aa, bb)
                for need in (0, 1, 2, 5, 10):
                    blocks, work = model.query(need)
                    expanded = [(aid, model.order[i]) for aid, first, last in blocks
                                for i in range(first, last)]
                    require(len(expanded) == len(set(expanded)), "duplicate pair emission")
                    require(len(expanded) == work["candidate_pairs"], "wrong descriptor mass")
                    expected = {(a, b) for a in aa for b in bb if model.direct(a, b) < need}
                    require(set(expanded) == expected, "query differs from exact axial sum")
                    for a in aa:
                        for b in bb:
                            axes: list[set[int]] = []
                            for axis in range(3):
                                witnesses = {z for z in aa if z != a and
                                             all(rotated[z][j] == rotated[a][j]
                                                 for j in range(3) if j != axis) and
                                             h_value(rotated[a], rotated[b], rotated[z]) > 0}
                                require(len(witnesses) == model.count(a, axis, rotated[b][axis]),
                                        "rank count differs from strict geometric witnesses")
                                axes.append(witnesses)
                            require(sum(map(len, axes)) == len(set().union(*axes)),
                                    "different exact columns share a witness")
                            depth = sum(h_value(rotated[a], rotated[b], z) > 0 for z in rotated)
                            stats["census_pairs"] += 1
                            if (a, b) not in expected:
                                require(depth >= need, "rejection loses a shallow q2 pair")
                                stats["rejected_pairs"] += 1
                            else:
                                stats["retained_pairs"] += 1
                            if max(map(len, axes)) < need <= sum(map(len, axes)):
                                stats["cross_axis_improvements"] += 1
                    stats["plans"] += 1

    # Enumerate every integer interval and all its contents around a column.
    interval_points = [(1000, t, 0) for t in (0, 2, 4, 7)] + [(60000, 0, 0)]
    interval_model = Model(interval_points, [0, 1, 2, 3], [4])
    for aid in range(4):
        for low in range(9):
            for high in range(low, 9):
                lower, upper = interval_model.bounds(aid, (60000, low, 0), (60000, high, 0))
                observed = [interval_model.count(aid, 1, t) for t in range(low, high + 1)]
                require((lower, upper) == (min(observed), max(observed)),
                        "interval extrema incorrect at strict boundary")
                stats["interval_samples"] += len(observed)

    fixture = fixtures[2]
    model = Model(*fixture)
    axes = [model.count(0, axis, fixture[0][3][axis]) for axis in range(3)]
    require(axes == [0, 1, 1] and max(axes) < 2 <= sum(axes), "cross-axis fixture vacuous")
    require(h_value(fixture[0][0], fixture[0][3], fixture[0][1]) == 1 and
            h_value(fixture[0][0], fixture[0][3], fixture[0][2]) == 1,
            "cross-axis witnesses are not strict")
    stats["mutants"] += 1  # Keeping only the maximum loses this improvement.

    boundary = [(1000, 0, 0), (1000, 1, 0), (60000, 1, 0)]
    bm = Model(boundary, [0, 1], [2])
    require(bm.direct(0, 2) == 0 and h_value(boundary[0], boundary[2], boundary[1]) == 0,
            "boundary unexpectedly strict")
    closed_count = sum(boundary[0][1] < boundary[z][1] <= boundary[2][1] for z in (0, 1))
    boundary_depth = sum(h_value(boundary[0], boundary[2], z) > 0 for z in boundary)
    require(closed_count == 1 and boundary_depth == 0,
            "closed-interval mutant does not falsely reject a shallow pair")
    stats["mutants"] += 1

    approximate = [(1000, 0, 0), (1000, 1, 1), (60000, 2, 0)]
    am = Model(approximate, [0, 1], [2])
    require(am.direct(0, 2) == 0 and h_value(approximate[0], approximate[2], approximate[1]) == 0,
            "approximate-column mutant not refuted")
    thick_count = sum(approximate[z][0] == approximate[0][0] and
                      approximate[0][1] < approximate[z][1] < approximate[2][1]
                      for z in (0, 1))
    approximate_depth = sum(h_value(approximate[0], approximate[2], z) > 0 for z in approximate)
    require(thick_count == 1 and approximate_depth == 0,
            "thick-column mutant does not falsely reject a shallow pair")
    stats["mutants"] += 1

    bounds_points = [(1000, 1, 0), (1000, 2, 0), (60000, 2, 0), (60000, 3, 0)]
    mm = Model(bounds_points, [0, 1], [2, 3])
    require(mm.bounds(0, mm.root.low, mm.root.high) == (0, 1) and
            mm.direct(0, 2) == 0 and mm.direct(0, 3) == 1,
            "max-for-rejection mutant not refuted")
    edge_depth = sum(h_value(bounds_points[0], bounds_points[2], z) > 0 for z in bounds_points)
    require(mm.bounds(0, mm.root.low, mm.root.high)[1] >= 1 and edge_depth == 0,
            "interval-maximum mutant does not falsely reject a shallow pair")
    stats["mutants"] += 1
    require(stats["plans"] == 80 and stats["census_pairs"] > 10000 and
            stats["rejected_pairs"] > 1000 and stats["retained_pairs"] > 1000 and
            stats["cross_axis_improvements"] > 0 and stats["mutants"] == 4,
            "small gate vacuous")
    return stats


def grid_checks() -> list[dict[str, int]]:
    need = 10
    offsets = [(dy, dz) for dy in range(-need, need + 1)
               for dz in range(-need, need + 1)
               if max(0, abs(dy) - 1) + max(0, abs(dz) - 1) < need]
    require(len(offsets) == 261, "wrong complete-grid offset count")
    require(sum(abs(dy) for dy, _ in offsets) == 990 and
            sum(abs(dz) for _, dz in offsets) == 990 and
            sum(abs(dy * dz) for dy, dz in offsets) == 2860,
            "wrong complete-grid moment sums")
    results = []
    for ny, nz in ((50, 80), (80, 100), (100, 160)):
        expected = sum((ny - abs(dy)) * (nz - abs(dz)) for dy, dz in offsets)
        require(expected == 261 * ny * nz - 990 * (ny + nz) + 2860,
                "closed axial-union count differs from offset summation")
        isolated = ((2 * need + 1) * ny - need * (need + 1)) * (
            (2 * need + 1) * nz - need * (need + 1))
        points, aids, bids = grid(ny, nz)
        model = Model(points, aids, bids)
        blocks, work = model.query(need)
        require(work["candidate_pairs"] == expected and
                sum(last - first for _, first, last in blocks) == expected,
                "full-grid descriptors differ from finite offset summation")
        require(expected <= 261 * len(aids) and work["query_nodes"] > 0 and
                work["rejected_nodes"] > 0 and work["emitted_blocks"] > 0,
                "full-grid query non-vacuity failed")
        for sample in range(128):
            a, b = aids[(sample * 137) % len(aids)], bids[(sample * 271 + 7) % len(bids)]
            dy, dz = abs(points[a][1] - points[b][1]), abs(points[a][2] - points[b][2])
            require(model.direct(a, b) == max(0, dy - 1) + max(0, dz - 1),
                    "full-grid sample rank formula differs")
        results.append({"n": len(points), "ny": ny, "nz": nz, "need": need,
                        "offsets": len(offsets), "total_pairs": len(aids) * len(bids),
                        "isolated_axis_model_candidates": isolated,
                        "tree_nodes": model.tree_nodes, "tree_point_visits": model.tree_points,
                        **work})
    return results


def main() -> int:
    if sys.argv[1:] != ["--selftest"]:
        print("usage: p0_axis_union_probe.py --selftest", file=sys.stderr)
        return 2
    result = {"scope": "independent integer model; q2 exact-column union; no C++ qualification, "
                        "large census, timing, WSPD, FULL or tower claim",
              "countermodels": ["maximum_only_loses_a_valid_improvement",
                                "closed_boundary_can_overcredit",
                                "approximate_column_can_overcredit",
                                "interval_maximum_cannot_reject_whole_node"],
              "small": small_checks(), "complete_grids": grid_checks()}
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
