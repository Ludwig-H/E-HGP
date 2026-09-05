"""Independent Gamma judgment of captured FULL EAGER and first-C policies."""
from __future__ import annotations

import argparse
from collections import Counter
import copy
from fractions import Fraction as Q
import json
from pathlib import Path
import sys

import full_producer_audit as reference

AUDIT = Path(__file__).resolve().parent
ROOT = AUDIT.parents[1]
OUT = AUDIT / "receipts_full_lazy_20260905"
OLD = AUDIT / "receipts_full_producer_20260905"
SEALED = {
    "fixtures.json": "fd6b4b9d213729a85edcdbce46c7232cdb64ab4745185bf326c18cefefc2c6c9",
    "O2_output.json": "9587acd7157fdbeb56b721c023695029f4cf50821e138b67b34eff940addb384",
}
EXTRA_STATS = {"minimum_lookups", "minimum_hits", "cache_lookups", "cache_hits",
               "cache_inserts", "cache_skips", "singleton_intruder_resolutions", "direct_lookups"}
POLICIES = ("eager_all_incident_facets_v1", "lazy_first_c_strict_resolutions_v1")
MODES = ((0, 0), (1, 0), (1, 1), (1, 100000))
require = reference.require
sha = reference.digest
save = reference.save


def verify_fixtures(fixture: dict) -> None:
    for name, expected in SEALED.items():
        require(sha(OLD / name) == expected, "sealed_base_changed." + name)
    for name, expected in fixture["source_pins"].items():
        require(sha(ROOT / name) == expected, "fixture_source_changed." + name)
    require(len(fixture["records"]) == 872 and len(fixture["cases"]) == 20,
            "fixture_population")
    require((OUT / "fixtures.txt").read_text() == fixture_text(fixture), "fixture_transport_changed")


def compact_fixture(fixture: dict) -> dict:
    """Keep the sealed original data by hash instead of four redundant copies."""
    result = {k: v for k, v in fixture.items() if k not in {"cases", "catalogues", "records"}}
    result["added_cases"] = fixture["cases"][18:]
    result["added_catalogues"] = fixture["catalogues"][18:]
    result["added_records"] = [dict({k: v for k, v in r.items() if k not in {"base_id", "lazy", "capacity"}}, id=r["base_id"])
                               for r in fixture["records"] if r["base_id"] >= 200 and not r["lazy"]]
    return result


def load_fixtures() -> dict:
    data = json.loads((OUT / "fixtures.json").read_text())
    for name, expected in SEALED.items():
        require(sha(OLD / name) == expected, "sealed_base_changed." + name)
    old = json.loads((OLD / "fixtures.json").read_text())
    records = []
    for r in old["records"] + data["added_records"]:
        for lazy, capacity in MODES:
            records.append(dict(r, id=len(records), base_id=r["id"], lazy=lazy, capacity=capacity))
    return dict(data, cases=old["cases"] + data["added_cases"],
                catalogues=old["catalogues"] + data["added_catalogues"], records=records)


def make_fixtures() -> dict:
    for name, expected in SEALED.items():
        require(sha(OLD / name) == expected, "sealed_base_changed." + name)
    old = json.loads((OLD / "fixtures.json").read_text())
    cases = copy.deepcopy(old["cases"])
    base = copy.deepcopy(old["records"])
    catalogues = copy.deepcopy(old["catalogues"])
    additions = [
        ("lazy_J1", [[0, 5, 0], [4, 5, 0], [2, 6, 0], [2, 0, 0]]),
        ("lazy_shared_lot", [[0, 50, 0], [40, 50, 0], [20, 61, 0], [20, 0, 0], [20, 10, 30]]),
    ]
    for name, points in additions:
        case = dict(name=name, points=points, ids=[0, 17, 41, 63, 107][:len(points)],
                    reversed_ids=False, origin="explicit_fixture_coordinates_independently_recognized")
        case_index = len(cases)
        cases.append(case)
        events, oracle = reference.catalogue(case)
        catalogues.append(dict(case_index=case_index, events=events,
                               global_regular_labels_checked=len(oracle.levels)))
        for k in range(1, len(points) + 1):
            minimum = [e for e in events if len(e["label"]) == k] if k > 1 else []
            direct = [e for e in events if len(e["label"]) == k + 1]
            expected = reference.reference_forest(oracle, k)
            for variant in (0, 1):
                def encode(source: list[dict]) -> list[dict]:
                    values = [dict(q=e["q"], d=e["d"], mask=e["mask"], support=e["support"],
                                   interior=e["interior"], **reference.level(Q(e["squared_level"]),
                                                                         1 if not variant else 11 + i))
                              for i, e in enumerate(source)]
                    return values if not variant else values[::-1]
                inputs = [dict(id=i, xyz=p) for i, p in zip(case["ids"], points)]
                base.append(dict(id=len(base), case_index=case_index, order=k, representation=variant,
                                 points=inputs if not variant else inputs[::-1], minima_source=encode(minimum),
                                 direct_source=encode(direct), cuts=expected["cuts"], budget_probe=int(k == 2 and not variant),
                                 expected=expected, expected_stats=reference.independent_stats(case, k, minimum, direct, oracle),
                                 named_meb_support_sum=None))
    require(len(base) == 218, "base_order_representations")
    records = []
    for source in base:
        for lazy, capacity in MODES:
            records.append(dict(source, id=len(records), base_id=source["id"], lazy=lazy, capacity=capacity))
    # Permanent negative from the constructor's discarded shared-lot proposal.
    bad_points = [[0, 5, 0], [4, 5, 0], [2, 6, 0], [2, 0, 0], [2, 1, 3]]
    ball = reference.expected_meb([tuple(bad_points[i]) for i in (2, 4)])
    powers = [reference.power(ball, tuple(p)) for p in bad_points]
    require(ball["radius"] == Q(17, 2) and powers[0] == powers[1] == powers[2] == powers[4] == 0,
            "negative_diameter_CV_shell")
    negative = dict(points=bad_points, essential_indices=[2, 4], level="17/2",
                    global_powers=list(map(str, powers)), rejected_reason="unsupported_global_shell",
                    scope="Independent rational witness; not submitted as a valid regular product input")
    sources = {str((OLD / n).relative_to(ROOT)): v for n, v in SEALED.items()}
    for path in [Path(__file__), AUDIT / "full_producer_audit.py", AUDIT / "full_cpp_audit.py",
                 AUDIT / "gabriel_full_probe.py", AUDIT / "horizontal_rational_oracle.py",
                 AUDIT / "meb_rational_oracle_20260905.py"]:
        sources[str(path.relative_to(ROOT))] = sha(path)
    return dict(schema="mhgp7-independent-lazy-fixtures-v1", cases=cases, catalogues=catalogues,
                records=records, unique_orders=109, representations=218, policies_and_capacities=MODES,
                source_pins=sources, negative_global_shell=negative,
                scope="100 sealed independent orders explicitly reused as fixture data, plus 9 new orders; "
                      "each representation is executed anew on captured source in four modes, not inherited as a result.")


def fixture_text(fixture: dict) -> str:
    lines = [f"FULLLAZY1 {len(fixture['records'])}"]
    for r in fixture["records"]:
        lines.append(" ".join(map(str, (r["id"], len(r["points"]), r["order"], len(r["minima_source"]),
                                       len(r["direct_source"]), len(r["cuts"]), r["budget_probe"], r["lazy"], r["capacity"]))))
        lines.extend(" ".join(map(str, (p["id"], *p["xyz"]))) for p in r["points"])
        for e in r["minima_source"] + r["direct_source"]:
            lines.append(" ".join(map(str, (e["q"], e["d"], e["mask"], *e["num"], e["den"], *e["support"], *e["interior"]))))
        lines.extend(" ".join(map(str, (*c["num"], c["den"], c["closed"]))) for c in r["cuts"])
    return "\n".join(lines) + "\n"


def check_stats(r: dict, s: dict) -> None:
    require(set(s) == reference.STAT_FIELDS | EXTRA_STATS, "lazy.stats_schema")
    require(set(s["geometry"]) == reference.GEOMETRY_FIELDS, "lazy.geometry_schema")
    require(all(type(v) is int and 0 <= v < 1 << 64 for k, v in s.items() if k != "geometry")
            and all(type(v) is int and 0 <= v < 1 << 64 for v in s["geometry"].values()), "lazy.stats_domain")
    if not r["lazy"]:
        reference.check_stats({k: s[k] for k in reference.STAT_FIELDS}, r["expected_stats"])
        require(all(s[k] == 0 for k in EXTRA_STATS - {"direct_lookups"}), "eager.cache_counters_zero")
        require(s["direct_lookups"] == s["chain_steps"], "eager.direct_lookups")
        return
    require(s["aliases"] == s["alias_hits"] == 0, "lazy.no_eager_aliases")
    require(s["input_records"] == r["expected_stats"]["input_records"]
            and s["no_op_connections"] == r["expected_stats"]["no_op_connections"], "lazy.exact_catalogue_counts")
    strict = sum(e["q"] for e in r["direct_source"])
    require(s["minimum_lookups"] == s["face_visits"] == strict, "lazy.strict_visits")
    require(s["minimum_hits"] + s["cache_lookups"] == strict, "lazy.minimum_partition")
    require(s["cache_hits"] + s["portal_requests"] == s["cache_lookups"], "lazy.cache_partition")
    require(s["cache_inserts"] == min(r["capacity"], s["portal_requests"]), "lazy.first_c_admission")
    require(s["cache_inserts"] + s["cache_skips"] == s["portal_requests"], "lazy.resident_and_skipped")
    require(s["direct_lookups"] == s["singleton_intruder_resolutions"] + s["chain_steps"], "lazy.direct_lookups")
    require(s["meb_calls"] == s["geometry"]["meb_calls"] == s["portal_requests"] + s["chain_steps"], "lazy.meb_calls")
    require(s["terminal_direct"] == s["portal_requests"], "lazy.terminals")
    j2 = s["portal_requests"] - s["singleton_intruder_resolutions"]
    require(0 <= j2 <= s["chain_steps"], "lazy.j2_steps")
    require((j2 == 0 and s["max_chain_length"] == 0) or
            (1 <= s["max_chain_length"] <= s["chain_steps"] <= j2 * s["max_chain_length"]), "lazy.chain_length")
    require(s["geometry"]["meb_supports"] >= s["meb_calls"], "lazy.support_floor")
    require(all(s["geometry"][k] == 0 for k in reference.GEOMETRY_FIELDS -
                {"query_nodes", "query_leaves", "query_range_skips", "meb_calls", "meb_supports"}), "lazy.no_old_core")


def signature(row: dict) -> dict:
    return dict(levels=[str(reference.rational(n)) for n in row["nodes"]],
                structure=[(n["first"], n["parent_count"]) for n in row["nodes"]],
                minima=row["minima"], parents=row["parents"], coverage=row["coverage"],
                roots=[c["roots"] for c in row["cuts"]])


def check_output(path: Path, fixture: dict) -> dict:
    rows = json.loads(path.read_text())["records"]
    require(len(rows) == len(fixture["records"]) == 872, "lazy.record_population")
    totals = Counter()
    named = []
    grouped = {}
    old_rows = json.loads((OLD / "O2_output.json").read_text())["records"]
    for r, actual in zip(fixture["records"], rows):
        name = fixture["cases"][r["case_index"]]["name"]
        require(actual["id"] == r["id"] and actual["order"] == r["order"], "lazy.identity")
        require(actual["status"] == 0 and actual["reason"] == reference.AUTHORITY, "lazy.status")
        require(actual["alias_policy"] == POLICIES[r["lazy"]] and actual["cache_capacity"] == r["capacity"], "lazy.policy")
        expected = r["expected"]
        require(len(actual["nodes"]) == len(expected["nodes"]), "lazy.node_count")
        for node, wanted in zip(actual["nodes"], expected["nodes"]):
            require(reference.rational(node) == reference.rational(wanted), "lazy.node_level")
            require((node["first"], node["parent_count"]) == (wanted["first"], wanted["parent_count"]), "lazy.node_structure")
        for key in ("minima", "parents", "coverage"):
            require(actual[key] == expected[key], "lazy." + key)
        require(len(actual["cuts"]) == len(expected["roots"]), "lazy.cut_population")
        for cut, wanted in zip(actual["cuts"], expected["roots"]):
            require(cut["status"] == 0 and cut["reason"] == "structural_only" and cut["roots"] == wanted, "lazy.Gamma_cut")
        s = actual["stats"]
        check_stats(r, s)
        if not r["lazy"] and r["base_id"] < 200:
            old = old_rows[r["base_id"]]
            require(signature(actual) == signature(old) and
                    {k: s[k] for k in reference.STAT_FIELDS} == old["stats"], "eager.historical_regression")
            totals["historical_eager_representations_equal"] += 1
        trials = actual["budget_trials"]
        if r["budget_probe"]:
            caps = reference.budget_values(r, actual)
            wanted = {("all", "exact")} | {(d, "minus_one") for d, cap in caps.items() if cap}
            if r["lazy"]:
                wanted.add(("aliases", "conflict"))
            require(len(trials) == len(wanted) and {(t["dimension"], t["kind"]) for t in trials} == wanted, "lazy.budget_population")
            for trial in trials:
                if trial["kind"] == "exact":
                    require(trial["status"] == 0 and trial["reason"] == reference.AUTHORITY and trial["same"]
                            and not trial["empty"] and trial["stats"] == s, "lazy.exact_budget")
                    totals["exact_budget_successes"] += 1
                elif trial["kind"] == "conflict":
                    require(trial["status"] == 1 and trial["reason"] == "full_gabriel_lazy_alias_budget_conflict"
                            and trial["empty"] and trial["cap"] == 1, "lazy.alias_api_conflict")
                    totals["api_conflict_refusals"] += 1
                else:
                    dim = trial["dimension"]
                    require(trial["status"] == 3 and trial["reason"] == reference.DIMENSIONS[dim]
                            and trial["empty"] and trial["cap"] == caps[dim] - 1, "lazy.cap_minus_one." + dim)
                    totals["budget_refusals"] += 1
        else:
            require(not trials, "lazy.unrequested_budget")
        if r["lazy"] and r["order"] == 2 and name in {"lazy_J1", "lazy_shared_lot", "E5_chain_two"}:
            if name == "lazy_J1":
                require(s["singleton_intruder_resolutions"] == s["portal_requests"] == s["meb_calls"]
                        == s["geometry"]["meb_supports"] == 1 and s["chain_steps"] == 0, "lazy.named_j1")
            elif name == "lazy_shared_lot":
                if r["capacity"] == 0:
                    require(s["singleton_intruder_resolutions"] >= 2, "lazy.named_repeat_j1")
                elif r["capacity"] == 100000:
                    require(s["cache_hits"] > 0, "lazy.named_same_lot_hit")
            else:
                require(s["max_chain_length"] >= 2, "lazy.named_second_step")
            named.append(dict(case=name, representation=r["representation"], capacity=r["capacity"], stats=s))
        grouped[(r["base_id"], r["lazy"], r["capacity"])] = actual
        for key in EXTRA_STATS | {"portal_requests", "chain_steps", "meb_calls"}:
            totals[key] += s[key]
        totals["records"] += 1
        totals["cuts"] += len(actual["cuts"])
        totals["nodes"] += len(actual["nodes"])
        totals["minima"] += len(actual["minima"])
        totals["parent_refs"] += len(actual["parents"])
    for base_id in range(218):
        eager = grouped[(base_id, 0, 0)]
        for lazy, cap in MODES:
            row = grouped[(base_id, lazy, cap)]
            require(signature(row) == signature(eager), "lazy.paired_forest")
            if lazy:
                totals["cross_policy_forest_comparisons"] += 1
            if base_id % 2:
                previous = grouped[(base_id - 1, lazy, cap)]
                require(signature(row) == signature(previous) and row["stats"] == previous["stats"], "lazy.representation_invariance")
        large = grouped[(base_id, 1, 100000)]["stats"]
        old = eager["stats"]
        require(large["cache_skips"] == 0 and large["portal_requests"] - large["singleton_intruder_resolutions"] == old["portal_requests"]
                and large["chain_steps"] == old["chain_steps"]
                and large["meb_calls"] == old["meb_calls"] + large["singleton_intruder_resolutions"], "lazy.no_skip_work_identity")
        totals["no_skip_work_comparisons"] += 1
    require(totals["cache_hits"] > 0 and totals["cache_skips"] > 0 and len(named) == 18, "lazy.named_nonvacuity")
    return dict(status="completed_independent_lazy_full_judgment", counts=dict(totals), named=named,
                python_optimized=bool(sys.flags.optimize), output_sha256=sha(path), fixtures_sha256=sha(OUT / "fixtures.json"),
                checker_sha256=sha(Path(__file__)), public_status="not_claimed",
                scope="109 unique orders, 218 representations, EAGER plus lazy C0/C1/C100000. "
                      "Exact independent Gamma fixture judgment, no industrial catalogue, digest-probe, timing, vertical or mass claim.")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prepare", action="store_true")
    args = parser.parse_args()
    require(args.prepare, "use --prepare or full_lazy_run.py judge")
    OUT.mkdir(exist_ok=True)
    require(not (OUT / "fixtures.json").exists(), "fixtures already sealed")
    fixture = make_fixtures()
    save(OUT / "fixtures.json", compact_fixture(fixture))
    (OUT / "fixtures.txt").write_text(fixture_text(fixture))
    verify_fixtures(load_fixtures())
    print(json.dumps({"unique_orders": 109, "representations": 218, "executions_per_build": len(fixture["records"])}))


if __name__ == "__main__":
    main()
