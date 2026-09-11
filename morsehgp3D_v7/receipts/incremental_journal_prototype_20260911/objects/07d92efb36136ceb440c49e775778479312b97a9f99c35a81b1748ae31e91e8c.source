"""Independent bounded inputs and expectations for the dated C++ journal.

Geometry and cut coverages come from the historical rational/Gamma model.
This module never imports product code, invokes C++, or writes a receipt.
"""

from __future__ import annotations

from collections import Counter, defaultdict
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import sys
from typing import Any


HERE = Path(__file__).resolve().parent
MODEL_DIR = HERE.parent / "receipts_plateaux_full_20260906"
if str(MODEL_DIR) not in sys.path:
    sys.path.insert(0, str(MODEL_DIR))

import coverage_contribution_model as historical  # noqa: E402


ABSENT = (1 << 64) - 1
Json = dict[str, Any]


def need(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def level_pair(numerator: int, denominator: int, factor: int = 1) -> list[Any]:
    numerator *= factor
    denominator *= factor
    need(0 <= numerator < 1 << 192, "level numerator fits U192")
    need(0 < denominator < 1 << 127, "positive level denominator fits i128")
    return [(numerator >> (64 * limb)) & ABSENT for limb in range(3)] + [str(denominator)]


def level(value: Fraction, factor: int = 1) -> list[Any]:
    return level_pair(value.numerator, value.denominator, factor)


def rational(raw: list[Any]) -> Fraction:
    return Fraction(sum(int(raw[limb]) << (64 * limb) for limb in range(3)), int(raw[3]))


def arena_expectation(order: int, batches: list[Json]) -> Json:
    """Encode the expected direct action records, without a coverage reader."""
    nodes: list[list[Any]] = []
    parents: list[int] = []
    successors: list[int] = []
    contributions: list[list[Any]] = []
    for batch in batches:
        for action in batch["actions"]:
            incoming = action["parents"]
            if len(incoming) == 1:
                segment = incoming[0]
            else:
                segment = len(nodes)
                nodes.append(batch["level"] + [len(parents), len(incoming)])
                successors.append(ABSENT)
                for parent in incoming:
                    need(parent < segment and successors[parent] == ABSENT,
                         "expected parent belongs to one later merge")
                    successors[parent] = segment
                parents.extend(incoming)
            contributions.extend(batch["level"] + [segment] + ref for ref in action["refs"])
    return dict(status=0, order=order, nodes=nodes, parents=parents,
                successors=successors, contributions=contributions, queries=[])


def query_expectation(node_count: int, coverage: dict[int, frozenset[int]],
                      ancestry: dict[int, set[int]]) -> Json:
    """Assign ancestors from live roots; never chase product-style successors."""
    root_of: dict[int, int] = {}
    for root in coverage:
        for ancestor in ancestry[root]:
            need(ancestor not in root_of, "live components have disjoint node ancestries")
            root_of[ancestor] = root
    queried = list(range(node_count + 1)) + [ABSENT]
    return dict(
        roots=[root_of.get(token, ABSENT) for token in queried],
        reads=[[0, sorted(coverage[token])] if token in coverage else [1, []]
               for token in queried],
    )


def geometric_case(name: str, model: Any, state: Any, order: int,
                   population_keys: list[tuple[int, ...]], cuts: list[tuple[Fraction, bool]],
                   stream_name: str, factor: int) -> Json:
    populations = {key: index for index, key in enumerate(population_keys)}
    rows = [dict(interior=sorted(model.balls[key]["interior"]),
                 shell=sorted(model.balls[key]["shell"])) for key in population_keys]
    stream = getattr(state, stream_name)
    by_activation: dict[tuple[Fraction, int], list[Any]] = defaultdict(list)
    for contribution in stream:
        by_activation[contribution.radius, contribution.token].append(contribution)

    original_ids = list(range(len(state.nodes)))
    if order == 1:
        singleton: dict[int, int] = {}
        for token, node in enumerate(state.nodes):
            if node.radius == 0:
                records = by_activation[node.radius, token]
                need(not node.parents and len(records) == 1, "K1 has one population per initial root")
                population = model.balls[records[0].ball]["closed"]
                need(len(population) == 1, "K1 zero root is a singleton")
                singleton[token] = next(iter(population))
        original_ids.sort(key=lambda token: (state.nodes[token].radius,
                                            singleton.get(token, token)))
    remap = {old: new for new, old in enumerate(original_ids)}

    activations = set(radius for radius, _ in by_activation)
    activations.update(node.radius for node in state.nodes)
    batches = []
    created: list[int] = []
    for radius in sorted(activations):
        tokens = {token for activation, token in by_activation if activation == radius}
        tokens.update(token for token, node in enumerate(state.nodes) if node.radius == radius)
        actions = []
        for token in sorted(tokens, key=remap.__getitem__):
            node = state.nodes[token]
            refs = []
            for item in by_activation[radius, token]:
                row = rows[populations[item.ball]]
                include = bool(item.include_interior and row["interior"])
                need(include or item.shell_mask, "only nonempty contributions are exported")
                refs.append([populations[item.ball], item.shell_mask, include])
            if node.radius == radius:
                incoming = sorted(remap[parent] for parent in node.parents)
                created.append(remap[token])
                if not incoming:
                    need(len(refs) == 1, "one immutable whole population for each geometric birth")
            else:
                if not refs:
                    continue
                incoming = [remap[token]]
            actions.append(dict(parents=incoming, refs=refs))
        if actions:
            batches.append(dict(level=level(radius, factor), actions=actions))
    need(created == list(range(len(state.nodes))), "node remapping follows exported action order")

    expected = arena_expectation(order, batches)
    ancestry: dict[int, set[int]] = {}
    for old in original_ids:
        token = remap[old]
        ancestry[token] = {token}
        for parent in state.nodes[old].parents:
            ancestry[token].update(ancestry[remap[parent]])
    queries = []
    for cut, closed in cuts:
        observed = historical.read_cut(model, state.nodes, stream, cut, closed)
        coverage = {remap[token]: points for token, points in observed.items()}
        queries.append(dict(level=level(cut), closed=closed))
        expected["queries"].append(query_expectation(len(state.nodes), coverage, ancestry))
    return dict(name=f"{name}/K{order}/{stream_name}/factor{factor}", domain=list(model.ids),
                rows=rows, order=order, batches=batches, queries=queries, expected=expected)


def structural_case(wide: bool, factor: int) -> Json:
    rows = [dict(interior=[0], shell=[1]), dict(interior=[], shell=[1, 2]),
            dict(interior=[], shell=[3, 4]), dict(interior=[], shell=[4, 5]),
            dict(interior=[6], shell=[7]), dict(interior=[], shell=list(range(10)))]
    numerator_base = (1 << 180) if wide else 0
    denominator = (1 << 120) - 1 if wide else 1

    def raw(offset: int) -> list[Any]:
        return level_pair(numerator_base + offset, denominator, factor)

    batches = [
        dict(level=raw(1), actions=[
            dict(parents=[], refs=[[index, (1 << len(rows[index]["shell"])) - 1,
                                     bool(rows[index]["interior"])]]) for index in range(4)]),
        dict(level=raw(2), actions=[
            dict(parents=[], refs=[[4, 1, True]]),
            dict(parents=[3], refs=[[5, 1 << 8, False], [5, 1 << 8, False]]),
            dict(parents=[0, 1, 2], refs=[[5, 1 << 9, False]]),
        ]),
        dict(level=raw(3), actions=[dict(parents=[4], refs=[[5, 1, False]])]),
        dict(level=raw(4), actions=[dict(parents=[3, 4, 5], refs=[])]),
        dict(level=raw(5), actions=[dict(parents=[6], refs=[[5, 1023, False], [5, 256, False]])]),
    ]
    expected = arena_expectation(2, batches)
    need(len(expected["nodes"]) == 7 and len(expected["parents"]) == 6,
         "structural fixture has two ternary fusions and five births")

    # Independent forward replay with explicit point and ancestor sets. These
    # large levels are compared as Python rationals, not as fixed-width limbs.
    queries = []
    for twice_offset in range(13):
        cut = Fraction(2 * numerator_base + twice_offset, 2 * denominator)
        for closed in (False, True):
            active: dict[int, frozenset[int]] = {}
            ancestry: dict[int, set[int]] = {}
            next_token = 0
            for batch in batches:
                value = rational(batch["level"])
                if value > cut or (value == cut and not closed):
                    break
                prelot = set(active)
                consumed: set[int] = set()
                for action in batch["actions"]:
                    incoming = action["parents"]
                    need(set(incoming) <= prelot and not (set(incoming) & consumed),
                         "mixed fixture uses disjoint prelot roots")
                    consumed.update(incoming)
                    points: set[int] = set()
                    lineage: set[int] = set()
                    for parent in incoming:
                        points.update(active.pop(parent))
                        lineage.update(ancestry[parent])
                    for population, mask, include in action["refs"]:
                        row = rows[population]
                        if include:
                            points.update(row["interior"])
                        points.update(point for bit, point in enumerate(row["shell"])
                                      if mask & (1 << bit))
                    if len(incoming) == 1:
                        token = incoming[0]
                    else:
                        token = next_token
                        next_token += 1
                    lineage.add(token)
                    ancestry[token] = lineage
                    active[token] = frozenset(points)
            queries.append(dict(level=level(cut), closed=closed))
            expected["queries"].append(query_expectation(7, active, ancestry))
    return dict(name=f"structural_mixed/{'wide' if wide else 'small'}/factor{factor}",
                domain=list(range(10)), rows=rows, order=2, batches=batches,
                queries=queries, expected=expected)


def build_corpus() -> tuple[list[Json], Json]:
    pins = dict(historical.PINS)
    pins.update({
        "coverage_contribution_model.py": "a0b713954a1af606db06277f0d0bd39669ad2002595ff829382bb6de7bede908",
        "coverage_contribution_normal.json": "479e0b38103da9041841bdfa50309c9e90acce3bde9aaba362d63a991e505460",
    })
    for name, expected in pins.items():
        need(hashlib.sha256((MODEL_DIR / name).read_bytes()).hexdigest() == expected,
             "historical source pin: " + name)
    source = json.loads((MODEL_DIR / "coverage_contribution_normal.json").read_text())
    cases = []
    totals: Counter[str] = Counter()
    runs = {}
    for name, points in sorted(source["clouds"].items()):
        model = historical.Model([tuple(point) for point in points])
        kmax = (2 if name == "arity_window_tetra_Kmax2" else
                5 if name == "window_shell7_Kmax5" else len(model.ids))
        final, snapshots, production = historical.produce(model, kmax)
        judged = historical.verify(model, final, snapshots)
        totals.update(judged["counters"])
        totals["geometry_orders"] += kmax
        totals["geometry_clouds"] += 1
        # One canonical bank per cloud, reused identically for every order and
        # encoding variant. It contains only populations actually referenced.
        population_keys = sorted({row.ball for state in final for row in state.whole})
        cuts = sorted(snapshots)
        for order, state in enumerate(final, 1):
            for stream_name in ("gap", "whole"):
                for factor in (1, 2):
                    cases.append(geometric_case(name, model, state, order, population_keys,
                                                cuts, stream_name, factor))
        runs[name] = dict(n=len(model.ids), kmax=kmax, population_rows=len(population_keys),
                          production=dict(production), verification=judged["counters"],
                          partition_digest=judged["partition_digest"])
    totals["geometric_cases"] = len(cases)
    need(totals["geometry_clouds"] == 10 and totals["geometry_orders"] == 45 and
         len(cases) == 180, "historical corpus nonvacuity")
    need(totals["order_cuts"] == 718 and totals["active_facets"] == 2588 and
         totals["live_component_coverages"] == 779, "historical Gamma counts are unchanged")
    for wide in (False, True):
        for factor in (1, 2):
            cases.append(structural_case(wide, factor))
    totals["structural_cases"] = 4
    totals["cpp_cases"] = len(cases)
    totals["cpp_queries"] = sum(len(case["queries"]) for case in cases)
    totals["cpp_root_queries"] = sum(len(query["roots"]) for case in cases
                                    for query in case["expected"]["queries"])
    totals["cpp_coverage_queries"] = totals["cpp_root_queries"]
    return cases, dict(counters=dict(totals), runs=runs,
                       geometric_cuts="historical read_cut after independent Gram/Gamma verify",
                       structural_cuts="separate forward point-set and ancestor-set replay",
                       population_bank="same canonical BallKey populations across orders; the transport builds a bank instance per case",
                       level_encodings=["reduced_fraction", "same_fraction_times_two"],
                       wide_fixture="(2^180+i)/(2^120-1), also numerator and denominator times two")


if __name__ == "__main__":
    corpus, statistics = build_corpus()
    print(json.dumps(dict(cases=corpus, stats=statistics), sort_keys=True, separators=(",", ":")))
