#!/usr/bin/env python3
"""Record normal/-O verification of explicit q4 and regression captures."""
import argparse
import json
import os
from pathlib import Path
import signal
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
from run_q4_family_checks import SOURCES, digest, pins
from run_p0_matrix import invoke, on_signal, parse_result, require, utc_stamp, write_json


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("captures", nargs=3, type=Path)
    parser.add_argument("--regression", required=True, type=Path)
    args = parser.parse_args()
    captures = [path.resolve() for path in args.captures]
    regression = args.regression.resolve()
    require(len(set(captures)) == 3 and all(path.is_relative_to(ROOT / "morsehgp3D_v8/receipts")
            for path in [*captures, regression]), "explicit local captures required")
    paths = [Path(__file__).resolve(), HERE / "record_regression.py"]
    for capture in [*captures, regression]:
        paths.extend(path for path in capture.rglob("*") if path.is_file())
    inputs = {str(path.relative_to(ROOT)): digest(path) for path in paths}
    sources = pins(SOURCES)
    output = Path(tempfile.mkdtemp(prefix="readers_", dir=HERE))
    probe_reader = str(ROOT / "morsehgp3D_v8/bench/run_q4_family_checks.py")
    commands = [[probe_reader, "read", str(path), "--check-live"] for path in captures]
    commands += [[probe_reader, "selftest", str(captures[0])],
                 [str(HERE / "record_regression.py"), "read", str(regression), "--check-live"]]
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, summaries = [], []
    status, error = "failed", None
    try:
        for optimized in (False, True):
            values = []
            for number, command in enumerate(commands):
                argv = [sys.executable, "-B", *(["-O"] if optimized else []), *command]
                record = dict(command=argv, cwd=str(ROOT), started_utc=utc_stamp(), status="failed",
                              exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="")
                try:
                    invoke(argv, dict(os.environ), ROOT, record, new_session=True)
                    require(record["exit_code"] == 0, "reader command failed")
                    value = parse_result(record["stdout"].encode())
                    require(value["status"] == "passed", "reader result failed")
                    values.append(value)
                    record["status"] = "passed"
                finally:
                    record["finished_utc"] = utc_stamp()
                    target = output / f"{'optimized' if optimized else 'normal'}_{number}.json"
                    write_json(target, record)
                    records.append(dict(path=target.name, sha256=digest(target)))
            summaries.append(values)
        require(summaries[0] == summaries[1], "normal/-O verification differs")
        require(pins(SOURCES) == sources and pins(inputs) == inputs, "verification inputs changed")
        write_json(output / "SUMMARY.json", dict(status="passed", results=summaries[0],
            commands=len(records), normal_optimized_identical=True,
            full_contract_qualified=False, gcp_used=False))
        status = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        for sig in previous:
            signal.signal(sig, signal.SIG_IGN)
        write_json(output / "COMPLETION.json", dict(status=status, error=error,
            finished_utc=utc_stamp(), records=records, input_sha256=inputs,
            input_sha256_after=pins(inputs), source_sha256=sources,
            source_sha256_after=pins(SOURCES),
            summary_sha256=digest(output / "SUMMARY.json") if (output / "SUMMARY.json").exists() else None))
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(path=str(output), status=status, error=error)))


if __name__ == "__main__":
    main()
