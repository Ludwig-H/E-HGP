#!/usr/bin/env python3
"""Capture one full Release CTest regression (82 tests); never build anything."""
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
from run_q4_family_checks import SOURCES, digest, pins, read_json  # noqa: E402
from run_p0_matrix import invoke, on_signal, require, utc_stamp, write_json  # noqa: E402

SCHEMA = "mhgp8_reprise_regression_v1"
TESTS = 82


def artifacts(build):
    names = sorted(path.name for path in build.glob("mhgp8_*")
                   if path.is_file() and os.access(path, os.X_OK))
    require(names and len(names) == len(set(names)), "empty or duplicate executable inventory")
    require(all((build / name).resolve() == build / name for name in names),
            "executable inventory must contain real build files, not symlinks")
    paths = [str((build / name).relative_to(ROOT)) for name in names + ["CMakeCache.txt"]]
    return names, frozenset(paths)


def command(ctest, build, capture):
    return [ctest, "--test-dir", str(build), "--output-on-failure", "--parallel", "2",
            "--output-junit", str(capture / "result.xml")]


def environment_record(environment):
    # Preserve execution-relevant non-secret values, not account/API credentials.
    selected = {key: value for key, value in environment.items()
                if key in {"PATH", "LANG", "LC_ALL", "LC_CTYPE", "TZ", "LD_LIBRARY_PATH", "LD_PRELOAD"}
                or key.startswith(("ASAN_", "UBSAN_", "TSAN_", "LSAN_", "CTEST_", "OMP_", "PYTHON"))}
    encoded = json.dumps(environment, sort_keys=True, ensure_ascii=True).encode()
    return dict(selected=selected, names=sorted(environment),
                complete_sha256=hashlib.sha256(encoded).hexdigest(),
                scope="selected_values_full_environment_fingerprint_no_secrets")


def git_state():
    def run(*arguments):
        return subprocess.check_output(["git", *arguments], cwd=ROOT, text=True).strip()
    return dict(commit=run("rev-parse", "HEAD"), branch=run("branch", "--show-current"),
                status=run("status", "--short"))


def validate_xml(path):
    tree = ET.parse(path).getroot()
    require(tree.tag == "testsuite" and tree.get("tests") == str(TESTS), "wrong CTest XML test count")
    require(all(tree.get(key) == "0" for key in ("failures", "disabled", "skipped")),
            "CTest XML records unsuccessful tests")
    cases = tree.findall("testcase")
    names = [case.get("name") for case in cases]
    require(len(cases) == TESTS and len(set(names)) == TESTS and
            all(type(name) is str and name.startswith("mhgp8_") for name in names),
            "CTest XML lacks 82 distinct v8 tests")
    require("mhgp8_q4_family_gate" in names, "q4 gate missing from full regression")
    require(all(case.get("status") == "run" and
                all(case.find(tag) is None for tag in ("failure", "error", "skipped")) for case in cases),
            "CTest XML contains a failed or unexecuted testcase")
    return sorted(names)


def run(args):
    build = args.build.resolve()
    require(build.is_dir() and build.is_relative_to(ROOT / "build"), "local existing build required")
    require(len(SOURCES) == 135, "source inventory changed: this capture expects 135 sources")
    cache = (build / "CMakeCache.txt").read_text()
    require("CMAKE_BUILD_TYPE:STRING=Release\n" in cache and
            "MHGP8_SANITIZE:BOOL=OFF\n" in cache, "full regression requires the Release non-sanitizer build")
    ctest = shutil.which("ctest")
    require(ctest is not None, "ctest executable unavailable")
    ctest = str(Path(ctest).resolve())
    args.output.mkdir(parents=True, exist_ok=True)
    capture = Path(tempfile.mkdtemp(prefix="regression_", dir=args.output)).resolve()
    names, files = artifacts(build)
    environment = dict(os.environ)
    manifest = dict(schema=SCHEMA, started_utc=utc_stamp(), build=str(build), tests=TESTS,
                    command=command(ctest, build, capture), cwd=str(ROOT), git=git_state(),
                    environment=environment_record(environment), affinity=sorted(os.sched_getaffinity(0)),
                    source_sha256=pins(SOURCES), executable_names=names, artifact_sha256=pins(files),
                    helper_sha256=digest(Path(__file__)), ctest_sha256=digest(Path(ctest)),
                    public_status="not_claimed", full_contract_qualified=False, gcp_used=False)
    write_json(capture / "MANIFEST.json", manifest)
    record = dict(command=manifest["command"], cwd=str(ROOT), environment=manifest["environment"],
                  started_utc=utc_stamp(), status="failed", exit_code=None,
                  stdout="", stderr="", stdout_base64="", stderr_base64="")
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    status, error, test_names = "failed", None, []
    try:
        invoke(manifest["command"], environment, ROOT, record, new_session=True)
        require(record["exit_code"] == 0, "full CTest command failed")
        test_names = validate_xml(capture / "result.xml")
        require(pins(SOURCES) == manifest["source_sha256"] and artifacts(build)[0] == names and
                pins(files) == manifest["artifact_sha256"] and
                digest(Path(__file__)) == manifest["helper_sha256"] and
                digest(Path(ctest)) == manifest["ctest_sha256"], "sources/artifacts changed during regression")
        record["status"] = status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        # After invoke has safely cancelled/joined the child, defer further
        # signals through closure so a repeated SIGTERM cannot discard logs.
        for sig in previous:
            signal.signal(sig, signal.SIG_IGN)
        record["finished_utc"] = utc_stamp()
        write_json(capture / "record.json", record)
        closing_errors = []

        def close_value(label, function):
            try:
                return function()
            except Exception as cause:
                closing_errors.append(f"{label}: {type(cause).__name__}: {cause}")
                return None

        completion = dict(status=status, error=error, finished_utc=utc_stamp(), test_names=test_names,
            manifest_sha256=digest(capture / "MANIFEST.json"), record_sha256=digest(capture / "record.json"),
            source_sha256_after=close_value("sources", lambda: pins(SOURCES)),
            artifact_sha256_after=close_value("artifacts", lambda: pins(files)),
            executable_names_after=close_value("inventory", lambda: artifacts(build)[0]),
            helper_sha256_after=close_value("helper", lambda: digest(Path(__file__))),
            ctest_sha256_after=close_value("ctest", lambda: digest(Path(ctest))),
            xml_sha256=digest(capture / "result.xml") if (capture / "result.xml").is_file() else None,
            closing_errors=closing_errors)
        if closing_errors or any(completion.get(after) != manifest[before] for before, after in (
                ("source_sha256", "source_sha256_after"), ("artifact_sha256", "artifact_sha256_after"),
                ("executable_names", "executable_names_after"), ("helper_sha256", "helper_sha256_after"),
                ("ctest_sha256", "ctest_sha256_after"))):
            completion["status"] = "failed"
            completion["error"] = error or "closure changed or could not be read"
        write_json(capture / "COMPLETION.json", completion)
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(path=str(capture), status=completion["status"], error=completion["error"])), flush=True)
    require(completion["status"] == "passed", "regression closure failed")


def read(capture, check_live=False):
    capture = capture.resolve()
    manifest = read_json(capture / "MANIFEST.json")
    completion = read_json(capture / "COMPLETION.json")
    require(manifest["schema"] == SCHEMA and manifest["tests"] == TESTS and
            completion["status"] == "passed" and completion["error"] is None and
            completion["closing_errors"] == [], "capture is not a successful closed regression")
    require(manifest["public_status"] == "not_claimed" and manifest["full_contract_qualified"] is False and
            manifest["gcp_used"] is False, "regression scope changed")
    build = Path(manifest["build"])
    require(build.is_absolute() and build.is_relative_to(ROOT / "build") and ".." not in build.parts,
            "invalid build path")
    require(manifest["cwd"] == str(ROOT) and
            manifest["command"] == command(manifest["command"][0], build, capture) and
            Path(manifest["command"][0]).name == "ctest" and Path(manifest["command"][0]).is_absolute(),
            "full regression command changed")
    require(manifest["git"]["branch"] == "main", "regression not captured on main")
    names = manifest["executable_names"]
    require(type(names) is list and names == sorted(set(names)) and names and
            all(type(name) is str and Path(name).name == name and name.startswith("mhgp8_") for name in names),
            "invalid executable inventory")
    expected = {str((build / name).relative_to(ROOT)) for name in names + ["CMakeCache.txt"]}
    require(set(manifest["source_sha256"]) == SOURCES and len(SOURCES) == 135 and
            set(manifest["artifact_sha256"]) == expected, "strict pin inventory mismatch")
    for before, after in (("source_sha256", "source_sha256_after"),
                          ("artifact_sha256", "artifact_sha256_after"),
                          ("executable_names", "executable_names_after"),
                          ("helper_sha256", "helper_sha256_after"), ("ctest_sha256", "ctest_sha256_after")):
        require(manifest[before] == completion[after], "closure hashes/inventory differ")
    for key, file in (("manifest_sha256", "MANIFEST.json"), ("record_sha256", "record.json"),
                      ("xml_sha256", "result.xml")):
        require(completion[key] == digest(capture / file), "capture file hash mismatch")
    record = read_json(capture / "record.json")
    require(record["command"] == manifest["command"] and record["cwd"] == str(ROOT) and
            record["environment"] == manifest["environment"] and record["status"] == "passed" and
            type(record["exit_code"]) is int and record["exit_code"] == 0, "command record mismatch")
    for stream in ("stdout", "stderr"):
        require(base64.b64decode(record[stream + "_base64"], validate=True).decode("utf-8", errors="replace") ==
                record[stream], "raw command output differs from decoded output")
    require(validate_xml(capture / "result.xml") == completion["test_names"], "XML test names changed")
    if check_live:
        require(pins(SOURCES) == manifest["source_sha256"] and artifacts(build)[0] == names and
                pins(expected) == manifest["artifact_sha256"] and
                digest(Path(__file__)) == manifest["helper_sha256"] and
                digest(Path(manifest["command"][0])) == manifest["ctest_sha256"], "live sources/artifacts differ")
    return dict(status="passed", path=str(capture), tests=TESTS, sources=len(SOURCES),
                executables=len(names), command_count=1, full_contract_qualified=False, gcp_used=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    capture = sub.add_parser("run")
    capture.add_argument("--build", type=Path, required=True)
    capture.add_argument("--output", type=Path, default=HERE)
    reader = sub.add_parser("read")
    reader.add_argument("path", type=Path)
    reader.add_argument("--check-live", action="store_true")
    args = parser.parse_args()
    if args.operation == "run":
        run(args)
    else:
        print(json.dumps(read(args.path, args.check_live), sort_keys=True, allow_nan=False))


if __name__ == "__main__":
    main()
