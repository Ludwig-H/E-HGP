#!/usr/bin/env python3
"""Constructor34: actual seed/cell traversal, explicit new geometric ledgers.

The private collector instance reuses frozen32 capture/closure mechanics, not
its verdicts. Unchanged schema3 fields retain their real values; Joined's
distinct seed, family and incidence counts are never rewritten as old work.
"""
from __future__ import annotations
import argparse
from copy import deepcopy
import importlib.util
import json
import math
from pathlib import Path

import run_q34_affine_checks as base
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import InvalidReceipt, parse_result, require, uint

ROOT = base.ROOT
previous, edge = base.previous, base.edge
MODES, BOUNDS_MODES = base.MODES, base.BOUNDS_MODES
SEED_MODES = ("individual", "live", "joined")
SCHEMA = "mhgp8_q4_seed_cells_capture_v1"
SOURCES = base.SOURCES | frozenset({
    "morsehgp3D_v8/src/lanes/q4_seed_cells.hpp",
    "morsehgp3D_v8/tests/q4_seed_cells_gate.cpp",
    "morsehgp3D_v8/tests/q4_seed_cells_mutations.py",
    "morsehgp3D_v8/bench/run_q4_seed_cells_checks.py",
    "morsehgp3D_v8/bench/run_q4_seed_cells_lidar.py",
})
GATES = ("mhgp8_q4_seed_cells_gate", "mhgp8_wspd_q34_gate",
         "mhgp8_q4_local_gate", "mhgp8_q34_pair_bounds_gate")
CTEST_COUNT = 96
WITNESS_FIELDS, SEARCH_FIELDS = base.WITNESS_FIELDS, base.SEARCH_FIELDS
Q3_BLOCK_FIELDS, BOUNDS_FIELDS = base.Q3_BLOCK_FIELDS, base.BOUNDS_FIELDS
SUM_FIELDS = ("queries live_preparations live_node_visits live_child_reads live_leaves whole_atlas_skips "
    "live_skipped_nodes antichain_node_visits antichain_splits blocks block_sites cache_entries_initialized "
    "cache_hits cache_misses invalid_cache_hits family_preparations family_cache_hits form_preparations "
    "product_visits product_seed_rejections positive_products negative_products uncertain_products "
    "zero_bound_products block_bound_tests singleton_bound_tests spatial_tests_reused x_splits cell_splits "
    "terminal_pairs").split()
MAX_FIELDS = ("max_block_sites peak_cache_bytes peak_live_bytes peak_product_stack product_stack_bytes "
              "peak_auxiliary_bytes peak_total_buffer_bytes").split()
SEED_FIELDS = [*SUM_FIELDS, *MAX_FIELDS]
# Explicit inventories of the constructor34 rational and global judges.
SEED_GATE_FIELDS = ("checks bounds_queries bounds_corners bounds_site_checks bounds_rounding bounds_wide bounds_contacts "
    "interior_minimum_cases axis_permutations extreme_cases edge_calls reference_calls oracle_completions oracle_sites "
    "candidates q4 max_shell individual_calls live_calls product_calls work_checks cache_hits all_dead_cases dead_branch_cases "
    "live_skips positive_rejections negative_rejections seed_splits cell_splits terminal_pairs family_builds max_stack "
    "cached_fixture obtuse_completions q4_without_q3 inactive_calls exhaustive_edges permutations invalid_inputs "
    "allocation_failures late_allocation_failures callback_failures parallel_calls owner_resets").split()
GLOBAL_GATE_ADDED = ("seed_cell_calls seed_cell_live_calls seed_cell_joined_calls seed_cell_work_checks seed_cell_parallel_calls "
    "seed_cell_inactive_calls seed_cell_invalid_inputs seed_cell_callback_failures seed_cell_allocation_failures "
    "seed_cell_shared_calls seed_cell_owner_resets seed_cell_parallel_failures").split()

_spec = importlib.util.spec_from_file_location("_mhgp8_seed_cells_capture_protocol", Path(base.base.__file__).resolve())
require(_spec is not None and _spec.loader is not None, "missing frozen capture protocol")
protocol = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(protocol)
_protocol_selftest = protocol.selftest
validate_search, validate_auxiliary = base.validate_search, base.validate_auxiliary


def validate_matrix(matrix):
    require(type(matrix) is dict and {"q4_seed_modes", "q4_seed_block_sizes"} <= set(matrix), "missing seed/cell matrix")
    base.validate_matrix({k: v for k, v in matrix.items() if k not in ("q4_seed_modes", "q4_seed_block_sizes")})
    modes, grains = matrix["q4_seed_modes"], matrix["q4_seed_block_sizes"]
    require(type(modes) is list and modes and all(type(v) is str and v in SEED_MODES for v in modes) and
            len(modes) == len(set(modes)), "invalid/duplicate seed modes")
    require(type(grains) is list and grains and all(type(v) is int and 0 < v < (1 << 64) for v in grains) and
            len(grains) == len(set(grains)), "invalid/duplicate cache grains")
    require(30 not in matrix["backends"] or modes == ["individual"], "Window30 cannot use local seed/cell traversal")


def configuration(args):
    matrix = None
    if args.campaign == "smoke":
        matrix = dict(scans=[0], sizes=[64, 128], kmax=[5, 10], s=[8], workers=[1, 4], backends=[28],
            witness_modes=["rectangle-pair"], witness_bounds_modes=["affine"], payload="records",
            q3_census_mode=args.q3_census_mode, q4_seed_modes=list(SEED_MODES), q4_seed_block_sizes=[64])
    return dict(campaign=args.campaign, qualification=args.qualification, matrix=matrix)


def configuration_from_launch(command):
    require(type(command) is list and len(command) >= 3 and all(type(v) is str for v in command) and
            command[2] == "run", "invalid seed/cell launch command")
    name = Path(command[1]).name
    if name == "run_q4_seed_cells_lidar.py":
        import run_q4_seed_cells_lidar as lidar
        parser, configure = lidar.make_parser(), lidar.configuration
    else:
        require(name == "run_q4_seed_cells_checks.py", "unrecognized seed/cell launcher")
        parser, configure = make_parser(), configuration
    try:
        args = parser.parse_args(command[2:])
    except SystemExit:
        raise InvalidReceipt("unreadable seed/cell launch configuration")
    require(args.operation == "run", "launch is not a run")
    return args.build.resolve(), configure(args)


def plan(build, config, capture, ctest=None):
    require(type(config) is dict and set(config) == {"campaign", "qualification", "matrix"} and
            config["qualification"] in ("preflight", "candidate"), "invalid seed/cell configuration")
    campaign = config["campaign"]
    require(campaign in ("gate", "smoke", "regression", "lidar"), "invalid seed/cell campaign")
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
    omitted = ("witness_modes", "witness_bounds_modes", "q3_census_mode", "q4_seed_modes", "q4_seed_block_sizes")
    datasets, native, files = previous.prepare_matrix(build, {k: v for k, v in matrix.items() if k not in omitted})
    commands.extend(("probe", [*command, witness, matrix["q3_census_mode"], bounds, mode, str(grain)])
        for command in native for witness in matrix["witness_modes"] for bounds in matrix["witness_bounds_modes"]
        for mode in matrix["q4_seed_modes"] for grain in matrix["q4_seed_block_sizes"])
    return datasets, files, commands


def validate_gate(row, name):
    if name == "mhgp8_q34_pair_bounds_gate":
        return base.validate_gate(row, name)
    if name == "mhgp8_q4_local_gate":
        previous.exact_fields(row, [*edge.local.GATE_FLOORS, "schema", "status"], "local28 gate")
        require(row["schema"] == "mhgp8_q4_local_gate_v1" and row["status"] == "passed", "local28 gate identity")
        for key, floor in edge.local.GATE_FLOORS.items():
            require(uint(row[key], key) >= floor, "local28 gate vacuity")
        return
    if name == "mhgp8_wspd_q34_gate":
        require(GLOBAL_GATE_ADDED, "global34 gate inventory is not yet frozen")
        previous.exact_fields(row, [*edge.GLOBAL_GATE_FIELDS, *base.base.GLOBAL_GATE_ADDED,
            *base.GLOBAL_GATE_ADDED, *GLOBAL_GATE_ADDED, "schema", "status"], "global34 gate")
        unchanged = {k: v for k, v in row.items() if k not in GLOBAL_GATE_ADDED}
        base.validate_gate(unchanged, name)
        for field in GLOBAL_GATE_ADDED:
            require(uint(row[field], field) > 0, "global34 gate vacuity")
        exact = dict(seed_cell_calls=10, seed_cell_live_calls=5, seed_cell_joined_calls=5, seed_cell_work_checks=10,
            seed_cell_parallel_calls=12, seed_cell_inactive_calls=6, seed_cell_invalid_inputs=12,
            seed_cell_callback_failures=2, seed_cell_allocation_failures=4, seed_cell_shared_calls=4,
            seed_cell_owner_resets=1, seed_cell_parallel_failures=1)
        require(all(row[k] == v for k, v in exact.items()), "global34 lifecycle fixtures")
        return
    require(name == "mhgp8_q4_seed_cells_gate" and SEED_GATE_FIELDS, "seed/cell gate inventory is not yet frozen")
    previous.exact_fields(row, [*SEED_GATE_FIELDS, "schema", "status"], "seed/cell gate")
    require(row["schema"] == "mhgp8_q4_seed_cells_gate_v1" and row["status"] == "PASS", "seed/cell gate identity")
    for field in SEED_GATE_FIELDS:
        require(uint(row[field], field) > 0, "seed/cell gate vacuity")
    require(row["candidates"] == row["q4"] and row["max_shell"] >= 30 and
            row["edge_calls"] == 3*row["reference_calls"] == row["individual_calls"] + row["live_calls"] + row["product_calls"] and
            row["work_checks"] + row["inactive_calls"] == row["live_calls"] + row["product_calls"] and
            row["max_stack"] <= 181 and row["allocation_failures"] == 4 and row["callback_failures"] == 1 and
            row["late_allocation_failures"] == 2 and row["parallel_calls"] == 4 and row["owner_resets"] == 1,
            "seed/cell gate geometry/lifecycle")


def validate_seed_cells(row):
    w, mode = row["work"], row["q4_seed_mode"]
    extra = w["q4_seed_cells"]
    edge.counts(extra, SEED_FIELDS, "q4 seed/cell work")
    require(row["q4_backend"] == 28 or mode == "individual", "non-Individual Window30")
    if row["q4_backend"] == 28:
        validate_local_payload(row)
    if mode == "individual":
        require(all(value == 0 for value in extra.values()), "Individual charged new seed/cell work")
        return
    require(row["q4_backend"] == 28, "non-Individual Window30")
    e, a, s = (w["local28"][name] for name in ("edge", "atlas", "sweep"))
    require(extra["queries"] == extra["live_preparations"] + extra["whole_atlas_skips"] == w["q4_edges"] and
            extra["live_node_visits"] <= a["cells_created"] and extra["live_child_reads"] <= 4*a["splits"] and
            extra["live_child_reads"] % 4 == 0 and
            extra["live_leaves"] == a["leaf_cells"] and
            extra["live_preparations"] <= extra["live_node_visits"],
            "paid immutable live summary")
    if extra["whole_atlas_skips"] == 0:
        require(extra["live_node_visits"] == a["cells_created"] and extra["live_child_reads"] == 4*a["splits"],
                "nonempty atlas summary omitted cells")
    if extra["live_preparations"] == 0:
        require(extra["live_node_visits"] == extra["live_child_reads"] == extra["live_leaves"] == 0,
                "dead atlas charged summary reads")
    require(e["acute_seeds"] == e["seeds"] + e["owner_rejections"] and
            e["acute_seeds"] <= e["point_tests"] and
            e["acute_seeds"] <= e["owner_tests"] <= 2*e["acute_seeds"] and
            e["node_visits"] == e["bound_tests"] + e["point_tests"], "actual seed certification")
    require(extra["peak_total_buffer_bytes"] == e["peak_live_buffer_bytes"] and
            extra["peak_total_buffer_bytes"] >= max(extra["peak_auxiliary_bytes"], a["peak_build_bytes"], s["peak_buffer_bytes"]) and
            extra["peak_auxiliary_bytes"] >= max(extra["peak_cache_bytes"], extra["peak_live_bytes"], extra["product_stack_bytes"]),
            "coupled real capacity peaks")
    if mode == "live":
        joined = set(SEED_FIELDS) - {"queries", "live_preparations", "live_node_visits", "live_child_reads", "live_leaves",
            "whole_atlas_skips", "live_skipped_nodes", "family_preparations", "form_preparations",
            "peak_live_bytes", "peak_auxiliary_bytes", "peak_total_buffer_bytes"}
        require(all(extra[key] == 0 for key in joined), "LiveOnly charged Joined work")
        require(e["seeds"] == s["seed_queries"] == extra["family_preparations"] == extra["form_preparations"] and
                s["seed_owner_tests"] == 2*s["seed_queries"] and s["seed_owner_rejections"] == 0 and
                e["bound_tests"] == e["rejected_nodes"] + e["split_nodes"] and
                e["rejected_sites"] + e["point_tests"] == row["n"]*(extra["queries"]-extra["whole_atlas_skips"]) and
                s["line_tests"] + extra["live_skipped_nodes"] == s["query_visits"], "LiveOnly seed/family count")
        return
    require(extra["product_visits"] == extra["live_skipped_nodes"] + extra["product_seed_rejections"] +
            extra["positive_products"] + extra["negative_products"] + extra["uncertain_products"] and
            extra["uncertain_products"] == extra["x_splits"] + extra["cell_splits"] + extra["terminal_pairs"] and
            extra["terminal_pairs"] == extra["family_preparations"] + extra["family_cache_hits"] == s["leaf_queries"],
            "Joined product/terminal partition")
    require(extra["cache_misses"] == e["point_tests"] and extra["invalid_cache_hits"] <= extra["cache_hits"] and
            extra["family_preparations"] == s["seed_queries"] <= extra["form_preparations"] == e["seeds"] and
            extra["cache_misses"] <= extra["cache_entries_initialized"] == extra["block_sites"] and
            extra["family_cache_hits"] <= extra["cache_hits"] and
            extra["blocks"] <= extra["block_sites"] <= row["n"]*(extra["queries"]-extra["whole_atlas_skips"]),
            "lazy cache/family ledger")
    require(e["split_nodes"] == extra["antichain_splits"] + extra["x_splits"] and
            extra["product_visits"] == extra["blocks"] + 2*extra["x_splits"] + 4*extra["cell_splits"] and
            extra["antichain_node_visits"] <= (2*row["n"]-1)*(extra["queries"]-extra["whole_atlas_skips"]) and
            extra["cache_misses"] + extra["cache_hits"] == extra["singleton_bound_tests"] +
            extra["cache_misses"] - e["seeds"] + extra["invalid_cache_hits"], "complete product/cache traversal")
    require(all(s[name] == 0 for name in ("query_visits", "line_tests", "line_skips", "seed_owner_tests", "seed_owner_rejections")),
            "Joined fabricated old root navigation")
    require(extra["zero_bound_products"] <= extra["uncertain_products"] and
            extra["block_bound_tests"] + extra["singleton_bound_tests"] ==
            extra["positive_products"] + extra["negative_products"] + extra["uncertain_products"] and
            extra["max_block_sites"] <= row["q4_seed_block_size"] and extra["peak_product_stack"] <= 181,
            "strict signs/cache grain/product stack")


def validate_local_payload(row):
    """Aggregate identities of the actual unchanged leaf sweep, not an old run."""
    w = row["work"]
    e, a, g, s = (w["local28"][name] for name in ("edge", "atlas", "geometry", "sweep"))
    p = a["partition"]
    require(g["preparations"] == w["q4_edges"] and
            a["cells_created"] == w["q4_edges"] + 4*a["splits"] ==
            a["outside_cells"] + a["deep_cells"] + a["leaf_cells"] + a["splits"] and
            a["leaf_cells"] + a["terminal_deep_cells"] == a["depth_stops"] + a["node_stops"] + a["small_stops"] and
            a["domain"]["disk_tests"] == a["cells_created"] and
            p["root_factories"] + p["child_factories"] == a["cells_created"] - a["outside_cells"],
            "local atlas preparation/topology")
    require(p["node_visits"] == p["block_bound_tests"] + p["point_tests"] + p["budget_unexamined_nodes"] ==
            p["inside_nodes"] + p["outside_nodes"] + p["active_nodes"] + p["z_splits"] and
            p["refine_factories"] == a["terminal_refinements"] and
            p["input_sites"] == p["inside_sites"] + p["outside_sites"] + p["active_sites"] and
            p["frontier_ids_copied"] == p["active_nodes"] and a["terminal_deep_cells"] <= a["deep_cells"],
            "local exact fragment work")
    require(s["active_sites"] == sum(s[key] for key in
            ("constant_inside", "constant_outside", "constant_shell_ids", "clipped_events", "kept_events")) and
            s["kept_events"] == s["entries"] + s["exits"] and s["clipped_inside"] <= s["clipped_events"] and
            3*s["leaf_queries"] <= s["constant_shell_ids"] and
            s["leaf_queries"] <= s["active_blocks"] <= s["active_sites"], "local leaf/site/event partition")
    # The global probe uses the default clipped Local28 path. The separate
    # native gate also qualifies unclipped traversal and custom atlas budgets.
    require(s["reference_points"] == s["leaf_queries"] and
            s["reference_points"] <= s["reference_side_tests"] <= 4*s["reference_points"] and
            s["root_locations"] == s["kept_events"] + s["clipped_events"] + s["groups"] and
            0 <= s["kept_events"]-s["group_comparisons"] <= s["leaf_queries"] and
            s["max_group"]*s["groups"] >= s["kept_events"], "local clipping/grouping work")
    require(s["groups"] == s["boundary_skips"] + s["depth_rejections"] + s["groups_without_support"] + s["emitted"] and
            s["kept_events"] == s["presentations"] + s["unexamined_after_emit"] + s["boundary_skipped_ids"] + s["depth_skipped_ids"] and
            s["presentations"] == s["owner_rejections"] + s["positive_tests"] and
            s["positive_tests"] == s["positive_rejections"] + s["canonical_tests"] and
            s["canonical_tests"] == s["canonical_rejections"] + s["emitted"] and
            s["presentations"] <= s["owner_tests"] <= 5*s["presentations"] and
            s["shell_ids"] + w["q3"]["shell_ids"] == w["payload_shell_ids"], "local support/coquille partition")
    require(e["node_visits"] == e["bound_tests"] + e["point_tests"] and
            e["acute_seeds"] == e["seeds"] + e["owner_rejections"] and
            e["acute_seeds"] <= e["owner_tests"] <= 2*e["acute_seeds"], "actual local generator")
    if row["q4_seed_mode"] != "joined":
        require(s["seed_queries"] == e["seeds"] and s["seed_owner_tests"] == 2*s["seed_queries"] and
                s["seed_owner_rejections"] == 0 and
                s["seed_queries"] <= s["query_visits"] and
                s["leaf_queries"] <= s["line_tests"]-s["line_skips"] <= s["query_visits"] and
                s["line_skips"] <= s["line_tests"] <= s["query_visits"], "individual/live actual navigation")
    if row["q4_seed_mode"] == "individual":
        require(e["bound_tests"] == e["rejected_nodes"] + e["split_nodes"] and
                e["rejected_sites"] + e["point_tests"] == row["n"]*w["q4_edges"], "historical generator partition")


# Explicitly ported schema3 row validation follows; only new metadata, Joined
# seed/family semantics and the additional ledger are changed.
def validate_row(row, command):
    require(type(command) is list and len(command) == 15 and all(type(v) is str for v in command) and
            command[10] in MODES and type(row) is dict and row.get("schema") == "mhgp8_wspd_q34_probe_v4" and
            type(row.get("witness_mode")) is str and row["witness_mode"] == command[10] and
            type(row.get("q3_census_mode")) is str and row["q3_census_mode"] == command[11] and
            command[11] in ("scalar", "boxes") and command[12] in BOUNDS_MODES and
            type(row.get("witness_bounds_mode")) is str and row["witness_bounds_mode"] == command[12],
            "affine command/schema/mode")
    require(command[13] in SEED_MODES and type(row.get("q4_seed_mode")) is str and
            row["q4_seed_mode"] == command[13] and type(row.get("q4_seed_block_size")) is int and
            row["q4_seed_block_size"] == int(command[14]) and 0 < row["q4_seed_block_size"] < (1 << 64),
            "seed/cell command/schema/mode/grain")
    # Shape-only view of unchanged fields, not a synthetic old execution. The
    # changed expansion/lane partitions are validated below on the actual row.
    require(type(row.get("work")) is dict and {"witness", "q3_blocks", "q4_seed_cells"} <= set(row["work"]), "missing indexed work")
    common = deepcopy(row)
    common.pop("witness_mode")
    common.pop("q3_census_mode")
    common.pop("witness_bounds_mode")
    common.pop("q4_seed_mode")
    common.pop("q4_seed_block_size")
    common["work"].pop("q4_seed_cells")
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
        if row["q3_census_mode"] == "scalar" and row["q4_seed_mode"] == "individual":
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
            (row["q4_seed_mode"] == "joined" or selected["edge"]["seeds"] == selected["sweep"]["seed_queries"]) and
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
    validate_seed_cells(row)
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
    outputs, profiles, unchanged = {}, {}, {}
    for row in rows:
        key = (row["scan"], row["input_hash"], row["n"], row["kmax"], row["mask"])
        payload = (row["output"], row.get("records"))
        require(key not in outputs or outputs[key] == payload, "seed/cell traversal changed full output")
        outputs[key] = payload
        discrete = deepcopy(row["work"])
        discrete["q3"]["peak_shell_bytes"] = 0
        discrete["peak_edge_buffer_bytes"] = 0
        configuration_key = key + tuple(row[field] for field in ("s", "front_mode", "q4_backend", "witness_mode",
            "q3_census_mode", "witness_bounds_mode"))
        profile = configuration_key + (row["q4_seed_mode"], row["q4_seed_block_size"])
        value = (row["front"], discrete)
        require(profile not in profiles or profiles[profile] == value, "workers changed seed/cell geometric work")
        profiles[profile] = value
        stable = {k: v for k, v in discrete.items() if k not in ("q4_seed_cells", "local28")}
        local = discrete["local28"]
        stable["local_preparation"] = {k: local[k] for k in ("atlas", "geometry")}
        # Navigation differs. The same closed terminal incidences must still
        # execute the same downstream sweep, including boundary-only roots.
        navigation = {"seed_queries", "seed_owner_tests", "seed_owner_rejections", "query_visits", "line_tests", "line_skips"}
        stable["local_payload"] = {k: v for k, v in local["sweep"].items() if k not in navigation}
        value = (row["front"], stable)
        require(configuration_key not in unchanged or unchanged[configuration_key] == value,
                "seed/cell mode changed front/q3/atlas/terminal payload work")
        unchanged[configuration_key] = value


def summary(rows):
    paired(rows)
    groups, growth = {}, []
    fields = ("scan", "kmax", "s", "q4_backend", "workers", "witness_mode", "q3_census_mode",
              "witness_bounds_mode", "q4_seed_mode", "q4_seed_block_size")
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
    eligible = [r for r in records if r["kind"] == "probe" and r["row"]["q4_seed_mode"] == "joined"
                and r["row"]["output"]["callbacks"] > 0]
    require(eligible, "reader tests require a real nonempty Joined row")
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
            raise InvalidReceipt("seed/cell receipt corruption survived")
    def alter(route, value):
        changed = deepcopy(row)
        target = changed
        for key in route[:-1]:
            target = target[key]
        require(target[route[-1]] != value or type(target[route[-1]]) is not type(value), "no-op mutation")
        target[route[-1]] = value
        reject(lambda: validate_row(changed, original["command"]))
    alter(["q4_seed_mode"], "unknown")
    alter(["q4_seed_block_size"], True)
    for field in SEED_FIELDS:
        alter(["work", "q4_seed_cells", field], True)
    for field in ("queries", "live_child_reads", "product_visits", "uncertain_products", "terminal_pairs", "cache_misses",
                  "family_preparations", "form_preparations"):
        alter(["work", "q4_seed_cells", field], row["work"]["q4_seed_cells"][field]+1)
    for field in ("q4_seed_mode", "q4_seed_block_size"):
        changed = deepcopy(row)
        del changed[field]
        reject(lambda: validate_row(changed, original["command"]))
    changed = deepcopy(row)
    del changed["work"]["q4_seed_cells"]
    reject(lambda: validate_row(changed, original["command"]))
    for index, value in ((13, "individual"), (14, "0"), (14, "65")):
        command = original["command"].copy()
        command[index] = value
        reject(lambda: validate_row(row, command))
    altered = deepcopy(original["row"])
    altered["output"]["sum"] = format(int(altered["output"]["sum"],16) ^ 1, "x")
    reject(lambda: paired([original["row"], altered]))
    return dict(inherited, mutants=inherited["mutants"]+tests, inherited_reader_tests=inherited["mutants"],
                new_seed_cell_reader_tests=tests)


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


def configure_protocol():
    for name in ("SCHEMA", "SOURCES", "GATES", "CTEST_COUNT", "validate_matrix", "configuration",
                 "configuration_from_launch", "plan", "validate_gate", "validate_row", "paired", "summary"):
        setattr(protocol, name, globals()[name])


def main():
    args = make_parser().parse_args()
    if args.operation == "run":
        run(args, configuration(args))
    else:
        result = read(args.path, args.check_live) if args.operation == "read" else selftest(args.path)
        if getattr(args, "compact", False):
            result = {key: value for key, value in result.items() if key != "growth"}
        print(json.dumps(result, sort_keys=True, allow_nan=False))


configure_protocol()
run, read = protocol.run, protocol.read


if __name__ == "__main__":
    main()
