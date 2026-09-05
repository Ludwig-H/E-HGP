#!/usr/bin/env python3
"""Fresh execution of all 323 gates after an explicitly incremental Release build.

Uses the pinned historical run_release.py execution/snapshot helper, never its
results or output directory. No benchmark or cloud command; no result reuse.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shlex
import signal
import subprocess
import time
import xml.etree.ElementTree as ET

RECEIPT = Path(__file__).resolve().parent
ROOT = RECEIPT.parents[1]
SOURCE = ROOT / "morsehgp3D_v7"
BUILD = ROOT / "build/v7_meb_qualification"
OUT = RECEIPT / "full_release"
HELPER = SOURCE / "receipts/release_20260904/run_release.py"
EXPECTED_C = "25c9bf8e4ef3cded5647a22f16d81af7a1e778196ad3bff73884a7f58da985f2"
HELPER_SHA = "582922e64fcd2927be3e63c7156fefa839fd1f8c055f92a75c97514cad7a0ee5"
EXPECTED_D = "127c5f923fcc9618d826b89dedda4de0f5201ea48e27330e2ea68e83d76a1b3f"
D_BUILD_RECORD = ROOT / "build/v7_meb_build_20260905/build_D.json"
D_BUILD_SHA = "aedfe1b48d0b7bbef211a92d72c678e6cd6e9a53f20328db6740a0c8ec367812"
PROTECTED = {
    ROOT / "build/v7/mhgp7": EXPECTED_C,
    ROOT / "build/v7_c_qualification/mhgp7": EXPECTED_C,
    BUILD / "mhgp7": EXPECTED_D,
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def load_helper():
    source_bytes = HELPER.read_bytes()
    require(hashlib.sha256(source_bytes).hexdigest() == HELPER_SHA, "helper bytes changed")
    spec = importlib.util.spec_from_file_location("meb_full_release_helper", HELPER)
    require(spec is not None and spec.loader is not None, "helper unavailable")
    helper = importlib.util.module_from_spec(spec)
    exec(compile(source_bytes, str(HELPER), "exec"), helper.__dict__)
    helper.OUT, helper.BUILD = OUT, BUILD
    return helper


H = load_helper()


def expected_names() -> list[str]:
    names = json.loads((RECEIPT / "expected_test_names.json").read_text())
    require(isinstance(names, list) and len(names) == 323 and len(set(names)) == 323
            and all(isinstance(name, str) and name.startswith("mhgp7_") for name in names), "expected name inventory malformed")
    return sorted(names)


def judge_junit(document: ET.Element, expected: list[str]) -> dict:
    cases = document.findall(".//testcase")
    names = [case.get("name") for case in cases]
    require(len(names) == len(expected) and len(set(names)) == len(expected)
            and sorted(names) == expected, "JUnit names missing, repeated or unexpected")
    failed = [case.get("name") for case in cases
              if case.find("failure") is not None or case.find("error") is not None]
    skipped = [case.get("name") for case in cases
               if case.find("skipped") is not None or case.get("status") != "run"]
    require(not failed and not skipped, "JUnit failed, skipped or non-run case")
    return {"tests": len(cases), "names": sorted(names), "failed": failed, "skipped": skipped}


def commands() -> list[tuple[str, list[str], int]]:
    return [
        ("configure", ["cmake", "-S", str(SOURCE), "-B", str(BUILD), "-DCMAKE_BUILD_TYPE=Release",
                       "-DMHGP7_ENABLE_CUDA=OFF", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"], 120),
        ("build_incremental", ["cmake", "--build", str(BUILD), "--parallel", "2"], 3600),
        ("inventory", ["ctest", "--test-dir", str(BUILD), "--show-only=json-v1", "-L", "^gate$"], 60),
        ("ctest", ["ctest", "--test-dir", str(BUILD), "--output-on-failure", "--no-tests=error",
                   "-L", "^gate$", "--parallel", "2", "--output-junit", str(OUT / "ctest.junit.xml")], 7200),
    ]


def input_inventory() -> dict:
    return {str(path): H.digest(path) for path in
            (Path(__file__), RECEIPT / "expected_test_names.json", HELPER, D_BUILD_RECORD)}


def validate_d_record(baseline: list[dict]) -> dict:
    raw = D_BUILD_RECORD.read_bytes()
    require(hashlib.sha256(raw).hexdigest() == D_BUILD_SHA, "original D build receipt changed")
    record = json.loads(raw)
    require(record["schema"] == "mhgp7-mono-meb-build-v1" and record["status"] == "completed"
            and record["build_exit_code"] == 0, "D build was not completed")
    require(Path(record["source_root"]).resolve() == SOURCE.resolve(), "D source root mismatch")
    require(record["sources_before"] == record["sources_after"], "D source changed during initial build")
    require(Path(record["binary"]["path"]).resolve() == (BUILD / "mhgp7").resolve()
            and record["binary"]["sha256"] == EXPECTED_D
            and record["binary"]["bytes"] == (BUILD / "mhgp7").stat().st_size, "D binary binding mismatch")
    for key, name in (("compile_database", "compile_commands.json"), ("cmake_cache", "CMakeCache.txt")):
        require(Path(record[key]["path"]).resolve() == (BUILD / name).resolve()
                and H.digest(BUILD / name) == record[key]["sha256"], "initial D metadata binding mismatch")
    require(record["compile_command"] == binary_binding()["product_command"], "initial D compile command mismatch")
    actual = {str(Path(row["path"]).relative_to("morsehgp3D_v7")): row["sha256"] for row in baseline}
    engine = {name: pin for name, pin in actual.items()
              if name == "CMakeLists.txt" or name.split("/", 1)[0] in ("src", "cli", "oracle")}
    require(engine == record["sources_after"], "D engine source inventory changed")
    for name, pin in record["sources_after"].items():
        require(actual.get(name) == pin, "D build source does not match current qualification source: " + name)
    return record


def binary_binding() -> dict:
    records = json.loads((BUILD / "compile_commands.json").read_text())
    product = [record for record in records if "CMakeFiles/mhgp7.dir/" in record.get("command", "")]
    require(len(product) == 1, "product compilation binding ambiguous")
    command = shlex.split(product[0]["command"])
    require(all(flag in command for flag in ("-std=c++20", "-O3", "-DNDEBUG")),
            "product is not C++20 Release")
    require(not any(flag in " ".join(command) for flag in
                    ("MHGP7_TESTING", "MHGP7_PROFILE", "-ffast-math", "-Ofast", "-fsanitize")),
            "product flags unsafe or instrumented")
    require(Path(product[0]["file"]).resolve() == SOURCE / "cli/mhgp7.cpp", "product source mismatch")
    require(Path(product[0]["directory"]).resolve() == BUILD.resolve(), "product build directory mismatch")
    cache = (BUILD / "CMakeCache.txt").read_text().splitlines()
    require("CMAKE_BUILD_TYPE:STRING=Release" in cache
            and "CMAKE_HOME_DIRECTORY:INTERNAL=" + str(SOURCE.resolve()) in cache
            and "MHGP7_ENABLE_CUDA:BOOL=OFF" in cache, "product CMake cache mismatch")
    require((BUILD / "mhgp7").is_file() and os.access(BUILD / "mhgp7", os.X_OK)
            and H.digest(BUILD / "mhgp7") == EXPECTED_D, "D product binary changed")
    return {"build_kind": "incremental_existing_D_release_tree", "product_command": product[0],
            "product_path": str(BUILD / "mhgp7"), "product_sha256": EXPECTED_D,
            "configuration": {str(path.relative_to(ROOT)): H.digest(path) for path in
                              (BUILD / "CMakeCache.txt", BUILD / "CTestTestfile.cmake", BUILD / "compile_commands.json")},
            "qualification": "fresh test executions; build is explicitly not fresh or hermetic"}


def execute() -> int:
    require(BUILD.is_dir(), "explicitly incremental build is absent")
    require(not OUT.exists(), "refuse overwriting any prior receipt attempt")
    require(not any(value for key, value in os.environ.items() if key.startswith("LD_")),
            "nonempty LD_* environment is forbidden; values intentionally not printed")
    expected = expected_names()
    baseline = H.source_inventory()
    protected = {str(path): H.digest(path) for path in PROTECTED}
    require(protected == {str(path): value for path, value in PROTECTED.items()}, "protected C/D before hash mismatch")
    binding_before = binary_binding()
    d_record = validate_d_record(baseline)
    pinned_inputs = input_inventory()
    OUT.mkdir(mode=0o700)
    H.save("sources_before.json", baseline)
    H.save("protected_before.json", protected)
    H.save("build_binding_before.json", binding_before)
    H.save("build_D_record.json", d_record)
    H.save("runner_inputs_before.json", pinned_inputs)
    H.save("environment.json", {"public_status": "not_claimed", "build_kind": "incremental",
           "all_gate_executions": "fresh", "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
           "expected_C_sha256": EXPECTED_C, "expected_D_sha256": EXPECTED_D, "gcp": "not_used", "helper_sha256": H.digest(HELPER),
           "runner_sha256": H.digest(Path(__file__)), "head": subprocess.check_output(
               ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
           "worktree": subprocess.check_output(["git", "status", "--porcelain=v1"], cwd=ROOT, text=True),
           "uname": list(os.uname()), "affinity_cpus": sorted(os.sched_getaffinity(0)),
           "expected_names_sha256": H.digest(RECEIPT / "expected_test_names.json")})
    results, failure, inventory, tested, binding = {}, None, None, None, None
    try:
        for label, command, limit in commands():
            require(input_inventory() == pinned_inputs, "runner/authority input drift before " + label)
            require(H.source_inventory() == baseline, "source changed before " + label)
            require(all(H.digest(Path(path)) == value for path, value in protected.items()),
                    "protected CLI changed before " + label)
            results[label] = H.run(label, command, limit)
            require(results[label]["exit_code"] == 0, label + " failed or censored")
            H.save("sources_after_" + label + ".json", H.source_inventory())
            if label == "build_incremental":
                tested, binding = H.binaries(), binary_binding()
                H.save("binaries_tested.json", tested)
                H.save("build_binding.json", binding)
            if label == "inventory":
                inventory = json.loads((OUT / "inventory.stdout").read_text())
                names = [test["name"] for test in inventory.get("tests", [])]
                require(len(names) == 323 and len(set(names)) == 323 and sorted(names) == expected,
                        "actual CMake inventory differs from the 323 required names")
    except BaseException as error:
        failure = f"{type(error).__name__}: {error}"
        print("FAILURE " + failure, flush=True)
    finally:
        final_errors = []

        def observe(call):
            try:
                return call()
            except Exception as error:
                final_errors.append(f"{type(error).__name__}: {error}")
                return None

        after = observe(H.source_inventory)
        binaries_after = observe(H.binaries)
        protected_after = observe(lambda: {str(path): H.digest(path) for path in PROTECTED})
        inputs_after = observe(input_inventory)
        binding_after = observe(binary_binding)
        junit = observe(lambda: judge_junit(ET.parse(OUT / "ctest.junit.xml").getroot(), expected))
        passed = (failure is None and not final_errors and set(results) == {label for label, _, _ in commands()}
                  and all(record["exit_code"] == 0 for record in results.values())
                  and after == baseline and protected_after == protected and tested is not None
                  and binaries_after == tested and binding is not None and binding_after == binding
                  and inputs_after == pinned_inputs and inventory is not None and junit is not None)
        H.save("sources_after.json", after)
        H.save("binaries_after.json", binaries_after)
        H.save("protected_after.json", protected_after)
        H.save("runner_inputs_after.json", inputs_after)
        H.save("build_binding_after.json", binding_after)
        summary = {"status": "passed" if passed else "failed_or_invalid", "failure": failure,
                   "final_observation_errors": final_errors, "public_status": "not_claimed", "gcp": "not_used",
                   "build_kind": "incremental_existing_D_release_tree", "gate_results_reused": False,
                   "source_stable": after == baseline, "protected_cli_unchanged": protected_after == protected,
                   "tested_binaries_stable": tested is not None and binaries_after == tested,
                   "runner_inputs_stable": inputs_after == pinned_inputs,
                   "build_binding_stable": binding is not None and binding_after == binding,
                   "expected_C_sha256": EXPECTED_C, "expected_D_sha256": EXPECTED_D,
                   "junit": junit, "results": results}
        H.save("summary.json", summary)
        H.save("receipt_manifest.json", [{"path": path.name, "sha256": H.digest(path), "size": path.stat().st_size}
               for path in sorted(OUT.iterdir()) if path.is_file() and path.name != "receipt_manifest.json"])
        print(json.dumps({key: value for key, value in summary.items() if key != "results"}), flush=True)
    return 0 if passed else 1


def selftest() -> int:
    require(len(expected_names()) == 323, "expected inventory nonvacuity")
    expected = ["a", "b"]
    nominal = ET.fromstring('<testsuite><testcase name="a" status="run"/><testcase name="b" status="run"/></testsuite>')
    require(judge_junit(nominal, expected)["tests"] == 2, "positive judge fixture")
    cases = [
        '<testsuite/>',
        '<testsuite><testcase name="a" status="run"/></testsuite>',
        '<testsuite><testcase name="a" status="run"/><testcase name="a" status="run"/></testsuite>',
        '<testsuite><testcase name="a" status="run"/><testcase name="c" status="run"/></testsuite>',
        '<testsuite><testcase name="a" status="run"><failure/></testcase><testcase name="b" status="run"/></testsuite>',
        '<testsuite><testcase name="a" status="run"><error/></testcase><testcase name="b" status="run"/></testsuite>',
        '<testsuite><testcase name="a" status="run"><skipped/></testcase><testcase name="b" status="run"/></testsuite>',
        '<testsuite><testcase name="a" status="notrun"/><testcase name="b" status="run"/></testsuite>',
    ]
    rejected = 0
    for xml in cases:
        try:
            judge_junit(ET.fromstring(xml), expected)
        except RuntimeError:
            rejected += 1
    require(rejected == len(cases), "negative judge fixture survived")
    print(f"full_release_junit_selftest positive=1 rejected={rejected} required_names=323 PASS")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--execute", action="store_true")
    group.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    if args.selftest:
        return selftest()
    if not args.execute:
        print(json.dumps({"status": "prepared_not_executed", "commands": commands(), "expected_tests": 323,
                          "build_kind": "incremental_existing_D_release_tree",
                          "expected_D_sha256": EXPECTED_D, "execution_requires_root_GO": True}))
        return 0
    return execute()


if __name__ == "__main__":
    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, lambda signum, _frame: (_ for _ in ()).throw(InterruptedError(f"signal {signum}")))
    raise SystemExit(main())
