#!/usr/bin/env python3
"""Create-only private MEB selection qualification; one job at a time."""
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
    parser.add_argument("--mode", choices=("o2", "san", "nvcc"), required=True)
    args = parser.parse_args()
    out = ROOT / args.out
    out.mkdir(exist_ok=False)
    sources = [p for p in (ROOT / "source").rglob("*") if p.is_file()]
    sources += [p for p in ROOT.iterdir() if p.is_file() and p.suffix in {".py", ".cpp", ".cu", ".cuh", ".source"}]
    before = {p.relative_to(ROOT).as_posix(): sha(p) for p in sorted(sources)}
    for source in sources:
        target = out / "source_snapshot" / source.relative_to(ROOT)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    receipt = {"status": "running", "mode": args.mode, "sources_before": before,
        "commands": [], "device_executed": False, "gcp_used": False,
        "snapshot_commit": "ad7ffd28b35e153a20bd8cf42534d1cd29160bcd"}
    environment = os.environ.copy()
    environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    def save():
        (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    def command(name, argv, expected=0):
        start = time.time()
        with (out / (name + ".stdout")).open("wb") as stdout, (out / (name + ".stderr")).open("wb") as stderr:
            done = subprocess.run(argv, cwd=REPO, env=environment, stdout=stdout, stderr=stderr)
        receipt["commands"].append({"name": name, "argv": argv, "exit_code": done.returncode,
            "expected_exit_code": expected, "started_unix": start, "elapsed_seconds": time.time() - start,
            "stdout_sha256": sha(out / (name + ".stdout")), "stderr_sha256": sha(out / (name + ".stderr"))})
        save(); print(name, done.returncode, flush=True)
        if done.returncode != expected:
            raise RuntimeError(name + ": unexpected exit code")
    try:
        if args.mode == "nvcc":
            toolkit = REPO / "build/v7_nvcc_pedantic_20260910/toolkit"
            command("nvcc_version", [str(toolkit / "bin/nvcc"), "--version"])
            command("strict_kernel_compile_link", [str(toolkit / "bin/nvcc"), "-std=c++20", "-O2",
                "--gpu-architecture=sm_120", "-fmad=false", "--expt-relaxed-constexpr",
                "-Xcompiler=-Wall,-Wextra,-Wpedantic,-Werror", "-ccbin", str(ROOT / "nvcc_strict_host.py"),
                "-L" + str(toolkit / "lib"), "-MMD", "-MF", str(out / "kernel.d"),
                str(ROOT / "kernel_compile.cu"), "-o", str(out / "kernel")])
            receipt["kernel_binary_sha256"] = sha(out / "kernel")
        else:
            flags = (["-O2"] if args.mode == "o2" else
                     ["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined"])
            for name, source, runtime_args in (
                ("oracle", ROOT / "source/morsehgp3D_v7/tests/anchor_meb_gate.cpp", ["--selftest"]),
                ("transport", ROOT / "transport_gate.cpp", [])):
                binary = out / name
                command("compile_" + name, ["g++", "-std=c++20", *flags, "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-isystem", str(REPO / "build/v7_boost_gate/extracted/usr/include"),
                    "-MMD", "-MF", str(binary) + ".d", str(source), "-o", str(binary)])
                command("run_" + name, [str(binary), *runtime_args])
                command("args_" + name, [str(binary), "invalid"], 2)
        receipt["status"] = "passed"
    except Exception as error:
        receipt["status"] = "failed"; receipt["error"] = str(error)
    finally:
        after = {p.relative_to(ROOT).as_posix(): sha(p) for p in sorted(sources)}
        receipt["sources_after"] = after; receipt["sources_stable"] = before == after
        if not receipt["sources_stable"]: receipt["status"] = "failed"
        save()
    return 0 if receipt["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
