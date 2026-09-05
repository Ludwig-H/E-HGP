#!/usr/bin/env python3
"""Compare two pinned audit journals; never invoke geometry or a product engine."""

from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import sys
from typing import Any


JsonObject = dict[str, Any]
CaseKey = tuple[str, bool]
RecordKey = tuple[str, bool, int]
Coverage = frozenset[int]
State = dict[int, Coverage]
BASE = Path(__file__).resolve().parent
RECEIPTS = BASE / "receipts_gabriel_20260905"
INPUTS = {
    "full_normal.json": "00911669a9045b0bd466ee991d9972e764c46a0fa941d7ab79cfeca3082e627f",
    "portal_normal.json": "327965e3aaa9cf5d5993c9048f0a556239c80d682d135105a9bb8515b194d5f9",
}


class Rejection(RuntimeError):
    """Explicit rejection also active under optimized Python."""

    def __init__(self, code: str, details: JsonObject) -> None:
        super().__init__(code)
        self.code = code
        self.details = details


def require(condition: bool, code: str, **details: Any) -> None:
    if not condition:
        raise Rejection(code, details)


def unique_object(pairs: list[tuple[str, Any]]) -> JsonObject:
    result: JsonObject = {}
    for key, value in pairs:
        require(key not in result, "input.duplicate_json_key", key=key)
        result[key] = value
    return result


def forbidden_constant(value: str) -> None:
    raise Rejection("input.nonfinite_json", {"value": value})


def read_receipt(name: str) -> tuple[JsonObject, JsonObject]:
    raw = (RECEIPTS / name).read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    require(digest == INPUTS[name], "input.pin", path=name)
    data = json.loads(raw, object_pairs_hook=unique_object,
                      parse_constant=forbidden_constant)
    require(type(data) is dict, "input.object")
    require(data.get("public_status") == "not_claimed", "input.public_status")
    return data, {"path": name, "sha256": digest, "bytes": len(raw)}


def uint(value: Any) -> int:
    require(type(value) is int and value >= 0, "journal.uint")
    return value


def ids(value: Any) -> list[int]:
    require(type(value) is list, "journal.ids")
    parsed = [uint(item) for item in value]
    require(parsed == sorted(set(parsed)), "journal.unique_sorted_ids")
    return parsed


def level(value: Any) -> Fraction:
    require(type(value) is str, "journal.level_type")
    try:
        parsed = Fraction(value)
    except (ValueError, ZeroDivisionError) as error:
        raise Rejection("journal.level", {"value": value}) from error
    require(parsed >= 0 and str(parsed) == value, "journal.canonical_level")
    return parsed


def batches(journal: list[JsonObject]) -> list[tuple[Fraction, list[JsonObject]]]:
    require(type(journal) is list, "journal.list")
    grouped: dict[Fraction, list[JsonObject]] = {}
    previous = Fraction(-1)
    for event in journal:
        require(type(event) is dict, "journal.event")
        current = level(event.get("level"))
        require(current >= previous, "journal.level_order")
        grouped.setdefault(current, []).append(event)
        previous = current
    return list(grouped.items())


def inventory(data: JsonObject, full: bool) -> tuple[
        dict[CaseKey, JsonObject], dict[RecordKey, JsonObject]]:
    cases: dict[CaseKey, JsonObject] = {}
    for case in data["cases"]:
        require(type(case["name"]) is str and
                type(case["reversed_ids"]) is bool, "input.case_key")
        key = (case["name"], case["reversed_ids"])
        require(key not in cases, "input.duplicate_case")
        point_ids = case["ids"]
        require(type(point_ids) is list and len(point_ids) >= 2,
                "input.case_ids")
        require(sorted(uint(item) for item in point_ids) ==
                list(range(len(point_ids))), "input.case_id_permutation")
        require(len(case["points"]) == len(point_ids), "input.case_points")
        cases[key] = case
    records: dict[RecordKey, JsonObject] = {}
    for record in data["records"]:
        key = (record["case"], record["reversed_ids"], uint(record["K"]))
        require(key[:2] in cases and key not in records,
                "input.record_identity")
        records[key] = record
    expected = {(name, reversed_ids, order)
                for (name, reversed_ids), case in cases.items()
                for order in range(1, len(case["ids"]) + int(full))}
    require(set(records) == expected, "input.order_inventory")
    require(len(cases) == (10 if full else 8), "input.case_nonvacuity")
    require(len(records) == (50 if full else 36), "input.order_nonvacuity")
    return cases, records


@dataclass(frozen=True)
class FullNode:
    coverage: Coverage
    leaves: frozenset[int]
    born: Fraction


@dataclass
class FullModel:
    nodes: dict[int, FullNode]
    snapshots: dict[Fraction, dict[int, FullNode]]
    leaves: int
    merges: int
    roots: int


def inspect_full(record: JsonObject, point_ids: list[int]) -> FullModel:
    order = record["K"]
    nodes: dict[int, FullNode] = {}
    active: dict[int, FullNode] = {}
    snapshots: dict[Fraction, dict[int, FullNode]] = {}
    labels: set[tuple[int, ...]] = set()
    leaf_count = 0
    merge_count = 0
    edges = 0
    for at, events in batches(record["journal"]):
        pending: dict[int, FullNode] = {}
        consumed: set[int] = set()
        for event in events:
            output = uint(event.get("output"))
            require(output not in nodes and output not in pending,
                    "full.unique_output")
            if event.get("kind") == "birth":
                require(set(event) == {"kind", "level", "output", "label"},
                        "full.birth_schema")
                label = ids(event["label"])
                require(len(label) == order and set(label) <= set(point_ids),
                        "full.label")
                require(tuple(label) not in labels, "full.unique_leaf_label")
                labels.add(tuple(label))
                require(order != 1 or at == 0, "full.k1_zero_birth")
                node = FullNode(frozenset(label), frozenset({output}), at)
                leaf_count += 1
            else:
                require(event.get("kind") == "merge" and
                        set(event) == {"kind", "level", "output", "parents"},
                        "full.merge_schema")
                parents = ids(event["parents"])
                require(len(parents) >= 2, "full.multifusion_degree")
                require(set(parents) <= active.keys() and
                        not (set(parents) & consumed), "full.strict_parents")
                parent_nodes = [active[parent] for parent in parents]
                require(all(parent.born < at for parent in parent_nodes),
                        "full.strict_parent_level")
                leaves = frozenset().union(*(p.leaves for p in parent_nodes))
                require(len(leaves) == sum(len(p.leaves) for p in parent_nodes),
                        "full.disjoint_genealogy")
                node = FullNode(frozenset().union(
                    *(p.coverage for p in parent_nodes)), leaves, at)
                consumed.update(parents)
                merge_count += 1
                edges += len(parents)
            pending[output] = node
        for parent in consumed:
            del active[parent]
        active.update(pending)
        nodes.update(pending)
        snapshots[at] = dict(active)
    if order == 1:
        require(labels == {(point,) for point in point_ids}, "full.k1_roots")
    roots = len(active)
    require(merge_count <= leaf_count - roots and
            edges == leaf_count + merge_count - roots, "full.forest_bound")
    if order == len(point_ids):
        require(leaf_count == roots == 1 and merge_count == 0 and
                next(iter(active.values())).coverage == frozenset(point_ids),
                "full.terminal_order")
    return FullModel(nodes, snapshots, leaf_count, merge_count, roots)


@dataclass
class Projection:
    journal: list[JsonObject]
    roots: State
    full_to_token: dict[int, int]
    omitted: int


def project(record: JsonObject, model: FullModel,
            mutant: str | None = None) -> Projection:
    order = record["K"]
    mapping: dict[int, int] = {}
    roots: State = {}
    journal: list[JsonObject] = []
    next_token = 100_000
    omitted = 0
    for event in record["journal"]:
        output = event["output"]
        if event["kind"] == "birth":
            if order == 1:
                mapping[output] = next_token
                roots[next_token] = model.nodes[output].coverage
                next_token += 1
            elif mutant == "full_leaf_as_reduced_root":
                mapping[output] = next_token
                journal.append({"level": event["level"], "output": next_token,
                                "parents": [], "added_points": event["label"]})
                next_token += 1
            continue
        retained = [parent for parent in event["parents"]
                    if order == 1 or len(model.nodes[parent].leaves) >= 2 or
                    mutant == "full_leaf_as_reduced_root"]
        parents = sorted(mapping[parent] for parent in retained)
        require(len(parents) == len(set(parents)), "projection.distinct_parents")
        old_coverage = frozenset().union(
            *(model.nodes[parent].coverage for parent in retained))
        added = model.nodes[output].coverage - old_coverage
        if mutant == "drop_single_leaf_parent_growth" and retained:
            added = frozenset()
        if len(parents) == 1:
            token = parents[0]
        else:
            token = next_token
            next_token += 1
        mapping[output] = token
        if len(parents) == 1 and not added:
            omitted += 1
            continue
        journal.append({"level": event["level"], "output": token,
                        "parents": parents, "added_points": sorted(added)})
    return Projection(journal, roots, mapping, omitted)


@dataclass
class ReducedModel:
    roots: State
    snapshots: dict[Fraction, State]
    event_covers: dict[tuple[Fraction, int], Coverage]


def inspect_reduced(journal: list[JsonObject], roots: State,
                    point_ids: list[int]) -> ReducedModel:
    active = dict(roots)
    seen = set(roots)
    snapshots: dict[Fraction, State] = {}
    covers: dict[tuple[Fraction, int], Coverage] = {}
    for at, events in batches(journal):
        require(at > 0, "reduced.positive_event_level")
        consumed: set[int] = set()
        pending: State = {}
        for event in events:
            require(set(event) == {"level", "output", "parents", "added_points"},
                    "reduced.event_schema")
            parents = ids(event["parents"])
            added = frozenset(ids(event["added_points"]))
            output = uint(event["output"])
            require(set(parents) <= active.keys() and
                    not (set(parents) & consumed), "reduced.strict_parents")
            require(output not in pending, "reduced.output_per_lot")
            if len(parents) == 1:
                require(output == parents[0] and bool(added),
                        "reduced.useful_continuation")
            else:
                require(output not in seen, "reduced.fresh_output")
            old = frozenset().union(*(active[parent] for parent in parents))
            require(not (old & added) and added <= set(point_ids),
                    "reduced.new_points")
            coverage = old | added
            require(bool(coverage), "reduced.nonempty_component")
            pending[output] = coverage
            consumed.update(parents)
            covers[(at, output)] = coverage
        for parent in consumed:
            del active[parent]
        active.update(pending)
        seen.update(pending)
        snapshots[at] = dict(active)
    return ReducedModel(dict(roots), snapshots, covers)


def reduced_state(model: ReducedModel, at: Fraction, closed: bool) -> State:
    if at == 0 and not closed:
        return {}
    result = model.roots
    for event_level, state in model.snapshots.items():
        if event_level < at or (closed and event_level == at):
            result = state
        else:
            break
    return result


def full_state(model: FullModel, at: Fraction,
               closed: bool) -> dict[int, FullNode]:
    result: dict[int, FullNode] = {}
    for event_level, state in model.snapshots.items():
        if event_level < at or (closed and event_level == at):
            result = state
        else:
            break
    return result


def event_isomorphism(candidate: Projection, candidate_model: ReducedModel,
                      reference: JsonObject,
                      reference_model: ReducedModel) -> dict[int, int]:
    mapping: dict[int, int] = {}
    for token, coverage in candidate.roots.items():
        matches = [other for other, points in reference_model.roots.items()
                   if points == coverage]
        require(len(matches) == 1, "compare.unique_root_mapping")
        mapping[token] = matches[0]
    reference_batches = dict(batches(reference["journal"]))
    candidate_batches = dict(batches(candidate.journal))
    require(candidate_batches.keys() == reference_batches.keys(),
            "compare.event_levels")
    for at, events in candidate_batches.items():
        others = reference_batches[at]
        require(len(events) == len(others), "compare.event_multiplicity")
        remaining = set(range(len(others)))
        pending: dict[int, int] = {}
        for event in events:
            require(all(parent in mapping for parent in event["parents"]),
                    "compare.parent_mapping_exists")
            parents = sorted(mapping[parent] for parent in event["parents"])
            coverage = candidate_model.event_covers[(at, event["output"])]
            matches = [index for index in remaining
                       if others[index]["parents"] == parents and
                       others[index]["added_points"] == event["added_points"] and
                       reference_model.event_covers[
                           (at, others[index]["output"])] == coverage]
            require(len(matches) == 1, "compare.unique_event_mapping",
                    level=str(at), candidates=len(matches))
            index = matches[0]
            target = others[index]["output"]
            if event["output"] in mapping:
                require(mapping[event["output"]] == target,
                        "compare.persistent_continuation")
            pending[event["output"]] = target
            remaining.remove(index)
        require(not remaining, "compare.surjective_event_mapping")
        mapping.update(pending)
        require(len(set(mapping.values())) == len(mapping),
                "compare.injective_history_mapping")
    return mapping


def encoded_state(state: State) -> list[JsonObject]:
    return [{"token": token, "points": sorted(points)}
            for token, points in sorted(state.items())]


def compare_cuts(record: JsonObject, full: FullModel, projection: Projection,
                 candidate: ReducedModel, cuts: list[Fraction],
                 reference: ReducedModel | None = None,
                 mapping: dict[int, int] | None = None) -> int:
    checked = 0
    for at in cuts:
        for closed in (False, True):
            expected: State = {}
            for node_id, node in full_state(full, at, closed).items():
                if record["K"] == 1 or len(node.leaves) >= 2:
                    require(node_id in projection.full_to_token,
                            "projection.full_branch_mapped")
                    token = projection.full_to_token[node_id]
                    require(token not in expected, "projection.active_injective")
                    expected[token] = node.coverage
            actual = reduced_state(candidate, at, closed)
            require(actual == expected, "projection.cut_state", level=str(at),
                    side="closed" if closed else "open",
                    expected=encoded_state(expected), actual=encoded_state(actual))
            if reference is not None:
                require(mapping is not None and all(token in mapping for token in actual),
                        "compare.cut_mapping_exists")
                mapped = {mapping[token]: coverage for token, coverage in actual.items()}
                require(mapped == reduced_state(reference, at, closed),
                        "compare.cut_state", level=str(at), closed=closed)
            checked += 1
    return checked


def mutation_checks(record: JsonObject, model: FullModel,
                    point_ids: list[int]) -> list[JsonObject]:
    results: list[JsonObject] = []
    for mutant in ("drop_single_leaf_parent_growth", "full_leaf_as_reduced_root"):
        projection = project(record, model, mutant)
        candidate = inspect_reduced(projection.journal, projection.roots, point_ids)
        witnesses: list[JsonObject] = []
        if mutant == "drop_single_leaf_parent_growth":
            for text, missing in (("189/17", {0}), ("83886/3563", {0, 1})):
                at = Fraction(text)
                actual = reduced_state(candidate, at, True)
                covered = frozenset().union(*actual.values())
                expected = frozenset().union(*(
                    node.coverage for node in full_state(model, at, True).values()
                    if len(node.leaves) >= 2))
                require(expected - covered == missing,
                        "mutant.e5_missing_a_then_ab")
                witnesses.append({"level": text, "missing_PointId": sorted(missing)})
        cuts = sorted({Fraction(0), *(level(e["level"]) for e in record["journal"])})
        try:
            compare_cuts(record, model, projection, candidate, cuts)
        except Rejection as error:
            require(error.code == "projection.cut_state", "mutant.unexpected_rejection",
                    mutant=mutant, actual=error.code)
            results.append({"name": mutant, "kind": "audit_projection_mutant",
                            "rejection": error.code, "first_difference": error.details,
                            "witnesses": witnesses})
        else:
            raise Rejection("mutant.survived", {"mutant": mutant})
    return results


def run() -> JsonObject:
    full_data, full_pin = read_receipt("full_normal.json")
    reduced_data, reduced_pin = read_receipt("portal_normal.json")
    require(full_data.get("status") == "completed_bounded_full_audit" and
            reduced_data.get("status") == "completed_bounded_audit",
            "input.terminal_status")
    full_cases, full_records = inventory(full_data, True)
    reduced_cases, reduced_records = inventory(reduced_data, False)
    require(reduced_cases.keys() <= full_cases.keys(), "input.common_cases")
    for key, case in reduced_cases.items():
        require(case == full_cases[key], "input.same_case_identity")
    global_levels: dict[CaseKey, set[Fraction]] = {key: {Fraction(0)} for key in full_cases}
    for records in (full_records, reduced_records):
        for key, record in records.items():
            global_levels[key[:2]].update(level(e["level"]) for e in record["journal"])
    counts: Counter[str] = Counter()
    records_out: list[JsonObject] = []
    for key, record in full_records.items():
        point_ids = full_cases[key[:2]]["ids"]
        model = inspect_full(record, point_ids)
        projection = project(record, model)
        candidate = inspect_reduced(projection.journal, projection.roots, point_ids)
        common = key in reduced_records
        reference_model = None
        mapping = None
        if common:
            reference = reduced_records[key]
            roots = {point: frozenset({point}) for point in point_ids} if key[2] == 1 else {}
            reference_model = inspect_reduced(reference["journal"], roots, point_ids)
            mapping = event_isomorphism(projection, candidate, reference, reference_model)
            counts["common_orders"] += 1
            counts["reference_events"] += len(reference["journal"])
            counts["matched_events"] += len(projection.journal)
        levels = global_levels[key[:2]]
        cuts = sorted(levels | {max(levels) + 1})
        checked = compare_cuts(record, model, projection, candidate, cuts,
                               reference_model, mapping)
        terminal = key[2] == len(point_ids)
        if terminal:
            require(not projection.journal and not projection.roots,
                    "projection.terminal_order_empty")
            counts["terminal_empty_orders"] += 1
        counts["full_orders"] += 1
        counts["full_events"] += len(record["journal"])
        counts["projected_events"] += len(projection.journal)
        counts["omitted_pointless_continuations"] += projection.omitted
        counts["full_projection_cuts"] += checked
        counts["paired_reference_cuts"] += checked if common else 0
        counts["k1_zero_boundaries"] += 2 if key[2] == 1 else 0
        counts["full_minimum_leaves"] += model.leaves
        counts["full_merges"] += model.merges
        records_out.append({
            "case": key[0], "reversed_ids": key[1], "K": key[2],
            "comparison": "paired_reduced_journal" if common else
                          "terminal_empty" if terminal else "FULL_projection_only",
            "cuts": checked, "root_PointIds": sorted(point_ids) if key[2] == 1 else [],
            "projected_journal": projection.journal,
            "history_mapping_to_reference":
                [[source, target] for source, target in sorted(mapping.items())]
                if mapping is not None else None,
            "full_leaves": model.leaves, "full_merges": model.merges,
            "full_final_roots": model.roots,
            "omitted_pointless_continuations": projection.omitted,
        })
    require(counts["common_orders"] == 36 and counts["matched_events"] == 100 and
            counts["full_events"] == 285 and counts["terminal_empty_orders"] == 10 and
            counts["paired_reference_cuts"] > 500, "result.nonvacuity")
    e5_key = ("E5", False, 2)
    e5 = full_records[e5_key]
    e5_ids = full_cases[e5_key[:2]]["ids"]
    mutants = mutation_checks(e5, inspect_full(e5, e5_ids), e5_ids)
    for name, expected in INPUTS.items():
        require(hashlib.sha256((RECEIPTS / name).read_bytes()).hexdigest() == expected,
                "input.changed_during_replay")
    return {
        "schema": "mhgp7-full-to-reduced-journal-replay-v1",
        "status": "completed_bounded_journal_comparison",
        "phase": "exploration_v7_hors_registre", "backend": "cpu_reference",
        "profile": "quantized_u16_input_only",
        "mode": "audit_independant_math_and_architecture", "public_status": "not_claimed",
        "python_optimized": bool(sys.flags.optimize),
        "inputs": [full_pin, reduced_pin],
        "inspector_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "counts": dict(counts), "records": records_out, "mutants": mutants,
        "scope": {
            "authority": "Two pinned audit journals, inspected as data; no source oracle imported or executed.",
            "comparison": "8 common cases, K1..n-1; exact levels, unique event isomorphism preserving parents, deltas and multiplicity; ambiguous matching rejected.",
            "cuts": "Union of FULL and reduced journal levels across all orders of each case, both sides, plus zero and one terminal level; omitted Gamma levels are not newly enumerated.",
            "k1_zero": "Implicit reference singleton roots are assigned birth zero: open zero empty, closed zero all PointIds. This declares the replay boundary, not a new check of the original oracle's zero-cut code.",
            "unpaired": "10 terminal K=n projections empty; 4 other orders from the 2 obtuse FULL-only cases have no independent reduced journal comparison.",
            "limits": "No new geometry, general theorem, product run, high-rank enumeration, mass or vertical-map qualification; runtime cost of a future constructor is unmeasured.",
        },
        "gcp": "not_used",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--receipt", choices=("normal", "optimized"), required=True)
    arguments = parser.parse_args()
    try:
        require((arguments.receipt == "optimized") == bool(sys.flags.optimize),
                "invocation.optimization_label")
        report = run()
    except (Rejection, ValueError, KeyError, TypeError) as error:
        details = error.details if isinstance(error, Rejection) else {}
        print(json.dumps({"status": "rejected", "reason": str(error),
                          "details": details}, ensure_ascii=False), file=sys.stderr)
        return 1
    target = RECEIPTS / f"projection_{arguments.receipt}.json"
    target.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                      encoding="utf-8")
    print(json.dumps({"status": report["status"], "counts": report["counts"],
                      "mutants_rejected": len(report["mutants"]), "receipt": str(target)}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
