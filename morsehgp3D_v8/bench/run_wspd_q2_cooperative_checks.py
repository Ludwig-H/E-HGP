#!/usr/bin/env python3
"""Fresh cooperative q2 captures, strict closed receipts, never FULL claims.

Explicit structural-validator port from run_wspd_q2_parallel_matrix.py at
897085f8: the sole partition difference is all W workers when jobs exist.
Capture/read port from run_q2_split_checks.py; interruption-safe collector,
field contracts and source pinning are reused, never old qualifications.
"""
from __future__ import annotations

import argparse
import base64
from copy import deepcopy
import itertools
import json
import math
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from typing import Any

from run_p0_matrix import invoke, on_signal, utc_stamp, require, uint
from run_q2_split_checks import source_pins, digest, write_json
from run_q2_resume_checks import FIELDS as RESUME_FIELDS
from cooperative_source_paths import SOURCE_PATHS, ARTIFACT_NAMES
from paired_receipts import close, finite_times
from run_wspd_q2_parallel_matrix import (
    FIXED, KEYS, DISCRETE_FIELDS, POOL_FIELDS, POOL_TIMES, SIBLING_FIELDS,
    ORDER_FIELDS, JOINT_FIELDS, PARALLEL_FIELDS, WORKER_FIELDS, CALLBACK_FIELDS,
    TIMES, MATRIX_KEYS, counters, matrix, validate_geometry, validate_digest,
    cross_check, validate_result as validate_reference)

ROOT = Path(__file__).resolve().parents[2]
FAMILIES = ("uniform", "terrain", "clusters", "rows")
COOPERATIVE = """continued_anchors continued_pairs completed_pairs fragments_started
completed_fragments donations donations_after_seeds_exhausted offer_checks
offer_busy offer_full offer_no_waiter offer_no_sibling waits wakes
max_queue_size max_active_tasks max_fragment_bytes""".split()
MAXIMA = ("max_queue_size", "max_active_tasks", "max_fragment_bytes")
RESUME = RESUME_FIELDS["resume_work"].split()
DETACH = "attempts detached_frames imported_frames transferred_pairs moved_frames".split()


def strict_json(text):
    def unique(pairs):
        result = {}
        for key, value in pairs:
            require(key not in result, "duplicate JSON key")
            result[key] = value
        return result
    def invalid(value):
        raise RuntimeError("nonfinite JSON token " + value)
    def finite(value):
        number = float(value)
        require(math.isfinite(number), "nonfinite JSON number")
        return number
    return json.loads(text, object_pairs_hook=unique, parse_constant=invalid, parse_float=finite)


def validate_pins(manifest):
    sources, artifacts = manifest["source_sha256"], manifest["artifact_sha256"]
    require(type(sources) is dict and set(sources) == SOURCE_PATHS, "source pin inventory mismatch")
    require(type(artifacts) is dict and bool(artifacts), "missing artifact pins")
    for pins in (sources, artifacts):
        require(all(type(value) is str and re.fullmatch(r"[0-9a-f]{64}", value)
                    for value in pins.values()), "malformed SHA256 pin")
    build = Path(manifest["build"])
    require(build.is_absolute() and build.is_relative_to(ROOT) and ".." not in build.parts,
            "build escaped repository")
    build_relative = build.relative_to(ROOT)
    expected = {"mhgp8_wspd_q2_cooperative_gate"} if manifest["campaign"] == "tsan" else ARTIFACT_NAMES
    require(set(artifacts) == {str(build_relative / name) for name in {*expected, "CMakeCache.txt"}},
            "artifact inventory does not close all tests/probes")
    require(str(build_relative / "CMakeCache.txt") in artifacts, "missing CMake cache pin")
    for name in artifacts:
        path = Path(name)
        require(path.parent == build_relative and (path.name == "CMakeCache.txt" or
                re.fullmatch(r"mhgp8_[a-z0-9_]+", path.name)), "artifact escaped build")
    for kind, command in manifest["planned_commands"]:
        if kind != "ctest":
            executable = Path(command[0])
            require(executable.is_relative_to(ROOT) and
                    str(executable.relative_to(ROOT)) in artifacts, "command executable is not pinned")


def validate_pipeline(row: dict[str, Any], command: list[str]) -> None:
    expected_fields = {*FIXED, *KEYS, *DISCRETE_FIELDS, "gcp_used", "execution", "parallel_work",
                       "pool_work", "workers", "memory", "timings"}
    require(set(row) == expected_fields, "missing or unknown parallel probe fields")
    for key, value in FIXED.items():
        require(row.get(key) == value, f"{key}: unsupported contract")
    require(row["gcp_used"] is False and len(command) == 9, "wrong cloud use or command")
    for key in KEYS:
        if key != "family":
            uint(row[key], key)
    expected = (command[2], int(command[1]), int(command[3]), int(command[4]), int(command[5]),
                int(command[6]), int(command[7]), int(command[8]))
    require(tuple(row[key] for key in KEYS) == expected, "command/result tuple mismatch")
    matrix(dict(zip(MATRIX_KEYS, ([row[key]] for key in KEYS)), repeats=1))
    require(row["execution"] == ("mono_reference" if row["threads"] == 0 else "parallel_front"),
            "wrong reference/parallel execution mode")
    for key in ("total_unordered_pairs", "active_lane_mask"):
        uint(row[key], key)
    pool = row["pool_work"]
    require(type(pool) is dict and set(pool) == {*POOL_FIELDS, *POOL_TIMES}, "wrong Pool fields")
    counters({key: pool[key] for key in POOL_FIELDS}, POOL_FIELDS, "pool_work")
    finite_times(pool, POOL_TIMES)
    validate_geometry(row, pool)
    require(pool["selected_rectangles"] <= row["input_rectangles"] and
            pool["passthrough_rectangles"] <= pool["selected_rectangles"] and
            pool["passthrough_anchors"] <= pool["original_selected_anchors"] <= row["anchor_queries"] and
            pool["passthrough_pairs"] <= pool["residual_pairs"] <= pool["selected_pairs"] and
            pool["selected_pairs"] <= row["front_work"]["residual_pair_mass"][0] and
            pool["filtered_pairs"] == pool["selected_pairs"] - pool["residual_pairs"] and
            pool["pair_roots"] == pool["residual_pairs"] - pool["passthrough_pairs"],
            "Pool selected or passthrough mass mismatch")
    fsites = pool["factor_sites"]
    require(pool["factor_read_visits"] == pool["grouping_visits"] == 2 * fsites and
            pool["prefix_class_visits"] == (row["kmax"] + 1) * pool["selected_rectangles"] and
            pool["bands"] <= row["kmax"] * pool["selected_rectangles"] and
            pool["bands"] <= pool["selected_anchors"] <= pool["original_selected_anchors"] - pool["passthrough_anchors"] and
            pool["selected_anchors"] <= pool["pair_roots"] and
            (pool["selected_anchors"] == 0) == (pool["pair_roots"] == 0) and
            pool["selection_tests"] <= (row["kmax"] + 1) * fsites and
            pool["witness_attempts"] <= (row["kmax"] + 1) * fsites and
            pool["universal_queries"] <= pool["witness_attempts"] and
            pool["q2_axis_terms"] == 3 * pool["universal_queries"] and
            pool["pool_insertions"] <= fsites and
            pool["pool_shifted_entries"] <= (row["kmax"] + 1) * pool["pool_insertions"],
            "Pool work or band accounting mismatch")
    if row["pool_min_factor"] == 0 or pool["selected_rectangles"] == 0:
        require(all(value == 0 for value in pool.values()), "disabled/unselected Pool performed work")
    census, sibling, order = row["census_work"], row["sibling_work"], row["order_work"]
    counters(sibling, SIBLING_FIELDS, "sibling_work")
    counters(order, ORDER_FIELDS, "order_work")
    counters(row["joint_work"], JOINT_FIELDS, "joint_work")
    require(all(value == 0 for value in row["joint_work"].values()), "fixed individual policy used joint work")
    require(sibling["proposals"] == 2 * census["query_splits"] and
            sibling["cardinality_skips"] + sibling["bound_tests"] == sibling["proposals"] and
            sibling["rejected_tasks"] <= sibling["bound_tests"] and
            sibling["rejected_tasks"] <= sibling["rejected_pairs"] <= row["rejected_pairs"] and
            sibling["rejected_after_credit"] <= sibling["rejected_tasks"] and
            (sibling["rejected_tasks"] == 0) == (sibling["rejected_pairs"] == 0), "sibling accounting mismatch")
    require(order["structural_splits"] <= 96 * census["query_tasks"] and
            all(order[key] <= census["query_tasks"] for key in ORDER_FIELDS[1:]), "order accounting mismatch")
    parallel = row["parallel_work"]
    counters(parallel, PARALLEL_FIELDS, "parallel_work")
    require(parallel["requested_workers"] == row["threads"] and
            parallel["completed_jobs"] == parallel["jobs"], "requested workers or completed jobs mismatch")
    workers = row["workers"]
    require(type(workers) is list and len(workers) == parallel["started_workers"], "missing worker slots")
    if row["threads"] == 0:
        require(parallel["started_workers"] == 1 and all(parallel[key] == 0 for key in
                ("target_jobs", "jobs", "completed_jobs", "terminal_jobs", "prefix_product_visits", "job_storage_bytes")),
                "mono reference reported scheduler work")
    else:
        require(parallel["target_jobs"] == row["threads"] * row["jobs_per_worker"] and
                1 <= parallel["jobs"] <= parallel["target_jobs"] + 2 and
                parallel["started_workers"] == row["threads"] and
                parallel["terminal_jobs"] <= parallel["jobs"] and
                parallel["terminal_jobs"] <= parallel["prefix_product_visits"] <= row["front_work"]["product_visits"] and
                parallel["job_storage_bytes"] >= parallel["jobs"], "invalid parallel partition")
    for i, worker in enumerate(workers):
        require(type(worker) is dict and set(worker) == {*WORKER_FIELDS, "elapsed_ms", "payload_ms", "callback_work", "digest"},
                "wrong worker fields")
        counters({key: worker[key] for key in WORKER_FIELDS}, WORKER_FIELDS, "worker")
        finite_times(worker, ("elapsed_ms", "payload_ms"))
        require(worker["slot"] == i and worker["elapsed_ms"] + 1e-6 >= worker["payload_ms"], "worker slot/clock mismatch")
        validate_digest(worker["digest"], worker["callback_work"], "worker.digest", row["kmax"], row["n"])
        require(worker["supports"] == worker["digest"]["supports"] and
                (worker["supports"] == 0 or worker["callback_buffers_capacity_bytes"] >= 16), "worker payload mismatch")
    for per_worker, expected_total in (("jobs", parallel["jobs"]), ("front_products",
            row["front_work"]["product_visits"] - parallel["prefix_product_visits"]),
            ("input_rectangles", row["input_rectangles"]), ("count_node_visits", census["count_node_visits"]),
            ("supports", row["accepted_pairs"]), ("pool_peak_bytes", parallel["pool_peak_bytes_sum"]),
            ("callback_buffers_capacity_bytes", row["memory"]["callback_buffers_capacity_bytes"])):
        require(sum(worker[per_worker] for worker in workers) == expected_total, "worker reduction lost " + per_worker)
    require(max((worker["pool_peak_bytes"] for worker in workers), default=0) == pool["plan_peak_bytes"],
            "maximum plan storage differs from worker maxima")
    for key in CALLBACK_FIELDS:
        require(sum(worker["callback_work"][key] for worker in workers) == row["callback_work"][key],
                "callback worker reduction lost " + key)
    for key in ("supports", "interior_ids", "shell_ids"):
        require(sum(worker["digest"][key] for worker in workers) == row["digest"][key], "digest reduction lost " + key)
    summed, xored = 0, 0
    for worker in workers:
        summed = (summed + int(worker["digest"]["sum"], 16)) & ((1 << 64) - 1)
        xored ^= int(worker["digest"]["xor"], 16)
    require(summed == int(row["digest"]["sum"], 16) and xored == int(row["digest"]["xor"], 16),
            "worker canonical digest reduction mismatch")
    require(row["memory"]["callback_slots"] == max(1, row["threads"]) and
            row["memory"]["callback_state_bytes"] >= row["memory"]["callback_slots"], "callback state storage missing")
    timings = row["timings"]
    require(type(timings) is dict and set(timings) == set(TIMES), "wrong parallel timing fields")
    finite_times(timings, TIMES)
    close(timings["total_ms"], sum(timings[key] for key in TIMES[:6]), "outer clock")
    close(timings["worker_ms_sum"], sum(worker["elapsed_ms"] for worker in workers), "worker intervals")
    close(timings["payload_ms_sum"], sum(worker["payload_ms"] for worker in workers), "worker payload")
    require(timings["pipeline_and_callback_ms"] + 1e-6 >= timings["pipeline_wall_ms"] >= timings["partition_ms"] and
            all(worker["elapsed_ms"] <= timings["pipeline_wall_ms"] + 1e-6 for worker in workers) and
            pool["preparation_ms_sum"] <= pool["selected_total_ms_sum"] + 1e-6 and
            pool["selected_total_ms_sum"] <= timings["worker_ms_sum"] + 1e-6,
            "worker/partition/Pool clocks escaped enclosing execution")
    if row["threads"] == 0:
        require(timings["partition_ms"] == 0, "mono paid a parallel partition")
        close(timings["pipeline_wall_ms"], timings["worker_ms_sum"], "mono elapsed interval")


def validate_work(work, row, global_work):
    require(type(work) is dict and set(work) == {*COOPERATIVE, "resume_work", "detach_work"},
            "cooperative fields mismatch")
    counters({key: work[key] for key in COOPERATIVE}, COOPERATIVE, "cooperative")
    r, d = work["resume_work"], work["detach_work"]
    counters(r, RESUME, "resume")
    counters(d, DETACH, "detach")
    require(r["transitions"] == sum(r[key] for key in
            ("entry_steps", "witness_steps", "admission_steps", "payload_steps")) and
            r["admission_steps"] <= r["payload_steps"] and
            r["advance_calls"] == r["pauses"] + work["completed_fragments"] and
            r["advance_calls"] >= (r["transitions"] + row["quantum"] - 1) // row["quantum"],
            "continuation transition ledger mismatch")
    require(all(r[key] <= r["pauses"] for key in
            ("pauses_after_credit", "pauses_inside_deferred", "pauses_during_emission")) and
            r["max_pending_tasks"] <= 49 and
            work["max_queue_size"] <= row["queue_capacity"] and
            work["max_active_tasks"] <= row["threads"], "continuation state outside bounds")
    require(work["offer_checks"] == work["offer_busy"] + work["offer_full"] +
            work["offer_no_waiter"] + d["attempts"] and
            d["attempts"] == work["donations"] + work["offer_no_sibling"] and
            d["detached_frames"] == work["donations"] and
            work["donations_after_seeds_exhausted"] <= work["donations"] and
            work["fragments_started"] == work["completed_fragments"] and
            work["waits"] == work["wakes"], "cooperative obligation ledger mismatch")
    require(work["fragments_started"] == work["continued_anchors"] + d["imported_frames"] and
            work["completed_pairs"] >= work["completed_fragments"] and
            work["offer_checks"] <= r["pauses"], "worker fragment provenance mismatch")
    require(d["detached_frames"] <= d["moved_frames"] <= 48 * d["detached_frames"] and
            d["transferred_pairs"] >= d["detached_frames"], "detachment traffic mismatch")
    if work["completed_fragments"]:
        require(work["max_fragment_bytes"] >= 6272 and r["max_pending_tasks"] > 0,
                "missing continuation capacity")
    else:
        require(work["max_fragment_bytes"] == 0 and all(v == 0 for v in r.values()) and
                all(v == 0 for v in d.values()), "unused continuation performed work")
    if row["threads"] == 1:
        require(work["offer_checks"] == work["donations"] == 0, "single worker donated")
    if global_work:
        require(work["continued_pairs"] == work["completed_pairs"] and
                work["completed_fragments"] == work["continued_anchors"] + work["donations"] and
                d["imported_frames"] == work["donations"] and
                work["continued_anchors"] <= row["anchor_queries"] and
                work["continued_pairs"] <= row["candidate_pairs"] and
                work["donations"] <= row["census_work"]["query_splits"] and
                r["entry_steps"] <= row["census_work"]["query_tasks"] and
                r["payload_steps"] <= row["accepted_pairs"], "global continuation mass mismatch")
        require((work["continued_anchors"] == 0) == (work["continued_pairs"] == 0) and
                work["continued_anchors"] * row["min_b_size"] <= work["continued_pairs"],
                "continued root selection mismatch")
        require((work["continued_anchors"] == 0) == (work["completed_fragments"] == 0) and
                (work["donations"] == 0) == (work["max_queue_size"] == 0),
                "rootless lineage or queue without donation")
        if row["pool_min_factor"] and row["pool_min_factor"] <= row["min_b_size"]:
            require(work["continued_anchors"] == 0, "synchronous Pool escaped into continuation")


def validate_row(row, args):
    require(len(args) == 11, "cooperative command arity")
    n, family, k, separation, seed, workers, jobs, quantum, queue, min_b, pool = args
    expected = (int(n), family, int(k), int(separation), int(seed), int(workers),
                int(jobs), int(quantum), int(queue), int(min_b), int(pool))
    keys = ("n", "family", "kmax", "s", "seed", "threads", "jobs_per_worker",
            "quantum", "queue_capacity", "min_b_size", "pool_min_factor")
    require(type(row) is dict and tuple(row.get(key) for key in keys) == expected,
            "cooperative command/result mismatch")
    for key in keys:
        if key != "family":
            uint(row[key], key)
    require(all(row[key] > 0 for key in ("threads", "quantum", "queue_capacity", "min_b_size")),
            "positive cooperative options required")
    require(row.get("schema") == "mhgp8_wspd_q2_cooperative_probe_v1" and
            row.get("execution") == "cooperative_front_census" and
            row.get("worker_clock_scope") == "presence_including_waits", "cooperative scope mismatch")
    # A projection of field contracts, NOT a historical receipt or qualification.
    # The local port above explicitly admits W>jobs, unlike the old scheduler.
    projected = deepcopy(row)
    projected["schema"], projected["execution"] = FIXED["schema"], "parallel_front"
    for key in ("quantum", "queue_capacity", "min_b_size", "worker_clock_scope", "cooperative_work"):
        del projected[key]
    queue_bytes = projected["parallel_work"].pop("queue_storage_bytes")
    uint(queue_bytes, "queue_storage_bytes")
    require(queue_bytes >= 8 * row["queue_capacity"], "missing census queue allocation")
    for worker in projected["workers"]:
        del worker["cooperative_work"]
    validate_pipeline(projected, ["projected-probe", n, family, k, separation, seed, workers, jobs, pool])
    global_work = row["cooperative_work"]
    validate_work(global_work, row, True)
    for worker in row["workers"]:
        validate_work(worker["cooperative_work"], row, False)
    for group, fields in ((None, COOPERATIVE), ("resume_work", RESUME), ("detach_work", DETACH)):
        total = global_work if group is None else global_work[group]
        values = [w["cooperative_work"] if group is None else w["cooperative_work"][group] for w in row["workers"]]
        for key in fields:
            maximum = key in MAXIMA or (group == "resume_work" and key == "max_pending_tasks")
            combined = max((v[key] for v in values), default=0) if maximum else sum(v[key] for v in values)
            require(total[key] == combined, "cooperative worker reduction lost " + key)
    return row


def validate_ctest(path):
    suite = ET.fromstring(path.read_bytes())
    cases = list(suite.iter("testcase"))
    require(suite.tag == "testsuite" and len(cases) == len({c.attrib["name"] for c in cases}) == 69 and
            int(suite.attrib["tests"]) == 69 and int(suite.attrib["failures"]) == 0 and
            int(suite.attrib.get("errors", 0)) == int(suite.attrib.get("skipped", 0)) == 0 and
            all(c.attrib["name"].startswith("mhgp8_") and c.find("failure") is None and
                c.find("error") is None and c.find("skipped") is None for c in cases),
            "expected 69 distinct passing CTests")


def validate_pairs(rows):
    identities, signatures = {}, {}
    for row in rows:
        cross_check(row, identities, signatures)


def plan(build, output, campaign):
    probe = str(build / "mhgp8_wspd_q2_cooperative_probe")
    reference = str(build / "mhgp8_wspd_q2_parallel_probe")
    if campaign == "qualification":
        return [("ctest", ["ctest", "--test-dir", str(build), "--output-on-failure",
                           "--output-junit", str(output / "ctest.xml")]),
                ("gate", [str(build / "mhgp8_wspd_q2_cooperative_gate"), "--selftest"])]
    if campaign == "tsan":
        return [("gate", [str(build / "mhgp8_wspd_q2_cooperative_gate"), "--selftest"])]
    if campaign == "scale":
        configurations = itertools.product((8000, 16000, 32000), FAMILIES, (10,), (8,), (64,))
    elif campaign == "separations":
        configurations = itertools.product((8000,), FAMILIES, (5, 10), (8, 10, 12), (64,))
    elif campaign == "rows_large":
        configurations = itertools.product((8000, 16000, 32000), ("rows",), (5, 10), (8, 10, 12), (0,))
    elif campaign == "smoke":
        configurations = itertools.product((32,), FAMILIES, (5,), (8,), (0,))
    else:
        raise RuntimeError("unknown campaign")
    commands = []
    for n, family, k, s, pool in configurations:
        common = [str(n), family, str(k), str(s), "3"]
        commands.append(("reference", [reference, *common, "4", "16", str(pool)]))
        for w in (1, 4):
            commands.append(("measure", [probe, *common, str(w), "16", "256", "8",
                                         "64" if campaign == "rows_large" else "16", str(pool)]))
    return commands


def run(args):
    build = args.build.resolve()
    require(build.is_relative_to(ROOT) and (build / "CMakeCache.txt").is_file(), "build must exist inside repository")
    args.output.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix=args.campaign + "_", dir=args.output.resolve()))
    environment = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    sources = source_pins()
    artifacts = {str(p.relative_to(ROOT)): digest(p) for p in sorted(build.glob("mhgp8_*"))
                 if p.is_file() and os.access(p, os.X_OK)}
    artifacts[str((build / "CMakeCache.txt").relative_to(ROOT))] = digest(build / "CMakeCache.txt")
    commands = plan(build, output, args.campaign)
    manifest = dict(schema="mhgp8_q2_cooperative_attempt_v1", started_utc=utc_stamp(), build=str(build),
                    campaign=args.campaign, command=[sys.executable, *sys.argv],
                    source_sha256=sources, artifact_sha256=artifacts,
                    planned_commands=commands, affinity=sorted(os.sched_getaffinity(0)),
                    cmake_cache=(build / "CMakeCache.txt").read_text(),
                    commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                    worktree_status=subprocess.check_output(["git", "status", "--short"], cwd=ROOT, text=True),
                    environment={key: environment.get(key) for key in
                                 ("ASAN_OPTIONS", "UBSAN_OPTIONS", "TSAN_OPTIONS", "OMP_NUM_THREADS")},
                    public_status="not_claimed", gcp_used=False, full_contract_qualified=False)
    write_json(output / "MANIFEST.json", manifest)
    records = []
    rows = []
    status, error = "failed", "not started"
    try:
        validate_pins(manifest)
        for number, (kind, command) in enumerate(commands):
            print(json.dumps({"capture": str(output), "number": number, "command": command}), flush=True)
            record = dict(kind=kind, command=command, cwd=str(ROOT), started_utc=utc_stamp(),
                          status="failed", exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="")
            try:
                invoke(command, environment, ROOT, record, new_session=True)
                require(record["exit_code"] == 0, "command failed")
                if kind in ("measure", "reference"):
                    parsed = strict_json(record["stdout"])
                    if kind == "measure":
                        validate_row(parsed, command[1:])
                    else:
                        validate_reference(parsed, command)
                    record["row"] = parsed
                    rows.append(parsed)
                if kind == "gate":
                    parsed = strict_json(record["stdout"])
                    require(parsed.get("status") == "passed", "gate did not pass")
                if kind == "ctest":
                    validate_ctest(output / "ctest.xml")
                record["status"] = "passed"
            finally:
                record["finished_utc"] = utc_stamp()
                path = output / f"record_{number:04}.json"
                write_json(path, record)
                records.append({"path": path.name, "sha256": digest(path)})
        validate_pairs(rows)
        require(source_pins() == sources and all(digest(ROOT / path) == value for path, value in artifacts.items()),
                "sources or binaries changed during capture")
        status, error = "passed", None
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        extra = {}
        if (output / "ctest.xml").is_file():
            extra["ctest.xml"] = digest(output / "ctest.xml")
        write_json(output / "COMPLETION.json", dict(status=status, error=error, finished_utc=utc_stamp(),
            manifest_sha256=digest(output / "MANIFEST.json"), records=records, extra_sha256=extra,
            source_sha256_after=source_pins(),
            artifact_sha256_after={p: digest(ROOT / p) for p in artifacts}))
        for sig, handler in previous.items():
            signal.signal(sig, handler)
    read(output)
    return 0


def read(path):
    manifest = strict_json((path / "MANIFEST.json").read_text())
    completion = strict_json((path / "COMPLETION.json").read_text())
    require(manifest["schema"] == "mhgp8_q2_cooperative_attempt_v1" and completion["status"] == "passed" and
            completion["error"] is None and
            completion["manifest_sha256"] == digest(path / "MANIFEST.json"), "capture incomplete or corrupt")
    require(manifest["public_status"] == "not_claimed" and manifest["gcp_used"] is False and
            manifest["full_contract_qualified"] is False, "capture changed scope")
    require(manifest["source_sha256"] == completion["source_sha256_after"] and
            manifest["artifact_sha256"] == completion["artifact_sha256_after"], "closure hash mismatch")
    commands = manifest["planned_commands"]
    expected_commands = plan(Path(manifest["build"]), path.resolve(), manifest["campaign"])
    require(commands == [[kind, command] for kind, command in expected_commands], "unrecognized qualification commands")
    validate_pins(manifest)
    require(len(commands) == len(completion["records"]) == len(expected_commands), "missing records")
    require({p.name for p in path.glob("record_*.json")} ==
            {f"record_{i:04}.json" for i in range(len(commands))}, "orphan or missing record")
    rows = []
    for number, ((kind, command), info) in enumerate(zip(commands, completion["records"], strict=True)):
        require(info["path"] == f"record_{number:04}.json", "invalid or repeated record path")
        record_path = path / info["path"]
        require(digest(record_path) == info["sha256"], "record hash mismatch")
        record = strict_json(record_path.read_text())
        require(record["kind"] == kind and record["command"] == command and
                record["status"] == "passed" and type(record["exit_code"]) is int and
                record["exit_code"] == 0 and record["cwd"] == str(ROOT), "command/result mismatch")
        for channel in ("stdout", "stderr"):
            require(base64.b64decode(record[channel + "_base64"], validate=True).decode("utf-8", errors="replace") ==
                    record[channel], "raw/decoded log mismatch")
        if kind in ("measure", "reference"):
            row = strict_json(record["stdout"])
            if kind == "measure":
                validate_row(row, command[1:])
            else:
                validate_reference(row, command)
            require(row == record["row"], "parsed row changed")
            rows.append(row)
        if kind == "gate":
            require(strict_json(record["stdout"]).get("status") == "passed", "gate did not pass")
    validate_pairs(rows)
    if manifest["campaign"] == "qualification":
        require(completion["extra_sha256"].get("ctest.xml") == digest(path / "ctest.xml"), "CTest hash mismatch")
        validate_ctest(path / "ctest.xml")
    print(json.dumps(dict(status="passed", path=str(path), records=len(commands),
                          q2_measures=len(rows), full_contract_qualified=False), sort_keys=True))



def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    capture = sub.add_parser("run")
    capture.add_argument("--build", type=Path, required=True)
    capture.add_argument("--output", type=Path, required=True)
    capture.add_argument("--campaign", choices=("qualification", "tsan", "smoke", "scale", "separations", "rows_large"),
                         default="qualification")
    reader = sub.add_parser("read")
    reader.add_argument("path", type=Path)
    args = parser.parse_args()
    if args.operation == "run":
        return run(args)
    read(args.path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
