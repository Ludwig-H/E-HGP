"""FULL HGP audit: Gabriel minima and merges, including isolated components.

Gamma is an exhaustive fixture source/judge for n <= 7, never the constructor's
resolver. The frozen reduced prototype supplies only geometric portal helpers.
"""

from __future__ import annotations

import argparse
from collections import Counter
from copy import deepcopy
from fractions import Fraction as Q
import hashlib
import itertools
import json
from pathlib import Path
import sys
from typing import Any

from gabriel_portal_probe import Portal, Point, Facet, corpus, require
from horizontal_rational_oracle import Gamma, coverage, facets
from meb_rational_oracle_20260905 import expected_meb, power

AUDIT = Path(__file__).resolve().parent
ROOT = AUDIT.parent.parent


class FullPortal(Portal):
    def __init__(self, points: dict[int, Point], k: int, direct: dict[Facet, Q],
                 minima: dict[Facet, Q], mutant: str = "") -> None:
        super().__init__(points, k, direct, mutant)
        # FULL singletons are events at zero, not roots in the strict zero cut.
        self.roots = {}
        self.known = {}
        self.initial = {}
        self.next_token = 0
        self.minima = minima
        self.false_births: set[Facet] = set()

    def locate(self, facet: Facet, level: Q) -> int | None:
        if facet in self.known:
            return self.normalize(self.known[facet])
        require(len(facet) >= 2, "full.singleton_not_born")
        ball = self.ball(facet)
        outside = [i for i, p in ball["powers"].items() if p < 0 and i not in facet]
        if len(outside) <= 1:
            require(len(outside) == 1 and ball["radius"] == level,
                    "full.unknown_facet_not_simultaneous_incidence")
            self.stats["single_intruder_simultaneous_incidence"] += 1
            if self.mutant == "false_single_intruder_as_isolated_birth":
                self.false_births.add(facet)
            return None
        return super().locate(facet, level)

    def run(self) -> None:
        for level in sorted(set(self.direct.values()) | set(self.minima.values())):
            batch = sorted(c for c, a in self.direct.items() if a == level)
            births = sorted(f for f, a in self.minima.items() if a == level)
            self.stats["processed_levels"] += 1
            self.stats["direct_batches"] += bool(batch)
            self.stats["direct_cofaces"] += len(batch)
            self.stats["multiple_direct_batches"] += len(batch) > 1
            self.stats["multiple_minima_batches"] += len(births) > 1
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

            used_facets = sorted({f for c in batch for f in facets(c)})
            require(not set(used_facets) & set(births), "full.gabriel_birth_incident_same_lot")
            for facet in used_facets:
                find(("f", facet))
                target = self.locate(facet, level)
                if target is not None:
                    union(("f", facet), ("r", target))
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
                require(bool(parents), "full.coface_missing_prior_parents")
                group = used.union(*(self.roots[p] for p in parents))
                old_points = frozenset().union(*(coverage(self.roots[p]) for p in parents))
                require(coverage(group) == old_points, "full.coface_point_growth")
                self.stats["coface_groups_with_prior_parents_and_zero_growth"] += 1
                token = parents[0] if len(parents) == 1 else self.allocate()
                pending.append((token, parents, group, used))
                if len(parents) >= 2:
                    self.journal.append(dict(kind="merge", level=str(level),
                                             output=token, parents=parents))
                    self.stats["multifusions"] += 1
                else:
                    self.stats["omitted_continuations"] += 1
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
            # No birth at this level was available to the coface DSU above.
            births = sorted(set(births) | self.false_births)
            self.false_births.clear()
            for facet in births:
                if self.mutant == "omit_minima":
                    continue
                require(facet not in self.known, "full.gabriel_birth_incident_same_lot")
                if len(facet) > 1:
                    ball = self.ball(facet)
                    require(ball["radius"] == level, "full.minimum_level")
                    require(all(p > 0 for i, p in ball["powers"].items() if i not in facet),
                            "full.minimum_not_gabriel")
                else:
                    require(level == 0, "full.singleton_level")
                token = self.allocate()
                self.known[facet] = token
                self.roots[token] = frozenset({facet})
                self.journal.append(dict(kind="birth", level=str(level), output=token, label=facet))
                self.stats["isolated_births"] += 1
            self.history.append((level, deepcopy(self.roots)))

    def read_journal(self, cut: Q, closed: bool) -> dict[int, frozenset[int]]:
        roots: dict[int, frozenset[int]] = {}
        for event in self.journal:
            level = Q(event["level"])
            if not (level <= cut if closed else level < cut):
                continue
            if event["kind"] == "birth":
                require(set(event) == {"kind", "level", "output", "label"}, "full.leaf_schema")
                require(len(event["label"]) == self.k, "full.leaf_cardinality")
                points = frozenset(event["label"])
            else:
                require(event["kind"] == "merge" and
                        set(event) == {"kind", "level", "output", "parents"}, "full.merge_schema")
                parents = event["parents"]
                require(len(parents) >= 2 and len(set(parents)) == len(parents), "full.true_merge")
                require(all(p in roots for p in parents), "full.reader_parent_not_live")
                points = frozenset().union(*(roots[p] for p in parents))
                for parent in parents:
                    del roots[parent]
            require(event["output"] not in roots, "full.reader_output_clobber")
            roots[event["output"]] = points
        return roots


def full_partition(ids: list[int], levels: dict[Facet, Q], k: int,
                   cut: Q, closed: bool) -> list[frozenset[Facet]]:
    def active(level: Q) -> bool:
        return level <= cut if closed else level < cut

    result = [frozenset({f}) for f in itertools.combinations(sorted(ids), k)
              if active(Q(0) if k == 1 else levels[f])]
    for coface, level in levels.items():
        if len(coface) != k + 1 or not active(level):
            continue
        boundary = frozenset(facets(coface))
        connected = [group for group in result if group & boundary]
        require(boundary <= frozenset().union(*connected), "oracle.inactive_boundary")
        result = [group for group in result if not group & boundary]
        result.append(boundary.union(*connected))
    return result


def judge(model: FullPortal, oracle: Gamma) -> dict[str, int]:
    levels = sorted({Q(0), *oracle.levels.values()})
    samples = []
    for i, level in enumerate(levels):
        samples += [(level, False), (level, True)]
        if i + 1 < len(levels):
            samples.append(((level + levels[i + 1]) / 2, True))
    samples.append((levels[-1] + 1, True))
    stats: Counter[str] = Counter(orders=1)
    previous: dict[int, frozenset[Facet]] = {}
    previous_map: dict[int, frozenset[Facet]] = {}
    for cut, closed in samples:
        roots = model.snapshot(cut, closed)
        reference = full_partition(oracle.ids, oracle.levels, model.k, cut, closed)
        mapping = {}
        for token, group in roots.items():
            images = [c for c in reference if group <= c]
            require(len(images) == 1, "full.judge_inclusion")
            require(images[0] not in mapping.values(), "full.judge_injective")
            require(coverage(group) == coverage(images[0]), "full.judge_coverage")
            mapping[token] = images[0]
        require(set(mapping.values()) == set(reference), "full.judge_surjective")
        require(model.read_journal(cut, closed) == {t: coverage(c) for t, c in roots.items()},
                "full.judge_minima_merge_reader")
        for token, group in previous.items():
            targets = [t for t, c in roots.items() if group <= c]
            require(len(targets) == 1 and previous_map[token] <= mapping[targets[0]],
                    "full.judge_naturality")
            stats["naturality_squares"] += 1
        previous, previous_map = roots, mapping
        stats["cuts"] += 1
        stats["isolated_gamma_component_observations"] += sum(len(c) == 1 for c in reference)
        stats["omitted_gamma_facets"] += sum(map(len, reference)) - sum(map(len, roots.values()))
        if cut == 0:
            require(len(roots) == (len(oracle.ids) if closed and model.k == 1 else 0),
                    "full.zero_cut")
            stats["zero_cut_checks"] += 1
    for event in model.journal:
        cut = Q(event["level"])
        before = model.snapshot(cut, False)
        after = model.snapshot(cut, True)
        pre = full_partition(oracle.ids, oracle.levels, model.k, cut, False)
        post = full_partition(oracle.ids, oracle.levels, model.k, cut, True)
        image = next(c for c in post if after[event["output"]] <= c)
        parents = {next(c for c in pre if before[p] <= c) for p in event.get("parents", [])}
        require(parents == {c for c in pre if c <= image}, "full.judge_abstract_parents")
        if event["kind"] == "birth":
            require(image == frozenset({tuple(event["label"])}), "full.judge_isolated_birth")
        else:
            require(coverage(image) == frozenset().union(*(coverage(c) for c in parents)),
                    "full.judge_merge_no_new_points")
        stats["journal_events"] += 1
    if model.k == len(oracle.ids):
        require(len(model.journal) == 1 and model.journal[0]["kind"] == "birth",
                "full.terminal_order_unique_leaf")
        stats["terminal_order_unique_leaf_checks"] += 1
    return dict(stats)


def make_model(case: dict[str, Any], oracle: Gamma, k: int, mutant: str = "") -> FullPortal:
    direct = {c: oracle.levels[c] for c in oracle.direct if len(c) == k + 1}
    minima = ({(i,): Q(0) for i in case["ids"]} if k == 1 else
              {f: oracle.levels[f] for f in oracle.direct if len(f) == k})
    return FullPortal(dict(zip(case["ids"], case["points"])), k, direct, minima, mutant)


def execute() -> dict[str, Any]:
    cases = corpus() + [dict(name="obtuse_J1", points=[(0, 0, 0), (4, 0, 0), (1, 1, 0)],
                            ids=[0, 1, 2], reversed_ids=False),
                        dict(name="symmetric_obtuse_J1", points=[(0, 0, 0), (4, 0, 0), (2, 1, 0)],
                             ids=[0, 1, 2], reversed_ids=False)]
    totals: Counter[str] = Counter()
    records = []
    oracles = []
    for case in cases:
        oracle = Gamma(case)
        oracles.append(oracle)
        for k in range(1, len(case["ids"]) + 1):
            model = make_model(case, oracle, k)
            model.run()
            checks = judge(model, oracle)
            totals.update(model.stats)
            totals.update(checks)
            records.append(dict(case=case["name"], reversed_ids=case["reversed_ids"], K=k,
                                counts=dict(model.stats), checks=checks, journal=model.journal,
                                portals=model.portal_traces))
    for name in ("portals", "strict_descent_steps", "isolated_births", "multifusions",
                 "single_intruder_simultaneous_incidence", "multiple_direct_batches",
                 "multiple_minima_batches",
                 "coface_groups_with_prior_parents_and_zero_growth", "omitted_continuations",
                 "isolated_gamma_component_observations", "omitted_gamma_facets",
                 "zero_cut_checks", "terminal_order_unique_leaf_checks"):
        require(totals[name] > 0, "nonvacuity." + name)
    for name, birth_levels in (("obtuse_J1", ["1/2", "5/2"]),
                              ("symmetric_obtuse_J1", ["5/4", "5/4"])):
        row = next(r for r in records if r["case"] == name and r["K"] == 2)
        require([e["kind"] for e in row["journal"]] == ["birth", "birth", "merge"]
                and [e["level"] for e in row["journal"]] == birth_levels + ["4"]
                and [e["label"] for e in row["journal"][:2]] == [(0, 2), (1, 2)],
                "named.obtuse_full_minima_and_merge")
        terminal = next(r for r in records if r["case"] == name and r["K"] == 3)
        require(terminal["journal"] == [dict(kind="birth", level="4", output=0, label=(0, 1, 2))],
                "named.obtuse_terminal_leaf")
    mutants = []
    for mutant, index, k, expected in (
            ("omit_minima", 0, 5, "full.judge_surjective"),
            ("false_single_intruder_as_isolated_birth", 8, 2, "full.gabriel_birth_incident_same_lot")):
        try:
            model = make_model(cases[index], oracles[index], k, mutant)
            model.run()
            judge(model, oracles[index])
        except ValueError as error:
            require(str(error) == expected, "mutant.wrong_rejection." + str(error))
            mutants.append(dict(name=mutant, kind="audit_algorithm_mutant", rejection=str(error)))
        else:
            raise ValueError("mutant.survived." + mutant)
    # Outside the regular model, full Gamma remains defined at an extra-shell.
    points = {0: (0, 0, 0), 1: (2, 0, 0), 2: (1, 1, 0)}
    shell_levels = {(0, 1): Q(1), (0, 2): Q(1, 2), (1, 2): Q(1, 2), (0, 1, 2): Q(1)}
    ball = expected_meb([points[0], points[1]])
    require(power(ball, points[2]) == 0, "extra_shell.exact_contact")
    strict = full_partition(list(points), shell_levels, 2, Q(1), False)
    closed = full_partition(list(points), shell_levels, 2, Q(1), True)
    require(set(strict) == {frozenset({(0, 2)}), frozenset({(1, 2)})}
            and closed == [frozenset({(0, 1), (0, 2), (1, 2)})], "extra_shell.full_gamma")
    try:
        FullPortal(points, 2, {(0, 1, 2): Q(1)},
                   {f: a for f, a in shell_levels.items() if len(f) == 2}).run()
    except ValueError as error:
        require(str(error) == "full.gabriel_birth_incident_same_lot", "extra_shell.wrong_rejection")
        shell_rejection = str(error)
    else:
        raise ValueError("extra_shell.model_accepted")
    try:
        Portal(points, 2, {}).ball((0, 1))
    except ValueError as error:
        require(str(error) == "portal.unsupported_extra_shell", "extra_shell.geometry_gate")
        geometry_rejection = str(error)
    else:
        raise ValueError("extra_shell.geometry_accepted")
    paths = [Path(__file__), AUDIT / "gabriel_portal_probe.py",
             AUDIT / "horizontal_rational_oracle.py", AUDIT / "meb_rational_oracle_20260905.py"]
    return dict(status="completed_bounded_full_audit", public_status="not_claimed",
                python_optimized=not __debug__, cases=cases, counts=dict(totals), records=records,
                mutants=mutants, extra_shell=dict(points=points, squared_level="1",
                                                 strict_components=[list(c) for c in strict],
                                                 closed_components=[list(c) for c in closed],
                                                 model_rejection=shell_rejection,
                                                 geometric_rejection=geometry_rejection,
                                                 scope="Valid FULL Gamma fixture outside regular portal domain"),
                sources_sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                                for p in paths},
                scope="FULL components and point coverage, isolated minima included, all K=1..n. "
                      "Journal stores only Gabriel leaf labels/levels and true merges. "
                      "Gamma exhaustive catalogue is bounded fixture authority; constructor sees only "
                      "points and Gabriel catalogues of cardinal K and K+1. "
                      "No mass-timing, weighted leaves, vertical-map, scalability or product claim.",
                gcp="not_used")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = execute()
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps(dict(status=result["status"], counts=result["counts"], mutants=result["mutants"],
                          extra_shell=result["extra_shell"])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
