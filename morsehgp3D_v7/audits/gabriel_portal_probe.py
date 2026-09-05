"""Bounded rational portal constructor; Gamma is fixture source and judge only.

No product code, C++ execution, timing claim, or exhaustive product architecture.
The constructor receives points and the direct catalogue, never a Gamma object.
"""

from __future__ import annotations

import argparse
from collections import Counter
from copy import deepcopy
from fractions import Fraction as Q
import hashlib
import json
from pathlib import Path
import sys
from typing import Any

from horizontal_rational_oracle import Gamma, coverage, facets
from meb_rational_oracle_20260905 import expected_meb, power

AUDIT = Path(__file__).resolve().parent
ROOT = AUDIT.parent.parent
Facet = tuple[int, ...]
Point = tuple[int, int, int]


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


class Portal:
    def __init__(self, points: dict[int, Point], k: int,
                 direct: dict[Facet, Q], mutant: str = "") -> None:
        require(2 <= len(points) <= 7, "fixture.point_bound")
        self.points, self.k, self.direct, self.mutant = points, k, direct, mutant
        self.cache: dict[Facet, dict[str, Any]] = {}
        self.redirect: dict[int, int] = {}
        self.known: dict[Facet, int] = {}
        self.terminals: dict[Facet, int] = {}
        self.roots: dict[int, frozenset[Facet]] = {}
        self.journal: list[dict[str, Any]] = []
        self.history: list[tuple[Q, dict[int, frozenset[Facet]]]] = []
        self.stats: Counter[str] = Counter()
        self.portal_traces: list[dict[str, Any]] = []
        self.next_token = 0
        if k == 1:
            for ident in sorted(points):
                token = self.allocate()
                self.roots[token] = frozenset({(ident,)})
                self.known[(ident,)] = token
        self.initial = deepcopy(self.roots)

    def allocate(self) -> int:
        token = self.next_token
        self.next_token += 1
        return token

    def normalize(self, token: int) -> int:
        old = token
        while token in self.redirect:
            token = self.redirect[token]
        self.stats["historical_token_normalizations"] += token != old
        require(token in self.roots, "portal.target_not_live")
        return token

    def ball(self, label: Facet) -> dict[str, Any]:
        if label not in self.cache:
            ball = expected_meb([self.points[i] for i in label])
            support = {label[i] for i in ball["support"]}
            values = {i: power(ball, p) for i, p in self.points.items()}
            require({i for i, v in values.items() if v == 0} == support,
                    "portal.unsupported_extra_shell")
            require(not ball["degenerate"], "portal.nonessential_support")
            self.cache[label] = dict(ball, support_ids=support, powers=values)
            self.stats["visited_meb_labels"] += 1
            self.stats["point_power_comparisons"] += len(self.points)
        return self.cache[label]

    def locate(self, facet: Facet, level: Q) -> int | None:
        if facet in self.known:
            return self.normalize(self.known[facet])
        if self.mutant == "disable_portals":
            return None
        ball = self.ball(facet)
        intruders = sorted(i for i, p in ball["powers"].items()
                           if p < 0 and i not in facet)
        if len(intruders) <= 1:
            self.stats["first_use_latent"] += 1
            return None
        require(ball["radius"] < level, "portal.not_strictly_pre_batch")
        coface = tuple(sorted((*facet, intruders[0])))
        chain = [coface]
        levels = [ball["radius"]]
        for _ in range(128):
            current = self.ball(coface)
            outsiders = sorted(i for i, p in current["powers"].items()
                               if p < 0 and i not in coface)
            if not outsiders:
                require(coface in self.terminals, "portal.terminal_not_processed")
                require(self.direct[coface] == current["radius"] < level,
                        "portal.terminal_not_strict")
                token = self.normalize(self.terminals[coface])
                self.stats["portals"] += 1
                self.portal_traces.append(dict(facet=facet, consumer_level=str(level),
                                               first_incidence=str(ball["radius"]),
                                               chain=chain, levels=list(map(str, levels)),
                                               terminal_token=token))
                return token
            remove = min(current["support_ids"])
            following = tuple(sorted((set(coface) - {remove}) | {outsiders[0]}))
            next_ball = self.ball(following)
            require(next_ball["radius"] < current["radius"], "portal.descent_not_strict")
            coface = following
            chain.append(coface)
            levels.append(next_ball["radius"])
            self.stats["strict_descent_steps"] += 1
        raise ValueError("portal.descent_budget")

    def run(self) -> None:
        for level in sorted(set(self.direct.values())):
            batch = sorted(c for c, a in self.direct.items() if a == level)
            self.stats["direct_batches"] += 1
            self.stats["direct_cofaces"] += len(batch)
            self.stats["multiple_direct_batches"] += len(batch) > 1
            # This DSU is private to the level. All root lookups use the pre-lot state.
            dsu: dict[tuple[str, Any], tuple[str, Any]] = {}

            def find(node: tuple[str, Any]) -> tuple[str, Any]:
                dsu.setdefault(node, node)
                if dsu[node] != node:
                    dsu[node] = find(dsu[node])
                return dsu[node]

            def union(left: tuple[str, Any], right: tuple[str, Any]) -> None:
                left, right = find(left), find(right)
                if left != right:
                    dsu[right] = left

            batch_facets = sorted({f for c in batch for f in facets(c)})
            for facet in batch_facets:
                node = ("f", facet)
                find(node)
                target = self.locate(facet, level)
                if target is not None:
                    union(node, ("r", target))
            for coface in batch:
                faces = facets(coface)
                for facet in faces[1:]:
                    union(("f", faces[0]), ("f", facet))
            groups: dict[tuple[str, Any], list[tuple[str, Any]]] = {}
            for node in list(dsu):
                groups.setdefault(find(node), []).append(node)
            pending = []
            for nodes in groups.values():
                parents = sorted(v for kind, v in nodes if kind == "r")
                used = frozenset(v for kind, v in nodes if kind == "f")
                group = used.union(*(self.roots[p] for p in parents))
                old_points = frozenset().union(*(coverage(self.roots[p]) for p in parents))
                added = sorted(coverage(group) - old_points)
                token = parents[0] if len(parents) == 1 else self.allocate()
                pending.append((token, parents, group, used))
                if len(parents) != 1 or added:
                    if not (self.mutant == "ignore_growth" and len(parents) == 1):
                        self.journal.append(dict(level=str(level), output=token,
                                                 parents=parents, added_points=added))
                    self.stats["births"] += not parents
                    self.stats["multifusions"] += len(parents) >= 2
                    self.stats["growths"] += len(parents) == 1
                else:
                    self.stats["omitted_pointless_continuations"] += 1
            for token, parents, group, used in pending:
                for parent in parents:
                    del self.roots[parent]
                    if parent != token:
                        self.redirect[parent] = token
                self.roots[token] = group
                for facet in used:
                    self.known.setdefault(facet, token)
            for coface in batch:
                self.terminals[coface] = self.normalize(self.known[facets(coface)[0]])
            self.history.append((level, deepcopy(self.roots)))

    def snapshot(self, cut: Q, closed: bool) -> dict[int, frozenset[Facet]]:
        result = self.initial
        for level, roots in self.history:
            if level <= cut if closed else level < cut:
                result = roots
        return result

    def read_journal(self, cut: Q, closed: bool) -> dict[int, frozenset[int]]:
        roots = {token: coverage(group) for token, group in self.initial.items()}
        for delta in self.journal:
            level = Q(delta["level"])
            if not (level <= cut if closed else level < cut):
                continue
            parents = delta["parents"]
            require(all(p in roots for p in parents), "journal.parent_not_live")
            points = frozenset(delta["added_points"]).union(*(roots[p] for p in parents))
            for parent in parents:
                del roots[parent]
            require(delta["output"] not in roots, "journal.output_clobber")
            roots[delta["output"]] = points
        return roots


def judge(portal: Portal, oracle: Gamma) -> dict[str, int]:
    order_levels = {a for c, a in oracle.levels.items() if len(c) == portal.k + 1}
    levels = sorted(set(oracle.levels.values()))
    samples = [(Q(0), True)]
    for i, level in enumerate(levels):
        samples += [(level, False), (level, True)]
        if i + 1 < len(levels):
            samples.append(((level + levels[i + 1]) / 2, True))
    samples.append((levels[-1] + 1, True))
    stats: Counter[str] = Counter(orders=1)
    previous: dict[int, frozenset[Facet]] = {}
    previous_map: dict[int, frozenset[Facet]] = {}
    for cut, closed in samples:
        roots = portal.snapshot(cut, closed)
        reference = oracle.partition(portal.k, cut, closed)
        mapping = {}
        for token, group in roots.items():
            images = [c for c in reference if group <= c]
            require(len(images) == 1, "judge.inclusion")
            require(images[0] not in mapping.values(), "judge.bijection_injective")
            require(coverage(group) == coverage(images[0]), "judge.coverage")
            mapping[token] = images[0]
        require(set(mapping.values()) == set(reference), "judge.bijection_surjective")
        require(portal.read_journal(cut, closed) == {t: coverage(c) for t, c in roots.items()},
                "judge.minimal_journal_coverage")
        for token, group in previous.items():
            targets = [t for t, c in roots.items() if group <= c]
            require(len(targets) == 1 and previous_map[token] <= mapping[targets[0]],
                    "judge.naturality")
            stats["naturality_squares"] += 1
        previous, previous_map = roots, mapping
        stats["cuts"] += 1
        stats["omitted_gamma_facets"] += sum(map(len, reference)) - sum(map(len, roots.values()))
        if cut in order_levels and cut not in portal.direct.values():
            stats["silent_level_cuts"] += 1
    # Validate abstract parent sets against Gamma, independently of the batch DSU.
    for delta in portal.journal:
        cut = Q(delta["level"])
        before = portal.snapshot(cut, False)
        after = portal.snapshot(cut, True)
        pre_gamma = oracle.partition(portal.k, cut, False)
        post_gamma = oracle.partition(portal.k, cut, True)
        image = next(c for c in post_gamma if after[delta["output"]] <= c)
        parents = {next(c for c in pre_gamma if before[p] <= c) for p in delta["parents"]}
        require(parents == {c for c in pre_gamma if c <= image}, "judge.abstract_parents")
        old_points = frozenset().union(*(coverage(c) for c in parents))
        require(set(delta["added_points"]) == coverage(image) - old_points, "judge.point_delta")
        stats["journal_events"] += 1
    return dict(stats)


def corpus() -> list[dict[str, Any]]:
    e5 = [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)]
    clouds = [("E5", e5), ("ACDE", [e5[i] for i in (0, 2, 3, 4)]),
              ("moment7", [(t, t*t, t**3) for t in range(7)]),
              ("two_triangles", [(t, t*t, t**3) for t in (0, 1, 2, 10, 11, 12)])]
    cases = []
    for name, points in clouds:
        for reverse in (False, True):
            ids = list(range(len(points)))
            if reverse:
                ids.reverse()
            cases.append(dict(name=name, points=points, ids=ids, reversed_ids=reverse))
    return cases


def execute() -> dict[str, Any]:
    totals: Counter[str] = Counter()
    records = []
    e5_oracle = None
    e5_case = None
    for case in corpus():
        oracle = Gamma(case)
        points = dict(zip(case["ids"], case["points"]))
        if case["name"] == "E5" and not case["reversed_ids"]:
            e5_oracle, e5_case = oracle, case
        for k in range(1, len(points)):
            direct = {c: oracle.levels[c] for c in oracle.direct if len(c) == k + 1}
            portal = Portal(points, k, direct)
            portal.run()
            checked = judge(portal, oracle)
            totals.update(portal.stats)
            totals.update(checked)
            records.append(dict(case=case["name"], reversed_ids=case["reversed_ids"], K=k,
                                counts=dict(portal.stats), checks=checked,
                                journal=portal.journal, portals=portal.portal_traces))
    for name in ("portals", "strict_descent_steps", "historical_token_normalizations",
                 "births", "multifusions", "growths", "multiple_direct_batches",
                 "omitted_pointless_continuations", "omitted_gamma_facets",
                 "silent_level_cuts", "naturality_squares"):
        require(totals[name] > 0, "nonvacuity." + name)
    require(e5_case is not None and e5_oracle is not None, "fixture.E5_missing")
    mutants = []
    for mutant, expected in (("disable_portals", "judge.coverage"),
                             ("ignore_growth", "judge.minimal_journal_coverage")):
        direct = {c: e5_oracle.levels[c] for c in e5_oracle.direct if len(c) == 3}
        portal = Portal(dict(zip(e5_case["ids"], e5_case["points"])), 2, direct, mutant)
        try:
            portal.run()
            judge(portal, e5_oracle)
        except ValueError as error:
            require(str(error) == expected, "mutant.wrong_rejection." + str(error))
            mutants.append(dict(name=mutant, kind="audit_algorithm_mutant", rejection=str(error)))
        else:
            raise ValueError("mutant.survived." + mutant)
    paths = [Path(__file__), AUDIT / "horizontal_rational_oracle.py",
             AUDIT / "meb_rational_oracle_20260905.py"]
    return dict(status="completed_bounded_audit", public_status="not_claimed",
                python_optimized=not __debug__, cases=corpus(), counts=dict(totals),
                mutants=mutants, records=records,
                sources_sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                                for p in paths},
                scope="Gamma supplies the bounded direct fixture catalogue and independent cut judge; "
                      "Portal receives only coordinates, IDs, order and direct cofaces/levels. "
                      "Geometry is queried only for visited labels. Journal contains no descent cofaces. "
                      "No mass, vertical-map, product integration, scalability or runtime claim.",
                gcp="not_used")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = execute()
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps(dict(status=result["status"], counts=result["counts"], mutants=result["mutants"])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
