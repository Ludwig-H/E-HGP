#!/usr/bin/env python3
"""Capture/check serial cloud-reuse components, not WSPD or an HGP tower."""

from __future__ import annotations

import argparse
import base64
from collections import defaultdict
from datetime import datetime
import itertools
import json
import os
from pathlib import Path
import platform
import re
import signal
import statistics
import subprocess
import sys
from typing import Any

from check_paired_campaign import PROVENANCE_POLICY, provenance, sha256
from paired_receipts import ADDITIVE_FIELDS, AXIS_FIELDS, close, finite_times, fingerprint
from run_p0_matrix import (digest, invoke, on_signal,
                           parse_result, require, uint, utc_stamp, validate_work, write_json)
from run_q2_census_matrix import COUNTERS


ROOT = Path(__file__).resolve().parents[2]
RUNNER_SOURCE = "morsehgp3D_v8/bench/run_cloud_reuse_matrix.py"
SCOPE = "single_rectangle_partition_cloud_reuse_not_wspd"
SCHEMA = "mhgp8_cloud_reuse_campaign_v1"
FAMILIES = ("grid", "sheet_full", "skew")
ORDERS = ("fresh-first", "shared-first")
KEYS = ("family", "n", "kmax", "separation_s", "rectangles", "order")
MATRIX_KEYS = ("families", "sizes", "kmax", "separations", "rectangles", "orders")
RECEIPT_FILES = ("MANIFEST.json", "MEASURES.jsonl", "COMPLETION.json")
CLOUD_FIELDS = ("coordinate_copies", "validation_points", "uniqueness_comparisons",
                "uniqueness_adjacent_tests", "range_tree_leaf_visits", "range_tree_nodes", "range_tree_merges")
INDEX_FIELDS = ("point_visits", "nodes", "max_depth", "escape_links")
ARM_TIMES = ("processing_ms", "total_ms", "cloud_ms", "index_ms", "rectangle_ms", "local_plan_ms",
             "axis_selection_ms", "consumption_ms", "census_query_index_ms", "census_count_ms",
             "census_payload_ms", "census_total_ms", "destruction_ms")
COMMON_TIMES = ("generation_ms", "input_destruction_ms", "comparison_ms", "paired_execution_ms")
LOCAL_FIELDS = ("rectangle_preparations", "factor_box_visits", "factor_box_steps", "total_pairs",
                "candidate_pairs", "candidate_descriptors", "accepted_pairs", "rejected_pairs",
                "max_cloud_retained_bytes", "max_index_retained_bytes", "preparation_work", "local_work",
                "axis_work", "census_work", "digest")


def sources() -> dict[str, str]:
    source = ROOT / "morsehgp3D_v8"
    bench = source / "bench"
    paths = [source / "CMakeLists.txt", *sorted((source / "src").rglob("*.hpp")),
             *sorted((source / "src").rglob("*.cpp")), *sorted(bench.glob("*.hpp")),
             *(bench / name for name in ("cloud_reuse_probe.cpp", "run_cloud_reuse_matrix.py",
                 "run_p0_matrix.py", "paired_receipts.py", "check_paired_campaign.py",
                 "run_q2_census_matrix.py"))]
    return {str(path.relative_to(ROOT)): digest(path) for path in paths}


def timestamp(value: Any, name: str) -> datetime:
    require(type(value) is str, f"{name}: missing timestamp")
    result = datetime.fromisoformat(value)
    require(result.tzinfo is not None and result.utcoffset().total_seconds() == 0,
            f"{name}: timestamp must be UTC")
    return result


def counters(value: Any, names: tuple[str, ...], name: str) -> None:
    require(type(value) is dict and set(value) == set(names), f"{name}: wrong counter fields")
    for field in names:
        uint(value[field], f"{name}.{field}")


def grouped_work(value: Any, name: str) -> None:
    require(type(value) is dict and set(value) == {"counters", "predicates"} and
            type(value["counters"]) is dict, f"{name}: wrong grouped work")
    validate_work({**value["counters"], "predicates": value["predicates"]}, name)


def command_for(probe: str, key: tuple[Any, ...]) -> list[str]:
    family, n, kmax, separation, rectangles, order = key
    return [probe, str(n), family, str(kmax), str(separation), str(rectangles), order]


def validate_result(row: dict[str, Any], command: list[str]) -> None:
    fixed = dict(schema="mhgp8_cloud_reuse_probe_v1", status="completed", scope=SCOPE,
        backend="cpu_reference", profile="quantized_u16_input_only", public_status="not_claimed",
        partition_recipe="balanced_contiguous_a_original_order_v1",
        s_role="rectangle_precondition_not_wspd_generation", prefilter="intersection_pool",
        census_mode="pairwise", output_sink="streaming_digest", digest_kind="sum_xor_fnv1a64_q2_payload_v1",
        work_aggregation="sum_except_depth_maxima", cloud_memory_kind="maximum_live_cloud_vector_capacities_only",
        index_memory_kind="maximum_live_index_vector_capacities_cloud_excluded",
        totals_kind="common_generation_and_input_destruction_plus_enclosing_arm")
    for field, expected in fixed.items():
        require(row.get(field) == expected, f"{field}: unsupported scope or convention")
    for field in ("payload_materialized", "output_digests_match", "local_work_match", "global_work_scaling_match"):
        require(row.get(field) is True, f"{field}: expected true")
    for field in ("gcp_used", "canonical_balls_deduplicated", "full_pipeline_measured"):
        require(row.get(field) is False, f"{field}: unsupported claim")
    require(uint(row.get("threads"), "threads") == uint(row.get("contexts_retained"), "contexts_retained") == 1 and
            row.get("seed", False) is None, "unexpected concurrency or fixture randomness")
    require(len(command) == 7, "wrong command length")
    for field in ("n", "kmax", "separation_s", "rectangles", "n_a", "n_b", "fixture_version"):
        uint(row.get(field), field)
    expected = command[2], int(command[1]), int(command[3]), int(command[4]), int(command[5]), command[6]
    require(tuple(row.get(field) for field in KEYS) == expected, "command/result tuple mismatch")
    n, r = row["n"], row["rectangles"]
    require(n >= 2 and 1 <= row["kmax"] <= 10 and row["separation_s"] > 0 and
            row["family"] in FAMILIES and row["order"] in ORDERS, "unsupported tuple")
    b = max(1, n // 16) if row["family"] == "skew" else n - n // 2
    a = n - b
    require(row["n_a"] == a and row["n_b"] == b and 1 <= r <= a, "wrong factors or partition count")
    require(row["fixture_version"] == (2 if row["family"] == "sheet_full" else 1), "wrong fixture version")
    fingerprint(row.get("input_fnv1a64_le_u16_xyz"), "input")
    finite_times(row, COMMON_TIMES)
    arms = row.get("arms")
    require(type(arms) is list and len(arms) == 2, "missing or duplicate arms")
    for mode, multiplicity, arm in zip(("fresh", "shared"), (r, 1), arms):
        require(type(arm) is dict and arm.get("mode") == mode, "incorrect arm order")
        finite_times(arm, ARM_TIMES)
        for field in LOCAL_FIELDS[:10] + ("cloud_preparations", "index_preparations"):
            uint(arm.get(field), f"{mode}.{field}")
        require(arm["cloud_preparations"] == arm["index_preparations"] == multiplicity and
                arm["rectangle_preparations"] == r, "wrong preparation multiplicity")
        cloud, index = arm.get("cloud_work"), arm.get("index_work")
        counters(cloud, CLOUD_FIELDS, "cloud_work")
        counters(index, INDEX_FIELDS, "index_work")
        require(cloud["coordinate_copies"] == cloud["validation_points"] == cloud["range_tree_leaf_visits"] == n * multiplicity and
                cloud["range_tree_nodes"] == (2 * n - 1) * multiplicity and
                cloud["range_tree_merges"] == cloud["uniqueness_adjacent_tests"] == (n - 1) * multiplicity and
                cloud["uniqueness_comparisons"] > 0, "cloud global work missing or nonlinear")
        require(index["nodes"] == index["escape_links"] == (2 * n - 1) * multiplicity and
                1 <= index["max_depth"] <= 48 and n * multiplicity <= index["point_visits"] <= 97 * n * multiplicity,
                "index global work outside its declared bound")
        logn = (n - 1).bit_length()
        require(2 * r <= arm["factor_box_visits"] <= r * (4 * logn + 4) and
                2 * r <= arm["factor_box_steps"] <= r * (2 * logn + 2), "factor query work missing or nonlogarithmic")
        require(6 * n <= arm["max_cloud_retained_bytes"] <= 128 * n and
                8 * n <= arm["max_index_retained_bytes"] <= 256 * n, "declared vector memory outside linear envelope")
        grouped_work(arm.get("preparation_work"), "preparation_work")
        require(all(value == 0 for group in arm["preparation_work"].values() for value in group.values()),
                "no-core shared rectangle repeated global preparation")
        grouped_work(arm.get("local_work"), "local_work")
        local = arm["local_work"]["counters"]
        require(local["validation_points"] == local["uniqueness_comparisons"] == local["dual_tasks"] == local["tube_records"] == 0,
                "local restriction is not a shared-cloud Pool plan")
        candidate, descriptors, accepted = arm["candidate_pairs"], arm["candidate_descriptors"], arm["accepted_pairs"]
        require(arm["total_pairs"] == a * b and descriptors <= candidate <= a * b and
                (candidate == 0) == (descriptors == 0) and accepted + arm["rejected_pairs"] == candidate,
                "residual or partition accounting mismatch")
        axis = arm.get("axis_work")
        counters(axis, (*AXIS_FIELDS, *ADDITIVE_FIELDS), "axis_work")
        require(axis["sort_passes"] == 3 * r and axis["sorted_sites"] == 3 * a and
                axis["emitted_blocks"] == descriptors and axis["restriction_credit_copies"] == a + r * b and
                axis["max_tree_depth"] <= 48 and
                axis["contained_nodes"] + axis["whole_factor_accepts"] == descriptors + axis["coalesced_blocks"],
                "axial local work differs from partition structure")
        census = arm.get("census_work")
        counters(census, COUNTERS, "census_work")
        require(census["input_descriptors"] == descriptors and
                census["query_tasks"] == census["count_root_starts"] == candidate and
                census["query_build_nodes"] == census["query_build_point_visits"] ==
                census["cursor_advances"] == census["cursor_reuses"] == census["query_splits"] ==
                census["frontier_restarts"] == 0 and arm["census_query_index_ms"] == 0 and
                census["count_node_visits"] == census["count_bound_tests"] + census["count_point_tests"] and
                census["payload_node_visits"] == census["payload_bound_tests"] + census["payload_point_tests"],
                "unexpected pairwise census work")
        output = arm.get("digest")
        require(type(output) is dict and set(output) == {"supports", "interior_ids", "shell_ids", "sum", "xor"},
                "wrong digest fields")
        for field in ("supports", "interior_ids", "shell_ids"):
            uint(output[field], field)
        fingerprint(output["sum"], "payload sum")
        fingerprint(output["xor"], "payload xor")
        require(output["supports"] == accepted == census["payload_supports"] and
                output["interior_ids"] == census["payload_interior_sites"] <= (row["kmax"] - 1) * accepted and
                output["shell_ids"] == census["payload_shell_sites"] and 2 * accepted <= output["shell_ids"] <= n * accepted,
                "materialized payload accounting mismatch")
        require(census["count_node_visits"] >= candidate and
                (candidate != 0 or all(value == 0 for value in census.values())), "census non-vacuity mismatch")
        close(arm["total_ms"], row["generation_ms"] + row["input_destruction_ms"] + arm["processing_ms"], "arm total")
        close(arm["census_total_ms"], sum(arm[name] for name in
              ("census_query_index_ms", "census_count_ms", "census_payload_ms")), "census partition")
        require(arm["consumption_ms"] + 1e-6 >= arm["census_total_ms"] and
                arm["processing_ms"] + 1e-6 >= sum(arm[name] for name in
                    ("cloud_ms", "index_ms", "rectangle_ms", "local_plan_ms", "axis_selection_ms", "consumption_ms", "destruction_ms")),
                "enclosing arm clock omitted work")
    fresh, shared = arms
    require(all(fresh[name] == shared[name] for name in LOCAL_FIELDS), "fresh/shared local work or output differs")
    require(all(fresh["cloud_work"][name] == r * shared["cloud_work"][name] for name in CLOUD_FIELDS) and
            all(fresh["index_work"][name] == (1 if name == "max_depth" else r) * shared["index_work"][name]
                for name in INDEX_FIELDS), "global work scaling differs")
    require(row["paired_execution_ms"] + 1e-6 >= row["generation_ms"] + row["input_destruction_ms"] +
            row["comparison_ms"] + sum(arm["processing_ms"] for arm in arms), "paired clock omitted work")


def matrix(manifest: dict[str, Any]) -> list[tuple[Any, ...]]:
    require(uint(manifest.get("repeats"), "repeats") >= 1, "empty repetitions")
    for name in MATRIX_KEYS:
        values = manifest.get(name)
        require(type(values) is list and bool(values), f"{name}: empty matrix")
        for value in values:
            if name in ("families", "orders"):
                require(type(value) is str and value in (FAMILIES if name == "families" else ORDERS), f"{name}: invalid value")
            else:
                uint(value, name)
                require(value >= (2 if name == "sizes" else 1) and (name != "kmax" or value <= 10), f"{name}: invalid value")
        require(len(set(values)) == len(values), f"{name}: duplicate value")
    cases = list(itertools.product(*(manifest[name] for name in MATRIX_KEYS), range(manifest["repeats"])))
    for family, n, _, _, rectangles, _, _ in cases:
        b = max(1, n // 16) if family == "skew" else n - n // 2
        require(rectangles <= n - b, "rectangle count exceeds A cardinality")
    return cases


def cross_check(row: dict[str, Any], identities: dict, signatures: dict, outputs: dict) -> None:
    identity = row["family"], row["n"]
    value = row["input_fnv1a64_le_u16_xyz"]
    require(identity not in identities or identities[identity] == value, "fixture identity changed")
    identities[identity] = value
    output_key = (*identity, row["kmax"])
    output = row["arms"][0]["digest"]
    require(output_key not in outputs or outputs[output_key] == output, "output changed across partitions, s or order")
    outputs[output_key] = output
    key = (*output_key, row["rectangles"])
    signature = [{name: arm[name] for name in (*LOCAL_FIELDS, "cloud_work", "index_work")}
                 for arm in row["arms"]]
    require(key not in signatures or signatures[key] == signature, "discrete work changed across s, order or repetition")
    signatures[key] = signature


def campaign_directories(root: Path) -> list[Path]:
    require(root.is_dir(), "receipt root is not a directory")
    directories = sorted({path.parent for name in RECEIPT_FILES for path in root.rglob(name)})
    require(bool(directories), "no cloud reuse campaigns")
    require(not any(parent in child.parents for parent in directories for child in directories), "nested campaign roots")
    for directory in directories:
        require(all((directory / name).is_file() for name in RECEIPT_FILES), f"incomplete campaign triplet: {directory}")
    return directories


def capture(args: argparse.Namespace) -> int:
    parameters = dict(families=args.families, sizes=args.sizes, kmax=args.kmax, separations=args.s,
                      rectangles=args.rectangles, orders=args.orders, repeats=args.repeats)
    cases = matrix(parameters)
    args.output.mkdir(parents=True, exist_ok=False)
    binary = args.probe.resolve()
    pins, binary_pin = {}, None
    attempts = completed = 0
    status, error, code = "failed", "campaign did not start", 1
    previous = {signum: signal.signal(signum, on_signal) for signum in (signal.SIGINT, signal.SIGTERM)}
    try:
        pins, binary_pin = sources(), digest(binary)
        cache = (binary.parent / "CMakeCache.txt").read_text()
        compilers = [line.split("=", 1)[1] for line in cache.splitlines() if line.startswith("CMAKE_CXX_COMPILER:") and "=" in line]
        require(len(compilers) == 1 and bool(compilers[0]), "missing compiler provenance")
        manifest = {**parameters, "schema": SCHEMA, "scope": SCOPE, "public_status": "not_claimed",
            "s_role": "rectangle_precondition_not_wspd_generation", "started_utc": utc_stamp(),
            "threads": 1, "gcp_used": False, "processes_sequential": True, "warmup_runs": 0,
            "probe": str(binary), "probe_sha256": binary_pin, "source_sha256": pins, "cmake_cache": cache,
            "compiler_version": subprocess.check_output([compilers[0], "--version"], text=True),
            "platform": platform.platform(), "logical_cpu_count": os.cpu_count(),
            "cpuinfo": Path("/proc/cpuinfo").read_text().split("\n\n", 1)[0],
            "commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
            "worktree_status": subprocess.check_output(["git", "status", "--short"], cwd=ROOT, text=True),
            "runner_command": [sys.executable, *sys.argv]}
        write_json(args.output / "MANIFEST.json", manifest)
        environment = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
        identities, signatures, outputs = {}, {}, {}
        with (args.output / "MEASURES.jsonl").open("x") as stream:
            for *key, repeat in cases:
                command = command_for(str(binary), tuple(key))
                record = dict(command=command, repeat=repeat, status="failed", exit_code=None,
                              stdout="", stderr="", stdout_base64="", stderr_base64="", started_utc=utc_stamp())
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
                    require(not record["stderr_base64"], "probe wrote unexpected stderr")
                    row = parse_result(base64.b64decode(record["stdout_base64"], validate=True))
                    validate_result(row, command)
                    cross_check(row, identities, signatures, outputs)
                    record.update(status="completed", result=row)
                    completed += 1
                except KeyboardInterrupt as cause:
                    record.update(status="interrupted", error=str(cause))
                    raise
                except Exception as cause:
                    record.update(status="invalid" if isinstance(cause, ValueError) else "failed", error=str(cause))
                    raise
                finally:
                    record["finished_utc"] = utc_stamp()
                    stream.write(json.dumps(record, allow_nan=False) + "\n")
                    stream.flush()
        status, error, code = "completed", "", 0
    except KeyboardInterrupt as cause:
        status, error, code = "interrupted", str(cause), 128 + getattr(cause, "signum", signal.SIGINT)
    except Exception as cause:
        status, error = "invalid" if isinstance(cause, ValueError) else "failed", str(cause)
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
        write_json(args.output / "COMPLETION.json", dict(status=status, error=error, runs=completed, attempts=attempts,
            finished_utc=utc_stamp(), source_hashes_unchanged=source_same, probe_hash_unchanged=binary_same,
            source_sha256_closing=closing, probe_sha256_closing=binary_closing))
        for signum, handler in previous.items():
            signal.signal(signum, handler)
    print(json.dumps(dict(status=status, runs=completed, attempts=attempts, error=error)))
    return code


def check(args: argparse.Namespace) -> int:
    directories = campaign_directories(args.receipt)
    pins = sources()
    identities, signatures, outputs, groups = {}, {}, {}, defaultdict(list)
    common_provenance = published = None
    measures = 0
    for directory in directories:
        manifest = parse_result((directory / "MANIFEST.json").read_bytes())
        completion = parse_result((directory / "COMPLETION.json").read_bytes())
        require(manifest.get("schema") == SCHEMA and manifest.get("scope") == SCOPE and
                manifest.get("public_status") == "not_claimed" and manifest.get("gcp_used") is False and
                manifest.get("s_role") == "rectangle_precondition_not_wspd_generation" and
                uint(manifest.get("threads"), "threads") == 1 and manifest.get("processes_sequential") is True and
                uint(manifest.get("warmup_runs"), "warmup_runs") == 0, "wrong campaign scope")
        require(manifest.get("source_sha256") == pins, "source coverage/hash differs from current version")
        sha256(manifest.get("probe_sha256"), "probe_sha256")
        require(type(manifest.get("probe")) is str and Path(manifest["probe"]).is_absolute(), "missing absolute binary path")
        require(type(manifest.get("commit")) is str and re.fullmatch(r"[0-9a-f]{40}", manifest["commit"]) is not None and
                type(manifest.get("worktree_status")) is str and type(manifest.get("runner_command")) is list and
                bool(manifest["runner_command"]) and all(type(arg) is str for arg in manifest["runner_command"]),
                "missing repository/capture provenance")
        build, machine, published = provenance(manifest)
        require(common_provenance is None or common_provenance == (build, machine), "heterogeneous build/machine provenance")
        common_provenance = build, machine
        start, end = timestamp(manifest.get("started_utc"), "campaign start"), timestamp(completion.get("finished_utc"), "campaign end")
        require(start <= end and completion.get("status") == "completed" and completion.get("source_hashes_unchanged") is True and
                completion.get("probe_hash_unchanged") is True and completion.get("source_sha256_closing") == pins and
                completion.get("probe_sha256_closing") == manifest["probe_sha256"], "invalid campaign closure")
        expected = set(matrix(manifest))
        records = [parse_result(line) for line in (directory / "MEASURES.jsonl").read_bytes().splitlines()]
        require(uint(completion.get("runs"), "runs") == uint(completion.get("attempts"), "attempts") ==
                len(records) == len(expected), "incomplete matrix")
        seen, previous_end = set(), start
        for record in records:
            begun, finished = timestamp(record.get("started_utc"), "invocation start"), timestamp(record.get("finished_utc"), "invocation end")
            require(previous_end <= begun <= finished <= end, "invocations are not serial inside campaign timestamps")
            previous_end = finished
            require(record.get("status") == "completed" and uint(record.get("exit_code"), "exit_code") == 0 and
                    record.get("stderr") == record.get("stderr_base64") == "" and
                    record.get("probe_sha256_before") == record.get("probe_sha256_after") == manifest["probe_sha256"],
                    "invalid invocation or binary pin")
            raw = base64.b64decode(record["stdout_base64"], validate=True)
            row = parse_result(raw)
            key = tuple(row[name] for name in KEYS)
            command = command_for(manifest["probe"], key)
            require(raw.decode() == record.get("stdout") and row == record.get("result") and
                    record.get("command") == command, "raw/parsed/command mismatch")
            validate_result(row, command)
            identity = (*key, uint(record.get("repeat"), "repeat"))
            require(identity not in seen, "duplicate invocation")
            seen.add(identity)
            cross_check(row, identities, signatures, outputs)
            groups[key].append(row)
            measures += 1
        require(seen == expected, "missing or extra matrix tuple")
    require(sources() == pins, "sources changed while reading")
    result = dict(status="passed", campaigns=len(directories), measurements=measures,
                  configurations_including_order=len(groups), scope=SCOPE, full_contract_qualified=False,
                  gcp_used=False, provenance_policy=PROVENANCE_POLICY, provenance=published,
                  current_reader_sha256=pins[RUNNER_SOURCE], source_check="current_content_and_closing_pins_not_a_rebuild")
    if args.summary:
        result["summary"] = []
        for key, rows in sorted(groups.items()):
            item = dict(zip(KEYS, key), repeats=len(rows), arms=[])
            for field in COMMON_TIMES:
                item[f"median_{field}"] = statistics.median(row[field] for row in rows)
            for index, mode in enumerate(("fresh", "shared")):
                arm = {name: rows[0]["arms"][index][name] for name in (*LOCAL_FIELDS, "cloud_work", "index_work")}
                arm.update(mode=mode, **{f"median_{field}": statistics.median(row["arms"][index][field] for row in rows)
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
    run.add_argument("--families", nargs="+", choices=FAMILIES, default=list(FAMILIES))
    run.add_argument("--kmax", nargs="+", type=int, default=[5, 10])
    run.add_argument("--s", nargs="+", type=int, default=[8, 10, 12])
    run.add_argument("--rectangles", nargs="+", type=int, default=[32])
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
        print(f"cloud reuse receipt rejected: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
