#!/usr/bin/env python3
"""Four-mode collective pool comparison against tranche25 Jung/Universal.

The unchanged collector supplies process handling, raw logs, provenance and
source/artifact closure. Only this process's adapters change; historical
readers, receipts and source files are not rewritten. No build or GCP here.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
import json
import math
import os
from pathlib import Path
import xml.etree.ElementTree as ET

import run_q34_pruning_checks as reference
previous = reference.previous
from run_p0_matrix import InvalidReceipt, parse_result, require, uint
from run_q4_family_checks import counts, read_json, validate_closure

collector = previous.collector
ROOT = previous.ROOT
SOURCES = reference.SOURCES | frozenset({
    "morsehgp3D_v8/src/lanes/q34_collective.hpp", "morsehgp3D_v8/src/lanes/q34_collective.cpp",
    "morsehgp3D_v8/tests/q34_collective_gate.cpp",
    "morsehgp3D_v8/bench/q34_collective_probe.cpp", "morsehgp3D_v8/bench/run_q34_collective_checks.py",
})
SCHEMA = "mhgp8_q34_collective_attempt_v1"
GATES = (*reference.GATES, "mhgp8_q34_collective_gate")
REGIMES = previous.REGIMES
BUDGETS = (32, 64)
MODES = ("jung_universal", "jung_collective", "variance_universal", "variance_collective")
FILTER_FIELDS = ["seed_owner_tests","seed_owner_rejections","seed_queries","certificate_builds","sqrt_iterations","variance_bounds","variance_sqrt_iterations","proposed_sites","paired_predicate_tests","q3_credits","q4_universal_credits","q3_rejected","q4_universal_rejected","collective_queries","endpoint_tests","constant_tests","event_count","sort_comparisons","group_comparisons","event_side_tests","groups","max_group","collective_minimum_sum","collective_q4_rejected","q4_rejected","both_rejected","q3_only_survivors","q4_only_survivors","both_survivors","peak_event_bytes"]

PRUNING_FIELDS = ("seed_queries", "certificate_builds", "sqrt_iterations", "proposed_sites", "paired_predicate_tests",
    "q3_credits", "q4_credits", "q3_rejected", "q4_rejected", "both_rejected", "q3_only_survivors", "q4_only_survivors", "both_survivors")
POOL_FIELDS = ("range_visits", "selected_sites")
MEMORY_FIELDS = ("id_bytes", "input_capacity_bytes", "cloud_retained_bytes", "index_retained_bytes", "cover_retained_bytes",
                 "pool_retained_bytes", "baseline_output_retained_bytes", "output_retained_bytes")
PART_TIMES = ("generation_ms", "fixture_validation_ms", "cloud_ms", "index_ms", "cover_ms", "baseline_run_callback_ms",
              "baseline_validation_ms", "pool_prepare_ms", "run_callback_ms", "validation_ms", "release_ms")
FIXED = dict(schema="mhgp8_q34_collective_probe_v1", status="completed", scope="one_edge_collective_pool_not_global_q34_producer",
             public_status="not_claimed", backend="cpu_reference", profile="quantized_u16_input_only", threads=1, seed=3,
             recipe="owned_edge_far_cap_dense_v1", edge_ids=[0, 1],
             timing_scope="paired_wall_including_validation_release_pipeline_sums_share_measured_preparation")
GATE_FLOORS = dict(checks=1, assessments=1, collective_queries=1, universal_shortcuts=1,
    collective_gains=1, tighter_bounds=1, variance_roundings=1, wide_variance_numerators=1,
    lower_tangencies=1, upper_tangencies=1, equal_mixed_roots=1, endpoint_dips=1,
    initial_saturation_drops=1, exact_minimum_checks=1, workspace_reuses=1, option_calls=1,
    zero_budget_calls=1, jung_universal_pairs=1, foreign_owner_calls=4, edge_calls=1,
    seed_calls=1, reference_calls=1, oracle_completions=1, oracle_sites=1, candidates=1,
    q3=1, q4=1, max_shell=30, pool_calls=1, exhaustive_edges=1, permutations=1,
    invalid_inputs=1, callback_failures=1, parallel_calls=4,
    q3_only_rejections=1, q4_only_rejections=1, both_rejections=1, neither_rejections=1)



def validate_pruned_seed(work, seeds, population, pruning, digest, id_bytes, enabled):
    require(type(work) is dict and set(work) == set(previous.SEED_FIELDS) | {"family"}, "pruned seed fields differ")
    for key in previous.SEED_FIELDS: uint(work[key], "seed." + key)
    f = work["family"]
    counts(f, previous.FAMILY_FIELDS, "family")
    active3 = seeds - pruning["q3_rejected"]
    active4 = seeds - pruning["q4_rejected"]
    both = pruning["both_survivors"] if enabled else seeds
    require(work["seed_owner_tests"] == 2 * seeds and work["seed_owner_rejections"] == 0 and
            work["q3_depth_rejections"] + work["q3_emitted"] == active3 and
            work["q3_emitted"] == digest["q3"] and work["q4_emitted"] == digest["q4"], "pruned q3 lane/output ledger differs")
    require(population * both <= work["q3_point_tests"] <= active3 * population and
            f["sites"] == active4 * population and f["sites"] == f["entries"] + f["exits"] + f["constant_inside"] +
            f["constant_on"] + f["constant_outside"] and f["event_count"] == f["entries"] + f["exits"],
            "pruned scan/site partition differs")
    require(f["constant_on"] >= 3 * active4 and f["event_count"] >= active4 and
            f["groups"] == f["callbacks"] and active4 <= f["groups"] <= f["event_count"] and
            f["group_comparisons"] == f["event_count"] - active4 and
            (not active4 or (f["max_group"] > 0 and f["sort_comparisons"] > 0)) and
            f["max_group"] * f["groups"] >= f["event_count"], "pruned family grouping differs")
    require(work["q4_presentations"] + work["q4_depth_skipped_ids"] + work["q4_unexamined_after_emit"] == f["event_count"] and
            work["q4_depth_rejected_groups"] + work["q4_groups_without_support"] + work["q4_emitted"] == f["groups"],
            "pruned roots/IDs partition differs")
    require(work["q4_presentations"] == work["q4_owner_rejections"] + work["q4_positive_tests"] and
            work["q4_positive_tests"] == work["q4_positive_rejections"] + work["q4_seed_tests"] and
            work["q4_seed_tests"] == work["q4_seed_rejections"] + work["q4_emitted"] and
            work["q4_presentations"] <= work["q4_owner_tests"] <= 5 * work["q4_presentations"],
            "pruned presentation cascade differs")
    require((not active4 or f["retained_capacity_bytes"] >= 2 * population * id_bytes) and
            (not digest["q3"] or work["q3_shell_capacity_bytes"] >= 3 * id_bytes), "pruned capacities missing")


def validate_filter(w, seeds, selected, k, mode, id_bytes):
    counts(w, FILTER_FIELDS, "filter")
    if selected == 0:
        # The edge wrapper bypasses assessment for an empty pool; the normal
        # fallback still validates ownership in its existing work ledger.
        require(all(w[key] == 0 for key in FILTER_FIELDS), "empty pool performed filtering")
        return
    require(w["seed_owner_tests"] == 2*seeds and w["seed_owner_rejections"] == 0, "filter ownership work differs")
    require(w["seed_queries"] == w["certificate_builds"] == seeds and seeds <= w["sqrt_iterations"] <= 128*seeds and
            seeds <= w["proposed_sites"] == w["paired_predicate_tests"] <= seeds*selected, "filter preparation/proposal work differs")
    if mode.startswith("variance"):
        require(w["variance_bounds"] == seeds and seeds <= w["variance_sqrt_iterations"] <= 128*seeds, "variance preparation missing")
    else:
        require(w["variance_bounds"] == w["variance_sqrt_iterations"] == 0, "Jung performed variance preparation")
    require(w["q3_rejected"] == w["both_rejected"] + w["q4_only_survivors"] and
            w["q4_rejected"] == w["both_rejected"] + w["q3_only_survivors"] and
            w["q4_rejected"] == w["q4_universal_rejected"] + w["collective_q4_rejected"] and
            sum(w[key] for key in ("both_rejected", "q3_only_survivors", "q4_only_survivors", "both_survivors")) == seeds,
            "filter independent lane partition differs")
    for credits, rejected, threshold in (("q3_credits", "q3_rejected", k-1), ("q4_universal_credits", "q4_universal_rejected", k-2)):
        killed = w[rejected]
        require(threshold*killed <= w[credits] <= threshold*killed + (threshold-1)*(seeds-killed), "filter saturated universal credits differ")
    collective_fields = ("collective_queries", "endpoint_tests", "constant_tests", "event_count", "sort_comparisons",
                         "group_comparisons", "event_side_tests", "groups", "max_group", "collective_minimum_sum",
                         "collective_q4_rejected", "peak_event_bytes")
    if mode.endswith("universal"):
        require(all(w[key] == 0 for key in collective_fields), "universal mode performed collective work")
        return
    require(w["collective_queries"] == seeds-w["q4_universal_rejected"] and
            w["endpoint_tests"] % 2 == 0 and w["endpoint_tests"]//2 + w["constant_tests"] <= w["proposed_sites"] and
            w["event_count"] <= w["endpoint_tests"]//2, "collective preparation partition differs")
    # event_count includes prepared prefixes later abandoned by universals.
    # Only event_side_tests count IDs actually swept; do not subtract the
    # number of all queries from the total prepared-event population.
    require(w["groups"] <= w["event_side_tests"] <= w["event_count"] and
            0 <= w["event_side_tests"]-w["group_comparisons"] <= w["collective_queries"] and
            w["max_group"]*w["groups"] >= w["event_side_tests"] and
            w["peak_event_bytes"] >= w["max_group"]*id_bytes, "collective sweep accounting differs")
    if w["event_side_tests"] == 0:
        require(w["groups"] == w["max_group"] == w["group_comparisons"] == 0, "empty collective sweep counted groups")
    else:
        require(w["max_group"] > 0 and w["event_side_tests"] > w["group_comparisons"], "nonempty collective sweep missed groups")
    require((k-2)*w["collective_q4_rejected"] <= w["collective_minimum_sum"] <=
            selected*w["collective_q4_rejected"] + (k-3)*(w["collective_queries"]-w["collective_q4_rejected"]),
            "exact collective minimum/rejection ledger differs")


def validate_row(row, command):
    require(type(row) is dict and len(command) == 6, "pruning probe command arity")
    n, regime, k, budget, mode = int(command[1]), command[2], int(command[3]), int(command[4]), command[5]
    require(n >= 8 and regime in REGIMES and k in (5, 10) and budget in (0, *BUDGETS) and mode in MODES and
            (regime != "cap" or n <= 35307) and (regime != "adversarial" or n <= 514), "fixture domain mismatch")
    extra = {"n", "regime", "kmax", "budget", "input_hash", "expected_seeds", "cover_sites", "generation", "validation",
             "baseline_work", "edge_work", "baseline_pruning_work", "filter_work", "mode", "peak_live_buffer_bytes", "pool_work", "pool_hash", "cover_work", "index_work", "cloud_work",
             "baseline_digest", "digest", "memory", "timings"}
    require(set(row) == set(FIXED) | extra and all(type(row[key]) is type(value) and row[key] == value for key, value in FIXED.items()) and
            all(type(value) is int for value in row["edge_ids"]), "probe schema/scope fields differ")
    require(all(type(row[key]) is int and row[key] == value for key, value in dict(n=n, kmax=k, budget=budget).items()) and
            row["regime"] == regime and row["mode"] == mode, "command/result differs")
    uint(row["input_hash"], "input_hash"); uint(row["pool_hash"], "pool_hash")
    seeds, covered = n - 2 if regime == "adversarial" else 2, 6 if regime == "far" else n
    selected = min(budget, covered)
    require(uint(row["expected_seeds"], "expected_seeds") == seeds and uint(row["cover_sites"], "cover_sites") == covered,
            "fixture seed/cover counts differ")
    counts(row["generation"], ("proposals", "duplicates", "random_calls"), "generation")
    g = row["generation"]
    require(g["proposals"] == n - (2 if regime == "adversarial" else 6) + g["duplicates"] and
            g["random_calls"] == (3 * g["proposals"] if regime == "far" else 0) and
            (regime == "far" or g["duplicates"] == 0), "generation ledger differs")
    v = row["validation"]
    require(type(v) is dict and set(v) == {"fixture_point_tests", "independent_ball_point_tests", "pool_ids_checked", "method"} and
            uint(v["fixture_point_tests"], "fixture_point_tests") == n and
            uint(v["independent_ball_point_tests"], "independent_ball_point_tests") == (0 if regime == "adversarial" else 3*n) and
            uint(v["pool_ids_checked"], "pool_ids_checked") == selected and
            v["method"] == ("qualified_pruned_path_differential" if regime == "adversarial" else "three_closed_form_balls_plus_differential"),
            "linear validation work/scope differs")
    for key in ("baseline_digest", "digest"):
        counts(row[key], previous.DIGEST_FIELDS, key)
        d = row[key]
        require(d["callbacks"] == d["q3"] + d["q4"] and d["support_ids_visited"] == 3*d["q3"] + 4*d["q4"] and
                d["shell_ids_visited"] >= d["support_ids_visited"] and min(d["q3"], d["q4"]) > 0, "nonvacuous output ledger failed")
    require(row["baseline_digest"] == row["digest"] and (regime == "adversarial" or row["digest"] == previous.expected_digest()),
            "baseline/pruned/independent digest differs")
    m = row["memory"]
    require(type(m) is dict and set(m) == set(MEMORY_FIELDS) | {"scope"} and
            m["scope"] == "retained_capacities_not_RSS_shared_preparation_and_both_outputs", "memory scope/fields differ")
    for key in MEMORY_FIELDS: uint(m[key], "memory." + key)
    require(m["id_bytes"] in (4, 8) and min(m["input_capacity_bytes"], m["cloud_retained_bytes"]) >= 6*n and
            m["index_retained_bytes"] >= n*m["id_bytes"] and m["pool_retained_bytes"] >= selected*m["id_bytes"] and
            min(m["baseline_output_retained_bytes"], m["output_retained_bytes"]) >= row["digest"]["shell_ids_visited"]*m["id_bytes"],
            "retained capacity missing")
    counts(row["cloud_work"], previous.CLOUD_FIELDS, "cloud")
    cloud = row["cloud_work"]
    require(cloud["coordinate_copies"] == cloud["validation_points"] == cloud["range_tree_leaf_visits"] == n and
            cloud["uniqueness_adjacent_tests"] == cloud["range_tree_merges"] == n-1 and cloud["range_tree_nodes"] == 2*n-1,
            "shared cloud preparation missing")
    counts(row["index_work"], previous.INDEX_FIELDS, "index")
    ix = row["index_work"]
    require(ix["nodes"] == ix["escape_links"] == 2*n-1 and ix["point_visits"] >= n and ix["max_depth"] > 0,
            "shared index preparation missing")
    counts(row["cover_work"], previous.COVER_FIELDS, "cover")
    c = row["cover_work"]
    require(c["node_visits"] == c["bound_tests"] + c["point_tests"] and c["node_visits"] <= 2*n-1 and
            c["node_visits"] == c["admitted_nodes"] + c["rejected_nodes"] + c["split_nodes"] and
            c["admitted_sites"] == covered and c["admitted_sites"] + c["rejected_sites"] == n and
            c["retained_ranges"] + c["merged_ranges"] == c["admitted_nodes"] and c["retained_ranges"] > 0 and
            m["cover_retained_bytes"] >= 2*m["id_bytes"]*c["retained_ranges"], "cover partition differs")
    counts(row["pool_work"], POOL_FIELDS, "pool")
    pool = row["pool_work"]
    require(pool["selected_sites"] == selected and
            ((selected == 0 and pool["range_visits"] == 0 and row["pool_hash"] == 14695981039346656037 and m["pool_retained_bytes"] == 0) or
             (selected > 0 and 1 <= pool["range_visits"] <= c["retained_ranges"])), "pool selection/range work differs")
    p = row["baseline_pruning_work"]
    counts(p, PRUNING_FIELDS, "pruning")
    if budget == 0:
        require(all(value == 0 for value in p.values()) and row["baseline_work"] == row["edge_work"], "disabled pruning changed reference work")
    else:
        require(p["seed_queries"] == p["certificate_builds"] == seeds and seeds <= p["sqrt_iterations"] <= 128*seeds and
                seeds <= p["proposed_sites"] == p["paired_predicate_tests"] <= seeds*selected,
                "pruning preparation/predicate work differs")
        require(p["q3_rejected"] == p["both_rejected"] + p["q4_only_survivors"] and
                p["q4_rejected"] == p["both_rejected"] + p["q3_only_survivors"] and
                sum(p[key] for key in ("both_rejected", "q3_only_survivors", "q4_only_survivors", "both_survivors")) == seeds,
                "independent lane partition differs")
        for lane, threshold in (("q3", k-1), ("q4", k-2)):
            rejected = p[lane + "_rejected"]
            require(threshold*rejected <= p[lane + "_credits"] <= threshold*rejected + (threshold-1)*(seeds-rejected),
                    "distinct saturated lane credits differ")
    q = row["filter_work"]
    validate_filter(q, seeds, selected, k, mode, m["id_bytes"])
    require(q["q3_rejected"] == p["q3_rejected"] and q["q4_rejected"] >= p["q4_rejected"], "stronger filter lost a baseline rejection")
    for section in ("baseline_work", "edge_work"):
        e = row[section]
        require(type(e) is dict and set(e) == set(previous.EDGE_FIELDS) | {"covered"}, "edge work fields differ")
        for key in previous.EDGE_FIELDS: uint(e[key], section + "." + key)
        require(e["node_visits"] == e["bound_tests"] + e["point_tests"] and e["node_visits"] <= 2*n-1 and
                e["bound_tests"] == e["rejected_nodes"] + e["split_nodes"] and e["rejected_sites"] + e["point_tests"] == n and
                e["seeds"] == seeds and e["acute_seeds"] == seeds + e["owner_rejections"] and
                e["acute_seeds"] <= e["point_tests"] and e["acute_seeds"] <= e["owner_tests"] <= 2*e["acute_seeds"],
                "edge seed/traversal partition differs")
        w = e["covered"]
        require(type(w) is dict and set(w) == set(previous.COVERED_FIELDS) | {"seed"}, "covered fields differ")
        for key in previous.COVERED_FIELDS: uint(w[key], "covered." + key)
        lane = p if section == "baseline_work" else q
        validate_pruned_seed(w["seed"], seeds, covered, lane, row["digest"], m["id_bytes"], budget > 0)
        both = lane["both_survivors"] if budget else seeds
        require(w["site_reads"] == w["seed"]["family"]["sites"] + w["seed"]["q3_point_tests"] - covered*both and
                w["site_reads"] <= covered*(seeds-lane["both_rejected"]), "remaining lane scan union differs")
        capacities = (w["seed"]["q3_shell_capacity_bytes"], w["seed"]["family"]["retained_capacity_bytes"])
        require(max(capacities) <= w["peak_buffer_bytes"] <= sum(capacities), "simultaneous peak capacity differs")
    require(all(row["baseline_work"][key] == row["edge_work"][key] for key in previous.EDGE_FIELDS), "pruning changed seed enumeration")
    peak = uint(row["peak_live_buffer_bytes"], "peak_live_buffer_bytes")
    capacities = (q["peak_event_bytes"], row["edge_work"]["covered"]["peak_buffer_bytes"])
    require(max(capacities) <= peak <= sum(capacities), "coupled workspace/fallback peak differs")
    if budget == 0 or mode == "jung_universal":
        require(row["baseline_work"] == row["edge_work"], "Jung/Universal or empty pool changed fallback")
    t = row["timings"]
    require(type(t) is dict and set(t) == set(PART_TIMES) | {"baseline_prepare_run_sum_ms", "collective_prepare_run_sum_ms", "paired_total_ms"} and
            all(type(value) in (int, float) and math.isfinite(value) and value >= 0 for value in t.values()), "invalid timing")
    common = ("generation_ms", "cloud_ms", "index_ms", "cover_ms", "pool_prepare_ms")
    for total, parts in (("paired_total_ms", PART_TIMES), ("baseline_prepare_run_sum_ms", (*common, "baseline_run_callback_ms")),
                         ("collective_prepare_run_sum_ms", (*common, "run_callback_ms"))):
        require(math.isclose(t[total], sum(t[key] for key in parts), rel_tol=1e-12, abs_tol=1e-6), "timing sum scope/partition differs")


def validate_gate(row, executable):
    if executable in reference.GATES:
        reference.validate_gate(row, executable)
        return
    require(executable == GATES[4] and type(row) is dict and set(row) == set(GATE_FLOORS) | {"schema", "status"} and
            row["schema"] == executable + "_v1" and row["status"] == "passed", "collective gate schema/fields differ")
    for key, floor in GATE_FLOORS.items(): require(uint(row[key], "gate." + key) >= floor, "collective gate floor failed")
    require(row["candidates"] == row["q3"] + row["q4"] and row["foreign_owner_calls"] == 4 and
            row["parallel_calls"] == 4 and row["callback_failures"] == 1, "collective gate ledger/lifecycle differs")


def validate_xml(path):
    tree = ET.parse(path).getroot()
    require(tree.tag == "testsuite" and tree.get("tests") == "87" and
            all(tree.get(key) == "0" for key in ("failures", "disabled", "skipped")), "unsuccessful 87-test regression")
    cases = tree.findall("testcase")
    names = [case.get("name") for case in cases]
    require(len(cases) == len(set(names)) == 87 and all(type(name) is str and name.startswith("mhgp8_") for name in names) and
            set(GATES) | {"mhgp8_q4_family_gate"} <= set(names), "regression inventory differs")
    require(all(case.get("status") == "run" and all(case.find(tag) is None for tag in ("failure", "error", "skipped")) for case in cases),
            "regression failed/skipped testcase")
    return sorted(names)


def plan(build, campaign, output, ctest=None):
    if campaign == "regression":
        return [("ctest", [ctest, "--test-dir", str(build), "--output-on-failure", "--parallel", "2", "--output-junit", str(output / "result.xml")])]
    result = [("gate", [str(build / name), "--selftest"]) for name in GATES]
    for regime in REGIMES:
        sizes = () if campaign == "gate" else (32,) if campaign == "smoke" else ((32, 64, 128, 256) if regime == "adversarial" else (8000, 16000, 32000))
        for k in (5, 10):
            for budget in ((32,) if campaign == "smoke" else BUDGETS):
                for mode in MODES:
                    for n in sizes:
                        result.append(("measure", [str(build / "mhgp8_q34_collective_probe"), str(n), regime, str(k), str(budget), mode]))
    return result


def executable_names(build, campaign):
    if campaign == "regression":
        names = sorted(p.name for p in build.glob("mhgp8_*") if p.is_file() and os.access(p, os.X_OK))
        require(names and all((build / name).resolve() == build / name for name in names), "missing or linked executable")
        return names
    return sorted([*GATES, *(["mhgp8_q34_collective_probe"] if campaign != "gate" else [])])


def install_adapters():
    collector.SOURCES, collector.SCHEMA, collector.GATES = SOURCES, SCHEMA, GATES
    collector.FAMILIES = ()
    collector.plan, collector.executable_names = plan, executable_names
    collector.validate_row, collector.validate_gate, collector.validate_xml = validate_row, validate_gate, validate_xml


def read(path, check_live=False):
    install_adapters()
    summary = previous.BASE_READ(path, check_live)
    rows = [record["row"] for file in sorted(path.glob("record_*.json")) if (record := read_json(file))["kind"] == "measure"]
    budgets = (32,) if summary["campaign"] == "smoke" else BUDGETS
    for regime in REGIMES:
        for n in sorted({r["n"] for r in rows if r["regime"] == regime}):
            group = [r for r in rows if r["n"] == n and r["regime"] == regime]
            require(len(group) == 2*len(budgets)*len(MODES) and
                    {(r["kmax"],r["budget"],r["mode"]) for r in group} == {(k,b,mode) for k in (5,10) for b in budgets for mode in MODES},
                    "missing K/budget/mode comparison")
            require(all(all(r[key] == group[0][key] for key in ("input_hash", "generation", "cloud_work", "index_work", "cover_work")) for r in group),
                    "same-size immutable preparation differs")
            for budget in budgets:
                by_budget = [r for r in group if r["budget"] == budget]
                require(all(r["pool_work"] == by_budget[0]["pool_work"] and r["pool_hash"] == by_budget[0]["pool_hash"] for r in by_budget),
                        "mode/K changed pool preparation")
                for k in (5,10):
                    modes = [r for r in by_budget if r["kmax"] == k]
                    require(all(all(r[key] == modes[0][key] for key in ("baseline_work", "baseline_pruning_work", "digest", "baseline_digest")) for r in modes),
                            "mode changed historical baseline or outputs")
            for k in (5,10):
                by_k = [r for r in group if r["kmax"] == k]
                require(all(r["digest"] == by_k[0]["digest"] for r in by_k), "budget changed complete output")
    growth = {}
    if summary["campaign"] == "scale":
        def flatten(value, prefix=""):
            result = {}
            for key,item in value.items():
                if type(item) is dict: result.update(flatten(item,prefix+key+"."))
                elif type(item) in (int,float) and key not in ("hash","id_bytes"): result[prefix+key]=item
            return result
        for regime in REGIMES:
            for k in (5,10):
                for budget in BUDGETS:
                    for mode in MODES:
                        series = sorted((r for r in rows if (r["regime"],r["kmax"],r["budget"],r["mode"]) == (regime,k,budget,mode)),key=lambda r:r["n"])
                        measured = [flatten({key:r[key] for key in ("baseline_work","baseline_pruning_work","edge_work","filter_work","peak_live_buffer_bytes",
                            "pool_work","cover_work","index_work","cloud_work","digest","memory","timings")}) for r in series]
                        key = f"{regime}_K{k}_C{budget}_{mode}"
                        growth[key] = {}
                        for metric in measured[0]:
                            values=[r[metric] for r in measured]
                            growth[key][metric]=dict(values=values,ratios=[b/a if a else None for a,b in zip(values,values[1:])],
                                above_quadrupling=[a>0 and b>4*a for a,b in zip(values,values[1:])])
    summary.update(scope="one_edge_collective_pool_not_global_q34_producer",growth=growth,
        baseline="tranche25_Jung_Universal",shared_preparation_charged_in_both_sums=True,
        pipeline_sums_are_not_independent_wall_measurements=True,dense_large_seed_growth_not_executed=True)
    return summary


def selftest(path):
    summary = read(path)
    records = [read_json(file) for file in sorted(path.glob("record_*.json"))]
    base = next(r for r in records if r["kind"] == "measure" and r["row"]["regime"] == "far" and r["row"]["budget"] == 32)
    mutants = 0
    def reject(action):
        nonlocal mutants
        try: action()
        except InvalidReceipt: mutants += 1
        else: raise InvalidReceipt("reader mutant survived")
    for route, value in ((["n"], True), (["budget"], True), (["edge_ids"], [False, 1]),
            (["digest", "hash"], 0), (["digest", "shell_ids_visited"], 0), (["validation", "independent_ball_point_tests"], 0),
            (["filter_work", "paired_predicate_tests"], 0), (["filter_work", "seed_queries"], True),
            (["filter_work", "both_rejected"], 1), (["filter_work", "q3_credits"], 999999),
            (["filter_work", "event_side_tests"], 999999), (["filter_work", "variance_bounds"], 999999),
            (["peak_live_buffer_bytes"], 0),
            (["pool_work", "selected_sites"], 0), (["edge_work", "covered", "site_reads"], 0),
            (["cover_work", "admitted_sites"], 0), (["index_work", "nodes"], 0),
            (["memory", "id_bytes"], True), (["timings", "collective_prepare_run_sum_ms"], float("nan"))):
        row = deepcopy(base["row"])
        target = row
        for key in route[:-1]: target = target[key]
        target[route[-1]] = value
        reject(lambda: validate_row(row, base["command"]))
    command = base["command"].copy(); command[5] = "jung_collective" if base["row"]["mode"] != "jung_collective" else "jung_universal"
    reject(lambda: validate_row(base["row"], command))
    reject(lambda: parse_result(b'{"x":NaN}'))
    reject(lambda: parse_result(b'{"x":1,"x":2}'))
    for record in records:
        if record["kind"] != "gate": continue
        gate = deepcopy(record["row"]); gate["checks"] = True
        reject(lambda: validate_gate(gate, Path(record["command"][0]).name))
        gate["checks"] = 0
        reject(lambda: validate_gate(gate, Path(record["command"][0]).name))
    m, c = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
    c["source_sha256_after"][next(iter(m["source_sha256"]))] = "0"*64
    reject(lambda: validate_closure(m, c))
    return dict(status="passed", mutants=mutants, real_measurements=summary["measurements"], scope="receipt_reader_not_geometry")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    run_parser = sub.add_parser("run")
    run_parser.add_argument("--build", type=Path, required=True)
    run_parser.add_argument("--output", type=Path, required=True)
    run_parser.add_argument("--campaign", choices=("gate", "gates", "smoke", "scale", "regression"), required=True)
    reader = sub.add_parser("read"); reader.add_argument("path", type=Path)
    reader.add_argument("--check-live", action="store_true")
    unit = sub.add_parser("selftest"); unit.add_argument("path", type=Path)
    args = parser.parse_args()
    install_adapters()
    if args.operation == "run":
        if args.campaign == "gates": args.campaign = "gate"
        collector.run(args)
    else:
        print(json.dumps(read(args.path, args.check_live) if args.operation == "read" else selftest(args.path), sort_keys=True, allow_nan=False))


if __name__ == "__main__":
    main()
