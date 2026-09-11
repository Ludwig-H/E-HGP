"""Bounded sequential witness for composable, dated MSF certificates.

No product imports, threads, timing, random corpus, or filesystem writes.
The measured edge slots describe the model's explicit input/certificate
buffers, not Python memory: fixture storage, sort scratch, DSU and object
overhead are excluded. Vertex identities and birth dates are never sparsified.
"""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass
from fractions import Fraction
import json


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


@dataclass(frozen=True)
class Vertex:
    name: str
    birth: int


@dataclass(frozen=True)
class Edge:
    name: str
    u: str
    v: str
    weight: int


@dataclass(frozen=True)
class Graph:
    name: str
    vertices: tuple[Vertex, ...]
    edges: tuple[Edge, ...]


def validate(graph: Graph) -> None:
    births = {v.name: v.birth for v in graph.vertices}
    require(len(births) == len(graph.vertices), "duplicate vertex identity")
    require(len({e.name for e in graph.edges}) == len(graph.edges),
            "duplicate edge identity; parallel edges need distinct identities")
    for edge in graph.edges:
        require(edge.u in births and edge.v in births, "unknown endpoint")
        require(edge.weight >= max(births[edge.u], births[edge.v]),
                "edge predates an endpoint; ordinary MSF theorem inapplicable")


def edge_key(edge: Edge) -> tuple[int, str, str, str]:
    return edge.weight, min(edge.u, edge.v), max(edge.u, edge.v), edge.name


def msf(vertices: tuple[Vertex, ...], edges: tuple[Edge, ...],
        ordered_by_weight: bool = True) -> tuple[Edge, ...]:
    """Sequential Kruskal REFERENCE; false selects the deliberate bad mutant."""
    parents = {vertex.name: vertex.name for vertex in vertices}

    def root(name: str) -> str:
        while parents[name] != name:
            parents[name] = parents[parents[name]]
            name = parents[name]
        return name

    selected = []
    candidates = sorted(edges, key=edge_key) if ordered_by_weight else edges
    for edge in candidates:
        u, v = root(edge.u), root(edge.v)
        if u != v:
            parents[v] = u
            selected.append(edge)
    return tuple(selected)


def active(level: int, cut: Fraction, closed: bool) -> bool:
    return level <= cut if closed else level < cut


def partition(vertices: tuple[Vertex, ...], edges: tuple[Edge, ...],
              cut: Fraction, closed: bool,
              birth_closed: bool | None = None) -> tuple[tuple[str, ...], ...]:
    """Independent BFS oracle; it neither sorts edges nor uses a DSU."""
    if birth_closed is None:
        birth_closed = closed
    adjacency: dict[str, set[str]] = {
        vertex.name: set() for vertex in vertices
        if active(vertex.birth, cut, birth_closed)
    }
    for edge in edges:
        if (active(edge.weight, cut, closed)
                and edge.u in adjacency and edge.v in adjacency):
            adjacency[edge.u].add(edge.v)
            adjacency[edge.v].add(edge.u)
    unseen = set(adjacency)
    groups = []
    while unseen:
        initial = min(unseen)
        unseen.remove(initial)
        pending = [initial]
        found = []
        while pending:
            vertex = pending.pop()
            found.append(vertex)
            for neighbor in sorted(adjacency[vertex]):
                if neighbor in unseen:
                    unseen.remove(neighbor)
                    pending.append(neighbor)
        groups.append(tuple(sorted(found)))
    return tuple(sorted(groups))


def cuts_for(graph: Graph) -> tuple[Fraction, ...]:
    dates = sorted({v.birth for v in graph.vertices}
                   | {e.weight for e in graph.edges})
    if not dates:
        return (Fraction(0),)
    cuts = {Fraction(date) for date in dates}
    cuts.update((Fraction(dates[0] - 1), Fraction(dates[-1] + 1)))
    cuts.update(Fraction(a + b, 2) for a, b in zip(dates, dates[1:]))
    return tuple(sorted(cuts))


def mismatch(graph: Graph, vertices: tuple[Vertex, ...],
             edges: tuple[Edge, ...]) -> dict[str, object] | None:
    for cut in cuts_for(graph):
        for closed in (False, True):
            expected = partition(graph.vertices, graph.edges, cut, closed)
            actual = partition(vertices, edges, cut, closed)
            if expected != actual:
                return {"cut": str(cut), "closed": closed,
                        "expected": expected, "actual": actual}
    return None


def events(vertices: tuple[Vertex, ...], edges: tuple[Edge, ...]
           ) -> tuple[tuple[int, tuple[tuple[str, ...], ...]], ...]:
    """Canonical simultaneous multifusions; births are independent records."""
    output = []
    for weight in sorted({edge.weight for edge in edges}):
        cut = Fraction(weight)
        before = partition(vertices, edges, cut, False, birth_closed=True)
        after = partition(vertices, edges, cut, True, birth_closed=True)
        for component in after:
            members = set(component)
            children = tuple(c for c in before if set(c) <= members)
            if len(children) >= 2:
                output.append((weight, children))
    return tuple(sorted(output))


@dataclass
class Costs:
    local_calls: int = 0
    local_input_edges: int = 0
    local_output_edges: int = 0
    merge_calls: int = 0
    merge_input_edges: int = 0
    merge_output_edges: int = 0
    max_resident_edge_slots: int = 0

    def observe(self, slots: int) -> None:
        self.max_resident_edge_slots = max(self.max_resident_edge_slots, slots)

    def local(self, vertices: tuple[Vertex, ...], batch: tuple[Edge, ...],
              retained: int) -> tuple[Edge, ...]:
        result = msf(vertices, batch)
        self.local_calls += 1
        self.local_input_edges += len(batch)
        self.local_output_edges += len(result)
        # The incoming raw batch and the emitted local certificate coexist.
        self.observe(retained + len(batch) + len(result))
        return result

    def merge(self, vertices: tuple[Vertex, ...], left: tuple[Edge, ...],
              right: tuple[Edge, ...], retained: int) -> tuple[Edge, ...]:
        combined = left + right
        result = msf(vertices, combined)
        self.merge_calls += 1
        self.merge_input_edges += len(combined)
        self.merge_output_edges += len(result)
        # Input certificate arrays, explicit concatenation, and result coexist.
        # Each array entry counts once, even when edge objects are shared.
        self.observe(retained + len(left) + len(right)
                     + len(combined) + len(result))
        return result


def compose(vertices: tuple[Vertex, ...], batches: tuple[tuple[Edge, ...], ...],
            strategy: str) -> tuple[tuple[Edge, ...], Costs]:
    costs = Costs()
    if strategy == "balanced":
        current: deque[tuple[Edge, ...]] = deque()
        retained = 0
        for batch in batches:
            local = costs.local(vertices, batch, retained)
            current.append(local)
            retained += len(local)
        while len(current) > 1:
            following: deque[tuple[Edge, ...]] = deque()
            following_slots = 0
            current_slots = sum(map(len, current))
            while len(current) >= 2:
                left, right = current.popleft(), current.popleft()
                current_slots -= len(left) + len(right)
                merged = costs.merge(vertices, left, right,
                                     current_slots + following_slots)
                following.append(merged)
                following_slots += len(merged)
            following.extend(current)
            current = following
        return (current[0] if current else ()), costs
    if strategy == "fold":
        accumulated: tuple[Edge, ...] = ()
        started = False
        for batch in batches:
            local = costs.local(vertices, batch, len(accumulated))
            if started:
                accumulated = costs.merge(vertices, accumulated, local, 0)
            else:
                accumulated, started = local, True
        return accumulated, costs
    require(strategy == "binary_flush", "unknown composition strategy")
    buckets: dict[int, tuple[Edge, ...]] = {}
    for batch in batches:
        carry = costs.local(vertices, batch, sum(map(len, buckets.values())))
        level = 0
        while level in buckets:
            previous = buckets.pop(level)
            carry = costs.merge(vertices, previous, carry,
                                sum(map(len, buckets.values())))
            level += 1
        buckets[level] = carry
    accumulated = ()
    started = False
    for level in sorted(tuple(buckets)):
        certificate = buckets.pop(level)
        if started:
            accumulated = costs.merge(vertices, accumulated, certificate,
                                      sum(map(len, buckets.values())))
        else:
            accumulated, started = certificate, True
    return accumulated, costs


def make_graph(name: str, births: tuple[tuple[str, int], ...],
               rows: tuple[tuple[str, str, int], ...]) -> Graph:
    graph = Graph(name, tuple(Vertex(*row) for row in births),
                  tuple(Edge(f"e{i}", *row) for i, row in enumerate(rows)))
    validate(graph)
    return graph


def corpus() -> tuple[Graph, ...]:
    return (
        make_graph("unordered_weighted_cycle", (("a", 0), ("b", 0), ("c", 0)),
                   (("a", "b", 9), ("b", "c", 8), ("a", "c", 1))),
        make_graph("split_ternary_plateau", (("a", 0), ("b", 0), ("c", 0)),
                   (("a", "b", 4), ("b", "c", 4), ("a", "c", 4))),
        make_graph("disjoint_same_date", tuple((x, 0) for x in "abcde"),
                   (("a", "b", 4), ("c", "d", 4), ("d", "e", 4),
                    ("b", "e", 8))),
        make_graph("isolated_and_future", (("a", 0), ("b", 2), ("z", 9)),
                   (("a", "b", 3),)),
        make_graph("parallel_edges_and_loops", tuple((x, 0) for x in "abc"),
                   (("a", "a", 0), ("a", "b", 6), ("a", "b", 2),
                    ("b", "a", 2), ("b", "c", 5), ("c", "c", 1),
                    ("a", "c", 3), ("a", "c", 3))),
        make_graph("late_lighter_edge", tuple((x, 0) for x in "abc"),
                   (("a", "b", 10), ("b", "c", 10), ("a", "c", 1))),
        make_graph("strictly_dated_birth_graph",
                   (("a", -2), ("b", 0), ("c", 3), ("d", 1), ("z", 20)),
                   (("a", "d", 8), ("b", "c", 6), ("a", "b", 2),
                    ("d", "c", 7), ("b", "d", 4), ("a", "c", 9))),
        make_graph("edge_at_endpoint_birth", (("a", 0), ("b", 4), ("z", 4)),
                   (("a", "b", 4),)),
        make_graph("isolated_only", (("a", 0), ("b", 7)), ()),
        make_graph("empty_graph", (), ()),
    )


def batches_for(edges: tuple[Edge, ...], width: int, reverse: bool
                ) -> tuple[tuple[Edge, ...], ...]:
    # Include empty batches deliberately. Reverse the batches and the edge
    # order inside each batch without changing the full edge multiset.
    batches = ((),) + tuple(edges[i:i + width] for i in range(0, len(edges), width)) + ((),)
    if reverse:
        return tuple(tuple(reversed(batch)) for batch in reversed(batches))
    return batches


def qualify(graph: Graph) -> dict[str, object]:
    validate(graph)
    reference = msf(graph.vertices, graph.edges)
    reference_events = events(graph.vertices, graph.edges)
    require(mismatch(graph, graph.vertices, reference) is None, "reference MSF fails")
    widths = sorted({1, 2, 3, max(1, len(graph.edges))})
    records = []
    comparisons = 0
    for width in widths:
        for reverse in (False, True):
            batches = batches_for(graph.edges, width, reverse)
            local_union = tuple(edge for batch in batches for edge in msf(graph.vertices, batch))
            require(mismatch(graph, graph.vertices, local_union) is None,
                    "union of local MSFs loses filtration")
            require(events(graph.vertices, local_union) == reference_events,
                    "local certificate union changes global multifusions")
            comparisons += 2 * len(cuts_for(graph))
            for strategy in ("balanced", "fold", "binary_flush"):
                certificate, costs = compose(graph.vertices, batches, strategy)
                require(mismatch(graph, graph.vertices, certificate) is None,
                        f"{graph.name}: {strategy} loses filtration")
                require(events(graph.vertices, certificate) == reference_events,
                        "composed certificate changes global multifusions")
                require(set(certificate) <= set(graph.edges), "certificate fabricates an edge")
                require(certificate == reference,
                        "stable global tie order must preserve exact canonical edge identities")
                require(len(certificate) == len(reference), "wrong final forest cardinality")
                require(sum(e.weight for e in certificate) == sum(e.weight for e in reference),
                        "wrong final forest weight")
                require(costs.local_input_edges == len(graph.edges), "input count mismatch")
                require(costs.local_calls == len(batches), "local calls count mismatch")
                require(costs.merge_calls == max(0, len(batches) - 1),
                        "merge reduction count mismatch")
                comparisons += 2 * len(cuts_for(graph))
                records.append({"batch_width": width, "reverse": reverse,
                                "strategy": strategy, "batches": len(batches),
                                "final_edges": len(certificate), **vars(costs)})
    return {"name": graph.name, "vertices": len(graph.vertices),
            "input_edges": len(graph.edges), "cut_values": len(cuts_for(graph)),
            "partition_comparisons": comparisons, "global_events": reference_events,
            "variants": records}


def mutants(graphs: tuple[Graph, ...]) -> list[dict[str, object]]:
    by_name = {graph.name: graph for graph in graphs}
    rejected = []

    def reject(name: str, graph: Graph, vertices: tuple[Vertex, ...],
               edges: tuple[Edge, ...]) -> None:
        witness = mismatch(graph, vertices, edges)
        require(witness is not None, f"vacuous mutant: {name}")
        rejected.append({"name": name, "witness": witness})

    graph = by_name["unordered_weighted_cycle"]
    reject("arbitrary_local_maximal_forest", graph, graph.vertices,
           msf(graph.vertices, graph.edges, ordered_by_weight=False))

    graph = by_name["isolated_and_future"]
    incident = {x for edge in graph.edges for x in (edge.u, edge.v)}
    reject("discard_isolated_vertices", graph,
           tuple(vertex for vertex in graph.vertices if vertex.name in incident),
           msf(graph.vertices, graph.edges))

    graph = by_name["late_lighter_edge"]
    reject("backdate_certificate_weights", graph, graph.vertices,
           tuple(Edge(edge.name, edge.u, edge.v, 0) for edge in msf(graph.vertices, graph.edges)))

    plateau = by_name["split_ternary_plateau"]
    local_events = tuple(sorted(event for edge in plateau.edges
                                for event in events(plateau.vertices, (edge,))))
    global_events = events(plateau.vertices, plateau.edges)
    require(len(global_events) == 1 and len(global_events[0][1]) == 3,
            "plateau fixture must be one ternary multifusion")
    require(local_events != global_events, "vacuous local-as-global event mutant")
    rejected.append({"name": "publish_local_fusions_as_global",
                     "witness": {"expected": global_events, "actual": local_events}})

    early = msf(graph.vertices, graph.edges[:-1])
    require(set(early) != set(msf(graph.vertices, graph.edges)),
            "late lighter edge must change the MSF")
    reject("discard_late_lighter_edge", graph, graph.vertices, early)
    return rejected


def projection_map(graph: Graph, pivots: dict[str, str]) -> dict[str, str]:
    """Pivots are separate, dated witnesses; never infer them from the MSF."""
    births = {vertex.name: vertex.birth for vertex in graph.vertices}
    for child, parent in pivots.items():
        require(child in births and parent in births, "unknown pivot endpoint")
        require(births[parent] < births[child], "pivot must descend strictly")
        require(any({edge.u, edge.v} == {child, parent}
                    and edge.weight == births[child] for edge in graph.edges),
                "pivot lacks its original connection witness at hub birth")
    mapping = {}
    for vertex in graph.vertices:
        current = vertex.name
        while current in pivots:
            current = pivots[current]
        mapping[vertex.name] = current
    return mapping


def project(graph: Graph, edges: tuple[Edge, ...], mapping: dict[str, str]) -> Graph:
    """Keep root births and original edge dates/IDs; quotient endpoints only."""
    projected = Graph(
        graph.name + "_projected",
        tuple(vertex for vertex in graph.vertices if mapping[vertex.name] == vertex.name),
        tuple(Edge(edge.name, mapping[edge.u], mapping[edge.v], edge.weight)
              for edge in edges),
    )
    validate(projected)
    return projected


def projection_corpus() -> tuple[tuple[Graph, dict[str, str]], ...]:
    return (
        (make_graph(
            "projection_changes_canonical_certificate",
            (("0", 0), ("1", 0), ("2", 0), ("3", 1), ("4", 1)),
            (("0", "3", 1), ("0", "4", 1), ("1", "3", 1),
             ("1", "4", 1), ("2", "3", 1))),
         {"3": "2", "4": "0"}),
        (make_graph(
            "strict_pivot_chain_and_future_isolated_birth",
            (("a", 0), ("b", 2), ("z", 9),
             ("h1", 3), ("h2", 5), ("h3", 4)),
            (("a", "h1", 3), ("b", "h3", 4), ("h1", "h2", 5),
             ("h2", "h3", 7), ("b", "h2", 8), ("a", "b", 10))),
         {"h1": "a", "h2": "h1", "h3": "b"}),
        (make_graph(
            "projection_disjoint_plateau_with_duplicates",
            (("a", 0), ("b", 0), ("c", 0), ("d", 0), ("z", 6),
             ("h1", 2), ("h2", 2)),
            (("a", "h1", 2), ("b", "h1", 4), ("c", "h2", 2),
             ("d", "h2", 4), ("a", "b", 4), ("c", "d", 4),
             ("h1", "h1", 2))),
         {"h1": "a", "h2": "c"}),
    )


def qualify_projection(graph: Graph, pivots: dict[str, str]) -> dict[str, object]:
    validate(graph)
    mapping = projection_map(graph, pivots)
    source_certificate = msf(graph.vertices, graph.edges)
    require(mismatch(graph, graph.vertices, source_certificate) is None,
            "source hub MSF loses the source filtration")
    full_projection = project(graph, graph.edges, mapping)
    certificate_projection = project(graph, source_certificate, mapping)
    reduced_after_projection = msf(certificate_projection.vertices,
                                   certificate_projection.edges)
    reduced_full_projection = msf(full_projection.vertices, full_projection.edges)
    expected_events = events(full_projection.vertices, full_projection.edges)
    for candidate in (certificate_projection.edges, reduced_after_projection,
                      reduced_full_projection):
        require(mismatch(full_projection, full_projection.vertices, candidate) is None,
                "hub MSF then projection loses a birth-level threshold partition")
        require(events(full_projection.vertices, candidate) == expected_events,
                "hub MSF then projection changes global multifusions")
    if graph.name == "projection_changes_canonical_certificate":
        require(tuple(edge.name for edge in source_certificate)
                == ("e0", "e1", "e2", "e4"), "wrong upstream canonical fixture")
        require(tuple(edge.name for edge in reduced_after_projection) == ("e0", "e2"),
                "expected ac, bc after sparse projection")
        require(tuple(edge.name for edge in reduced_full_projection) == ("e3", "e0"),
                "expected ab, ac after full projection")
        require(reduced_after_projection != reduced_full_projection,
                "canonical projection counterexample became vacuous")
        require(expected_events == ((1, (("0",), ("1",), ("2",))),),
                "different certificates must retain the single ternary multifusion")
    return {
        "name": graph.name,
        "vertices_before": len(graph.vertices),
        "birth_vertices_after": len(full_projection.vertices),
        "input_edges": len(graph.edges),
        "source_certificate_edges": len(source_certificate),
        "projected_source_edges_including_loops": len(certificate_projection.edges),
        "final_certificate_edges": len(reduced_after_projection),
        "source_partition_comparisons": 2 * len(cuts_for(graph)),
        "projection_partition_comparisons": 6 * len(cuts_for(full_projection)),
        "pivots": pivots,
        "terminal_mapping": mapping,
        "preserved_births": tuple((vertex.name, vertex.birth)
                                  for vertex in full_projection.vertices),
        "source_certificate_ids": tuple(edge.name for edge in source_certificate),
        "sparse_projection_certificate_ids": tuple(edge.name for edge in reduced_after_projection),
        "full_projection_certificate_ids": tuple(edge.name for edge in reduced_full_projection),
        "same_canonical_certificate": reduced_after_projection == reduced_full_projection,
        "global_events": expected_events,
    }


def main() -> None:
    graphs = corpus()
    cases = [qualify(graph) for graph in graphs]
    rejected = mutants(graphs)
    require(len(cases) == 10 and len(rejected) == 5, "non-vacuity floor")
    projection_cases = [qualify_projection(graph, pivots)
                        for graph, pivots in projection_corpus()]
    require(len(projection_cases) == 3, "projection non-vacuity floor")
    strategies = ("balanced", "fold", "binary_flush")
    totals = {}
    for strategy in strategies:
        records = [record for case in cases for record in case["variants"]
                   if record["strategy"] == strategy]
        totals[strategy] = {
            "runs": len(records),
            "local_input_edges_sum": sum(record["local_input_edges"] for record in records),
            "local_output_edges_sum": sum(record["local_output_edges"] for record in records),
            "merge_input_edges_sum": sum(record["merge_input_edges"] for record in records),
            "merge_output_edges_sum": sum(record["merge_output_edges"] for record in records),
            "max_resident_edge_slots": max(record["max_resident_edge_slots"] for record in records),
        }
    output = {
        "status": "passed_bounded_sequential_model",
        "claim": "all threshold partitions, births and global multifusions preserved; exact canonical MSF edge identities under a stable global tie order on unchanged endpoints, on this corpus",
        "not_claimed": ["parallel backend", "product integration", "runtime speedup", "RAM bound in bytes", "identical certificates across endpoint projection"],
        "dates": "exact integer weights and Fraction cuts; open and closed",
        "memory_counter": "logical algorithm buffers: local raw batch, retained certificate arrays, merge concatenation and output slots; excludes fixture storage, Python sort scratch, DSU, interpreter aliases and object overhead",
        "corpus_cases": len(cases),
        "composition_runs": sum(len(case["variants"]) for case in cases),
        "partition_comparisons": sum(case["partition_comparisons"] for case in cases),
        "partition_counter_scope": "local certificate unions and final composed certificates against the original BFS; preliminary reference and mutant checks excluded",
        "reference_partition_comparisons": sum(2 * len(cuts_for(graph)) for graph in graphs),
        "projection_cases_count": len(projection_cases),
        "projection_source_partition_comparisons": sum(
            case["source_partition_comparisons"] for case in projection_cases),
        "projection_partition_comparisons": sum(
            case["projection_partition_comparisons"] for case in projection_cases),
        "mutants_rejected": len(rejected),
        "strategy_totals": totals,
        "cases": cases,
        "mutants": rejected,
        "projection_cases": projection_cases,
    }
    print(json.dumps(output, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
