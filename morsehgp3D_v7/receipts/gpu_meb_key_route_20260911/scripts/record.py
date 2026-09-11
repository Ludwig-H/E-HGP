#!/usr/bin/env python3
"""Create-only MEB/key local gate captures; CUDA is compiled, never executed."""
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
    parser.add_argument("--mode", required=True, choices=("stub", "san", "nvcc"))
    args = parser.parse_args()
    out = ROOT / args.out
    out.mkdir(exist_ok=False)
    sources = sorted(path for path in (ROOT / "source").rglob("*") if path.is_file())
    sources += [ROOT / "record.py", ROOT / "prepare.py", ROOT / "pins.json", ROOT / "nvcc_strict_host.py"]
    before = {str(path.relative_to(ROOT)): sha(path) for path in sources}
    for path in sources:
        target = out / "source_snapshot" / path.relative_to(ROOT)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
    gate = out / "source_snapshot/source/morsehgp3D_v7/tests/anchor_meb_key_route_gate.cu"
    adapter = out / "source_snapshot/nvcc_strict_host.py"
    receipt = {"mode": args.mode, "status": "running", "snapshot_commit": "c03f6be8488453486b112811071827a96303ec86",
               "sources_before": before, "commands": [], "device_executed": False, "GCP_used": False}
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    receipt["sanitizer_environment"] = {key: env[key] for key in ("ASAN_OPTIONS", "UBSAN_OPTIONS")}
    def save():
        (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    def run(name, argv, expected=0):
        started = time.time_ns()
        with (out / (name + ".stdout")).open("wb") as stdout, (out / (name + ".stderr")).open("wb") as stderr:
            done = subprocess.run(argv, cwd=REPO, env=env, stdout=stdout, stderr=stderr)
        receipt["commands"].append({"name": name, "argv": argv, "expected_exit_code": expected,
            "exit_code": done.returncode, "started_ns": started, "ended_ns": time.time_ns(),
            "stdout_sha256": sha(out / (name + ".stdout")), "stderr_sha256": sha(out / (name + ".stderr"))})
        save(); print(name, done.returncode, flush=True)
        if done.returncode != expected:
            raise RuntimeError(name + " unexpected exit")
    try:
        if args.mode == "nvcc":
            toolkit = REPO / "build/v7_nvcc_pedantic_20260910/toolkit"
            run("version", [str(toolkit / "bin/nvcc"), "--version"])
            run("compile_link", [str(toolkit / "bin/nvcc"), "-std=c++20", "-O2", "--gpu-architecture=sm_120",
                "-fmad=false", "--expt-relaxed-constexpr", "-Xcompiler=-Wall,-Wextra,-Wpedantic,-Werror",
                "-ccbin", str(adapter), "-L" + str(toolkit / "lib"),
                "-MMD", "-MF", str(out / "gate.d"), str(gate), "-o", str(out / "device_gate")])
            receipt["binary_sha256"] = sha(out / "device_gate")
        else:
            flags = ["-O2"] if args.mode == "stub" else ["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined"]
            run("compiler", ["g++", "--version"])
            run("compile", ["g++", "-x", "c++", "-std=c++20", *flags, "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread",
                "-MMD", "-MF", str(out / "gate.d"), str(gate), "-o", str(out / "stub_gate")])
            receipt["binary_sha256"] = sha(out / "stub_gate")
            run("selftest", [str(out / "stub_gate"), "--selftest"])
            run("unknown", [str(out / "stub_gate"), "--unknown"], 2)
            run("missing_arg", [str(out / "stub_gate")], 2)
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
