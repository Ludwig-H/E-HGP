#!/usr/bin/env python3
"""Close four readers of the corrected causal mutations; no native replay."""
import json
import os
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
import run_q4_seed_cells_checks as checks
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import invoke, parse_result, require, utc_stamp, write_json


def main():
    output = HERE / "MUTANTS_R2_READBACK.json"
    require(not output.exists(), "mutation readback already exists")
    capture = HERE / "mutations_r2/compiled_lds5cf_8"
    manifest, completion = read_json(capture / "MANIFEST.json"), read_json(capture / "COMPLETION.json")
    sources = pins(checks.SOURCES)
    require(completion["status"] == "passed" and completion["error"] is None and completion["closing_errors"] == [] and
            manifest["source_sha256"] == sources == completion["source_sha256_after"] and
            completion["manifest_sha256"] == digest(capture / "MANIFEST.json"), "causal mutations not closed")
    artifacts = dict(manifest["artifact_sha256"])
    require(artifacts == completion["artifact_sha256_after"], "mutation build closure changed")
    for evidence in completion["evidence"]:
        for field in ("object", "binary"):
            artifacts[evidence[field]] = evidence[field + "_sha256"]
    require(len(completion["killed"]) == 3 and len(completion["records"]) == 10 and pins(artifacts) == artifacts,
            "mutation count or native artifact changed")
    inputs = {str(p.relative_to(ROOT)) for p in capture.rglob("*") if p.is_file()}
    inputs.update((str(Path(__file__).resolve().relative_to(ROOT)), manifest["compiler"]))
    before = pins(inputs)
    records, values, error, started = [], [], None, utc_stamp()
    try:
        for optimized in (False, True):
            results = []
            for live in (False, True):
                command = [sys.executable, "-B", *(["-O"] if optimized else []),
                    str(ROOT / "morsehgp3D_v8/tests/q4_seed_cells_mutations.py"), "read", str(capture),
                    *(["--check-live"] if live else [])]
                record = dict(command=command, cwd=str(ROOT), started_utc=utc_stamp(), status="failed", exit_code=None,
                    stdout="", stderr="", stdout_base64="", stderr_base64="")
                try:
                    invoke(command, dict(os.environ), ROOT, record, new_session=True)
                    require(record["exit_code"] == 0 and not record["stderr"], "mutation reader failed")
                    value = parse_result(record["stdout"].encode())
                    require(value["status"] == "passed" and value["compiled_product_mutants"] == 3, "mutation reader verdict")
                    results.append(value)
                    record["status"] = "passed"
                finally:
                    record["finished_utc"] = utc_stamp()
                    records.append(record)
            values.append(results)
        require(values[0] == values[1], "normal/-O mutation readers differ")
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
    after_sources, after_inputs, after_artifacts = pins(checks.SOURCES), pins(inputs), pins(artifacts)
    errors = []
    if sources != after_sources: errors.append("sources changed")
    if before != after_inputs: errors.append("inputs changed")
    if artifacts != after_artifacts: errors.append("artifacts changed")
    result = dict(schema="mhgp8_q4_seed_cells_mutants_r2_readback_v1", status="passed" if error is None and not errors else "failed",
        error=error, started_utc=started, finished_utc=utc_stamp(), commands=len(records), records=records, results=values,
        native_reexecutions=0, compiled_product_mutants=3, normal_optimized_identical=len(values) == 2 and values[0] == values[1],
        source_sha256=sources, source_sha256_after=after_sources, input_sha256=before, input_sha256_after=after_inputs,
        artifact_sha256=artifacts, artifact_sha256_after=after_artifacts, closing_errors=errors,
        full_contract_qualified=False, universal_subquadratic_claim=False, gcp_used=False)
    write_json(output, result)
    print(json.dumps(dict(path=str(output), status=result["status"], error=error, commands=len(records),
        sources=len(sources), inputs=len(before), artifacts=len(artifacts), closing_errors=errors)))
    require(result["status"] == "passed", "mutation readers did not close")


if __name__ == "__main__":
    main()
