#!/usr/bin/env python3
"""Minimal full-dimensional fixture for premature B refinement in shared q2.

This bounded Python mirror follows the pinned C++ midpoint/left-first tree
and split decision; it does not execute the C++ binary or measure time. An
exact reflection rebuilds the index: no live continuation is reordered.
The example concerns isolated census / a Pure-front rectangle. The current
MidpointSamples front already finds the common witness in this tiny case.
The supplied seed-node extension is a proposed count/exclusion protocol,
not the current API; its proposal search cost and payload collection are
not measured. The seed is immutable and inherited separately from Z's cursor.
"""

from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import sys

AUDITS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(AUDITS))
from p0_q2_census_bounds_probe import box_bounds, power_value, require, singleton


REFERENCE_PINS = {
    "morsehgp3D_v8/src/pipeline/q2_census.cpp":
        "f46faf68812ac7082c8d3d74a995f9f5ae928735a8c3653f1ef1bb9269b39919",
    "morsehgp3D_v8/src/spindle/q2_prepared_bounds.hpp":
        "7bb46b4b7af3beede9bc2fc8926bafda9c900eb573671583206a6d94ffac5d21",
    "morsehgp3D_v8/audits/p0_q2_census_bounds_probe.py":
        "c33510de452250b83a67f3eddb9e6fe1ba9319e3e5510cd6f338c3f046de6ceb",
}
POINTS = ((0, 0, 0), (100, 0, 0), (102, 1, 0), (50, 2, 1))


@dataclass
class Node:
    ids: tuple
    box: tuple
    first: int
    last: int
    children: tuple = ()
    escape: int = 0


def midpoint_tree(points: tuple) -> tuple[list[Node], tuple]:
    nodes, order = [], []

    def build(ids: tuple, first: int) -> int:
        box = tuple((min(points[i][k] for i in ids), max(points[i][k] for i in ids))
                    for k in range(3))
        node_id = len(nodes)
        nodes.append(Node(ids, box, first, first + len(ids)))
        if len(ids) > 1:
            # First maximal axis, integer midpoint, <= on the left as in C++.
            axis = max(range(3), key=lambda k: box[k][1] - box[k][0])
            middle = sum(box[axis]) // 2
            left = tuple(i for i in ids if points[i][axis] <= middle)
            right = tuple(i for i in ids if points[i][axis] > middle)
            require(bool(left) and bool(right), "midpoint failed to split")
            nodes[node_id].children = (build(left, first), build(right, first + len(left)))
        else:
            order.extend(ids)
        nodes[node_id].escape = len(nodes)
        return node_id

    build(tuple(range(len(points))), 0)
    require(len(nodes) == 2 * len(points) - 1, "tree size mismatch")
    for i, node in enumerate(nodes):
        require(node.escape == i + 2 * len(node.ids) - 1, "invalid preorder escape")
        require(set(order[node.first:node.last]) == set(node.ids), "range/order mismatch")
        if node.children:
            left, right = node.children
            require(left == i + 1 and nodes[left].escape == right
                    and nodes[right].escape == node.escape, "child continuation mismatch")
    return nodes, tuple(order)


def determinant(points: tuple) -> int:
    u, v, w = (tuple(x - a for x, a in zip(p, points[0])) for p in points[1:])
    return (u[0] * (v[1] * w[2] - v[2] * w[1])
            - u[1] * (v[0] * w[2] - v[2] * w[0])
            + u[2] * (v[0] * w[1] - v[1] * w[0]))


def mirror(points: tuple, kmax: int = 1, seed: bool = False, mutant: bool = False) -> dict:
    nodes, order = midpoint_tree(points)
    b_root = next(i for i, node in enumerate(nodes) if set(node.ids) == {1, 2})
    work = {key: 0 for key in (
        "tasks", "visits", "bound_tests", "point_tests", "query_splits",
        "witness_splits", "uniform_rejected_pairs",
    )}
    rejected, split_events, accepted = [], [], {}
    seed_node = next(i for i, node in enumerate(nodes) if node.ids == (3,)) if seed else None
    seed_count = len(nodes[seed_node].ids) if seed else 0
    seed_handoffs = 0
    exclusion_checks = skipped_seed_nodes = 0
    if seed:
        lower, _ = box_bounds(singleton(points[0]), nodes[b_root].box, nodes[seed_node].box)
        require(lower > 0, "proposed seed block is not universally interior")

    # Independently reproduce the existing one-path midpoint proposal for
    # this rectangle only, so the declared Samples limitation is checked.
    center4 = tuple(2 * points[0][k] + sum(nodes[b_root].box[k]) for k in range(3))
    def midpoint_distance(node: Node) -> int:
        return sum(max(0, 4 * low - c, c - 4 * high) ** 2
                   for (low, high), c in zip(node.box, center4))
    proposed = 0
    while nodes[proposed].children:
        proposed = min(nodes[proposed].children, key=lambda i: midpoint_distance(nodes[i]))
    require(nodes[proposed].ids == (3,), "tiny fixture no longer proposed by midpoint search")

    def diagonal(node: Node) -> int:
        return sum((hi - lo) ** 2 for lo, hi in node.box)

    def walk(query: int, count: int, cursor: int, excluded: int | None) -> None:
        nonlocal seed_handoffs, exclusion_checks, skipped_seed_nodes
        work["tasks"] += 1
        b = nodes[query]
        if count == kmax:
            rejected.extend(b.ids)
            work["uniform_rejected_pairs"] += len(b.ids) if len(b.ids) > 1 else 0
            return
        while cursor != len(nodes):
            z = nodes[cursor]
            # E is a single supplied, certified node. Its ranks are excluded
            # until the ordinary DFS prefix covers E, when the old count-only
            # continuation becomes valid again. Each child owns this value.
            exclusion_checks += int(excluded is not None)
            if excluded is not None and z.first >= nodes[excluded].last:
                excluded = None
                seed_handoffs += 1
            overlap = (max(0, min(z.last, nodes[excluded].last)
                           - max(z.first, nodes[excluded].first))
                       if excluded is not None and not mutant else 0)
            if overlap == len(z.ids):
                skipped_seed_nodes += 1
                cursor = z.escape
                continue
            work["visits"] += 1
            work["point_tests" if len(b.ids) == len(z.ids) == 1 else "bound_tests"] += 1
            low, high4 = box_bounds(singleton(points[0]), b.box, z.box)
            # Independent rational-center evaluations verify every represented
            # discrete triple, in addition to the imported continuous bounds.
            values = [power_value(points[0], points[j], points[k]) for j in b.ids for k in z.ids]
            require(all(low <= value and 4 * value <= high4 for value in values), "unsafe box bound")
            if low > 0:
                count = min(kmax, count + len(z.ids) - overlap)
                cursor = z.escape
                if count == kmax:
                    rejected.extend(b.ids)
                    work["uniform_rejected_pairs"] += len(b.ids) if len(b.ids) > 1 else 0
                    return
            elif high4 <= 0:
                cursor = z.escape
            elif len(b.ids) == 1 or (z.children and diagonal(z) > diagonal(b)):
                require(bool(z.children), "uncertain singleton triple")
                work["witness_splits"] += 1
                cursor = z.children[0]
            else:
                require(bool(b.children), "query cannot split")
                work["query_splits"] += 1
                split_events.append({"b_ids": b.ids, "z_ids": z.ids, "count": count,
                                     "b_diagonal2": diagonal(b), "z_diagonal2": diagonal(z)})
                for child in b.children:
                    walk(child, count, cursor, excluded)
                return
        for site in b.ids:
            require(site not in accepted, "support accepted twice")
            accepted[site] = count

    walk(b_root, min(kmax, seed_count), 0, seed_node)
    depths = [sum(power_value(points[0], points[j], p) > 0 for p in points) for j in (1, 2)]
    common = [int(power_value(points[0], points[j], points[3])) for j in (1, 2)]
    require(len(rejected) == len(set(rejected)) and not set(rejected).intersection(accepted),
            "support duplicated or contradictory")
    require(set(rejected).union(accepted) == {1, 2}, "support coverage mismatch")
    require(depths == [1, 2], "strict oracle depths changed")
    for site, depth in zip((1, 2), depths):
        require((site in rejected and depth >= kmax)
                or (site in accepted and accepted[site] == depth < kmax), "oracle/rejection mismatch")
    require(common == [2495, 2597], "common witness mismatch")
    return {"points": points, "spatial_order": order, "determinant": determinant(points),
            "strict_depths": depths, "common_witness_powers": common,
            "work": work, "split_events": split_events,
            "accepted_depths": accepted, "rejected_support_b_ids": sorted(rejected),
            "seed_bound_tests": int(seed), "exclusion_checks": exclusion_checks,
            "skipped_seed_nodes": skipped_seed_nodes,
            "seed_handoffs_to_ordinary_prefix": seed_handoffs}


def selftest() -> dict:
    original = mirror(POINTS)
    reflected_points = tuple((102 - x, y, z) for x, y, z in POINTS)
    reflected = mirror(reflected_points)
    require(original["determinant"] == 100 and reflected["determinant"] == -100,
            "fixture is not full-dimensional")
    for i in range(4):
        for j in range(i):
            before = sum((a - b) ** 2 for a, b in zip(POINTS[i], POINTS[j]))
            after = sum((a - b) ** 2 for a, b in zip(reflected_points[i], reflected_points[j]))
            require(before == after, "reflection changed geometry")
    expected = (
        {"tasks": 1, "visits": 4, "bound_tests": 4, "point_tests": 0,
         "query_splits": 0, "witness_splits": 2, "uniform_rejected_pairs": 2},
        {"tasks": 3, "visits": 8, "bound_tests": 5, "point_tests": 3,
         "query_splits": 1, "witness_splits": 3, "uniform_rejected_pairs": 0},
    )
    require(original["work"] == expected[0] and reflected["work"] == expected[1],
            "expected premature-refinement cost changed")
    event, = reflected["split_events"]
    require(set(event["z_ids"]) == {1, 2} and event["count"] == 0
            and event["b_diagonal2"] == event["z_diagonal2"] == 5, "tie not exercised")
    seed_rows = []
    mutant_hits = []
    for label, points in (("original", POINTS), ("reflected", reflected_points)):
        for kmax in (1, 2, 3):
            row = mirror(points, kmax, seed=True)
            unseeded = mirror(points, kmax)
            require(row["accepted_depths"] == unseeded["accepted_depths"]
                    and row["rejected_support_b_ids"] == unseeded["rejected_support_b_ids"],
                    "seed/exclusion changed exact decisions")
            if kmax == 1:
                require(row["work"]["visits"] == 0 and row["work"]["query_splits"] == 0,
                        "saturating seed failed to reject immediately")
            seed_rows.append({"orientation": label, "kmax": kmax, "work": row["work"],
                              "seed_bound_tests": row["seed_bound_tests"],
                              "exclusion_checks": row["exclusion_checks"],
                              "skipped_seed_nodes": row["skipped_seed_nodes"],
                              "prefix_handoffs": row["seed_handoffs_to_ordinary_prefix"]})
        try:
            mirror(points, 2, seed=True, mutant=True)
        except RuntimeError as error:
            mutant_hits.append({"orientation": label, "rejection": str(error)})
        else:
            raise RuntimeError("seed without rank exclusion was accepted")
    require(any(row["prefix_handoffs"] for row in seed_rows), "prefix handoff not exercised")
    repo = AUDITS.parent.parent
    current_pins = {path: hashlib.sha256((repo / path).read_bytes()).hexdigest()
                    for path in REFERENCE_PINS}
    require(current_pins["morsehgp3D_v8/audits/p0_q2_census_bounds_probe.py"]
            == REFERENCE_PINS["morsehgp3D_v8/audits/p0_q2_census_bounds_probe.py"],
            "imported bound model changed: re-audit required")
    return {"status": "passed", "scope": "bounded_python_mirror_cost_fixture",
            "kmax": 1, "support_anchor_id": 0, "b_ids": [1, 2], "common_witness_id": 3,
            "orientations": {"original": original, "reflected": reflected},
            "supplied_seed_block_runs": seed_rows,
            "seed_without_exclusion_mutant": mutant_hits,
            "seed_proposal_cost_measured": False,
            "reference_source_sha256": REFERENCE_PINS,
            "current_sources_match_reference": current_pins == REFERENCE_PINS,
            "cpp_binary_executed": False, "timing_measured": False,
            "midpoint_samples_finds_this_witness": True}


if __name__ == "__main__":
    print(json.dumps(selftest(), sort_keys=True))
