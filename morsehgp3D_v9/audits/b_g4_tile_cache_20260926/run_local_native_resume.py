#!/usr/bin/env python3
"""Fresh, bounded Release revalidation of the portable S2 tile cache.

This writes audit receipts, never product sources. It deliberately refuses
to reuse a build or output directory. No CUDA/GCP execution is implied.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path: Path, value: object) -> None:
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--boost-root", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    source = root / "morsehgp3D_v9"
    if args.build.exists() or args.output.exists():
        parser.error("fresh build and output directories required")
    args.build.mkdir(parents=True)
    args.output.mkdir(parents=True)
    tracked = subprocess.check_output(
        ["git", "ls-files", "morsehgp3D_v9/src", "morsehgp3D_v9/tests/gpu",
         "morsehgp3D_v9/bench/gpu_filter_probe.cpp", "morsehgp3D_v9/CMakeLists.txt",
         "morsehgp3D_v9/cmake", "morsehgp3D_v9/audits/b_g4_tile_cache_20260926/mapping_gate.cpp"],
        cwd=root, text=True).splitlines()
    before = {name: digest(root / name) for name in tracked}
    records: list[dict[str, object]] = []
    manifest: dict[str, object] = {
        "schema": "mhgp9_tile_cache_native_resume_v1",
        "scope": "portable_cpu_release_only_not_cuda_not_full",
        "git_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(),
        "git_status_start": subprocess.check_output(["git", "status", "--short"], cwd=root, text=True),
        "build": str(args.build), "boost_root": str(args.boost_root),
        "runner_sha256": digest(Path(__file__)), "source_before": before,
        "commands": records, "status": "running", "gcp_used": False,
    }
    write_json(args.output / "manifest.json", manifest)

    def run(label: str, command: list[str], expected: int = 0,
            required: str = "", timeout: int = 600) -> None:
        started = time.monotonic()
        filename = f"{len(records):02d}_{label}"
        record: dict[str, object] = {
            "label": label, "command": command, "cwd": str(root),
            "expected_exit": expected, "required_text": required,
        }
        try:
            result = subprocess.run(command, cwd=root, capture_output=True,
                                    timeout=timeout, check=False)
            stdout, stderr, code = result.stdout, result.stderr, result.returncode
        except subprocess.TimeoutExpired as error:
            stdout, stderr, code = error.stdout or b"", error.stderr or b"", None
            record["timed_out"] = True
        (args.output / (filename + ".stdout")).write_bytes(stdout)
        (args.output / (filename + ".stderr")).write_bytes(stderr)
        record.update({
            "exit_code": code, "wall_seconds": time.monotonic() - started,
            "stdout": filename + ".stdout", "stderr": filename + ".stderr",
            "stdout_sha256": hashlib.sha256(stdout).hexdigest(),
            "stderr_sha256": hashlib.sha256(stderr).hexdigest(),
            "pass": code == expected and required in (stdout + stderr).decode(errors="replace"),
        })
        records.append(record)
        write_json(args.output / "manifest.json", manifest)
        print(f"{label}: exit={code} pass={record['pass']} wall={record['wall_seconds']:.3f}s", flush=True)
        if not record["pass"]:
            raise RuntimeError(f"{label} failed; raw outputs retained")

    failure = None
    try:
        run("compiler", ["g++", "--version"])
        run("configure", ["cmake", "-S", str(source), "-B", str(args.build),
                          "-DCMAKE_BUILD_TYPE=Release", f"-DBOOST_ROOT={args.boost_root}",
                          "-DMHGP9_ENABLE_CUDA=OFF"])
        run("build", ["cmake", "--build", str(args.build), "--parallel", "4", "--target",
                      "mhgp9_gpu_witness_filter_port_gate", "mhgp9_gpu_witness_cache_port_gate",
                      "mhgp9_gpu_witness_cache_no_retest_mutant_gate", "mhgp9_gpu_filter_probe"], timeout=1200)
        run("five_gates", ["ctest", "--test-dir", str(args.build), "-V", "-R",
                           "^mhgp9_gpu_witness_(filter_port|filter_port_bad_argument|cache_port|cache_port_bad_argument|cache_no_retest_mutant)$"],
            required="100% tests passed, 0 tests failed out of 5", timeout=900)
        run("probe_refusals", ["ctest", "--test-dir", str(args.build), "-V", "-R",
                               "^mhgp9_gpu_filter_probe_(bad_argument|empty_input)$"],
            required="100% tests passed, 0 tests failed out of 2")
        mapping = source / "audits/b_g4_tile_cache_20260926/mapping_gate.cpp"
        common = ["g++", "-std=c++20", "-O3", "-DNDEBUG", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
        for mutant in (False, True):
            name = "mapping_mutant" if mutant else "mapping_gate"
            command = common + (["-DMHGP9_TILE_MAPPING_MUTANT_FLAT=1"] if mutant else [])
            run("compile_" + name, command + [str(mapping), "-o", str(args.build / name)])
            run(name, [str(args.build / name)], expected=1 if mutant else 0,
                required="cause=tile_mapping.row_representative" if mutant else '"status":"pass"')
    except BaseException as error:
        failure = f"{type(error).__name__}: {error}"
    after = {name: digest(root / name) for name in tracked}
    binaries = ["mhgp9_gpu_witness_filter_port_gate", "mhgp9_gpu_witness_cache_port_gate",
                "mhgp9_gpu_witness_cache_no_retest_mutant_gate", "mhgp9_gpu_filter_probe",
                "mapping_gate", "mapping_mutant"]
    manifest.update({
        "source_after": after, "sources_unchanged": before == after,
        "binaries": {name: digest(args.build / name) for name in binaries if (args.build / name).is_file()},
        "git_status_end": subprocess.check_output(["git", "status", "--short"], cwd=root, text=True),
        "status": "passed" if failure is None and before == after else "failed",
        "failure": failure,
    })
    write_json(args.output / "manifest.json", manifest)
    if failure:
        print(failure, file=sys.stderr)
    return 0 if manifest["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
