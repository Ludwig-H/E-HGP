#!/usr/bin/env python3
"""Close three corrected-source r2 receipts; twelve reads, no native rerun."""
import json
import os
from pathlib import Path
import signal
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
from run_q34_lidar_checks import SOURCES
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import invoke, on_signal, parse_result, require, utc_stamp, write_json

CAPTURES = ("gate_1a7ujimj", "gate_1q8uuu7m", "regression_0cd1l_3e")


def main():
    output = HERE / "R2_READBACK.json"
    require(not output.exists(), "immutable readback already exists")
    inputs = {str(Path(__file__).resolve().relative_to(ROOT))}
    artifacts, captures = {}, []
    for name in CAPTURES:
        capture = HERE / name
        manifest, completion = read_json(capture / "MANIFEST.json"), read_json(capture / "COMPLETION.json")
        require(completion["status"] == "passed", "capture not closed successfully")
        inputs.update(str(p.relative_to(ROOT)) for p in capture.rglob("*") if p.is_file())
        inputs.update(manifest["input_sha256"])
        for path, value in manifest["artifact_sha256"].items():
            require(path not in artifacts or artifacts[path] == value, "artifact differs between captures")
            artifacts[path] = value
        captures.append(dict(path=str(capture.relative_to(ROOT)), manifest_sha256=digest(capture / "MANIFEST.json"),
                             completion_sha256=digest(capture / "COMPLETION.json")))
    sources, files = pins(SOURCES), pins(inputs)
    require(pins(artifacts) == artifacts, "captured artifacts changed")
    started = utc_stamp()
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, summaries, status, error = [], [], "failed", None
    try:
        for optimized in (False, True):
            for live in (False, True):
                values = []
                for name in CAPTURES:
                    argv = [sys.executable, "-B", *(["-O"] if optimized else []),
                            str(ROOT / "morsehgp3D_v8/bench/run_q34_lidar_checks.py"),
                            "read", str(HERE / name), "--compact", *(["--check-live"] if live else [])]
                    record = dict(command=argv, cwd=str(ROOT), optimized=optimized, check_live=live,
                        started_utc=utc_stamp(), status="failed", exit_code=None,
                        stdout="", stderr="", stdout_base64="", stderr_base64="")
                    try:
                        invoke(argv, dict(os.environ), ROOT, record, new_session=True)
                        require(record["exit_code"] == 0 and not record["stderr"], "reader command failed")
                        value = parse_result(record["stdout"].encode())
                        require(value["status"] == "passed", "reader result failed")
                        values.append(value)
                        record["status"] = "passed"
                    finally:
                        record["finished_utc"] = utc_stamp()
                        records.append(record)
                summaries.append(values)
        require(all(values == summaries[0] for values in summaries), "four reader modes differ")
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
        result = dict(schema="mhgp8_lidar_r2_readback_v1", status=status, error=error,
            started_utc=started, finished_utc=utc_stamp(), commands=len(records), records=records,
            native_reexecutions=0, captures=captures,
            four_reader_modes_identical=len(summaries) == 4 and all(v == summaries[0] for v in summaries),
            results=summaries[0] if summaries else [], full_contract_qualified=False, gcp_used=False,
            source_sha256=sources, source_sha256_after=close("sources", lambda: pins(SOURCES)),
            input_sha256=files, input_sha256_after=close("inputs", lambda: pins(inputs)),
            artifact_sha256=artifacts, artifact_sha256_after=close("artifacts", lambda: pins(artifacts)),
            closing_errors=errors, scope="existing corrected-source r2 receipts only; no historical promotion")
        if errors or any(result[key] != result[key + "_after"]
                         for key in ("source_sha256", "input_sha256", "artifact_sha256")):
            result["status"] = "failed"
            result["error"] = error or "verification inputs changed"
        write_json(output, result)
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(path=str(output), status=result["status"], error=result["error"],
            commands=len(records), sources=len(sources), inputs=len(files), artifacts=len(artifacts))), flush=True)
    require(result["status"] == "passed", "verification closure failed")


if __name__ == "__main__":
    main()
