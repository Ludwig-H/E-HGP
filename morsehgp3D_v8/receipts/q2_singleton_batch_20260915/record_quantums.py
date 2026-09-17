#!/usr/bin/env python3
"""Explicit bounded quantum experiment outside the frozen product source inventory.

Same current product, exact Coarse work/payload contracts and failure collector.
Fourteen commands: two n8k families, Coarse W1 then L1/8/16 x Q8/64 W1.
This helper is separately hashed before/after; no historical qualification
or timing is imported and no product/test source is changed for this sweep.
"""
import argparse
import base64
from copy import deepcopy
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SELF = Path(__file__).resolve()
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
import run_wspd_q2_batched_checks as reader
from run_p0_matrix import invoke, on_signal, utc_stamp

SCHEMA = "mhgp8_q2_quantums_attempt_v1"


def plan(build):
    commands = []
    reference = str(build / "mhgp8_wspd_q2_parallel_probe")
    probe = str(build / "mhgp8_wspd_q2_batched_probe")
    for family in ("uniform", "terrain"):
        common = ["8000", family, "10", "8", "3", "1", "16"]
        commands.append(("reference", [reference, *common, "64"]))
        for lanes in (1, 8, 16):
            for quantum in (8, 64):
                commands.append(("measure", [probe, *common, str(lanes), str(quantum), "64"]))
    return commands


def validate_manifest(manifest):
    reader.require(manifest["schema"] == SCHEMA and manifest["campaign"] == "quantums" and
                   manifest["public_status"] == "not_claimed" and manifest["gcp_used"] is False and
                   manifest["full_contract_qualified"] is False and
                   manifest["helper_path"] == str(SELF.relative_to(ROOT)), "quantum scope mismatch")
    reader.require(type(manifest["helper_sha256"]) is str and
                   reader.re.fullmatch(r"[0-9a-f]{64}", manifest["helper_sha256"]),
                   "malformed quantum helper pin")
    projected = deepcopy(manifest)
    # Same two executable/cache inventory as tuning; this is only the path
    # contract, not an assertion that this is an older tuning qualification.
    projected["campaign"] = "tuning"
    reader.validate_pins(projected)
    reader.require(manifest["planned_commands"] ==
                   [[kind, command] for kind, command in plan(Path(manifest["build"]))],
                   "quantum command plan mismatch")


def read(path):
    manifest = reader.strict_json((path / "MANIFEST.json").read_text())
    completion = reader.strict_json((path / "COMPLETION.json").read_text())
    validate_manifest(manifest)
    reader.require(completion["status"] == "passed" and completion["error"] is None and
                   completion["manifest_sha256"] == reader.digest(path / "MANIFEST.json") and
                   completion["helper_sha256_after"] == manifest["helper_sha256"] and
                   completion["source_sha256_after"] == manifest["source_sha256"] and
                   completion["artifact_sha256_after"] == manifest["artifact_sha256"],
                   "quantum capture is incomplete or changed")
    commands = manifest["planned_commands"]
    records = completion["records"]
    reader.require(len(records) == len(commands) == 14 and
                   {p.name for p in path.glob("record_*.json")} ==
                   {f"record_{i:04}.json" for i in range(len(records))},
                   "missing or orphan quantum records")
    rows = []
    for number, ((kind, command), info) in enumerate(zip(commands, records, strict=True)):
        reader.require(info["path"] == f"record_{number:04}.json", "quantum record path changed")
        target = path / info["path"]
        reader.require(info["sha256"] == reader.digest(target), "quantum record hash changed")
        record = reader.strict_json(target.read_text())
        reader.require(record["kind"] == kind and record["command"] == command and
                       record["cwd"] == str(ROOT) and record["status"] == "passed" and
                       type(record["exit_code"]) is int and record["exit_code"] == 0,
                       "quantum command/result mismatch")
        for channel in ("stdout", "stderr"):
            reader.require(base64.b64decode(record[channel + "_base64"], validate=True)
                           .decode("utf-8", errors="replace") == record[channel],
                           "quantum raw log mismatch")
        row = reader.strict_json(record["stdout"])
        if kind == "measure":
            reader.validate_row(row, command[1:])
        else:
            reader.validate_reference(row, command)
        reader.require(row == record["row"], "quantum parsed row changed")
        rows.append(row)
    reader.validate_pairs(rows)
    print(json.dumps(dict(status="passed", path=str(path), records=len(records),
                          q2_measures=len(rows), full_contract_qualified=False), sort_keys=True))


def run(args):
    build = args.build.resolve()
    reader.require(build.is_relative_to(ROOT) and (build / "CMakeCache.txt").is_file(),
                   "quantum build must exist inside repository")
    args.output.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix="quantums_", dir=args.output.resolve()))
    sources = reader.source_pins()
    artifacts = {str((build / name).relative_to(ROOT)): reader.digest(build / name)
                 for name in (*sorted(reader.artifact_names("tuning")), "CMakeCache.txt")}
    commands = plan(build)
    helper_pin = reader.digest(SELF)
    environment = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    manifest = dict(schema=SCHEMA, campaign="quantums", started_utc=utc_stamp(), build=str(build),
        command=[sys.executable, *sys.argv], planned_commands=[[kind, command] for kind, command in commands],
        helper_path=str(SELF.relative_to(ROOT)), helper_sha256=helper_pin,
        source_sha256=sources, artifact_sha256=artifacts,
        affinity=sorted(os.sched_getaffinity(0)),
        cmake_cache=(build / "CMakeCache.txt").read_text(),
        commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        worktree_status=subprocess.check_output(["git", "status", "--short"], cwd=ROOT, text=True),
        environment={key: environment.get(key) for key in
                     ("ASAN_OPTIONS", "UBSAN_OPTIONS", "TSAN_OPTIONS", "OMP_NUM_THREADS")},
        public_status="not_claimed", gcp_used=False, full_contract_qualified=False)
    reader.write_json(output / "MANIFEST.json", manifest)
    records, rows = [], []
    status, error = "failed", "not started"
    try:
        validate_manifest(manifest)
        for number, (kind, command) in enumerate(commands):
            print(json.dumps(dict(capture=str(output), number=number, command=command)), flush=True)
            record = dict(kind=kind, command=command, cwd=str(ROOT), started_utc=utc_stamp(),
                          status="failed", exit_code=None, stdout="", stderr="",
                          stdout_base64="", stderr_base64="")
            try:
                invoke(command, environment, ROOT, record, new_session=True)
                reader.require(record["exit_code"] == 0, "quantum command failed")
                row = reader.strict_json(record["stdout"])
                if kind == "measure":
                    reader.validate_row(row, command[1:])
                else:
                    reader.validate_reference(row, command)
                record["row"] = row
                rows.append(row)
                record["status"] = "passed"
            finally:
                record["finished_utc"] = utc_stamp()
                target = output / f"record_{number:04}.json"
                reader.write_json(target, record)
                records.append(dict(path=target.name, sha256=reader.digest(target)))
        reader.validate_pairs(rows)
        reader.require(reader.source_pins() == sources and reader.digest(SELF) == helper_pin and
                       all(reader.digest(ROOT / name) == pin for name, pin in artifacts.items()),
                       "quantum source/helper/artifact changed during capture")
        status, error = "passed", None
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        reader.write_json(output / "COMPLETION.json", dict(status=status, error=error, finished_utc=utc_stamp(),
            manifest_sha256=reader.digest(output / "MANIFEST.json"), records=records,
            helper_sha256_after=reader.digest(SELF), source_sha256_after=reader.source_pins(),
            artifact_sha256_after={name: reader.digest(ROOT / name) for name in artifacts}))
        for sig, handler in previous.items():
            signal.signal(sig, handler)
    read(output)
    return 0


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    capture = sub.add_parser("run")
    capture.add_argument("--build", type=Path, required=True)
    capture.add_argument("--output", type=Path, default=HERE)
    reader_parser = sub.add_parser("read")
    reader_parser.add_argument("path", type=Path)
    args = parser.parse_args()
    if args.operation == "run":
        raise SystemExit(run(args))
    read(args.path)
