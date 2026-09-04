#!/usr/bin/env python3
"""Fresh execution of all 316 gates after an explicitly incremental Release build.

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
import signal
import subprocess
import sys
import xml.etree.ElementTree as ET

RECEIPT = Path(__file__).resolve().parent
ROOT = RECEIPT.parents[2]
SOURCE = ROOT / "morsehgp3D_v7"
BUILD = ROOT / "build/v7_c_qualification"
OUT = RECEIPT / "full_release"
HELPER = SOURCE / "receipts/release_20260904/run_release.py"
EXPECTED_C = "25c9bf8e4ef3cded5647a22f16d81af7a1e778196ad3bff73884a7f58da985f2"
HELPER_SHA = "582922e64fcd2927be3e63c7156fefa839fd1f8c055f92a75c97514cad7a0ee5"
PROTECTED = (ROOT / "build/v7/mhgp7", BUILD / "mhgp7")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def load_helper():
    source_bytes = HELPER.read_bytes()
    require(hashlib.sha256(source_bytes).hexdigest() == HELPER_SHA, "helper bytes changed")
    spec = importlib.util.spec_from_file_location("arithmetic_full_release_helper", HELPER)
    require(spec is not None and spec.loader is not None, "helper unavailable")
    helper = importlib.util.module_from_spec(spec)
    exec(compile(source_bytes, str(HELPER), "exec"), helper.__dict__)
    helper.OUT, helper.BUILD = OUT, BUILD
    return helper


H = load_helper()


def expected_names() -> list[str]:
    names = json.loads((RECEIPT / "expected_test_names.json").read_text())
    require(isinstance(names, list) and len(names) == 316 and len(set(names)) == 316
            and all(isinstance(name, str) for name in names), "expected name inventory malformed")
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


def binary_binding() -> dict:
    records = json.loads((BUILD / "compile_commands.json").read_text())
    product = [record for record in records if "CMakeFiles/mhgp7.dir/" in record.get("command", "")]
    require(len(product) == 1, "product compilation binding ambiguous")
    command = product[0]["command"]
    require("-std=c++20" in command and "-O3" in command and "-DNDEBUG" in command,
            "product is not C++20 Release")
    require(not any(flag in command for flag in ("MHGP7_TESTING", "MHGP7_PROFILE", "-ffast-math", "-Ofast", "-fsanitize")),
            "product flags unsafe or instrumented")
    require(Path(product[0]["file"]).resolve() == SOURCE / "cli/mhgp7.cpp", "product source mismatch")
    return {"build_kind": "incremental_existing_release_tree", "product_command": product[0],
            "configuration": {str(path.relative_to(ROOT)): H.digest(path) for path in
                              (BUILD / "CMakeCache.txt", BUILD / "CTestTestfile.cmake", BUILD / "compile_commands.json")},
            "qualification": "fresh test executions; build is explicitly not fresh or hermetic"}


def execute() -> int:
    require(BUILD.is_dir(), "explicitly incremental build is absent")
    require(not OUT.exists(), "refuse overwriting any prior receipt attempt")
    expected = expected_names()
    baseline = H.source_inventory()
    protected = {str(path): H.digest(path) for path in PROTECTED}
    require(all(value == EXPECTED_C for value in protected.values()), "CLI C before hash mismatch")
    OUT.mkdir(mode=0o700)
    H.save("sources_before.json", baseline)
    H.save("protected_before.json", protected)
    H.save("environment.json", {"public_status": "not_claimed", "build_kind": "incremental",
           "all_gate_executions": "fresh", "gcp": "not_used", "helper_sha256": H.digest(HELPER),
           "runner_sha256": H.digest(Path(__file__)), "head": subprocess.check_output(
               ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
           "worktree": subprocess.check_output(["git", "status", "--porcelain=v1"], cwd=ROOT, text=True),
           "uname": list(os.uname()), "affinity_cpus": sorted(os.sched_getaffinity(0)),
           "expected_names_sha256": H.digest(RECEIPT / "expected_test_names.json")})
    results, failure, inventory, tested, binding = {}, None, None, None, None
    try:
        for label, command, limit in commands():
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
                require(len(names) == 316 and len(set(names)) == 316 and sorted(names) == expected,
                        "actual CMake inventory differs from the 316 required names")
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
        junit = observe(lambda: judge_junit(ET.parse(OUT / "ctest.junit.xml").getroot(), expected))
        passed = (failure is None and not final_errors and set(results) == {label for label, _, _ in commands()}
                  and all(record["exit_code"] == 0 for record in results.values())
                  and after == baseline and protected_after == protected and tested is not None
                  and binaries_after == tested and binding is not None and inventory is not None and junit is not None)
        H.save("sources_after.json", after)
        H.save("binaries_after.json", binaries_after)
        H.save("protected_after.json", protected_after)
        summary = {"status": "passed" if passed else "failed_or_invalid", "failure": failure,
                   "final_observation_errors": final_errors, "public_status": "not_claimed", "gcp": "not_used",
                   "build_kind": "incremental_existing_release_tree", "gate_results_reused": False,
                   "source_stable": after == baseline, "protected_cli_unchanged": protected_after == protected,
                   "tested_binaries_stable": tested is not None and binaries_after == tested,
                   "junit": junit, "results": results}
        H.save("summary.json", summary)
        H.save("receipt_manifest.json", [{"path": path.name, "sha256": H.digest(path), "size": path.stat().st_size}
               for path in sorted(OUT.iterdir()) if path.is_file() and path.name != "receipt_manifest.json"])
        print(json.dumps({key: value for key, value in summary.items() if key != "results"}), flush=True)
    return 0 if passed else 1


def selftest() -> int:
    expected = ["a", "b"]
    nominal = ET.fromstring('<testsuite><testcase name="a" status="run"/><testcase name="b" status="run"/></testsuite>')
    require(judge_junit(nominal, expected)["tests"] == 2, "positive judge fixture")
    cases = [
        '<testsuite/>',
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
    print(f"full_release_junit_selftest positive=1 rejected={rejected} PASS")
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
        print(json.dumps({"status": "prepared_not_executed", "commands": commands(), "expected_tests": 316}))
        return 0
    return execute()


if __name__ == "__main__":
    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, lambda signum, _frame: (_ for _ in ()).throw(InterruptedError(f"signal {signum}")))
    raise SystemExit(main())
