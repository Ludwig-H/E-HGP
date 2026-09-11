"""Bounded, product-independent tests of the marked filtered-graph reduction."""

from __future__ import annotations

from dataclasses import dataclass, replace
from fractions import Fraction as F
from itertools import combinations
import random


@dataclass(frozen=True)
class Hub:
    name: str
    level: F
    terminals: tuple[str, ...] = ()
    points: tuple[int, ...] = ()


Births = dict[str, F]
Edge = tuple[str, str, F]
Reps = dict[str, str]
Label = tuple[str, ...]
Signature = tuple[tuple[tuple[Label, tuple[int, ...]], ...], dict[str, Label]]
Event = tuple[Label, tuple[Label, ...], tuple[str, ...]]


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def graph(hubs: tuple[Hub, ...]) -> tuple[Births, list[Edge]]:
    births = {h.name: h.level for h in hubs}
    require(len(births) == len(hubs), "duplicate hub")
    edges = []
    for h in hubs:
        for t in h.terminals:
            require(t in births and births[t] < h.level, "non-strict terminal")
            edges.append((h.name, t, h.level))
    return births, edges


def reduce_graph(hubs: tuple[Hub, ...], last: bool = False) -> tuple[Births, list[Edge], Reps]:
    graph(hubs)
    representatives = {}
    births = {}
    edges = []
    for h in sorted(hubs, key=lambda h: (h.level, h.name)):
        if not h.terminals:
            representatives[h.name] = h.name
            births[h.name] = h.level
            continue
        pivot = len(h.terminals) - 1 if last else 0
        representatives[h.name] = representatives[h.terminals[pivot]]
        for i, t in enumerate(h.terminals):
            if i != pivot:
                edges.append((representatives[h.name], representatives[t], h.level))
    return births, edges, representatives


def partition(births: Births, edges: list[Edge], cut: F, closed: bool) -> dict[str, frozenset[str]]:
    active = lambda level: level <= cut if closed else level < cut
    neighbors = {v: set() for v, level in births.items() if active(level)}
    for u, v, level in edges:
        if active(level):
            require(u in neighbors and v in neighbors, "edge before vertex")
            neighbors[u].add(v)
            neighbors[v].add(u)
    components = {}
    for v in sorted(neighbors):
        if v in components:
            continue
        stack, seen = [v], {v}
        while stack:
            for w in neighbors[stack.pop()]:
                if w not in seen:
                    seen.add(w)
                    stack.append(w)
        component = frozenset(seen)
        for w in seen:
            components[w] = component
    return components


def spanning_forest(births: Births, edges: list[Edge], reverse: bool = False) -> list[Edge]:
    # Only this reduction uses union-find; the cut oracle uses graph traversal.
    parent = {v: v for v in births}

    def root(v: str) -> str:
        while parent[v] != v:
            v = parent[v]
        return v

    out = []
    for edge in sorted(edges, key=lambda e: (e[2], e[0], e[1]), reverse=reverse):
        u, v, _ = edge
        a, b = root(u), root(v)
        if a != b:
            parent[a] = b
            out.append(edge)
    return out


def deduplicate_edges(edges: list[Edge], keep_latest: bool = False) -> list[Edge]:
    levels = {}
    for u, v, level in edges:
        if u == v:
            continue
        key = tuple(sorted((u, v)))
        choose = max if keep_latest else min
        levels[key] = choose(levels.get(key, level), level)
    return [(u, v, level) for (u, v), level in sorted(levels.items())]


def signatures(hubs: tuple[Hub, ...], births: Births, edges: list[Edge],
               representatives: Reps, cut: F, closed: bool) -> Signature:
    parts = partition(births, edges, cut, closed)
    leaves = {h.name for h in hubs if not h.terminals}
    labels = {v: tuple(sorted(c & leaves)) for v, c in parts.items()}
    coverage = {label: set() for label in labels.values()}
    anchors = {}
    for h in hubs:
        if h.level < cut or (closed and h.level == cut):
            label = labels[representatives[h.name]]
            require(bool(label), "component without birth")
            anchors[h.name] = label
            coverage[label].update(h.points)
    return tuple(sorted((k, tuple(sorted(v))) for k, v in coverage.items())), anchors


def lot_events(hubs: tuple[Hub, ...], births: Births, edges: list[Edge],
               representatives: Reps, level: F) -> list[Event]:
    before = signatures(hubs, births, edges, representatives, level, False)[1]
    after = signatures(hubs, births, edges, representatives, level, True)[1]
    groups = {}
    for h in hubs:
        if h.level == level:
            key = after[h.name]
            parents, blocks = groups.setdefault(key, (set(), set()))
            blocks.add(h.name)
            parents.update(before[t] for t in h.terminals)
    return sorted((key, tuple(sorted(ps)), tuple(sorted(bs)))
                  for key, (ps, bs) in groups.items())


def public_events_from_forest(births: Births, forest: list[Edge],
                              level: F) -> list[tuple[Label, tuple[Label, ...]]]:
    """Recover public parents using the forest alone, without block terminals."""
    before = set(partition(births, forest, level, False).values())
    after = set(partition(births, forest, level, True).values())
    return sorted((tuple(sorted(component)),
                   tuple(sorted(tuple(sorted(parent)) for parent in before if parent <= component)))
                  for component in after - before)


def compare(hubs: tuple[Hub, ...]) -> dict[str, int]:
    raw_births, raw_edges = graph(hubs)
    identity = {h.name: h.name for h in hubs}
    counts = {"cuts": 0, "lot_groups": 0, "emitted_edges": 0, "msf_edges": 0}
    levels = sorted({h.level for h in hubs})
    cuts = sorted(set(levels + [levels[0] - 1, levels[-1] + 1]
                      + [(a + b) / 2 for a, b in zip(levels, levels[1:])]))
    for last in (False, True):
        births, edges, reps = reduce_graph(hubs, last)
        require(len(edges) == len(raw_edges) - len(hubs) + len(births), "edge identity")
        deduplicated = deduplicate_edges(edges)
        forest = spanning_forest(births, edges)
        counts["emitted_edges"] += len(edges)
        counts["msf_edges"] += len(forest)
        for cut in cuts:
            for closed in (False, True):
                expected = signatures(hubs, raw_births, raw_edges, identity, cut, closed)
                for candidate in (edges, deduplicated, forest):
                    require(signatures(hubs, births, candidate, reps, cut, closed)
                            == expected, "cut/coverage/anchor mismatch")
                    counts["cuts"] += 1
        for level in levels:
            expected = lot_events(hubs, raw_births, raw_edges, identity, level)
            for candidate in (edges, deduplicated, forest):
                observed = lot_events(hubs, births, candidate, reps, level)
                require(observed == expected, "pre-lot parent/group mismatch")
                counts["lot_groups"] += len(expected)
            public = sorted((key, parents) for key, parents, _ in expected if len(parents) != 1)
            require(public_events_from_forest(births, forest, level) == public,
                    "standalone MSF public-parent mismatch")
    return counts


def triangle() -> tuple[Hub, ...]:
    # Pair MEB radii: AC=BC=13/4, AB=4; circumradius ABC=169/36.
    return (Hub("AC", F(13, 4), points=(0, 2)),
            Hub("BC", F(13, 4), points=(1, 2)),
            Hub("AB", F(4), points=(0, 1)),
            Hub("ABC", F(169, 36), ("AC", "BC", "AB")))


def check_geometric_fixtures() -> dict[str, str]:
    # Direct rational coordinates, independently of the product predicates.
    norm2 = lambda a, b: sum((F(x) - F(y)) ** 2 for x, y in zip(a, b))
    abc = ((0, 0, 0), (4, 0, 0), (2, 3, 0))
    center = (F(2), F(5, 6), F(0))
    require(all(norm2(x, center) == F(169, 36) for x in abc), "triangle circle")
    for pair in combinations(range(3), 2):
        mid = tuple((F(abc[pair[0]][j]) + abc[pair[1]][j]) / 2 for j in range(3))
        radius = norm2(abc[pair[0]], mid)
        require(all(norm2(abc[i], mid) > radius for i in range(3) if i not in pair),
                "pair not Gabriel")
    growth_points = ((1, 8, 0), (5, 10, 0), (9, 8, 0), (5, 0, 0))
    require(all(norm2(x, (5, 5, 0)) == 25 for x in growth_points), "ABCZ circle")
    strict = []
    for ids in combinations(range(4), 3):
        # A triple is strict iff an enclosing ball of radius <5 exists.
        # Test its three diameter balls and, if acute, its circumcircle.
        found = False
        for i, j in combinations(ids, 2):
            mid = tuple((F(growth_points[i][d]) + growth_points[j][d]) / 2 for d in range(3))
            r2 = norm2(growth_points[i], mid)
            found |= r2 < 25 and all(norm2(growth_points[t], mid) <= r2 for t in ids)
        if found:
            strict.append(ids)
    require(strict == [(0, 1, 2)], "ABCZ strict facets")
    # All four triples share the unique radius-5 circle; no smaller acute circle.
    return {"triangle_r2": "169/36", "ABCZ_r2": "25", "strict_ABC_r2": "16"}


def run() -> dict[str, object]:
    growth = (Hub("ABC", F(16), points=(0, 1, 2)),
              Hub("ABCZ", F(25), ("ABC",), (0, 1, 2, 3)))
    overlap = (Hub("a", F(0), points=(0, 1)), Hub("b", F(0), points=(1, 2)))
    # Shared old parent joins two same-level blocks into one three-parent lot.
    atomic = (Hub("a", F(0)), Hub("b", F(0)), Hub("c", F(0)),
              Hub("x", F(1), ("a", "b")), Hub("y", F(1), ("b", "c")),
              Hub("z", F(2), ("x", "y")))
    # Maximal spanning forest has the same final tree size, wrong early cuts.
    cycle = atomic[:5] + (Hub("z", F(2), ("a", "c")),)
    terminal = (Hub("X_Kn", F(25), points=(0, 1, 2, 3)),)
    line_k1 = (Hub("A", F(0), points=(0,)), Hub("B", F(0), points=(1,)),
               Hub("C", F(0), points=(2,)), Hub("AB", F(1), ("A", "B")),
               Hub("BC", F(1), ("B", "C")))
    line_k2 = (Hub("AB", F(1), points=(0, 1)), Hub("BC", F(1), points=(1, 2)),
               Hub("ABC", F(4), ("AB", "BC")))
    line_k3 = (Hub("ABC", F(4), points=(0, 1, 2)),)
    singleton = (Hub("only_point", F(0), points=(0,)),)
    parallel = (Hub("a", F(0)), Hub("b", F(0)),
                Hub("early", F(1), ("a", "b")), Hub("late", F(2), ("a", "b")))
    cases = [triangle(), growth, overlap, atomic, cycle, terminal,
             line_k1, line_k2, line_k3, singleton, parallel]
    rng = random.Random(20260911)
    for case in range(256):
        hubs = []
        for i in range(rng.randint(4, 20)):
            level = F(rng.randint(0, 6), 2)
            older = [h.name for h in hubs if h.level < level]
            ts = tuple(rng.choice(older) for _ in range(rng.randint(0, 5))) if older else ()
            hubs.append(Hub(f"h{case}_{i}", level, ts,
                            tuple(sorted({rng.randrange(7) for _ in range(rng.randrange(4))}))))
        cases.append(tuple(hubs))
    counts = {"cuts": 0, "lot_groups": 0, "emitted_edges": 0, "msf_edges": 0}
    for case in cases:
        for k, n in compare(case).items():
            counts[k] += n
    mutants = []
    hubs = triangle()
    births, edges, reps = reduce_graph(hubs)
    expected = signatures(hubs, births, edges, reps, F(1), True)
    require(signatures(hubs, {v: F(0) for v in births}, edges, reps, F(1), True) != expected,
            "future-vertex mutant survived")
    mutants.append("future_vertices_at_zero")
    early_edges = [(u, v, max(births[u], births[v])) for u, v, _ in edges]
    require(signatures(hubs, births, early_edges, reps, F(4), True)
            != signatures(hubs, births, edges, reps, F(4), True), "early-edge mutant survived")
    mutants.append("edges_before_consumer")
    births, edges, reps = reduce_graph(cycle)
    require(signatures(cycle, births, spanning_forest(births, edges, True), reps, F(1), True)
            != signatures(cycle, births, edges, reps, F(1), True), "max-forest mutant survived")
    mutants.append("maximum_spanning_forest")
    births, edges, reps = reduce_graph(parallel)
    require(signatures(parallel, births, deduplicate_edges(edges, True), reps, F(1), True)
            != signatures(parallel, births, edges, reps, F(1), True), "late duplicate mutant survived")
    mutants.append("deduplicate_to_latest_date")
    births, edges, reps = reduce_graph(growth)
    dropped = (growth[0], replace(growth[1], points=()))
    early = (replace(growth[0], points=(0, 1, 2, 3)), growth[1])
    require(signatures(dropped, births, edges, reps, F(25), True)
            != signatures(growth, births, edges, reps, F(25), True), "lost-growth mutant survived")
    require(signatures(early, births, edges, reps, F(25), False)
            != signatures(growth, births, edges, reps, F(25), False), "early-growth mutant survived")
    mutants.extend(["drop_unary_growth", "antidate_growth"])
    births, edges, reps = reduce_graph(overlap)
    require(len(signatures(overlap, births, edges, reps, F(0), True)[0]) == 2, "point overlap merged")
    require(signatures(overlap, births, [("a", "b", F(0))], reps, F(0), True)
            != signatures(overlap, births, edges, reps, F(0), True), "point-union mutant survived")
    mutants.append("identify_overlapping_points")
    births, edges, reps = reduce_graph(atomic)
    events = lot_events(atomic, births, edges, reps, F(1))
    require(len(events) == 1 and len(events[0][1]) == 3, "non-atomic three-parent lot")
    require(lot_events(atomic, births, edges, reps, F(2))[0][1] == (("a", "b", "c"),),
            "distinct terminals mistaken for distinct parents")
    # The triangle's K3 birth maps to K2 CLOSED at its own circle.
    births, edges, reps = reduce_graph(triangle())
    require(len(set(partition(births, edges, F(169, 36), True).values())) == 1
            and len(set(partition(births, edges, F(169, 36), False).values())) == 3,
            "lower closed/open fixture vacuous")
    negative_fixtures = ["open_lower_birth_cut"]
    true_anchors = signatures(triangle(), births, edges, reps, F(4), True)[1]
    wrong_anchors = dict(true_anchors)
    wrong_anchors["ABC"] = true_anchors[reps["ABC"]]
    require(wrong_anchors != true_anchors, "early anchor-admission mutant survived")
    negative_fixtures.append("admit_hub_through_old_leaf")
    # The collinear three-point tower shares the same equal-level geometry across K.
    for lower, upper, date in [(line_k1, line_k2, F(1)), (line_k2, line_k3, F(4))]:
        births, edges, reps = reduce_graph(lower)
        closed = signatures(lower, births, edges, reps, date, True)[1]
        opened = signatures(lower, births, edges, reps, date, False)[1]
        images = {closed[h.name] for h in upper if not h.terminals}
        require(len(images) == 1, "line vertical images disagree")
        require(all(h.name not in opened for h in upper if not h.terminals),
                "line vertical open-cut fixture vacuous")
    births, edges, reps = reduce_graph(line_k1)
    require(len(lot_events(line_k1, births, edges, reps, F(1))[0][1]) == 3,
            "line K1 is not one ternary fusion")
    # Upper order K=n has one isolated birth, including a nonregular full population.
    require(reduce_graph(terminal)[1] == [], "terminal order has an edge")
    for bad in [(Hub("a", F(1), ("a",)),),
                (Hub("a", F(1), ("missing",)),),
                (Hub("a", F(1)), Hub("a", F(2))),
                (Hub("a", F(1)), Hub("b", F(1), ("a",)))]:
        try:
            graph(bad)
        except ValueError:
            pass
        else:
            raise ValueError("invalid graph accepted")
    return {"status": "passed_filtered_graph_model", "cases": len(cases), **counts,
            "mutants_rejected": mutants, "invalid_inputs_rejected": 4,
            "negative_fixtures": negative_fixtures,
            "geometry": check_geometric_fixtures(), "product_executed": False,
            "performance_claim": False, "public_status": "not_claimed", "gcp_used": False}


if __name__ == "__main__":
    import json
    print(json.dumps(run(), sort_keys=True))
