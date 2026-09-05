#!/usr/bin/env python3
"""Fresh bounded qualification of the integrated lazy-MEB delta; no cloud."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import xml.etree.ElementTree as ET

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
SOURCE = ROOT / "morsehgp3D_v7"
HELPER = SOURCE / "receipts/release_20260904/run_release.py"
HELPER_SHA = "582922e64fcd2927be3e63c7156fefa839fd1f8c055f92a75c97514cad7a0ee5"
EXPECTED_C = "25c9bf8e4ef3cded5647a22f16d81af7a1e778196ad3bff73884a7f58da985f2"
PROTECTED = [ROOT / "build/v7/mhgp7", ROOT / "build/v7_c_qualification/mhgp7"]
TARGETS = ["mhgp7_meb_lazy_gate", "mhgp7_silent_incidence_gate", "mhgp7",
           "mhgp7_archive_api_gate", "mhgp7_archive_cleanup_gate", "mhgp7_mono_inline_gate",
           "mhgp7_census_direct_gate", "mhgp7_bad_alloc_gate", "mhgp7_thread_failure_gate"]
NAMES = sorted([
    "mhgp7_meb_lazy", "mhgp7_meb_lazy_q3_reject_shell", "mhgp7_meb_lazy_q4_reject_shell",
    "mhgp7_meb_lazy_eager_materialization", "mhgp7_meb_lazy_unknown_mutant",
    "mhgp7_meb_lazy_empty_mutant", "mhgp7_meb_lazy_duplicate_mutant",
    "mhgp7_silent_incidence", "mhgp7_silent_incidence_mutant", "mhgp7_reduced_latent_parent_mutant",
    "mhgp7_reduced_drop_materialization_mutant", "mhgp7_silent_drop_nonmerge_mutant",
    "mhgp7_silent_csr_stale_level_mutant", "mhgp7_archive", "mhgp7_archive_optimized",
    "mhgp7_archive_api", "mhgp7_archive_cleanup", "mhgp7_mono_inline",
    "mhgp7_mono_inline_late_a", "mhgp7_mono_inline_late_b", "mhgp7_mono_inline_early_alloc",
    "mhgp7_census_direct", "mhgp7_census_direct_offset", "mhgp7_census_direct_transaction",
    "mhgp7_census_direct_residence", "mhgp7_bad_alloc_temoin", "mhgp7_bad_alloc_etage_provision",
    "mhgp7_bad_alloc_etage_census", "mhgp7_bad_alloc_etage_fold", "mhgp7_thread_failure",
    "mhgp7_thread_failure_mutant_admission", "mhgp7_thread_failure_refus_arg",
])
REGEX = "^(" + "|".join(re.escape(name) for name in NAMES) + ")$"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def helper():
    data = HELPER.read_bytes()
    require(hashlib.sha256(data).hexdigest() == HELPER_SHA, "helper source drift")
    spec = importlib.util.spec_from_file_location("meb_targeted_helper", HELPER)
    require(spec is not None, "helper spec")
    module = importlib.util.module_from_spec(spec)
    exec(compile(data, str(HELPER), "exec"), module.__dict__)
    return module


def judge_junit(root: ET.Element) -> dict:
    cases = list(root.iter("testcase"))
    names = [case.get("name") for case in cases]
    require(len(names) == 32 and len(set(names)) == 32 and sorted(names) == NAMES,
            "JUnit wrong/missing/repeated names")
    require(all(case.get("status") == "run" and
                not any(case.find(tag) is not None for tag in ("failure", "error", "skipped"))
                for case in cases), "JUnit failed, skipped or not run")
    return {"tests": 32, "names": NAMES, "failed": [], "skipped": []}


def selftest() -> int:
    require(len(NAMES) == 32 and len(set(NAMES)) == 32, "test inventory")
    require(all(re.fullmatch(REGEX, name) for name in NAMES), "regex misses expected test")
    require(not re.fullmatch(REGEX, "mhgp7_archive_suffix"), "regex is not anchored")
    nominal = ET.Element("testsuite")
    for name in NAMES:
        ET.SubElement(nominal, "testcase", name=name, status="run")
    judge_junit(nominal)
    killed = 0
    for kind in ("empty", "missing", "duplicate", "unknown", "failure", "error", "skipped", "notrun"):
        doc = ET.fromstring(ET.tostring(nominal))
        if kind == "empty":
            doc.clear()
        elif kind == "missing":
            doc.remove(doc[-1])
        elif kind == "duplicate":
            doc[-1].set("name", doc[0].get("name", ""))
        elif kind == "unknown":
            doc[-1].set("name", "unexpected")
        elif kind == "notrun":
            doc[-1].set("status", "notrun")
        else:
            ET.SubElement(doc[-1], kind)
        try:
            judge_junit(doc)
        except RuntimeError:
            killed += 1
        else:
            raise RuntimeError("JUnit mutant survived: " + kind)
    require(killed == 8, "judge nonvacuity")
    print("meb_targeted_judge=passed positive=1 rejected=8 names=32")
    return 0


def execute(mode: str) -> int:
    h = helper()
    build = BASE / mode
    out = BASE / (mode + "_receipts")
    require(not build.exists() and not out.exists(), "fresh build/receipt already exists")
    h.BUILD, h.OUT = build, out
    before = h.source_inventory()
    protected = {str(p): h.digest(p) for p in PROTECTED}
    require(all(digest == EXPECTED_C for digest in protected.values()), "C binary before pin")
    out.mkdir(mode=0o700)
    h.save("sources_before.json", before)
    h.save("protected_before.json", protected)
    h.save("expected_names.json", NAMES)
    h.save("environment.json", {"schema": "mhgp7.integrated-meb-targeted.v1", "mode": mode,
           "public_status": "not_claimed", "gcp": "not_used", "build_kind": "fresh_isolated",
           "runner_sha256": h.digest(Path(__file__)), "helper_sha256": HELPER_SHA,
           "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
           "worktree": subprocess.check_output(["git", "status", "--porcelain=v1"], cwd=ROOT, text=True),
           "affinity_cpus": sorted(os.sched_getaffinity(0)), "uname": list(os.uname())})
    flags = (["-DCMAKE_CXX_FLAGS_RELEASE=-O1 -g -DNDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer",
              "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined"] if mode == "sanitized" else [])
    commands = [
        ("configure", ["cmake", "-S", str(SOURCE), "-B", str(build), "-DCMAKE_BUILD_TYPE=Release",
                       "-DMHGP7_ENABLE_CUDA=OFF", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", *flags], 120),
        ("build_targets", ["cmake", "--build", str(build), "--parallel", "2", "--target", *TARGETS], 1200),
        ("inventory", ["ctest", "--test-dir", str(build), "--show-only=json-v1", "-R", REGEX], 60),
        ("ctest", ["ctest", "--test-dir", str(build), "--output-on-failure", "--no-tests=error",
                   "-R", REGEX, "--parallel", "2", "--output-junit", str(out / "ctest.junit.xml")], 1200),
    ]
    results, failure, tested, junit = {}, None, None, None
    try:
        for label, argv, seconds in commands:
            require(h.source_inventory() == before, "source drift before " + label)
            require(all(h.digest(Path(p)) == value for p, value in protected.items()), "protected C drift")
            results[label] = h.run(label, argv, seconds)
            require(results[label]["exit_code"] == 0, label + " failed/censored")
            h.save("sources_after_" + label + ".json", h.source_inventory())
            if label == "build_targets":
                tested = h.binaries()
                require(sorted(Path(entry["path"]).name for entry in tested) == sorted(TARGETS),
                        "built binary inventory mismatch")
                h.save("binaries_tested.json", tested)
                config = {str(path.relative_to(ROOT)): h.digest(path) for path in (
                    build / "CMakeCache.txt", build / "CTestTestfile.cmake", build / "compile_commands.json")}
                h.save("configuration.json", config)
                records = json.loads((build / "compile_commands.json").read_text())
                chosen = [r for r in records if any("CMakeFiles/" + t + ".dir/" in r["command"] for t in TARGETS)]
                require(len(chosen) == len(TARGETS), "compile binding missing or duplicate")
                for record in chosen:
                    command = record["command"]
                    require("-std=c++20" in command and "-DNDEBUG" in command, "not strict C++20 Release")
                    require(not any(flag in command for flag in ("-ffast-math", "-Ofast")), "unsafe compiler flags")
                    require(("-fsanitize=address,undefined" in command) == (mode == "sanitized"), "sanitizer mode mismatch")
                    product = "CMakeFiles/mhgp7.dir/" in command
                    require(("MHGP7_TESTING" not in command) == product, "test/product define mismatch")
                    require("-O1" in command if mode == "sanitized" else "-O3" in command, "optimization mode")
                    require(Path(record["file"]).resolve().is_relative_to(SOURCE), "compile outside real source")
                h.save("compile_binding.json", chosen)
            elif label == "inventory":
                inventory = json.loads((out / "inventory.stdout").read_text())
                names = [test["name"] for test in inventory.get("tests", [])]
                require(len(names) == 32 and len(set(names)) == 32 and sorted(names) == NAMES,
                        "CTest registration mismatch")
                require(tested == h.binaries(), "binary drift before tests")
            elif label == "ctest":
                junit = judge_junit(ET.parse(out / "ctest.junit.xml").getroot())
                log = build / "Testing/Temporary/LastTest.log"
                with (out / "LastTest.log").open("xb") as stream:
                    stream.write(log.read_bytes())
    except BaseException as error:
        failure = type(error).__name__ + ": " + str(error)
        print("FAILURE " + failure, flush=True)
    after = h.source_inventory()
    final_binaries = h.binaries()
    final_protected = {str(p): h.digest(p) for p in PROTECTED}
    h.save("sources_after.json", after)
    h.save("binaries_after.json", final_binaries)
    h.save("protected_after.json", final_protected)
    passed = (failure is None and set(results) == {name for name, _, _ in commands}
              and all(r["exit_code"] == 0 for r in results.values()) and junit is not None
              and after == before and tested == final_binaries and protected == final_protected)
    summary = {"status": "passed" if passed else "invalid_or_failed", "failure": failure,
               "junit": junit, "source_stable": before == after, "binary_stable": tested == final_binaries,
               "C_unchanged": protected == final_protected, "results": results,
               "public_status": "not_claimed", "gcp": "not_used"}
    h.save("summary.json", summary)
    h.save("receipt_manifest.json", [{"path": p.name, "sha256": h.digest(p), "size": p.stat().st_size}
                                    for p in sorted(out.iterdir()) if p.is_file()])
    print(json.dumps({k: v for k, v in summary.items() if k != "results"}, sort_keys=True), flush=True)
    return 0 if passed else 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--mode", choices=("release", "sanitized"))
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    if args.selftest == (args.mode is not None):
        parser.error("choose exactly one of --mode and --selftest")
    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, lambda signum, _frame: (_ for _ in ()).throw(InterruptedError(f"signal {signum}")))
    raise SystemExit(selftest() if args.selftest else execute(args.mode))
