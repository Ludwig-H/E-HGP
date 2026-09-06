#!/usr/bin/env python3
"""Rational K9/K10 fixtures: positive supports, exhaustive bounded Gamma.

No C++ invocation. Only cardinal K and K+1 labels are evaluated; no Gamma
constructor or producer catalogue() is called. DFS partitions are independent
of the separate rational descent witness used to establish 11-site demand.
"""
from __future__ import annotations

import argparse
from fractions import Fraction as Q
import hashlib
import itertools
import json
from pathlib import Path
import sys

AUDIT = Path(__file__).resolve().parents[1]
ROOT = AUDIT.parent.parent
ORACLE = AUDIT / "meb_rational_oracle_20260905.py"
ORACLE_SHA = "ad6c0d6c041ff788180a400f6ba2ad2b1546f8607e8f2c91fefca9133a8e7f2b"
if hashlib.sha256(ORACLE.read_bytes()).hexdigest() != ORACLE_SHA:
    raise RuntimeError("rational MEB oracle pin changed")
sys.path.insert(0, str(AUDIT))
from meb_rational_oracle_20260905 import circumball, power  # noqa: E402
from full_cpp_audit import level, rational  # noqa: E402
from full_producer_audit import bounded_samples  # noqa: E402


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise RuntimeError(reason)


def mask_of(values) -> int:
    return sum(1 << i for i in values)


def morton(point) -> int:
    return sum(((point[j] >> i) & 1) << (3 * i + j)
               for i in range(16) for j in range(3))


def cloud(seed: int) -> list[tuple[int, int, int]]:
    state = seed
    values = []
    for _ in range(42):
        state = (1664525 * state + 1013904223) & 0xFFFFFFFF
        values.append(1 + (state >> 8) % 397)
    return [tuple(values[i:i + 3]) for i in range(0, 42, 3)]


class Geometry:
    def __init__(self, points):
        self.points = points
        self.n = len(points)
        require(len(set(points)) == self.n, "duplicate positions")
        self.candidates, self.direct_all, self.cache = [], {}, {}
        for q in range(2, 5):
            for support in itertools.combinations(range(self.n), q):
                ball = circumball([points[i] for i in support])
                if ball is None or not ball["positive"]:
                    continue
                powers = [power(ball, point) for point in points]
                sm = mask_of(support)
                shell = mask_of(i for i, value in enumerate(powers) if value == 0)
                require(shell == sm, "global extra shell")
                closed = mask_of(i for i, value in enumerate(powers) if value <= 0)
                item = dict(support=support, sm=sm, closed=closed, radius=ball["radius"])
                self.candidates.append(item)
                require(closed not in self.direct_all, "nonunique positive basis")
                self.direct_all[closed] = item
        self.morton_order = sorted(range(self.n), key=lambda i: morton(points[i]))

    def label(self, mask):
        return tuple(i for i in range(self.n) if mask & (1 << i))

    def meb(self, key):
        if key not in self.cache:
            found = [item for item in self.candidates
                     if item["sm"] & key == item["sm"]
                     and item["closed"] & key == key]
            require(len(found) == 1, "regular MEB not unique")
            self.cache[key] = found[0]
        return self.cache[key]

    def foreign(self, ball, key):
        return [i for i in self.morton_order
                if ball["closed"] & (1 << i) and not key & (1 << i)]

    def event(self, key):
        item = self.direct_all[key]
        support = list(item["support"])
        interior = [i for i in self.label(key) if i not in support]
        return dict(label=list(self.label(key)), q=len(support), d=len(interior),
                    mask=(1 << len(support)) - 1, support=support,
                    interior=interior, squared_level=str(item["radius"]))

    def path_witness(self, k):
        """C0 path model only; never supplies expected forest parents."""
        minima = {key for key in self.direct_all if key.bit_count() == k}
        direct = {key for key in self.direct_all if key.bit_count() == k + 1}
        visits = []
        portal_requests = singleton = chain_steps = 0
        for co in sorted(direct, key=lambda x: (self.meb(x)["radius"], self.label(x))):
            cut = self.meb(co)["radius"]
            for removed in self.meb(co)["support"]:
                facet = co ^ (1 << removed)
                if facet in minima:
                    continue
                portal_requests += 1
                ball = self.meb(facet)
                require(ball["radius"] < cut, "facet not strict")
                foreign = self.foreign(ball, facet)
                require(bool(foreign), "missing minimum")
                key = facet | (1 << foreign[0])
                if len(foreign) == 1:
                    require(key in direct and self.meb(key)["radius"] == ball["radius"],
                            "J1 catalogue authority")
                    singleton += 1
                    continue
                intruder = foreign[1]
                trace = []
                while True:
                    previous = ball["radius"]
                    key = (key ^ (1 << ball["support"][0])) | (1 << intruder)
                    ball = self.meb(key)
                    chain_steps += 1
                    require(key.bit_count() == k + 1 and ball["radius"] < previous,
                            "descent not strict or wrong cardinal")
                    trace.append(dict(label=list(self.label(key)), level=str(ball["radius"]),
                                      support=list(ball["support"])))
                    if key in direct:
                        require(ball["radius"] < cut, "terminal not prior")
                        visits.append(dict(direct=list(self.label(co)), facet=list(self.label(facet)),
                                           cut=str(cut), initial_level=str(self.meb(facet)["radius"]),
                                           trace=trace))
                        break
                    foreign = self.foreign(ball, key)
                    require(bool(foreign), "nonterminal without intruder")
                    intruder = foreign[0]
        return dict(portal_requests=portal_requests, chain_steps=chain_steps,
                    singleton_intruder_resolutions=singleton,
                    physical_meb_calls_model=portal_requests + chain_steps,
                    chain_visits=visits, scope="rational C0 model; no compiled observation")


class GammaOrder:
    def __init__(self, geo, k):
        self.geo, self.k = geo, k
        self.facets = [mask_of(c) for c in itertools.combinations(range(geo.n), k)]
        self.cofaces = [mask_of(c) for c in itertools.combinations(range(geo.n), k + 1)]
        self.levels = {key: geo.meb(key)["radius"] for key in self.facets + self.cofaces}
        self.minima = {f for f in self.facets if geo.meb(f)["closed"] == f}
        self.direct = {c for c in self.cofaces if geo.meb(c)["closed"] == c}
        require(self.minima == {x for x in geo.direct_all if x.bit_count() == k},
                "minimum catalogue not exhaustive")
        require(self.direct == {x for x in geo.direct_all if x.bit_count() == k + 1},
                "direct catalogue not exhaustive")
        self.boundary = {co: [co ^ (1 << i) for i in geo.label(co)] for co in self.cofaces}
        self.partitions = {}

    def partition(self, cut, closed):
        request = cut, closed
        if request in self.partitions:
            return self.partitions[request]
        active = (lambda value: value <= cut) if closed else (lambda value: value < cut)
        graph = {f: set() for f in self.facets if active(self.levels[f])}
        for co in self.cofaces:
            if not active(self.levels[co]):
                continue
            boundary = self.boundary[co]
            require(all(f in graph for f in boundary), "inactive Gamma boundary")
            for f in boundary[1:]:
                graph[boundary[0]].add(f)
                graph[f].add(boundary[0])
        groups, unseen = [], set(graph)
        while unseen:
            todo, component = [min(unseen)], set()
            while todo:
                facet = todo.pop()
                if facet in component:
                    continue
                component.add(facet)
                todo.extend(graph[facet] - component)
            unseen -= component
            groups.append(frozenset(component))
        self.partitions[request] = groups
        return groups

    def cover(self, component):
        mask = 0
        for facet in component:
            mask |= facet
        return frozenset(self.geo.label(mask))

    def forest(self):
        nodes, minima, parents, leaves, covers = [], [], [], [], []
        live = {}
        ordered = sorted({Q(0), *self.levels.values()})
        for value in ordered:
            before, after = self.partition(value, False), self.partition(value, True)
            require(set(before) == set(live), "pre-level Gamma partition")
            rows = [(image, sorted(live[c] for c in before if c <= image)) for image in after]
            births = sorted((image for image, refs in rows if not refs),
                            key=lambda c: tuple(sorted(self.geo.label(f) for f in c)))
            merges = sorted(((refs, image) for image, refs in rows if len(refs) >= 2),
                            key=lambda row: row[0])
            following = {image: refs[0] for image, refs in rows if len(refs) == 1}
            for image in births:
                require(len(image) == 1, "birth not a single minimum")
                key = next(iter(image))
                require(key in self.minima, "birth not Gabriel")
                token = len(nodes)
                nodes.append(dict(**level(value), first=len(minima), parent_count=0))
                minima.append(list(self.geo.label(key)))
                leaves.append(frozenset({key}))
                covers.append(self.cover(image))
                following[image] = token
            for refs, image in merges:
                token = len(nodes)
                nodes.append(dict(**level(value), first=len(parents), parent_count=len(refs)))
                parents.extend(refs)
                leaves.append(frozenset().union(*(leaves[p] for p in refs)))
                covers.append(frozenset().union(*(covers[p] for p in refs)))
                require(covers[-1] == self.cover(image), "merge gained points")
                following[image] = token
            for image, token in following.items():
                require(covers[token] == self.cover(image), "continuation gained points")
                require(leaves[token] == image & self.minima, "Gamma minima partition")
            live = following
        cuts, roots = [], []
        for cut, closed in bounded_samples(list(self.levels.values())):
            active = {i for i, node in enumerate(nodes)
                      if (rational(node) <= cut if closed else rational(node) < cut)}
            consumed = {p for i in active for p in
                        parents[nodes[i]["first"]:nodes[i]["first"] + nodes[i]["parent_count"]]}
            current = sorted(active - consumed)
            gamma = self.partition(cut, closed)
            require({leaves[i] for i in current} == {group & self.minima for group in gamma},
                    "cut roots disagree with exhaustive Gamma")
            cuts.append(dict(**level(cut), closed=int(closed)))
            roots.append(current)
        noops = 0
        for co in self.direct:
            value = self.levels[co]
            boundary = frozenset(self.boundary[co])
            image = next(group for group in self.partition(value, True) if boundary <= group)
            old = [group for group in self.partition(value, False) if group <= image]
            require(bool(old), "connection without prior parent")
            noops += len(old) == 1
        aliases = self.minima | {f for co in self.direct for f in self.boundary[co]}
        stats = dict(input_records=len(self.minima) + len(self.direct), aliases=len(aliases),
                     face_visits=sum(len(self.geo.meb(co)["support"]) + self.k + 1 for co in self.direct),
                     no_op_connections=noops)
        return dict(nodes=nodes, minima=minima, parents=parents,
                    coverage=[sorted(c) for c in covers],
                    descendant_minima=[sorted(list(self.geo.label(f)) for f in group) for group in leaves],
                    cuts=cuts, roots=roots), stats


def prepare():
    attempts = []
    selected = None
    for seed in range(1, 9):
        try:
            geo = Geometry(cloud(seed))
            witness = geo.path_witness(10)
        except RuntimeError as error:
            attempts.append(dict(seed=seed, status="rejected_geometry", reason=str(error)))
            continue
        attempts.append(dict(seed=seed, status="regular", positive_supports=len(geo.candidates),
                             k10_portal_requests=witness["portal_requests"],
                             k10_chain_steps=witness["chain_steps"]))
        if witness["chain_steps"] > 0:
            selected = geo, seed
            break
    require(selected is not None, "bounded eight-seed search found no K10 chain")
    geo, seed = selected
    records, orders = [], []
    for k in (9, 10):
        gamma = GammaOrder(geo, k)
        expected, stats = gamma.forest()
        minimum = [geo.event(key) for key in sorted(gamma.minima, key=geo.label)]
        direct = [geo.event(key) for key in sorted(gamma.direct, key=geo.label)]
        witness = geo.path_witness(k)
        orders.append(dict(order=k, gamma_facets=len(gamma.facets), gamma_cofaces=len(gamma.cofaces),
                           gamma_levels=len(set(gamma.levels.values())), cuts=len(expected["cuts"]),
                           minima=len(minimum), direct=len(direct), nodes=len(expected["nodes"]),
                           witness=witness))
        for variant in (0, 1):
            def encode(source):
                result = [dict(q=e["q"], d=e["d"], mask=e["mask"], support=e["support"],
                               interior=e["interior"], **level(Q(e["squared_level"]),
                                                             1 if not variant else 2 + i % 17))
                          for i, e in enumerate(source)]
                return result if not variant else result[::-1]
            points = [dict(id=i, xyz=list(point)) for i, point in enumerate(geo.points)]
            cuts = [dict(**level(rational(c), 1 if not variant else 23 + i % 19), closed=c["closed"])
                    for i, c in enumerate(expected["cuts"])]
            for lazy, capacity in ((0, 0), (1, 0), (1, 1), (1, 100000)):
                records.append(dict(id=len(records), base_id=2 * (k - 9) + variant,
                                    case_index=0, order=k, representation=variant,
                                    points=points if not variant else points[::-1],
                                    minima_source=encode(minimum), direct_source=encode(direct),
                                    cuts=cuts, budget_probe=0, lazy=lazy, capacity=capacity,
                                    expected=expected, expected_stats=stats,
                                    named_meb_support_sum=None))
    paths = [Path(__file__), ORACLE, AUDIT / "full_cpp_audit.py", AUDIT / "full_producer_audit.py",
             AUDIT / "receipts_filtered_review_20260906/terminal_reuse_fixture.py"]
    return dict(schema="mhgp7-independent-higher-order-fixtures-v1", status="prepared",
                public_status="not_claimed", unique_orders=2, representations=4,
                policies_and_capacities=[[0, 0], [1, 0], [1, 1], [1, 100000]],
                cases=[dict(name=f"rational_n14_lcg_seed{seed}", points=geo.points,
                            ids=list(range(geo.n)), origin="bounded positive-support search")],
                preparation_attempts=attempts, global_regular_positive_supports=len(geo.candidates),
                preparation_diagnostic=dict(status="corrected_audit_replay_comparison",
                                            first_optimized_check_exit=1,
                                            cause="Python point tuples compared directly with decoded JSON lists",
                                            correction="Compare the JSON-normalized prepared object",
                                            product_execution=False),
                orders=orders, records=records,
                source_pins={str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                             for path in paths},
                scope="Only rational preparation; all cardinal K and K+1 Gamma labels and cuts. "
                      "No C++ execution, performance, vertical, weights or public exactness claim.")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check-only", action="store_true")
    args = parser.parse_args()
    result = prepare()
    target = Path(__file__).with_name("higher_order_fixtures.json")
    if args.check_only:
        require(json.loads(target.read_text()) == json.loads(json.dumps(result)),
                "saved fixture differs from rational replay")
    else:
        require(not target.exists(), "fixture output already exists; use --check-only")
        target.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(json.dumps(dict(status="passed", records=len(result["records"]),
                          preparation_attempts=result["preparation_attempts"],
                          orders=[{k: v for k, v in order.items() if k != "witness"}
                                  | {"chain_steps": order["witness"]["chain_steps"]}
                                  for order in result["orders"]]), sort_keys=True))


if __name__ == "__main__":
    main()
