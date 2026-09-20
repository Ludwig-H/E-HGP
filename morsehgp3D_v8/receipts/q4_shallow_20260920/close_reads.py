#!/usr/bin/env python3
"""Close explicit q4 shallow receipt reads and reader mutations in normal/-O modes.

Explicit small port of q4_local_20260920/close_reads.py. This helper is itself
pinned by its output; none of the 184 frozen implementation sources changes.
"""
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
from run_q4_shallow_checks import SOURCES
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import invoke, on_signal, parse_result, require, utc_stamp, write_json


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("captures", nargs="+", type=Path)
    p.add_argument("--selftest", required=True, type=Path)
    args = p.parse_args()
    captures = [path.resolve() for path in args.captures]
    selftest = args.selftest.resolve()
    require(len(set(captures)) == len(captures) and selftest in captures and
            all(path.parent == HERE for path in captures), "explicit distinct shallow captures required")
    paths = [Path(__file__).resolve()]
    artifacts = {}
    for capture in captures:
        paths.extend(path for path in capture.rglob("*") if path.is_file())
        m = read_json(capture / "MANIFEST.json")
        for name, pin in m["artifact_sha256"].items():
            require(name not in artifacts or artifacts[name] == pin, "binary changed between captures")
            artifacts[name] = pin
    inputs = {str(path.relative_to(ROOT)): digest(path) for path in paths}
    sources = pins(SOURCES)
    require(pins(artifacts) == artifacts, "captured binaries changed before verification")
    output = Path(tempfile.mkdtemp(prefix="readers_", dir=HERE))
    reader = str(ROOT / "morsehgp3D_v8/bench/run_q4_shallow_checks.py")
    commands = [[reader, "read", str(path), "--check-live"] for path in captures]
    commands.append([reader, "selftest", str(selftest)])
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, summaries, status, error = [], [], "failed", None
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
                    file = output / f"{'optimized' if optimized else 'normal'}_{number}.json"
                    write_json(file, record)
                    records.append(dict(path=file.name, sha256=digest(file)))
            summaries.append(values)
        require(summaries[0] == summaries[1], "normal/-O verification differs")
        require(pins(SOURCES) == sources and pins(inputs) == inputs and pins(artifacts) == artifacts,
                "verification inputs changed")
        write_json(output / "SUMMARY.json", dict(status="passed", results=summaries[0], commands=len(records),
            normal_optimized_identical=True, full_contract_qualified=False, gcp_used=False))
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
        completion = dict(status=status, error=error, finished_utc=utc_stamp(), records=records,
            input_sha256=inputs, input_sha256_after=close("inputs", lambda: pins(inputs)),
            source_sha256=sources, source_sha256_after=close("sources", lambda: pins(SOURCES)),
            artifact_sha256=artifacts, artifact_sha256_after=close("artifacts", lambda: pins(artifacts)),
            summary_sha256=digest(output / "SUMMARY.json") if (output / "SUMMARY.json").exists() else None,
            closing_errors=errors)
        if errors or any(completion[name] != completion[name + "_after"]
                         for name in ("input_sha256", "source_sha256", "artifact_sha256")):
            completion["status"] = "failed"
            completion["error"] = error or "verification closure changed or could not be read"
        write_json(output / "COMPLETION.json", completion)
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(path=str(output), status=completion["status"], error=completion["error"])), flush=True)
    require(completion["status"] == "passed", "verification closure failed")


if __name__ == "__main__":
    main()
