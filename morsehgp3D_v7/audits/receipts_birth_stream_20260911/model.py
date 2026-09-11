"""Bounded sequential model: immutable online phi, DSU on native births only.

The reference is offline Kruskal on the entire hub graph, independently
projected afterwards. All graph data are synthetic: no geometry, product code,
parallel backend, timing or RAM qualification is claimed. The total edge order
is (date, original source key, local occurrence); projected endpoints do not
replace that original order.
"""

from __future__ import annotations

from dataclasses import dataclass, replace
from fractions import Fraction
import json
import sys


class Rejection(RuntimeError):
    pass


def need(value: bool, reason: str) -> None:
    if not value:
        raise Rejection(reason)


@dataclass(frozen=True)
class Hub:
    identity: int
    date: int
    key: int
    terminals: tuple[int, ...]


@dataclass(frozen=True)
class Case:
    name: str
    interpretation: str
    hubs: tuple[Hub, ...]


@dataclass(frozen=True)
class Record:
    hub: int
    date: int
    key: int
    degree: int
    local: int = -1  # -1 is the explicit hub header, including zero-degree births.
    terminal: int = -1
    ordinal: int = -1


@dataclass(frozen=True)
class Edge:
    ordinal: int
    date: int
    source_key: int
    local: int
    a: int
    b: int


class NativeDSU:
    def __init__(self) -> None:
        self.parent: dict[int, int] = {}

    def add(self, identity: int) -> None:
        need(identity not in self.parent, "duplicate_native_birth")
        self.parent[identity] = identity

    def find(self, identity: int) -> int:
        need(identity in self.parent, "unknown_native_identity")
        while self.parent[identity] != identity:
            self.parent[identity] = self.parent[self.parent[identity]]
            identity = self.parent[identity]
        return identity

    def connect(self, a: int, b: int) -> bool:
        a, b = self.find(a), self.find(b)
        if a == b:
            return False
        self.parent[max(a, b)] = min(a, b)
        return True


class Online:
    """Chronological variant allocating native DSU entries at birth headers.

    Valid strict terminals never need a future entry. This is equivalent for
    the checked filtration to predeclaring births, but is not that allocation
    protocol. The tested/retained lists below are materialized diagnostics.
    """

    def __init__(self, corrupt_phi_with_find: bool = False) -> None:
        self.phi: dict[int, int] = {}
        self.hub_dates: dict[int, int] = {}
        self.births: dict[int, int] = {}
        self.native = NativeDSU()
        self.active: Record | None = None
        self.next_local = 0
        self.previous_source: tuple[int, int] | None = None
        self.R = 0
        self.pivots = 0
        self.tested: list[Edge] = []
        self.retained: list[Edge] = []
        self.corrupt_phi_with_find = corrupt_phi_with_find

    def consume(self, record: Record) -> None:
        if record.local == -1:
            need(self.active is None or self.next_local == self.active.degree,
                 "incomplete_previous_hub")
            need(record.degree >= 0 and record.ordinal == -1 and record.terminal == -1,
                 "invalid_hub_header")
            source = record.date, record.key
            need(self.previous_source is None or self.previous_source < source,
                 "source_date_key_regression")
            need(record.hub not in self.hub_dates, "duplicate_hub_identity")
            self.previous_source = source
            self.hub_dates[record.hub] = record.date
            self.active, self.next_local = record, 0
            if record.degree == 0:
                self.phi[record.hub] = record.hub
                self.births[record.hub] = record.date
                self.native.add(record.hub)
            return
        need(self.active is not None, "occurrence_without_hub_header")
        header = self.active
        # Every occurrence is dated EXACTLY at its source hub's birth.
        # Mere endpoint admission would not force the pivot to arrive first.
        need((record.hub, record.date, record.key, record.degree)
             == (header.hub, header.date, header.key, header.degree),
             "occurrence_source_mismatch")
        need(record.local == self.next_local and record.local < header.degree,
             "local_occurrence_before_pivot_or_out_of_order")
        need(record.ordinal == self.R, "occurrence_ordinal_regression")
        need(record.terminal in self.hub_dates, "terminal_not_previously_admitted")
        need(self.hub_dates[record.terminal] < record.date, "terminal_not_strictly_earlier")
        need(record.terminal in self.phi, "terminal_phi_unavailable")
        terminal_native = self.phi[record.terminal]
        if record.local == 0:
            need(record.hub not in self.phi, "pivot_rewrites_phi")
            self.phi[record.hub] = (self.native.find(terminal_native)
                                    if self.corrupt_phi_with_find else terminal_native)
            self.pivots += 1
        else:
            edge = Edge(record.ordinal, record.date, record.key, record.local,
                        self.phi[record.hub], terminal_native)
            self.tested.append(edge)
            if self.native.connect(edge.a, edge.b):
                self.retained.append(edge)
        self.R += 1
        self.next_local += 1

    def finish(self) -> dict[str, int]:
        need(self.active is None or self.next_local == self.active.degree, "unfinished_final_hub")
        need(len(self.phi) == len(self.hub_dates), "missing_native_assignment")
        A, L = len(self.hub_dates), len(self.births)
        C = len({self.native.find(vertex) for vertex in self.births})
        need(self.pivots == A - L, "pivot_accounting")
        need(len(self.tested) == self.R - A + L, "nonpivot_accounting")
        need(len(self.retained) == L - C, "retained_accounting")
        need(len(self.native.parent) == L, "native_dsu_domain")
        return {"R": self.R, "A": A, "L": L, "C": C,
                "pivots": self.pivots, "nonpivot_tests": len(self.tested),
                "retained": len(self.retained), "native_dsu_vertices": L,
                "phi_entries": A}


def records_for(case: Case) -> tuple[Record, ...]:
    records = []
    ordinal = 0
    for hub in case.hubs:
        records.append(Record(hub.identity, hub.date, hub.key, len(hub.terminals)))
        for local, terminal in enumerate(hub.terminals):
            records.append(Record(hub.identity, hub.date, hub.key, len(hub.terminals),
                                  local, terminal, ordinal))
            ordinal += 1
    return tuple(records)


def offline_reference(case: Case) -> tuple[dict[int, int], dict[int, int],
                                         tuple[Edge, ...], tuple[Edge, ...], tuple[Edge, ...]]:
    """Full hub Kruskal with a separate union representation and offline phi."""
    hubs = {hub.identity: hub for hub in case.hubs}
    need(len(hubs) == len(case.hubs), "reference_duplicate_hub")
    need(tuple(sorted(case.hubs, key=lambda h: (h.date, h.key))) == case.hubs,
         "reference_unordered_hubs")
    need(len({(hub.date, hub.key) for hub in case.hubs}) == len(hubs),
         "reference_duplicate_source_key")

    def native_of(identity: int) -> int:
        hub = hubs[identity]
        while hub.terminals:
            previous = hubs[hub.terminals[0]]
            need(previous.date < hub.date, "reference_nonstrict_pivot")
            hub = previous
        return hub.identity

    phi = {identity: native_of(identity) for identity in hubs}
    births = {hub.identity: hub.date for hub in case.hubs if not hub.terminals}
    edges = []
    for hub in case.hubs:
        for local, terminal in enumerate(hub.terminals):
            need(terminal in hubs and hubs[terminal].date < hub.date,
                 "reference_nonstrict_terminal")
            edges.append(Edge(len(edges), hub.date, hub.key, local, hub.identity, terminal))
    # Intentionally independent from NativeDSU and the online consumer.
    component = {identity: frozenset({identity}) for identity in hubs}
    selected = []
    for edge in sorted(edges, key=lambda e: (e.date, e.source_key, e.local, e.ordinal)):
        left, right = component[edge.a], component[edge.b]
        if left != right:
            selected.append(edge)
            union = left | right
            for identity in union:
                component[identity] = union
    projected = tuple(replace(edge, a=phi[edge.a], b=phi[edge.b]) for edge in edges)
    selected_nonpivots = tuple(replace(edge, a=phi[edge.a], b=phi[edge.b])
                              for edge in selected if edge.local > 0)
    need(sum(edge.local == 0 for edge in selected) == len(hubs) - len(births),
         "reference_dropped_pivot")
    return phi, births, tuple(edges), projected, selected_nonpivots


def partition(births: dict[int, int], edges: tuple[Edge, ...] | list[Edge],
              cut: Fraction, closed: bool) -> tuple[tuple[int, ...], ...]:
    """Independent BFS, no sorted edges, no DSU and no phi lookup."""
    def active(date: int) -> bool:
        return date <= cut if closed else date < cut

    adjacency = {identity: set() for identity, date in births.items() if active(date)}
    for edge in edges:
        if active(edge.date) and edge.a in adjacency and edge.b in adjacency:
            adjacency[edge.a].add(edge.b)
            adjacency[edge.b].add(edge.a)
    unseen = set(adjacency)
    groups = []
    while unseen:
        first = min(unseen)
        unseen.remove(first)
        pending = [first]
        group = []
        while pending:
            identity = pending.pop()
            group.append(identity)
            for neighbor in sorted(adjacency[identity]):
                if neighbor in unseen:
                    unseen.remove(neighbor)
                    pending.append(neighbor)
        groups.append(tuple(sorted(group)))
    return tuple(sorted(groups))


def cuts_for(case: Case) -> tuple[Fraction, ...]:
    dates = sorted({hub.date for hub in case.hubs})
    if not dates:
        return (Fraction(0),)
    cuts = {Fraction(date) for date in dates}
    cuts.update((Fraction(dates[0] - 1), Fraction(dates[-1] + 1)))
    cuts.update(Fraction(a + b, 2) for a, b in zip(dates, dates[1:]))
    return tuple(sorted(cuts))


def windowed(records: tuple[Record, ...], width: int,
             corrupt_phi_with_find: bool = False) -> tuple[Online, dict[str, int]]:
    need(width > 0, "nonpositive_window")
    consumer = Online(corrupt_phi_with_find)
    windows = split_inside = split_after_pivot = 0
    maximum = 0
    for start in range(0, len(records), width):
        window = records[start:start + width]
        windows += 1
        maximum = max(maximum, len(window))
        if start and records[start - 1].hub == window[0].hub:
            split_inside += 1
            split_after_pivot += records[start - 1].local == 0 and window[0].local == 1
        for record in window:
            consumer.consume(record)
    return consumer, {**consumer.finish(), "window_width": width, "windows": windows,
                      "max_window_records": maximum,
                      "boundaries_inside_hub": split_inside,
                      "boundaries_after_pivot": split_after_pivot}


def qualify(case: Case) -> dict[str, object]:
    phi, births, full_edges, full_projection, selected = offline_reference(case)
    original_births = {hub.identity: hub.date for hub in case.hubs}
    raw_nonpivots = tuple(edge for edge in full_projection if edge.local > 0)
    records = records_for(case)
    runs = []
    comparisons = 0
    for width in (1, 7, 31):
        online, costs = windowed(records, width)
        need(online.phi == phi, "native_phi_identity_mismatch")
        need(online.births == births, "explicit_native_births_mismatch")
        need(tuple(online.tested) == raw_nonpivots, "direct_nonpivot_edge_identity_mismatch")
        need(tuple(online.retained) == selected, "projected_hub_kruskal_edge_identity_mismatch")
        for cut in cuts_for(case):
            for closed in (False, True):
                expected = partition(births, full_projection, cut, closed)
                hub_components = partition(original_births, full_edges, cut, closed)
                hub_native_components = tuple(sorted(
                    tuple(identity for identity in component if identity in births)
                    for component in hub_components))
                need(all(hub_native_components), "active_hub_component_without_native_birth")
                need(hub_native_components == expected, "hub_to_native_filtration_mismatch")
                need(partition(online.births, online.tested, cut, closed) == expected,
                     "direct_candidates_filtration_mismatch")
                need(partition(online.births, online.retained, cut, closed) == expected,
                     "direct_certificate_filtration_mismatch")
                comparisons += 3
        runs.append(costs)
    return {"name": case.name, "interpretation": case.interpretation,
            "hubs": len(case.hubs), "records": len(records),
            "cut_values": len(cuts_for(case)), "partition_comparisons": comparisons,
            "phi": sorted(phi.items()), "native_births": sorted(births.items()),
            "retained_edge_ids": [edge.ordinal for edge in selected], "runs": runs}


def fixed_corpus() -> tuple[Case, ...]:
    return (
        Case("k1_explicit_points", "K1-like explicit point births; synthetic incidence",
             (Hub(0, 0, 0, ()), Hub(1, 0, 1, ()), Hub(2, 0, 2, ()),
              Hub(3, 1, 3, (0, 1)), Hub(4, 2, 4, (1, 2)), Hub(5, 3, 5, (3, 4)))),
        Case("plateau_disjoint_and_duplicate_terminals", "equal-date sources, no same-date terminal",
             tuple(Hub(i, 0, i, ()) for i in range(5))
             + (Hub(10, 2, 10, (0, 1)), Hub(11, 2, 11, (1, 2)),
                Hub(12, 2, 12, (3, 4)), Hub(13, 3, 13, (10, 11, 10, 11)),
                Hub(20, 8, 20, ()))),
        Case("strict_silent_chain_and_future_birth", "degree-one pivots and a future isolated birth",
             (Hub(0, 0, 0, ()), Hub(1, 1, 1, (0,)), Hub(2, 2, 2, (1,)),
              Hub(3, 3, 3, ()), Hub(4, 4, 4, (2, 3)), Hub(5, 10, 5, ()))),
        Case("find_is_not_native_phi", "wrong native identity can retain every threshold component",
             (Hub(0, 0, 0, ()), Hub(1, 0, 1, ()), Hub(2, 0, 2, ()),
              Hub(3, 1, 3, (0, 1)), Hub(4, 2, 4, (1, 2)), Hub(5, 3, 5, (4, 4)))),
        Case("terminal_k_equals_n", "terminal order represented by one explicit full-population birth",
             (Hub(40, 7, 40, ()),)),
        Case("k1_singleton", "single point, no occurrence and no artificial edge",
             (Hub(90, 0, 90, ()),)),
    )


def deterministic_corpus() -> tuple[Case, ...]:
    """Eight fixed formula-generated cases, not a random/fuzz campaign."""
    cases = []
    for recipe in range(8):
        hubs = [Hub(i, 0, i, ()) for i in range(3 + recipe % 3)]
        identity = 20
        for date in range(1, 7):
            prior = tuple(hub.identity for hub in hubs)
            fresh = []
            for slot in range(2):
                degree = 1 + (recipe + date + slot) % 4
                terminals = tuple(prior[(recipe + date * 7 + slot * 3 + j * j) % len(prior)]
                                  for j in range(degree))
                if slot == 1 and date in (2, 4) and (recipe + date) % 3 == 0:
                    terminals = ()
                elif degree > 1 and (recipe + date + slot) % 2 == 0:
                    terminals = terminals[:-1] + (terminals[0],)
                fresh.append(Hub(identity, date, (recipe * 17 + identity * 13) % 997, terminals))
                identity += 1
            hubs.extend(sorted(fresh, key=lambda hub: hub.key))
        hubs.append(Hub(identity, 9, identity, ()))  # Preserved, future and isolated.
        cases.append(Case(f"fixed_chronology_{recipe}", "fixed bounded chronological recipe", tuple(hubs)))
    return tuple(cases)


def causal_rejections(cases: tuple[Case, ...]) -> list[dict[str, object]]:
    by_name = {case.name: case for case in cases}
    rejected = []

    def reject_stream(name: str, records: tuple[Record, ...], expected: str) -> None:
        try:
            windowed(records, 1)
        except Rejection as error:
            need(str(error) == expected, "unexpected_rejection_cause")
            rejected.append({"name": name, "cause": expected, "scope": "stream_guard"})
            return
        raise Rejection("stream_mutant_survived")

    base = records_for(by_name["find_is_not_native_phi"])
    first_occurrence = next(i for i, record in enumerate(base) if record.local == 0)
    mutant = list(base)
    mutant[first_occurrence] = replace(mutant[first_occurrence], local=1)
    reject_stream("local_one_before_pivot", tuple(mutant),
                  "local_occurrence_before_pivot_or_out_of_order")
    mutant = list(base)
    mutant[first_occurrence] = replace(mutant[first_occurrence], terminal=4)
    reject_stream("future_terminal", tuple(mutant), "terminal_not_previously_admitted")
    plateau = list(records_for(by_name["plateau_disjoint_and_duplicate_terminals"]))
    same_date = next(i for i, record in enumerate(plateau) if record.hub == 11 and record.local == 0)
    plateau[same_date] = replace(plateau[same_date], terminal=10)
    reject_stream("same_date_terminal", tuple(plateau), "terminal_not_strictly_earlier")
    mutant = list(base)
    mutant[first_occurrence] = replace(mutant[first_occurrence], ordinal=-1)
    reject_stream("occurrence_ordinal_regression", tuple(mutant), "occurrence_ordinal_regression")
    mutant = list(base)
    mutant[first_occurrence] = replace(mutant[first_occurrence], date=mutant[first_occurrence].date + 1)
    reject_stream("occurrence_after_source_birth", tuple(mutant), "occurrence_source_mismatch")
    mutant = list(base)
    header = next(i for i, record in enumerate(mutant) if record.hub == 4 and record.local == -1)
    mutant[header] = replace(mutant[header], date=0)
    reject_stream("source_date_regression", tuple(mutant), "source_date_key_regression")

    identity_case = by_name["find_is_not_native_phi"]
    correct, _ = windowed(base, 7)
    wrong, _ = windowed(base, 7, corrupt_phi_with_find=True)
    need(correct.phi[4] == 1 and wrong.phi[4] == 0, "phi_find_fixture_vacuous")
    need(correct.tested != wrong.tested, "phi_find_edge_fixture_vacuous")
    cuts = 0
    for cut in cuts_for(identity_case):
        for closed in (False, True):
            need(partition(correct.births, correct.retained, cut, closed)
                 == partition(wrong.births, wrong.retained, cut, closed),
                 "phi_find_fixture_must_preserve_components")
            cuts += 1
    rejected.append({"name": "replace_native_phi_by_find", "cause": "native_phi_identity_mismatch",
                     "scope": "identity_gate_despite_equal_partitions", "hub": 4,
                     "expected_native": correct.phi[4], "wrong_native": wrong.phi[4],
                     "equal_open_closed_partitions": cuts})

    birth_case = by_name["strict_silent_chain_and_future_birth"]
    good, _ = windowed(records_for(birth_case), 31)
    forgotten = dict(good.births)
    del forgotten[5]
    backdated = dict(good.births)
    backdated[5] = 0
    for name, bad in (("omit_future_isolated_birth", forgotten),
                      ("discard_future_birth_date", backdated)):
        witness = None
        for cut in cuts_for(birth_case):
            for closed in (False, True):
                expected = partition(good.births, good.retained, cut, closed)
                actual = partition(bad, good.retained, cut, closed)
                if actual != expected:
                    witness = {"cut": str(cut), "closed": closed,
                               "expected": expected, "actual": actual}
                    break
            if witness is not None:
                break
        need(witness is not None, "birth_mutant_vacuous")
        rejected.append({"name": name, "cause": "birth_filtration_mismatch",
                         "scope": "independent_BFS", "witness": witness})
    need(len(rejected) == 9, "rejection_nonvacuity")
    return rejected


def selftest() -> dict[str, object]:
    corpus = fixed_corpus() + deterministic_corpus()
    cases = [qualify(case) for case in corpus]
    rejected = causal_rejections(corpus)
    need(len(cases) == 14, "corpus_nonvacuity")
    need(sum(run["boundaries_after_pivot"] for case in cases for run in case["runs"]) > 0,
         "mid_hub_window_nonvacuity")
    first_runs = [case["runs"][0] for case in cases]
    totals = {name: sum(run[name] for run in first_runs)
              for name in ("R", "A", "L", "C", "pivots", "nonpivot_tests", "retained")}
    need(totals["pivots"] == totals["A"] - totals["L"], "total_pivot_accounting")
    need(totals["nonpivot_tests"] == totals["R"] - totals["A"] + totals["L"],
         "total_nonpivot_accounting")
    need(totals["retained"] == totals["L"] - totals["C"], "total_retained_accounting")
    return {
        "status": "passed_bounded_sequential_birth_stream_model",
        "corpus_cases": len(cases), "window_runs": sum(len(case["runs"]) for case in cases),
        "window_widths": [1, 7, 31],
        "partition_comparisons": sum(case["partition_comparisons"] for case in cases),
        "partition_counter_scope": "three independent BFS comparisons per cut side and window run; mutants excluded",
        "causal_rejections": len(rejected),
        "cost_totals_once_per_case": totals,
        "cost_symbols": {"R": "terminal occurrences", "A": "all source hubs including explicit K1 point births",
                         "L": "native births, including future and isolated births", "C": "final native components"},
        "total_order": "date, original source key, local occurrence; globally stable occurrence ordinal",
        "occurrence_date": "exactly the source hub birth date; every terminal birth is strictly earlier",
        "native_identity": "immutable phi; DSU find never replaces the native birth identity",
        "memory_scope": "native DSU has L entries and phi has A entries; Python object memory and materialized fixtures not qualified",
        "native_allocation": "lazy at explicit zero-degree birth headers; future birth entries are not preallocated in this model",
        "diagnostic_storage": "input, offline reference and every tested nonpivot edge remain materialized; no executed streaming residence qualification",
        "not_claimed": ["geometric census", "product OrderedReducer", "parallel execution", "speedup", "RAM in bytes"],
        "cases": cases, "rejections": rejected,
    }


def main(argv: list[str]) -> int:
    if argv != ["--selftest"]:
        return 2
    try:
        print(json.dumps(selftest(), indent=2, sort_keys=True))
    except Rejection as error:
        print(json.dumps({"status": "failed", "cause": str(error)}, sort_keys=True), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
