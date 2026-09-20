#!/usr/bin/env python3
"""Twenty paired default-path comparisons; all JSON except timings must match.

Auxiliary qualification outside the 158-source engine inventory. Reuses the
existing cover row validator and raw-output collector; no historical source,
build or receipt is modified. This checks default compatibility, not speed.
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

from run_mutants import ROOT, HERE, digest, invoke, on_signal, pins, read_json, require, utc_stamp, write_json
from run_p0_matrix import parse_result
from run_q34_cover_checks import validate_row
from run_q34_seed_checks import environment_record
from run_q34_pruning_checks import SOURCES

OLD_BUILD = ROOT / "build/v8_q34_cover_20260920"
AUTHORITY = ROOT / "morsehgp3D_v8/receipts/q34_cover_20260920/scale_7ganupv_"
PROBE = "mhgp8_q34_cover_probe"
CONFIGS = tuple((n, regime, k) for regime in ("far", "cap") for k in (5, 10)
                for n in (8000, 16000, 32000)) + tuple(
                    (n, "adversarial", k) for k in (5, 10) for n in (32, 64, 128, 256))


def inventory(build):
    require(len(SOURCES) == 158, "expected frozen 158-source inventory")
    artifacts = {str((parent / name).relative_to(ROOT))
                 for parent in (OLD_BUILD, build)
                 for name in ("CMakeCache.txt", "libmhgp8_p0.a", PROBE)}
    helpers = {str(Path(__file__).resolve().relative_to(ROOT)),
               str((HERE / "run_mutants.py").relative_to(ROOT))}
    authority = {str((AUTHORITY / name).relative_to(ROOT))
                 for name in ("MANIFEST.json", "COMPLETION.json")}
    return dict(source_sha256=SOURCES, artifact_sha256=artifacts,
                helper_sha256=helpers, authority_sha256=authority)


def plan(build):
    return [(arm, [str(parent / PROBE), str(n), regime, str(k)])
            for n, regime, k in CONFIGS for arm, parent in (("old", OLD_BUILD), ("new", build))]


def validate_authority(manifest, old, completion, archived_manifest_hash):
    require(old["schema"] == "mhgp8_q34_cover_attempt_v1" and
            old["campaign"] == "scale" and old["build"] == str(OLD_BUILD) and
            completion["status"] == "passed" and completion["error"] is None and
            completion["manifest_sha256"] == archived_manifest_hash and
            old["source_sha256"] == completion["source_sha256_after"] and
            old["artifact_sha256"] == completion["artifact_sha256_after"],
            "historical baseline capture is not closed")
    for name in ("CMakeCache.txt", PROBE):
        relative = str((OLD_BUILD / name).relative_to(ROOT))
        require(manifest["artifact_sha256"][relative] == old["artifact_sha256"][relative],
                "old baseline is not the historically qualified executable/cache")


def semantic(row):
    return {key: value for key, value in row.items() if key != "timings"}


def run(args):
    build = args.build.resolve()
    require(build.is_dir() and build.is_relative_to(ROOT / "build") and build != OLD_BUILD,
            "new local build required, distinct from frozen baseline")
    inputs = inventory(build)
    for parent in (OLD_BUILD, build):
        cache = (parent / "CMakeCache.txt").read_text()
        require("CMAKE_BUILD_TYPE:STRING=Release\n" in cache and
                "MHGP8_SANITIZE:BOOL=OFF\n" in cache, "Release nonsanitized probes required")
    args.output.mkdir(parents=True, exist_ok=True)
    capture = Path(tempfile.mkdtemp(prefix="differential_", dir=args.output)).resolve()
    environment = dict(os.environ)
    manifest = dict(schema="mhgp8_q34_pruning_default_differential_v1", started_utc=utc_stamp(),
        old_build=str(OLD_BUILD), new_build=str(build), planned_commands=plan(build),
        configs=[list(c) for c in CONFIGS], excluded_fields=["timings"],
        scope="default_compatibility_one_edge_not_performance_or_global_completeness",
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
    require(manifest["schema"] == "mhgp8_q34_pruning_default_differential_v1" and
            manifest["old_build"] == str(OLD_BUILD) and manifest["configs"] == [list(c) for c in CONFIGS] and
            manifest["excluded_fields"] == ["timings"] and completion["status"] == "passed" and
            completion["error"] is None and digest(capture / "MANIFEST.json") == completion["manifest_sha256"],
            "invalid differential closure")
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
                compared="all_fields_except_timings", live_checked=check_live,
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
