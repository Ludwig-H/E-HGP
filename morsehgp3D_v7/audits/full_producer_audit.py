"""Bounded independent Gabriel catalogues and FULL producer output judgment.

No FullPortal producer and no C++ invocation. Exact MEB certificates build the
input catalogues; exhaustive Gamma is a separate small-fixture judgment path.
"""

from __future__ import annotations

import argparse
from collections import Counter
from fractions import Fraction as Q
import hashlib
import itertools
import json
from pathlib import Path
import sys
from typing import Any

from full_cpp_audit import level, rational
from gabriel_full_probe import full_partition
from horizontal_rational_oracle import Gamma, coverage, facets
from meb_rational_oracle_20260905 import expected_meb, power

AUDIT = Path(__file__).resolve().parent
ROOT = AUDIT.parent.parent
OUT = AUDIT / "receipts_full_producer_20260905"
SEALED = AUDIT / "receipts_gabriel_20260905/full_normal.json"
SEALED_SHA = "00911669a9045b0bd466ee991d9972e764c46a0fa941d7ab79cfeca3082e627f"
AUTHORITY = "full_horizontal_relative_to_supplied_complete_exact_regular_gabriel_catalogues"
DIMENSIONS = dict(points="full_gabriel_point_budget", input_records="full_gabriel_input_budget",
                  aliases="full_gabriel_alias_budget", face_visits="full_gabriel_face_budget",
                  portal_requests="full_gabriel_portal_budget", chain_steps="full_gabriel_chain_budget",
                  meb_calls="full_gabriel_meb_call_budget", query_nodes="silent_query_node_budget",
                  meb_supports="silent_meb_support_budget", successor_steps="full_gabriel_successor_budget",
                  batches="full_gabriel_batch_budget", nodes="full_gabriel_node_budget",
                  parent_refs="full_gabriel_parent_budget")
GEOMETRY_FIELDS = {"core_records", "core_facets", "facets_with_two_intruders", "chain_steps",
                   "added_cofaces", "terminal_direct", "terminal_cached", "max_chain_length",
                   "query_nodes", "query_leaves", "query_range_skips", "meb_calls", "meb_supports"}
STAT_FIELDS = {"input_records", "face_visits", "aliases", "alias_hits", "portal_requests",
               "chain_steps", "terminal_direct", "max_chain_length", "normalized_anchors",
               "successor_steps", "no_op_connections", "meb_calls", "geometry"}


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path: Path, value: Any) -> None:
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n")


def bounded_samples(values: list[Q]) -> list[tuple[Q, bool]]:
    """Exact intermediate dyadics avoid an unbounded product of denominators."""
    ordered = sorted({Q(0), *values})
    result = []
    for i, value in enumerate(ordered):
        result.extend(((value, False), (value, True)))
        if i + 1 < len(ordered):
            following = ordered[i + 1]
            for shift in range(58):
                den = 1 << shift
                candidate = Q((value * den).numerator // (value * den).denominator + 1, den)
                if value < candidate < following:
                    result.append((candidate, True))
                    break
            else:
                raise ValueError("fixture.no_bounded_intermediate_cut")
    result.append((ordered[-1] + 1, True))
    return result


def corpus() -> list[dict[str, Any]]:
    require(digest(SEALED) == SEALED_SHA, "source.sealed_full_changed")
    source = json.loads(SEALED.read_text())
    require(len(source["cases"]) == 10 and len(source["records"]) == 50, "source.population")
    cases = [dict(c, origin="sealed_FULL_fixture") for c in source["cases"]]
    cases.append(dict(name="singleton", points=[[65535, 0, 65535]], ids=[4294967295],
                      reversed_ids=False, origin="new_bounded_fixture"))
    e5 = [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)]
    clouds = [("E5_chain_two", e5 + [(4, 2, 2), (12, 3, 11)]),
              ("E5_extended", e5 + [(2, 2, 4), (5, 6, 3)])]
    for seed in range(5):
        state = 0xE5F011 + seed * 65537
        points = []
        for _ in range(7):
            point = []
            for _ in range(3):
                state = (1664525 * state + 1013904223) & 0xFFFFFFFF
                point.append((state >> 8) % 97)
            points.append(tuple(point))
        clouds.append((f"deterministic_random_{seed}", points))
    for name, points in clouds:
        cases.append(dict(name=name, points=list(map(list, points)),
                          ids=[0, 17, 41, 63, 107, 4294967294, 4294967295],
                          reversed_ids=False, origin="new_bounded_fixture"))
    require(sum(len(c["ids"]) for c in cases) == 100, "fixture.order_budget")
    return cases


def catalogue(case: dict[str, Any]) -> tuple[list[dict[str, Any]], Gamma]:
    points = dict(zip(case["ids"], case["points"]))
    require(1 <= len(points) <= 7 and len(set(map(tuple, points.values()))) == len(points),
            "fixture.point_bound_or_duplicate")
    require(all(0 <= v <= 65535 for p in points.values() for v in p), "fixture.u16")
    records = []
    all_levels = {}
    for cardinal in range(2, len(points) + 1):
        for label in itertools.combinations(sorted(points), cardinal):
            ball = expected_meb([tuple(points[i]) for i in label])
            support = sorted(label[i] for i in ball["support"])
            values = {i: power(ball, tuple(p)) for i, p in points.items()}
            require(not ball["degenerate"] and {i for i, v in values.items() if v == 0} == set(support),
                    "fixture.unsupported_global_shell")
            all_levels[label] = ball["radius"]
            if any(v < 0 for i, v in values.items() if i not in label):
                continue
            interior = sorted(set(label) - set(support))
            require(all(values[i] < 0 for i in interior), "catalogue.interior")
            q, d = len(support), len(interior)
            records.append(dict(label=list(label), q=q, d=d, mask=(1 << q) - 1,
                                support=support, interior=interior, squared_level=str(ball["radius"]),
                                support_reserved=[0] * (11 - q), interior_reserved=[0] * (9 - d),
                                center=list(map(str, ball["center"])),
                                global_powers={str(i): str(v) for i, v in sorted(values.items())}))
    oracle = Gamma(case)
    require(oracle.levels == all_levels and oracle.direct == {tuple(e["label"]) for e in records},
            "catalogue.independent_gamma_agreement")
    return records, oracle


def reference_forest(oracle: Gamma, k: int) -> dict[str, Any]:
    nodes, minima, parents, leaf_sets, coverage_sets = [], [], [], [], []
    live: dict[frozenset[tuple[int, ...]], int] = {}
    for value in sorted({Q(0), *oracle.levels.values()}):
        before = full_partition(oracle.ids, oracle.levels, k, value, False)
        after = full_partition(oracle.ids, oracle.levels, k, value, True)
        require(set(before) == set(live), "reference.pre_level_partition")
        rows = [(image, sorted(live[c] for c in before if c <= image)) for image in after]
        births = sorted((image for image, refs in rows if not refs), key=lambda c: tuple(sorted(c)))
        merges = sorted(((refs, image) for image, refs in rows if len(refs) >= 2), key=lambda r: r[0])
        following = {image: refs[0] for image, refs in rows if len(refs) == 1}
        for image in births:
            require(len(image) == 1, "reference.birth_not_single_minimum")
            label = next(iter(image))
            require(k == 1 or label in oracle.direct, "reference.minimum_not_gabriel")
            token = len(nodes)
            nodes.append(dict(**level(value), first=len(minima), parent_count=0))
            minima.append(list(label))
            leaf_sets.append(frozenset({label}))
            coverage_sets.append(coverage(image))
            following[image] = token
        for refs, image in merges:
            token = len(nodes)
            nodes.append(dict(**level(value), first=len(parents), parent_count=len(refs)))
            parents.extend(refs)
            leaves = frozenset().union(*(leaf_sets[p] for p in refs))
            points = frozenset().union(*(coverage_sets[p] for p in refs))
            require(points == coverage(image), "reference.merge_gained_points")
            leaf_sets.append(leaves)
            coverage_sets.append(points)
            following[image] = token
        for image, token in following.items():
            require(coverage_sets[token] == coverage(image), "reference.continuation_gained_points")
        live = following
    cuts, roots = [], []
    for cut, closed in bounded_samples(list(oracle.levels.values())):
        active = {i for i, n in enumerate(nodes) if (rational(n) <= cut if closed else rational(n) < cut)}
        consumed = {p for i in active for p in parents[nodes[i]["first"]:nodes[i]["first"] + nodes[i]["parent_count"]]}
        cuts.append(dict(**level(cut), closed=int(closed)))
        roots.append(sorted(active - consumed))
    return dict(nodes=nodes, minima=minima, parents=parents, coverage=list(map(lambda c: sorted(c), coverage_sets)),
                descendant_minima=[sorted(map(list, c)) for c in leaf_sets], cuts=cuts, roots=roots)


def independent_stats(case: dict[str, Any], k: int, minimum: list[dict[str, Any]],
                      direct: list[dict[str, Any]], oracle: Gamma) -> dict[str, int]:
    aliases = ({(i,) for i in case["ids"]} if k == 1 else {tuple(e["label"]) for e in minimum})
    aliases |= {f for e in direct for f in facets(tuple(e["label"]))}
    noops = 0
    for event in direct:
        value = Q(event["squared_level"])
        boundary = frozenset(facets(tuple(event["label"])))
        image = next(c for c in full_partition(oracle.ids, oracle.levels, k, value, True) if boundary <= c)
        old = [c for c in full_partition(oracle.ids, oracle.levels, k, value, False) if c <= image]
        require(bool(old), "reference.connection_without_parent")
        noops += len(old) == 1
    return dict(input_records=len(minimum) + len(direct), aliases=len(aliases),
                face_visits=sum(e["q"] + len(e["label"]) for e in direct), no_op_connections=noops)


def make_fixtures() -> dict[str, Any]:
    cases = corpus()
    records = []
    catalogues = []
    for case_index, case in enumerate(cases):
        events, oracle = catalogue(case)
        if case["name"] == "E5":
            ordinals = [expected_meb([tuple(case["points"][i]) for i in label])["charged"]
                        for label in ((0, 2), (0, 3, 4), (2, 3, 4))]
            require(ordinals == [1, 4, 4], "fixture.E5_support_ordinals")
        catalogues.append(dict(case_index=case_index, events=events,
                               global_regular_labels_checked=len(oracle.levels)))
        for k in range(1, len(case["ids"]) + 1):
            minimum = [e for e in events if len(e["label"]) == k] if k > 1 else []
            direct = [e for e in events if len(e["label"]) == k + 1]
            expected = reference_forest(oracle, k)
            stats = independent_stats(case, k, minimum, direct, oracle)
            for variant in (0, 1):
                rid = len(records)

                def encode(source: list[dict[str, Any]], offset: int) -> list[dict[str, Any]]:
                    result = []
                    for i, event in enumerate(source):
                        factor = 1 if not variant else 2 + (rid + 7 * i + offset) % 17
                        result.append(dict(q=event["q"], d=event["d"], mask=event["mask"],
                                           support=event["support"], interior=event["interior"],
                                           **level(Q(event["squared_level"]), factor)))
                    return result if not variant else list(reversed(result))

                points = [dict(id=i, xyz=p) for i, p in zip(case["ids"], case["points"])]
                if variant:
                    points.reverse()
                cuts = [dict(**level(rational(c), 1 if not variant else 23 + (rid + 3 * i) % 19),
                             closed=c["closed"]) for i, c in enumerate(expected["cuts"])]
                probe = (not variant and not case["reversed_ids"] and k == 2
                         and case["name"] in {"E5", "E5_chain_two"})
                records.append(dict(id=rid, case_index=case_index, order=k, representation=variant,
                                    points=points, minima_source=encode(minimum, 0),
                                    direct_source=encode(direct, 3), cuts=cuts, budget_probe=int(probe),
                                    expected=expected, expected_stats=stats,
                                    named_meb_support_sum=5 if case["name"] == "E5" and k == 2 else None))
    require(len(records) == 200 and sum(r["budget_probe"] for r in records) == 2, "fixture.record_budget")
    paths = [Path(__file__), SEALED, AUDIT / "full_cpp_audit.py", AUDIT / "gabriel_full_probe.py",
             AUDIT / "gabriel_portal_probe.py", AUDIT / "horizontal_rational_oracle.py",
             AUDIT / "meb_rational_oracle_20260905.py"]
    chain = next(c for c in catalogues if cases[c["case_index"]]["name"] == "E5_chain_two")
    by_label = {tuple(e["label"]): e for e in chain["events"]}
    require((0, 17, 41) in by_label and (63, 107, 4294967294) in by_label
            and (41, 63, 107) not in by_label, "fixture.chain_two_catalogue")
    result = dict(schema="audit-full-producer-fixtures-v1", status="prepared_bounded_fixtures",
                  public_status="not_claimed", unique_orders=100, representations=200,
                  cases=cases, catalogues=catalogues, records=records,
                  sources_sha256={str(p.relative_to(ROOT)): digest(p) for p in paths},
                  selection_note="Fixed seven new 7-point clouds, five LCG seeds and two E5 extensions. "
                                 "An initial E5 extension with (3,2,3),(6,7,5) had only one-step candidates; "
                                 "it was replaced before C++ fixtures by the explicit chain-two construction. "
                                 "No C++ result or long-chain observation is attributed at preparation.",
                  chain_two_mathematical_witness=dict(facet=[0, 41], first_level="33/2",
                                                      intermediates=[[41, 63, 107], [63, 107, 4294967294]],
                                                      levels=["162/25", "21/4"],
                                                      intermediate_intruder_power="-3/5",
                                                      scope="Valid strict path; engine choice is observed separately"),
                  named_support_counter_witness=dict(case="E5", order=2, initial_facet="AC",
                                                      initial_ordinal=1, possible_terminals=["ADE", "CDE"],
                                                      terminal_ordinals=[4, 4], cumulative_supports=5,
                                                      scope="Two foreign points force one replacement; both terminals are acute triangles. "
                                                            "Expected MEB oracle verifies the ordinals for both PointId assignments."),
                  preparation_diagnostic=dict(status="corrected_audit_transport_limit",
                                              failed_method="arithmetic midpoint of neighboring rational levels",
                                              failure="fixture.level_bound: midpoint denominator, multiplied by variant factor, can exceed u64",
                                              replacement="strictly interior dyadic, verified by Fraction; denominator <=2^57 before scaling",
                                              attribution="Audit transport preparation only; no product run or product defect."),
                  scope="Complete exact regular Gabriel catalogues for bounded fixtures, independent FULL "
                        "Gamma cut judge. No FullPortal producer call, no C++ execution at preparation, "
                        "no global catalogue algorithm, mass, vertical-map or performance claim.")
    return result


def fixture_text(fixture: dict[str, Any]) -> str:
    lines = [f"FULLPROD1 {len(fixture['records'])}"]
    for r in fixture["records"]:
        lines.append(" ".join(map(str, (r["id"], len(r["points"]), r["order"], len(r["minima_source"]),
                                       len(r["direct_source"]), len(r["cuts"]), r["budget_probe"]))))
        lines.extend(" ".join(map(str, (p["id"], *p["xyz"]))) for p in r["points"])
        for e in r["minima_source"] + r["direct_source"]:
            lines.append(" ".join(map(str, (e["q"], e["d"], e["mask"], *e["num"], e["den"],
                                            *e["support"], *e["interior"]))))
        lines.extend(" ".join(map(str, (*c["num"], c["den"], c["closed"]))) for c in r["cuts"])
    return "\n".join(lines) + "\n"


def check_stats(stats: dict[str, Any], expected: dict[str, int]) -> None:
    require(set(stats) == STAT_FIELDS and set(stats["geometry"]) == GEOMETRY_FIELDS, "producer.stats_schema")
    require(all(type(v) is int and 0 <= v < 1 << 64 for key, v in stats.items() if key != "geometry")
            and all(type(v) is int and 0 <= v < 1 << 64 for v in stats["geometry"].values()),
            "producer.stats_domain")
    for key, value in expected.items():
        require(stats[key] == value, "producer.stats_exact." + key)
    require(stats["terminal_direct"] == stats["portal_requests"]
            and stats["chain_steps"] >= stats["portal_requests"]
            and stats["meb_calls"] == stats["portal_requests"] + stats["chain_steps"],
            "producer.portal_accounting")
    if stats["portal_requests"]:
        require(1 <= stats["max_chain_length"] <= stats["chain_steps"]
                <= stats["portal_requests"] * stats["max_chain_length"], "producer.chain_lengths")
    else:
        require(stats["max_chain_length"] == 0, "producer.empty_chain_length")
    geo = stats["geometry"]
    require(geo["meb_calls"] == stats["meb_calls"] and geo["meb_supports"] >= geo["meb_calls"],
            "producer.geometry_meb_accounting")
    require(all(geo[key] == 0 for key in GEOMETRY_FIELDS -
                {"query_nodes", "query_leaves", "query_range_skips", "meb_calls", "meb_supports"}),
            "producer.old_core_not_called")
    require(stats["normalized_anchors"] <= stats["successor_steps"], "producer.successor_accounting")


def budget_values(record: dict[str, Any], actual: dict[str, Any]) -> dict[str, int]:
    stats = actual["stats"]
    return dict(points=len(record["points"]), input_records=stats["input_records"], aliases=stats["aliases"],
                face_visits=stats["face_visits"], portal_requests=stats["portal_requests"],
                chain_steps=stats["chain_steps"], meb_calls=stats["meb_calls"],
                query_nodes=stats["geometry"]["query_nodes"], meb_supports=stats["geometry"]["meb_supports"],
                successor_steps=stats["successor_steps"],
                batches=len({rational(n) for n in actual["nodes"]}), nodes=len(actual["nodes"]),
                parent_refs=len(actual["parents"]))


def check_output(path: Path, fixture: dict[str, Any]) -> dict[str, Any]:
    rows = json.loads(path.read_text())["records"]
    require(len(rows) == len(fixture["records"]) == 200, "producer.record_count")
    oracles = {i: Gamma(c) for i, c in enumerate(fixture["cases"])}
    totals: Counter[str] = Counter()
    pair_signatures = []
    long_chains = []
    for r, actual in zip(fixture["records"], rows):
        expected = r["expected"]
        require(actual["id"] == r["id"] and actual["order"] == r["order"], "producer.identity")
        require(actual["status"] == 0 and actual["reason"] == AUTHORITY, "producer.status")
        require(len(actual["nodes"]) == len(expected["nodes"]), "producer.node_count")
        for node, ref in zip(actual["nodes"], expected["nodes"]):
            require(rational(node) == rational(ref), "producer.node_level")
            require(node["first"] == ref["first"] and node["parent_count"] == ref["parent_count"],
                    "producer.node_structure")
        for field in ("minima", "parents", "coverage"):
            require(actual[field] == expected[field], "producer." + field)
        check_stats(actual["stats"], r["expected_stats"])
        if r["named_meb_support_sum"] is not None:
            require(actual["stats"]["geometry"]["meb_supports"] == r["named_meb_support_sum"],
                    "producer.named_E5_support_sum")
        require(len(actual["cuts"]) == len(r["cuts"]), "producer.cut_count")
        oracle = oracles[r["case_index"]]
        previous = []
        for cut, row, roots in zip(r["cuts"], actual["cuts"], expected["roots"]):
            require(row["status"] == 0 and row["reason"] == "structural_only", "producer.cut_status")
            require(row["roots"] == roots, "producer.cut_roots")
            reference = full_partition(oracle.ids, oracle.levels, r["order"], rational(cut), bool(cut["closed"]))
            images, current = [], []
            for root in row["roots"]:
                leaves = frozenset(map(tuple, expected["descendant_minima"][root]))
                candidates = [c for c in reference if leaves <= c]
                require(len(candidates) == 1 and candidates[0] not in images, "producer.gamma_inclusion")
                image = candidates[0]
                require(set(actual["coverage"][root]) == coverage(image), "producer.gamma_coverage")
                images.append(image)
                current.append((leaves, image))
            require(set(images) == set(reference), "producer.gamma_surjective")
            for old_leaves, old_image in previous:
                targets = [image for leaves, image in current if old_leaves <= leaves]
                require(len(targets) == 1 and old_image <= targets[0], "producer.gamma_naturality")
                totals["naturality_squares"] += 1
            previous = current
            totals["cuts"] += 1
        trials = actual["budget_trials"]
        if r["budget_probe"]:
            caps = budget_values(r, actual)
            wanted = {("all", "exact")} | {(d, "minus_one") for d, value in caps.items() if value}
            require(len(trials) == len(wanted) and {(t["dimension"], t["kind"]) for t in trials} == wanted,
                    "producer.budget_trial_population")
            for trial in trials:
                if trial["kind"] == "exact":
                    require(trial["status"] == 0 and trial["reason"] == AUTHORITY and not trial["empty"]
                            and trial["same"] is True and trial["stats"] == actual["stats"], "producer.exact_budget")
                    totals["exact_budget_successes"] += 1
                else:
                    dim = trial["dimension"]
                    require(trial["status"] == 3 and trial["reason"] == DIMENSIONS[dim] and trial["empty"] is True,
                            "producer.minus_one." + dim)
                    require(trial["cap"] == caps[dim] - 1, "producer.minus_one_cap." + dim)
                    if dim in {"query_nodes", "meb_supports"}:
                        count = trial["stats"]["geometry"][dim]
                    else:
                        count = trial["stats"].get(dim)
                    if count is not None:
                        require(0 <= count <= caps[dim] - 1, "producer.prospective_budget." + dim)
                    totals["minus_one_budget_refusals"] += 1
        else:
            require(trials == [], "producer.unrequested_budget_trials")
        stats = actual["stats"]
        if stats["max_chain_length"] > 1:
            long_chains.append(dict(id=r["id"], case=fixture["cases"][r["case_index"]]["name"],
                                    order=r["order"], max_chain_length=stats["max_chain_length"],
                                    portal_requests=stats["portal_requests"], chain_steps=stats["chain_steps"]))
        for key in ("portal_requests", "chain_steps", "normalized_anchors", "meb_calls", "face_visits"):
            totals[key] += stats[key]
        totals["records"] += 1
        totals["nodes"] += len(actual["nodes"])
        totals["minima"] += len(actual["minima"])
        totals["parent_refs"] += len(actual["parents"])
        pair_signatures.append(dict(levels=list(map(lambda n: str(rational(n)), actual["nodes"])),
                                    minima=actual["minima"], parents=actual["parents"], coverage=actual["coverage"],
                                    roots=[c["roots"] for c in actual["cuts"]], stats=stats))
    require(all(a == b for a, b in zip(pair_signatures[::2], pair_signatures[1::2])), "producer.paired_variants")
    for key in ("portal_requests", "chain_steps", "normalized_anchors", "cuts", "naturality_squares",
                "exact_budget_successes", "minus_one_budget_refusals"):
        require(totals[key] > 0, "nonvacuity." + key)
    return dict(status="completed_bounded_full_producer_judgment", public_status="not_claimed",
                python_optimized=not __debug__, counts=dict(totals), long_chains_observed=long_chains,
                long_chain_floor_met=bool(long_chains), output_sha256=digest(path),
                fixtures_sha256=digest(OUT / "fixtures.json"), fixture_text_sha256=digest(OUT / "fixtures.txt"),
                checker_sha256=digest(Path(__file__)),
                scope="100 unique orders/200 representations on independent complete regular catalogues; "
                      "bounded FULL Gamma agreement, node/coverage/cut and prospective budget checks. "
                      "No WSPD source completeness, mass, vertical, industrial-scale or runtime claim.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prepare", action="store_true")
    parser.add_argument("--check", type=Path)
    parser.add_argument("--result", type=Path)
    args = parser.parse_args()
    require(args.prepare != (args.check is not None), "arguments.choose_mode")
    fixture = make_fixtures()
    if args.prepare:
        OUT.mkdir(exist_ok=True)
        save(OUT / "fixtures.json", fixture)
        (OUT / "fixtures.txt").write_text(fixture_text(fixture))
        print(json.dumps(dict(status=fixture["status"], orders=fixture["unique_orders"], records=len(fixture["records"]))))
        return 0
    require(args.result is not None, "arguments.result_required")
    require(json.loads((OUT / "fixtures.json").read_text()) == fixture, "fixture.changed")
    require((OUT / "fixtures.txt").read_text() == fixture_text(fixture), "fixture.text_changed")
    try:
        result = check_output(args.check, fixture)
    except (ValueError, KeyError, TypeError) as error:
        save(args.result, dict(status="rejected", reason=str(error), python_optimized=not __debug__,
                               output_sha256=digest(args.check), checker_sha256=digest(Path(__file__))))
        print(str(error), file=sys.stderr)
        return 1
    save(args.result, result)
    print(json.dumps(dict(status=result["status"], counts=result["counts"], long_chains=result["long_chains_observed"])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
