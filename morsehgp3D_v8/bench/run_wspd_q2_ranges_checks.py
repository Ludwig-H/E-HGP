#!/usr/bin/env python3
"""Fresh anchor-range q2 captures and strict, failure-preserving closed receipts.

Explicit collector/pin port from the cooperative runner at beee3341.
Its pipeline field validator is imported as a contract projection only:
same W>jobs semantics, no old qualification, source hash or timing inherited.
"""
from __future__ import annotations

import argparse
import base64
from copy import deepcopy
import itertools
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
import json

from run_p0_matrix import invoke, on_signal, utc_stamp, require, uint
from run_q2_split_checks import source_pins, digest, write_json
from ranges_source_paths import SOURCE_PATHS, ARTIFACT_NAMES
from run_wspd_q2_parallel_matrix import POOL_FIELDS
from run_wspd_q2_cooperative_checks import (
    strict_json, validate_pipeline, validate_reference, FIXED, cross_check, counters)

ROOT = Path(__file__).resolve().parents[2]
FAMILIES = ("uniform", "terrain", "clusters", "rows")

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
    expected = {"mhgp8_wspd_q2_ranges_gate"} if manifest["campaign"] == "tsan" else ARTIFACT_NAMES
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


RANGE_FIELDS = """initial_ranges completed_ranges received_ranges initial_anchors completed_anchors initial_pairs completed_pairs initial_shared_ranges initial_pool_ranges initial_passthrough_ranges donations shared_donations pool_donations passthrough_donations donated_anchors donated_pairs donations_after_seeds_exhausted offer_checks offer_busy offer_full offer_no_waiter waits wakes max_queue_size max_active_tasks""".split()
MAXIMA = ("max_queue_size", "max_active_tasks")


def validate_work(work, row, global_work):
    counters(work, RANGE_FIELDS, "range_work")
    require(work["completed_ranges"] == work["initial_ranges"] + work["received_ranges"] and
            work["initial_ranges"] == work["initial_shared_ranges"] + work["initial_pool_ranges"] +
            work["initial_passthrough_ranges"] and
            work["offer_checks"] == work["offer_busy"] + work["offer_full"] +
            work["offer_no_waiter"] + work["donations"] and
            work["waits"] == work["wakes"] and
            work["donations"] == work["shared_donations"] + work["pool_donations"] +
            work["passthrough_donations"], "range worker lineage mismatch")
    require(work["donations_after_seeds_exhausted"] <= work["donations"] and
            work["donations"] <= work["donated_anchors"] <= work["donated_pairs"] and
            work["initial_ranges"] <= work["initial_anchors"] <= work["initial_pairs"] and
            work["completed_ranges"] <= work["completed_anchors"] <= work["completed_pairs"] and
            work["max_queue_size"] <= row["queue_capacity"] and
            work["max_active_tasks"] <= row["threads"], "range quantities outside bounds")
    if row["threads"] == 1:
        require(work["offer_checks"] == work["donations"] == work["waits"] == 0,
                "single worker donated or waited")
    if global_work:
        pool = row["pool_work"]
        require(work["initial_anchors"] == work["completed_anchors"] and
                work["initial_pairs"] == work["completed_pairs"] == row["candidate_pairs"] and
                work["received_ranges"] == work["donations"] and
                work["initial_shared_ranges"] == row["input_rectangles"] - pool["selected_rectangles"] and
                work["initial_pool_ranges"] == pool["bands"] and
                work["initial_passthrough_ranges"] == pool["passthrough_rectangles"] and
                work["initial_anchors"] == row["anchor_queries"] - pool["original_selected_anchors"] +
                pool["selected_anchors"] + pool["passthrough_anchors"],
                "range global mass, selection or preparation mismatch")
        require((work["donations"] == 0) == (work["max_queue_size"] == 0),
                "queue without donation")


def validate_row(row, args):
    require(len(args) == 10, "range command arity")
    n, family, k, separation, seed, workers, jobs, grain, queue, pool = args
    expected = (int(n), family, int(k), int(separation), int(seed), int(workers),
                int(jobs), int(grain), int(queue), int(pool))
    keys = ("n", "family", "kmax", "s", "seed", "threads", "jobs_per_worker",
            "anchor_grain", "queue_capacity", "pool_min_factor")
    require(type(row) is dict and tuple(row.get(key) for key in keys) == expected,
            "range command/result mismatch")
    for key in keys:
        if key != "family":
            uint(row[key], key)
    require(all(row[key] > 0 for key in ("threads", "anchor_grain", "queue_capacity")),
            "positive range options required")
    require(row.get("schema") == "mhgp8_wspd_q2_ranges_probe_v1" and
            row.get("execution") == "anchor_ranges_front_census" and
            row.get("worker_clock_scope") == "presence_including_waits" and
            row.get("pool_clock_scope") == "preparation_plus_active_range_intervals",
            "range scope mismatch")
    # Explicit field-contract projection, not a historical result. Imported
    # cooperative validator admits all W workers whenever front jobs exist.
    projected = deepcopy(row)
    projected["schema"], projected["execution"] = FIXED["schema"], "parallel_front"
    for key in ("anchor_grain", "queue_capacity", "worker_clock_scope", "pool_clock_scope",
                "range_work", "range_storage"):
        del projected[key]
    queue_bytes = projected["parallel_work"].pop("queue_storage_bytes")
    uint(queue_bytes, "queue_storage_bytes")
    for worker in projected["workers"]:
        del worker["range_work"]
    validate_pipeline(projected, ["projected-probe", n, family, k, separation, seed, workers, jobs, pool])
    storage = row["range_storage"]
    counters(storage, ("task_bytes", "max_live_pool_parents", "max_live_pool_bytes"), "range_storage")
    require(storage["task_bytes"] > 0 and queue_bytes >= row["queue_capacity"] * storage["task_bytes"],
            "range queue allocation missing")
    require(storage["max_live_pool_parents"] <= row["queue_capacity"] + row["threads"] and
            storage["max_live_pool_parents"] <= row["pool_work"]["selected_rectangles"] and
            storage["max_live_pool_bytes"] >= row["pool_work"]["plan_peak_bytes"] and
            (storage["max_live_pool_parents"] == 0) == (storage["max_live_pool_bytes"] == 0) and
            (storage["max_live_pool_parents"] == 0) == (row["pool_work"]["selected_rectangles"] == 0),
            "distinct live Pool parent storage mismatch")
    total = row["range_work"]
    validate_work(total, row, True)
    for worker in row["workers"]:
        validate_work(worker["range_work"], row, False)
    for key in RANGE_FIELDS:
        parts = [worker["range_work"][key] for worker in row["workers"]]
        combined = max(parts, default=0) if key in MAXIMA else sum(parts)
        require(total[key] == combined, "range worker reduction lost " + key)
    return row


def validate_ctest(path):
    suite = ET.fromstring(path.read_bytes())
    cases = list(suite.iter("testcase"))
    require(suite.tag == "testsuite" and len(cases) == len({c.attrib["name"] for c in cases}) == 72 and
            int(suite.attrib["tests"]) == 72 and int(suite.attrib["failures"]) == 0 and
            int(suite.attrib.get("errors", 0)) == int(suite.attrib.get("skipped", 0)) == 0 and
            all(c.attrib["name"].startswith("mhgp8_") and c.find("failure") is None and
                c.find("error") is None and c.find("skipped") is None for c in cases),
            "expected 72 distinct passing CTests")


def validate_pairs(rows):
    identities, signatures = {}, {}
    for row in rows:
        cross_check(row, identities, signatures)


def plan(build, output, campaign):
    probe = str(build / "mhgp8_wspd_q2_ranges_probe")
    reference = str(build / "mhgp8_wspd_q2_parallel_probe")
    if campaign == "qualification":
        return [("ctest", ["ctest", "--test-dir", str(build), "--parallel", "2", "--output-on-failure",
                           "--output-junit", str(output / "ctest.xml")]),
                ("gate", [str(build / "mhgp8_wspd_q2_ranges_gate"), "--selftest"])]
    if campaign == "tsan":
        return [("gate", [str(build / "mhgp8_wspd_q2_ranges_gate"), "--selftest"])]
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
            commands.append(("measure", [probe, *common, str(w), "16", "64", "8", str(pool)]))
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
    manifest = dict(schema="mhgp8_q2_ranges_attempt_v1", started_utc=utc_stamp(), build=str(build),
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
    require(manifest["schema"] == "mhgp8_q2_ranges_attempt_v1" and completion["status"] == "passed" and
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
