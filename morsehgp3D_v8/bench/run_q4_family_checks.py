#!/usr/bin/env python3
"""Targeted q4 one-family receipts, without launching the historical q2 suite.

Use separate fresh Release and sanitizer builds. Each run retains failed or
interrupted commands. `read --check-live` additionally checks present sources
and binaries; ordinary read preserves historical readability after later edits.
"""
from __future__ import annotations

import argparse
import base64
from copy import deepcopy
import hashlib
import json
import math
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile

from inheritance_source_paths import SOURCE_PATHS as OLD_SOURCE_PATHS
from run_p0_matrix import InvalidReceipt, invoke, on_signal, parse_result, require, uint, utc_stamp, write_json

ROOT = Path(__file__).resolve().parents[2]
SOURCES = OLD_SOURCE_PATHS | frozenset({
    "morsehgp3D_v8/src/lanes/q4_family.hpp",
    "morsehgp3D_v8/src/lanes/q4_family.cpp",
    "morsehgp3D_v8/tests/q4_family_gate.cpp",
    "morsehgp3D_v8/bench/q4_family_probe.cpp",
    "morsehgp3D_v8/bench/run_q4_family_checks.py",
})
FAMILIES = ("uniform", "rows", "coplanar")
WORK = ("sites", "entries", "exits", "constant_inside", "constant_on", "constant_outside",
        "sort_comparisons", "group_comparisons", "groups", "max_group", "callbacks",
        "event_count", "retained_capacity_bytes")
DIGEST = ("groups", "root_ids", "constant_shell", "depth_sum", "hash",
          "callback_root_ids_visited", "callback_constant_ids_visited")
TIMES = ("generation_ms", "cloud_ms", "oracle_prepare_ms", "family_with_validation_callback_ms",
         "validation_ms", "release_ms", "total_ms")
FIXED = dict(schema="mhgp8_q4_family_probe_v1", status="completed",
             scope="one_q4_family_all_sites_not_q4_producer", public_status="not_claimed",
             backend="cpu_reference", profile="quantized_u16_input_only", threads=1,
             seed=3, seed_ids=[0, 1, 2],
             timing_scope="generation_owner_family_callback_validation_release_excludes_json")
GATE_FIELDS = ("checks", "clouds", "runs", "primitive_sites", "root_comparisons", "oracle_sites",
               "groups", "mixed_groups", "decreasing_steps", "max_depth", "max_shell", "max_cross_bits",
               "permutations", "invalid_inputs", "callback_failures", "reentrant_calls", "parallel_calls", "judge_mutants")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_json(path):
    return parse_result(path.read_bytes())


def pins(paths):
    return {name: digest(ROOT / name) for name in sorted(paths)}


def counts(value, fields, name):
    require(type(value) is dict and set(value) == set(fields), name + ": fields differ")
    for field in fields:
        uint(value[field], name + "." + field)


def validate_row(row, command):
    require(type(row) is dict and len(command) == 3, "probe command arity")
    n, family = int(command[1]), command[2]
    require(n >= 8 and family in FAMILIES, "fixture outside domain")
    require(all(row.get(key) == value and type(row.get(key)) is type(value)
                for key, value in FIXED.items()), "scope/schema mismatch")
    require(set(row) == set(FIXED) | {"n", "family", "input_hash", "generation", "work", "digest",
                                    "cloud_work", "memory", "timings"}, "probe fields differ")
    require(all(type(value) is int for value in row["seed_ids"]), "seed IDs must be integers")
    require(row.get("n") == n and type(row.get("n")) is int and row.get("family") == family,
            "command/result mismatch")
    uint(row.get("input_hash"), "input_hash")
    work, out = row.get("work"), row.get("digest")
    counts(work, WORK, "work")
    counts(out, DIGEST, "digest")
    require(work["sites"] == n and work["event_count"] == work["entries"] + work["exits"] and
            n == work["event_count"] + sum(work[k] for k in ("constant_inside", "constant_on", "constant_outside")),
            "site classification partition mismatch")
    events, groups = work["event_count"], work["groups"]
    require(groups == work["callbacks"] == out["groups"] and
            events == out["root_ids"] == out["callback_root_ids_visited"], "group/event ledger mismatch")
    require(work["constant_on"] == out["constant_shell"] >= 3 and
            out["callback_constant_ids_visited"] == (out["constant_shell"] if groups else 0),
            "constant shell reread or cardinality mismatch")
    require(work["group_comparisons"] == max(0, events - 1) and groups <= events and
            (groups == 0) == (events == 0) and (work["max_group"] == 0) == (events == 0) and
            work["max_group"] <= events and (events == 0 or work["max_group"] * groups >= events),
            "event grouping ledger mismatch")
    require(out["depth_sum"] <= groups * (n - 4), "group depth exceeds strict site maximum")
    require(work["sort_comparisons"] > 0 if events > 1 else work["sort_comparisons"] == 0,
            "sort non-vacuity mismatch")
    require((events == 0 and groups == 0) if family == "coplanar" else
            (work["entries"] > 0 and work["exits"] > 0 and groups > 0), "fixture event coverage mismatch")
    counts(row.get("generation"), ("proposals", "duplicates", "random_calls"), "generation")
    gen = row["generation"]
    require(gen["proposals"] == n - 3 + gen["duplicates"], "input generation partition mismatch")
    require(gen["random_calls"] == (0 if family == "rows" else gen["proposals"] * (2 if family == "coplanar" else 3)),
            "random generator ledger mismatch")
    counts(row.get("cloud_work"), ("copies", "validation_points", "uniqueness_comparisons", "range_tree_nodes"), "cloud")
    require(row["cloud_work"]["copies"] == row["cloud_work"]["validation_points"] == n,
            "owner preparation not paid once")
    memory = row.get("memory")
    require(type(memory) is dict and set(memory) == {"input_capacity_bytes", "cloud_retained_bytes", "judge_capacity_bytes", "event_id_bytes", "scope"} and
            memory["scope"] == "retained_capacities_not_RSS", "memory scope mismatch")
    for key in ("input_capacity_bytes", "cloud_retained_bytes"):
        require(uint(memory[key], key) >= n * 6, "coordinate storage missing")
    require(uint(memory["judge_capacity_bytes"], "judge_capacity_bytes") >= n, "judge coverage storage missing")
    require(uint(memory["event_id_bytes"], "event_id_bytes") in (4, 8) and
            work["retained_capacity_bytes"] >= 2 * n * memory["event_id_bytes"],
            "event and constant-shell reservations missing")
    timings = row.get("timings")
    require(type(timings) is dict and set(timings) == set(TIMES), "timing fields differ")
    require(all(type(v) in (float, int) and math.isfinite(v) and v >= 0 for v in timings.values()),
            "timings must be finite nonnegative numbers")
    require(math.isclose(sum(timings[k] for k in TIMES[:-1]), timings["total_ms"], rel_tol=1e-12, abs_tol=1e-6),
            "timing partition does not close")


def validate_gate(row):
    require(type(row) is dict and set(row) == set(GATE_FIELDS) | {"schema", "status"} and
            row["schema"] == "mhgp8_q4_family_gate_v1" and row["status"] == "passed", "gate scope/fields mismatch")
    for key in GATE_FIELDS:
        uint(row[key], "gate." + key)
    floors = dict(checks=1, clouds=1, runs=200, primitive_sites=1, root_comparisons=20000,
                  oracle_sites=10000, groups=1000, mixed_groups=8, decreasing_steps=20,
                  max_depth=11, max_shell=30, max_cross_bits=129, permutations=1, invalid_inputs=1)
    require(all(row[key] >= floor for key, floor in floors.items()), "gate non-vacuity floor failed")
    require(row["callback_failures"] == 1 and row["reentrant_calls"] == 1 and
            row["parallel_calls"] == 4 and row["judge_mutants"] == 5, "gate lifecycle/judge counters differ")


def validate_closure(manifest, completion):
    require(completion["status"] == "passed" and completion["error"] is None, "capture incomplete")
    for key in ("source_sha256", "artifact_sha256"):
        require(manifest[key] == completion[key + "_after"] and
                all(type(v) is str and len(v) == 64 and all(c in "0123456789abcdef" for c in v)
                    for v in manifest[key].values()), "closure hash mismatch")


def plan(build, campaign):
    gate = [("gate", [str(build / "mhgp8_q4_family_gate"), "--selftest"])]
    if campaign == "gate":
        return gate
    sizes = (8000, 16000, 32000) if campaign == "scale" else (32,)
    return gate + [("measure", [str(build / "mhgp8_q4_family_probe"), str(n), family])
                   for family in FAMILIES for n in sizes]


def artifact_paths(build, campaign):
    names = {"CMakeCache.txt", "mhgp8_q4_family_gate"}
    if campaign != "gate":
        names.add("mhgp8_q4_family_probe")
    return {str((build / name).relative_to(ROOT)) for name in names}


def run(args):
    build = args.build.resolve()
    require(build.is_dir() and build.is_relative_to(ROOT / "build"), "fresh local build required")
    args.output.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix=args.campaign + "_", dir=args.output)).resolve()
    sources, artifacts = pins(SOURCES), pins(artifact_paths(build, args.campaign))
    commands = plan(build, args.campaign)
    def git(*arguments):
        return subprocess.run(["git", *arguments], cwd=ROOT, check=True, capture_output=True, text=True).stdout.strip()
    compiler_cache = "\n".join(line for line in (build / "CMakeCache.txt").read_text().splitlines()
                               if line.startswith(("CMAKE_CXX_COMPILER", "CMAKE_CXX_FLAGS", "CMAKE_BUILD_TYPE:",
                                                   "CMAKE_GENERATOR:", "MHGP8_SANITIZE:")))
    manifest = dict(schema="mhgp8_q4_family_attempt_v1", campaign=args.campaign, build=str(build),
                    started_utc=utc_stamp(), public_status="not_claimed", gcp_used=False,
                    full_contract_qualified=False, source_sha256=sources, artifact_sha256=artifacts,
                    planned_commands=commands, affinity=sorted(os.sched_getaffinity(0)),
                    commit=git("rev-parse", "HEAD"), worktree=git("status", "--porcelain=v1", "--untracked-files=all"),
                    launch_command=[sys.executable, *sys.argv], compiler_cache=compiler_cache,
                    sanitizer_environment={name: os.environ.get(name) for name in
                                           ("ASAN_OPTIONS", "UBSAN_OPTIONS", "TSAN_OPTIONS")})
    write_json(output / "MANIFEST.json", manifest)
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, error, status = [], None, "failed"
    try:
        for number, (kind, command) in enumerate(commands):
            record = dict(kind=kind, command=command, cwd=str(ROOT), started_utc=utc_stamp(),
                          status="failed", exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="")
            try:
                invoke(command, dict(os.environ), ROOT, record, new_session=True)
                require(record["exit_code"] == 0, "targeted command failed")
                row = parse_result(record["stdout"].encode())
                if kind == "measure":
                    validate_row(row, command)
                else:
                    validate_gate(row)
                record["row"] = row
                record["status"] = "passed"
            finally:
                record["finished_utc"] = utc_stamp()
                target = output / f"record_{number:04}.json"
                write_json(target, record)
                records.append(dict(path=target.name, sha256=digest(target)))
        require(pins(SOURCES) == sources and pins(artifacts) == artifacts, "sources/binaries changed during capture")
        status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        write_json(output / "COMPLETION.json", dict(status=status, error=error, finished_utc=utc_stamp(),
            manifest_sha256=digest(output / "MANIFEST.json"), records=records,
            source_sha256_after=pins(SOURCES), artifact_sha256_after=pins(artifacts)))
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(path=str(output), status=status, error=error)), flush=True)


def read(path, check_live=False):
    path = path.resolve()
    manifest, completion = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
    require(manifest["schema"] == "mhgp8_q4_family_attempt_v1" and completion["status"] == "passed" and
            completion["error"] is None and completion["manifest_sha256"] == digest(path / "MANIFEST.json"),
            "incomplete or corrupt capture")
    validate_closure(manifest, completion)
    require(manifest["public_status"] == "not_claimed" and manifest["gcp_used"] is False and
            manifest["full_contract_qualified"] is False, "capture changed scope")
    build, campaign = Path(manifest["build"]), manifest["campaign"]
    require(build.is_absolute() and build.is_relative_to(ROOT / "build") and ".." not in build.parts and
            campaign in ("gate", "smoke", "scale"), "invalid build/campaign")
    require(set(manifest["source_sha256"]) == SOURCES and
            set(manifest["artifact_sha256"]) == artifact_paths(build, campaign), "pin inventory mismatch")
    require(type(manifest.get("commit")) is str and len(manifest["commit"]) == 40 and
            all(c in "0123456789abcdef" for c in manifest["commit"]) and
            type(manifest.get("worktree")) is str and type(manifest.get("compiler_cache")) is str and
            "CMAKE_CXX_COMPILER:" in manifest["compiler_cache"] and
            type(manifest.get("launch_command")) is list and len(manifest["launch_command"]) >= 2 and
            all(type(s) is str for s in manifest["launch_command"]) and
            type(manifest.get("sanitizer_environment")) is dict and
            set(manifest["sanitizer_environment"]) == {"ASAN_OPTIONS", "UBSAN_OPTIONS", "TSAN_OPTIONS"},
            "capture provenance missing")
    for key in ("source_sha256", "artifact_sha256"):
        if check_live:
            require(pins(manifest[key]) == manifest[key], "present sources/binaries differ from capture")
    commands = [[kind, command] for kind, command in plan(build, campaign)]
    require(manifest["planned_commands"] == commands and len(completion["records"]) == len(commands),
            "command plan mismatch")
    require({p.name for p in path.glob("record_*.json")} == {f"record_{i:04}.json" for i in range(len(commands))},
            "orphan or missing record")
    rows = []
    for number, ((kind, command), info) in enumerate(zip(commands, completion["records"], strict=True)):
        require(info["path"] == f"record_{number:04}.json", "record order mismatch")
        file = path / info["path"]
        require(digest(file) == info["sha256"], "record hash mismatch")
        record = read_json(file)
        require(record["kind"] == kind and record["command"] == command and record["cwd"] == str(ROOT) and
                record["status"] == "passed" and type(record["exit_code"]) is int and record["exit_code"] == 0,
                "command/result mismatch")
        for channel in ("stdout", "stderr"):
            require(base64.b64decode(record[channel + "_base64"], validate=True).decode("utf-8", errors="replace") ==
                    record[channel], "raw/decoded output mismatch")
        row = parse_result(record["stdout"].encode())
        require(row == record["row"], "parsed row changed")
        if kind == "measure":
            validate_row(row, command)
            rows.append(row)
        else:
            validate_gate(row)
    growth = {}
    if campaign == "scale":
        for family in FAMILIES:
            selected = [row for row in rows if row["family"] == family]
            growth[family] = {}
            for field in WORK:
                values = [row["work"][field] for row in selected]
                growth[family][field] = dict(values=values,
                    ratios=[b / a if a else None for a, b in zip(values, values[1:])],
                    above_quadrupling=[a > 0 and b > 4 * a for a, b in zip(values, values[1:])])
    return dict(status="passed", path=str(path), campaign=campaign, records=len(commands),
                measurements=len(rows), scope="one_family_not_q4_producer", full_contract_qualified=False,
                general_subquadratic_bound=False, sources=len(SOURCES), growth=growth)


def selftest(path):
    summary = read(path)
    require(summary["measurements"] >= 3, "reader selftest requires a smoke or scale capture")
    records = [read_json(file) for file in sorted(path.glob("record_*.json"))]
    base = next(record for record in records if record["kind"] == "measure" and record["row"]["family"] == "uniform")
    mutants = 0
    def rejects(action):
        nonlocal mutants
        try:
            action()
        except InvalidReceipt:
            mutants += 1
        else:
            raise InvalidReceipt("reader mutant survived")
    for field, value in (("n", True), ("input_hash", True), ("seed_ids", [False, 1, 2])):
        row = deepcopy(base["row"])
        row[field] = value
        rejects(lambda: validate_row(row, base["command"]))
    for section, field, value in (("work", "sites", 0), ("work", "sort_comparisons", True),
            ("digest", "callback_constant_ids_visited", 0), ("digest", "root_ids", 0),
            ("memory", "event_id_bytes", True), ("work", "retained_capacity_bytes", 0),
            ("timings", "total_ms", float("nan"))):
        row = deepcopy(base["row"])
        row[section][field] = value
        rejects(lambda: validate_row(row, base["command"]))
    command = base["command"].copy()
    command[1] = str(int(command[1]) + 1)
    rejects(lambda: validate_row(base["row"], command))
    rejects(lambda: parse_result(b'{"x":NaN}'))
    rejects(lambda: parse_result(b'{"x":1,"x":2}'))
    gate = deepcopy(records[0]["row"])
    gate["runs"] = True
    rejects(lambda: validate_gate(gate))
    gate["runs"] = 0
    rejects(lambda: validate_gate(gate))
    manifest, completion = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
    completion["source_sha256_after"][next(iter(manifest["source_sha256"]))] = "0" * 64
    rejects(lambda: validate_closure(manifest, completion))
    return dict(status="passed", mutants=mutants, real_measurements=summary["measurements"],
                scope="receipt_reader_not_geometric_oracle")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    capture = sub.add_parser("run")
    capture.add_argument("--build", type=Path, required=True)
    capture.add_argument("--output", type=Path, required=True)
    capture.add_argument("--campaign", choices=("gate", "smoke", "scale"), required=True)
    reader = sub.add_parser("read")
    reader.add_argument("path", type=Path)
    reader.add_argument("--check-live", action="store_true")
    unit = sub.add_parser("selftest")
    unit.add_argument("path", type=Path)
    args = parser.parse_args()
    if args.operation == "run":
        run(args)
    elif args.operation == "read":
        print(json.dumps(read(args.path, args.check_live), sort_keys=True, allow_nan=False))
    else:
        print(json.dumps(selftest(args.path), sort_keys=True, allow_nan=False))


if __name__ == "__main__":
    main()
