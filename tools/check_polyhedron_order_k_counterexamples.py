#!/usr/bin/env python3
"""Recertify the exact counterexamples of the order-k polyhedron exploration.

The fixture gathers three minimal configurations found during the v11
exploration of 6-7 October 2026 (polyhedral representatives of the dense
components of HGP).  Every value is recomputed here in rational arithmetic;
the checker imports no project code.

1. ``outliers_fewer_than_k``: adding fewer than k sites can create or merge
   components of Omega_k(r) = {y : |B(y, r) ∩ P| >= k}.  The sites lie on the
   x axis.  The witness region of a k-subset is a convex body of revolution
   around this axis; a nonempty region meets the axis, and two regions meet
   exactly when their axis traces meet (project a common point on the axis).
   The components of Omega_k(r) are therefore those of its axis trace, a
   finite union of closed intervals.  The checker also verifies the correct
   statements on the same clouds: a group farther than 2r from the sites
   changes nothing, and Omega_k^P ⊆ Omega_k^{P ∪ O} ⊆ Omega_{k-m}^P.
2. ``contracted_chain_identity``: at k = 1, contracting every merge in which
   exactly one child lives longer than 2 delta (the parent continues that
   child) does not give chain identities that are stable under a matched
   displacement of delta: the image of one chain changes chain inside its
   own life.
3. ``no_nesting_across_orders``: planar clouds (z = 0) where a point of an
   active order-2 cell, in its barycentric realization or in its coverage
   shadow conv(union of labels), lies strictly inside an order-1 Delaunay
   triangle whose level (circumradius squared) exceeds r^2, hence outside the
   order-1 alpha complex at the same radius.  The order-2 cell is certified
   by a witness y0 whose closed 2-nearest sets are listed exhaustively, by
   d_2(y0)^2 <= r^2, and by strict perturbations proving that each listed
   order-2 domain is full-dimensional.
"""

from __future__ import annotations

import argparse
import json
import sys
from fractions import Fraction
from itertools import combinations
from pathlib import Path
from typing import Any


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_FIXTURE = (
    REPOSITORY_ROOT
    / "tests"
    / "fixtures"
    / "regressions"
    / "polyhedron_order_k_counterexamples.json"
)
PERTURBATION = Fraction(1, 100)


class ValidationError(RuntimeError):
    """Raised when the fixture or one of its exact certificates is invalid."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValidationError(message)


def _expect_keys(value: Any, keys: set[str], path: str) -> dict[str, Any]:
    require(isinstance(value, dict), f"{path} must be an object")
    require(
        set(value) == keys,
        f"{path} has unexpected fields: {sorted(set(value) ^ keys)!r}",
    )
    return value


def _integer(value: Any, path: str, *, minimum: int | None = None) -> int:
    require(
        isinstance(value, int) and not isinstance(value, bool),
        f"{path} must be an integer",
    )
    if minimum is not None:
        require(value >= minimum, f"{path} must be at least {minimum}")
    return value


def _fraction(value: Any, path: str) -> Fraction:
    if isinstance(value, int) and not isinstance(value, bool):
        return Fraction(value)
    record = _expect_keys(value, {"denominator", "numerator"}, path)
    numerator = _integer(record["numerator"], f"{path}.numerator")
    denominator = _integer(
        record["denominator"], f"{path}.denominator", minimum=2
    )
    result = Fraction(numerator, denominator)
    require(
        result.numerator == numerator and result.denominator == denominator,
        f"{path} must be a reduced canonical fraction",
    )
    return result


def _point(value: Any, path: str) -> tuple[Fraction, Fraction, Fraction]:
    require(
        isinstance(value, list) and len(value) == 3,
        f"{path} must be a point with three coordinates",
    )
    return (
        _fraction(value[0], f"{path}[0]"),
        _fraction(value[1], f"{path}[1]"),
        _fraction(value[2], f"{path}[2]"),
    )


def _points(value: Any, path: str) -> list[tuple[Fraction, Fraction, Fraction]]:
    require(isinstance(value, list), f"{path} must be an array")
    return [_point(item, f"{path}[{index}]") for index, item in enumerate(value)]


def _indices(value: Any, path: str, count: int) -> tuple[int, ...]:
    require(isinstance(value, list), f"{path} must be an array")
    result = tuple(
        _integer(item, f"{path}[{index}]", minimum=0)
        for index, item in enumerate(value)
    )
    require(all(item < count for item in result), f"{path} has an index out of range")
    require(len(set(result)) == len(result), f"{path} repeats an index")
    return result


def _intervals(value: Any, path: str) -> list[tuple[Fraction, Fraction]]:
    require(isinstance(value, list), f"{path} must be an array")
    result = []
    for index, item in enumerate(value):
        require(
            isinstance(item, list) and len(item) == 2,
            f"{path}[{index}] must be a closed interval",
        )
        result.append(
            (
                _fraction(item[0], f"{path}[{index}][0]"),
                _fraction(item[1], f"{path}[{index}][1]"),
            )
        )
    return result


# ------------------------------------------------------------------- case 1


def axis_trace(xs: list[Fraction], k: int, r: Fraction) -> list[tuple[Fraction, Fraction]]:
    """Closed axis trace of Omega_k(r) for sites on the x axis, merged into components."""
    intervals = []
    for subset in combinations(sorted(xs), k):
        low = max(subset) - r
        high = min(subset) + r
        if low <= high:
            intervals.append((low, high))
    intervals.sort()
    merged: list[tuple[Fraction, Fraction]] = []
    for low, high in intervals:
        if merged and low <= merged[-1][1]:
            if high > merged[-1][1]:
                merged[-1] = (merged[-1][0], high)
        else:
            merged.append((low, high))
    return merged


def _covered(inner: list[tuple[Fraction, Fraction]], outer: list[tuple[Fraction, Fraction]]) -> bool:
    return all(
        any(low <= inner_low and inner_high <= high for low, high in outer)
        for inner_low, inner_high in inner
    )


def _axis_sites(value: Any, path: str) -> list[Fraction]:
    points = _points(value, path)
    require(
        all(point[1] == 0 and point[2] == 0 for point in points),
        f"{path} must lie on the x axis",
    )
    xs = [point[0] for point in points]
    require(len(set(xs)) == len(xs), f"{path} must contain distinct sites")
    return xs


def validate_outliers(case: Any) -> None:
    case = _expect_keys(case, {"claim", "refuted_by", "subcases"}, "outliers_fewer_than_k")
    require(isinstance(case["subcases"], list) and case["subcases"], "outlier subcases missing")
    roles = set()
    for index, raw in enumerate(case["subcases"]):
        path = f"outliers_fewer_than_k.subcases[{index}]"
        sub = _expect_keys(
            raw,
            {"name", "role", "sites", "added", "order", "radius", "trace_before",
             "trace_after", "lower_order_trace"},
            path,
        )
        role = sub["role"]
        require(role in {"creation", "merge", "separated_control"}, f"{path}.role is unknown")
        roles.add(role)
        sites = _axis_sites(sub["sites"], f"{path}.sites")
        added = _axis_sites(sub["added"], f"{path}.added")
        require(not set(sites) & set(added), f"{path}.added repeats a site")
        k = _integer(sub["order"], f"{path}.order", minimum=2)
        m = len(added)
        require(1 <= m < k, f"{path} must add fewer than k sites")
        r = _fraction(sub["radius"], f"{path}.radius")
        require(r > 0, f"{path}.radius must be positive")
        before = axis_trace(sites, k, r)
        after = axis_trace(sites + added, k, r)
        lower = axis_trace(sites, k - m, r)
        require(before == _intervals(sub["trace_before"], f"{path}.trace_before"), f"{path}: trace before mismatch")
        require(after == _intervals(sub["trace_after"], f"{path}.trace_after"), f"{path}: trace after mismatch")
        require(lower == _intervals(sub["lower_order_trace"], f"{path}.lower_order_trace"), f"{path}: lower-order trace mismatch")
        require(_covered(before, after), f"{path}: Omega_k^P is not inside Omega_k^(P+O)")
        require(_covered(after, lower), f"{path}: Omega_k^(P+O) is not inside Omega_(k-m)^P")
        if role == "creation":
            require(not before and after, f"{path}: no component is created")
        elif role == "merge":
            require(len(before) >= 2 and len(after) < len(before), f"{path}: no components are merged")
        else:
            gap = min(abs(a - s) for a in added for s in sites)
            require(gap > 2 * r, f"{path}: separated control is not farther than 2r")
            require(before == after, f"{path}: separated control changes Omega_k(r)")
    require(roles == {"creation", "merge", "separated_control"}, "outlier subcases must cover the three roles")


# ------------------------------------------------------------------- case 2


def single_linkage_axis(xs: list[Fraction]) -> list[dict[str, Any]]:
    """Merge tree at k = 1 for distinct sites on the axis; radii are half gaps."""
    order = sorted(range(len(xs)), key=lambda index: xs[index])
    gaps = [
        (xs[order[i + 1]] - xs[order[i]], order[i], order[i + 1])
        for i in range(len(order) - 1)
    ]
    require(len({gap for gap, _, _ in gaps}) == len(gaps), "tied gaps are outside this fixture")
    nodes: list[dict[str, Any]] = [
        {"members": frozenset([index]), "birth": Fraction(0), "death": None, "children": ()}
        for index in range(len(xs))
    ]
    owner = list(range(len(xs)))
    for gap, left, right in sorted(gaps):
        a, b = owner[left], owner[right]
        node = {
            "members": nodes[a]["members"] | nodes[b]["members"],
            "birth": gap / 2,
            "death": None,
            "children": (a, b),
        }
        nodes.append(node)
        new = len(nodes) - 1
        nodes[a]["death"] = node["birth"]
        nodes[b]["death"] = node["birth"]
        for site in node["members"]:
            owner[site] = new
    return nodes


def contracted_chains(nodes: list[dict[str, Any]], delta: Fraction) -> list[Any]:
    """Chain label of each node under the contraction rule (None: forgotten node)."""

    def robust(index: int) -> bool:
        node = nodes[index]
        return node["death"] is None or node["death"] - node["birth"] > 2 * delta

    chain: list[Any] = []
    for index, node in enumerate(nodes):
        if not node["children"]:
            chain.append(("leaf", min(node["members"])) if robust(index) else None)
            continue
        strong = [child for child in node["children"] if robust(child)]
        if len(strong) == 1:
            chain.append(chain[strong[0]])
        else:
            chain.append(("node", index) if robust(index) else None)
    return chain


def _alive(nodes: list[dict[str, Any]], radius: Fraction, site: int) -> int:
    for index, node in enumerate(nodes):
        if site in node["members"] and node["birth"] <= radius and (
            node["death"] is None or radius < node["death"]
        ):
            return index
    raise ValidationError("no node alive at the probed radius")


def validate_contracted_chain(case: Any) -> None:
    case = _expect_keys(
        case,
        {"claim", "refuted_by", "order", "sites_before", "sites_after", "delta",
         "merges_before", "merges_after", "root_chain_leaf_before",
         "root_chain_leaf_after", "probes"},
        "contracted_chain_identity",
    )
    require(_integer(case["order"], "contracted_chain_identity.order") == 1, "this case is at k = 1")
    xs = _axis_sites(case["sites_before"], "contracted_chain_identity.sites_before")
    ys = _axis_sites(case["sites_after"], "contracted_chain_identity.sites_after")
    require(len(xs) == len(ys), "matched clouds must have the same size")
    delta = _fraction(case["delta"], "contracted_chain_identity.delta")
    require(delta == max(abs(x - y) for x, y in zip(xs, ys)) and delta > 0, "delta mismatch")
    trees = (single_linkage_axis(xs), single_linkage_axis(ys))
    for tree, key in zip(trees, ("merges_before", "merges_after")):
        expected = case[key]
        require(isinstance(expected, list), f"{key} must be an array")
        merges = [node for node in tree if node["children"]]
        require(len(merges) == len(expected), f"{key}: merge count mismatch")
        for index, (node, raw) in enumerate(zip(merges, expected)):
            record = _expect_keys(raw, {"radius", "members"}, f"{key}[{index}]")
            require(node["birth"] == _fraction(record["radius"], f"{key}[{index}].radius"), f"{key}: merge radius mismatch")
            members = _indices(record["members"], f"{key}[{index}].members", len(xs))
            require(node["members"] == frozenset(members), f"{key}: merge members mismatch")
    chains = (contracted_chains(trees[0], delta), contracted_chains(trees[1], delta))
    roots = (chains[0][-1], chains[1][-1])
    expected_roots = (
        _integer(case["root_chain_leaf_before"], "root_chain_leaf_before", minimum=0),
        _integer(case["root_chain_leaf_after"], "root_chain_leaf_after", minimum=0),
    )
    require(roots[0] == ("leaf", expected_roots[0]), "root chain before mismatch")
    require(roots[1] == ("leaf", expected_roots[1]), "root chain after mismatch")
    require(expected_roots[0] != expected_roots[1], "the root chains must differ for this counterexample")
    leaf = expected_roots[0]
    probes = case["probes"]
    require(isinstance(probes, list) and len(probes) >= 2, "at least two probes are required")
    images = []
    for index, raw in enumerate(probes):
        record = _expect_keys(raw, {"radius", "image_chain_leaf"}, f"probes[{index}]")
        radius = _fraction(record["radius"], f"probes[{index}].radius")
        require(radius >= trees[0][leaf]["birth"] + delta, f"probes[{index}] lies before the chain interior")
        node_before = _alive(trees[0], radius, leaf)
        require(chains[0][node_before] == ("leaf", leaf), f"probes[{index}] does not lie on the probed chain")
        node_after = _alive(trees[1], radius + delta, leaf)
        image = chains[1][node_after]
        if image is None:
            ancestor = node_after
            while chains[1][ancestor] is None:
                parents = [i for i, node in enumerate(trees[1]) if ancestor in node["children"]]
                require(len(parents) == 1, "forgotten node without a parent")
                ancestor = parents[0]
            image = chains[1][ancestor]
        expected = _integer(record["image_chain_leaf"], f"probes[{index}].image_chain_leaf", minimum=0)
        require(image == ("leaf", expected), f"probes[{index}]: image chain mismatch")
        images.append(expected)
    require(len(set(images)) >= 2, "the image of the chain must change chain inside its life")


# ------------------------------------------------------------------- case 3


Point2 = tuple[Fraction, Fraction]


def _orient(a: Point2, b: Point2, c: Point2) -> Fraction:
    return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])


def _sq(a: Point2, b: Point2) -> Fraction:
    return (a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2


def _circumcircle(a: Point2, b: Point2, c: Point2) -> tuple[Point2, Fraction]:
    d = 2 * _orient(a, b, c)
    require(d != 0, "degenerate triangle")
    a2 = a[0] ** 2 + a[1] ** 2
    b2 = b[0] ** 2 + b[1] ** 2
    c2 = c[0] ** 2 + c[1] ** 2
    ux = (a2 * (b[1] - c[1]) + b2 * (c[1] - a[1]) + c2 * (a[1] - b[1])) / d
    uy = (a2 * (c[0] - b[0]) + b2 * (a[0] - c[0]) + c2 * (b[0] - a[0])) / d
    center = (ux, uy)
    return center, _sq(center, a)


def _hull(points: list[Point2]) -> list[Point2]:
    unique = sorted(set(points))
    if len(unique) <= 2:
        return unique
    lower: list[Point2] = []
    for p in unique:
        while len(lower) >= 2 and _orient(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    upper: list[Point2] = []
    for p in reversed(unique):
        while len(upper) >= 2 and _orient(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]


def _in_convex_hull(points: list[Point2], query: Point2) -> bool:
    hull = _hull(points)
    if len(hull) == 1:
        return hull[0] == query
    if len(hull) == 2:
        a, b = hull
        return (
            _orient(a, b, query) == 0
            and min(a[0], b[0]) <= query[0] <= max(a[0], b[0])
            and min(a[1], b[1]) <= query[1] <= max(a[1], b[1])
        )
    return all(
        _orient(hull[i], hull[(i + 1) % len(hull)], query) >= 0
        for i in range(len(hull))
    )


def _planar(value: Any, path: str) -> list[Point2]:
    points = _points(value, path)
    require(all(point[2] == 0 for point in points), f"{path} must lie in the plane z = 0")
    return [(point[0], point[1]) for point in points]


def _closed_k_sets(points: list[Point2], y: Point2, k: int) -> set[frozenset[int]]:
    distances = [_sq(y, p) for p in points]
    result = set()
    for subset in combinations(range(len(points)), k):
        inside = max(distances[i] for i in subset)
        outside = [distances[i] for i in range(len(points)) if i not in subset]
        if not outside or inside <= min(outside):
            result.add(frozenset(subset))
    return result


def _strict_k_set(points: list[Point2], y: Point2, subset: frozenset[int]) -> bool:
    distances = [_sq(y, p) for p in points]
    inside = max(distances[i] for i in subset)
    outside = [distances[i] for i in range(len(points)) if i not in subset]
    return not outside or inside < min(outside)


def validate_nesting(case: Any) -> None:
    case = _expect_keys(case, {"claim", "refuted_by", "subcases"}, "no_nesting_across_orders")
    require(isinstance(case["subcases"], list) and case["subcases"], "nesting subcases missing")
    realizations = set()
    for index, raw in enumerate(case["subcases"]):
        path = f"no_nesting_across_orders.subcases[{index}]"
        sub = _expect_keys(
            raw,
            {"name", "realization", "points", "radius_squared", "query",
             "order_one_triangles", "containing_triangle",
             "containing_circumradius_squared", "witness", "witness_order",
             "witness_k_sets", "witness_kth_distance_squared", "perturbations"},
            path,
        )
        realization = sub["realization"]
        require(realization in {"barycentric", "shadow"}, f"{path}.realization is unknown")
        realizations.add(realization)
        points = _planar(sub["points"], f"{path}.points")
        require(len(set(points)) == len(points), f"{path}.points must be distinct")
        r2 = _fraction(sub["radius_squared"], f"{path}.radius_squared")
        query = _planar([sub["query"]], f"{path}.query")[0]
        # Order 1: the listed triangles are exactly the Delaunay triangles (strictly empty circles).
        triangles = []
        for t_index, raw_triangle in enumerate(sub["order_one_triangles"]):
            triangle = _indices(raw_triangle, f"{path}.order_one_triangles[{t_index}]", len(points))
            require(len(triangle) == 3, f"{path}: triangles have three vertices")
            triangles.append(frozenset(triangle))
        listed = set(triangles)
        require(len(listed) == len(triangles), f"{path}: repeated triangle")
        for triple in combinations(range(len(points)), 3):
            a, b, c = (points[i] for i in triple)
            if _orient(a, b, c) == 0:
                require(frozenset(triple) not in listed, f"{path}: flat triangle listed")
                continue
            center, radius2 = _circumcircle(a, b, c)
            others = [_sq(center, points[i]) for i in range(len(points)) if i not in triple]
            require(all(value != radius2 for value in others), f"{path}: cocircular points are outside this fixture")
            empty = all(value > radius2 for value in others)
            require(empty == (frozenset(triple) in listed), f"{path}: order-one Delaunay triangles mismatch")
        container = _integer(sub["containing_triangle"], f"{path}.containing_triangle", minimum=0)
        require(container < len(triangles), f"{path}.containing_triangle out of range")
        a, b, c = (points[i] for i in sorted(triangles[container]))
        if _orient(a, b, c) < 0:
            b, c = c, b
        require(
            _orient(a, b, query) > 0 and _orient(b, c, query) > 0 and _orient(c, a, query) > 0,
            f"{path}: query is not strictly inside the containing triangle",
        )
        _, radius2 = _circumcircle(a, b, c)
        require(
            radius2 == _fraction(sub["containing_circumradius_squared"], f"{path}.containing_circumradius_squared"),
            f"{path}: containing circumradius mismatch",
        )
        require(radius2 > r2, f"{path}: the containing triangle is active, the query is not outside A_1(r)")
        # Order 2: witness, exhaustive closed k-sets, full-dimensional domains, active cell.
        k = _integer(sub["witness_order"], f"{path}.witness_order", minimum=2)
        witness = _planar([sub["witness"]], f"{path}.witness")[0]
        expected_sets = set()
        for s_index, raw_set in enumerate(sub["witness_k_sets"]):
            subset = _indices(raw_set, f"{path}.witness_k_sets[{s_index}]", len(points))
            require(len(subset) == k, f"{path}: witness sets must have k sites")
            expected_sets.add(frozenset(subset))
        require(_closed_k_sets(points, witness, k) == expected_sets, f"{path}: witness k-sets mismatch")
        kth = sorted(_sq(witness, p) for p in points)[k - 1]
        require(
            kth == _fraction(sub["witness_kth_distance_squared"], f"{path}.witness_kth_distance_squared"),
            f"{path}: witness k-th distance mismatch",
        )
        require(kth <= r2, f"{path}: the order-k cell is not active at r")
        perturbed = set()
        for p_index, raw_perturbation in enumerate(sub["perturbations"]):
            record = _expect_keys(raw_perturbation, {"k_set", "direction"}, f"{path}.perturbations[{p_index}]")
            subset = frozenset(_indices(record["k_set"], f"{path}.perturbations[{p_index}].k_set", len(points)))
            direction = _planar([record["direction"]], f"{path}.perturbations[{p_index}].direction")[0]
            moved = (witness[0] + PERTURBATION * direction[0], witness[1] + PERTURBATION * direction[1])
            require(_strict_k_set(points, moved, subset), f"{path}: perturbation does not isolate its k-set")
            perturbed.add(subset)
        require(perturbed == expected_sets, f"{path}: every witness k-set needs a full-dimensional perturbation")
        if realization == "barycentric":
            carriers = [
                (sum(points[i][0] for i in subset) / k, sum(points[i][1] for i in subset) / k)
                for subset in expected_sets
            ]
        else:
            labels = set().union(*expected_sets)
            carriers = [points[i] for i in labels]
        require(_in_convex_hull(carriers, query), f"{path}: query is not in the active order-k cell realization")
    require(realizations == {"barycentric", "shadow"}, "both realizations must be refuted")


# ------------------------------------------------------------------- driver


def validate_fixture(payload: Any) -> None:
    payload = _expect_keys(
        payload,
        {"schema_version", "kind", "fixture_id", "description", "provenance", "cases"},
        "fixture",
    )
    require(payload["schema_version"] == 1, "unsupported schema version")
    require(payload["kind"] == "morsehgp3d_v11_polyhedron_order_k_counterexamples", "unexpected fixture kind")
    require(payload["fixture_id"] == "polyhedron-order-k-counterexamples-v1", "unexpected fixture id")
    cases = _expect_keys(
        payload["cases"],
        {"outliers_fewer_than_k", "contracted_chain_identity", "no_nesting_across_orders"},
        "fixture.cases",
    )
    validate_outliers(cases["outliers_fewer_than_k"])
    validate_contracted_chain(cases["contracted_chain_identity"])
    validate_nesting(cases["no_nesting_across_orders"])


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("fixture", nargs="?", type=Path, default=DEFAULT_FIXTURE)
    arguments = parser.parse_args(argv)
    with arguments.fixture.open(encoding="utf-8") as fixture_file:
        payload = json.load(fixture_file)
    try:
        validate_fixture(payload)
    except ValidationError as error:
        print(f"polyhedron order-k counterexamples: FAIL: {error}", file=sys.stderr)
        return 1
    print("polyhedron order-k counterexamples: PASS (3 cases)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
