#!/usr/bin/env python3
"""Three bounded r2 checks, NOT CTest, sanitizers, speed or FULL qualification.

Explicit collector pattern from record_quantums.py, with its own fixed plan:
native --selftest, receipt Python gate normally, then under -O. Sources, this
helper and every gate/probe/cache artifact are hashed before and after. Raw
child bytes and failed/interrupted attempts are preserved by the collector.
"""
import argparse
import base64
import json
import os
from pathlib import Path
import re
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

SCHEMA = "mhgp8_q2_batched_small_checks_v1"
ARTIFACTS = frozenset(("mhgp8_wspd_q2_batched_gate", "mhgp8_wspd_q2_batched_probe",
                       "mhgp8_wspd_q2_parallel_probe", "CMakeCache.txt"))
GATE = ROOT / "morsehgp3D_v8/tests/wspd_q2_batched_receipts_gate.py"
SCOPE = dict(public_status="not_claimed", gcp_used=False, full_contract_qualified=False,
             ctest_run=False, asan_ubsan_run=False, tsan_run=False)


def plan(build, python):
    common = [str(GATE), "--probe", str(build / "mhgp8_wspd_q2_batched_probe"), "--selftest"]
    return [["native", [str(build / "mhgp8_wspd_q2_batched_gate"), "--selftest"]],
            ["python", [python, "-B", *common]],
            ["python_optimized", [python, "-B", "-O", *common]]]


def valid_sha(value):
    return type(value) is str and re.fullmatch(r"[0-9a-f]{64}", value) is not None


def validate_manifest(manifest):
    reader.require(manifest["schema"] == SCHEMA and manifest["campaign"] == "small_checks" and
                   all(type(manifest[key]) is type(value) and manifest[key] == value
                       for key, value in SCOPE.items()), "small-check scope changed")
    reader.require(manifest["helper_path"] == str(SELF.relative_to(ROOT)) and
                   valid_sha(manifest["helper_sha256"]), "small-check helper pin changed")
    build = Path(manifest["build"])
    python = Path(manifest["python_executable"])
    reader.require(build.is_absolute() and build.is_relative_to(ROOT) and ".." not in build.parts,
                   "small-check build escaped repository")
    reader.require(python.is_absolute() and ".." not in python.parts and
                   re.fullmatch(r"python(?:[0-9]+(?:\.[0-9]+)*)?", python.name),
                   "small-check Python executable is not an absolute interpreter path")
    sources, artifacts = manifest["source_sha256"], manifest["artifact_sha256"]
    reader.require(type(sources) is dict and set(sources) == reader.SOURCE_PATHS and len(sources) == 120,
                   "small-check source membership mismatch")
    reader.require(type(artifacts) is dict and set(artifacts) ==
                   {str(build.relative_to(ROOT) / name) for name in ARTIFACTS},
                   "small-check gate/probe/cache membership mismatch")
    reader.require(all(valid_sha(value) for pins in (sources, artifacts) for value in pins.values()),
                   "small-check malformed SHA256 pin")
    reader.require(manifest["planned_commands"] == plan(build, str(python)),
                   "small-check exact three-command plan changed")


def validate_row(kind, row):
    reader.require(type(row) is dict and row.get("status") == "passed", "bounded gate did not pass")
    if kind == "native":
        reader.require(row.get("schema") == "mhgp8_wspd_q2_batched_gate_v1" and
                       row.get("public_status") == "not_claimed", "native gate scope changed")
        excluded = {"schema", "status", "public_status"}
        reader.require(row.get("batch_runs") == 595 and row.get("coarse_runs") == 178 and
                       row.get("clouds") == 13 and row.get("max_shell") == 30,
                       "native bounded gate coverage changed")
    else:
        reader.require(kind in ("python", "python_optimized") and
                       row.get("full_contract_qualified") is False and
                       row.get("real_captures") == 24 and row.get("paired_workers") == 12,
                       "Python bounded gate scope/coverage changed")
        excluded = {"status", "full_contract_qualified"}
    reader.require(all(type(value) is int and value >= 0 for key, value in row.items() if key not in excluded),
                   "bounded gate counter is not a nonnegative integer")


def read(path):
    path = path.resolve()
    manifest = reader.strict_json((path / "MANIFEST.json").read_text())
    completion = reader.strict_json((path / "COMPLETION.json").read_text())
    validate_manifest(manifest)
    reader.require(completion["status"] == "passed" and completion["error"] is None and
                   completion["closure_errors"] == [] and
                   completion["manifest_sha256"] == reader.digest(path / "MANIFEST.json") and
                   completion["helper_sha256_after"] == manifest["helper_sha256"] and
                   completion["source_sha256_after"] == manifest["source_sha256"] and
                   completion["artifact_sha256_after"] == manifest["artifact_sha256"],
                   "small checks are incomplete, failed or changed")
    commands, records = manifest["planned_commands"], completion["records"]
    reader.require(type(records) is list and len(records) == len(commands) == 3 and
                   {p.name for p in path.glob("record_*.json")} ==
                   {f"record_{i:04}.json" for i in range(3)}, "missing or orphan small-check records")
    rows = []
    for number, ((kind, command), info) in enumerate(zip(commands, records, strict=True)):
        reader.require(info["path"] == f"record_{number:04}.json" and valid_sha(info["sha256"]),
                       "small-check record path or pin changed")
        target = path / info["path"]
        reader.require(info["sha256"] == reader.digest(target), "small-check record hash changed")
        record = reader.strict_json(target.read_text())
        reader.require(record["kind"] == kind and record["command"] == command and record["cwd"] == str(ROOT) and
                       record["status"] == "passed" and type(record["exit_code"]) is int and record["exit_code"] == 0,
                       "small-check command/result mismatch")
        for channel in ("stdout", "stderr"):
            reader.require(base64.b64decode(record[channel + "_base64"], validate=True)
                           .decode("utf-8", errors="replace") == record[channel],
                           "small-check raw byte/text logs disagree")
        row = reader.strict_json(record["stdout"])
        validate_row(kind, row)
        reader.require(row == record["row"], "small-check parsed row changed")
        rows.append(row)
    reader.require(rows[1] == rows[2], "normal/-O bounded gate summaries differ")
    result = dict(status="passed", path=str(path), records=3, source_files=120, artifacts=4,
                  normal_optimized_equal=True, **SCOPE)
    print(json.dumps(result, sort_keys=True))
    return result


def run(args):
    build = args.build.resolve()
    reader.require(build.is_relative_to(ROOT) and (build / "CMakeCache.txt").is_file(),
                   "small-check build must exist inside repository")
    sources = reader.source_pins()
    artifacts = {str((build / name).relative_to(ROOT)): reader.digest(build / name) for name in sorted(ARTIFACTS)}
    helper_pin = reader.digest(SELF)
    python = str(Path(sys.executable).resolve())
    commands = plan(build, python)
    args.output.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix="small_checks_", dir=args.output.resolve()))
    environment = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1", PYTHONDONTWRITEBYTECODE="1")
    manifest = dict(schema=SCHEMA, campaign="small_checks", started_utc=utc_stamp(), build=str(build),
        python_executable=python, command=[sys.executable, *sys.argv], planned_commands=commands,
        helper_path=str(SELF.relative_to(ROOT)), helper_sha256=helper_pin,
        source_sha256=sources, artifact_sha256=artifacts, affinity=sorted(os.sched_getaffinity(0)),
        cmake_cache=(build / "CMakeCache.txt").read_text(),
        commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        environment={key: environment.get(key) for key in
                     ("ASAN_OPTIONS", "UBSAN_OPTIONS", "TSAN_OPTIONS", "OMP_NUM_THREADS", "PYTHONDONTWRITEBYTECODE")},
        **SCOPE)
    reader.write_json(output / "MANIFEST.json", manifest)
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, rows = [], []
    status, error = "failed", "not started"
    try:
        validate_manifest(manifest)
        for number, (kind, command) in enumerate(commands):
            print(json.dumps(dict(capture=str(output), number=number, command=command)), flush=True)
            record = dict(kind=kind, command=command, cwd=str(ROOT), started_utc=utc_stamp(),
                          status="failed", exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="")
            try:
                invoke(command, environment, ROOT, record, new_session=True)
                reader.require(type(record["exit_code"]) is int and record["exit_code"] == 0, "bounded command failed")
                row = reader.strict_json(record["stdout"])
                validate_row(kind, row)
                record["row"] = row
                rows.append(row)
                record["status"] = "passed"
            finally:
                record["finished_utc"] = utc_stamp()
                target = output / f"record_{number:04}.json"
                reader.write_json(target, record)
                records.append(dict(path=target.name, sha256=reader.digest(target)))
        reader.require(rows[1] == rows[2], "normal/-O bounded gate summaries differ")
        reader.require(reader.source_pins() == sources and reader.digest(SELF) == helper_pin and
                       all(reader.digest(ROOT / name) == pin for name, pin in artifacts.items()),
                       "small-check source/helper/artifact changed during capture")
        status, error = "passed", None
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        closure_errors = []
        def after_pin(path):
            try:
                return reader.digest(path)
            except BaseException as cause:
                closure_errors.append(f"{path}: {type(cause).__name__}: {cause}")
                return None
        completion = dict(status=status, error=error, finished_utc=utc_stamp(), records=records,
            manifest_sha256=after_pin(output / "MANIFEST.json"), helper_sha256_after=after_pin(SELF),
            source_sha256_after={name: after_pin(ROOT / name) for name in sources},
            artifact_sha256_after={name: after_pin(ROOT / name) for name in artifacts}, closure_errors=closure_errors)
        if closure_errors:
            completion["status"] = "failed"
            if completion["error"] is None:
                completion["error"] = "closure pin collection failed"
        reader.write_json(output / "COMPLETION.json", completion)
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
