#!/usr/bin/env python3
"""Bounded audit of q2 box extrema and strict-depth/shell semantics.

This independent Python model is not a production census, a joint traversal
of pair products, a WSPD, or a qualification of the HGP FULL tower.
"""

from __future__ import annotations

from dataclasses import dataclass
from fractions import Fraction
from itertools import combinations, product
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
            "Python exact-integer/rational bounds and fixed-pair tiny-tree "
            "census; no product integration, joint pair traversal, "
            "performance, or FULL claim"
        ),
        "extrema": check_extrema(),
        "census": check_census(),
        "rejected_model_mutants": check_mutants(),
    }
    require(len(result["rejected_model_mutants"]) == 4, "mutant floor")
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
