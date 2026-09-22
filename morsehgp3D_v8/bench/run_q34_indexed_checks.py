#!/usr/bin/env python3
"""Tranche32 indexed q3/q4 qualification and shared LiDAR receipt collector.

Reuse the tranche31 input validator and low-level pinned invocation utilities.
Capture schemas, witness work, command matrices and verdicts are new. Existing
sources/receipts are never patched or imported as inherited qualifications.
"""
from __future__ import annotations
import argparse
import base64
from copy import deepcopy
import json
import math
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

import run_wspd_q34_lidar as previous
import run_q34_lidar_checks as edge
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import InvalidReceipt, invoke, on_signal, parse_result, require, uint, utc_stamp, write_json

# Largeur de coordonnée du moteur entier (18 bits depuis le 22 septembre 2026) : au plus 18 coupes
# au milieu par axe, donc 54 niveaux d'index et 55 cadres de pile ; bornes prouvées, jamais des quotas.
COORDINATE_BITS = 18
MAX_INDEX_DEPTH = 3 * COORDINATE_BITS
INDEX_STACK_FRAMES = MAX_INDEX_DEPTH + 1

ROOT = previous.ROOT
MODES = ("disabled", "pair", "rectangle-pair")
SCHEMA = "mhgp8_q34_indexed_capture_v1"
SOURCES = previous.SOURCES | frozenset({
    "morsehgp3D_v8/src/lanes/q34_witness_search.hpp",
    "morsehgp3D_v8/src/lanes/q34_witness_search.cpp",
    "morsehgp3D_v8/tests/q34_witness_search_gate.cpp",
    "morsehgp3D_v8/src/lanes/q3_ball_census.hpp",
    "morsehgp3D_v8/src/lanes/q3_ball_census.cpp",
    "morsehgp3D_v8/tests/q3_ball_census_gate.cpp",
    "morsehgp3D_v8/tests/q34_indexed_mutations.py",
    "morsehgp3D_v8/bench/run_q34_indexed_checks.py",
    "morsehgp3D_v8/bench/run_q34_indexed_lidar.py",
})
GATES = ("mhgp8_q34_witness_search_gate", "mhgp8_q3_ball_census_gate", "mhgp8_wspd_q34_gate")
CTEST_COUNT = 94
WITNESS_FIELDS = ("input_pair_mass rejected_rectangles rectangle_pair_mass rectangle_q3_pairs "
                  "rectangle_q4_pairs rejected_pairs pair_q3_pairs pair_q4_pairs").split()
SEARCH_FIELDS = ("queries q3_queries q4_queries prepared_bounds node_visits h_bound_tests xi_bound_tests "
    "point_tests h_excluded_nodes admitted_nodes fully_admitted_nodes leaf_remainders split_nodes "
    "q3_lane_tests q4_lane_tests q3_admitted_nodes q4_admitted_nodes q3_credits q4_credits q3_rejected "
    "q4_rejected midpoint_box_tests pending_nodes_skipped peak_stack stack_storage_bytes").split()
GLOBAL_GATE_ADDED = ("indexed_calls", "indexed_pair_rejections", "indexed_rectangle_rejections", "boxed_calls")
Q3_BLOCK_FIELDS = ("queries preparations vertex_axes accepted_queries rejected_queries count_node_visits "
    "count_bounds_prepared count_box_bound_tests count_point_tests count_inside_nodes count_inside_sites "
    "count_nonnegative_nodes count_nonnegative_sites count_split_nodes count_prepared_unvisited count_saturations "
    "shell_node_visits shell_bounds_prepared shell_box_bound_tests shell_point_tests shell_excluded_nodes "
    "shell_split_nodes shell_ids peak_count_stack peak_shell_stack stack_storage_bytes").split()
SEARCH_GATE_FIELDS = ("checks queries singleton_queries rectangle_queries oracle_sites oracle_pairs "
    "q3_rejections q4_rejections q3_only_rejections q4_only_rejections both_rejections no_rejections "
    "inactive_queries zero_mask_queries partial_q3 partial_q4 h_contacts q3_contacts q4_contacts "
    "wrong_alpha_cases positive_q4_external whole_admissions point_admission_cases node_rejections saturations "
    "near_right_splits exhausted_queries permutations extreme_queries invalid_inputs parallel_calls "
    "repeated_calls source_alias_checks left_tie_cases partial_admission_cases deep_index_cases peak_stack "
    "wide_predicate_cases overflow_exceptions").split()
BALL_GATE_FIELDS = ("checks calls oracle_sites accepted rejected q2_balls q3_balls q4_balls shell_ids max_shell "
    "empty_shells integer_grid_contacts ceil_required floor_required whole_inside whole_nonnegative shell_exclusions "
    "saturating_overshoot prepared_unvisited near_first_cases singleton_cases extreme_cases wide_linear_squares "
    "permutations huge_threshold invalid_inputs allocation_failures allocation_free_rejections parallel_calls "
    "repeated_calls source_alias_checks overflow_exceptions").split()
BALL_GATE_U18_ADDED = ("wide_linear_squares_u18", "extreme_cases_u18")


def validate_matrix(matrix):
    require(type(matrix) is dict and set(matrix) ==
            {"scans", "sizes", "kmax", "s", "workers", "backends", "payload", "witness_modes", "q3_census_mode"},
            "indexed matrix fields differ")
    previous.validate_matrix({key: value for key, value in matrix.items() if key not in ("witness_modes", "q3_census_mode")})
    require(type(matrix["q3_census_mode"]) is str and matrix["q3_census_mode"] in ("scalar", "boxes"), "q3 census mode")
    modes = matrix["witness_modes"]
    require(type(modes) is list and modes and all(type(v) is str and v in MODES for v in modes) and
            len(modes) == len(set(modes)), "empty/duplicate/invalid witness modes")


def configuration(args):
    matrix = None
    if args.campaign == "smoke":
        matrix = dict(scans=[0], sizes=[64], kmax=[5, 10], s=[8], workers=[1, 4], backends=[28],
                      witness_modes=list(MODES), payload="records", q3_census_mode=args.q3_census_mode)
    return dict(campaign=args.campaign, qualification=args.qualification, matrix=matrix)


def configuration_from_launch(command):
    require(type(command) is list and len(command) >= 3 and all(type(v) is str for v in command) and
            command[2] == "run", "invalid indexed launch command")
    name = Path(command[1]).name
    if name == "run_q34_indexed_lidar.py":
        import run_q34_indexed_lidar as lidar
        parser, configure = lidar.make_parser(), lidar.configuration
    else:
        require(name == "run_q34_indexed_checks.py", "unrecognized indexed launcher")
        parser, configure = make_parser(), configuration
    try:
        args = parser.parse_args(command[2:])
    except SystemExit:
        raise InvalidReceipt("unreadable indexed launch configuration")
    require(args.operation == "run", "launch is not a run")
    return args.build.resolve(), configure(args)


def plan(build, config, capture, ctest=None):
    require(type(config) is dict and set(config) == {"campaign", "qualification", "matrix"} and
            config["qualification"] in ("preflight", "candidate"), "invalid indexed configuration")
    campaign = config["campaign"]
    require(campaign in ("gate", "smoke", "regression", "lidar"), "invalid indexed campaign")
    if campaign == "regression":
        require(config["matrix"] is None and ctest and Path(ctest).is_absolute(), "invalid regression plan")
        return [], set(), [("ctest", [ctest, "--test-dir", str(build), "--output-on-failure",
            "--parallel", "2", "--output-junit", str(capture / "result.xml")])]
    commands = [("gate", [str(build / name), "--selftest"]) for name in GATES] if campaign != "lidar" else []
    if campaign == "gate":
        require(config["matrix"] is None, "gate matrix must be absent")
        return [], set(), commands
    matrix = config["matrix"]
    validate_matrix(matrix)
    if campaign == "smoke":
        require(matrix == configuration(argparse.Namespace(campaign="smoke", qualification="candidate",
                        q3_census_mode=matrix["q3_census_mode"]))["matrix"],
                "smoke matrix differs")
    base = {key: value for key, value in matrix.items() if key not in ("witness_modes", "q3_census_mode")}
    datasets, native, files = previous.prepare_matrix(build, base)
    commands.extend(("probe", [*command, mode, matrix["q3_census_mode"]])
                    for command in native for mode in matrix["witness_modes"])
    return datasets, files, commands


def executable_names(build, campaign):
    if campaign == "regression":
        names = sorted(p.name for p in build.glob("mhgp8_*") if p.is_file() and os.access(p, os.X_OK))
        require(set(GATES) <= set(names), "regression executable inventory lacks new gates")
    else:
        names = sorted({*(GATES if campaign != "lidar" else ()),
                        *(["mhgp8_wspd_q34_probe"] if campaign in ("smoke", "lidar") else [])})
    require(all((build / name).resolve() == build / name for name in names), "linked executable")
    return names


def artifact_paths(build, names):
    return {str((build / name).relative_to(ROOT)) for name in [*names, "libmhgp8_p0.a", "CMakeCache.txt"]}


def validate_gate(row, name):
    if name == "mhgp8_wspd_q34_gate":
        previous.exact_fields(row, [*edge.GLOBAL_GATE_FIELDS, *GLOBAL_GATE_ADDED, "schema", "status"], "global gate")
        require(row["schema"] == "mhgp8_wspd_q34_gate_v1" and row["status"] == "PASS", "global gate identity")
        for key in (*edge.GLOBAL_GATE_FIELDS, *GLOBAL_GATE_ADDED):
            require(uint(row[key], key) > 0, "global gate vacuity")
        # Explicit tranche32 lifecycle: six new empty indexed/census cases
        # extend the two tranche31 cases. Never rewrite observed gate output.
        exact = dict(allocation_failures=4, callback_failures=1, parallel_calls=4, nested_calls=1,
            owner_reset_calls=1, input_alias_checks=1, parallel_callback_failures=1,
            parallel_owner_resets=1, parallel_join_checks=2, parallel_empty_calls=8)
        require(all(row[key] == value for key, value in exact.items()) and
                row["candidates"] == row["q3"] + row["q4"] and row["max_shell"] >= 30,
                "indexed global gate lifecycle differs")
        return
    if name == "mhgp8_q34_witness_search_gate":
        previous.exact_fields(row, [*SEARCH_GATE_FIELDS, "schema", "status"], "witness gate")
        require(row["schema"] == "mhgp8_q34_witness_search_gate_v1" and row["status"] == "PASS", "witness gate identity")
        for key in SEARCH_GATE_FIELDS:
            require(uint(row[key], key) > 0, "witness gate vacuity")
        exact = dict(parallel_calls=4, permutations=1, source_alias_checks=1, repeated_calls=1, left_tie_cases=1,
            partial_admission_cases=1, deep_index_cases=1, peak_stack=INDEX_STACK_FRAMES, overflow_exceptions=1, q3_contacts=1,
            q4_contacts=1, wrong_alpha_cases=1, positive_q4_external=1, h_contacts=1, extreme_queries=15, invalid_inputs=11,
            point_admission_cases=1)
        require(all(row[key] == value for key, value in exact.items()), "witness gate contract fixture counts")
        return
    require(name == "mhgp8_q3_ball_census_gate", "unknown indexed gate")
    u18 = type(row) is dict and any(field in row for field in BALL_GATE_U18_ADDED)
    fields = [*BALL_GATE_FIELDS, *(BALL_GATE_U18_ADDED if u18 else ())]
    previous.exact_fields(row, [*fields, "schema", "status"], "ball census gate")
    require(row["schema"] == "mhgp8_q3_ball_census_gate_v1" and row["status"] == "PASS", "ball gate identity")
    for key in fields:
        require(uint(row[key], key) > 0, "ball gate vacuity")
    exact = dict(integer_grid_contacts=1, ceil_required=1, floor_required=1, saturating_overshoot=1,
        near_first_cases=1, singleton_cases=4 if u18 else 3, extreme_cases=5, wide_linear_squares=3, permutations=1,
        huge_threshold=1, invalid_inputs=1, allocation_failures=1, allocation_free_rejections=1,
        parallel_calls=4, repeated_calls=1, source_alias_checks=1, overflow_exceptions=1)
    require(all(row[key] == value for key, value in exact.items()) and row["max_shell"] >= 30 and
            row["calls"] == row["accepted"] + row["rejected"],
            "ball gate contract fixtures")
    if u18:
        require(row["wide_linear_squares_u18"] == 3 and row["extreme_cases_u18"] == 5,
                "ball census u18 wide-support floors")


def validate_search(search, n, k, id_bytes):
    edge.counts(search, SEARCH_FIELDS, "indexed witness search")
    s = search
    require(max(s["q3_queries"], s["q4_queries"]) <= s["queries"] <= s["q3_queries"] + s["q4_queries"] and
            s["prepared_bounds"] == s["queries"], "active search query ledger")
    require(s["node_visits"] == s["h_bound_tests"] == s["h_excluded_nodes"] + s["fully_admitted_nodes"] +
            s["leaf_remainders"] + s["split_nodes"] and
            s["midpoint_box_tests"] == 2 * s["split_nodes"] and
            s["leaf_remainders"] <= s["point_tests"] <= s["node_visits"] <= s["queries"] * (2*n-1),
            "search node partition")
    require(s["fully_admitted_nodes"] <= s["admitted_nodes"] <= s["xi_bound_tests"] <=
            s["node_visits"] - s["h_excluded_nodes"] and
            max(s["q3_lane_tests"], s["q4_lane_tests"]) <= s["xi_bound_tests"] <=
            s["q3_lane_tests"] + s["q4_lane_tests"] and
            max(s["q3_admitted_nodes"], s["q4_admitted_nodes"]) <= s["admitted_nodes"] <=
            s["q3_admitted_nodes"] + s["q4_admitted_nodes"], "search admission/lane ledger")
    for lane, threshold in (("q3", k-1), ("q4", k-2)):
        require(s[lane + "_admitted_nodes"] <= s[lane + "_lane_tests"] and
                s[lane + "_admitted_nodes"] <= s[lane + "_credits"] and
                s[lane + "_rejected"] <= s[lane + "_queries"] and
                threshold * s[lane + "_rejected"] <= s[lane + "_credits"] <=
                threshold * s[lane + "_rejected"] + (threshold-1) *
                (s[lane + "_queries"] - s[lane + "_rejected"]), "saturated lane credits")
    require(s["node_visits"] + s["pending_nodes_skipped"] <= s["queries"] + 2*s["split_nodes"],
            "search pending frames ledger")
    if s["queries"]:
        require(1 <= s["peak_stack"] <= INDEX_STACK_FRAMES and s["stack_storage_bytes"] in (49*2*id_bytes, INDEX_STACK_FRAMES*2*id_bytes),
                "fixed witness stack capacity (49 frames before the 18-bit widening, 55 after)")
    else:
        require(all(value == 0 for value in s.values()), "inactive search did work")


def validate_row(row, command):
    require(type(command) is list and len(command) == 12 and all(type(v) is str for v in command) and
            command[10] in MODES and type(row) is dict and row.get("schema") == "mhgp8_wspd_q34_probe_v2" and
            type(row.get("witness_mode")) is str and row["witness_mode"] == command[10] and
            type(row.get("q3_census_mode")) is str and row["q3_census_mode"] == command[11] and
            command[11] in ("scalar", "boxes"), "indexed command/schema/mode")
    # Shape-only view of unchanged fields, not a synthetic old execution. The
    # changed expansion/lane partitions are validated below on the actual row.
    require(type(row.get("work")) is dict and {"witness", "q3_blocks"} <= set(row["work"]), "missing indexed work")
    common = deepcopy(row)
    common.pop("witness_mode")
    common.pop("q3_census_mode")
    common["schema"] = previous.FIXED["schema"]
    witness = common["work"].pop("witness")
    blocks = common["work"].pop("q3_blocks")
    previous.strict_shape(common, command[:10])
    edge.structural(witness, WITNESS_FIELDS, ("rectangles", "pairs"), "witness")
    edge.counts(blocks, Q3_BLOCK_FIELDS, "q3 blocks")
    n, k, s, mask, backend, workers = map(int, command[2:8])
    require(Path(command[0]).name == "mhgp8_wspd_q34_probe" and
            [row[key] for key in ("n", "kmax", "s", "mask", "q4_backend", "workers")] ==
            [n, k, s, mask, backend, workers] and row["front_mode"] == command[8] and
            row["output_mode"] == command[9] and 0 < n <= row["source_n"] and k in (5,10) and
            s in (8,10,12) and mask == 6 and backend in (28,30) and workers > 0,
            "indexed command/result mismatch")
    for value in row["timings_ms"].values():
        require(type(value) in (int, float) and math.isfinite(value) and value >= 0, "invalid time")
    front, w, par, out = row["front"], row["work"], row["parallel"], row["output"]
    fw, q3, z = front["work"], w["q3"], witness
    require(front["total_unordered_pairs"] == n*(n-1)//2 and front["active_lane_mask"] == 6, "front metadata")
    for lane in range(3):
        require(fw["rejected_pair_mass"][lane] + fw["residual_pair_mass"][lane] ==
                (n*(n-1)//2 if lane else 0), "front mass partition")
    require(w["input_rectangles"] == fw["emitted_rectangles"] == sum(fw["size_class_rectangles"]) and
            z["input_pair_mass"] == sum(fw["size_class_pair_mass"]) and
            max(fw["residual_pair_mass"]) <= z["input_pair_mass"] <= sum(fw["residual_pair_mass"]) and
            w["expanded_pairs"] + z["rectangle_pair_mass"] == z["input_pair_mass"] and
            w["cover_builds"] + z["rejected_pairs"] == w["expanded_pairs"] and
            w["q3_edges"] + w["q4_edges"] - w["both_edges"] == w["cover_builds"] and
            w["both_edges"] <= min(w["q3_edges"], w["q4_edges"]), "new expansion union partition")
    require(z["rejected_rectangles"] <= w["input_rectangles"] and
            z["rejected_rectangles"] <= z["rectangle_pair_mass"] <=
            z["rectangle_q3_pairs"] + z["rectangle_q4_pairs"] and
            z["rejected_pairs"] <= z["pair_q3_pairs"] + z["pair_q4_pairs"], "union rejections")
    for lane, number in (("q3", 1), ("q4", 2)):
        require(w[lane + "_edges"] + z["rectangle_" + lane + "_pairs"] + z["pair_" + lane + "_pairs"] ==
                fw["residual_pair_mass"][number], "independent before/after lane partition")
    for search in (z["rectangles"], z["pairs"]):
        validate_search(search, n, k, row["memory"]["id_bytes"])
    if row["witness_mode"] == "disabled":
        require(all(value == 0 for name, value in previous.flatten(z).items() if name != "input_pair_mass"),
                "disabled witness mode did work")
        if row["q3_census_mode"] == "scalar":
            previous.validate(common, command[:10])
    else:
        require(z["pairs"]["queries"] == w["expanded_pairs"], "pair search count")
        for lane, number in (("q3",1), ("q4",2)):
            require(z["pairs"][lane + "_queries"] == fw["residual_pair_mass"][number] -
                    z["rectangle_" + lane + "_pairs"] and
                    z["pairs"][lane + "_rejected"] == z["pair_" + lane + "_pairs"], "pair lane search/rejections")
        if row["witness_mode"] == "pair":
            require(all(value == 0 for value in z["rectangles"].values()) and
                    all(z[name] == 0 for name in ("rejected_rectangles", "rectangle_pair_mass",
                                                 "rectangle_q3_pairs", "rectangle_q4_pairs")), "pair-only rectangle work")
        else:
            require(z["rectangles"]["queries"] == w["input_rectangles"], "rectangle search count")
            for lane, number in (("q3",1), ("q4",2)):
                require(z["rectangles"][lane + "_queries"] == fw["lane_rectangles"][number] and
                        z["rectangles"][lane + "_rejected"] <= z["rectangle_" + lane + "_pairs"],
                        "rectangle lane mass/query count")
    require(w["cover_sites"] == w["cover"]["admitted_sites"] <= n*w["cover_builds"] and
            w["cover"]["admitted_sites"] + w["cover"]["rejected_sites"] == n*w["cover_builds"] and
            w["max_cover_sites"] <= n, "cover ledger")
    require(q3["edge_queries"] == w["q3_edges"] and
            q3["census_point_tests"] == q3["census_inside_sites"] + q3["census_outside_sites"] + q3["census_shell_sites"] and
            q3["ball_builds"] == q3["seeds"] == q3["depth_rejections"] + q3["emitted"] and
            q3["emitted"] == w["q3_emitted"] == out["q3"] and q3["shell_ids"] <= w["payload_shell_ids"], "q3 census")
    selected = w["local28"] if backend == 28 else w["window30"]
    unused = w["window30"] if backend == 28 else w["local28"]
    require(all(value == 0 for value in previous.flatten(unused).values()), "unused q4 backend did work")
    require(selected["sweep"]["emitted"] == w["q4_emitted"] == out["q4"] and
            selected["edge"]["seeds"] == selected["sweep"]["seed_queries"] and
            out["callbacks"] == out["q3"] + out["q4"] and
            out["support_ids"] == 3*out["q3"] + 4*out["q4"] and out["shell_ids"] == w["payload_shell_ids"],
            "emitted payload ledger")
    require(par["requested_workers"] == workers and par["target_jobs"] == workers*16 and
            par["started_workers"] == len(row["workers_work"]) == min(workers, par["jobs"]) and
            par["completed_jobs"] == par["jobs"] == sum(item["jobs"] for item in row["workers_work"]), "parallel jobs")
    for field in ("input_rectangles", "expanded_pairs", "q3_emitted", "q4_emitted"):
        require(sum(item[field] for item in row["workers_work"]) == w[field], "worker reduction")
    require(sum(item["front_products"] for item in row["workers_work"]) + par["prefix_product_visits"] == fw["product_visits"] and
            sum(item["peak_edge_buffer_bytes"] for item in row["workers_work"]) == par["edge_buffer_bytes_sum"], "parallel sums")
    validate_auxiliary(row)
    times = row["timings_ms"]
    require(abs(times["cloud"] + times["index"] + times["front_edges_collect"] -
                times["pipeline_including_shared_preparation"]) <= 0.000003 and
            abs(times["load_prefix_hash"] + times["pipeline_including_shared_preparation"] +
                times["record_normalization"] - times["total_before_serialization_and_release"]) <= 0.000003,
            "timing partition")
    if command[9] == "records":
        require(len(row["records"]) == out["callbacks"], "full records missing")
        hashes = []
        for record in row["records"]:
            h = edge.word(edge.word(14695981039346656037, record["arity"]), record["depth"])
            for identifier in record["support"]:
                h = edge.word(h, identifier)
            for value in record["coefficients"]:
                coefficient = int(value) % (1 << 128)
                h = edge.word(edge.word(h, coefficient & edge.MASK64), coefficient >> 64)
            for identifier in record["shell"]:
                h = edge.word(h, identifier)
            hashes.append(edge.word(h, len(record["shell"])))
        xor = 0
        for value in hashes:
            xor ^= value
        require(format(xor, "x") == out["xor"] and format(sum(hashes) & edge.MASK64, "x") == out["sum"],
                "full records/digest differ")


def validate_auxiliary(row):
    """Explicit port of shared31 ledgers, with distinct scalar/boxes census."""
    n, w, m = row["n"], row["work"], row["memory"]
    q, b, c, ix = w["q3"], w["q3_blocks"], row["cloud_work"], row["index_work"]
    require(c["coordinate_copies"] == c["validation_points"] == c["range_tree_leaf_visits"] == n and
            c["uniqueness_adjacent_tests"] == c["range_tree_merges"] == n-1 and c["range_tree_nodes"] == 2*n-1,
            "cloud preparation")
    require(ix["nodes"] == ix["escape_links"] == 2*n-1 and ix["point_visits"] >= n, "index preparation")
    require(m["id_bytes"] in (4,8) and min(m["input_capacity_bytes"],m["cloud_retained_bytes"]) >= 6*n and
            m["index_retained_bytes"] >= n*m["id_bytes"], "shared capacities")
    if row["output_mode"] == "digest":
        require(m["worker_record_capacity_bytes_before_merge"] == m["merged_record_capacity_bytes"] == 0,
                "digest allocated records")
    else:
        require(min(m["worker_record_capacity_bytes_before_merge"],m["merged_record_capacity_bytes"]) >=
                row["output"]["shell_ids"] * m["id_bytes"], "full record capacity")
    cover = w["cover"]
    require(cover["node_visits"] == cover["bound_tests"] + cover["point_tests"] ==
            cover["admitted_nodes"] + cover["rejected_nodes"] + cover["split_nodes"] and
            cover["retained_ranges"] + cover["merged_ranges"] == cover["admitted_nodes"], "cover nodes/ranges")
    require(q["seed_node_visits"] == q["seed_bound_tests"] + q["seed_point_tests"] and
            q["seed_bound_tests"] == q["seed_rejected_nodes"] + q["seed_split_nodes"] and
            q["seed_rejected_sites"] + q["seed_point_tests"] == n*q["edge_queries"] and
            q["acute_seeds"] == q["seeds"] + q["owner_rejections"] and
            q["acute_seeds"] <= q["owner_tests"] <= 2*q["acute_seeds"], "q3 seed generation")
    require(q["shell_ids"] >= 3*q["emitted"] and q["peak_shell_bytes"] >= 3*m["id_bytes"]*(q["emitted"] > 0),
            "q3 accepted shell capacity")
    par = row["parallel"]
    require(par["terminal_jobs"] <= par["jobs"] and w["peak_edge_buffer_bytes"] ==
            max((x["peak_edge_buffer_bytes"] for x in row["workers_work"]),default=0), "parallel capacity/terminal ledger")
    threshold = row["kmax"] - 1
    if row["q3_census_mode"] == "scalar":
        require(all(value == 0 for value in b.values()) and q["census_shell_sites"] >= q["shell_ids"] and
                q["census_inside_sites"] >= threshold*q["depth_rejections"], "scalar census branch")
        return
    require(all(q[name] == 0 for name in ("census_range_visits", "census_point_tests", "census_inside_sites",
            "census_outside_sites", "census_shell_sites", "early_unread_sites")), "boxes charged scalar work")
    require(b["queries"] == b["preparations"] == q["seeds"] == b["accepted_queries"] + b["rejected_queries"] and
            b["vertex_axes"] == 3*b["queries"] and b["accepted_queries"] == q["emitted"] and
            b["rejected_queries"] == b["count_saturations"] == q["depth_rejections"] and
            b["shell_ids"] == q["shell_ids"], "q3 block query/output partition")
    require(b["count_node_visits"] == b["count_inside_nodes"] + b["count_nonnegative_nodes"] + b["count_split_nodes"] and
            b["count_bounds_prepared"] == b["count_node_visits"] + b["count_prepared_unvisited"] ==
            b["count_box_bound_tests"] + b["count_point_tests"] == b["queries"] + 2*b["count_split_nodes"] and
            b["count_bounds_prepared"] <= b["queries"]*(2*n-1), "q3 first-pass cached bounds partition")
    require(b["count_inside_nodes"] <= b["count_inside_sites"] and
            threshold*b["rejected_queries"] <= b["count_inside_sites"] <=
            threshold*b["rejected_queries"] + (threshold-1)*b["accepted_queries"] and
            b["count_nonnegative_nodes"] <= b["count_nonnegative_sites"] and
            n*b["accepted_queries"] <= b["count_inside_sites"] + b["count_nonnegative_sites"] <= n*b["queries"],
            "q3 strict saturated depth")
    require(b["shell_bounds_prepared"] == b["shell_node_visits"] ==
            b["shell_box_bound_tests"] + b["shell_point_tests"] == b["accepted_queries"] + 2*b["shell_split_nodes"] and
            b["shell_node_visits"] == b["shell_excluded_nodes"] + b["shell_split_nodes"] + b["shell_ids"] and
            b["shell_node_visits"] <= b["accepted_queries"]*(2*n-1) and
            b["shell_ids"] <= b["shell_point_tests"] and b["count_prepared_unvisited"] <= INDEX_STACK_FRAMES*b["rejected_queries"],
            "q3 second pass/contact partition")
    if b["queries"]:
        require(1 <= b["peak_count_stack"] <= INDEX_STACK_FRAMES and b["stack_storage_bytes"] >= 49*(m["id_bytes"]+32) and
                (b["stack_storage_bytes"] % 49 == 0 or b["stack_storage_bytes"] % INDEX_STACK_FRAMES == 0), "q3 fixed cached-bounds stack")
        require((1 <= b["peak_shell_stack"] <= INDEX_STACK_FRAMES) if b["accepted_queries"] else b["peak_shell_stack"] == 0,
                "q3 shell stack activity")
    else:
        require(all(value == 0 for value in b.values()), "unused q3 blocks did work")


def validate_xml(path):
    root = ET.parse(path).getroot()
    cases = root.findall("testcase")
    names = [item.get("name") for item in cases]
    require(root.tag == "testsuite" and root.get("tests") == str(CTEST_COUNT) and
            all(root.get(key) == "0" for key in ("failures", "disabled", "skipped")) and
            len(cases) == len(set(names)) == CTEST_COUNT and set(GATES) <= set(names) and
            all(type(name) is str and name.startswith("mhgp8_") for name in names), "CTest inventory differs")
    require(all(item.get("status") == "run" and all(item.find(tag) is None for tag in
                ("failure", "error", "skipped")) for item in cases), "CTest failed/skipped")
    return sorted(names)


def paired(rows):
    outputs, work = {}, {}
    for row in rows:
        key = (row["scan"], row["input_hash"], row["n"], row["kmax"], row["mask"])
        payload = (row["output"], row.get("records"))
        require(key not in outputs or outputs[key] == payload, "witness/backend/s/workers changed output")
        outputs[key] = payload
        discrete = deepcopy(row["work"])
        discrete["q3"]["peak_shell_bytes"] = 0
        discrete["peak_edge_buffer_bytes"] = 0
        key += (row["s"], row["front_mode"], row["q4_backend"], row["witness_mode"], row["q3_census_mode"])
        value = (row["front"], discrete)
        require(key not in work or work[key] == value, "worker count changed geometric work")
        work[key] = value


def summary(rows):
    paired(rows)
    groups, growth = {}, []
    for row in rows:
        key = tuple(row[field] for field in ("scan", "kmax", "s", "q4_backend", "workers", "witness_mode", "q3_census_mode"))
        groups.setdefault(key, []).append(row)
    def metrics(row):
        return previous.flatten({"front": row["front"]["work"], "work": row["work"],
            "cloud_work": row["cloud_work"], "index_work": row["index_work"],
            "parallel": row["parallel"], "memory": row["memory"], "timings_ms": row["timings_ms"],
            "output": {key: row["output"][key] for key in previous.OUTPUT_FIELDS}})
    for key, sequence in sorted(groups.items()):
        sequence.sort(key=lambda row: row["n"])
        for a, b in zip(sequence, sequence[1:]):
            before, after = metrics(a), metrics(b)
            require(before.keys() == after.keys(), "growth field inventory differs")
            ratios = {field: after[field] / value if value else None for field, value in before.items()}
            threshold = (b["n"] / a["n"]) ** 2
            growth.append(dict(zip(("scan", "kmax", "s", "backend", "workers", "witness_mode", "q3_census_mode"), key)) |
                dict(n=[a["n"], b["n"]], ratios=ratios, quadratic_step=threshold,
                     above_quadratic_step=[field for field, value in ratios.items()
                                           if value is not None and value > threshold]))
    return dict(measurements=len(rows), growth=growth,
                full_contract_qualified=False, universal_subquadratic_claim=False)


def run(args, config):
    build = args.build.resolve()
    require(build.is_dir() and build.is_relative_to(ROOT / "build"), "local build required")
    cache = (build / "CMakeCache.txt").read_text()
    campaign = config["campaign"]
    if campaign == "regression":
        require("CMAKE_BUILD_TYPE:STRING=Release\n" in cache and "MHGP8_SANITIZE:BOOL=OFF\n" in cache,
                "regression requires nonsanitized Release")
    args.output.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix=campaign + "_", dir=args.output)).resolve()
    ctest = str(Path(shutil.which("ctest")).resolve()) if campaign == "regression" and shutil.which("ctest") else None
    names = executable_names(build, campaign)
    artifacts = artifact_paths(build, names)
    sources_before, artifacts_before = pins(SOURCES), pins(artifacts)
    try:
        datasets, files, commands = plan(build, config, output, ctest)
    except BaseException as cause:
        write_json(output / "SETUP_FAILURE.json", dict(status="failed", error=f"{type(cause).__name__}: {cause}",
            config=config, launch_command=[sys.executable, *sys.argv], source_sha256=sources_before,
            artifact_sha256=artifacts_before, finished_utc=utc_stamp()))
        raise
    environment = dict(os.environ)
    def git(*options):
        return subprocess.check_output(["git", *options], cwd=ROOT, text=True).strip()
    manifest = dict(schema=SCHEMA, config=config, build=str(build), started_utc=utc_stamp(),
        launch_command=[sys.executable, *sys.argv], planned_commands=commands, datasets=datasets,
        source_sha256=sources_before, artifact_sha256=artifacts_before, input_sha256=pins(files),
        executable_names=names, ctest=ctest, ctest_sha256=digest(Path(ctest)) if ctest else None,
        environment=edge.environment_record(environment), affinity=sorted(os.sched_getaffinity(0)),
        git_commit=git("rev-parse", "HEAD"), branch=git("branch", "--show-current"), worktree=git("status", "--short"),
        compiler_cache="\n".join(line for line in cache.splitlines() if line.startswith(
            ("CMAKE_CXX_COMPILER", "CMAKE_CXX_FLAGS", "CMAKE_BUILD_TYPE:", "CMAKE_GENERATOR:", "MHGP8_SANITIZE:"))),
        sanitizer_environment={key: environment.get(key) for key in ("ASAN_OPTIONS", "UBSAN_OPTIONS", "TSAN_OPTIONS")},
        gcp_used=False, full_contract_qualified=False, public_status="not_claimed")
    write_json(output / "MANIFEST.json", manifest)
    handlers = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, rows, status, error = [], [], "failed", None
    try:
        for number, (kind, command) in enumerate(commands):
            print(json.dumps(dict(path=str(output), index=number, command=command)), flush=True)
            record = dict(kind=kind, command=command, cwd=str(ROOT), environment=manifest["environment"],
                started_utc=utc_stamp(), status="failed", exit_code=None, stdout="", stderr="",
                stdout_base64="", stderr_base64="")
            try:
                invoke(command, environment, ROOT, record, new_session=True)
                require(record["exit_code"] == 0, "indexed command failed")
                if kind == "ctest":
                    record["test_names"] = validate_xml(output / "result.xml")
                else:
                    require(not record["stderr"], "unexpected gate/probe stderr")
                    row = parse_result(record["stdout"].encode())
                    if kind == "gate":
                        validate_gate(row, Path(command[0]).name)
                    else:
                        validate_row(row, command)
                        dataset = next(d for d in datasets if d["source"] == command[1] and d["n"] == row["n"])
                        require(row["input_hash"] == dataset["input_hash"], "prepared input hash differs")
                        row["scan"] = dataset["scan"]
                        rows.append(row)
                    record["row"] = row
                record["status"] = "passed"
            finally:
                record["finished_utc"] = utc_stamp()
                file = output / f"record_{number:04}.json"
                write_json(file, record)
                records.append(dict(path=file.name, sha256=digest(file)))
        summary(rows)
        status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        errors = []
        def close(label, function):
            try:
                return function()
            except BaseException as cause:
                errors.append(f"{label}: {type(cause).__name__}: {cause}")
                return None
        completion = dict(status=status, error=error, finished_utc=utc_stamp(), records=records,
            manifest_sha256=close("manifest", lambda: digest(output / "MANIFEST.json")),
            source_sha256_after=close("sources", lambda: pins(SOURCES)),
            artifact_sha256_after=close("artifacts", lambda: pins(artifacts)),
            input_sha256_after=close("inputs", lambda: pins(files)),
            executable_names_after=close("executables", lambda: executable_names(build, campaign)),
            ctest_sha256_after=close("ctest", lambda: digest(Path(ctest))) if ctest else None,
            xml_sha256=close("xml", lambda: digest(output / "result.xml")) if (output / "result.xml").is_file() else None,
            closing_errors=errors)
        for key in ("source_sha256", "artifact_sha256", "input_sha256", "executable_names", "ctest_sha256"):
            if errors or manifest[key] != completion[key + "_after"]:
                completion.update(status="failed", error=error or "indexed closure changed")
        write_json(output / "COMPLETION.json", completion)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(path=str(output), status=completion["status"], error=completion["error"])), flush=True)
    require(completion["status"] == "passed", "indexed capture failed")


def manifest_plan(path, manifest, completion):
    require(manifest["schema"] == SCHEMA and completion["status"] == "passed" and
            completion["error"] is None and completion["closing_errors"] == [] and
            completion["manifest_sha256"] == digest(path / "MANIFEST.json"), "capture not closed successfully")
    require(manifest["branch"] == "main" and manifest["gcp_used"] is False and
            manifest["full_contract_qualified"] is False and manifest["public_status"] == "not_claimed" and
            type(manifest["worktree"]) is str and "CMAKE_CXX_COMPILER:" in manifest["compiler_cache"] and
            manifest["environment"]["scope"] == "selected_values_full_environment_fingerprint_no_secrets",
            "indexed provenance differs")
    commit = manifest["git_commit"]
    require(type(commit) is str and len(commit) == 40 and all(v in "0123456789abcdef" for v in commit), "commit")
    build, config = configuration_from_launch(manifest["launch_command"])
    require(build.is_absolute() and build.is_relative_to(ROOT / "build") and ".." not in build.parts and
            manifest["build"] == str(build) and manifest["config"] == config, "launch/config differs")
    names = manifest["executable_names"]
    require(names == sorted(set(names)) and all(type(name) is str and Path(name).name == name and
            name.startswith("mhgp8_") for name in names), "executable names")
    campaign = config["campaign"]
    if campaign == "regression":
        require(set(GATES) <= set(names) and Path(manifest["ctest"]).is_absolute() and
                Path(manifest["ctest"]).name == "ctest" and "CMAKE_BUILD_TYPE:STRING=Release" in manifest["compiler_cache"] and
                "MHGP8_SANITIZE:BOOL=OFF" in manifest["compiler_cache"], "regression provenance")
    else:
        require(names == executable_names(build, campaign) and manifest["ctest"] is None and
                manifest["ctest_sha256"] is None and completion["xml_sha256"] is None, "targeted artifacts")
    datasets, files, commands = plan(build, config, path, manifest["ctest"])
    require(manifest["datasets"] == datasets and manifest["planned_commands"] == [[k, c] for k, c in commands] and
            len(completion["records"]) == len(commands), "complete command plan differs")
    require(set(manifest["source_sha256"]) == SOURCES and set(manifest["artifact_sha256"]) == artifact_paths(build, names) and
            set(manifest["input_sha256"]) == files, "pin inventory differs")
    for key in ("source_sha256", "artifact_sha256", "input_sha256"):
        require(manifest[key] == completion[key + "_after"] and all(type(h) is str and len(h) == 64 and
                all(v in "0123456789abcdef" for v in h) for h in manifest[key].values()), "hash closure differs")
    require(manifest["executable_names"] == completion["executable_names_after"] and
            manifest["ctest_sha256"] == completion["ctest_sha256_after"] and
            pins(files) == manifest["input_sha256"], "input/tool closure differs")
    require({p.name for p in path.glob("record_*.json")} == {f"record_{i:04}.json" for i in range(len(commands))},
            "record inventory differs")
    return build, config, datasets, commands


def read(path, check_live=False):
    path = path.resolve()
    manifest, completion = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
    build, config, datasets, commands = manifest_plan(path, manifest, completion)
    if check_live:
        for key in ("source_sha256", "artifact_sha256", "input_sha256"):
            require(manifest[key] == pins(manifest[key]), "live source/build/input changed")
        require(executable_names(build, config["campaign"]) == manifest["executable_names"] and
                (not manifest["ctest"] or digest(Path(manifest["ctest"])) == manifest["ctest_sha256"]),
                "live executable/CTest changed")
    rows = []
    for number, ((kind, command), item) in enumerate(zip(commands, completion["records"], strict=True)):
        require(item["path"] == f"record_{number:04}.json" and digest(path / item["path"]) == item["sha256"],
                "record hash/path differs")
        record = read_json(path / item["path"])
        require(record["kind"] == kind and record["command"] == command and record["cwd"] == str(ROOT) and
                record["environment"] == manifest["environment"] and record["status"] == "passed" and
                type(record["exit_code"]) is int and record["exit_code"] == 0, "command/result differs")
        for stream in ("stdout", "stderr"):
            require(base64.b64decode(record[stream + "_base64"], validate=True).decode(errors="replace") ==
                    record[stream], "raw/decoded output differs")
        if kind == "ctest":
            require(completion["xml_sha256"] == digest(path / "result.xml") and
                    validate_xml(path / "result.xml") == record["test_names"], "JUnit differs")
        else:
            require(not record["stderr"], "unexpected gate/probe stderr")
            row = parse_result(record["stdout"].encode())
            if kind == "gate":
                validate_gate(row, Path(command[0]).name)
            else:
                validate_row(row, command)
                dataset = next(d for d in datasets if d["source"] == command[1] and d["n"] == row["n"])
                require(row["input_hash"] == dataset["input_hash"], "prepared input hash differs")
                row["scan"] = dataset["scan"]
                rows.append(row)
            require(row == record["row"], "raw/parsed row differs")
    return dict(status="passed", path=str(path), campaign=config["campaign"], qualification=config["qualification"],
        sources=len(SOURCES), commands=len(commands), gates=sum(kind == "gate" for kind, _ in commands),
        tests=CTEST_COUNT if config["campaign"] == "regression" else 0,
        input_files=len(manifest["input_sha256"]), **summary(rows))


def selftest(path):
    result = read(path)
    path = path.resolve()
    manifest, completion = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
    records = [read_json(file) for file in sorted(path.glob("record_*.json"))]
    eligible = [item for item in records if item["kind"] == "probe" and item["row"]["output"]["callbacks"] > 0]
    base = next((item for item in eligible if item["row"]["witness_mode"] == "rectangle-pair"),
                eligible[0] if eligible else None)
    require(base is not None, "reader selftest needs a completed nonempty probe")
    row = deepcopy(base["row"])
    row.pop("scan")
    tests = 0
    def reject(action):
        nonlocal tests
        try:
            action()
        except InvalidReceipt:
            tests += 1
        else:
            raise InvalidReceipt("indexed reader corruption survived")
    def alter(route, value):
        changed = deepcopy(row)
        target = changed
        for key in route[:-1]:
            target = target[key]
        require(target[route[-1]] != value or type(target[route[-1]]) is not type(value), "mutation was a no-op")
        target[route[-1]] = value
        reject(lambda: validate_row(changed, base["command"]))
    for field in ("n", "source_n", "kmax", "s", "mask", "q4_backend", "workers", "input_hash"):
        alter([field], True)
    alter(["witness_mode"], "invalid")
    alter(["q3_census_mode"], "invalid")
    alter(["schema"], previous.FIXED["schema"])
    for field in WITNESS_FIELDS:
        alter(["work", "witness", field], True)
    for section in ("rectangles", "pairs"):
        for field in ("queries", "node_visits", "q3_credits", "peak_stack", "stack_storage_bytes"):
            alter(["work", "witness", section, field], True)
    for field in Q3_BLOCK_FIELDS:
        alter(["work", "q3_blocks", field], True)
    if row["witness_mode"] != "disabled":
        search = row["work"]["witness"]["pairs"]
        for field, value in (("q3_credits", (row["kmax"]-1)*search["q3_queries"]+1),
                             ("midpoint_box_tests", search["midpoint_box_tests"] ^ 1),
                             ("q3_lane_tests", search["xi_bound_tests"]+1), ("peak_stack",50)):
            alter(["work", "witness", "pairs", field], value)
    if row["q3_census_mode"] == "boxes":
        block = row["work"]["q3_blocks"]
        for field, value in (("count_prepared_unvisited", block["count_prepared_unvisited"]+1),
                             ("shell_bounds_prepared", block["shell_bounds_prepared"]+1),
                             ("count_inside_sites", (row["kmax"]-1)*block["queries"]+1),
                             ("peak_count_stack",50)):
            alter(["work", "q3_blocks", field], value)
    for route in (("work", "expanded_pairs"), ("work", "cover_builds"),
                  ("work", "witness", "input_pair_mass"), ("parallel", "completed_jobs"),
                  ("output", "callbacks")):
        target = row
        for key in route:
            target = target[key]
        alter(route, target + 1)
    alter(["timings_ms", "cloud"], float("nan"))
    for route in (("work", "witness"), ("work", "q3_blocks"), ("work", "witness", "pairs", "q4_rejected"),
                  ("memory", "input_capacity_bytes")):
        changed = deepcopy(row)
        target = changed
        for key in route[:-1]:
            target = target[key]
        del target[route[-1]]
        # Missing nested groups are rejected by an explicit shape check below.
        reject(lambda: validate_row(changed, base["command"]))
    if "records" in row:
        alter(["output", "sum"], format(int(row["output"]["sum"], 16) ^ 1, "x"))
        for route, value in ((["records",0,"depth"], True), (["records",0,"shell"], []),
                             (["records",0,"support"], [False,1,2]),
                             (["records",0,"coefficients"], ["01","0","0","0","0"])):
            alter(route, value)
    else:
        changed = deepcopy(base["row"])
        changed["output"]["sum"] = format(int(changed["output"]["sum"],16) ^ 1, "x")
        reject(lambda: paired([base["row"], changed]))
    changed_command = base["command"].copy()
    changed_command[2] = str(int(changed_command[2]) + 1)
    reject(lambda: validate_row(row, changed_command))
    for field, name in (("source_sha256", "invented.cpp"), ("artifact_sha256", "invented.bin"),
                        ("input_sha256", "invented.u16le")):
        changed = deepcopy(manifest)
        changed[field][name] = "0"*64
        reject(lambda: manifest_plan(path, changed, completion))
    changed = deepcopy(completion)
    changed["source_sha256_after"][next(iter(manifest["source_sha256"]))] = "0"*64
    reject(lambda: manifest_plan(path, manifest, changed))
    changed = deepcopy(completion)
    changed["records"] = changed["records"][:-1]
    reject(lambda: manifest_plan(path, manifest, changed))
    changed = deepcopy(manifest)
    changed["planned_commands"] = changed["planned_commands"][:-1]
    reject(lambda: manifest_plan(path, changed, completion))
    reject(lambda: parse_result(b'{"x":NaN}'))
    reject(lambda: parse_result(b'{"x":1,"x":2}'))
    for item in records:
        if item["kind"] == "gate":
            changed = deepcopy(item["row"])
            changed["checks"] = True
            reject(lambda: validate_gate(changed, Path(item["command"][0]).name))
    return dict(status="passed", mutants=tests, real_measurements=result["measurements"],
                scope="receipt_reader_not_geometry", changes_actual_values=True)


def add_read_parsers(sub):
    reader = sub.add_parser("read")
    reader.add_argument("path", type=Path)
    reader.add_argument("--check-live", action="store_true")
    reader.add_argument("--compact", action="store_true")
    unit = sub.add_parser("selftest")
    unit.add_argument("path", type=Path)


def make_parser():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    run = sub.add_parser("run")
    run.add_argument("--build", type=Path, required=True)
    run.add_argument("--output", type=Path, required=True)
    run.add_argument("--campaign", choices=("gate", "smoke", "regression"), required=True)
    run.add_argument("--qualification", choices=("preflight", "candidate"), default="preflight")
    run.add_argument("--q3-census-mode", choices=("scalar", "boxes"), default="boxes")
    add_read_parsers(sub)
    return parser


def main():
    args = make_parser().parse_args()
    if args.operation == "run":
        run(args, configuration(args))
    else:
        result = read(args.path, args.check_live) if args.operation == "read" else selftest(args.path)
        if getattr(args, "compact", False):
            result = {key: value for key, value in result.items() if key != "growth"}
        print(json.dumps(result, sort_keys=True, allow_nan=False))


if __name__ == "__main__":
    main()
