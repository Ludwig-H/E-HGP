#!/usr/bin/env python3
"""Tranche33 affine witness bounds: new verdicts, frozen32 capture machinery.

The private protocol instance reuses capture, raw-byte preservation, pin closure,
and historical-reader mechanics only. v3 row validation is explicitly ported
from32, with actual new counters; no transformed row is an earlier execution.
"""
from __future__ import annotations
import argparse
from copy import deepcopy
import importlib.util
import json
import math
from pathlib import Path
import sys

import run_q34_indexed_checks as base
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import InvalidReceipt, parse_result, require, uint

ROOT = base.ROOT
previous, edge = base.previous, base.edge
MODES = base.MODES
BOUNDS_MODES = ("legacy", "exclude", "affine")
SCHEMA = "mhgp8_q34_affine_capture_v1"
SOURCES = base.SOURCES | frozenset({
    "morsehgp3D_v8/src/lanes/q34_pair_bounds.hpp",
    "morsehgp3D_v8/tests/q34_pair_bounds_gate.cpp",
    "morsehgp3D_v8/tests/q34_affine_mutations.py",
    "morsehgp3D_v8/bench/run_q34_affine_checks.py",
    "morsehgp3D_v8/bench/run_q34_affine_lidar.py",
})
GATES = ("mhgp8_q34_pair_bounds_gate", *base.GATES)
CTEST_COUNT = 95
WITNESS_FIELDS, SEARCH_FIELDS, Q3_BLOCK_FIELDS = base.WITNESS_FIELDS, base.SEARCH_FIELDS, base.Q3_BLOCK_FIELDS
BOUNDS_FIELDS = ("queries pair_preparations general_preparations affine_h_tests affine_xi_tests "
    "xi_on_nonpositive_minimum q3_exclusion_tests q4_exclusion_tests q3_excluded_nodes q4_excluded_nodes "
    "fully_excluded_nodes mixed_terminal_nodes").split()
# Explicit gate inventories from the three constructor33 judges.
PAIR_GATE_FIELDS = ("checks queries corner_evaluations sample_evaluations singleton_boxes nondegenerate_boxes "
    "half_integer_summits cross_zero_components positive_xi_lower wide_xi wide_h_squares reversed_pairs "
    "coincident_pairs extreme_cases axis_permutations invalid_inputs allocation_free_calls parallel_calls").split()
SEARCH_GATE_ADDED = ("bounds_calls legacy_overload_calls exclusion_calls affine_calls mode_singletons mode_rectangles "
    "local_exclusion_cases nonpositive_minimum_cases mixed_terminal_cases mode_parallel_calls mode_invalid_inputs "
    "mode_repeated_calls mode_overflows affine_preparations general_preparations excluded_q3_nodes excluded_q4_nodes "
    "fully_excluded_nodes mode_xi_nonpositive").split()
GLOBAL_GATE_ADDED = ("bounds_mode_calls bounds_exclusion_calls bounds_affine_calls bounds_work_checks "
    "bounds_parallel_calls bounds_invalid_inputs bounds_inactive_calls bounds_callback_failures "
    "bounds_allocation_failures bounds_shared_calls bounds_owner_resets").split()

_spec = importlib.util.spec_from_file_location("_mhgp8_affine_capture_protocol",
    Path(base.__file__).resolve())
require(_spec is not None and _spec.loader is not None, "missing frozen capture protocol")
protocol = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(protocol)
_protocol_selftest = protocol.selftest
validate_auxiliary = base.validate_auxiliary


def validate_matrix(matrix):
    require(type(matrix) is dict and "witness_bounds_modes" in matrix, "missing bounds-mode matrix")
    base.validate_matrix({k: v for k, v in matrix.items() if k != "witness_bounds_modes"})
    modes = matrix["witness_bounds_modes"]
    require(type(modes) is list and modes and len(modes) == len(set(modes)) and
            all(type(v) is str and v in BOUNDS_MODES for v in modes), "invalid bounds modes")


def configuration(args):
    matrix = None
    if args.campaign == "smoke":
        matrix = dict(scans=[0], sizes=[64], kmax=[5, 10], s=[8], workers=[1, 4], backends=[28],
            witness_modes=["pair", "rectangle-pair"], witness_bounds_modes=list(BOUNDS_MODES),
            payload="records", q3_census_mode=args.q3_census_mode)
    return dict(campaign=args.campaign, qualification=args.qualification, matrix=matrix)


def configuration_from_launch(command):
    require(type(command) is list and len(command) >= 3 and all(type(v) is str for v in command) and
            command[2] == "run", "invalid affine launch command")
    name = Path(command[1]).name
    if name == "run_q34_affine_lidar.py":
        import run_q34_affine_lidar as lidar
        parser, configure = lidar.make_parser(), lidar.configuration
    else:
        require(name == "run_q34_affine_checks.py", "unrecognized affine launcher")
        parser, configure = make_parser(), configuration
    try:
        args = parser.parse_args(command[2:])
    except SystemExit:
        raise InvalidReceipt("unreadable affine launch configuration")
    require(args.operation == "run", "launch is not a run")
    return args.build.resolve(), configure(args)


def plan(build, config, capture, ctest=None):
    require(type(config) is dict and set(config) == {"campaign", "qualification", "matrix"} and
            config["qualification"] in ("preflight", "candidate"), "invalid affine configuration")
    campaign = config["campaign"]
    require(campaign in ("gate", "smoke", "regression", "lidar"), "invalid affine campaign")
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
            q3_census_mode=matrix["q3_census_mode"]))["matrix"], "smoke matrix differs")
    common = {k: v for k, v in matrix.items() if k not in ("witness_modes", "q3_census_mode", "witness_bounds_modes")}
    datasets, native, files = previous.prepare_matrix(build, common)
    commands.extend(("probe", [*command, mode, matrix["q3_census_mode"], bounds])
        for command in native for mode in matrix["witness_modes"] for bounds in matrix["witness_bounds_modes"])
    return datasets, files, commands


def validate_gate(row, name):
    if name == "mhgp8_q3_ball_census_gate":
        return base.validate_gate(row, name)
    if name == "mhgp8_q34_pair_bounds_gate":
        previous.exact_fields(row, [*PAIR_GATE_FIELDS, "schema", "status"], "pair bounds gate")
        require(row["schema"] == "mhgp8_q34_pair_bounds_gate_v1" and row["status"] == "PASS", "pair gate identity")
        for field in PAIR_GATE_FIELDS:
            require(uint(row[field], field) > 0, "pair bounds gate vacuity")
        exact = dict(extreme_cases=50, axis_permutations=1, invalid_inputs=6, allocation_free_calls=1, parallel_calls=4)
        require(all(row[k] == v for k, v in exact.items()) and
                row["queries"] == row["singleton_boxes"] + row["nondegenerate_boxes"] == row["reversed_pairs"] and
                row["corner_evaluations"] == 8*row["queries"] and row["sample_evaluations"] == 27*row["queries"],
                "pair bounds gate fixture counts")
        return
    if name == "mhgp8_q34_witness_search_gate":
        previous.exact_fields(row, [*base.SEARCH_GATE_FIELDS, *SEARCH_GATE_ADDED, "schema", "status"], "witness gate33")
        require(row["schema"] == "mhgp8_q34_witness_search_gate_v1" and row["status"] == "PASS", "witness gate identity")
        for field in (*base.SEARCH_GATE_FIELDS, *SEARCH_GATE_ADDED):
            require(uint(row[field], field) > 0, "affine witness gate vacuity")
        exact = dict(parallel_calls=4, permutations=1, source_alias_checks=1, repeated_calls=1, left_tie_cases=1,
            partial_admission_cases=1, deep_index_cases=1, peak_stack=49, overflow_exceptions=1, q3_contacts=1,
            q4_contacts=1, wrong_alpha_cases=1, positive_q4_external=1, h_contacts=1, extreme_queries=15,
            invalid_inputs=11, point_admission_cases=1, mode_parallel_calls=4, mode_invalid_inputs=2,
            mode_repeated_calls=1, mode_overflows=1, local_exclusion_cases=2, nonpositive_minimum_cases=2,
            mixed_terminal_cases=2)
        require(all(row[k] == v for k, v in exact.items()) and row["bounds_calls"] ==
                row["legacy_overload_calls"] + row["exclusion_calls"] + row["affine_calls"] ==
                row["mode_singletons"] + row["mode_rectangles"], "affine witness gate fixture counts")
        return
    require(name == "mhgp8_wspd_q34_gate", "unknown affine gate")
    fields = [*edge.GLOBAL_GATE_FIELDS, *base.GLOBAL_GATE_ADDED, *GLOBAL_GATE_ADDED]
    previous.exact_fields(row, [*fields, "schema", "status"], "global gate33")
    require(row["schema"] == "mhgp8_wspd_q34_gate_v1" and row["status"] == "PASS", "global gate identity")
    for field in fields:
        require(uint(row[field], field) > 0, "affine global gate vacuity")
    exact = dict(allocation_failures=4, callback_failures=1, parallel_calls=4, nested_calls=1,
        owner_reset_calls=1, input_alias_checks=1, parallel_callback_failures=1,
        parallel_owner_resets=1, parallel_join_checks=2, parallel_empty_calls=8,
        bounds_parallel_calls=24, bounds_invalid_inputs=4, bounds_inactive_calls=6,
        bounds_callback_failures=2, bounds_allocation_failures=4, bounds_shared_calls=4, bounds_owner_resets=1)
    require(all(row[k] == v for k, v in exact.items()) and
            row["candidates"] == row["q3"] + row["q4"] and row["max_shell"] >= 30 and
            row["bounds_mode_calls"] == row["bounds_work_checks"] ==
            row["bounds_exclusion_calls"] + row["bounds_affine_calls"], "affine global gate lifecycle differs")


def validate_search(search, bounds, n, k, id_bytes, mode, section):
    edge.counts(search, SEARCH_FIELDS, "affine witness search")
    edge.counts(bounds, BOUNDS_FIELDS, "affine bounds work")
    s, b = search, bounds
    if mode == "legacy":
        require(all(v == 0 for v in b.values()), "legacy bounds charged new work")
        return base.validate_search(s, n, k, id_bytes)
    require(max(s["q3_queries"], s["q4_queries"]) <= s["queries"] <= s["q3_queries"] + s["q4_queries"] and
            s["prepared_bounds"] == b["queries"] == s["queries"] and
            b["pair_preparations"] + b["general_preparations"] == b["queries"], "bounds preparation ledger")
    if mode == "exclude":
        require(b["pair_preparations"] == b["affine_h_tests"] == b["affine_xi_tests"] == 0,
                "exclusion-only mode used affine bounds")
    if mode == "affine" and section == "pairs":
        require(b["general_preparations"] == 0 and b["pair_preparations"] == b["queries"],
                "singleton pair did not use affine preparation")
        require(b["affine_h_tests"] == s["h_bound_tests"] and b["affine_xi_tests"] == s["xi_bound_tests"],
                "affine pair bound tests unpaid")
    require(b["affine_h_tests"] <= s["h_bound_tests"] and b["affine_xi_tests"] <= s["xi_bound_tests"] and
            b["xi_on_nonpositive_minimum"] <= s["xi_bound_tests"], "bound-test accounting")
    require(s["node_visits"] == s["h_bound_tests"] == s["h_excluded_nodes"] + s["fully_admitted_nodes"] +
            s["leaf_remainders"] + s["split_nodes"] + b["fully_excluded_nodes"] + b["mixed_terminal_nodes"] and
            s["midpoint_box_tests"] == 2*s["split_nodes"] and
            s["leaf_remainders"] <= s["point_tests"] <= s["node_visits"] <= s["queries"]*(2*n-1),
            "affine node partition")
    admission_xi = s["xi_bound_tests"] - b["xi_on_nonpositive_minimum"]
    require(s["fully_admitted_nodes"] <= s["admitted_nodes"] <= admission_xi and
            s["xi_bound_tests"] == s["node_visits"] - s["h_excluded_nodes"] and
            max(s["q3_lane_tests"], s["q4_lane_tests"]) <= admission_xi and
            s["q3_lane_tests"] + s["q4_lane_tests"] <= 2*admission_xi and
            max(s["q3_admitted_nodes"], s["q4_admitted_nodes"]) <= s["admitted_nodes"] <=
            s["q3_admitted_nodes"] + s["q4_admitted_nodes"], "separate admission/Xi ledger")
    require(max(b["q3_exclusion_tests"], b["q4_exclusion_tests"]) <= s["xi_bound_tests"] <=
            b["q3_exclusion_tests"] + b["q4_exclusion_tests"], "exclusion lane tests unpaid")
    require(b["fully_excluded_nodes"] + b["mixed_terminal_nodes"] <=
            b["q3_excluded_nodes"] + b["q4_excluded_nodes"] and
            b["mixed_terminal_nodes"] <= s["admitted_nodes"], "terminal excluded/mixed classifications")
    for lane, threshold in (("q3", k-1), ("q4", k-2)):
        require(b[lane+"_excluded_nodes"] <= b[lane+"_exclusion_tests"] and
                s[lane+"_admitted_nodes"] <= s[lane+"_lane_tests"] and
                s[lane+"_admitted_nodes"] <= s[lane+"_credits"] and
                s[lane+"_rejected"] <= s[lane+"_queries"] and
                threshold*s[lane+"_rejected"] <= s[lane+"_credits"] <=
                threshold*s[lane+"_rejected"] + (threshold-1)*(s[lane+"_queries"]-s[lane+"_rejected"]),
                "independent saturated lane credits")
    require(s["node_visits"] + s["pending_nodes_skipped"] <= s["queries"] + 2*s["split_nodes"],
            "search pending frames ledger")
    if s["queries"]:
        require(1 <= s["peak_stack"] <= 49 and s["stack_storage_bytes"] == 49*2*id_bytes,
                "fixed u16 witness stack")
    else:
        require(all(v == 0 for v in s.values()) and all(v == 0 for v in b.values()), "inactive search did work")


def validate_row(row, command):
    require(type(command) is list and len(command) == 13 and all(type(v) is str for v in command) and
            command[10] in MODES and type(row) is dict and row.get("schema") == "mhgp8_wspd_q34_probe_v3" and
            type(row.get("witness_mode")) is str and row["witness_mode"] == command[10] and
            type(row.get("q3_census_mode")) is str and row["q3_census_mode"] == command[11] and
            command[11] in ("scalar", "boxes") and command[12] in BOUNDS_MODES and
            type(row.get("witness_bounds_mode")) is str and row["witness_bounds_mode"] == command[12],
            "affine command/schema/mode")
    # Shape-only view of unchanged fields, not a synthetic old execution. The
    # changed expansion/lane partitions are validated below on the actual row.
    require(type(row.get("work")) is dict and {"witness", "q3_blocks"} <= set(row["work"]), "missing indexed work")
    common = deepcopy(row)
    common.pop("witness_mode")
    common.pop("q3_census_mode")
    common.pop("witness_bounds_mode")
    common["schema"] = previous.FIXED["schema"]
    witness = common["work"].pop("witness")
    blocks = common["work"].pop("q3_blocks")
    previous.strict_shape(common, command[:10])
    edge.structural(witness, WITNESS_FIELDS, ("rectangles", "pairs", "rectangles_bounds", "pairs_bounds"), "witness")
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
    for section in ("rectangles", "pairs"):
        validate_search(z[section], z[section + "_bounds"], n, k, row["memory"]["id_bytes"], command[12], section)
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



def paired(rows):
    outputs, work, downstream = {}, {}, {}
    for row in rows:
        key = (row["scan"], row["input_hash"], row["n"], row["kmax"], row["mask"])
        payload = (row["output"], row.get("records"))
        require(key not in outputs or outputs[key] == payload, "bounds/backend/s/workers changed output")
        outputs[key] = payload
        discrete = deepcopy(row["work"])
        discrete["q3"]["peak_shell_bytes"] = 0
        discrete["peak_edge_buffer_bytes"] = 0
        geometry_key = key + (row["s"], row["front_mode"], row["q4_backend"], row["witness_mode"], row["q3_census_mode"])
        value = (row["front"], discrete)
        profile_key = geometry_key + (row["witness_bounds_mode"],)
        require(profile_key not in work or work[profile_key] == value, "workers changed geometric work")
        work[profile_key] = value
        # Pair searches are exact in all three profiles. Stronger conservative
        # rectangle proofs can change expansion/pre-filter counts, not final
        # surviving edges or their downstream work on identical inputs.
        residue = {name: value for name, value in discrete.items() if name not in ("witness", "expanded_pairs")}
        require(geometry_key not in downstream or downstream[geometry_key] == residue,
                "bounds mode changed final residual/downstream geometry")
        downstream[geometry_key] = residue


def summary(rows):
    paired(rows)
    groups, growth = {}, []
    fields = ("scan", "kmax", "s", "q4_backend", "workers", "witness_mode", "q3_census_mode", "witness_bounds_mode")
    for row in rows:
        groups.setdefault(tuple(row[name] for name in fields), []).append(row)
    def metrics(row):
        return previous.flatten({"front": row["front"]["work"], "work": row["work"],
            "cloud_work": row["cloud_work"], "index_work": row["index_work"], "parallel": row["parallel"],
            "memory": row["memory"], "timings_ms": row["timings_ms"],
            "output": {key: row["output"][key] for key in previous.OUTPUT_FIELDS}})
    for key, sequence in sorted(groups.items()):
        sequence.sort(key=lambda row: row["n"])
        for a, b in zip(sequence, sequence[1:]):
            require(a["n"] < b["n"], "duplicate configuration cannot define growth")
            before, after = metrics(a), metrics(b)
            require(before.keys() == after.keys(), "growth field inventory differs")
            ratios = {field: after[field]/value if value else None for field, value in before.items()}
            threshold = (b["n"]/a["n"])**2
            growth.append(dict(zip(fields, key)) | dict(n=[a["n"], b["n"]], ratios=ratios,
                quadratic_step=threshold, above_quadratic_step=[name for name, value in ratios.items()
                    if value is not None and value > threshold]))
    return dict(measurements=len(rows), growth=growth,
                full_contract_qualified=False, universal_subquadratic_claim=False)


def selftest(path):
    inherited = _protocol_selftest(path)
    records = [read_json(p) for p in sorted(path.glob("record_*.json"))]
    eligible = [r for r in records if r["kind"] == "probe" and r["row"]["witness_bounds_mode"] == "affine"
                and r["row"]["witness_mode"] == "rectangle-pair" and r["row"]["output"]["callbacks"] > 0]
    require(eligible, "affine reader tests require a real nonempty affine row")
    original = eligible[0]
    row = deepcopy(original["row"])
    row.pop("scan")
    tests = 0
    def reject(action):
        nonlocal tests
        try:
            action()
        except InvalidReceipt:
            tests += 1
        else:
            raise InvalidReceipt("affine receipt corruption survived")
    def alter(route, value):
        changed = deepcopy(row)
        target = changed
        for key in route[:-1]:
            target = target[key]
        require(target[route[-1]] != value or type(target[route[-1]]) is not type(value), "no-op mutation")
        target[route[-1]] = value
        reject(lambda: validate_row(changed, original["command"]))
    alter(["witness_bounds_mode"], "unknown")
    for section in ("rectangles_bounds", "pairs_bounds"):
        for field in BOUNDS_FIELDS:
            alter(["work", "witness", section, field], True)
        for field in ("queries", "fully_excluded_nodes", "mixed_terminal_nodes"):
            alter(["work", "witness", section, field], row["work"]["witness"][section][field]+1)
        changed = deepcopy(row)
        del changed["work"]["witness"][section]
        reject(lambda: validate_row(changed, original["command"]))
    altered = original["command"].copy()
    altered[12] = "legacy"
    reject(lambda: validate_row(row, altered))
    changed = deepcopy(row)
    changed["witness_bounds_mode"] = "legacy"
    reject(lambda: validate_row(changed, altered))
    return dict(inherited, mutants=inherited["mutants"]+tests, inherited32_reader_tests=inherited["mutants"],
                new_bounds_reader_tests=tests)


def add_read_parsers(sub):
    base.add_read_parsers(sub)


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


# Only this private module instance is configured. Importers of frozen32 keep
# the original validators and inventory. The input source file remains pinned.
for _name in ("SCHEMA", "SOURCES", "GATES", "CTEST_COUNT", "validate_matrix", "configuration",
              "configuration_from_launch", "plan", "validate_gate", "validate_row", "paired", "summary"):
    setattr(protocol, _name, globals()[_name])
run, read = protocol.run, protocol.read


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
