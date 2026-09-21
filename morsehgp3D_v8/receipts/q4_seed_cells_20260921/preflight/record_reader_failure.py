#!/usr/bin/env python3
"""Preserve the actual r1 reader-selftest failure, without native reruns."""
import json
import os
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[4]
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
import run_q4_seed_cells_checks as checks
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import invoke, require, utc_stamp, write_json


def main():
    output = HERE / "SELFTEST_R1_FAILURE.json"
    require(not output.exists(), "failure evidence already exists")
    capture = HERE.parent / "qualification/smoke_o0m_lvjj"
    manifest, completion = read_json(capture / "MANIFEST.json"), read_json(capture / "COMPLETION.json")
    require(completion["status"] == "passed" and completion["closing_errors"] == [], "smoke is not closed")
    sources = pins(checks.SOURCES)
    require(manifest["source_sha256"] == sources == completion["source_sha256_after"], "r1 source closure differs")
    inputs = {str(p.relative_to(ROOT)) for p in capture.rglob("*") if p.is_file()}
    inputs.add(str(Path(__file__).resolve().relative_to(ROOT)))
    inputs.update(manifest["input_sha256"])
    before = pins(inputs)
    artifacts = pins(manifest["artifact_sha256"])
    require(artifacts == manifest["artifact_sha256"] == completion["artifact_sha256_after"], "r1 artifact differs")
    records = []
    for optimized in (False, True):
        command = [sys.executable, "-B", *(["-O"] if optimized else []),
            str(ROOT / "morsehgp3D_v8/bench/run_q4_seed_cells_checks.py"), "selftest", str(capture)]
        record = dict(command=command, cwd=str(ROOT), started_utc=utc_stamp(), exit_code=None,
            stdout="", stderr="", stdout_base64="", stderr_base64="", status="failed")
        try:
            invoke(command, dict(os.environ), ROOT, record, new_session=True)
        finally:
            record["finished_utc"] = utc_stamp()
            records.append(record)
    after_sources, after_inputs, after_artifacts = pins(checks.SOURCES), pins(inputs), pins(artifacts)
    expected = all(r["exit_code"] == 1 and "seed/cell receipt corruption survived" in r["stderr"] for r in records)
    errors = []
    if sources != after_sources: errors.append("sources changed")
    if before != after_inputs: errors.append("inputs changed")
    if artifacts != after_artifacts: errors.append("artifacts changed")
    result = dict(schema="mhgp8_q4_seed_cells_r1_reader_failure_v1", status="failed",
        failure_reproduced_normal_and_optimized=expected, native_reexecutions=0,
        cause="live_child_reads+1 can remain below the aggregate upper bound when skipped dead atlases contain branches; reader lacks the necessary modulo-four check",
        manifest_sha256=digest(capture / "MANIFEST.json"), completion_sha256=digest(capture / "COMPLETION.json"),
        records=records, source_sha256=sources, source_sha256_after=after_sources,
        input_sha256=before, input_sha256_after=after_inputs, artifact_sha256=artifacts,
        artifact_sha256_after=after_artifacts, closing_errors=errors,
        full_contract_qualified=False, receipt_failure_not_product_failure=True)
    write_json(output, result)
    print(json.dumps(dict(path=str(output), status="failed", expected_failure_reproduced=expected,
        commands=len(records), closing_errors=errors)))
    require(expected and not errors, "r1 failure was not preserved as expected")


if __name__ == "__main__":
    main()
