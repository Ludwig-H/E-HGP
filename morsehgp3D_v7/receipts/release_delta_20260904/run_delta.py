#!/usr/bin/env python3
"""Qualify only the archive/interface delta; retain the sealed Release A proof."""
from __future__ import annotations

import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import signal
import sys
import xml.etree.ElementTree as ET


OUT = Path(__file__).resolve().parent
A = OUT.with_name("release_20260904")
SPEC = importlib.util.spec_from_file_location("release_a_helpers", A / "run_release.py")
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("sealed Release A helper unavailable")
H = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(H)
H.OUT = OUT
ROOT, SOURCE, BUILD = H.ROOT, H.SOURCE, H.BUILD
OLD_TESTS = {
    "mhgp7_cli_refus_s7", "mhgp7_cli_refus_s_suffix", "mhgp7_cli_refus_n_suffix",
    "mhgp7_cli_ok_s8", "mhgp7_cli_budget_signature", "mhgp7_cli_layout_classic_signature",
    "mhgp7_cli_layout_csr_signature", "mhgp7_cli_refus_layout_inconnu", "mhgp7_cli_refus_layout_vide",
    "mhgp7_archive", "mhgp7_archive_optimized", "mhgp7_archive_api",
}
NEW_TESTS = {"mhgp7_archive_cleanup", "mhgp7_incidence_campaign", "mhgp7_incidence_campaign_optimized"}
REBUILT = {"build/v7/mhgp7", "build/v7/mhgp7_archive_api_gate"}
CHANGED = {"morsehgp3D_v7/CMakeLists.txt", "morsehgp3D_v7/src/io/archive.hpp",
           "morsehgp3D_v7/bench/incidence_campaign.py", "morsehgp3D_v7/tests/incidence_campaign_gate.py"}
ADDED = {"morsehgp3D_v7/tests/archive_cleanup_gate.cpp"}


def read(path: Path) -> object:
    return json.loads(path.read_text())


def keyed(records: list[dict]) -> dict[str, dict]:
    return {record["path"]: record for record in records}


def test_contract(test: dict) -> dict:
    # CTest's numeric backtrace indices are not execution semantics.
    return {key: test[key] for key in ("name", "command", "properties")}


def dependency_proof(old_sources: dict, now_sources: dict, unchanged: list[str]) -> list[dict]:
    result = []
    for binary in unchanged:
        target = Path(binary).name
        depfiles = sorted((BUILD / "CMakeFiles" / (target + ".dir")).rglob("*.o.d"))
        if not depfiles:
            raise RuntimeError("no compiler dependency evidence for " + target)
        own = set()
        for depfile in depfiles:
            for token in depfile.read_text().replace("\\\n", " ").split():
                if token.startswith(str(SOURCE) + "/"):
                    own.add(str(Path(token).resolve().relative_to(ROOT)))
        if not own:
            raise RuntimeError("no v7 source dependency for " + target)
        for path in own:
            if path not in old_sources or path not in now_sources or old_sources[path] != now_sources[path]:
                raise RuntimeError("reused binary depends on a changed source: " + target + ":" + path)
        result.append({"binary": binary, "v7_dependencies_unchanged": sorted(own),
                       "compiler_depfiles": [{"path": str(p.relative_to(ROOT)), "sha256": H.digest(p)} for p in depfiles]})
    return result


def main() -> int:
    if (OUT / "sources_before.json").exists():
        raise RuntimeError("delta receipt already attempted; refuse overwrite")
    if read(A / "summary.json")["status"] != "passed":
        raise RuntimeError("Release A was not passed")
    for record in read(A / "receipt_manifest.json"):
        path = A / record["path"]
        if path.stat().st_size != record["size"] or H.digest(path) != record["sha256"]:
            raise RuntimeError("Release A seal mismatch: " + record["path"])
    baseline = H.source_inventory()
    old_sources = keyed(read(A / "sources_after.json"))
    current = keyed(baseline)
    changes = sorted(path for path in old_sources.keys() & current.keys() if old_sources[path] != current[path])
    additions = sorted(current.keys() - old_sources.keys())
    removed = sorted(old_sources.keys() - current.keys())
    if set(changes) != CHANGED or set(additions) != ADDED or removed:
        raise RuntimeError("source delta differs from the coordinated four changes and one addition: " +
                           repr((changes, additions, removed)))
    old_binaries = keyed(read(A / "binaries_after.json"))
    before_binaries = keyed(H.binaries())
    if any(before_binaries.get(path) != record for path, record in old_binaries.items()):
        raise RuntimeError("a Release A binary already changed before delta build")
    H.save("sources_before.json", baseline)
    H.save("binaries_before.json", H.binaries())
    H.save("source_delta.json", {"changed": changes, "added": additions, "removed": removed})
    H.save("environment.json", {"schema": "mhgp7.local.release-delta.v1", "public_status": "not_claimed",
           "scope": "cpu_release_archive_interface_delta_not_fresh_full_suite_not_slo",
           "release_a_manifest_sha256": H.digest(A / "receipt_manifest.json"),
           "runner_sha256": H.digest(Path(__file__)), "gcp": "not_used", "uname": list(os.uname()),
           "affinity_cpus": sorted(os.sched_getaffinity(0))})
    results = {}
    failure = None
    reused_tests = []
    dependency_records = []
    tested_binaries = None
    inventory = {}
    try:
        for label, command, limit in [
            ("configure", ["cmake", "-S", str(SOURCE), "-B", str(BUILD), "-DCMAKE_BUILD_TYPE=Release"], 120),
            ("build_delta", ["cmake", "--build", str(BUILD), "--parallel", "2", "--target",
                             "mhgp7", "mhgp7_archive_api_gate", "mhgp7_archive_cleanup_gate"], 600),
            ("inventory", ["ctest", "--test-dir", str(BUILD), "--show-only=json-v1", "-L", "^gate$"], 60),
        ]:
            results[label] = H.run(label, command, limit)
            if results[label]["exit_code"]:
                raise RuntimeError(label + " failed")
        tested_binaries = H.binaries()
        current_binaries = keyed(tested_binaries)
        unchanged = sorted(old_binaries.keys() - REBUILT)
        if len(unchanged) != 29 or any(current_binaries.get(path) != old_binaries[path] for path in unchanged):
            raise RuntimeError("one of the 29 reusable binaries changed")
        if any(current_binaries[path] == old_binaries[path] for path in REBUILT):
            raise RuntimeError("expected archive consumer was not rebuilt")
        H.save("binaries_tested.json", tested_binaries)
        dependency_records = dependency_proof(old_sources, current, unchanged)
        H.save("reused_binary_dependencies.json", dependency_records)
        old_tests = {test["name"]: test for test in read(A / "inventory.stdout")["tests"]}
        inventory = read(OUT / "inventory.stdout")
        now_tests = {test["name"]: test for test in inventory["tests"]}
        if len(old_tests) != 279 or set(now_tests) - set(old_tests) != NEW_TESTS or set(old_tests) - set(now_tests):
            raise RuntimeError("CTest inventory is not the coordinated 279 plus 3")
        changed_contracts = sorted(name for name in old_tests if test_contract(old_tests[name]) != test_contract(now_tests[name]))
        if changed_contracts:
            raise RuntimeError("existing CTest execution contracts changed: " + repr(changed_contracts))
        if not OLD_TESTS <= set(old_tests):
            raise RuntimeError("an archive/interface test is absent")
        reused_tests = sorted(set(old_tests) - OLD_TESTS)
        H.save("test_reuse.json", {"release_a_executed": 279, "reused_without_rerun": reused_tests,
               "rerun_existing": sorted(OLD_TESTS), "new_tests": sorted(NEW_TESTS),
               "existing_execution_contracts_identical": True,
               "ignored_metadata_only": "CTest numeric backtrace indices and the backtraceGraph"})
        print("PRODUCT_B_SHA256=" + H.digest(BUILD / "mhgp7"), flush=True)
        regex = "^(" + "|".join(re.escape(name) for name in sorted(OLD_TESTS | NEW_TESTS)) + ")$"
        results["ctest_delta"] = H.run("ctest_delta", ["ctest", "--test-dir", str(BUILD), "--output-on-failure",
                                      "--no-tests=error", "-L", "^gate$", "-R", regex, "--parallel", "2",
                                      "--output-junit", str(OUT / "ctest_delta.junit.xml")], 1800)
    except BaseException as error:
        failure = f"{type(error).__name__}: {error}"
        print("FAILURE " + failure, flush=True)
    finally:
        after = H.source_inventory()
        final_binaries = H.binaries()
        H.save("sources_after.json", after)
        H.save("binaries_after.json", final_binaries)
        junit = None
        if (OUT / "ctest_delta.junit.xml").exists():
            cases = ET.parse(OUT / "ctest_delta.junit.xml").getroot().findall(".//testcase")
            junit = {"tests": len(cases), "names": sorted(case.get("name") for case in cases),
                     "failed": [case.get("name") for case in cases if case.find("failure") is not None or case.find("error") is not None],
                     "skipped": [case.get("name") for case in cases if case.find("skipped") is not None]}
        passed = not failure and set(results) == {"configure", "build_delta", "inventory", "ctest_delta"} and \
            all(record["exit_code"] == 0 for record in results.values()) and baseline == after and \
            tested_binaries == final_binaries and junit is not None and junit["tests"] == 15 and \
            set(junit["names"]) == OLD_TESTS | NEW_TESTS and not junit["failed"] and not junit["skipped"]
        summary = {"status": "passed" if passed else "invalid_or_failed", "failure": failure,
                   "public_status": "not_claimed", "source_stable": baseline == after,
                   "tested_binaries_stable": tested_binaries == final_binaries,
                   "release_a_tests_executed_on_snapshot_a": 279,
                   "snapshot_b_reused_tests_without_rerun": len(reused_tests),
                   "snapshot_b_reused_binary_count": len(dependency_records),
                   "snapshot_b_fresh_ctest": junit, "ctest_inventory_count": len(inventory.get("tests", [])),
                   "claim": "selective delta qualification with byte-identical unaffected consumers; not a fresh full suite",
                   "product_a_sha256": old_binaries["build/v7/mhgp7"]["sha256"],
                   "product_b_sha256": H.digest(BUILD / "mhgp7"), "gcp": "not_used", "results": results}
        H.save("summary.json", summary)
        H.save("receipt_manifest.json", [{"path": path.name, "sha256": H.digest(path), "size": path.stat().st_size}
               for path in sorted(OUT.iterdir()) if path.is_file() and path.name != "receipt_manifest.json"])
        print(json.dumps({k: v for k, v in summary.items() if k != "results"}, sort_keys=True), flush=True)
    return 0 if passed else 1


if __name__ == "__main__":
    for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, lambda signum, _frame: (_ for _ in ()).throw(InterruptedError(f"signal {signum}")))
    raise SystemExit(main())
