#!/usr/bin/env python3
"""Close this local qualification from checked campaigns and existing CTest XML."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
DEST = Path(__file__).resolve().parent
SOURCE = ROOT / "morsehgp3D_v8"
READER = SOURCE / "bench/run_cloud_reuse_matrix.py"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(name: str, value: dict) -> None:
    with (DEST / name).open("x") as stream:
        json.dump(value, stream, indent=2, allow_nan=False)
        stream.write("\n")


def xml_receipt(build: Path, name: str, expected: int, failures: int) -> dict:
    path = build / name
    raw = path.read_text()
    suite = ET.fromstring(raw)
    cases = list(suite.iter("testcase"))
    require(len(cases) == expected and int(suite.attrib["tests"]) == expected and
            int(suite.attrib["failures"]) == failures and
            sum(case.find("failure") is not None for case in cases) == failures,
            "unexpected CTest result")
    return dict(path=str(path.relative_to(ROOT)), sha256=digest(path), xml=raw,
                tests=expected, failures=failures,
                test_names=[case.attrib["name"] for case in cases])


def main() -> int:
    paths = [SOURCE / "CMakeLists.txt", Path(__file__).resolve()]
    for directory in ("src", "tests", "oracle", "bench"):
        paths.extend(path for path in (SOURCE / directory).rglob("*")
                     if path.suffix in (".hpp", ".cpp", ".py"))
    pins = {str(path.relative_to(ROOT)): digest(path) for path in sorted(paths)}
    reads = []
    for optimized in (False, True):
        command = [sys.executable, "-B", *(["-O"] if optimized else []), str(READER),
                   "check", str(DEST), "--summary"]
        process = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        reads.append(dict(command=command, exit_code=process.returncode,
                          stdout=process.stdout, stderr=process.stderr))
        require(process.returncode == 0 and not process.stderr, "campaign reader rejected")
    require(reads[0]["stdout"] == reads[1]["stdout"], "optimized reader differs")
    summary = json.loads(reads[0]["stdout"])
    require(summary["status"] == "passed", "campaigns incomplete")
    builds = []
    for suffix, sanitized in (("v8_cloud_20260914", False), ("v8_cloud_sanitize_20260914", True)):
        build = ROOT / "build" / suffix
        first = "ctest_core_unsandboxed.xml" if sanitized else "ctest_core.xml"
        parts = [xml_receipt(build, first, 35, 0),
                 xml_receipt(build, "ctest_cloud_receipts.xml", 2, 0)]
        names = [name for part in parts for name in part["test_names"]]
        require(len(names) == len(set(names)) == 37, "test parts overlap or omit names")
        builds.append(dict(path=str(build.relative_to(ROOT)), sanitized=sanitized,
            sandbox="require_escalated_for_lsan" if sanitized else "workspace",
            cmake_cache=(build / "CMakeCache.txt").read_text(), parts=parts,
            binary_sha256={name: digest(build / name) for name in
                           ("mhgp8_cloud_owner_gate", "mhgp8_cloud_reuse_probe", "mhgp8_q2_census_gate")}))
    failed = xml_receipt(ROOT / "build/v8_cloud_sanitize_20260914", "ctest_core.xml", 35, 33)
    require("LeakSanitizer does not work under ptrace" in failed["xml"], "lost sandbox failure")
    require(all(digest(ROOT / name) == value for name, value in pins.items()), "sources changed")
    write("SUMMARY.json", summary)
    write("VALIDATION.json", dict(status="passed", scope="receipt_consistency_not_geometry",
        readers=reads, full_contract_qualified=False, gcp_used=False))
    write("QUALIFICATION.json", dict(schema="mhgp8_cloud_reuse_qualification_v1", status="passed",
        phase="exploration_v8_hors_registre", backend="cpu_reference", profile="quantized_u16_input_only",
        mode="implementation_v8_p0", public_status="not_claimed", gcp_used=False,
        scope="local_cpp_and_receipt_gates_not_wspd_or_full", source_sha256=pins,
        source_base_commit=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        builds=builds, environmental_failure=failed,
        environmental_failure_resolution="rerun_with_approved_escalation_without_disabling_lsan",
        development_failures=["initial strict build: signed/unsigned comparison in new gate corrected before qualification",
            "initial strict build: overloaded prepare_rectangle required an explicit legacy signature in p0_gate"],
        source_hashes_unchanged=True, test_coverage="35 core plus 2 receipt tests per build; sources unchanged during performance captures"))
    print(json.dumps(dict(status="passed", measurements=summary["measurements"], tests_per_build=37)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
