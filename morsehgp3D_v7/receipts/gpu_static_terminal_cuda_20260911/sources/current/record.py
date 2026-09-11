#!/usr/bin/env python3
"""Create-only local host helper/FULL captures from an actual frozen snapshot."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--mode", choices=("stub", "san"), required=True)
    args = parser.parse_args()
    out = ROOT / args.out
    out.mkdir(exist_ok=False)
    sources = sorted(path for path in (ROOT / "source").rglob("*") if path.is_file())
    sources += sorted(path for path in ROOT.iterdir() if path.is_file() and
        (path.suffix in (".cpp", ".cu", ".cuh", ".hpp", ".py") or path.name == "baseline_pins.json"))
    before = {str(path.relative_to(ROOT)): sha(path) for path in sources}
    for path in sources:
        target = out / "source_snapshot" / path.relative_to(ROOT)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
    receipt = {"status": "running", "mode": args.mode, "base_commit": "c03f6be8488453486b112811071827a96303ec86",
        "sources_before": before, "commands": [], "device_executed": False, "gcp_used": False}
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    receipt["sanitizer_environment"] = {name: env[name] for name in ("ASAN_OPTIONS", "UBSAN_OPTIONS")}
    def save():
        (out / "receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    def run(name, argv, expected=0):
        started = time.time_ns()
        with (out / (name + ".stdout")).open("wb") as stdout, (out / (name + ".stderr")).open("wb") as stderr:
            done = subprocess.run(argv, cwd=REPO, env=env, stdout=stdout, stderr=stderr)
        receipt["commands"].append({"name": name, "argv": argv, "exit_code": done.returncode,
            "expected_exit_code": expected, "started_ns": started, "ended_ns": time.time_ns(),
            "stdout_sha256": sha(out / (name + ".stdout")), "stderr_sha256": sha(out / (name + ".stderr"))})
        save(); print(name, done.returncode, flush=True)
        if done.returncode != expected:
            raise RuntimeError(name + " unexpected exit")
    try:
        run("compiler", ["g++", "--version"])
        flags = ["-O2"] if args.mode == "stub" else ["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined"]
        run("compile", ["g++", "-std=c++20", *flags, "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread",
            "-isystem", str(REPO / "build/v7_boost_gate/extracted/usr/include"), "-MMD", "-MF", str(out / "gate.d"),
            str(out / "source_snapshot/gate.cpp"), "-o", str(out / "gate")])
        receipt["binary_sha256"] = sha(out / "gate")
        run("static1", [str(out / "gate"), "--selftest"])
        run("static4", [str(out / "gate"), "--static-4"])
        run("unknown", [str(out / "gate"), "--unknown"], 2)
        run("missing_arg", [str(out / "gate")], 2)
        receipt["status"] = "passed"
    except Exception as error:
        receipt["status"] = "failed"; receipt["error"] = str(error)
    finally:
        receipt["sources_after"] = {str(path.relative_to(ROOT)): sha(path) for path in sources}
        receipt["sources_stable"] = before == receipt["sources_after"]
        if not receipt["sources_stable"]: receipt["status"] = "failed"
        save()
    return 0 if receipt["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
