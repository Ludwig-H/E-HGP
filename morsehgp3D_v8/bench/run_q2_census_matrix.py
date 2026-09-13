#!/usr/bin/env python3
"""Capture/check paired materialized q2 components, never a FULL/GPU result."""

from __future__ import annotations

import argparse
import base64
from collections import defaultdict
import hashlib
import itertools
import json
import os
import platform
import re
import signal
import statistics
import subprocess
import sys
from pathlib import Path
from typing import Any

from check_paired_campaign import PROVENANCE_POLICY, provenance
from paired_receipts import ADDITIVE_FIELDS, AXIS_FIELDS, close, finite_times, fingerprint
from run_p0_matrix import (InvalidReceipt, WORK_FIELDS, digest, invoke, on_signal,
                           parse_result, require, uint, utc_stamp, validate_work, write_json)


ROOT = Path(__file__).resolve().parents[2]
BENCH = ROOT / "morsehgp3D_v8/bench"
RUNNER_SOURCE = "morsehgp3D_v8/bench/run_q2_census_matrix.py"
LEGACY_RUNNER_SHA256 = "311fce7f66e3b0d6e9a0a8d60ad07ad22382e21255416d5f49b27df7219e00c4"
CAPTURE_RUNNER_ARCHIVE = ROOT / "morsehgp3D_v8/receipts/q2_census_20260913/CAPTURE_RUNNER.json"
RECEIPT_FILES = ("MANIFEST.json", "COMPLETION.json", "MEASURES.jsonl")
FAMILIES = ("grid", "sheet", "sheet_full", "skew", "tube", "rails")
PREFILTERS = ("independent", "additive", "intersection_pool")
ORDERS = ("pairwise-first", "shared-first")
KEYS = ("family", "n", "kmax", "separation_s", "prefilter", "order")
MATRIX_KEYS = ("families", "sizes", "kmax", "separations", "prefilters", "orders")
COUNTERS = (
    "query_build_point_visits", "query_build_nodes", "query_build_max_depth",
    "input_descriptors", "query_cover_visits", "query_tasks", "query_splits", "witness_splits",
    "count_root_starts", "shared_splits_after_credit", "cursor_reuses", "count_node_visits",
    "count_bound_tests", "count_point_tests", "uniform_credited_pairs", "uniform_rejected_pairs",
    "uniform_accepted_pairs", "consumed_witness_sites", "cursor_advances", "frontier_restarts",
    "payload_node_visits", "payload_bound_tests",
    "payload_point_tests", "payload_interior_sites", "payload_shell_sites", "payload_supports",
)
ARM_TIMES = ("consumption_ms", "query_index_ms", "count_ms", "payload_ms", "census_total_ms", "total_ms")
COMMON_TIMES = ("generation_ms", "prepare_ms", "index_ms", "local_plan_ms", "axis_selection_ms",
                "prefilter_ms", "setup_ms", "destruction_ms", "inspection_ms", "paired_execution_ms")


def sources() -> dict[str, str]:
    source = ROOT / "morsehgp3D_v8"
    paths = [source / "CMakeLists.txt", *sorted((source / "src").rglob("*.hpp")),
             *sorted((source / "src").rglob("*.cpp")), *sorted(BENCH.glob("*.hpp")),
             *(BENCH / name for name in ("q2_census_probe.cpp", "run_q2_census_matrix.py",
                                        "run_p0_matrix.py", "paired_receipts.py", "check_paired_campaign.py"))]
    return {str(path.relative_to(ROOT)): digest(path) for path in paths}


def validate_legacy_archive(archive: dict[str, Any]) -> None:
    """Authenticate one evidence snapshot, never execute an old implementation."""
    require(archive.get("schema") == "mhgp8_capture_runner_snapshot_v1" and
            archive.get("path") == RUNNER_SOURCE and
            archive.get("sha256") == LEGACY_RUNNER_SHA256 and
            type(archive.get("source_base64")) is str, "invalid capture runner archive metadata")
    raw = base64.b64decode(archive["source_base64"], validate=True)
    require(hashlib.sha256(raw).hexdigest() == LEGACY_RUNNER_SHA256,
            "capture runner archive content/hash mismatch")


def validate_capture_sources(pins: Any, expected: dict[str, str]) -> str:
    require(type(pins) is dict and set(pins) == set(expected), "source coverage/hash differs from this version")
    require(all(pins[name] == value for name, value in expected.items() if name != RUNNER_SOURCE),
            "non-runner source coverage/hash differs from this version")
    runner_pin = pins[RUNNER_SOURCE]
    if runner_pin != expected[RUNNER_SOURCE]:
        require(runner_pin == LEGACY_RUNNER_SHA256, "unsupported capture runner hash")
        validate_legacy_archive(parse_result(CAPTURE_RUNNER_ARCHIVE.read_bytes()))
    return runner_pin


def campaign_directories(root: Path) -> list[Path]:
    """An initial failure may have only COMPLETION; do not hide such siblings."""
    require(root.is_dir(), "q2 census receipt root is not a directory")
    directories = sorted({path.parent for name in RECEIPT_FILES for path in root.rglob(name)})
    require(bool(directories), "no q2 census campaigns")
    require(not any(parent in child.parents for parent in directories for child in directories),
            "mixed/nested q2 campaign roots are not supported")
    for directory in directories:
        require(all((directory / name).is_file() for name in RECEIPT_FILES),
                f"incomplete q2 campaign receipt: {directory}")
    return directories


def command_for(probe: str, key: tuple[Any, ...]) -> list[str]:
    family, n, kmax, separation, prefilter, order = key
    return [probe, str(n), family, str(kmax), str(separation), prefilter, order]


def validate_result(row: dict[str, Any], command: list[str]) -> None:
    fixed = {
        "schema": "mhgp8_q2_census_probe_v1", "status": "completed",
        "scope": "single_rectangle_q2_materialized_census", "backend": "cpu_reference",
        "profile": "quantized_u16_input_only", "public_status": "not_claimed",
        "output_sink": "streaming_digest", "digest_kind": "sum_xor_fnv1a64_q2_payload_v1",
        "count_timing_kind": "residual_including_instrumentation",
        "totals_kind": "common_setup_and_destruction_plus_one_arm_inspection_separate",
        "s_role": "rectangle_precondition_not_wspd_generation",
    }
    for name, expected in fixed.items():
        require(row.get(name) == expected, f"{name}: wrong scope or convention")
    for name in ("same_owner", "payload_materialized", "output_digests_match"):
        require(row.get(name) is True, f"{name}: required true")
    for name in ("canonical_balls_deduplicated", "full_pipeline_measured"):
        require(row.get(name) is False, f"{name}: unsupported claim")
    for name in ("threads", "owner_preparations", "index_preparations", "prefilter_preparations"):
        require(uint(row.get(name), name) == 1, f"{name}: must equal one")
    require(row.get("seed", False) is None, "unexpected random fixture")
    expected = (command[2], int(command[1]), int(command[3]), int(command[4]), command[5], command[6])
    require(len(command) == 7 and tuple(row.get(key) for key in KEYS) == expected,
            "command/result tuple mismatch")
    for name in ("n", "kmax", "separation_s", "n_a", "n_b", "candidate_pairs", "candidate_descriptors"):
        uint(row.get(name), name)
    n, kmax = row["n"], row["kmax"]
    require(n >= 2 and 1 <= kmax <= 10 and row["separation_s"] > 0 and
            row["family"] in FAMILIES and row["prefilter"] in PREFILTERS and row["order"] in ORDERS,
            "unsupported probe tuple")
    b = max(1, n // 16) if row["family"] == "skew" else n - n // 2
    require(row["n_b"] == b and row["n_a"] == n - b, "wrong factor sizes")
    require(uint(row.get("fixture_version"), "fixture_version") ==
            (2 if row["family"] == "sheet_full" else 1), "wrong fixture version")
    fingerprint(row.get("input_fnv1a64_le_u16_xyz"), "input")
    candidates, descriptors = row["candidate_pairs"], row["candidate_descriptors"]
    require(descriptors <= candidates <= b * (n - b) and (candidates == 0) == (descriptors == 0),
            "invalid candidate descriptors")
    finite_times(row, COMMON_TIMES)
    close(row["prefilter_ms"], row["local_plan_ms"] + row["axis_selection_ms"], "prefilter")
    close(row["setup_ms"], sum(row[key] for key in ("generation_ms", "prepare_ms", "index_ms", "prefilter_ms")),
          "common setup")
    validate_work(row.get("preparation_work"), "preparation_work")
    prep = row["preparation_work"]
    require(prep["validation_points"] == n and all(prep[key] == 0 for key in WORK_FIELDS
            if key not in ("validation_points", "uniqueness_comparisons")) and
            all(value == 0 for value in prep["predicates"].values()), "unexpected owner preparation")
    index = row.get("index_work")
    require(type(index) is dict and set(index) == {"point_visits", "nodes", "max_depth", "escape_links"},
            "wrong global index counters")
    for name, value in index.items():
        uint(value, f"index.{name}")
    # Bounding-box scan AND partition scan at each internal depth, then leaves.
    require(index["nodes"] == index["escape_links"] == 2 * n - 1 and index["max_depth"] <= 48 and
            n <= index["point_visits"] <= 97 * n, "global index omitted sites or exceeded u16 depth")
    axis = row.get("prefilter_work")
    require(type(axis) is dict and set(axis) == {*AXIS_FIELDS, *ADDITIVE_FIELDS}, "wrong prefilter counters")
    for name, value in axis.items():
        uint(value, f"prefilter.{name}")
    require(axis["sort_passes"] == 3 and axis["sorted_sites"] == 3 * row["n_a"] and
            axis["emitted_blocks"] == descriptors and axis["max_tree_depth"] <= 48 and
            axis["contained_nodes"] + axis["whole_factor_accepts"] == descriptors + axis["coalesced_blocks"],
            "inconsistent prefilter accounting")
    restricted = row["prefilter"] == "intersection_pool"
    require(axis["restriction_credit_copies"] == (n if restricted else 0), "wrong restriction copies")
    if restricted:
        validate_work(row.get("local_work"), "local_work")
        require(row["local_work"]["validation_points"] == 0 and
                row["local_work"]["tube_records"] == row["local_work"]["dual_tasks"] == 0,
                "local restriction is not a shared-owner Pool plan")
    else:
        require(row.get("local_work", False) is None and row["local_plan_ms"] == 0,
                "local work fabricated without intersection")
    require(type(row.get("arms")) is list and len(row["arms"]) == 2, "missing/duplicated census arms")
    for mode, arm in zip(("pairwise", "shared"), row["arms"]):
        require(type(arm) is dict and arm.get("mode") == mode, "incorrect census arm order")
        finite_times(arm, ARM_TIMES)
        close(arm["census_total_ms"], arm["query_index_ms"] + arm["count_ms"] + arm["payload_ms"],
              "census count/payload partition")
        close(arm["total_ms"], row["setup_ms"] + row["destruction_ms"] + arm["consumption_ms"],
              "one-arm materialized total")
        require(arm["consumption_ms"] + 1e-6 >= arm["census_total_ms"], "census outer clock omitted work")
        for name in ("candidate_pairs", "accepted_pairs", "rejected_pairs"):
            uint(arm.get(name), name)
        require(arm["candidate_pairs"] == candidates and
                arm["accepted_pairs"] + arm["rejected_pairs"] == candidates,
                "accepted/rejected counts do not partition candidates")
        work = arm.get("work")
        require(type(work) is dict and set(work) == set(COUNTERS), "incorrect census counter fields")
        for name, value in work.items():
            uint(value, f"{mode}.{name}")
        require(work["frontier_restarts"] == 0 and
                work["query_build_max_depth"] <= 48 and work["input_descriptors"] == descriptors and
                work["uniform_accepted_pairs"] <= arm["accepted_pairs"] and
                work["uniform_rejected_pairs"] <= arm["rejected_pairs"] and
                work["count_node_visits"] == work["count_bound_tests"] + work["count_point_tests"] and
                work["payload_node_visits"] == work["payload_bound_tests"] + work["payload_point_tests"],
                "invalid census work accounting")
        if candidates:
            # Root query ranges partition candidates; every task starts on a
            # real witness node (children inherit the node that caused a split).
            require(1 <= work["count_root_starts"] <= candidates and
                    work["count_node_visits"] >= work["query_tasks"] >= work["count_root_starts"],
                    "nonempty census omitted counting work")
        if mode == "pairwise":
            require(work["query_build_nodes"] == work["query_build_point_visits"] == 0 and
                    work["count_root_starts"] == work["query_tasks"] == candidates and arm["query_index_ms"] == 0,
                    "pairwise baseline performed shared query preparation")
            require(work["cursor_advances"] == work["cursor_reuses"] == 0,
                    "pairwise baseline used shared query cursors")
        else:
            if candidates:
                require(work["query_build_nodes"] == 2 * b - 1 and
                        work["query_build_point_visits"] == b,
                        "shared query index omitted nodes or point visits")
                require(work["query_cover_visits"] >= work["count_root_starts"],
                        "shared query cover omitted root starts")
            require(work["cursor_reuses"] == 2 * work["query_splits"] and
                    work["cursor_advances"] + work["query_splits"] == work["count_node_visits"] and
                    work["query_tasks"] == work["count_root_starts"] + 2 * work["query_splits"],
                    "shared cursor/task partition is inconsistent")
        output = arm.get("digest")
        require(type(output) is dict and set(output) == {"supports", "interior_ids", "shell_ids", "sum", "xor"},
                "wrong materialized output digest")
        for name in ("supports", "interior_ids", "shell_ids"):
            uint(output[name], name)
        fingerprint(output["sum"], "payload sum")
        fingerprint(output["xor"], "payload xor")
        require(output["supports"] == arm["accepted_pairs"] == work["payload_supports"] and
                output["interior_ids"] == work["payload_interior_sites"] <= (kmax - 1) * arm["accepted_pairs"] and
                output["shell_ids"] == work["payload_shell_sites"] and
                2 * arm["accepted_pairs"] <= output["shell_ids"] <= n * arm["accepted_pairs"],
                "payload IDs/supports were not fully materialized")
        if candidates == 0:
            require(all(value == 0 for value in work.values()), "empty candidate set performed census work")
    require(row["arms"][0]["digest"] == row["arms"][1]["digest"], "census output digests differ")
    require(row["paired_execution_ms"] + 1e-6 >= row["setup_ms"] + row["destruction_ms"] +
            row["inspection_ms"] + sum(arm["consumption_ms"] for arm in row["arms"]),
            "paired outer clock omitted setup, materialization or destruction")


def stable(row: dict[str, Any]) -> Any:
    return (tuple(row[key] for key in ("candidate_pairs", "candidate_descriptors", "preparation_work",
                                      "index_work", "prefilter_work", "local_work")),
            [(arm["accepted_pairs"], arm["rejected_pairs"], arm["digest"], arm["work"]) for arm in row["arms"]])


def cross_check(row: dict[str, Any], identities: dict, work: dict, outputs: dict) -> None:
    key = row["family"], row["n"]
    fingerprint_value = row["input_fnv1a64_le_u16_xyz"]
    require(key not in identities or identities[key] == fingerprint_value, "fixture identity changed")
    identities[key] = fingerprint_value
    output_key = (*key, row["kmax"])
    output = row["arms"][0]["digest"]
    require(output_key not in outputs or outputs[output_key] == output,
            "final q2 supports/payload changed across prefilters, s or order")
    outputs[output_key] = output
    work_key = (*output_key, row["prefilter"])
    signature = stable(row)
    require(work_key not in work or work[work_key] == signature, "same census configuration changed work or result")
    work[work_key] = signature


def matrix(manifest: dict[str, Any]) -> list[tuple[Any, ...]]:
    require(uint(manifest.get("repeats"), "repeats") >= 1, "empty repetitions")
    for name in MATRIX_KEYS:
        values = manifest.get(name)
        require(type(values) is list and bool(values), f"{name}: empty matrix")
        if name in ("families", "prefilters", "orders"):
            allowed = {"families": FAMILIES, "prefilters": PREFILTERS, "orders": ORDERS}[name]
            require(all(type(value) is str and value in allowed for value in values), f"{name}: invalid values")
        else:
            for value in values:
                uint(value, name)
                require(value >= (2 if name == "sizes" else 1) and (name != "kmax" or value <= 10),
                        f"{name}: unsupported value")
        require(len(set(values)) == len(values), f"{name}: duplicate values")
    return list(itertools.product(*(manifest[key] for key in MATRIX_KEYS), range(manifest["repeats"])))


def capture(args: argparse.Namespace) -> int:
    parameters = dict(families=args.families, sizes=args.sizes, kmax=args.kmax, separations=args.s,
                      prefilters=args.prefilters, orders=args.orders, repeats=args.repeats)
    cases = matrix(parameters)
    args.output.mkdir(parents=True, exist_ok=False)
    binary = args.probe.resolve()
    pins: dict[str, str] = {}
    binary_pin = None
    completed = attempts = 0
    status, error, code = "failed", "campaign did not start", 1
    previous = {signum: signal.signal(signum, on_signal) for signum in (signal.SIGINT, signal.SIGTERM)}
    try:
        pins, binary_pin = sources(), digest(binary)
        cache = (binary.parent / "CMakeCache.txt").read_text()
        compilers = [line.split("=", 1)[1] for line in cache.splitlines()
                     if line.startswith("CMAKE_CXX_COMPILER:") and "=" in line]
        require(len(compilers) == 1 and bool(compilers[0]), "missing CMake compiler provenance")
        manifest = {**parameters, "schema": "mhgp8_q2_census_campaign_v1", "started_utc": utc_stamp(),
            "scope": "single_rectangle_q2_materialized_census", "public_status": "not_claimed",
            "threads": 1, "gcp_used": False, "processes_sequential": True, "warmup_runs": 0,
            "probe": str(binary), "probe_sha256": binary_pin, "source_sha256": pins,
            "cmake_cache": cache, "compiler_version": subprocess.check_output([compilers[0], "--version"], text=True),
            "platform": platform.platform(), "logical_cpu_count": os.cpu_count(),
            "cpuinfo": Path("/proc/cpuinfo").read_text().split("\n\n", 1)[0],
            "commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
            "worktree_status": subprocess.check_output(["git", "status", "--short"], cwd=ROOT, text=True),
            "runner_command": [sys.executable, *sys.argv]}
        write_json(args.output / "MANIFEST.json", manifest)
        identities, work, outputs = {}, {}, {}
        environment = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
        with (args.output / "MEASURES.jsonl").open("x") as stream:
            for *key, repeat in cases:
                command = command_for(str(binary), tuple(key))
                record = dict(command=command, repeat=repeat, status="failed", exit_code=None,
                              stdout="", stderr="", stdout_base64="", stderr_base64="")
                attempts += 1
                try:
                    record["probe_sha256_before"] = digest(binary)
                    require(record["probe_sha256_before"] == binary_pin, "binary changed before invocation")
                    print(json.dumps({"starting": command[1:], "repeat": repeat}), flush=True)
                    invoke(command, environment, ROOT, record)
                    record["probe_sha256_after"] = digest(binary)
                    require(record["probe_sha256_after"] == binary_pin, "binary changed during invocation")
                    if record["exit_code"] != 0:
                        raise RuntimeError(f"probe exited {record['exit_code']}")
                    require(not record["stderr_base64"], "unexpected stderr")
                    row = parse_result(base64.b64decode(record["stdout_base64"]))
                    validate_result(row, command)
                    cross_check(row, identities, work, outputs)
                    record.update(status="completed", result=row)
                    completed += 1
                except (ValueError, KeyError, TypeError, UnicodeError) as cause:
                    record.update(status="invalid", error=str(cause))
                    raise InvalidReceipt(str(cause)) from cause
                except KeyboardInterrupt as cause:
                    record.update(status="interrupted", error=str(cause))
                    raise
                except Exception as cause:
                    record.update(status="failed", error=str(cause))
                    raise
                finally:
                    stream.write(json.dumps(record, allow_nan=False) + "\n")
                    stream.flush()
        status, error, code = "completed", "", 0
    except InvalidReceipt as cause:
        status, error = "invalid", str(cause)
    except KeyboardInterrupt as cause:
        status, error, code = "interrupted", str(cause), 128 + getattr(cause, "signum", signal.SIGINT)
    except Exception as cause:
        status, error = "failed", str(cause)
    finally:
        closing = {}
        for name in pins:
            try:
                closing[name] = digest(ROOT / name)
            except OSError:
                closing[name] = None
        try:
            binary_closing = digest(binary)
        except OSError:
            binary_closing = None
        source_same = bool(pins) and pins == closing
        binary_same = binary_pin is not None and binary_pin == binary_closing
        if status == "completed" and not (source_same and binary_same):
            status, error, code = "invalid", "source or binary changed at closure", 1
        write_json(args.output / "COMPLETION.json", dict(status=status, error=error, runs=completed,
            attempts=attempts, finished_utc=utc_stamp(), source_hashes_unchanged=source_same,
            probe_hash_unchanged=binary_same, source_sha256_closing=closing, probe_sha256_closing=binary_closing))
        for signum, handler in previous.items():
            signal.signal(signum, handler)
    print(json.dumps({"status": status, "runs": completed, "attempts": attempts, "error": error}))
    return code


def check(args: argparse.Namespace) -> int:
    paths = [directory / "MANIFEST.json" for directory in campaign_directories(args.receipt)]
    expected_sources = sources()
    capture_runners = set()
    identities, work, outputs, groups = {}, {}, {}, defaultdict(list)
    common_provenance = None
    published = None
    measures = 0
    for path in paths:
        manifest = parse_result(path.read_bytes())
        completion = parse_result((path.parent / "COMPLETION.json").read_bytes())
        require(manifest.get("schema") == "mhgp8_q2_census_campaign_v1" and
                manifest.get("scope") == "single_rectangle_q2_materialized_census" and
                manifest.get("public_status") == "not_claimed" and manifest.get("gcp_used") is False and
                uint(manifest.get("threads"), "threads") == 1 and manifest.get("processes_sequential") is True and
                uint(manifest.get("warmup_runs"), "warmup_runs") == 0, "wrong campaign scope")
        capture_runners.add(validate_capture_sources(manifest.get("source_sha256"), expected_sources))
        require(type(manifest.get("probe")) is str and bool(manifest["probe"]) and
                type(manifest.get("probe_sha256")) is str and
                re.fullmatch(r"[0-9a-f]{64}", manifest["probe_sha256"]) is not None,
                "invalid binary provenance")
        build, machine, published = provenance(manifest)
        require(common_provenance is None or common_provenance == (build, machine), "heterogeneous provenance")
        common_provenance = build, machine
        require(completion.get("status") == "completed" and completion.get("source_hashes_unchanged") is True and
                completion.get("probe_hash_unchanged") is True and
                completion.get("source_sha256_closing") == manifest["source_sha256"] and
                completion.get("probe_sha256_closing") == manifest["probe_sha256"], "invalid campaign closure")
        expected = set(matrix(manifest))
        records = [parse_result(line) for line in (path.parent / "MEASURES.jsonl").read_bytes().splitlines()]
        require(uint(completion.get("runs"), "runs") == uint(completion.get("attempts"), "attempts") ==
                len(records) == len(expected), "incomplete campaign matrix")
        seen = set()
        for record in records:
            require(record.get("status") == "completed" and uint(record.get("exit_code"), "exit_code") == 0 and
                    record.get("stderr") == record.get("stderr_base64") == "" and
                    record.get("probe_sha256_before") == record.get("probe_sha256_after") == manifest["probe_sha256"],
                    "invalid invocation or binary pin")
            raw = base64.b64decode(record["stdout_base64"], validate=True)
            row = parse_result(raw)
            key = tuple(row[name] for name in KEYS)
            command = command_for(manifest["probe"], key)
            require(raw.decode() == record["stdout"] and row == record["result"] and
                    record["command"] == command, "command/raw/result mismatch")
            validate_result(row, command)
            identity = (*key, uint(record.get("repeat"), "repeat"))
            require(identity not in seen, "duplicate invocation")
            seen.add(identity)
            cross_check(row, identities, work, outputs)
            groups[key].append(row)
            measures += 1
        require(seen == expected, "missing/extra matrix tuples")
    result = dict(status="passed", campaigns=len(paths), measurements=measures,
                  configurations_including_order=len(groups), scope="materialized_q2_component_receipts",
                  full_contract_qualified=False, gcp_used=False, provenance_policy=PROVENANCE_POLICY,
                  provenance=published, current_reader_sha256=expected_sources[RUNNER_SOURCE],
                  capture_runner_sha256=sorted(capture_runners))
    if args.summary:
        result["summary"] = []
        for key, rows in sorted(groups.items()):
            item = dict(zip(KEYS, key), repeats=len(rows), candidate_pairs=rows[0]["candidate_pairs"],
                        candidate_descriptors=rows[0]["candidate_descriptors"], arms=[])
            for field in COMMON_TIMES:
                item[f"median_{field}"] = statistics.median(row[field] for row in rows)
            for index, mode in enumerate(("pairwise", "shared")):
                arm = dict(mode=mode, work=rows[0]["arms"][index]["work"], digest=rows[0]["arms"][index]["digest"],
                           accepted_pairs=rows[0]["arms"][index]["accepted_pairs"],
                           rejected_pairs=rows[0]["arms"][index]["rejected_pairs"])
                arm.update({f"median_{field}": statistics.median(row["arms"][index][field] for row in rows)
                            for field in ARM_TIMES})
                item["arms"].append(arm)
            result["summary"].append(item)
    print(json.dumps(result, indent=2, allow_nan=False))
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    run = sub.add_parser("run")
    run.add_argument("--probe", required=True, type=Path)
    run.add_argument("--output", required=True, type=Path)
    run.add_argument("--sizes", nargs="+", type=int, default=[8000, 16000, 32000])
    run.add_argument("--families", nargs="+", choices=FAMILIES, default=["grid", "sheet_full"])
    run.add_argument("--kmax", nargs="+", type=int, default=[5, 10])
    run.add_argument("--s", nargs="+", type=int, default=[8, 10, 12])
    run.add_argument("--prefilters", nargs="+", choices=PREFILTERS, default=["intersection_pool"])
    run.add_argument("--orders", nargs="+", choices=ORDERS, default=list(ORDERS))
    run.add_argument("--repeats", type=int, default=1)
    reader = sub.add_parser("check")
    reader.add_argument("receipt", type=Path)
    reader.add_argument("--summary", action="store_true")
    args = parser.parse_args()
    try:
        return capture(args) if args.operation == "run" else check(args)
    except (ValueError, OSError, KeyError, TypeError) as error:
        if args.operation == "run":
            parser.error(str(error))
        print(f"q2 census receipt rejected: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
