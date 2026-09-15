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
from run_q2_split_checks import digest, require, source_pins, write_json  # noqa: E402
from run_p0_matrix import invoke, on_signal, utc_stamp  # noqa: E402


def main():
    require(len(sys.argv) == 1, "analysis capture takes no arguments")
    names = ("qualification_u3qvrjyo", "qualification_ulbtho0b", "matrix_ocf7f2oa", "full_q2_regression", "tsan_jxjhkb5n")
    inputs = [p for name in names for p in (HERE / name).rglob("*") if p.is_file()]
    inputs.extend([HERE / "analyze.py", Path(__file__).resolve(), HERE / "record_tsan.py"])
    pins = {str(p.relative_to(ROOT)): digest(p) for p in inputs}
    sources = source_pins()
    tsan = HERE / "tsan_jxjhkb5n"
    attempt = json.loads((tsan / "attempt.json").read_text())
    close = json.loads((tsan / "completion.json").read_text())
    require(close["status"] == attempt["status"] == "passed" and attempt["exit_code"] == 0 and
            close["attempt_sha256"] == digest(tsan / "attempt.json") and
            close["source_sha256_after"] == attempt["source_sha256"] == sources and
            close["artifact_sha256_after"] == attempt["artifact_sha256"], "TSan closure mismatch")
    require(all(digest(ROOT / p) == h for p, h in attempt["artifact_sha256"].items()), "TSan artifacts changed")
    row = json.loads(attempt["stdout"])
    require(row["status"] == "passed" and row["parallel_calls"] == 2304 and
            row["donations"] > 0 and row["multi_slot_outputs"] > 0 and
            row["callback_failures"] == 4 and not attempt["stderr"], "TSan coverage missing")
    output = Path(tempfile.mkdtemp(prefix="analysis_", dir=HERE))
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, summaries = [], []
    status, error = "failed", None
    try:
        scripts = [
            [str(ROOT / "morsehgp3D_v8/bench/run_q2_split_checks.py"), "read", str(HERE / name)]
            for name in names[:3]]
        scripts += [[str(ROOT / "morsehgp3D_v8/bench/run_wspd_q2_parallel_matrix.py"),
                     "check", str(HERE / "full_q2_regression")], [str(HERE / "analyze.py")]]
        for optimized in (False, True):
            parsed = []
            for number, script in enumerate(scripts):
                command = [sys.executable, "-B", *(["-O"] if optimized else []), *script]
                record = dict(command=command, cwd=str(ROOT), started_utc=utc_stamp(), status="failed", exit_code=None,
                              stdout="", stderr="", stdout_base64="", stderr_base64="")
                try:
                    invoke(command, dict(os.environ), ROOT, record, new_session=True)
                    require(record["exit_code"] == 0, "reader/analysis failed")
                    summary = json.loads(record["stdout"])
                    require(summary["status"] == "passed", "reader/analysis did not pass")
                    parsed.append(summary)
                    record["status"] = "passed"
                finally:
                    path = output / f"{'optimized' if optimized else 'normal'}_{number}.json"
                    write_json(path, record)
                    records.append({"path": path.name, "sha256": digest(path)})
            summaries.append(parsed)
        require(summaries[0] == summaries[1], "normal/-O analyses differ")
        require(sources == source_pins() and all(digest(ROOT / path) == pin for path, pin in pins.items()), "analysis inputs changed")
        write_json(output / "SUMMARY.json", summaries[0][-1])
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
