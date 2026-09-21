#!/usr/bin/env python3
"""Four Python reads; preserve the first rejected capacity comparison separately."""
import json
import os
from pathlib import Path
import signal
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(HERE))
import check_default_compatibility as proof
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import invoke, on_signal, parse_result, require, utc_stamp, write_json


def main():
    output = HERE / "DEFAULT_COMPATIBILITY_READBACK.json"
    require(not output.exists(), "refuse overwriting a readback")
    capture = read_json(proof.OUTPUT)
    failed_path = HERE / "DEFAULT_COMPATIBILITY_FIRST_FAILED.json"
    failed = read_json(failed_path)
    archive = HERE / "preflight/default_compatibility_before_capacity_fix.py"
    helper_name = str((HERE / "check_default_compatibility.py").relative_to(ROOT))
    require(failed["status"] == "failed" and len(failed["records"]) == 4 and
            len(failed["comparisons"]) == 1 and failed["closing_errors"] == [] and
            "memory.worker_record_capacity_bytes_before_merge" in failed["error"],
            "initial failure history differs")
    require(digest(archive) == failed["input_sha256"][helper_name], "initial helper archive differs")
    for key in ("source_sha256", "input_sha256", "artifact_sha256"):
        require(failed[key] == failed[key + "_after"], "initial capture was not closed")
    require(failed["source_sha256"] == capture["source_sha256"] and
            failed["artifact_sha256"] == capture["artifact_sha256"], "native sources/binaries changed between attempts")
    # Diagnose the preserved four raw rows without changing their failed status.
    initial_pairs = []
    for number, record in enumerate(failed["records"]):
        require(record["exit_code"] == 0 and not record["stderr"], "initial native command failed")
        row = parse_result(record["stdout"].encode())
        proof.legacy.validate(row, record["command"])
        require(row == record["row"], "initial raw/parsed row differs")
        if number % 2:
            initial_pairs.append(proof.compare(failed["records"][number - 1]["row"], row))
    inputs = set(capture["input_sha256"])
    inputs.update(str(p.relative_to(ROOT)) for p in (proof.OUTPUT, failed_path, archive, Path(__file__).resolve()))
    source_before, input_before = pins(proof.current.SOURCES), pins(inputs)
    artifact_before = pins(capture["artifact_sha256"])
    require(source_before == capture["source_sha256"] and artifact_before == capture["artifact_sha256"],
            "live qualification changed before reads")
    previous = {s: signal.signal(s, on_signal) for s in (signal.SIGINT, signal.SIGTERM)}
    records, results, error, status, started = [], [], None, "failed", utc_stamp()
    try:
        for optimized in (False, True):
            for live in (False, True):
                command = [sys.executable, "-B", *(["-O"] if optimized else []), str(HERE / "check_default_compatibility.py"),
                           "read", *(["--check-live"] if live else [])]
                record = dict(command=command, cwd=str(ROOT), started_utc=utc_stamp(), status="failed",
                              exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="")
                try:
                    invoke(command, dict(os.environ), ROOT, record, new_session=True)
                    require(record["exit_code"] == 0 and not record["stderr"], "compatibility reader failed")
                    result = parse_result(record["stdout"].encode())
                    require(result["status"] == "passed", "reader status differs")
                    results.append(result)
                    record["status"] = "passed"
                finally:
                    record["finished_utc"] = utc_stamp()
                    records.append(record)
        require(len(results) == 4 and all(x == results[0] for x in results), "normal/-O live/historical reads differ")
        status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        for sig in previous:
            signal.signal(sig, signal.SIG_IGN)
        errors = []
        def close(label, function):
            try:
                return function()
            except Exception as cause:
                errors.append(f"{label}: {type(cause).__name__}: {cause}")
                return None
        receipt = dict(schema="mhgp8_default31_32_readback_v1", status=status, error=error,
            started_utc=started, finished_utc=utc_stamp(), commands=len(records), records=records,
            results=results, native_reexecutions=0, total_native_attempt_commands=20,
            final_capture_commands=16, initial_capture_status="failed", initial_capture_promoted=False,
            initial_failure=str(failed_path.relative_to(ROOT)), initial_pairs_reanalysed=initial_pairs,
            normal_optimized_identical=len(results) == 4 and all(x == results[0] for x in results),
            source_sha256=source_before, source_sha256_after=close("sources", lambda: pins(proof.current.SOURCES)),
            input_sha256=input_before, input_sha256_after=close("inputs", lambda: pins(inputs)),
            artifact_sha256=artifact_before, artifact_sha256_after=close("artifacts", lambda: pins(artifact_before)),
            closing_errors=errors, full_contract_qualified=False, gcp_used=False)
        if errors or any(receipt[k] != receipt[k + "_after"] for k in ("source_sha256", "input_sha256", "artifact_sha256")):
            receipt["status"] = "failed"
            receipt["error"] = error or "readback closure changed"
        write_json(output, receipt)
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(status=receipt["status"], error=receipt["error"], commands=len(records),
            sources=len(source_before), inputs=len(input_before), artifacts=len(artifact_before))))
    require(receipt["status"] == "passed", "readback failed")


if __name__ == "__main__":
    main()
