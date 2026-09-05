"""Prepare and judge bounded FULL C++ fixtures from the sealed Python journal.

This script never invokes the product or the FullPortal producer. Gamma supplies
only the independent geometric judge, bounded to seven input points. Separate
structural fixtures make no geometric-realizability claim.
"""

from __future__ import annotations

import argparse
from collections import Counter
from fractions import Fraction as Q
import hashlib
import json
from pathlib import Path
import sys
from typing import Any

from gabriel_full_probe import full_partition
from horizontal_rational_oracle import Gamma, coverage

AUDIT = Path(__file__).resolve().parent
ROOT = AUDIT.parent.parent
OUT = AUDIT / "receipts_full_cpp_20260905"
SEALED = AUDIT / "receipts_gabriel_20260905/full_normal.json"
SEALED_SHA = "00911669a9045b0bd466ee991d9972e764c46a0fa941d7ab79cfeca3082e627f"


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path: Path, value: Any) -> None:
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n")


def level(value: Q, factor: int = 1) -> dict[str, Any]:
    numerator, denominator = value.numerator * factor, value.denominator * factor
    require(0 <= numerator < 1 << 192 and 0 < denominator < 1 << 64, "fixture.level_bound")
    return dict(num=[(numerator >> (64 * i)) & ((1 << 64) - 1) for i in range(3)],
                den=denominator)


def rational(raw: dict[str, Any]) -> Q:
    return Q(sum(word << (64 * i) for i, word in enumerate(raw["num"])), raw["den"])


def samples(levels: list[Q]) -> list[tuple[Q, bool]]:
    levels = sorted({Q(0), *levels})
    result = []
    for i, value in enumerate(levels):
        result.extend(((value, False), (value, True)))
        if i + 1 < len(levels):
            result.append(((value + levels[i + 1]) / 2, True))
    result.append((levels[-1] + 1, True))
    return result


def canonicalize(journal: list[dict[str, Any]], k: int, rid: int,
                 variant: str, cuts: list[tuple[Q, bool]]) -> dict[str, Any]:
    batches, nodes, minima, parents = [], [], [], []
    mapping: dict[int, int] = {}
    leaf_sets: list[frozenset[tuple[int, ...]]] = []
    node_sets: list[frozenset[int]] = []
    for batch_index, value in enumerate(sorted({Q(e["level"]) for e in journal})):
        events = [e for e in journal if Q(e["level"]) == value]
        births = sorted((e for e in events if e["kind"] == "birth"), key=lambda e: tuple(e["label"]))
        merges = sorted(((sorted(mapping[p] for p in e["parents"]), e)
                         for e in events if e["kind"] == "merge"), key=lambda row: row[0])
        raw = level(value, 1 if variant == "reduced_fraction" else 2 + (rid + 5 * batch_index) % 13)
        batch = dict(**raw, births=[e["label"] for e in births], merges=[p for p, _ in merges])
        prior_count = len(nodes)
        for event in births:
            ident = len(nodes)
            mapping[event["output"]] = ident
            nodes.append(dict(**raw, first=len(minima), parent_count=0))
            minima.append(event["label"])
            leaf_sets.append(frozenset({tuple(event["label"])}))
            node_sets.append(frozenset({ident}))
        for refs, event in merges:
            require(len(refs) >= 2 and len(set(refs)) == len(refs) and max(refs) < prior_count,
                    "fixture.parents_not_strict")
            ident = len(nodes)
            mapping[event["output"]] = ident
            nodes.append(dict(**raw, first=len(parents), parent_count=len(refs)))
            parents.extend(refs)
            leaf_sets.append(frozenset().union(*(leaf_sets[p] for p in refs)))
            node_sets.append(frozenset({ident}).union(*(node_sets[p] for p in refs)))
        batches.append(batch)
    encoded_cuts, expected_cuts = [], []
    for cut_index, (cut, closed) in enumerate(cuts):
        factor = 1 if variant == "reduced_fraction" else 17 + (3 * rid + 7 * cut_index) % 19
        encoded_cuts.append(dict(**level(cut, factor), closed=int(closed)))
        active = {i for i, node in enumerate(nodes)
                  if (rational(node) <= cut if closed else rational(node) < cut)}
        consumed = {p for i in active for p in
                    parents[nodes[i]["first"]:nodes[i]["first"] + nodes[i]["parent_count"]]}
        expected_cuts.append(sorted(active - consumed))
    return dict(batches=batches, nodes=nodes, minima=minima, parents=parents,
                old_to_dense={str(old): new for old, new in sorted(mapping.items())},
                descendant_minima=[sorted(map(list, group)) for group in leaf_sets],
                descendant_nodes=[sorted(group) for group in node_sets],
                subtree_nodes=[len(group) for group in node_sets],
                point_refs=[k * len(group) for group in leaf_sets],
                coverage=[sorted({p for facet in group for p in facet}) for group in leaf_sets],
                cuts=encoded_cuts, cut_roots=expected_cuts)


def structural_cases() -> list[dict[str, Any]]:
    base = [dict(kind="birth", level="1", output=i, label=label)
            for i, label in enumerate(([0, 1], [0, 2], [1, 3], [2, 3]))]
    base += [dict(kind="merge", level="2", output=4, parents=[0, 3]),
             dict(kind="merge", level="2", output=5, parents=[1, 2]),
             dict(kind="merge", level="3", output=6, parents=[4, 5])]
    remap = {0: 4294967295, 1: 63, 2: 17, 3: 0}
    permuted = [dict(e, label=sorted(remap[p] for p in e["label"])) if e["kind"] == "birth"
                else dict(e) for e in base]
    mixed = base[:4] + [dict(kind="birth", level="2", output=4, label=[4, 5]),
                       dict(kind="merge", level="2", output=5, parents=[0, 3]),
                       dict(kind="merge", level="2", output=6, parents=[1, 2]),
                       dict(kind="merge", level="3", output=7, parents=[5, 6]),
                       dict(kind="merge", level="4", output=8, parents=[4, 7])]
    return [dict(name="overlap_distinct_roots", ids=[0, 1, 2, 3], journal=base),
            dict(name="overlap_reindexed_u32max", ids=sorted(remap.values()), journal=permuted),
            dict(name="mixed_birth_merges", ids=list(range(6)), journal=mixed)]


def make_fixtures() -> dict[str, Any]:
    require(digest(SEALED) == SEALED_SHA, "source.sealed_full_changed")
    source = json.loads(SEALED.read_text())
    require(all(digest(ROOT / name) == pin for name, pin in source["sources_sha256"].items()),
            "source.sealed_oracle_dependencies_changed")
    require(len(source["records"]) == 50 and sum(len(r["journal"]) for r in source["records"]) == 285,
            "source.sealed_population")
    cases = {(c["name"], c["reversed_ids"]): c for c in source["cases"]}
    oracles = {key: Gamma(case) for key, case in cases.items()}
    records = []
    for source_index, row in enumerate(source["records"]):
        case = cases[row["case"], row["reversed_ids"]]
        oracle = oracles[row["case"], row["reversed_ids"]]
        for variant in ("reduced_fraction", "scaled_fraction"):
            rid = len(records)
            encoded = canonicalize(row["journal"], row["K"], rid, variant,
                                   samples(list(oracle.levels.values())))
            records.append(dict(id=rid, kind="gamma_fixture", source_record=source_index,
                                case=case, order=row["K"], points=sorted(case["ids"]),
                                representation=variant, **encoded))
    for case in structural_cases():
        for variant in ("reduced_fraction", "scaled_fraction"):
            rid = len(records)
            encoded = canonicalize(case["journal"], 2, rid, variant,
                                   samples([Q(e["level"]) for e in case["journal"]]))
            records.append(dict(id=rid, kind="structural_only", case=case["name"], order=2,
                                points=case["ids"], representation=variant, **encoded))
    paths = [Path(__file__), SEALED, AUDIT / "gabriel_full_probe.py", AUDIT / "gabriel_portal_probe.py",
             AUDIT / "horizontal_rational_oracle.py", AUDIT / "meb_rational_oracle_20260905.py"]
    result = dict(schema="audit-full-cpp-fixtures-v1", status="prepared_bounded_fixtures",
                  public_status="not_claimed", sealed_records=50, sealed_events=285,
                  gamma_representations=100, structural_representations=6, records=records,
                  sources_sha256={str(p.relative_to(ROOT)): digest(p) for p in paths},
                  scope="Sealed FULL journal translation, independent Gamma cut judge, "
                        "separate structural overlap/mixed-batch cases. No C++ run at preparation.")
    for left, right in zip(records[::2], records[1::2]):
        for field in ("order", "points", "minima", "parents", "old_to_dense", "descendant_minima",
                      "descendant_nodes", "subtree_nodes", "point_refs", "coverage", "cut_roots"):
            require(left[field] == right[field], "fractions.semantic_pair." + field)
        require([rational(n) for n in left["nodes"]] == [rational(n) for n in right["nodes"]]
                and [rational(c) for c in left["cuts"]] == [rational(c) for c in right["cuts"]],
                "fractions.level_pair")
    for record in records[100:]:
        index = next(i for i, c in enumerate(record["cuts"]) if rational(c) == 2 and c["closed"])
        roots = record["cut_roots"][index]
        if record["case"] == "mixed_birth_merges":
            require(roots == [4, 5, 6] and record["nodes"][4]["parent_count"] == 0
                    and record["coverage"][5] == record["coverage"][6] == [0, 1, 2, 3],
                    "structural.mixed_birth_first")
        else:
            require(roots == [4, 5] and record["coverage"][4] == record["coverage"][5]
                    == record["points"], "structural.equal_coverage_distinct_roots")
    require(4294967295 in records[102]["points"], "structural.u32max_reindexing")
    geometric_check(records, oracles)
    require(digest(SEALED) == SEALED_SHA, "source.sealed_full_changed_during_prepare")
    return result


def geometric_check(records: list[dict[str, Any]],
                    oracles: dict[tuple[str, bool], Gamma] | None = None) -> dict[str, int]:
    cache = {} if oracles is None else oracles
    totals: Counter[str] = Counter()
    for record in records:
        if record["kind"] != "gamma_fixture":
            continue
        case = record["case"]
        key = case["name"], case["reversed_ids"]
        if key not in cache:
            cache[key] = Gamma(case)
        oracle = cache[key]
        previous: list[tuple[frozenset[tuple[int, ...]], frozenset[tuple[int, ...]]]] = []
        for cut, roots in zip(record["cuts"], record["cut_roots"]):
            reference = full_partition(oracle.ids, oracle.levels, record["order"],
                                       rational(cut), bool(cut["closed"]))
            images = []
            current = []
            for root in roots:
                leaves = frozenset(map(tuple, record["descendant_minima"][root]))
                targets = [c for c in reference if leaves <= c]
                require(len(targets) == 1, "gamma.minimum_inclusion")
                image = targets[0]
                require(image not in images, "gamma.minimum_injective")
                require(set(record["coverage"][root]) == coverage(image), "gamma.coverage")
                images.append(image)
                current.append((leaves, image))
            require(set(images) == set(reference), "gamma.minimum_surjective")
            for old_leaves, old_image in previous:
                successors = [image for leaves, image in current if old_leaves <= leaves]
                require(len(successors) == 1 and old_image <= successors[0], "gamma.naturality")
                totals["gamma_naturality_squares"] += 1
            previous = current
            totals["gamma_cuts"] += 1
        totals["gamma_records"] += 1
    require(totals["gamma_records"] == 100 and totals["gamma_cuts"] == 4530,
            "gamma.nonvacuity_population")
    return dict(totals)


def fixture_text(fixture: dict[str, Any]) -> str:
    lines = [f"FULLCPP1 {len(fixture['records'])}"]
    for record in fixture["records"]:
        lines.append(" ".join(map(str, (record["id"], record["order"], len(record["points"]),
                                       len(record["batches"]), len(record["cuts"])))))
        lines.append(" ".join(map(str, record["points"])))
        for batch in record["batches"]:
            lines.append(" ".join(map(str, (*batch["num"], batch["den"],
                                            len(batch["births"]), len(batch["merges"])))))
            lines.extend(" ".join(map(str, f)) for f in batch["births"])
            lines.extend(" ".join(map(str, (len(p), *p))) for p in batch["merges"])
        lines.extend(f"{n} {p}" for n, p in zip(record["subtree_nodes"], record["point_refs"]))
        lines.extend(" ".join(map(str, (*cut["num"], cut["den"], cut["closed"])))
                     for cut in record["cuts"])
    return "\n".join(lines) + "\n"


def success(value: dict[str, Any], context: str) -> None:
    require(value.get("status") == 0 and value.get("reason") == "structural_only", context)


def exhausted(value: dict[str, Any], reason: str) -> None:
    require(value.get("status") == 2 and value.get("reason") == reason and value.get("empty") is True,
            "full_cpp.refusal." + reason)


def check_output(output: Path, fixture: dict[str, Any]) -> dict[str, Any]:
    raw = json.loads(output.read_text())
    require(isinstance(raw, dict) and isinstance(raw.get("records"), list), "full_cpp.transport")
    rows = raw["records"]
    require(len(rows) == len(fixture["records"]) == 106, "full_cpp.record_count")
    counts: Counter[str] = Counter()
    for expected, actual in zip(fixture["records"], rows):
        require(actual.get("id") == expected["id"] and actual.get("order") == expected["order"],
                "full_cpp.identity")
        success(actual, "full_cpp.build_status")
        require(actual.get("nodes") == expected["nodes"], "full_cpp.nodes")
        require(actual.get("minima") == expected["minima"], "full_cpp.minima")
        require(actual.get("parents") == expected["parents"], "full_cpp.parents")
        require(len(actual.get("coverage", [])) == len(expected["nodes"]), "full_cpp.coverage_count")
        for node, entry in enumerate(actual["coverage"]):
            require(entry.get("node") == node, "full_cpp.coverage_node")
            success(entry, "full_cpp.coverage_status")
            values = entry.get("values")
            require(isinstance(values, list) and values == sorted(set(values)), "full_cpp.coverage_duplicates")
            require(values == expected["coverage"][node], "full_cpp.coverage_values")
            exhausted(entry.get("under_nodes", {}), "full_read_node_budget")
            exhausted(entry.get("under_points", {}), "full_read_point_budget")
            counts["node_coverage_checks"] += 1
            counts["exact_subtree_budget_successes"] += 1
            counts["subtree_budget_minus_one_refusals"] += 2
        require(len(actual.get("cuts", [])) == len(expected["cuts"]), "full_cpp.cut_count")
        for cut, roots, entry in zip(expected["cuts"], expected["cut_roots"], actual["cuts"]):
            success(entry, "full_cpp.cut_status")
            observed = entry.get("roots")
            if observed != roots:
                extras = set(observed or []) - set(roots)
                ancestors = set().union(*(set(expected["descendant_nodes"][r]) - {r} for r in roots))
                require(not extras & ancestors, "full_cpp.cut_parent_still_active")
                boundary = {i for i, n in enumerate(expected["nodes"]) if rational(n) == rational(cut)}
                require(not (set(observed or []) ^ set(roots)) & boundary, "full_cpp.cut_boundary")
                raise ValueError("full_cpp.cut_roots")
            counts["root_cut_checks"] += 1
        exhausted(actual.get("build_under_nodes", {}), "full_node_budget")
        counts["build_node_budget_minus_one_refusals"] += 1
        counts["records"] += 1
        counts[expected["kind"] + "_records"] += 1
        counts["nodes"] += len(expected["nodes"])
        counts["parent_refs"] += len(expected["parents"])
        counts["minima"] += len(expected["minima"])
    counts.update(geometric_check(fixture["records"]))
    for name in ("node_coverage_checks", "root_cut_checks", "subtree_budget_minus_one_refusals",
                 "build_node_budget_minus_one_refusals", "gamma_naturality_squares"):
        require(counts[name] > 0, "nonvacuity." + name)
    return dict(status="completed_bounded_cpp_output_judgment", public_status="not_claimed",
                counts=dict(counts), python_optimized=not __debug__, output_sha256=digest(output),
                fixture_sha256=digest(OUT / "fixtures.json"),
                fixture_text_sha256=digest(OUT / "fixtures.txt"), checker_sha256=digest(Path(__file__)),
                scope="100 geometric representations plus 6 structural representations. "
                      "Gamma checks only the geometric population. No producer completeness, "
                      "mass, vertical-map or performance claim.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prepare", action="store_true")
    parser.add_argument("--check", type=Path)
    parser.add_argument("--result", type=Path)
    args = parser.parse_args()
    require(args.prepare != (args.check is not None), "arguments.choose_prepare_or_check")
    fixture = make_fixtures()
    if args.prepare:
        OUT.mkdir(exist_ok=True)
        save(OUT / "fixtures.json", fixture)
        (OUT / "fixtures.txt").write_text(fixture_text(fixture))
        print(json.dumps(dict(status=fixture["status"], records=len(fixture["records"]),
                              gamma=fixture["gamma_representations"], structural=fixture["structural_representations"])))
        return 0
    require(args.result is not None, "arguments.result_required")
    require(json.loads((OUT / "fixtures.json").read_text()) == fixture, "fixtures.changed")
    require((OUT / "fixtures.txt").read_text() == fixture_text(fixture), "fixtures.text_changed")
    try:
        result = check_output(args.check, fixture)
    except (ValueError, KeyError, TypeError) as error:
        save(args.result, dict(status="rejected", reason=str(error), checker_sha256=digest(Path(__file__)),
                               output_sha256=digest(args.check), python_optimized=not __debug__))
        print(str(error), file=sys.stderr)
        return 1
    save(args.result, result)
    print(json.dumps(dict(status=result["status"], counts=result["counts"])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
