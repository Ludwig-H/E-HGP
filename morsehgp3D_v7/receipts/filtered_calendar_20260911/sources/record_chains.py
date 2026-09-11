#!/usr/bin/env python3
"""Freeze and run the private historical-query gate. No cloud operations."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

BASE = Path(__file__).resolve().parent
SOURCES = ("filtered_calendar.hpp", "historical_chains.hpp",
           "historical_chains_gate.cpp", "record_chains.py")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path, value):
    with path.open("x") as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write("\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--san", action="store_true")
    args = parser.parse_args()
    if not args.out.isidentifier():
        raise ValueError("fresh local output name required")
    out = BASE / args.out
    out.mkdir()
    before = {name: sha(BASE / name) for name in SOURCES}
    for name in SOURCES:
        shutil.copyfile(BASE / name, out / name)
    environment = dict(os.environ)
    sanitizer_environment = {}
    if args.san:
        sanitizer_environment = {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
                                 "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
        environment.update(sanitizer_environment)
    flags = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread"]
    flags += (["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
               "-fno-pie", "-no-pie"] if args.san else ["-O2"])
    binary = out / "gate"
    commands = []
    status, error = "failed", None
    try:
        for name, argv in (("compiler", ["g++", "--version"]),
                           ("compile", [*flags, str(out / "historical_chains_gate.cpp"), "-o", str(binary)]),
                           ("selftest", [str(binary)])):
            with (out / (name + ".stdout")).open("xb") as so, (out / (name + ".stderr")).open("xb") as se:
                run = subprocess.run(argv, stdout=so, stderr=se, env=environment, check=False)
            commands.append({"name": name, "argv": argv, "exit_code": run.returncode,
                             "stdout_sha256": sha(out / (name + ".stdout")),
                             "stderr_sha256": sha(out / (name + ".stderr"))})
            print(name, run.returncode, flush=True)
            if run.returncode or (out / (name + ".stderr")).read_bytes():
                raise ValueError("process or diagnostic failure: " + name)
            if name == "compile" and (out / (name + ".stdout")).read_bytes():
                raise ValueError("unexpected compile output")
        result = json.loads((out / "selftest.stdout").read_text())
        if (result.get("status") != "passed" or result.get("checks", 0) < 922537
                or result.get("queries") != 307500 or result.get("rejected") != 9
                or result.get("workers") != [1, 2, 4] or result.get("geometry_tested") is not False
                or result.get("construction_parallel") is not False
                or result.get("batch_parallel") is not True or result.get("gcp_used") is not False):
            raise ValueError("result/nonvacuity contract")
        status = "passed"
    except BaseException as exc:
        error = type(exc).__name__ + ": " + str(exc)
    finally:
        after = {name: sha(BASE / name) for name in SOURCES}
        copied = {name: sha(out / name) for name in SOURCES}
        if before != after or before != copied:
            status, error = "failed", "source instability"
        write_json(out / "receipt.json", {"status": status, "error": error, "commands": commands,
                   "sources_before": before, "sources_after": after, "sources_copied": copied,
                   "binary_sha256": sha(binary) if binary.is_file() else None,
                   "sanitizer": args.san, "sanitizer_environment": sanitizer_environment,
                   "gcp_used": False, "geometry_qualified": False, "source_stable": before == after == copied})
    print(json.dumps({"status": status, "error": error, "output": str(out)}))
    if status != "passed":
        raise SystemExit(1)


if __name__ == "__main__":
    main()
