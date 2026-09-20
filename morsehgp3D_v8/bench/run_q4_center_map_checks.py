#!/usr/bin/env python3
"""Lazy center-map comparison against tranche26 Variance/Collective.

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

import run_q34_collective_checks as reference
previous = reference.previous
from run_p0_matrix import InvalidReceipt, parse_result, require, uint
from run_q4_family_checks import counts, read_json, validate_closure

collector = previous.collector
ROOT = previous.ROOT
SOURCES = reference.SOURCES | frozenset({
    "morsehgp3D_v8/src/lanes/q4_positive_domain.hpp", "morsehgp3D_v8/src/lanes/q4_positive_domain.cpp",
    "morsehgp3D_v8/src/lanes/q4_center_map.hpp", "morsehgp3D_v8/src/lanes/q4_center_map.cpp",
    "morsehgp3D_v8/tests/q4_center_map_gate.cpp",
    "morsehgp3D_v8/bench/q4_center_map_probe.cpp", "morsehgp3D_v8/bench/run_q4_center_map_checks.py",
})
SCHEMA = "mhgp8_q4_center_map_attempt_v1"
GATES = (*reference.GATES, "mhgp8_q4_center_map_gate")
REGIMES = previous.REGIMES
BUDGETS = (64,)
DOMAINS = ("disk", "positive")
DEPTHS = (5, 7)
NODE_BUDGET = 4096
FILTER_FIELDS = reference.FILTER_FIELDS
MAP_FIELDS = ["preparations","prepared_forms","projection_points","hull_sort_comparisons","hull_orientation_tests","hull_vertices","facets","queries","seed_owner_tests","seed_owner_rejections","rejected_queries","unknown_queries","query_visits","line_tests","line_skips","cells_created","pending_ids_copied","pending_lists_created","node_storage_growths","cells_evaluated","disk_tests","facet_tests","outside_cells","deep_cells","witness_tests","inside_credits","outside_witnesses","splits","compressed_deep","compressed_outside","depth_stops","budget_stops","empty_stops","max_depth","peak_pending_bytes","peak_retained_bytes"]
DOMAIN_FIELDS = ["node_visits","bound_tests","endpoint_box_tests","endpoint_leaf_tests","point_tests","admitted_nodes","rejected_nodes","split_nodes","excluded_endpoints","admitted_sites","rejected_sites","box_merges"]
POOL_FIELDS = ("range_visits", "selected_sites")
MEMORY_FIELDS = ("id_bytes", "input_capacity_bytes", "cloud_retained_bytes", "index_retained_bytes", "cover_retained_bytes",
                 "pool_retained_bytes", "baseline_output_retained_bytes", "output_retained_bytes")
PART_TIMES = ("generation_ms", "fixture_validation_ms", "cloud_ms", "index_ms", "cover_ms", "baseline_run_callback_ms",
              "baseline_validation_ms", "pool_prepare_ms", "run_callback_ms", "validation_ms", "release_ms")
FIXED = dict(schema="mhgp8_q4_center_map_probe_v1", status="completed", scope="one_edge_lazy_center_map_not_global_q34_producer",
             public_status="not_claimed", backend="cpu_reference", profile="quantized_u16_input_only", threads=1, seed=3,
             recipe="owned_edge_far_cap_dense_v1", edge_ids=[0, 1],
             timing_scope="paired_wall_including_validation_release_pipeline_sums_share_measured_preparation")
GATE_FLOORS = {
    "checks": 1,
    "domains": 1,
    "domain_sites": 1,
    "empty_domains": 1,
    "obtuse_completions": 1,
    "lens_boundary": 1,
    "outside_lens_inside": 1,
    "admitted_domain_blocks": 0,
    "rejected_domain_blocks": 0,
    "maps": 1,
    "queries": 1,
    "rejected_queries": 1,
    "unknown_queries": 1,
    "requeries": 1,
    "order_trials": 1,
    "positive_presentations": 1,
    "pool_depth_checks": 1,
    "shallow_pool_roots": 1,
    "disk_contacts": 1,
    "line_skips": 1,
    "witness_tests": 1,
    "inside_credits": 0,
    "outside_witnesses": 0,
    "outside_cells": 0,
    "deep_cells": 0,
    "splits": 1,
    "compressed_deep": 0,
    "compressed_outside": 0,
    "depth_stops": 1,
    "budget_stops": 1,
    "empty_stops": 1,
    "max_depth": 0,
    "rotations": 1,
    "extreme_calls": 1,
    "map_q3_only": 1,
    "map_both_rejected": 1,
    "bypass_calls": 1,
    "deferred_calls": 1,
    "edge_calls": 1,
    "seed_calls": 1,
    "reference_calls": 1,
    "oracle_completions": 1,
    "oracle_sites": 1,
    "candidates": 1,
    "q3": 1,
    "q4": 1,
    "max_shell": 30,
    "exhaustive_edges": 1,
    "invalid_inputs": 1,
    "callback_failures": 1,
    "allocation_failures": 4,
    "parallel_calls": 4
}


validate_pruned_seed = reference.validate_pruned_seed
validate_filter = reference.validate_filter


def validate_map(row, n, seeds, covered, selected, domain, depth, nodes, id_bytes):
    w = row["map_work"]
    require(type(w) is dict and set(w) == set(MAP_FIELDS) | {"domain"}, "map fields differ")
    for key in MAP_FIELDS: uint(w[key], "map." + key)
    d = w["domain"]
    counts(d, DOMAIN_FIELDS, "positive_domain")
    q = row["filter_work"]
    only3 = uint(row["q3_only_after_map"], "q3_only_after_map")
    both_rejected = uint(row["both_rejected_by_map"], "both_rejected_by_map")
    both_before = q["both_survivors"] if selected else seeds
    require(only3 + both_rejected == w["rejected_queries"] and
            only3 <= both_before and both_rejected <= q["q4_only_survivors"], "map rejection/lane partition differs")
    lane = dict(q)
    lane["q4_rejected"] += w["rejected_queries"]
    lane["both_rejected"] += both_rejected
    lane["both_survivors"] = both_before - only3
    lane["q3_only_survivors"] += only3
    lane["q4_only_survivors"] -= both_rejected
    available = seeds - q["q4_rejected"]
    if nodes == 0 or available == 0:
        require(all(w[key] == 0 for key in MAP_FIELDS) and all(value == 0 for value in d.values()),
                "disabled or unneeded map performed work")
        return lane
    require(w["preparations"] == 1 and w["prepared_forms"] == selected and w["queries"] == available and
            w["seed_owner_tests"] == 2*available and w["seed_owner_rejections"] == 0 and
            w["rejected_queries"] + w["unknown_queries"] == available, "lazy preparation/query partition differs")
    require(w["cells_created"] == 1 + 4*w["splits"] <= nodes and w["max_depth"] <= depth and
            w["cells_evaluated"] == w["disk_tests"] <= w["cells_created"] and
            w["outside_cells"] + w["deep_cells"] <= w["cells_evaluated"] and
            w["compressed_deep"] + w["compressed_outside"] <= w["splits"], "map cell state ledger differs")
    require(selected <= w["pending_ids_copied"] <= selected*w["cells_created"] and
            w["pending_lists_created"] <= min(w["cells_created"], w["pending_ids_copied"]) and
            1 <= w["node_storage_growths"] <= w["cells_created"] and
            (selected == 0 or w["pending_lists_created"] >= 1), "map inherited-list allocation/copy ledger differs")
    require(w["queries"] <= w["query_visits"] and w["line_skips"] <= w["line_tests"] <= w["query_visits"] and
            w["cells_evaluated"] <= w["line_tests"]-w["line_skips"] and
            w["depth_stops"] + w["budget_stops"] + w["empty_stops"] == w["unknown_queries"], "map query/UNKNOWN ledger differs")
    require(w["inside_credits"] + w["outside_witnesses"] <= w["witness_tests"] <= selected*w["cells_evaluated"] and
            w["facet_tests"] <= w["facets"]*w["disk_tests"] and
            w["peak_pending_bytes"] >= selected*id_bytes and w["peak_retained_bytes"] >= w["peak_pending_bytes"] + 24*selected,
            "map witness/capacity ledger differs")
    if domain == "disk":
        require(all(value == 0 for value in d.values()) and all(w[key] == 0 for key in
                ("projection_points", "hull_sort_comparisons", "hull_orientation_tests", "hull_vertices", "facets", "facet_tests")),
                "disk-only map performed positive-domain work")
    else:
        completions = n-2 if row["regime"] == "adversarial" else 4
        require(d["node_visits"] == d["bound_tests"] + d["endpoint_leaf_tests"] <= 2*n-1 and
                d["node_visits"] == d["admitted_nodes"] + d["rejected_nodes"] + d["split_nodes"] + d["excluded_endpoints"] and
                d["endpoint_leaf_tests"] == d["point_tests"] + d["excluded_endpoints"] and d["excluded_endpoints"] == 2 and
                d["admitted_sites"] == completions and d["admitted_sites"] + d["rejected_sites"] + 2 == n and
                d["box_merges"] == d["admitted_nodes"]-1 and d["endpoint_box_tests"] % 2 == 0 and
                d["endpoint_box_tests"] <= 2*d["bound_tests"], "positive-domain partition differs")
        require(w["projection_points"] == 9 and w["hull_sort_comparisons"] > 0 and w["hull_orientation_tests"] > 0 and
                3 <= w["hull_vertices"] == w["facets"] <= 9, "positive-domain projection/hull work missing")
    return lane


def validate_row(row, command):
    require(type(row) is dict and len(command) == 8, "pruning probe command arity")
    n, regime, k, budget = int(command[1]), command[2], int(command[3]), int(command[4])
    domain, depth, nodes = command[5], int(command[6]), int(command[7])
    require(n >= 8 and regime in REGIMES and k in (5, 10) and budget in (0, 32, 64) and domain in DOMAINS and 0 <= depth <= 44 and nodes >= 0 and
            (regime != "cap" or n <= 35307) and (regime != "adversarial" or n <= 514), "fixture domain mismatch")
    extra = {"n", "regime", "kmax", "budget", "input_hash", "expected_seeds", "cover_sites", "generation", "validation",
             "baseline_work", "edge_work", "baseline_filter_work", "filter_work", "filter_mode", "domain", "map_depth", "map_node_budget",
             "map_work", "q3_only_after_map", "both_rejected_by_map", "baseline_peak_live_buffer_bytes", "collective_peak_live_buffer_bytes", "peak_live_buffer_bytes", "pool_work", "pool_hash", "cover_work", "index_work", "cloud_work",
             "baseline_digest", "digest", "memory", "timings"}
    require(set(row) == set(FIXED) | extra and all(type(row[key]) is type(value) and row[key] == value for key, value in FIXED.items()) and
            all(type(value) is int for value in row["edge_ids"]), "probe schema/scope fields differ")
    require(all(type(row[key]) is int and row[key] == value for key, value in dict(n=n, kmax=k, budget=budget, map_depth=depth, map_node_budget=nodes).items()) and
            row["regime"] == regime and row["domain"] == domain and row["filter_mode"] == "variance_collective", "command/result differs")
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
            v["method"] == ("qualified_collective_path_differential" if regime == "adversarial" else "three_closed_form_balls_plus_differential"),
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
    p, q = row["baseline_filter_work"], row["filter_work"]
    validate_filter(p, seeds, selected, k, "variance_collective", m["id_bytes"])
    validate_filter(q, seeds, selected, k, "variance_collective", m["id_bytes"])
    require(p == q, "map changed upstream collective filter work")
    map_lane = validate_map(row, n, seeds, covered, selected, domain, depth, nodes, m["id_bytes"])
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
        lane = p if section == "baseline_work" else map_lane
        validate_pruned_seed(w["seed"], seeds, covered, lane, row["digest"], m["id_bytes"], budget > 0)
        both = lane["both_survivors"] if budget else seeds
        require(w["site_reads"] == w["seed"]["family"]["sites"] + w["seed"]["q3_point_tests"] - covered*both and
                w["site_reads"] <= covered*(seeds-lane["both_rejected"]), "remaining lane scan union differs")
        capacities = (w["seed"]["q3_shell_capacity_bytes"], w["seed"]["family"]["retained_capacity_bytes"])
        require(max(capacities) <= w["peak_buffer_bytes"] <= sum(capacities), "simultaneous peak capacity differs")
    require(all(row["baseline_work"][key] == row["edge_work"][key] for key in previous.EDGE_FIELDS), "pruning changed seed enumeration")
    for peak_name, work in (("baseline_peak_live_buffer_bytes", row["baseline_work"]),
                            ("collective_peak_live_buffer_bytes", row["edge_work"])):
        peak = uint(row[peak_name], peak_name)
        capacities = (q["peak_event_bytes"], work["covered"]["peak_buffer_bytes"])
        require(max(capacities) <= peak <= sum(capacities), "coupled workspace/fallback peak differs")
    peak = uint(row["peak_live_buffer_bytes"], "peak_live_buffer_bytes")
    capacities = (row["map_work"]["peak_retained_bytes"], row["collective_peak_live_buffer_bytes"])
    require(max(capacities) <= peak <= sum(capacities), "coupled map/workspace/fallback peak differs")
    if nodes == 0:
        require(row["baseline_work"] == row["edge_work"] and peak == row["baseline_peak_live_buffer_bytes"],
                "disabled map changed fallback or memory")
    t = row["timings"]
    require(type(t) is dict and set(t) == set(PART_TIMES) | {"baseline_prepare_run_sum_ms", "mapped_prepare_run_sum_ms", "paired_total_ms"} and
            all(type(value) in (int, float) and math.isfinite(value) and value >= 0 for value in t.values()), "invalid timing")
    common = ("generation_ms", "cloud_ms", "index_ms", "cover_ms", "pool_prepare_ms")
    for total, parts in (("paired_total_ms", PART_TIMES), ("baseline_prepare_run_sum_ms", (*common, "baseline_run_callback_ms")),
                         ("mapped_prepare_run_sum_ms", (*common, "run_callback_ms"))):
        require(math.isclose(t[total], sum(t[key] for key in parts), rel_tol=1e-12, abs_tol=1e-6), "timing sum scope/partition differs")


def validate_gate(row, executable):
    if executable in reference.GATES:
        reference.validate_gate(row, executable)
        return
    require(executable == GATES[5] and type(row) is dict and set(row) == set(GATE_FLOORS) | {"schema", "status"} and
            row["schema"] == executable + "_v1" and row["status"] == "passed", "map gate schema/fields differ")
    for key, floor in GATE_FLOORS.items(): require(uint(row[key], "gate." + key) >= floor, "map gate floor failed")
    require(row["candidates"] == row["q3"] + row["q4"] and row["parallel_calls"] == 4 and
            row["callback_failures"] == 1 and row["allocation_failures"] == 4, "map gate output/lifecycle ledger differs")


def validate_xml(path):
    tree = ET.parse(path).getroot()
    require(tree.tag == "testsuite" and tree.get("tests") == "88" and
            all(tree.get(key) == "0" for key in ("failures", "disabled", "skipped")), "unsuccessful 88-test regression")
    cases = tree.findall("testcase")
    names = [case.get("name") for case in cases]
    require(len(cases) == len(set(names)) == 88 and all(type(name) is str and name.startswith("mhgp8_") for name in names) and
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
            for domain in DOMAINS:
                for depth in DEPTHS:
                    for n in sizes:
                        result.append(("measure", [str(build / "mhgp8_q4_center_map_probe"), str(n), regime, str(k), "64", domain, str(depth), str(NODE_BUDGET)]))
    return result


def executable_names(build, campaign):
    if campaign == "regression":
        names = sorted(p.name for p in build.glob("mhgp8_*") if p.is_file() and os.access(p, os.X_OK))
        require(names and all((build / name).resolve() == build / name for name in names), "missing or linked executable")
        return names
    return sorted([*GATES, *(["mhgp8_q4_center_map_probe"] if campaign != "gate" else [])])


def install_adapters():
    collector.SOURCES, collector.SCHEMA, collector.GATES = SOURCES, SCHEMA, GATES
    collector.FAMILIES = ()
    collector.plan, collector.executable_names = plan, executable_names
    collector.validate_row, collector.validate_gate, collector.validate_xml = validate_row, validate_gate, validate_xml


def read(path, check_live=False):
    install_adapters()
    summary = previous.BASE_READ(path, check_live)
    rows = [record["row"] for file in sorted(path.glob("record_*.json")) if (record := read_json(file))["kind"] == "measure"]
    for regime in REGIMES:
        for n in sorted({r["n"] for r in rows if r["regime"] == regime}):
            group = [r for r in rows if r["n"] == n and r["regime"] == regime]
            require(len(group) == 2*len(DOMAINS)*len(DEPTHS) and
                    {(r["kmax"],r["domain"],r["map_depth"]) for r in group} ==
                    {(k,d,depth) for k in (5,10) for d in DOMAINS for depth in DEPTHS}, "missing K/domain/depth comparison")
            require(all(all(r[key] == group[0][key] for key in ("input_hash", "generation", "cloud_work", "index_work", "cover_work", "pool_work", "pool_hash")) for r in group),
                    "same-size immutable preparation differs")
            for k in (5,10):
                options = [r for r in group if r["kmax"] == k]
                require(all(all(r[key] == options[0][key] for key in ("baseline_work", "baseline_filter_work", "baseline_peak_live_buffer_bytes", "digest", "baseline_digest")) for r in options),
                        "map option changed baseline or outputs")
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
                for domain in DOMAINS:
                    for depth in DEPTHS:
                        series = sorted((r for r in rows if (r["regime"],r["kmax"],r["domain"],r["map_depth"]) == (regime,k,domain,depth)),key=lambda r:r["n"])
                        measured = [flatten({key:r[key] for key in ("baseline_work","baseline_filter_work","edge_work","filter_work","map_work",
                            "q3_only_after_map","both_rejected_by_map","baseline_peak_live_buffer_bytes","collective_peak_live_buffer_bytes","peak_live_buffer_bytes",
                            "pool_work","cover_work","index_work","cloud_work","digest","memory","timings")}) for r in series]
                        key = f"{regime}_K{k}_C64_{domain}_D{depth}"
                        growth[key] = {}
                        for metric in measured[0]:
                            values=[r[metric] for r in measured]
                            growth[key][metric]=dict(values=values,ratios=[b/a if a else None for a,b in zip(values,values[1:])],
                                above_quadrupling=[a>0 and b>4*a for a,b in zip(values,values[1:])])
    summary.update(scope="one_edge_lazy_center_map_not_global_q34_producer",growth=growth,
        baseline="tranche26_Variance_Collective",shared_preparation_charged_in_both_sums=True,
        map_and_domain_preparation_included_in_mapped_run=True,
        pipeline_sums_are_not_independent_wall_measurements=True,dense_large_seed_growth_not_executed=True)
    return summary


def selftest(path):
    summary = read(path)
    records = [read_json(file) for file in sorted(path.glob("record_*.json"))]
    base = next(r for r in records if r["kind"] == "measure" and r["row"]["regime"] == "far" and r["row"]["domain"] == "positive")
    mutants = 0
    def reject(action):
        nonlocal mutants
        try: action()
        except InvalidReceipt: mutants += 1
        else: raise InvalidReceipt("reader mutant survived")
    for route, value in ((["n"], True), (["budget"], True), (["edge_ids"], [False, 1]),
            (["digest", "hash"], 0), (["digest", "shell_ids_visited"], 0), (["validation", "independent_ball_point_tests"], 0),
            (["filter_work", "paired_predicate_tests"], 0), (["filter_work", "seed_queries"], True),
            (["filter_work", "both_rejected"], 1),
            (["map_work", "queries"], 0), (["map_work", "prepared_forms"], 0), (["map_work", "domain", "admitted_sites"], 0),
            (["map_work", "pending_ids_copied"], 0), (["map_work", "pending_lists_created"], 0),
            (["map_work", "node_storage_growths"], 0),
            (["q3_only_after_map"], 999999), (["map_work", "cells_created"], True), (["filter_work", "q3_credits"], 999999),
            (["filter_work", "event_side_tests"], 999999), (["filter_work", "variance_bounds"], 999999),
            (["peak_live_buffer_bytes"], 0),
            (["pool_work", "selected_sites"], 0), (["edge_work", "covered", "site_reads"], 0),
            (["cover_work", "admitted_sites"], 0), (["index_work", "nodes"], 0),
            (["memory", "id_bytes"], True), (["timings", "mapped_prepare_run_sum_ms"], float("nan"))):
        row = deepcopy(base["row"])
        target = row
        for key in route[:-1]: target = target[key]
        target[route[-1]] = value
        reject(lambda: validate_row(row, base["command"]))
    command = base["command"].copy(); command[5] = "disk"
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
