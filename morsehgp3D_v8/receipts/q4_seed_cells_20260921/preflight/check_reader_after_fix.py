#!/usr/bin/env python3
"""New reader selftests on immutable r1 captures, not a native requalification."""
import json
import os
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[4]
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
import run_q4_seed_cells_checks as checks
from run_q4_family_checks import pins, read_json
from run_p0_matrix import invoke, parse_result, require, utc_stamp, write_json


def main():
    output = HERE / "READER_AFTER_MODULO_FIX.json"
    require(not output.exists(), "reader retest evidence already exists")
    captures = [HERE.parent / name for name in
        ("qualification/smoke_o0m_lvjj", "qualification_sanitize/smoke_i7ezt4gx")]
    inputs = {str(Path(__file__).resolve().relative_to(ROOT))}
    artifacts = {}
    for capture in captures:
        manifest = read_json(capture / "MANIFEST.json")
        inputs.update(str(p.relative_to(ROOT)) for p in capture.rglob("*") if p.is_file())
        inputs.update(manifest["input_sha256"])
        artifacts.update(manifest["artifact_sha256"])
    # Only Python is executed here. Current C++ gate sources are deliberately
    # being strengthened concurrently; their old snapshots stay in r1 pins.
    readers = {p for p in checks.SOURCES if p.endswith(".py")}
    before_sources, before_inputs = pins(readers), pins(inputs)
    require(pins(artifacts) == artifacts, "pinned r1 binaries changed")
    records, results, error = [], [], None
    try:
        for optimized in (False, True):
            values = []
            for capture in captures:
                command = [sys.executable, "-B", *(["-O"] if optimized else []),
                    str(ROOT / "morsehgp3D_v8/bench/run_q4_seed_cells_checks.py"), "selftest", str(capture)]
                record = dict(command=command, cwd=str(ROOT), started_utc=utc_stamp(), exit_code=None,
                    stdout="", stderr="", stdout_base64="", stderr_base64="", status="failed")
                try:
                    invoke(command, dict(os.environ), ROOT, record, new_session=True)
                    require(record["exit_code"] == 0 and not record["stderr"], "corrected reader selftest failed")
                    value = parse_result(record["stdout"].encode())
                    require(value["status"] == "passed", "reader returned non-passed status")
                    values.append(value)
                    record["status"] = "passed"
                finally:
                    record["finished_utc"] = utc_stamp()
                    records.append(record)
            results.append(values)
        require(results[0] == results[1], "normal/-O selftests differ")
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
    after_sources, after_inputs, after_artifacts = pins(readers), pins(inputs), pins(artifacts)
    errors = []
    if before_sources != after_sources: errors.append("reader sources changed")
    if before_inputs != after_inputs: errors.append("inputs changed")
    if artifacts != after_artifacts: errors.append("r1 artifacts changed")
    result = dict(schema="mhgp8_q4_seed_cells_reader_after_modulo_fix_v1",
        status="passed" if error is None and not errors else "failed", error=error, records=records,
        commands=len(records), results=results, native_reexecutions=0,
        current_cpp_sources_not_executed_or_live_checked=True, r1_native_qualification_not_promoted=True,
        reader_source_sha256=before_sources, reader_source_sha256_after=after_sources,
        input_sha256=before_inputs, input_sha256_after=after_inputs,
        artifact_sha256=artifacts, artifact_sha256_after=after_artifacts, closing_errors=errors)
    write_json(output, result)
    print(json.dumps(dict(path=str(output), status=result["status"], error=error, commands=len(records), results=results,
        closing_errors=errors)))
    require(result["status"] == "passed", "reader retest failed; evidence preserved")


if __name__ == "__main__":
    main()
