#!/usr/bin/env python3
"""Create-only real CMake/CTest qualification of the private permanent gate."""
import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]
ACTIVE = REPO / "morsehgp3D_v7"
TARGET_NAME = "mhgp7_census_tower_gate"
TEST_NAMES = {"mhgp7_census_tower_" + name for name in (
    "historical", "line12", "shell14", "spatial12", "rejects", "bad_argument", "missing_argument",
    "mutant_assignment", "mutant_open", "mutant_adjacency", "mutant_census")}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def pins(root):
    return {path.relative_to(root).as_posix(): sha(path) for path in sorted(root.rglob("*")) if path.is_file()}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--san", action="store_true")
    args = parser.parse_args()
    relative = Path(args.out)
    if relative.is_absolute() or len(relative.parts) != 1 or str(relative) in ("", ".", ".."):
        raise RuntimeError("out must be a fresh local name")
    out = ROOT / relative
    out.mkdir(exist_ok=False)
    origin = json.loads((ROOT / "origin.json").read_text())
    proposal = [path for path in sorted(ROOT.iterdir()) if path.is_file()]
    for directory in ("candidate", "originals", "baseline", "overlay"):
        proposal += [path for path in sorted((ROOT / directory).rglob("*")) if path.is_file()]
    source = []
    for directory in ("src", "tests", "oracle", "bench", "cli", "cmake"):
        source += [path for path in sorted((ACTIVE / directory).rglob("*")) if path.is_file()]
    source.append(ACTIVE / "CMakeLists.txt")
    before = {str(path.relative_to(ACTIVE)): sha(path) for path in source}
    proposal_before = {str(path.relative_to(ROOT)): sha(path) for path in proposal}
    snapshot = out / "source_snapshot"
    v7 = snapshot / "morsehgp3D_v7"
    build = out / "cmake_build"
    receipt = {"status": "running", "mode": "san" if args.san else "o2", "commands": [],
        "baseline_before": before, "proposal_before": proposal_before,
        "compiler_jobs": 1, "CTest_jobs": 1, "device_executed": False, "gcp_used": False,
        "public_status": "not_claimed", "origin_sha256": sha(ROOT / "origin.json")}
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    receipt["sanitizer_environment"] = {key: env[key] for key in ("ASAN_OPTIONS", "UBSAN_OPTIONS")}

    def save():
        (out / "receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")

    def run(name, argv):
        started = time.time_ns()
        with (out / (name + ".stdout")).open("wb") as stdout, (out / (name + ".stderr")).open("wb") as stderr:
            done = subprocess.run(argv, cwd=REPO, env=env, stdout=stdout, stderr=stderr)
        receipt["commands"].append({"name": name, "argv": argv, "exit_code": done.returncode, "expected_exit_code": 0,
            "started_ns": started, "ended_ns": time.time_ns(), "stdout_sha256": sha(out / (name + ".stdout")),
            "stderr_sha256": sha(out / (name + ".stderr"))})
        save()
        print(name, done.returncode, flush=True)
        if done.returncode != 0:
            raise RuntimeError(name + " unexpected exit")

    try:
        for name, pin in origin["baseline_source_pins"].items():
            if before.get(name) != pin:
                raise RuntimeError("prepared baseline changed: " + name)
        for path in source:
            if path.is_symlink():
                raise RuntimeError("source symlink")
            target = v7 / path.relative_to(ACTIVE)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)
        for path in proposal:
            target = snapshot / "proposal" / path.relative_to(ROOT)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)
        for path in sorted((ROOT / "candidate/morsehgp3D_v7/tests").glob("*")):
            if (v7 / "tests" / path.name).exists():
                raise RuntimeError("candidate would overwrite an active test")
            shutil.copy2(path, v7 / "tests" / path.name)
        overlay = json.loads((ROOT / "overlay.json").read_text())
        for name, pin in overlay["files"].items():
            path = ROOT / "overlay/morsehgp3D_v7" / name
            if path.is_symlink() or sha(path) != pin:
                raise RuntimeError("explicit product overlay drift")
            shutil.copy2(path, v7 / name)
        receipt["product_overlay"] = overlay
        receipt["product_header_sha256"] = sha(v7 / "src/forest/full_ball_tower.hpp")
        cmake = (v7 / "CMakeLists.txt").read_text()
        anchor = "mhgp7_product_executable(mhgp7_full_ball_work_gate tests/full_ball_work_gate.cpp)"
        if cmake.count(anchor) != 1:
            raise RuntimeError("CMake insertion anchor drift")
        updated = cmake.replace(anchor, (ROOT / "CMake.fragment").read_text() + anchor)
        patch = "".join(difflib.unified_diff(cmake.splitlines(keepends=True), updated.splitlines(keepends=True),
            fromfile="a/morsehgp3D_v7/CMakeLists.txt", tofile="b/morsehgp3D_v7/CMakeLists.txt"))
        if patch != (ROOT / "CMake.diff").read_text():
            raise RuntimeError("CMake patch drift")
        (v7 / "CMakeLists.txt").write_text(updated)
        receipt["snapshot_before"] = pins(snapshot)
        run("compiler", ["g++", "--version"])
        flags = "-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all" if args.san else "-O2 -DNDEBUG"
        run("configure", ["cmake", "-S", str(v7), "-B", str(build), "-DCMAKE_BUILD_TYPE=Release",
            "-DCMAKE_CXX_COMPILER=g++", "-DCMAKE_CXX_FLAGS_RELEASE=" + flags,
            "-DMHGP7_DIGEST_BOOST_INCLUDE_DIR=" + str(REPO / "build/v7_boost_gate/extracted/usr/include"),
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"])
        run("build", ["cmake", "--build", str(build), "--target", TARGET_NAME, "--parallel", "1"])
        receipt["binary_sha256"] = sha(build / TARGET_NAME)
        receipt["compile_commands_sha256"] = sha(build / "compile_commands.json")
        run("inventory", ["ctest", "--test-dir", str(build), "--show-only=json-v1", "-R", "^mhgp7_census_tower_"])
        tests = json.loads((out / "inventory.stdout").read_text())["tests"]
        if {test["name"] for test in tests} != TEST_NAMES or len(tests) != 11:
            raise RuntimeError("CTest registration incomplete")
        run("ctest", ["ctest", "--test-dir", str(build), "--output-on-failure", "-j1", "-R", "^mhgp7_census_tower_"])
        last = build / "Testing/Temporary/LastTest.log"
        shutil.copy2(last, out / "LastTest.log")
        receipt["ctest_log_sha256"] = sha(out / "LastTest.log")
        results = [json.loads(line) for line in last.read_text().splitlines() if line.startswith('{"status":')]
        real = [data for data in results if data.get("scope") == "bounded_real_census_FULL_K1_K10"]
        if len(real) != 3 or any(data.get("status") != "passed" or data.get("clouds") != 2 or
                data.get("orders") != 180 or data.get("census_runs") != 6 or data.get("physical_tower_pairs") != 16 or
                data.get("high_order_facets", 0) <= 0 or data.get("high_order_verticals", 0) <= 0 for data in real):
            raise RuntimeError("real census/tower nonvacuity absent in CTest output")
        if not any(data.get("shell12_rows") == 6 for data in real) or \
                not any(data.get("q3_rows", 0) > 0 and data.get("q4_rows", 0) > 0 for data in real):
            raise RuntimeError("shell/three-dimensional support nonvacuity absent")
        receipt["CTest_results"] = results
        receipt["status"] = "passed"
    except Exception as error:
        receipt["status"] = "failed"
        receipt["error"] = str(error)
    finally:
        last = build / "Testing/Temporary/LastTest.log"
        if last.exists():
            shutil.copy2(last, out / "LastTest.log")
            receipt["ctest_log_sha256"] = sha(out / "LastTest.log")
        receipt["baseline_after"] = {str(path.relative_to(ACTIVE)): sha(path) for path in source}
        receipt["proposal_after"] = {str(path.relative_to(ROOT)): sha(path) for path in proposal}
        receipt["sources_stable"] = receipt["baseline_before"] == receipt["baseline_after"] and \
            receipt["proposal_before"] == receipt["proposal_after"]
        if "snapshot_before" in receipt:
            receipt["snapshot_after"] = pins(snapshot)
            receipt["snapshot_stable"] = receipt["snapshot_before"] == receipt["snapshot_after"]
        else:
            receipt["snapshot_stable"] = False
        if not receipt["sources_stable"] or not receipt["snapshot_stable"]:
            receipt["status"] = "failed"
        save()
    return 0 if receipt["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
