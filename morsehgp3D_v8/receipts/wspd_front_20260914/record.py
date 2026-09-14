#!/usr/bin/env python3
"""Close the local front qualification from checked campaigns and existing CTests.

This records component tests and measurements, never the 50k FULL/GPU contract.
No build, test, benchmark, cloud command or overwrite is performed by this script.
"""

from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[3]
DEST = Path(__file__).resolve().parent
SOURCE = ROOT / "morsehgp3D_v8"
READER = SOURCE / "bench/run_wspd_front_matrix.py"
OUTPUT_NAMES = ("SUMMARY.json", "VALIDATION.json", "QUALIFICATION.json")
BINARIES = ("mhgp8_wspd_front_probe", "mhgp8_wspd_front_gate", "mhgp8_q2_census_gate")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_pins() -> dict[str, str]:
    paths = [SOURCE / "CMakeLists.txt", Path(__file__).resolve()]
    for directory in ("src", "tests", "oracle", "bench"):
        paths.extend(path for path in (SOURCE / directory).rglob("*")
                     if path.is_file() and path.suffix in (".hpp", ".cpp", ".py"))
    return {str(path.relative_to(ROOT)): digest(path) for path in sorted(paths)}


def write(name: str, value: dict) -> None:
    with (DEST / name).open("x", encoding="utf-8") as stream:
        json.dump(value, stream, indent=2, allow_nan=False)
        stream.write("\n")


def xml_receipt(build: Path) -> dict:
    path = build / "ctest_full.xml"
    raw = path.read_text(encoding="utf-8")
    suite = ET.fromstring(raw)
    cases = list(suite.iter("testcase"))
    names = [case.attrib["name"] for case in cases]
    require(suite.tag == "testsuite" and len(cases) == 40 and len(set(names)) == 40 and
            int(suite.attrib["tests"]) == 40 and int(suite.attrib["failures"]) == 0 and
            int(suite.attrib.get("errors", "0")) == 0 and
            int(suite.attrib.get("skipped", "0")) == 0 and
            all(case.find("failure") is None and case.find("error") is None and
                case.find("skipped") is None for case in cases),
            f"expected 40 distinct passing CTests: {path}")
    return dict(path=str(path.relative_to(ROOT)), sha256=digest(path), xml=raw,
                tests=40, failures=0, test_names=names)


def main() -> int:
    require(all(not (DEST / name).exists() for name in OUTPUT_NAMES),
            "qualification outputs already exist; nothing may be overwritten")
    pins = source_pins()
    artifacts: dict[str, str] = {}
    builds = []
    for suffix, sanitized in (("v8_front_20260914", False),
                              ("v8_front_sanitize_20260914", True)):
        build = ROOT / "build" / suffix
        tests = xml_receipt(build)
        cache_path = build / "CMakeCache.txt"
        cache = cache_path.read_text(encoding="utf-8")
        require(f"MHGP8_SANITIZE:BOOL={'ON' if sanitized else 'OFF'}" in cache.splitlines(),
                "sanitizer cache setting disagrees with the declared build")
        require(f"CMAKE_BUILD_TYPE:STRING={'Debug' if sanitized else 'Release'}" in cache.splitlines(),
                "build type disagrees with the declared qualification")
        binary_pins = {name: digest(build / name) for name in BINARIES}
        for path in (cache_path, build / "ctest_full.xml", *(build / name for name in BINARIES)):
            artifacts[str(path.relative_to(ROOT))] = digest(path)
        builds.append(dict(path=str(build.relative_to(ROOT)), sanitized=sanitized,
            sandbox="require_escalated_for_lsan" if sanitized else "workspace",
            cmake_cache=cache, cmake_cache_sha256=digest(cache_path),
            tests=tests, binary_sha256=binary_pins))
    require(builds[0]["tests"]["test_names"] == builds[1]["tests"]["test_names"],
            "Release and sanitizer test names or ordering differ")

    failure_path = DEST / "ENVIRONMENTAL_FAILURE.json"
    failure_raw = failure_path.read_text(encoding="utf-8")
    failure = json.loads(failure_raw)
    require(failure.get("command") == ["ctest", "--test-dir", "build/v8_front_sanitize_20260914",
            "--output-on-failure", "-R", "^mhgp8_wspd_front_gate$"] and
            failure.get("exit_code") == 8 and failure.get("output_kind") == "observed tool-output excerpt" and
            "LeakSanitizer does not work under ptrace" in failure.get("output", ""),
            "missing or changed observed sandbox failure")
    artifacts[str(failure_path.relative_to(ROOT))] = digest(failure_path)

    manifests = sorted(DEST.rglob("MANIFEST.json"))
    require(bool(manifests), "missing performance campaigns")
    for path in manifests:
        for name in ("MANIFEST.json", "MEASURES.jsonl", "COMPLETION.json"):
            receipt = path.parent / name
            artifacts[str(receipt.relative_to(ROOT))] = digest(receipt)

    reads = []
    for optimized in (False, True):
        command = [sys.executable, "-B", *(["-O"] if optimized else []), str(READER),
                   "check", str(DEST), "--summary"]
        process = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        reads.append(dict(command=command, exit_code=process.returncode,
                          stdout=process.stdout, stderr=process.stderr))
        require(process.returncode == 0 and not process.stderr, "front campaign reader rejected")
    require(reads[0]["stdout"] == reads[1]["stdout"], "normal and optimized readers differ")
    summary = json.loads(reads[0]["stdout"])
    require(summary.get("status") == "passed" and
            summary.get("scope") == "real_wspd_front_without_census_or_full" and
            summary.get("full_contract_qualified") is False and summary.get("gcp_used") is False,
            "campaigns incomplete or unsupported scope")

    # Bind performance campaigns to the exact Release artefacts above, not
    # merely to some binary with a matching historical receipt structure.
    release = ROOT / builds[0]["path"]
    for path in manifests:
        manifest = json.loads(path.read_text(encoding="utf-8"))
        require(manifest.get("probe") == str(release / "mhgp8_wspd_front_probe") and
                manifest.get("probe_sha256") == builds[0]["binary_sha256"]["mhgp8_wspd_front_probe"] and
                manifest.get("cmake_cache") == builds[0]["cmake_cache"],
                "campaign is not bound to the qualified Release probe and cache")

    base_commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    worktree_status = subprocess.check_output(["git", "status", "--short"], cwd=ROOT, text=True)
    require(source_pins() == pins, "source set or content changed during qualification")
    require(sorted(DEST.rglob("MANIFEST.json")) == manifests,
            "campaign manifest set changed during qualification")
    require(all(digest(ROOT / name) == value for name, value in artifacts.items()),
            "build, failure or campaign artefact changed during qualification")
    write("SUMMARY.json", summary)
    write("VALIDATION.json", dict(status="passed", scope="receipt_consistency_not_geometry",
        readers=reads, strict_stdout_equality=True, full_contract_qualified=False,
        fifty_thousand_full_contract_qualified=False, gpu_qualified=False, gcp_used=False))
    write("QUALIFICATION.json", dict(schema="mhgp8_wspd_front_qualification_v1", status="passed",
        recorded_utc=datetime.now(timezone.utc).isoformat(), phase="exploration_v8_hors_registre",
        backend="cpu_reference", profile="quantized_u16_input_only", mode="implementation_v8_p0",
        public_status="not_claimed", gcp_used=False, full_contract_qualified=False,
        fifty_thousand_full_contract_qualified=False, gpu_qualified=False,
        scope="local_cpp_and_receipt_gates_for_front_not_census_or_full",
        source_sha256=pins, source_base_commit=base_commit, worktree_status=worktree_status,
        artifact_sha256=artifacts, builds=builds,
        environmental_failure=dict(path=str(failure_path.relative_to(ROOT)),
            sha256=digest(failure_path), raw_json=failure_raw, receipt=failure),
        environmental_failure_resolution="rerun_with_approved_escalation_without_disabling_lsan",
        development_failures=[], source_hashes_unchanged=True, artifact_hashes_unchanged=True,
        test_coverage="40 identical named CTests per build; existing XML, not a fresh rebuild"))
    print(json.dumps(dict(status="passed", measurements=summary["measurements"], tests_per_build=40)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
