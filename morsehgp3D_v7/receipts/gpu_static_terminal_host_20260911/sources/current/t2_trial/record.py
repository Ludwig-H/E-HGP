#!/usr/bin/env python3
"""Fresh T2 capture using one composed terminal source snapshot."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parent
BASE = ROOT.parent
REPO = BASE.parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--mode", required=True, choices=("stub", "san"))
    args = parser.parse_args()
    out = ROOT / args.out
    out.mkdir(exist_ok=False)
    sources = sorted(path for path in (BASE / "source").rglob("*") if path.is_file())
    sources += sorted(path for path in BASE.iterdir() if path.is_file() and
        (path.suffix in (".cpp", ".cu", ".cuh", ".hpp", ".py") or path.name == "baseline_pins.json"))
    sources += sorted(path for path in ROOT.iterdir() if path.is_file())
    before = {str(path.relative_to(BASE)): sha(path) for path in sources}
    snapshot = out / "source_snapshot"
    for path in sources:
        target = snapshot / path.relative_to(BASE)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
    receipt = {"status": "running", "mode": args.mode, "sources_before": before, "commands": [],
        "device_executed": False, "gcp_used": False, "inherited_T2_results": False}
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    receipt["sanitizer_environment"] = {key: env[key] for key in ("ASAN_OPTIONS", "UBSAN_OPTIONS")}
    def save():
        (out / "receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    def run(name, argv, expected=0, diagnostic=None):
        start = time.time_ns()
        with (out / (name + ".stdout")).open("wb") as stdout, (out / (name + ".stderr")).open("wb") as stderr:
            done = subprocess.run(argv, cwd=REPO, env=env, stdout=stdout, stderr=stderr)
        causal = diagnostic is None or diagnostic in (out / (name + ".stderr")).read_text()
        receipt["commands"].append({"name": name, "argv": argv, "exit_code": done.returncode,
            "expected_exit_code": expected, "causal_diagnostic": diagnostic, "causal_diagnostic_matched": causal,
            "started_ns": start, "ended_ns": time.time_ns(),
            "stdout_sha256": sha(out / (name + ".stdout")), "stderr_sha256": sha(out / (name + ".stderr"))})
        save(); print(name, done.returncode, flush=True)
        if done.returncode != expected or not causal:
            raise RuntimeError(name + " unexpected result")
    try:
        run("compiler", ["g++", "--version"])
        flags = ["-O2"] if args.mode == "stub" else ["-O1", "-g", "-fno-omit-frame-pointer",
            "-fsanitize=address,undefined", "-fno-sanitize-recover=all"]
        run("compile", ["g++", "-std=c++20", *flags, "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread",
            "-isystem", str(REPO / "build/v7_boost_gate/extracted/usr/include"), "-I", str(snapshot),
            "-MMD", "-MF", str(out / "gate.d"), str(snapshot / "t2_trial/gate.cpp"), "-o", str(out / "gate")])
        receipt["binary_sha256"] = sha(out / "gate")
        for mode in ("historical", "line12", "shell14", "spatial12", "rejects"):
            run(mode, [str(out / "gate"), "--" + mode])
        for mode, diagnostic in (("mutant-assignment", "T2 unassigned subset"),
            ("mutant-open", "T2.historical.exact_Gamma"), ("mutant-adjacency", "T2.historical.exact_Gamma"),
            ("mutant-census", "T2.census.exhaustive_ball_inventory")):
            run(mode, [str(out / "gate"), "--" + mode], 1, diagnostic)
        run("unknown", [str(out / "gate"), "--unknown"], 2)
        run("missing_arg", [str(out / "gate")], 2)
        receipt["status"] = "passed"
    except Exception as error:
        receipt["status"] = "failed"; receipt["error"] = str(error)
    finally:
        receipt["sources_after"] = {str(path.relative_to(BASE)): sha(path) for path in sources}
        receipt["sources_stable"] = before == receipt["sources_after"]
        if not receipt["sources_stable"]: receipt["status"] = "failed"
        save()
    return 0 if receipt["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
