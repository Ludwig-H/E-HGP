#!/usr/bin/env python3
"""One rational n=12/K=7 terminal witness; no C++ or performance authority."""

from __future__ import annotations

import hashlib
import itertools
import json
from pathlib import Path
import sys

AUDIT = Path(__file__).resolve().parents[1]
ORACLE = AUDIT / "meb_rational_oracle_20260905.py"
ORACLE_SHA256 = "ad6c0d6c041ff788180a400f6ba2ad2b1546f8607e8f2c91fefca9133a8e7f2b"
if hashlib.sha256(ORACLE.read_bytes()).hexdigest() != ORACLE_SHA256:
    raise RuntimeError("rational oracle pin mismatch before import")
CONSULTED_HEADER = {
    "path": "morsehgp3D_v7/audits/receipts_full_successor_20260905/"
            "source/morsehgp3D_v7/src/forest/full_gabriel.hpp",
    "sha256": "85c27ab91d7f159520a8db3098629447b0a213a134c5c042a86c585416847fad",
    "role": "immutable provenance consulted for model; neither imported nor executed",
}
sys.path.insert(0, str(AUDIT))
from meb_rational_oracle_20260905 import circumball, power  # noqa: E402

POINTS = [
    (369, 371, 209), (310, 285, 311), (224, 279, 139), (9, 190, 147),
    (340, 128, 40), (9, 236, 62), (170, 369, 100), (381, 30, 25),
    (199, 210, 329), (331, 246, 156), (255, 98, 249), (94, 275, 332),
]
N, K = len(POINTS), 7


def require(value: bool, message: str) -> None:
    if not value:
        raise RuntimeError(message)


def label(mask: int) -> tuple[int, ...]:
    return tuple(i for i in range(N) if mask & (1 << i))


def mask_of(values: tuple[int, ...]) -> int:
    return sum(1 << i for i in values)


def morton(point: tuple[int, int, int]) -> int:
    return sum(((point[j] >> i) & 1) << (3 * i + j)
               for i in range(16) for j in range(3))


def main() -> None:
    # Exhaust all possible positive supports in R^3. Every MEB has one;
    # every global closed set of one is its unique Gabriel label.
    candidates, direct_all, meb_cache = [], {}, {}
    for q in range(2, 5):
        for support in itertools.combinations(range(N), q):
            ball = circumball([POINTS[i] for i in support])
            if ball is None or not ball["positive"]:
                continue
            powers = [power(ball, point) for point in POINTS]
            sm = mask_of(support)
            shell = mask_of(tuple(i for i, value in enumerate(powers) if value == 0))
            require(shell == sm, "global extra shell in positive support enumeration")
            closed = mask_of(tuple(i for i, value in enumerate(powers) if value <= 0))
            item = {"support": support, "sm": sm, "closed": closed,
                    "radius": ball["radius"]}
            candidates.append(item)
            require(closed not in direct_all, "two positive bases for one regular MEB")
            direct_all[closed] = item

    def meb(key: int) -> dict:
        if key not in meb_cache:
            admissible = [item for item in candidates
                          if item["sm"] & key == item["sm"]
                          and item["closed"] & key == key]
            require(len(admissible) == 1, "MEB must have one regular positive support")
            meb_cache[key] = admissible[0]
        return meb_cache[key]

    minima = {key: item for key, item in direct_all.items() if key.bit_count() == K}
    directs = {key: item for key, item in direct_all.items() if key.bit_count() == K + 1}
    facets = [mask_of(x) for x in itertools.combinations(range(N), K)]
    cofaces = [mask_of(x) for x in itertools.combinations(range(N), K + 1)]
    # These labels are all evaluated, including non-Gabriel cells, for Gamma.
    for key in facets + cofaces:
        meb(key)
    require({x for x in facets if meb(x)["closed"] == x} == set(minima),
            "exhaustive cardinal-K catalogue closure")
    require({x for x in cofaces if meb(x)["closed"] == x} == set(directs),
            "exhaustive cardinal-(K+1) catalogue closure")
    morton_order = sorted(range(N), key=lambda i: morton(POINTS[i]))
    levels = sorted({x["radius"] for x in list(minima.values()) + list(directs.values())})
    gamma_cache = {}

    def gamma(level):
        """H0 by DFS on all active K-facets and all active (K+1)-cofaces."""
        if level in gamma_cache:
            return gamma_cache[level]
        active = {f for f in facets if meb(f)["radius"] < level}
        graph = {f: set() for f in active}
        edges = 0
        for co in cofaces:
            if meb(co)["radius"] >= level:
                continue
            boundary = [co ^ (1 << i) for i in label(co)]
            require(all(f in active for f in boundary), "Gamma active boundary")
            # A star realizes this coface's equivalence without mirroring
            # the producer's Gabriel catalogue, descent or union schedule.
            for f in boundary[1:]:
                graph[boundary[0]].add(f)
                graph[f].add(boundary[0])
                edges += 1
        components, membership = [], {}
        unseen = set(active)
        while unseen:
            todo, component = [min(unseen)], set()
            while todo:
                f = todo.pop()
                if f in component:
                    continue
                component.add(f)
                todo.extend(graph[f] - component)
            unseen -= component
            leaves = frozenset(component & minima.keys())
            require(bool(leaves), "nonempty Gamma component has a Gabriel minimum")
            components.append(leaves)
            for f in component:
                membership[f] = leaves
        answer = (frozenset(components), membership, len(active), edges)
        gamma_cache[level] = answer
        return answer

    def simulate(mutant: bool = False):
        successor, tokens, node_leaves, first, visits, batches = [], {}, [], {}, [], []
        gamma_cuts, first_causal = 0, None
        stats = {"portal_requests": 0, "chain_steps": 0, "j1": 0,
                 "terminal_chain_visits": 0, "reuses": 0}

        def root(token):
            seen = set()
            while successor[token] != token:
                require(token not in seen, "successor cycle")
                seen.add(token)
                token = successor[token]
            return token

        for level in levels:
            births = sorted((x for x in minima if minima[x]["radius"] == level), key=label)
            cons = sorted((x for x in directs if directs[x]["radius"] == level), key=label)
            gamma_parts, membership, active_count, edge_count = gamma(level)
            model_parts = frozenset(node_leaves[i] for i in range(len(successor))
                                    if successor[i] == i)
            if not mutant:
                require(model_parts == gamma_parts, "all pre-lot components differ from Gamma H0")
                gamma_cuts += 1
            groups, direct_tokens, requests = [], {}, []
            for co in cons:
                roots = []
                for removed in directs[co]["support"]:
                    facet = co ^ (1 << removed)
                    if facet in minima:
                        roots.append(root(tokens[facet]))
                        continue
                    stats["portal_requests"] += 1
                    ball = meb(facet)
                    require(ball["radius"] < level, "strict facet")
                    foreign = [i for i in morton_order if ball["closed"] & (1 << i)
                               and not facet & (1 << i)]
                    require(bool(foreign), "missing minimum")
                    key = facet | (1 << foreign[0])
                    if len(foreign) == 1:
                        require(key in directs, "J1 terminal exists")
                        require(directs[key]["radius"] == ball["radius"], "J1 level")
                        roots.append(root(tokens[key]))
                        stats["j1"] += 1
                        continue  # Deliberately no memo seed from J1.
                    intruder, trace = foreign[1], []
                    while True:
                        stats["chain_steps"] += 1
                        previous = ball["radius"]
                        essential = ball["support"][0]
                        require(key & (1 << essential), "essential in current coface")
                        key = (key ^ (1 << essential)) | (1 << intruder)
                        ball = meb(key)
                        trace.append(label(key))
                        require(ball["radius"] < previous, "strict descent")
                        if key in directs:
                            require(directs[key]["radius"] == ball["radius"], "terminal exact level")
                            require(ball["radius"] < level and key in tokens, "terminal prior")
                            current = root(tokens[key])
                            used = first[key]["root"] if mutant and key in first else current
                            stats["terminal_chain_visits"] += 1
                            stats["reuses"] += int(key in first)
                            if not mutant:
                                require(node_leaves[current] == membership[facet],
                                        "terminal component disagrees with exhaustive Gamma")
                                for i in label(key):
                                    require(membership[key ^ (1 << i)] == membership[facet],
                                            "terminal boundary belongs to same Gamma component")
                            entry = {"direct": label(co), "facet": label(facet),
                                     "terminal": label(key), "root": current, "used_root": used,
                                     "level": str(level), "chain": trace,
                                     "component_minima": [label(f) for f in sorted(node_leaves[current])],
                                     "gamma_active_facets": active_count, "gamma_star_edges": edge_count}
                            if key in first and used != current and first_causal is None:
                                first_causal = {"first": first[key], "repeated": entry}
                            visits.append(entry)
                            first.setdefault(key, entry)
                            roots.append(used)
                            break
                        foreign = [i for i in morton_order if ball["closed"] & (1 << i)
                                   and not key & (1 << i)]
                        require(bool(foreign), "missing terminal")
                        intruder = foreign[0]
                groups.append(set(roots))
                direct_tokens[co] = roots[0]
                requests.append({"direct": label(co), "roots": roots})
            merged = []
            for group in groups:
                at = 0
                while at < len(merged):
                    if group & merged[at]:
                        group |= merged.pop(at)
                        at = 0
                    else:
                        at += 1
                merged.append(group)
            for birth in births:
                tokens[birth] = len(successor)
                successor.append(len(successor))
                node_leaves.append(frozenset({birth}))
            merges = []
            for group in sorted(merged, key=lambda x: sorted(x)):
                if len(group) < 2:
                    continue
                new = len(successor)
                successor.append(new)
                node_leaves.append(frozenset().union(*(node_leaves[x] for x in group)))
                for old in group:
                    successor[old] = new
                merges.append(sorted(group))
            for co, token in direct_tokens.items():
                tokens[co] = root(token)
            batches.append({"level": str(level), "births": [label(x) for x in births],
                            "merges": merges, "requests": requests,
                            "successor": list(successor)})
        return {"stats": stats, "visits": visits, "batches": batches,
                "successor": successor, "gamma_cuts": gamma_cuts,
                "mutant_first_causal": first_causal}

    nominal, faulty = simulate(), simulate(True)
    changed = [(a, b) for a, b in zip(nominal["batches"], faulty["batches"])
               if a["merges"] != b["merges"] or a["successor"] != b["successor"]]
    require(nominal["stats"] == {"portal_requests": 11, "chain_steps": 3,
                                "j1": 9, "terminal_chain_visits": 2, "reuses": 1},
            "exact non-vacuity P11/C3/J9/T2/R1")
    require(len({tuple(v["terminal"]) for v in nominal["visits"]}) == 1,
            "exact unique terminal count U1")
    require(faulty["mutant_first_causal"] is not None and bool(changed),
            "cached-root mutant must change parent IDs or the successor table")
    require(any(a["merges"] != b["merges"] for a, b in changed), "mutant changes actual merge parents")
    require(any(a["successor"] != b["successor"] for a, b in changed), "mutant changes successor state")
    source_paths = [Path(__file__), ORACLE]
    result = {
        "status": "passed", "scope": "rational_fixture_and_model_only_no_cpp_no_timing",
        "point_ids": "zero-based positions in points", "points": POINTS, "k": K,
        "cache_capacity": 0, "root_ids": "model numbering; no compiled correspondence claimed",
        "global_regular_positive_supports": len(candidates),
        "exhaustive_gamma_facets": len(facets), "exhaustive_gamma_cofaces": len(cofaces),
        "minimum_catalogue": [label(x) for x in sorted(minima, key=label)],
        "direct_catalogue": [label(x) for x in sorted(directs, key=label)],
        "morton_point_order": morton_order, "nominal": nominal,
        "mutant": {"name": "return_cached_root_without_normalizing_current_token",
                   "status": "refuted", "first_causal": faulty["mutant_first_causal"],
                   "first_changed_lot": {"nominal": changed[0][0], "mutant": changed[0][1]},
                   "changed_lots": len(changed)},
        "pins": {str(p.relative_to(AUDIT.parent.parent)): hashlib.sha256(p.read_bytes()).hexdigest()
                 for p in source_paths},
        "consulted_header": CONSULTED_HEADER,
        "public_status": "not_claimed", "gcp": "not_used",
    }
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
