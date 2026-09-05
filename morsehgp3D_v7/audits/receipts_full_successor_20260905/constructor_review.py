"""Read-only review of the constructor's successor qualification captures.

No build, engine, CTest, import of the controller, or external write is made.
The failed first attempt stays a separate failure; only R2 is qualified.
"""

from __future__ import annotations

import hashlib
import json
import math
from pathlib import Path
import re
import shlex
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[3]
PACKET = ROOT / "morsehgp3D_v7/receipts/full_gabriel_successor_20260905"
SOURCE = "85c27ab91d7f159520a8db3098629447b0a213a134c5c042a86c585416847fad"
RECEIPT = "49be3d72045f9b3a2306a69230d114e6bac96861f6029e6e13c495091f61e72d"
SOURCE_MAP = "8c977bc529b5d354c1a69b50dc51d1a27b6cf04ec7901a75572367f27466d20e"
SEAL = "0e6c84ba2eb981a57b23bad34ed6bdb668308c4d450d4bff1388b30a90feb847"
FAILED = "70714475a7f642302a0c80efabd9062cb563a7dc1b541fee0c109d2b40f477cd"
OLD_GATE = "408532e71878b3d7227d8208ed23f6eac994561e3e4ee79924be03152ee7c97f"
NEW_GATE = "68815ac26e6b8fd4bc65a3cbe470bac00bdc71de680244cfeb1385a26ef8cfc1"
LABELS = r"^(full_certificate|full_gabriel|full_gabriel_lazy|full_gabriel_digest|full_gabriel_singleton|full_gabriel_successor)$"
TESTING_TARGETS = {
    "mhgp7_full_gabriel_singleton_gate", "mhgp7_full_gabriel_successor_gate"
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def unique(pairs: list[tuple[str, object]]) -> dict:
    result = {}
    for key, value in pairs:
        require(key not in result, "duplicate JSON key")
        result[key] = value
    return result


def read(name: str) -> dict:
    return json.loads((PACKET / name).read_text(), object_pairs_hook=unique)


def artifact_closure(prefix: str, receipt: dict, expected: int) -> None:
    require(len(receipt["artifacts"]) == expected, prefix + " artifact floor")
    for relative, digest in receipt["artifacts"].items():
        path = PACKET / prefix / relative
        require(path.resolve().is_relative_to(PACKET / prefix), "artifact escape")
        require(sha(path) == digest, prefix + "/" + relative)


def review_phase(mode: str, receipt: dict, sources: dict) -> dict:
    prefix = f"qualification/{mode}/"
    summary = read(prefix + "summary.json")
    require(summary["status"] == "completed" and not summary["errors"], "phase completion")
    require(all(summary[name] is True for name in (
        "sources_stable", "binaries_stable", "compile_binding_stable", "toolchain_stable"
    )), "phase stability")
    require([c["label"] for c in summary["commands"]] ==
            ["configure", "build", "inventory", "ctest"], "fresh command sequence")
    environment = read(prefix + "environment.json")
    expected_environment = {
        "LANG": "C", "LC_ALL": "C", "PATH": "/usr/bin:/bin",
        "PYTHONDONTWRITEBYTECODE": "1",
    }
    if mode == "san":
        expected_environment.update(
            ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
            UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1",
        )
    require(environment == expected_environment, "sanitizer environment")
    for command in summary["commands"]:
        require(command == read(prefix + command["label"] + ".command.json"),
                "command summary binding")
        require(command["exit_code"] == command["expected_code"] == 0
                and command["status"] == "completed" and command["error"] is None,
                mode + " command completion")
        require(command["environment"] == environment, "command environment")
        for stream, identity in command["raw"].items():
            path = PACKET / prefix / (command["label"] + "." + stream)
            require(sha(path) == identity["sha256"] and
                    path.stat().st_size == identity["bytes"], "command raw binding")
    command = summary["commands"][-1]
    require(command["argv"][:4] == ["/usr/bin/taskset", "-c", "6", "/usr/bin/ctest"],
            "CTest CPU binding")
    require(command["argv"][command["argv"].index("-L") + 1] == LABELS
            and "-j1" in command["argv"] and "--no-tests=error" in command["argv"],
            "CTest filter")
    for moment in ("before", "after"):
        host = read(prefix + f"host_{moment}.json")
        require(host["process_status"]["TracerPid"] == "0"
                and host["permission_override"] == "none", "untraced context")
    require(read(prefix + "sources_after.json") == sources, "phase source drift")
    binaries = read(prefix + "binaries.json")
    require(binaries == read(prefix + "binaries_after.json") and
            set(binaries) == set(receipt["targets"]), "binary inventory/stability")
    binding = read(prefix + "compile_binding.json")
    require(set(binding["targets"]) == set(binaries), "compiled target inventory")
    for target, item in binding["targets"].items():
        argv = shlex.split(item["compile"]["command"])
        require(all(flag in argv for flag in (
            "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror"
        )), "compiler contract")
        macros = [word for word in argv if word.startswith("-DMHGP7_TESTING")]
        require(macros == (["-DMHGP7_TESTING=1"] if target in TESTING_TARGETS else []),
                "testing scope")
        require(("-fsanitize=address,undefined" in argv) == (mode == "san"),
                "sanitizer compile")
        for relative, digest in item["project_dependencies"].items():
            require(sources[relative] == digest, "compiled dependency binding")
    before = read(prefix + "pre_ctest_binding.json")
    require(before == {
        "source_sha256": SOURCE_MAP,
        "binary_map_sha256": sha(PACKET / prefix / "binaries.json"),
        "compile_binding_sha256": sha(PACKET / prefix / "compile_binding.json"),
    }, "pre-CTest binding")
    require(read(prefix + "compile_binding_after_sha256.json")["sha256"] ==
            before["compile_binding_sha256"], "post-CTest compilation binding")
    tests = receipt["tests"]
    rows = read(prefix + "inventory.stdout")["tests"]
    require(len(rows) == 20 and {r["name"] for r in rows} == set(tests), "CTest inventory")
    for row in rows:
        target, argument, code = tests[row["name"]]
        build = str(Path(binaries[target]["path"]).parent)
        require(row["command"] == [
            "/usr/bin/cmake", f"-DEXPECTED={code}", "-DCMD=" + binaries[target]["path"],
            "-DARGS=" + argument, "-DEXPECT_LINE=", "-DEXPECT_PREFIX=", "-P",
            str(ROOT / "morsehgp3D_v7/cmake/run_expect.cmake"),
        ], "exact test argument/code/binary")
        props = unique([(p["name"], p["value"]) for p in row["properties"]])
        require(props.get("TIMEOUT") == 60 and props.get("WORKING_DIRECTORY") == build
                and any(re.fullmatch(LABELS, value) for value in props["LABELS"]),
                "test timeout/label binding")
        require(not any(name in props for name in (
            "DISABLED", "WILL_FAIL", "PASS_REGULAR_EXPRESSION", "SKIP_RETURN_CODE",
            "ENVIRONMENT", "ENVIRONMENT_MODIFICATION", "FIXTURES_REQUIRED", "DEPENDS"
        )), "test mutation property")
    junit = ET.fromstring((PACKET / prefix / "ctest.junit.xml").read_bytes())
    require(junit.tag == "testsuite" and junit.get("tests") == "20" and
            all(junit.get(key) == "0" for key in ("failures", "disabled", "skipped")),
            "JUnit aggregate")
    cases = junit.findall("testcase")
    require(len(cases) == 20 and {c.get("name") for c in cases} == set(tests), "JUnit names")
    for case in cases:
        elapsed = float(case.get("time", "nan"))
        require(case.get("status") == "run" and math.isfinite(elapsed) and elapsed >= 0
                and not any(case.find(key) is not None for key in ("failure", "error", "skipped")),
                "JUnit execution")
    require((PACKET / prefix / "JUnit.stdout").read_bytes() ==
            (PACKET / prefix / "ctest.junit.xml").read_bytes(), "JUnit copy")
    fence = read(prefix + "ctest.fence.json")["source"]
    outputs = read(prefix + "test_outputs.json")
    require(outputs["fresh"] is True, "test output freshness")
    for relative, item in outputs["copies"].items():
        source, archive = item["source"], item["archive"]
        require(source["sha256"] == archive["sha256"] == sha(PACKET / prefix / relative),
                "test output copy binding")
        require(source["mtime_ns"] > fence["mtime_ns"] and
                source["ctime_ns"] > fence["ctime_ns"], "post-fence test outputs")
    raw = (PACKET / prefix / "LastTest.stdout").read_text()
    names = re.findall(r"^\d+/\d+ Test: (.+)$", raw, re.MULTILINE)
    require(len(names) == 20 and set(names) == set(tests), "LastTest names")
    require(raw.count("Test Passed.") == 20, "LastTest passed blocks")
    lines = [line for line in raw.splitlines() if line.startswith("full_") and "floor=" in line]
    require(len(lines) == 13 and all(" failures=0 floor=1" in line for line in lines),
            "untruncated test floors")
    reports = {}
    for label in ("full_gabriel_allocation", "full_gabriel_lazy_allocation",
                  "full_gabriel_successor mode=--selftest", "full_gabriel_successor mode=--rejects"):
        selected = [line for line in lines if line.startswith(label + " ")]
        require(len(selected) == 1, "summary uniqueness")
        reports[label] = {k: int(v) for k, v in re.findall(r"(\w+)=(\d+)", selected[0])}
    for label, count in (("full_gabriel_allocation", 49), ("full_gabriel_lazy_allocation", 209)):
        row = reports[label]
        require(row["allocations"] == row["fault_runs"] == row["denied"] == count
                and row["escaped"] == 0, "allocation floor")
    for suffix, expected in {
        "--selftest": {"primitive_cases": 560, "primitive_calls": 1242,
                       "full_pairs": 180, "full_positive_pairs": 180, "cuts": 3320},
        "--rejects": {"full_pairs": 668, "full_refused": 640,
                      "successor_exact": 32, "successor_short": 64, "distinct_admissions": 16},
    }.items():
        report = reports["full_gabriel_successor mode=" + suffix]
        require(all(report[key] == value for key, value in expected.items()), "successor floors")
    return {"tests": 20, "binaries": 8, "testing_binaries": 2, "reports": reports}


def review() -> dict:
    inventory = read("manifest.json")
    actual = {str(p.relative_to(PACKET)) for p in PACKET.rglob("*") if p.is_file()}
    require(len(inventory) == 278 and len(actual) == 280 and
            set(inventory) | {"manifest.json", "SHA256SUMS"} == actual, "packet inventory")
    for relative, identity in inventory.items():
        path = PACKET / relative
        require(path.resolve().is_relative_to(PACKET), "inventory escape")
        require(sha(path) == identity["sha256"] and path.stat().st_size == identity["bytes"], relative)
    sums = {}
    for line in (PACKET / "SHA256SUMS").read_text().splitlines():
        digest, relative = line.split("  ", 1)
        require(relative not in sums, "duplicate checksum")
        path = PACKET / relative
        require(path.resolve().is_relative_to(PACKET) and sha(path) == digest, relative)
        sums[relative] = digest
    require(set(sums) == set(inventory) | {"manifest.json"} and
            sha(PACKET / "SHA256SUMS") == SEAL, "checksum closure/pin")
    receipt, publication = read("qualification/receipt.json"), read("publication.json")
    require(sha(PACKET / "qualification/receipt.json") ==
            publication["capture_receipt_sha256"] == RECEIPT, "receipt pin")
    require(receipt["status"] == "completed" and not receipt["errors"]
            and receipt["build_tag"] == "r2", "R2 completion")
    require(receipt["producer_sha256"] == publication["producer_sha256"] == SOURCE, "producer binding")
    require(publication["controller_sha256"] == receipt["controller_sha256"] ==
            sha(PACKET / "qualification/protocol/capture.py"), "controller binding")
    require(publication["publisher_sha256"] == sha(PACKET / "protocol/publish.py"), "publisher binding")
    require(publication["whole_suite_qualified"] is False and
            receipt["historical_results_reused"] is False and receipt["gcp_used"] is False
            and receipt["permission_override"] == "none", "qualification scope")
    artifact_closure("qualification", receipt, 184)
    sources = read("qualification/sources_before.json")
    require(sources == read("qualification/sources_after.json") and
            sha(PACKET / "qualification/sources_before.json") ==
            receipt["source_sha256"] == SOURCE_MAP, "source map binding")
    require(len(sources) == 585, "source floor")
    counts = {"v7": 0, "boost": 0, "historical_depfile": 0}
    for relative, digest in sources.items():
        require(sha(ROOT / relative) == digest, relative + " current source drift")
        category = "boost" if "/boost/" in relative else (
            "v7" if relative.startswith("morsehgp3D_v7/") else "historical_depfile")
        counts[category] += 1
    require(counts == {"v7": 63, "boost": 521, "historical_depfile": 1}, "source categories")
    require(len(receipt["tests"]) == 20 and len(receipt["targets"]) == 8, "test floor")
    phases = {mode: review_phase(mode, receipt, sources) for mode in ("release", "san")}
    require(phases["release"] == phases["san"], "cross-build semantic summaries")
    metadata = read("qualification/metadata_selftest_normal.stdout")
    require(metadata == read("qualification/metadata_selftest_optimized.stdout") and
            metadata["status"] == "selftests_passed" and metadata["engine_runs"] == 0
            and metadata["models_are_engine_receipts"] is False and metadata["positive_models"] == 2
            and len(set(metadata["mutants_killed"])) == len(metadata["mutants_killed"]) == 12,
            "metadata selftest scope")
    failed_prefix = "failed_qualification/00_run_qualification"
    failed = read(failed_prefix + "/receipt.json")
    require(sha(PACKET / failed_prefix / "receipt.json") == FAILED
            and failed["status"] == "failed" and len(failed["phases"]) == 1, "failed attempt pin/status")
    require(publication["failed_qualification"][0]["sha256"] == FAILED and
            publication["failed_qualification"][0]["not_imported_as_qualification"] is True,
            "failed attempt not promoted")
    commands = failed["phases"][0]["commands"]
    require([(c["label"], c["exit_code"]) for c in commands] ==
            [("configure", 0), ("build", 2)], "failed attempt has no CTest")
    require("reference to 'detail' is ambiguous" in
            (PACKET / failed_prefix / "release/build.stderr").read_text(), "initial error diagnosis")
    artifact_closure(failed_prefix, failed, 89)
    old_sources = read(failed_prefix + "/sources_before.json")
    gate = "morsehgp3D_v7/tests/full_gabriel_successor_gate.cpp"
    require(set(old_sources) == set(sources) and
            [p for p in sources if sources[p] != old_sources[p]] == [gate]
            and old_sources[gate] == OLD_GATE and sources[gate] == NEW_GATE,
            "only gate changed after failed attempt")
    return {
        "status": "pass", "scope": "constructor captures inspected, no engine rerun",
        "script_sha256": sha(Path(__file__)), "packet": str(PACKET.relative_to(ROOT)),
        "receipt_sha256": RECEIPT, "producer_sha256": SOURCE, "source_map_sha256": SOURCE_MAP,
        "packet_seal_sha256": SEAL, "packet_files": len(actual), "inventory_entries": len(inventory),
        "checksum_entries": len(sums), "source_pins": len(sources), "source_categories": counts,
        "phases": phases, "metadata_selftests": metadata,
        "failed_attempt": {"receipt_sha256": FAILED, "status": "failed", "ctests": 0,
                           "artifacts": 89, "not_imported_as_qualification": True},
        "closed_utc": receipt["ended_utc"], "fresh_not_hermetic": True,
        "performance_qualified": False, "public_status": "not_claimed", "gcp": "not_used",
    }


if __name__ == "__main__":
    print(json.dumps(review(), indent=2, sort_keys=True))
