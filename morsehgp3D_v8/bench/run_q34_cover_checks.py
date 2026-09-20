#!/usr/bin/env python3
"""Paired one-edge qualification, not the global q3/q4 producer or HGP tower.

Reuses the frozen seed collector in this process through explicit adapters;
no imported file or historical receipt is edited. Capture, interruption,
raw-log preservation and source/artifact closure retain the same code path.
The dense adversary is deliberately small: it exposes S*C, not a speed claim.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
import json
import math
import os
from pathlib import Path
import xml.etree.ElementTree as ET

import run_q34_seed_checks as collector
from run_p0_matrix import InvalidReceipt, parse_result, require, uint
from run_q4_family_checks import counts, read_json, validate_closure

ROOT = collector.ROOT
SOURCES = collector.SOURCES | frozenset({
    "morsehgp3D_v8/src/lanes/edge_cover.hpp", "morsehgp3D_v8/src/lanes/edge_cover.cpp",
    "morsehgp3D_v8/src/lanes/q34_cover.hpp", "morsehgp3D_v8/src/lanes/q34_cover.cpp",
    "morsehgp3D_v8/tests/q34_cover_gate.cpp", "morsehgp3D_v8/bench/q34_cover_probe.cpp",
    "morsehgp3D_v8/bench/run_q34_cover_checks.py",
})
SCHEMA = "mhgp8_q34_cover_attempt_v1"
GATES = (*collector.GATES, "mhgp8_q34_cover_gate")
REGIMES = ("far", "cap", "adversarial")
SEED_FIELDS = collector.WORK_FIELDS
FAMILY_FIELDS = collector.FAMILY_FIELDS
COVER_FIELDS = ("node_visits", "bound_tests", "point_tests", "admitted_nodes", "rejected_nodes",
                "split_nodes", "admitted_sites", "rejected_sites", "retained_ranges", "merged_ranges")
EDGE_FIELDS = ("node_visits", "bound_tests", "point_tests", "rejected_nodes", "split_nodes", "rejected_sites",
               "acute_seeds", "owner_tests", "owner_rejections", "seeds")
COVERED_FIELDS = ("site_reads", "q3_shell_sort_comparisons", "q4_shell_sort_comparisons", "peak_buffer_bytes")
CLOUD_FIELDS = ("coordinate_copies", "validation_points", "uniqueness_comparisons", "uniqueness_adjacent_tests",
                "range_tree_leaf_visits", "range_tree_nodes", "range_tree_merges")
INDEX_FIELDS = ("point_visits", "nodes", "max_depth", "escape_links")
DIGEST_FIELDS = ("callbacks", "q3", "q4", "support_ids_visited", "shell_ids_visited", "hash")
MEMORY_FIELDS = ("id_bytes", "input_capacity_bytes", "old_cloud_retained_bytes", "cloud_retained_bytes",
                 "index_retained_bytes", "cover_retained_bytes", "old_output_retained_bytes", "output_retained_bytes")
PART_TIMES = ("generation_ms", "fixture_validation_ms", "old_cloud_ms", "old_run_callback_ms",
              "old_validation_ms", "old_owner_release_ms", "cloud_ms", "index_ms", "cover_ms",
              "run_callback_ms", "validation_ms", "owner_release_ms", "shared_release_ms")
FIXED = dict(schema="mhgp8_q34_cover_probe_v1", status="completed", scope="one_edge_not_global_q34_producer",
             public_status="not_claimed", backend="cpu_reference", profile="quantized_u16_input_only",
             threads=1, seed=3, recipe="owned_edge_far_cap_dense_v1", edge_ids=[0, 1],
             timing_scope="paired_generation_fixture_owners_index_cover_callbacks_validation_release_excludes_json")
COVER_GATE_FLOORS = dict(checks=1, clouds=12, covers=150, edge_calls=150, seed_calls=100, reference_calls=100,
    oracle_completions=1, oracle_sites=1000, candidates=40, q3=30, q4=10, max_shell=30, cover_boundary=2,
    cover_excluded=1, cover_admitted_nodes=1, cover_rejected_nodes=1, cover_split_nodes=1, reused_covers=1,
    q4_without_q3=2, invalid_root_depth_changed=1, exhaustive_edges=150, permutations=1, invalid_inputs=9,
    callback_failures=1, parallel_calls=4, judge_mutants=3, edge_seed_count=1, edge_pruned_sites=1, covered_site_reads=1)
BASE_GATE_VALIDATOR = collector.validate_gate
BASE_READ = collector.read


def expected_digest():
    """Closed-form fixture; hash both 64-bit limbs of signed 128-bit keys."""
    value = 14695981039346656037
    def word(number):
        nonlocal value
        for byte in (number & ((1 << 64) - 1)).to_bytes(8, "little"):
            value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    for arity, ids, key in (
        (3, [0, 1, 2], [1, -2000, -2045, -2015, 3050000]),
        (3, [0, 1, 3], [1, -2000, -2045, -1985, 3020000]),
        (4, [0, 1, 2, 3], [1, -2000, -2050, -2000, 3040000])):
        word(arity); word(2)
        for coefficient in key:
            word(coefficient); word(coefficient >> 64)
        for site in ids: word(site)
        word(len(ids))
        for site in ids: word(site)
    return dict(callbacks=3, q3=2, q4=1, support_ids_visited=10, shell_ids_visited=10, hash=value)


def validate_seed(work, seeds, population, output, id_bytes, covered):
    require(type(work) is dict and set(work) == set(SEED_FIELDS) | {"family"}, "seed fields differ")
    for key in SEED_FIELDS: uint(work[key], "seed." + key)
    f = work["family"]
    counts(f, FAMILY_FIELDS, "family")
    require(work["seed_owner_tests"] == 2 * seeds and work["seed_owner_rejections"] == 0 and
            work["q3_depth_rejections"] + work["q3_emitted"] == seeds and
            work["q3_emitted"] == output["q3"] and work["q4_emitted"] == output["q4"], "seed/output partition mismatch")
    require(seeds <= work["q3_point_tests"] <= seeds * population and
            (not covered or work["q3_point_tests"] == seeds * population), "q3 work is not accounted")
    require(f["sites"] == seeds * population and f["sites"] == f["entries"] + f["exits"] +
            f["constant_inside"] + f["constant_on"] + f["constant_outside"] and
            f["event_count"] == f["entries"] + f["exits"] and f["event_count"] >= seeds and
            f["constant_on"] >= 3 * seeds, "family site partition mismatch")
    require(f["groups"] == f["callbacks"] and seeds <= f["groups"] <= f["event_count"] and
            1 <= f["max_group"] <= f["event_count"] and f["max_group"] * f["groups"] >= f["event_count"] and
            f["group_comparisons"] == f["event_count"] - seeds and f["sort_comparisons"] > 0,
            "family grouping mismatch")
    require(work["q4_presentations"] + work["q4_depth_skipped_ids"] + work["q4_unexamined_after_emit"] == f["event_count"] and
            work["q4_depth_rejected_groups"] + work["q4_groups_without_support"] + work["q4_emitted"] == f["groups"],
            "q4 roots/IDs partition mismatch")
    require(work["q4_presentations"] == work["q4_owner_rejections"] + work["q4_positive_tests"] and
            work["q4_positive_tests"] == work["q4_positive_rejections"] + work["q4_seed_tests"] and
            work["q4_seed_tests"] == work["q4_seed_rejections"] + work["q4_emitted"] and
            work["q4_presentations"] <= work["q4_owner_tests"] <= 5 * work["q4_presentations"],
            "q4 presentation cascade mismatch")
    require(f["retained_capacity_bytes"] >= 2 * population * id_bytes and
            work["q3_shell_capacity_bytes"] >= 3 * id_bytes, "seed capacity missing")


def validate_row(row, command):
    require(type(row) is dict and len(command) == 4, "cover probe command arity")
    n, regime, kmax = int(command[1]), command[2], int(command[3])
    require(n >= 8 and regime in REGIMES and kmax in (5, 10) and
            (regime != "cap" or n <= 35307) and (regime != "adversarial" or n <= 514), "fixture domain mismatch")
    extra = {"n", "regime", "kmax", "input_hash", "provided_old_seeds", "cover_sites", "generation", "validation",
             "old_work", "edge_work", "cover_work", "index_work", "old_cloud_work", "cloud_work", "old_digest", "digest", "memory", "timings"}
    require(set(row) == set(FIXED) | extra and all(type(row[k]) is type(v) and row[k] == v for k, v in FIXED.items()) and
            all(type(v) is int for v in row["edge_ids"]), "probe fields/scope mismatch")
    require(type(row["n"]) is int and row["n"] == n and type(row["kmax"]) is int and row["kmax"] == kmax and
            row["regime"] == regime, "command/result mismatch")
    uint(row["input_hash"], "input_hash")
    seeds, covered = n - 2 if regime == "adversarial" else 2, 6 if regime == "far" else n
    require(uint(row["provided_old_seeds"], "provided_old_seeds") == seeds and
            uint(row["cover_sites"], "cover_sites") == covered, "fixture seed/cover counts differ")
    counts(row["generation"], ("proposals", "duplicates", "random_calls"), "generation")
    g = row["generation"]
    require(g["proposals"] == n - (2 if regime == "adversarial" else 6) + g["duplicates"] and
            g["random_calls"] == (3 * g["proposals"] if regime == "far" else 0) and
            (regime == "far" or g["duplicates"] == 0), "input generation partition mismatch")
    v = row["validation"]
    require(type(v) is dict and set(v) == {"fixture_point_tests", "independent_ball_point_tests", "method"} and
            uint(v["fixture_point_tests"], "fixture_point_tests") == n and
            uint(v["independent_ball_point_tests"], "independent_ball_point_tests") == (0 if regime == "adversarial" else 3 * n) and
            v["method"] == ("qualified_old_path_differential" if regime == "adversarial" else "three_closed_form_balls_plus_differential"),
            "independent validation scope/work differs")
    for key in ("old_digest", "digest"):
        counts(row[key], DIGEST_FIELDS, key)
        d = row[key]
        require(d["callbacks"] == d["q3"] + d["q4"] and d["support_ids_visited"] == 3 * d["q3"] + 4 * d["q4"] and
                d["shell_ids_visited"] >= d["support_ids_visited"] and d["q3"] > 0 and d["q4"] > 0,
                "nonvacuous output ledger failed")
    require(row["old_digest"] == row["digest"] and (regime == "adversarial" or row["digest"] == expected_digest()),
            "old/new/independent digest mismatch")
    m = row["memory"]
    require(type(m) is dict and set(m) == set(MEMORY_FIELDS) | {"scope"} and
            m["scope"] == "retained_capacities_not_RSS_shared_old_output_persists_for_comparison", "memory fields/scope differ")
    for key in MEMORY_FIELDS: uint(m[key], "memory." + key)
    require(m["id_bytes"] in (4, 8) and all(m[key] >= 6 * n for key in
            ("input_capacity_bytes", "old_cloud_retained_bytes", "cloud_retained_bytes")) and
            m["old_cloud_retained_bytes"] == m["cloud_retained_bytes"] and m["index_retained_bytes"] >= n * m["id_bytes"] and
            m["cover_retained_bytes"] >= 2 * m["id_bytes"] and
            min(m["old_output_retained_bytes"], m["output_retained_bytes"]) >= row["digest"]["shell_ids_visited"] * m["id_bytes"],
            "retained capacity missing")
    e = row["edge_work"]
    require(type(e) is dict and set(e) == set(EDGE_FIELDS) | {"covered"}, "edge fields differ")
    for key in EDGE_FIELDS: uint(e[key], "edge." + key)
    require(e["node_visits"] == e["bound_tests"] + e["point_tests"] and e["node_visits"] <= 2 * n - 1 and
            e["bound_tests"] == e["rejected_nodes"] + e["split_nodes"] and e["rejected_sites"] + e["point_tests"] == n and
            e["seeds"] == seeds and e["acute_seeds"] == seeds + e["owner_rejections"] and
            e["acute_seeds"] <= e["point_tests"] and e["acute_seeds"] <= e["owner_tests"] <= 2 * e["acute_seeds"],
            "edge traversal/seed partition differs")
    cw = e["covered"]
    require(type(cw) is dict and set(cw) == set(COVERED_FIELDS) | {"seed"}, "covered fields differ")
    for key in COVERED_FIELDS: uint(cw[key], "covered." + key)
    require(cw["site_reads"] == seeds * covered, "fused cover scan does not expose S*C")
    validate_seed(row["old_work"], seeds, n, row["old_digest"], m["id_bytes"], False)
    validate_seed(cw["seed"], seeds, covered, row["digest"], m["id_bytes"], True)
    caps = (cw["seed"]["family"]["retained_capacity_bytes"], cw["seed"]["q3_shell_capacity_bytes"])
    require(max(caps) <= cw["peak_buffer_bytes"] <= sum(caps), "peak capacity is not a maximum over simultaneous buffers")
    c = row["cover_work"]
    counts(c, COVER_FIELDS, "cover")
    require(c["node_visits"] == c["bound_tests"] + c["point_tests"] and c["node_visits"] <= 2 * n - 1 and
            c["node_visits"] == c["admitted_nodes"] + c["rejected_nodes"] + c["split_nodes"] and
            c["admitted_sites"] == covered and c["admitted_sites"] + c["rejected_sites"] == n and
            c["retained_ranges"] + c["merged_ranges"] == c["admitted_nodes"] and c["retained_ranges"] > 0 and
            m["cover_retained_bytes"] >= 2 * m["id_bytes"] * c["retained_ranges"], "cover tree/range partition differs")
    counts(row["index_work"], INDEX_FIELDS, "index")
    ix = row["index_work"]
    require(ix["nodes"] == ix["escape_links"] == 2 * n - 1 and ix["point_visits"] >= n and ix["max_depth"] > 0,
            "global index preparation missing")
    for key in ("old_cloud_work", "cloud_work"):
        counts(row[key], CLOUD_FIELDS, key)
        cloud = row[key]
        require(cloud["coordinate_copies"] == cloud["validation_points"] == cloud["range_tree_leaf_visits"] == n and
                cloud["uniqueness_adjacent_tests"] == cloud["range_tree_merges"] == n - 1 and
                cloud["range_tree_nodes"] == 2 * n - 1, "owner preparation missing")
    require(row["old_cloud_work"] == row["cloud_work"], "arms have different owner preparation")
    t = row["timings"]
    require(type(t) is dict and set(t) == set(PART_TIMES) | {"old_arm_ms", "new_arm_ms", "total_ms"} and
            all(type(value) in (int, float) and math.isfinite(value) and value >= 0 for value in t.values()), "invalid timings")
    for result, parts in (("total_ms", PART_TIMES), ("old_arm_ms", PART_TIMES[2:6]), ("new_arm_ms", PART_TIMES[6:12])):
        require(math.isclose(t[result], sum(t[key] for key in parts), rel_tol=1e-12, abs_tol=1e-6), "inclusive timing partition differs")


def validate_gate(row, executable):
    if executable in GATES[:2]:
        BASE_GATE_VALIDATOR(row, executable)
        return
    require(executable == GATES[2] and type(row) is dict and set(row) == set(COVER_GATE_FLOORS) | {"schema", "status"} and
            row["schema"] == executable + "_v1" and row["status"] == "passed", "cover gate fields/schema differ")
    for key, floor in COVER_GATE_FLOORS.items():
        require(uint(row[key], "gate." + key) >= floor, "cover gate nonvacuity floor failed")
    require(row["candidates"] == row["q3"] + row["q4"] and row["seed_calls"] == row["reference_calls"] and
            all(row[key] == value for key, value in dict(clouds=12, invalid_inputs=9, callback_failures=1,
            parallel_calls=4, judge_mutants=3, invalid_root_depth_changed=1).items()), "cover gate ledger/lifecycle differs")


def validate_xml(path):
    tree = ET.parse(path).getroot()
    require(tree.tag == "testsuite" and tree.get("tests") == "85" and
            all(tree.get(key) == "0" for key in ("failures", "disabled", "skipped")), "unsuccessful 85-test regression")
    cases = tree.findall("testcase")
    names = [case.get("name") for case in cases]
    require(len(cases) == len(set(names)) == 85 and all(type(name) is str and name.startswith("mhgp8_") for name in names) and
            set(GATES) | {"mhgp8_q4_family_gate"} <= set(names), "regression inventory differs")
    require(all(case.get("status") == "run" and all(case.find(tag) is None for tag in ("failure", "error", "skipped")) for case in cases),
            "regression failed/skipped testcase")
    return sorted(names)


def plan(build, campaign, output, ctest=None):
    if campaign == "regression":
        return [("ctest", [ctest, "--test-dir", str(build), "--output-on-failure", "--parallel", "2",
                           "--output-junit", str(output / "result.xml")])]
    result = [("gate", [str(build / name), "--selftest"]) for name in GATES]
    for regime in REGIMES:
        sizes = () if campaign == "gate" else (32,) if campaign == "smoke" else (
            (32, 64, 128, 256) if regime == "adversarial" else (8000, 16000, 32000))
        for k in (5, 10):
            for n in sizes:
                result.append(("measure", [str(build / "mhgp8_q34_cover_probe"), str(n), regime, str(k)]))
    return result


def executable_names(build, campaign):
    if campaign == "regression":
        names = sorted(p.name for p in build.glob("mhgp8_*") if p.is_file() and os.access(p, os.X_OK))
        require(names and all((build / name).resolve() == build / name for name in names), "missing or linked executable")
        return names
    return sorted([*GATES, *(["mhgp8_q34_cover_probe"] if campaign != "gate" else [])])


def install_adapters():
    # These are in-memory module variables, not source rewrites. Suppress only
    # the old seed-specific post-read analysis; the complete capture reader,
    # provenance, command matching and source/artifact pin checks still run.
    collector.SOURCES, collector.SCHEMA, collector.GATES = SOURCES, SCHEMA, GATES
    collector.FAMILIES = ()
    collector.plan, collector.executable_names = plan, executable_names
    collector.validate_row, collector.validate_gate, collector.validate_xml = validate_row, validate_gate, validate_xml


def read(path, check_live=False):
    install_adapters()
    summary = BASE_READ(path, check_live)
    records = [read_json(p) for p in sorted(path.glob("record_*.json"))]
    rows = [r["row"] for r in records if r["kind"] == "measure"]
    for regime in REGIMES:
        for n in sorted({r["n"] for r in rows if r["regime"] == regime}):
            pair = [r for r in rows if r["regime"] == regime and r["n"] == n]
            require(len(pair) == 2 and {r["kmax"] for r in pair} == {5, 10}, "missing K5/K10 pair")
            require(all(pair[0][key] == pair[1][key] for key in
                    ("input_hash", "generation", "cloud_work", "old_cloud_work", "index_work", "cover_work", "cover_sites", "provided_old_seeds")),
                    "paired immutable input/preparation differs")
            if regime != "adversarial": require(pair[0]["digest"] == pair[1]["digest"], "positive local outputs differ by K")
    growth = {}
    if summary["campaign"] == "scale":
        for regime in REGIMES:
            for k in (5, 10):
                series = sorted((r for r in rows if r["regime"] == regime and r["kmax"] == k), key=lambda r: r["n"])
                metrics = {"old.family.sites": [r["old_work"]["family"]["sites"] for r in series],
                           "covered.site_reads": [r["edge_work"]["covered"]["site_reads"] for r in series],
                           "edge.seeds": [r["edge_work"]["seeds"] for r in series],
                           "cover.sites": [r["cover_sites"] for r in series]}
                for section in ("cover_work", "index_work"):
                    metrics.update({section + "." + field: [r[section][field] for r in series] for field in series[0][section]})
                metrics.update({"edge." + field: [r["edge_work"][field] for r in series] for field in EDGE_FIELDS})
                metrics.update({"covered." + field: [r["edge_work"]["covered"][field] for r in series] for field in COVERED_FIELDS})
                for section in ("old_cloud_work", "cloud_work"):
                    metrics.update({section + "." + field: [r[section][field] for r in series] for field in CLOUD_FIELDS})
                metrics.update({"old.family." + field: [r["old_work"]["family"][field] for r in series] for field in FAMILY_FIELDS})
                metrics.update({"old.seed." + field: [r["old_work"][field] for r in series] for field in SEED_FIELDS})
                metrics.update({"new.family." + field: [r["edge_work"]["covered"]["seed"]["family"][field] for r in series] for field in FAMILY_FIELDS})
                metrics.update({"new.seed." + field: [r["edge_work"]["covered"]["seed"][field] for r in series] for field in SEED_FIELDS})
                metrics.update({"digest." + field: [r["digest"][field] for r in series] for field in DIGEST_FIELDS if field != "hash"})
                metrics.update({"memory." + field: [r["memory"][field] for r in series] for field in MEMORY_FIELDS if field != "id_bytes"})
                metrics.update({"timings." + field: [r["timings"][field] for r in series] for field in ("old_run_callback_ms", "run_callback_ms", "old_arm_ms", "new_arm_ms", "total_ms")})
                growth[f"{regime}_K{k}"] = {metric: dict(values=values, ratios=[b / a if a else None for a, b in zip(values, values[1:])],
                    above_quadrupling=[a > 0 and b > 4 * a for a, b in zip(values, values[1:])]) for metric, values in metrics.items()}
    summary.update(scope="one_edge_not_global_q34_producer", growth=growth,
                   old_seed_discovery_included=False, new_seed_discovery_included=True,
                   dense_adversary_exposes_seed_count_times_cover_size=True)
    return summary


def selftest(path):
    summary = read(path)
    records = [read_json(p) for p in sorted(path.glob("record_*.json"))]
    base = next(r for r in records if r["kind"] == "measure" and r["row"]["regime"] == "far")
    mutants = 0
    def reject(action):
        nonlocal mutants
        try: action()
        except InvalidReceipt: mutants += 1
        else: raise InvalidReceipt("reader mutant survived")
    for route, value in ((["n"], True), (["kmax"], True), (["edge_ids"], [False, 1]),
            (["provided_old_seeds"], 1), (["cover_sites"], 7), (["digest", "hash"], 0),
            (["digest", "shell_ids_visited"], 0), (["validation", "independent_ball_point_tests"], 0),
            (["edge_work", "covered", "site_reads"], 0), (["edge_work", "seeds"], True),
            (["edge_work", "covered", "seed", "q4_presentations"], 0),
            (["cover_work", "admitted_sites"], 0), (["index_work", "nodes"], 0),
            (["memory", "id_bytes"], True), (["timings", "new_arm_ms"], float("nan"))):
        row = deepcopy(base["row"])
        target = row
        for key in route[:-1]: target = target[key]
        target[route[-1]] = value
        reject(lambda: validate_row(row, base["command"]))
    command = base["command"].copy(); command[1] = str(int(command[1]) + 1)
    reject(lambda: validate_row(base["row"], command))
    reject(lambda: parse_result(b'{"x":NaN}'))
    reject(lambda: parse_result(b'{"x":1,"x":2}'))
    for record in records:
        if record["kind"] != "gate": continue
        gate = deepcopy(record["row"])
        gate["checks"] = True
        reject(lambda: validate_gate(gate, Path(record["command"][0]).name))
        gate["checks"] = 0
        reject(lambda: validate_gate(gate, Path(record["command"][0]).name))
    m, c = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
    c["source_sha256_after"][next(iter(m["source_sha256"]))] = "0" * 64
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
        result = read(args.path, args.check_live) if args.operation == "read" else selftest(args.path)
        print(json.dumps(result, sort_keys=True, allow_nan=False))


if __name__ == "__main__":
    main()
