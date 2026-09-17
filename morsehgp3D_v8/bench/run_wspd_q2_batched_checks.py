#!/usr/bin/env python3
"""Fresh batched-singleton q2 captures and strict, failure-preserving closed receipts.

Explicit collector/pin port from the anchor-range runner at 2741d614.
The Coarse pipeline validator is imported as a contract projection only:
workers=min(requested,seeds), no old qualification or timing inherited.
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
from batched_source_paths import SOURCE_PATHS, ARTIFACT_NAMES
from run_wspd_q2_parallel_matrix import (
    POOL_FIELDS, validate_result as validate_reference, FIXED, cross_check, counters)
from run_wspd_q2_cooperative_checks import strict_json

ROOT = Path(__file__).resolve().parents[2]
FAMILIES = ("uniform", "terrain", "clusters", "rows")

def artifact_names(campaign):
    if campaign == "tsan":
        return {"mhgp8_wspd_q2_batched_gate"}
    if campaign in ("smoke", "tuning"):
        return {"mhgp8_wspd_q2_batched_probe", "mhgp8_wspd_q2_parallel_probe"}
    return ARTIFACT_NAMES


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
    expected = artifact_names(manifest["campaign"])
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


BATCH_FIELDS = """enqueued key_preparations completed completed_accepted completed_rejected
batch_passes lane_visits transitions entry_steps witness_steps admission_steps payload_steps
sibling_due sibling_bound_tests sibling_rejected sibling_rejected_after_credit
entry_after_credit entry_inside_deferred full_drains seed_flushes pool_flushes max_active""".split()
MAXIMA = ("max_active",)


def validate_work(work, row, worker=None):
    counters(work, BATCH_FIELDS, "batch_work")
    require(work["enqueued"] == work["key_preparations"] == work["completed"] == work["entry_steps"] ==
            work["completed_accepted"] + work["completed_rejected"] and
            work["admission_steps"] == work["payload_steps"] == work["completed_accepted"],
            "batched singleton completion ledger mismatch")
    require(work["transitions"] == work["entry_steps"] + work["witness_steps"] +
            work["admission_steps"] + work["payload_steps"] and
            work["lane_visits"] <= work["transitions"] <= row["quantum"] * work["lane_visits"] and
            work["batch_passes"] <= work["lane_visits"] <= row["lanes"] * work["batch_passes"] and
            work["max_active"] <= row["lanes"], "batched dispatch bounds mismatch")
    require(all(work[key] <= work["enqueued"] for key in
                ("sibling_due", "entry_after_credit", "entry_inside_deferred", "full_drains")) and
            (work["max_active"] == 0) == (work["enqueued"] == 0) and
            (work["batch_passes"] == 0) == (work["enqueued"] == 0) and
            work["sibling_rejected_after_credit"] <= work["sibling_rejected"] <=
            work["sibling_bound_tests"] <= work["sibling_due"],
            "batched submission provenance mismatch")
    if worker is None:
        require(work["enqueued"] <= row["census_work"]["query_tasks"] and
                work["completed_accepted"] <= row["accepted_pairs"] and
                work["completed_rejected"] <= row["rejected_pairs"] and
                work["seed_flushes"] == row["parallel_work"]["completed_jobs"] and
                work["pool_flushes"] == row["pool_work"]["selected_rectangles"] and
                work["sibling_due"] <= row["sibling_work"]["proposals"] and
                work["sibling_bound_tests"] <= row["sibling_work"]["bound_tests"] and
                work["sibling_rejected"] <= row["sibling_work"]["rejected_tasks"] and
                work["sibling_rejected_after_credit"] <= row["sibling_work"]["rejected_after_credit"],
                "global batch work exceeded actual pipeline work")
    else:
        require(work["seed_flushes"] == worker["jobs"] and
                work["completed_accepted"] <= worker["supports"],
                "worker batch flush or support mismatch")


def validate_row(row, args):
    require(len(args) == 10, "batched command arity")
    n, family, k, separation, seed, workers, jobs, lanes, quantum, pool = args
    expected = (int(n), family, int(k), int(separation), int(seed), int(workers),
                int(jobs), int(lanes), int(quantum), int(pool))
    keys = ("n", "family", "kmax", "s", "seed", "threads", "jobs_per_worker",
            "lanes", "quantum", "pool_min_factor")
    require(type(row) is dict and tuple(row.get(key) for key in keys) == expected,
            "batched command/result mismatch")
    for key in keys:
        if key != "family":
            uint(row[key], key)
    require(all(row[key] > 0 for key in ("threads", "lanes", "quantum")),
            "positive batched options required")
    require(row.get("schema") == "mhgp8_wspd_q2_batched_probe_v1" and
            row.get("execution") == "batched_singleton_front_census" and
            row.get("worker_clock_scope") == "active_coarse" and
            row.get("pool_clock_scope") == "synchronous_pool_parent",
            "batched scope mismatch")
    # Explicit current field-contract projection, not a historical result.
    # This entry keeps the ORIGINAL Coarse min(W,jobs) worker semantics.
    projected = deepcopy(row)
    projected["schema"], projected["execution"] = FIXED["schema"], "parallel_front"
    for key in ("lanes", "quantum", "worker_clock_scope", "pool_clock_scope",
                "batch_work", "batch_storage"):
        del projected[key]
    queue_bytes = projected["parallel_work"].pop("queue_storage_bytes")
    uint(queue_bytes, "queue_storage_bytes")
    require(queue_bytes == 0, "batched Coarse route allocated a dispatcher queue")
    for worker in projected["workers"]:
        for key in ("batch_work", "state_capacity", "state_storage_bytes"):
            del worker[key]
    validate_reference(projected, ["projected-probe", n, family, k, separation, seed, workers, jobs, pool])
    storage = row["batch_storage"]
    counters(storage, ("state_bytes", "state_capacity_sum", "state_storage_bytes_sum"), "batch_storage")
    require(storage["state_bytes"] > 0 and
            storage["state_storage_bytes_sum"] == storage["state_bytes"] * storage["state_capacity_sum"],
            "batch state storage reduction mismatch")
    for worker in row["workers"]:
        uint(worker["state_capacity"], "worker state capacity")
        uint(worker["state_storage_bytes"], "worker state storage bytes")
        require(worker["state_capacity"] >= row["lanes"] and
                worker["state_storage_bytes"] == worker["state_capacity"] * storage["state_bytes"],
                "worker lane allocation missing")
        validate_work(worker["batch_work"], row, worker)
    require(storage["state_capacity_sum"] == sum(w["state_capacity"] for w in row["workers"]) and
            storage["state_storage_bytes_sum"] == sum(w["state_storage_bytes"] for w in row["workers"]),
            "batch allocated capacities lost a worker")
    total = row["batch_work"]
    validate_work(total, row)
    for key in BATCH_FIELDS:
        parts = [worker["batch_work"][key] for worker in row["workers"]]
        combined = max(parts, default=0) if key in MAXIMA else sum(parts)
        require(total[key] == combined, "batch worker reduction lost " + key)
    return row


def validate_ctest(path):
    suite = ET.fromstring(path.read_bytes())
    cases = list(suite.iter("testcase"))
    require(suite.tag == "testsuite" and len(cases) == len({c.attrib["name"] for c in cases}) == 75 and
            int(suite.attrib["tests"]) == 75 and int(suite.attrib["failures"]) == 0 and
            int(suite.attrib.get("errors", 0)) == int(suite.attrib.get("skipped", 0)) == 0 and
            all(c.attrib["name"].startswith("mhgp8_") and c.find("failure") is None and
                c.find("error") is None and c.find("skipped") is None for c in cases),
            "expected 75 distinct passing CTests")


def validate_pairs(rows):
    identities, signatures = {}, {}
    for row in rows:
        cross_check(row, identities, signatures)


def plan(build, output, campaign, lanes, quantum=1):
    uint(lanes, "selected lanes")
    require(lanes > 0, "selected lanes must be positive")
    # Historical captures carry no quantum and replay their exact quantum-1 plan.
    uint(quantum, "selected quantum")
    require(quantum > 0, "selected quantum must be positive")
    probe = str(build / "mhgp8_wspd_q2_batched_probe")
    reference = str(build / "mhgp8_wspd_q2_parallel_probe")
    if campaign == "qualification":
        return [("ctest", ["ctest", "--test-dir", str(build), "--parallel", "2", "--output-on-failure",
                           "--output-junit", str(output / "ctest.xml")]),
                ("gate", [str(build / "mhgp8_wspd_q2_batched_gate"), "--selftest"])]
    if campaign == "tsan":
        return [("gate", [str(build / "mhgp8_wspd_q2_batched_gate"), "--selftest"])]
    if campaign == "scale":
        configurations = itertools.product((8000, 16000, 32000), FAMILIES, (10,), (8,), (1,))
    elif campaign == "parallel":
        configurations = itertools.product((32000,), FAMILIES, (10,), (8,), (4,))
    elif campaign == "separations":
        configurations = [v for v in itertools.product((8000,), FAMILIES, (5, 10), (8, 10, 12), (1,))
                          if (v[2], v[3]) != (10, 8)]
    elif campaign == "smoke":
        configurations = itertools.product((32,), FAMILIES, (5,), (8,), (1, 4))
    elif campaign == "tuning":
        configurations = itertools.product((8000,), ("uniform", "terrain"), (10,), (8,), (1,))
    else:
        raise RuntimeError("unknown campaign")
    commands = []
    for n, family, k, s, workers in configurations:
        common = [str(n), family, str(k), str(s), "3", str(workers), "16"]
        commands.append(("reference", [reference, *common, "64"]))
        for selected in ((1, 8, 16) if campaign in ("smoke", "tuning") else (lanes,)):
            commands.append(("measure", [probe, *common, str(selected), str(quantum), "64"]))
    return commands


def run(args):
    build = args.build.resolve()
    require(build.is_relative_to(ROOT) and (build / "CMakeCache.txt").is_file(), "build must exist inside repository")
    args.output.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix=args.campaign + "_", dir=args.output.resolve()))
    environment = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    sources = source_pins()
    selected_artifacts = [build / name for name in sorted(artifact_names(args.campaign))]
    require(all(p.is_file() and os.access(p, os.X_OK) for p in selected_artifacts), "missing campaign executable")
    artifacts = {str(p.relative_to(ROOT)): digest(p) for p in selected_artifacts}
    artifacts[str((build / "CMakeCache.txt").relative_to(ROOT))] = digest(build / "CMakeCache.txt")
    commands = plan(build, output, args.campaign, args.lanes, args.quantum)
    manifest = dict(schema="mhgp8_q2_batched_attempt_v1", started_utc=utc_stamp(), build=str(build),
                    campaign=args.campaign, lanes=args.lanes, quantum=args.quantum,
                    command=[sys.executable, *sys.argv],
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
    require(manifest["schema"] == "mhgp8_q2_batched_attempt_v1" and completion["status"] == "passed" and
            completion["error"] is None and
            completion["manifest_sha256"] == digest(path / "MANIFEST.json"), "capture incomplete or corrupt")
    require(manifest["public_status"] == "not_claimed" and manifest["gcp_used"] is False and
            manifest["full_contract_qualified"] is False, "capture changed scope")
    require(manifest["source_sha256"] == completion["source_sha256_after"] and
            manifest["artifact_sha256"] == completion["artifact_sha256_after"], "closure hash mismatch")
    commands = manifest["planned_commands"]
    expected_commands = plan(Path(manifest["build"]), path.resolve(), manifest["campaign"], manifest["lanes"],
                             manifest.get("quantum", 1))
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
    capture.add_argument("--campaign", choices=("qualification", "tsan", "smoke", "tuning", "scale", "parallel", "separations"),
                         default="qualification")
    capture.add_argument("--lanes", type=int, default=16)
    capture.add_argument("--quantum", type=int, default=1)
    reader = sub.add_parser("read")
    reader.add_argument("path", type=Path)
    args = parser.parse_args()
    if args.operation == "run":
        return run(args)
    read(args.path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
