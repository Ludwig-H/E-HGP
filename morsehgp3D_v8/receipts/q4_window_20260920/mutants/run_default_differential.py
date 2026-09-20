#!/usr/bin/env python3
"""Twenty paired default-path comparisons; all JSON except timings must match.

Explicit adaptation of the tranche29 differential harness, outside the
189-source engine inventory. The unchanged shallow-q4 probe checks default
compatibility, not the new window path, performance or global completeness.
No historical source, build or receipt is modified.
"""
from __future__ import annotations

import argparse
import base64
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
from run_p0_matrix import digest, invoke, on_signal, parse_result, require, utc_stamp, write_json
from run_q4_shallow_checks import validate_row
from run_q34_seed_checks import environment_record
from run_q4_window_checks import SOURCES

OLD_BUILD = ROOT / "build/v8_q4_shallow_20260920"
NEW_BUILDS = (ROOT / "build/v8_q4_window_20260920",)
AUTHORITY = ROOT / "morsehgp3D_v8/receipts/q4_shallow_20260920/scale__b3aquyb"
PROBE = "mhgp8_q4_shallow_probe"
OPTIONS = ("32",)
CONFIGS = tuple((n, regime, k) for regime in ("far", "cap") for k in (5, 10)
                for n in (8000, 16000, 32000)) + tuple(
                    (n, "adversarial", k) for k in (5, 10) for n in (32, 64, 128, 256))


def pins(paths):
    return {name: digest(ROOT / name) for name in sorted(paths)}


def read_json(path):
    return parse_result(path.read_bytes())


def inventory(build):
    require(len(SOURCES) == 189, "expected frozen 189-source inventory")
    require(build in NEW_BUILDS, "unexpected tranche30 build")
    artifacts = {str((parent / name).relative_to(ROOT))
                 for parent in (OLD_BUILD, build)
                 for name in ("CMakeCache.txt", "libmhgp8_p0.a", PROBE)}
    helpers = {str(Path(__file__).resolve().relative_to(ROOT))}
    authority = {str((AUTHORITY / name).relative_to(ROOT))
                 for name in ("MANIFEST.json", "COMPLETION.json")}
    return dict(source_sha256=SOURCES, artifact_sha256=artifacts,
                helper_sha256=helpers, authority_sha256=authority)


def plan(build):
    return [(arm, [str(parent / PROBE), str(n), regime, str(k), *OPTIONS])
            for n, regime, k in CONFIGS for arm, parent in (("old", OLD_BUILD), ("new", build))]


def validate_authority(manifest, old, completion, archived_manifest_hash):
    # This exact tranche29 scale receipt pins its shallow probe and cache.
    # Archives are additionally pinned during this paired run, but the old
    # scale receipt did not include an archive: do not invent that authority.
    require(old["schema"] == "mhgp8_q4_shallow_attempt_v1" and
            len(old["source_sha256"]) == 184 and old["build"] == str(OLD_BUILD) and
            old["campaign"] == "scale" and completion["status"] == "passed" and
            completion["error"] is None and completion["closing_errors"] == [] and
            completion["manifest_sha256"] == archived_manifest_hash,
            "historical baseline scale capture is not closed")
    for key in ("source_sha256", "artifact_sha256"):
        require(old[key] == completion[key + "_after"], "historical input closure differs")
    for name in ("CMakeCache.txt", PROBE):
        relative = str((OLD_BUILD / name).relative_to(ROOT))
        require(manifest["artifact_sha256"][relative] == old["artifact_sha256"][relative],
                "old baseline is not the historically captured executable/cache")


def semantic(row):
    return {key: value for key, value in row.items() if key != "timings"}


def run(args):
    build = args.build.resolve()
    require(build.is_dir() and build in NEW_BUILDS, "existing tranche30 build required")
    inputs = inventory(build)
    for parent in (OLD_BUILD, build):
        cache = (parent / "CMakeCache.txt").read_text()
        require("CMAKE_BUILD_TYPE:STRING=Release\n" in cache and
                "MHGP8_SANITIZE:BOOL=OFF\n" in cache, "Release nonsanitized probes required")
    args.output.mkdir(parents=True, exist_ok=True)
    capture = Path(tempfile.mkdtemp(prefix="differential_", dir=args.output)).resolve()
    environment = dict(os.environ)
    manifest = dict(schema="mhgp8_q4_window_default_differential_v1", started_utc=utc_stamp(),
        old_build=str(OLD_BUILD), new_build=str(build), planned_commands=plan(build),
        configs=[list(c) for c in CONFIGS], probe_options=list(OPTIONS), excluded_fields=["timings"],
        scope="default_compatibility_one_edge_not_shallow_q4_performance_or_global_completeness",
        public_status="not_claimed", full_contract_qualified=False, gcp_used=False,
        commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        launch_command=[sys.executable, *sys.argv], environment=environment_record(environment),
        **{key: pins(paths) for key, paths in inputs.items()})
    write_json(capture / "MANIFEST.json", manifest)
    records, comparisons = [], []
    handlers = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    status, error = "failed", None
    try:
        for name in ("MANIFEST.json", "COMPLETION.json"):
            (capture / ("OLD_" + name)).write_bytes((AUTHORITY / name).read_bytes())
        validate_authority(manifest, read_json(capture / "OLD_MANIFEST.json"),
                           read_json(capture / "OLD_COMPLETION.json"),
                           digest(capture / "OLD_MANIFEST.json"))
        previous = None
        for number, (arm, command) in enumerate(plan(build)):
            record = dict(arm=arm, command=command, cwd=str(ROOT), started_utc=utc_stamp(),
                          expected_exit_code=0, exit_code=None, status="failed",
                          stdout="", stderr="", stdout_base64="", stderr_base64="")
            try:
                invoke(command, environment, ROOT, record, new_session=True)
                require(type(record["exit_code"]) is int and record["exit_code"] == 0 and
                        record["stderr"] == "", "probe command failed")
                row = parse_result(record["stdout"].encode())
                validate_row(row, command)
                record["row"] = row
                if arm == "old":
                    previous = semantic(row)
                else:
                    require(semantic(row) == previous, "default path changed fields other than timings")
                    comparisons.append(dict(n=row["n"], regime=row["regime"], kmax=row["kmax"],
                                            status="equal", digest=row["digest"]))
                record["status"] = "passed"
            finally:
                record["finished_utc"] = utc_stamp()
                path = capture / f"record_{number:02}.json"
                write_json(path, record)
                records.append(dict(path=path.name, sha256=digest(path)))
        require(len(comparisons) == 20, "incomplete paired differential")
        status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        for sig in handlers: signal.signal(sig, signal.SIG_IGN)
        after = {key + "_after": pins(paths) for key, paths in inputs.items()}
        if any(manifest[key] != after[key + "_after"] for key in inputs):
            status, error = "failed", error or "closing input hashes differ"
        completion = dict(status=status, error=error, finished_utc=utc_stamp(),
            manifest_sha256=digest(capture / "MANIFEST.json"), records=records,
            comparisons=comparisons, **after)
        write_json(capture / "COMPLETION.json", completion)
        for sig, handler in handlers.items(): signal.signal(sig, handler)
        print(json.dumps(dict(path=str(capture), status=status, comparisons=len(comparisons), error=error)), flush=True)
    require(status == "passed", "differential capture did not close")


def read(capture, check_live=False):
    capture = capture.resolve()
    manifest, completion = read_json(capture / "MANIFEST.json"), read_json(capture / "COMPLETION.json")
    build = Path(manifest["new_build"])
    require(manifest["schema"] == "mhgp8_q4_window_default_differential_v1" and
            manifest["old_build"] == str(OLD_BUILD) and manifest["configs"] == [list(c) for c in CONFIGS] and
            manifest["probe_options"] == list(OPTIONS) and manifest["excluded_fields"] == ["timings"] and completion["status"] == "passed" and
            completion["error"] is None and digest(capture / "MANIFEST.json") == completion["manifest_sha256"],
            "invalid differential closure")
    require(manifest["public_status"] == "not_claimed" and not manifest["full_contract_qualified"] and
            not manifest["gcp_used"] and manifest["scope"] ==
            "default_compatibility_one_edge_not_shallow_q4_performance_or_global_completeness",
            "differential scope changed")
    commands = plan(build)
    require(manifest["planned_commands"] == [[arm, cmd] for arm, cmd in commands] and
            len(completion["records"]) == 40, "differential command plan changed")
    for key, paths in inventory(build).items():
        require(set(manifest[key]) == set(paths) and manifest[key] == completion[key + "_after"],
                "input inventory or closure differs")
        if check_live: require(pins(paths) == manifest[key], "live inputs differ")
    for name in ("MANIFEST.json", "COMPLETION.json"):
        key = str((AUTHORITY / name).relative_to(ROOT))
        require(digest(capture / ("OLD_" + name)) == manifest["authority_sha256"][key],
                "archived baseline authority differs")
    validate_authority(manifest, read_json(capture / "OLD_MANIFEST.json"),
                       read_json(capture / "OLD_COMPLETION.json"), digest(capture / "OLD_MANIFEST.json"))
    previous, comparisons = None, []
    for number, item in enumerate(completion["records"]):
        arm, command = commands[number]
        require(item["path"] == f"record_{number:02}.json" and digest(capture / item["path"]) == item["sha256"],
                "differential raw record hash/order changed")
        record = read_json(capture / item["path"])
        require(record["status"] == "passed" and record["arm"] == arm and record["command"] == command and
                record["cwd"] == str(ROOT) and type(record["exit_code"]) is int and record["exit_code"] == 0 and
                record["expected_exit_code"] == 0 and record["stderr"] == "", "command/result mismatch")
        for stream in ("stdout", "stderr"):
            require(base64.b64decode(record[stream + "_base64"], validate=True).decode("utf-8", errors="replace") ==
                    record[stream], "raw stream mismatch")
        row = parse_result(record["stdout"].encode())
        require(row == record["row"], "parsed row differs from raw output")
        validate_row(row, command)
        if arm == "old": previous = semantic(row)
        else:
            require(semantic(row) == previous, "differential changed non-timing fields")
            comparisons.append(dict(n=row["n"], regime=row["regime"], kmax=row["kmax"],
                                    status="equal", digest=row["digest"]))
    require(comparisons == completion["comparisons"] and len(comparisons) == 20, "pair ledger mismatch")
    return dict(status="passed", path=str(capture), pairs=20, command_count=40,
                compared="all_fields_except_timings", probe_options=list(OPTIONS), live_checked=check_live,
                performance_claimed=False, full_contract_qualified=False, gcp_used=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    runner = sub.add_parser("run")
    runner.add_argument("--build", type=Path, required=True)
    runner.add_argument("--output", type=Path, default=HERE)
    reader = sub.add_parser("read")
    reader.add_argument("path", type=Path)
    reader.add_argument("--check-live", action="store_true")
    args = parser.parse_args()
    if args.operation == "run": run(args)
    else: print(json.dumps(read(args.path, args.check_live), sort_keys=True))


if __name__ == "__main__":
    main()
