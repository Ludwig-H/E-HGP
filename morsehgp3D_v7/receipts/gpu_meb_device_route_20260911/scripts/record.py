#!/usr/bin/env python3
"""Create-only local qualification. Never executes CUDA or invokes GCP."""
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
SOURCE = ROOT / "source/morsehgp3D_v7"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--mode", required=True, choices=("fixture", "stub", "san", "nvcc"))
    args = parser.parse_args()
    out = ROOT / args.out
    out.mkdir(exist_ok=False)
    sources = sorted(p for p in (ROOT / "source").rglob("*") if p.is_file())
    sources += [ROOT / "record.py", ROOT / "nvcc_strict_host.py"]
    before = {str(p.relative_to(ROOT)): sha(p) for p in sources}
    for source in sources:
        target = out / "source_snapshot" / source.relative_to(ROOT)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    receipt = {"mode": args.mode, "status": "running", "snapshot_commit": "ce842a3f",
               "sources_before": before, "commands": [], "device_executed": False, "gcp_used": False}
    environment = os.environ.copy()
    environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    receipt["sanitizer_environment"] = {key: environment[key] for key in ("ASAN_OPTIONS", "UBSAN_OPTIONS")}
    def save():
        (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    def run(name, argv, expected=0):
        started = time.time()
        with (out / (name + ".stdout")).open("wb") as stdout, (out / (name + ".stderr")).open("wb") as stderr:
            done = subprocess.run(argv, cwd=REPO, env=environment, stdout=stdout, stderr=stderr)
        receipt["commands"].append({"name": name, "argv": argv, "expected_exit_code": expected,
            "exit_code": done.returncode, "elapsed_seconds": time.time() - started,
            "stdout_sha256": sha(out / (name + ".stdout")), "stderr_sha256": sha(out / (name + ".stderr"))})
        save(); print(name, done.returncode, flush=True)
        if done.returncode != expected:
            raise RuntimeError(name + " failed")
    try:
        if args.mode == "fixture":
            run("compile", ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-isystem", str(REPO / "build/v7_boost_gate/extracted/usr/include"),
                str(SOURCE / "tests/anchor_meb_fixture_generator.cpp"), "-o", str(out / "generator")])
            run("generate", [str(out / "generator"), "--selftest"])
            target = SOURCE / "tests/anchor_meb_fixtures.inc"
            with target.open("xb") as stream:
                stream.write((out / "generate.stdout").read_bytes())
            receipt["generated_fixture_sha256"] = sha(target)
        elif args.mode == "nvcc":
            toolkit = REPO / "build/v7_nvcc_pedantic_20260910/toolkit"
            run("version", [str(toolkit / "bin/nvcc"), "--version"])
            run("compile_link", [str(toolkit / "bin/nvcc"), "-std=c++20", "-O2", "--gpu-architecture=sm_120",
                "-fmad=false", "--expt-relaxed-constexpr", "-Xcompiler=-Wall,-Wextra,-Wpedantic,-Werror",
                "-ccbin", str(ROOT / "nvcc_strict_host.py"), "-L" + str(toolkit / "lib"),
                str(SOURCE / "tests/anchor_meb_route_device_gate.cu"), "-o", str(out / "device_gate")])
            receipt["binary_sha256"] = sha(out / "device_gate")
        else:
            flags = (["-O2"] if args.mode == "stub" else ["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined"])
            run("compile", ["g++", "-x", "c++", "-std=c++20", *flags, "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                str(SOURCE / "tests/anchor_meb_route_device_gate.cu"), "-o", str(out / "stub_gate")])
            run("selftest", [str(out / "stub_gate"), "--selftest"])
            run("unknown", [str(out / "stub_gate"), "--unknown"], 2)
            run("missing_arg", [str(out / "stub_gate")], 2)
        receipt["status"] = "passed"
    except Exception as error:
        receipt["status"] = "failed"; receipt["error"] = str(error)
    finally:
        receipt["sources_after"] = {str(p.relative_to(ROOT)): sha(p) for p in sources}
        receipt["sources_stable"] = before == receipt["sources_after"]
        if not receipt["sources_stable"]:
            receipt["status"] = "failed"
        save()
    return 0 if receipt["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
