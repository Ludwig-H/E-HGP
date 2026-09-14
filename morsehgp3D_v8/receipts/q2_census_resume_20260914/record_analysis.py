#!/usr/bin/env python3
"""Retain normal/-O analysis, input hashes and failed attempts without overwrite."""
import json
import os
from pathlib import Path
import signal
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
from run_q2_resume_checks import digest, require, source_pins, write_json  # noqa: E402
from run_p0_matrix import invoke, on_signal, utc_stamp  # noqa: E402


def main():
    require(len(sys.argv) == 1, "analysis capture takes no arguments")
    names = ("qualification_0ir4r0bq", "qualification_g4tpzakt", "matrix_7ys75y08", "full_q2_regression", "tsan_dy2fgg87")
    inputs = [p for name in names for p in (HERE / name).rglob("*") if p.is_file()]
    inputs.extend([HERE / "analyze.py", Path(__file__).resolve(), HERE / "record_tsan.py"])
    pins = {str(p.relative_to(ROOT)): digest(p) for p in inputs}
    sources = source_pins()
    output = Path(tempfile.mkdtemp(prefix="analysis_", dir=HERE))
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, summaries = [], []
    status, error = "failed", None
    try:
        for optimized in (False, True):
            command = [sys.executable, "-B", *(["-O"] if optimized else []), str(HERE / "analyze.py")]
            record = dict(command=command, cwd=str(ROOT), started_utc=utc_stamp(), status="failed", exit_code=None,
                          stdout="", stderr="", stdout_base64="", stderr_base64="")
            try:
                invoke(command, dict(os.environ), ROOT, record, new_session=True)
                require(record["exit_code"] == 0, "analysis failed")
                summary = json.loads(record["stdout"])
                require(summary["status"] == "passed", "analysis did not pass")
                summaries.append(summary)
                record["status"] = "passed"
            finally:
                path = output / ("optimized.json" if optimized else "normal.json")
                write_json(path, record)
                records.append({"path": path.name, "sha256": digest(path)})
        require(summaries[0] == summaries[1], "normal/-O analyses differ")
        require(sources == source_pins() and all(digest(ROOT / path) == pin for path, pin in pins.items()), "analysis inputs changed")
        write_json(output / "SUMMARY.json", summaries[0])
        status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        write_json(output / "COMPLETION.json", dict(status=status, error=error, finished_utc=utc_stamp(),
            input_sha256=pins, source_sha256=sources, records=records,
            summary_sha256=digest(output / "SUMMARY.json") if (output / "SUMMARY.json").is_file() else None,
            input_sha256_after={path: digest(ROOT / path) for path in pins}, source_sha256_after=source_pins()))
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(path=str(output), status=status, error=error)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
