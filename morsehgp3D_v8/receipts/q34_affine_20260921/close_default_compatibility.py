#!/usr/bin/env python3
"""Four normal/-O live/historical reads of the native v2 compatibility capture."""
import json
import os
from pathlib import Path
import signal
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(HERE))
import check_default_compatibility as proof
from run_q4_family_checks import pins, read_json
from run_p0_matrix import invoke, on_signal, parse_result, require, utc_stamp, write_json


def main():
    output = HERE / "DEFAULT_COMPATIBILITY_READBACK.json"
    require(not output.exists(), "refuse overwriting compatibility readback")
    capture = read_json(proof.OUTPUT)
    inputs = set(capture["input_sha256"])
    inputs.update(str(p.relative_to(ROOT)) for p in (proof.OUTPUT, Path(__file__).resolve()))
    sources, files, artifacts = pins(proof.current.SOURCES), pins(inputs), pins(capture["artifact_sha256"])
    require(capture["source_sha256"] == sources and capture["artifact_sha256"] == artifacts,
            "capture inputs changed before reads")
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, values, error, status, started = [], [], None, "failed", utc_stamp()
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
                    value = parse_result(record["stdout"].encode())
                    require(value["status"] == "passed", "compatibility result failed")
                    values.append(value)
                    record["status"] = "passed"
                finally:
                    record["finished_utc"] = utc_stamp()
                    records.append(record)
        require(len(values) == 4 and all(value == values[0] for value in values), "normal/-O/live read mismatch")
        status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        for sig in previous:
            signal.signal(sig, signal.SIG_IGN)
        errors = []
        def close(label, action):
            try:
                return action()
            except Exception as cause:
                errors.append(f"{label}: {type(cause).__name__}: {cause}")
                return None
        result = dict(schema="mhgp8_default32_33_readback_v1", status=status, error=error,
            started_utc=started, finished_utc=utc_stamp(), commands=len(records), records=records, results=values,
            native_reexecutions=0, normal_optimized_identical=len(values) == 4 and all(x == values[0] for x in values),
            source_sha256=sources, source_sha256_after=close("sources", lambda: pins(proof.current.SOURCES)),
            input_sha256=files, input_sha256_after=close("inputs", lambda: pins(inputs)),
            artifact_sha256=artifacts, artifact_sha256_after=close("artifacts", lambda: pins(artifacts)),
            closing_errors=errors, full_contract_qualified=False, gcp_used=False)
        if errors or any(result[k] != result[k+"_after"] for k in ("source_sha256", "input_sha256", "artifact_sha256")):
            result.update(status="failed", error=error or "compatibility readback closure changed")
        write_json(output, result)
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(status=result["status"], error=result["error"], commands=len(records),
            sources=len(sources), inputs=len(files), artifacts=len(artifacts))))
    require(result["status"] == "passed", "compatibility readback failed")


if __name__ == "__main__":
    main()
