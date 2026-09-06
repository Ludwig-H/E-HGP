"""Four-order rational tower model, with minima-only parent resolution.

Fixed certificates are consumed by hash. No geometry search, product helper,
C++ engine, direct-coface anchor cache, or benchmark is used.
"""
from fractions import Fraction as Q
from collections.abc import Iterable
from itertools import combinations
from pathlib import Path
from typing import Any
import hashlib
import json

BASE = Path(__file__).resolve().parent
SOURCE = BASE / "normal.json"
SOURCE_SHA = "7ac7608813cb853e361827a41b232c79beabb4c0da6f13c669cb09381aeb7021"


def require(ok: bool, why: str) -> None:
    if not ok:
        raise ValueError(why)


def fraction(raw: dict[str, int]) -> Q:
    return Q(raw["numerator"], raw["denominator"])


def joined(a: str, b: str) -> str:
    return "".join(sorted(set(a) | set(b)))


def components(vertices: Iterable[Any], edges: Iterable[tuple[Any, Any]]) -> list[frozenset[Any]]:
    graph = {v: set() for v in vertices}
    for a, b in edges:
        graph[a].add(b)
        graph[b].add(a)
    unseen, result = set(vertices), []
    while unseen:
        todo, group = [min(unseen)], set()
        while todo:
            v = todo.pop()
            if v in unseen:
                unseen.remove(v)
                group.add(v)
                todo.extend(graph[v])
        result.append(frozenset(group))
    return sorted(result, key=lambda group: sorted(group))


def cover(labels: Iterable[str]) -> str:
    return "".join(sorted(set("".join(labels))))


def run(source: dict[str, Any], reverse_choice: bool = False) -> dict[str, Any]:
    cert = source["certificates"]
    beta = {q: fraction(c["radius_squared"]) for q, c in cert.items()}
    gab = {q for q, c in cert.items() if c["gabriel"]}
    labels = sorted(cert)
    points = sorted(source["points_u16"])
    orders = range(1, 5)
    states = {k: dict(nodes=[], successor=[], minima={}) for k in orders}
    traces, batches, vertical = [], [], []
    counts = {k: dict(requests=0, minimum_hits=0, certificate_reads=0, descent_steps=0,
                      max_requested_cardinality=0) for k in orders}

    def normalize(k: int, token: int) -> int:
        successor = states[k]["successor"]
        while successor[token] != token:
            token = successor[token]
        return token

    def resolve(k: int, facet: str, cut: Q) -> int:
        counts[k]["requests"] += 1
        initial, path = facet, [facet]
        while facet not in states[k]["minima"]:
            require(facet not in gab, "missing prior minimum")
            c = cert[facet]
            counts[k]["certificate_reads"] += 1
            counts[k]["max_requested_cardinality"] = max(k, counts[k]["max_requested_cardinality"])
            intruders = [p for p in points if p not in facet and fraction(c["all_point_powers"][p]) < 0]
            require(bool(intruders), "non-Gabriel facet needs strict intruder")
            at = -1 if reverse_choice else 0
            z, removed = intruders[at], c["support"][at]
            following = joined(facet.replace(removed, ""), z)
            require(len(following) == k and beta[following] < beta[facet] < cut,
                    "strict same-cardinality descent before queried cut")
            require(beta[joined(facet, z)] == beta[facet], "same-level connecting coface")
            facet = following
            path.append(facet)
            counts[k]["descent_steps"] += 1
        require(beta[facet] < cut, "minimum must predate parent request")
        counts[k]["minimum_hits"] += 1
        root = normalize(k, states[k]["minima"][facet])
        traces.append(dict(order=k, source=initial, cut=str(cut), path=path,
                           terminal=facet, normalized_root=root))
        return root

    for value in sorted({beta[q] for q in gab}):
        events = sorted(q for q in gab if beta[q] == value)
        pending = {k: [] for k in orders}
        for event in events:
            k = len(event) - 1
            if k >= 1:
                refs = [resolve(k, event.replace(s, ""), value) for s in cert[event]["support"]]
                pending[k].append((event, set(refs)))
        batch = dict(level=str(value), events=events, orders=[])
        for k in orders:
            state = states[k]
            groups = components({r for _, refs in pending[k] for r in refs},
                                [(min(refs), r) for _, refs in pending[k] for r in refs if r != min(refs)])
            born, merged = [], []
            for label in (q for q in events if len(q) == k):
                token = len(state["nodes"])
                state["nodes"].append(dict(level=str(value), minimum=label, parents=[], minima=[label]))
                state["successor"].append(token)
                state["minima"][label] = token
                born.append(token)
            for group in sorted((sorted(g) for g in groups if len(g) >= 2)):
                token = len(state["nodes"])
                leaves = sorted({leaf for p in group for leaf in state["nodes"][p]["minima"]})
                state["nodes"].append(dict(level=str(value), minimum=None, parents=group, minima=leaves))
                state["successor"].append(token)
                for parent in group:
                    state["successor"][parent] = token
                merged.append(token)
            batch["orders"].append(dict(order=k, births=born, merges=merged))
        # These links are output-only vertical witnesses, never resolver state.
        for k in orders:
            for event, refs in pending[k]:
                vertical.append(dict(upper_order=k + 1, leaf=event, level=str(value),
                                     upper_birth=states[k + 1]["minima"][event],
                                     lower_closed_anchor=normalize(k, min(refs))))
        batches.append(batch)

    def snapshot(k: int, cut: Q, closed: bool) -> dict[int, frozenset[str]]:
        nodes = states[k]["nodes"]
        active = {i for i, n in enumerate(nodes) if (Q(n["level"]) <= cut if closed else Q(n["level"]) < cut)}
        consumed = {p for i in active for p in nodes[i]["parents"]}
        return {i: frozenset(nodes[i]["minima"]) for i in sorted(active - consumed)}

    observations, previous, previous_vertical = [], {}, {}
    comparisons = naturality = vertical_checks = vertical_squares = 0
    cuts = sorted((fraction(r["squared_level"]), r["side"] == "closed") for r in source["states"])
    for cut, closed in cuts:
        active = lambda q: beta[q] <= cut if closed else beta[q] < cut
        gammas, current, current_vertical = {}, {}, {}
        row = dict(level=str(cut), side="closed" if closed else "open", orders=[], vertical=[])
        for k in orders:
            vertices = [q for q in labels if len(q) == k and active(q)]
            gamma = components(vertices, [(a, b) for a, b in combinations(vertices, 2) if active(joined(a, b))])
            roots = snapshot(k, cut, closed)
            projected = {frozenset(group & gab) for group in gamma}
            require(all(projected) and projected == set(roots.values()), "Gamma vs tower minima partition")
            require(sorted(cover(g) for g in gamma) == sorted(cover(g) for g in roots.values()), "point coverage")
            for old in previous.get(k, []):
                require(sum(old <= new for new in projected) == 1, "horizontal naturality")
                naturality += 1
            previous[k] = projected
            gammas[k], current[k] = gamma, roots
            comparisons += 1
            row["orders"].append(dict(order=k, Gamma=[sorted(g) for g in gamma],
                                      roots=[dict(id=i, minima=sorted(g), point_cover=cover(g)) for i, g in roots.items()]))
        for k in range(2, 5):
            current_vertical[k] = {}
            for upper in current[k].values():
                target_roots = set()
                for leaf in upper:
                    link = next(v for v in vertical if v["upper_order"] == k and v["leaf"] == leaf)
                    old = set(states[k - 1]["nodes"][link["lower_closed_anchor"]]["minima"])
                    target_roots.update(i for i, leaves in current[k - 1].items() if old <= leaves)
                require(len(target_roots) == 1, "vertical leaf anchors need one current lower component")
                target = current[k - 1][next(iter(target_roots))]
                source_group = next(g for g in gammas[k] if g & gab == upper)
                boundary = {q.replace(p, "") for q in source_group for p in q}
                lower = [g for g in gammas[k - 1] if boundary <= g]
                require(len(lower) == 1 and lower[0] & gab == target, "vertical direct Gamma authority")
                current_vertical[k][upper] = target
                row["vertical"].append(dict(upper_order=k, upper_minima=sorted(upper), lower_minima=sorted(target)))
                vertical_checks += 1
            for old, target in previous_vertical.get(k, {}).items():
                newer = [g for g in current_vertical[k] if old <= g]
                require(len(newer) == 1 and target <= current_vertical[k][newer[0]], "vertical naturality square")
                vertical_squares += 1
        previous_vertical = current_vertical
        observations.append(row)
    require(comparisons == 76 and counts[2]["descent_steps"] > 0, "four orders and nonempty facet descent")
    require(all(c["certificate_reads"] == c["descent_steps"] for c in counts.values()), "lookup-before-certificate schedule")
    return dict(orders=states, global_batches=batches, vertical_birth_links=vertical, traces=traces,
                counts=counts, observations=observations, Gamma_comparisons=comparisons,
                horizontal_inclusions=naturality, vertical_component_checks=vertical_checks,
                vertical_naturality_squares=vertical_squares)


def main() -> None:
    require(hashlib.sha256(SOURCE.read_bytes()).hexdigest() == SOURCE_SHA, "fixed certificate receipt pin")
    source = json.loads(SOURCE.read_text())
    nominal, alternate = run(source), run(source, True)
    require(nominal["orders"] == alternate["orders"] and nominal["observations"] == alternate["observations"],
            "different descent choices must preserve tower and all cut components")
    require(nominal["traces"] != alternate["traces"], "choice independence must be non-vacuous")
    result = dict(status="passed", schema="mhgp7-rational-unified-tower-minima-descent-v1",
                  scope="n=4 fixed certificates, K1..4 rational topology model; no geometric search or product execution",
                  source_sha256=SOURCE_SHA, script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  nominal=nominal, alternative_choices=alternate,
                  authority="parent resolver reads only minimum tokens and per-order successors; vertical links are output-only",
                  limits="No physical MEB counters, performance, mass/vote, catalogue-generation or industrial qualification",
                  public_status="not_claimed", engine_calls=0, gcp="not_used")
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
