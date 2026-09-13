#!/usr/bin/env python3
"""Bounded audit of q2 box extrema and strict-depth/shell semantics.

This independent Python model checks fixed-pair and shared pair-product
continuations. It is not a production census, WSPD, or HGP FULL tower.
"""

from __future__ import annotations

from collections.abc import Iterator
from dataclasses import dataclass
from fractions import Fraction
from itertools import combinations, permutations, product
import json


Interval = tuple[int, int]
Point = tuple[int, int, int]
Box = tuple[Interval, Interval, Interval]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def interval_bounds(a: Interval, b: Interval, z: Interval) -> tuple[int, int]:
    """Return exact continuous-box minimum H and four times maximum H."""
    lower = min((zz - aa) * (bb - zz) for aa, bb, zz in product(a, b, z))
    upper4 = max(
        (bb - aa) ** 2
        - (max(2 * z[0], min(aa + bb, 2 * z[1])) - aa - bb) ** 2
        for aa, bb in product(a, b)
    )
    return lower, upper4


def box_bounds(a: Box, b: Box, z: Box) -> tuple[int, int]:
    pieces = [interval_bounds(aa, bb, zz) for aa, bb, zz in zip(a, b, z)]
    return sum(piece[0] for piece in pieces), sum(piece[1] for piece in pieces)


def power_value(a: tuple, b: tuple, z: tuple) -> Fraction:
    """Independent negative sphere power, evaluated with rational centers."""
    center = tuple((Fraction(aa) + bb) / 2 for aa, bb in zip(a, b))
    radius2 = sum((Fraction(bb) - aa) ** 2 / 4 for aa, bb in zip(a, b))
    return radius2 - sum((Fraction(zz) - cc) ** 2 for zz, cc in zip(z, center))


def oracle_bounds(a: tuple, b: tuple, z: tuple) -> tuple[Fraction, Fraction, int]:
    """Enumerate endpoint anchors and rational parabola vertices in Z."""
    values = []
    for aa, bb in product(product(*a), product(*b)):
        z_choices = []
        for low_high, left, right in zip(z, aa, bb):
            low, high = low_high
            choices = {Fraction(low), Fraction(high)}
            vertex = Fraction(left + right, 2)
            if low <= vertex <= high:
                choices.add(vertex)
            z_choices.append(tuple(sorted(choices)))
        values.extend(power_value(aa, bb, zz) for zz in product(*z_choices))
    return min(values), max(values), len(values)


def classify(lower: int, upper4: int) -> str:
    if lower > 0:
        return "interior"
    if upper4 < 0:
        return "exterior"
    if lower == 0 and upper4 == 0:
        return "shell"
    return "uncertain"


def singleton(point: Point) -> Box:
    return tuple((value, value) for value in point)


def check_extrema() -> dict:
    intervals = [(low, high) for low in range(4) for high in range(low, 4)]
    oracle_evaluations = 0
    dense_evaluations = 0
    interval_cases = 0
    for a, b, z in product(intervals, repeat=3):
        lower, upper4 = interval_bounds(a, b, z)
        expected_low, expected_high, visits = oracle_bounds((a,), (b,), (z,))
        require(lower == expected_low, f"minimum mismatch: {a}, {b}, {z}")
        require(upper4 == 4 * expected_high, f"maximum mismatch: {a}, {b}, {z}")
        oracle_evaluations += visits
        # A second, dense grid includes interior a,b values; all values are
        # scaled by 16, independently of the endpoint/vertex enumerator.
        values = [
            (zz - aa) * (bb - zz)
            for aa, bb, zz in product(
                range(4 * a[0], 4 * a[1] + 1),
                range(4 * b[0], 4 * b[1] + 1),
                range(4 * z[0], 4 * z[1] + 1),
            )
        ]
        require(min(values) == 16 * lower, "dense-grid minimum mismatch")
        require(max(values) == 4 * upper4, "dense-grid maximum mismatch")
        dense_evaluations += len(values)
        interval_cases += 1

    fixtures = [
        ("interior", ((0, 1),) * 3, ((10, 11),) * 3, ((5, 6),) * 3),
        ("exterior", ((0, 1),) * 3, ((10, 11),) * 3, ((50, 51),) * 3),
        ("shell", singleton((0, 0, 0)), singleton((2, 0, 0)), singleton((1, 1, 0))),
        ("uncertain", singleton((1, 1, 1)), singleton((3, 3, 3)), ((0, 4),) * 3),
        ("uncertain", ((0, 65535),) * 3, ((0, 65535),) * 3, ((0, 65535),) * 3),
    ]
    states = {name: 0 for name in ("interior", "exterior", "shell", "uncertain")}
    box_evaluations = 0
    for expected, a, b, z in fixtures:
        lower, upper4 = box_bounds(a, b, z)
        expected_low, expected_high, visits = oracle_bounds(a, b, z)
        require(lower == expected_low and upper4 == 4 * expected_high, "3D extrema mismatch")
        require(classify(lower, upper4) == expected, "3D decision mismatch")
        require(-(1 << 63) < 4 * lower <= upper4 < (1 << 63), "u16 i64 bound")
        states[expected] += 1
        box_evaluations += visits
    require(interval_cases == 1000 and dense_evaluations == 125000, "interval floor")
    require(all(states.values()), "decision non-vacuity")
    return {
        "interval_triples": interval_cases,
        "rational_oracle_evaluations": oracle_evaluations,
        "quarter_grid_evaluations": dense_evaluations,
        "box_3d_fixtures": len(fixtures),
        "box_3d_oracle_evaluations": box_evaluations,
        "decision_fixtures": states,
    }


@dataclass(frozen=True)
class Node:
    ids: tuple[int, ...]
    box: Box
    children: tuple[Node, ...]


def build_tree(points: tuple[Point, ...], ids: tuple[int, ...]) -> Node:
    box = tuple(
        (min(points[i][axis] for i in ids), max(points[i][axis] for i in ids))
        for axis in range(3)
    )
    if len(ids) == 1:
        return Node(ids, box, ())
    axis = max(range(3), key=lambda j: box[j][1] - box[j][0])
    order = tuple(sorted(ids, key=lambda i: (points[i][axis], i)))
    middle = len(order) // 2
    return Node(ids, box, (build_tree(points, order[:middle]), build_tree(points, order[middle:])))


def tree_census(points: tuple[Point, ...], a: int, b: int, root: Node, cap: int | None) -> dict:
    # Deliberately one fixed pair per query: this validates boundary semantics,
    # not the proposed shared traversal over boxes of endpoint pairs.
    pending = [root]
    interior = []
    shell = []
    visits = 0
    while pending:
        node = pending.pop()
        visits += 1
        decision = classify(*box_bounds(singleton(points[a]), singleton(points[b]), node.box))
        if decision == "interior":
            interior.extend(node.ids)
            if cap is not None and len(interior) >= cap:
                return {"status": "saturated", "depth": cap, "visits": visits}
        elif decision == "shell":
            shell.extend(node.ids)
        elif decision == "uncertain":
            require(bool(node.children), "singleton classification must decide")
            pending.extend(node.children)
    require(len(set(interior)) == len(interior), "duplicate interior ID")
    require(len(set(shell)) == len(shell), "duplicate shell ID")
    require(not set(interior).intersection(shell), "interior/shell overlap")
    require(a in shell and b in shell, "support endpoints lost from shell")
    return {
        "status": "complete", "depth": len(interior),
        "interior": sorted(interior), "shell": sorted(shell), "visits": visits,
    }


def check_census() -> dict:
    points = ((10, 10, 10), (18, 10, 10), (14, 10, 10), (14, 14, 10),
              (14, 6, 10), (14, 10, 14), (14, 10, 6), (30, 30, 30), (0, 0, 0))
    root = build_tree(points, tuple(range(len(points))))
    pairs = full_queries = capped_queries = saturated = complete = visits = 0
    for a, b in combinations(range(len(points)), 2):
        expected_inside = [
            i for i, z in enumerate(points)
            if power_value(points[a], points[b], z) > 0
        ]
        expected_shell = [
            i for i, z in enumerate(points)
            if power_value(points[a], points[b], z) == 0
        ]
        exact = tree_census(points, a, b, root, None)
        require(exact["status"] == "complete", "uncapped census incomplete")
        require(
            exact["interior"] == expected_inside
            and exact["shell"] == expected_shell,
            "uncapped census mismatch",
        )
        visits += exact["visits"]
        full_queries += 1
        for cap in (1, 2, 3, 5, 10):
            bounded = tree_census(points, a, b, root, cap)
            visits += bounded["visits"]
            require(bounded["depth"] == min(cap, len(expected_inside)), "capped depth mismatch")
            if len(expected_inside) >= cap:
                require(bounded["status"] == "saturated", "missing saturation")
                require(
                    "shell" not in bounded and "interior" not in bounded,
                    "incomplete IDs advertised",
                )
                saturated += 1
            else:
                require(bounded["status"] == "complete", "eligible query not complete")
                require(
                    bounded["interior"] == expected_inside
                    and bounded["shell"] == expected_shell,
                    "eligible census mismatch",
                )
                complete += 1
            capped_queries += 1
        pairs += 1
    first = tree_census(points, 0, 1, root, 2)
    require(first["depth"] == 1 and len(first["shell"]) == 6, "extra-shell fixture")
    require(pairs == 36 and full_queries == 36 and capped_queries == 180, "census floor")
    require(saturated > 0 and complete > 0, "census status non-vacuity")
    return {
        "input_sites": len(points), "pairs": pairs, "full_queries": full_queries,
        "capped_queries": capped_queries, "saturated_queries": saturated,
        "complete_below_cap_queries": complete, "tree_visits": visits,
        "shell_fixture_ids": first["shell"],
    }


@dataclass(frozen=True)
class Link:
    """Persistent payload reference; no copy of the inherited ID blocks."""

    node: Node
    tail: Link | None


def linked_ids(head: Link | None) -> list[int]:
    ids = []
    while head is not None:
        ids.extend(head.node.ids)
        head = head.tail
    return sorted(ids)


@dataclass(frozen=True)
class ThreadedNode:
    node: Node
    escape: int


def thread_tree(root: Node) -> tuple[ThreadedNode, ...]:
    """Preorder, right child first as in tree_census; escape skips a subtree."""
    entries = []

    def visit(node: Node) -> None:
        position = len(entries)
        entries.append(ThreadedNode(node, -1))
        for child in reversed(node.children):
            visit(child)
        entries[position] = ThreadedNode(node, len(entries))

    visit(root)
    result = tuple(entries)
    validate_threaded(result)
    return result


def validate_threaded(index: tuple[ThreadedNode, ...]) -> None:
    require(bool(index), "empty threaded index")
    require(len(index) == 2 * len(index[0].node.ids) - 1, "threaded tree coverage")
    for position, entry in enumerate(index):
        require(position < entry.escape <= len(index), "invalid escape")
        require(entry.escape == position + 2 * len(entry.node.ids) - 1,
                "escape skips or repeats an ID interval")


def joint_census(
    points: tuple[Point, ...], u: Node, v: Node,
    index: tuple[ThreadedNode, ...], cap: int,
    split_budget: int | None = None, mutant: str = "",
) -> tuple[list[dict], dict]:
    """Shared traversal with one witness cursor per product continuation.

    Fixed DFS ordering lets current_z encode every unconsumed witness subtree.
    A product split copies this cursor, count and immutable payload heads.
    The per-lineage split budget changes grain, never search completeness.
    """
    require(cap > 0, "positive census threshold required")
    stats = {
        "bound_tests": 0, "pair_splits": 0, "witness_splits": 0,
        "cursor_copies": 0, "payload_links": 0, "max_call_depth": 0,
        "fallback_pairs": 0, "splits_after_credit": 0,
        "splits_after_shell": 0, "index_nodes": len(index),
    }

    def extent(node: Node) -> int:
        return max(high - low for low, high in node.box)

    def walk(
        left: Node, right: Node, cursor: int, count: int,
        inside: Link | None, shell: Link | None, depth: int, pair_depth: int,
    ) -> Iterator[dict]:
        stats["max_call_depth"] = max(stats["max_call_depth"], depth)
        while cursor < len(index):
            current, escape = index[cursor].node, index[cursor].escape
            stats["bound_tests"] += 1
            decision = classify(*box_bounds(left.box, right.box, current.box))
            if decision == "interior":
                count += len(current.ids)
                if count >= cap:
                    yield {"u": left.ids, "v": right.ids,
                           "status": "saturated", "depth": cap}
                    return
                inside = Link(current, inside)
                stats["payload_links"] += 1
                cursor = escape
            elif decision == "shell":
                shell = Link(current, shell)
                stats["payload_links"] += 1
                cursor = escape
            elif decision == "exterior":
                cursor = escape
            else:
                pair_is_singleton = len(left.ids) == len(right.ids) == 1
                budget_used = split_budget is not None and pair_depth >= split_budget
                if not pair_is_singleton and budget_used:
                    # No A*B array: continue each unresolved pair from cursor,
                    # preserving previously acquired counts and payloads.
                    for a, b in product(left.ids, right.ids):
                        stats["fallback_pairs"] += 1
                        stats["cursor_copies"] += 1
                        aa = Node((a,), singleton(points[a]), ())
                        bb = Node((b,), singleton(points[b]), ())
                        yield from walk(aa, bb, cursor, count, inside, shell,
                                        depth + 1, pair_depth)
                    return
                factors = [(extent(left), 0, left), (extent(right), 1, right)]
                factors = [entry for entry in factors if entry[2].children]
                candidate = max(factors, default=None,
                                key=lambda entry: (entry[0], entry[1]))
                if candidate is not None and (
                    not current.children or candidate[0] >= extent(current)
                ):
                    stats["pair_splits"] += 1
                    stats["splits_after_credit"] += int(count > 0)
                    stats["splits_after_shell"] += int(shell is not None)
                    inherited = cursor
                    if mutant == "restart_root_after_credit" and count > 0:
                        inherited = 0
                    elif mutant == "drop_undecided_head":
                        inherited = escape
                    inherited_shell = None if mutant == "forget_inherited_shell" else shell
                    for child in candidate[2].children:
                        stats["cursor_copies"] += 1
                        aa, bb = ((child, right) if candidate[1] == 0
                                  else (left, child))
                        yield from walk(aa, bb, inherited, count, inside,
                                        inherited_shell, depth + 1, pair_depth + 1)
                    return
                require(bool(current.children), "three singleton boxes must decide")
                stats["witness_splits"] += 1
                cursor += 1  # first child in this fixed preorder
        interior_ids, shell_ids = linked_ids(inside), linked_ids(shell)
        require(len(interior_ids) == count, "continuation count lost its IDs")
        require(len(set(interior_ids)) == count, "continuation counted an ID twice")
        require(len(set(shell_ids)) == len(shell_ids), "continuation repeated shell")
        require(not set(interior_ids).intersection(shell_ids), "continuation payload overlap")
        yield {
            "u": left.ids, "v": right.ids, "status": "complete", "depth": count,
            "interior": interior_ids, "shell": shell_ids,
        }

    records = list(walk(u, v, 0, 0, None, None, 1, 0))
    stats["output_records"] = len(records)
    stats["largest_complete_shell"] = max(
        (len(record["shell"]) for record in records if record["status"] == "complete"),
        default=0,
    )
    stats["saturated_multi_pair_records"] = sum(
        record["status"] == "saturated" and len(record["u"]) * len(record["v"]) > 1
        for record in records
    )
    return records, stats


def check_joint_result(points: tuple[Point, ...], a_ids: tuple, b_ids: tuple,
                       cap: int, records: list[dict]) -> dict:
    expected_pairs = set(product(a_ids, b_ids))
    seen = set()
    for record in records:
        if record["status"] == "complete":
            require(len(record["u"]) == len(record["v"]) == 1,
                    "uniform complete shells must isolate distinct support pairs")
        for a, b in product(record["u"], record["v"]):
            require((a, b) in expected_pairs and (a, b) not in seen, "product coverage failure")
            seen.add((a, b))
            # Rational center/radius oracle; no traversal bounds reused.
            powers = [power_value(points[a], points[b], z) for z in points]
            interior = [i for i, value in enumerate(powers) if value > 0]
            shell = [i for i, value in enumerate(powers) if value == 0]
            require(record["depth"] == min(cap, len(interior)), "joint depth mismatch")
            if len(interior) >= cap:
                require(record["status"] == "saturated", "joint saturation lost")
                require("shell" not in record and "interior" not in record,
                        "partial payload advertised")
            else:
                require(record["status"] == "complete", "joint exact status lost")
                require(record["interior"] == interior and record["shell"] == shell,
                        "joint payload mismatch")
    require(seen == expected_pairs, "joint product omitted pairs")
    return {"pairs": len(seen), "point_oracle_evaluations": len(seen) * len(points)}


def check_joint() -> dict:
    fixtures = []
    # Both ordinary and irregular transverse sheets, with sites outside A+B.
    for width in (2, 3, 4):
        a = tuple((100, 10 * y, 10 * z) for y in range(width) for z in range(width))
        b = tuple((60000, 10 * y, 10 * z) for y in range(width) for z in range(width))
        for exterior in ((), ((30000, 10, 10), (0, 0, 0), (65535, 65535, 65535))):
            fixtures.append((f"sheet_{width}_extra{len(exterior)}", a + b + exterior,
                             tuple(range(len(a))), tuple(range(len(a), len(a) + len(b)))))
    points = ((10, 10, 10), (18, 10, 10), (14, 10, 10), (14, 14, 10),
              (14, 6, 10), (14, 10, 14), (14, 10, 6), (30, 30, 30), (0, 0, 0))
    fixtures.append(("shell_six", points, (0,), (1, 3, 4, 5, 6, 7)))
    fixtures.append(("inherited_credit_and_shell",
                     ((0, 0, 0), (0, 2, 0), (100, 0, 0), (50, 1, 0), (0, 1, 0)),
                     (0, 1), (2,)))
    sphere = tuple(sorted({
        tuple(10 + sign * value for sign, value in zip(signs, xyz))
        for base in ((5, 0, 0), (4, 3, 0))
        for xyz in permutations(base) for signs in product((-1, 1), repeat=3)
    }))
    require(len(sphere) == 30, "large-shell fixture size")
    fixtures.append(("shell_thirty", sphere,
                     (sphere.index((5, 10, 10)), sphere.index((10, 5, 10))),
                     (sphere.index((15, 10, 10)), sphere.index((10, 15, 10)))))
    rows = []
    pairs = oracle_tests = 0
    mutant_hits = {name: None for name in (
        "restart_root_after_credit", "drop_undecided_head", "forget_inherited_shell",
    )}
    for name, points, a_ids, b_ids in fixtures:
        root = build_tree(points, tuple(range(len(points))))
        u, v = build_tree(points, a_ids), build_tree(points, b_ids)
        index = thread_tree(root)
        for cap in (1, 2, 5, 10):
            independent_visits = sum(
                tree_census(points, a, b, root, cap)["visits"] for a, b in product(a_ids, b_ids)
            )
            for budget in (None, 0, 2):
                records, work = joint_census(points, u, v, index, cap, budget)
                checked = check_joint_result(points, a_ids, b_ids, cap, records)
                pairs += checked["pairs"]
                oracle_tests += checked["point_oracle_evaluations"]
                rows.append({"fixture": name, "sites": len(points), "cap": cap,
                             "split_budget": budget,
                             "independent_visits": independent_visits, **work})
            for mutant in mutant_hits:
                if mutant_hits[mutant] is not None:
                    continue
                try:
                    wrong, _ = joint_census(points, u, v, index, cap, mutant=mutant)
                    check_joint_result(points, a_ids, b_ids, cap, wrong)
                except RuntimeError as error:
                    mutant_hits[mutant] = {"fixture": name, "cap": cap, "rejection": str(error)}
    # The fixture tree visits x=4,2,0. Alter escape at the first leaf:
    # skipping to end loses the interior; a backward jump could loop forever.
    escape_root = build_tree(((0, 0, 0), (2, 0, 0), (4, 0, 0)), (0, 1, 2))
    index = thread_tree(escape_root)
    leaf = next(i for i, entry in enumerate(index) if not entry.node.children)
    for name, destination in (("escape_skips_sibling", len(index)),
                              ("escape_returns_to_consumed_prefix", 0)):
        invalid = list(index)
        invalid[leaf] = ThreadedNode(index[leaf].node, destination)
        try:
            validate_threaded(tuple(invalid))
        except RuntimeError as error:
            mutant_hits[name] = {"fixture": "line_three", "rejection": str(error)}
        else:
            raise RuntimeError("invalid escape accepted")
    require(all(mutant_hits.values()), "continuation mutant not exercised")
    require(any(row["splits_after_credit"] for row in rows), "credit inheritance vacuous")
    require(any(row["splits_after_shell"] for row in rows), "shell inheritance vacuous")
    require(any(row["fallback_pairs"] for row in rows), "bounded continuation vacuous")
    require(any(row["largest_complete_shell"] == 30 for row in rows), "large shell not retained")
    require(any(row["saturated_multi_pair_records"] for row in rows), "product saturation vacuous")
    require(any(row["bound_tests"] < row["independent_visits"] for row in rows),
            "shared work never observed")
    require(any(row["bound_tests"] > row["independent_visits"]
                for row in rows if row["split_budget"] == 0),
            "initial fallback overhead not exercised")
    return {"fixtures": len(fixtures), "runs": len(rows), "pairs_checked": pairs,
            "point_oracle_evaluations": oracle_tests, "rows": rows,
            "rejected_continuation_mutants": mutant_hits}


def check_mutants() -> list[dict]:
    lower, upper4 = interval_bounds((1, 1), (5, 5), (0, 6))
    wrong_corner_max = max((z - 1) * (5 - z) for z in (0, 6))
    require(lower == -5 and upper4 == 16 and wrong_corner_max == -5, "maximum mutant fixture")
    require(power_value((1,), (5,), (3,)) == 4, "maximum mutant interior")
    a, b0, b1, z = (0, 0, 0), (100, 0, 0), (104, 0, 0), (102, 0, 0)
    h0, h1 = power_value(a, b0, z), power_value(a, b1, z)
    require(h0 == -204 and h1 == 204, "NoCredit/outside mutant")
    shell = power_value((0, 0, 0), (2, 0, 0), (1, 1, 0))
    require(shell == 0 and classify(0, 0) == "shell", "shell mutant")
    # An old core certificate names the same ID that the global traversal
    # encounters. Its count cannot initialize a census restarted at root.
    actual_depth = int(power_value((0,), (100,), (50,)) > 0)
    wrong_depth = 1 + actual_depth
    require(actual_depth == 1 and wrong_depth >= 2, "core double-count mutant")
    return [
        {"name": "maximum_only_at_z_corners", "wrong_max": wrong_corner_max, "true_max4": upper4},
        {"name": "universal_nocredit_as_global_exterior", "h_at_b0": int(h0), "h_at_b1": int(h1)},
        {"name": "nonpositive_max_discards_shell", "h": int(shell), "required_class": "shell"},
        {
            "name": "core_added_to_restarted_global_census",
            "true_depth": actual_depth, "wrong_depth": wrong_depth, "cap": 2,
        },
    ]


def main() -> None:
    result = {
        "status": "passed",
        "scope": (
            "Python exact bounds, fixed-pair and joint tiny-tree census; "
            "no product integration, performance, or FULL claim"
        ),
        "extrema": check_extrema(),
        "census": check_census(),
        "joint_continuations": check_joint(),
        "rejected_model_mutants": check_mutants(),
    }
    require(len(result["rejected_model_mutants"]) == 4, "mutant floor")
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
