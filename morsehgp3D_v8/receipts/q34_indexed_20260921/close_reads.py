#!/usr/bin/env python3
"""Close tranche32 smoke reads and the pre-reader-fix native qualifications.

Only Python readers run. The old94CTest/three large LiDAR rows stay attached
to their original206-source snapshot; no favourable native replay is made.
"""
import json
import os
from pathlib import Path
import signal
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
import run_q34_indexed_checks as checks
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import invoke, on_signal, parse_result, require, utc_stamp, write_json

CAPTURES = (
    ("qualifications/smoke_2x8dvljr", True),
    ("qualifications/smoke_nc5osoja", True),
    ("qualifications/smoke_qbh9yxw5", True),
    ("qualifications/regression_ucul7g41", False),
    ("lidar/lidar_2x8pm0nw", False),
)
FAILURES = ("qualifications/smoke_5vvacjai", "qualifications/smoke_yclsnvf8")


def main():
    output = HERE / "READBACK.json"
    require(not output.exists(), "immutable readback already exists")
    inputs = {str(Path(__file__).resolve().relative_to(ROOT))}
    archive = HERE / "preflight/before_empty_calls_fix"
    inputs.update(str(p.relative_to(ROOT)) for p in archive.iterdir() if p.is_file())
    artifacts, captured, commands, rows = {}, [], [], []
    source_before = pins(checks.SOURCES)
    reader = str(ROOT / "morsehgp3D_v8/bench/run_q34_indexed_checks.py")
    for relative, live in CAPTURES:
        path = HERE / relative
        m, c = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
        require(c["status"] == "passed", "capture not closed successfully")
        inputs.update(str(p.relative_to(ROOT)) for p in path.rglob("*") if p.is_file())
        inputs.update(m["input_sha256"])
        for name, value in m["artifact_sha256"].items():
            require(name not in artifacts or artifacts[name] == value, "artifact changed between captures")
            artifacts[name] = value
        differences = {name: dict(captured=value, current=source_before[name])
                       for name, value in m["source_sha256"].items() if value != source_before[name]}
        if live:
            require(differences == {}, "current smoke sources changed")
        else:
            checker = "morsehgp3D_v8/bench/run_q34_indexed_checks.py"
            require(set(differences) == {checker}, "unexpected historical source change")
            for name in ("run_q34_indexed_checks.py", "run_q34_indexed_lidar.py"):
                require(digest(archive / name) == m["source_sha256"]["morsehgp3D_v8/bench/" + name],
                        "historical runner archive mismatch")
        captured.append(dict(path=relative, check_live=live, source_differences=differences,
            manifest_sha256=digest(path / "MANIFEST.json"), completion_sha256=digest(path / "COMPLETION.json")))
        commands.append(([reader, "read", str(path), "--compact", *(["--check-live"] if live else [])], True))
        if live:
            for file in sorted(path.glob("record_*.json")):
                item = read_json(file)
                if item["kind"] == "probe":
                    rows.append(item["row"])
    sanitizer = read_json(HERE / CAPTURES[2][0] / "MANIFEST.json")
    require(sanitizer["sanitizer_environment"]["ASAN_OPTIONS"] == "detect_leaks=1:halt_on_error=1" and
            sanitizer["sanitizer_environment"]["UBSAN_OPTIONS"] == "halt_on_error=1:print_stacktrace=1" and
            "MHGP8_SANITIZE:BOOL=ON" in sanitizer["compiler_cache"], "sanitizer capture did not keep leak checks enabled")
    for relative, live in CAPTURES:
        if live:
            commands.append(([reader, "selftest", str(HERE / relative)], True))
    for relative in FAILURES:
        path = HERE / relative
        require(read_json(path / "COMPLETION.json")["status"] == "failed", "preserved failure changed")
        inputs.update(str(p.relative_to(ROOT)) for p in path.rglob("*") if p.is_file())
        commands.append(([reader, "read", str(path), "--compact"], False))
    files = pins(inputs)
    require(pins(artifacts) == artifacts, "original binaries changed")
    started = utc_stamp()
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, summaries, status, error = [], [], "failed", None
    try:
        for optimized in (False, True):
            values = []
            for command, expected_success in commands:
                argv = [sys.executable, "-B", *(["-O"] if optimized else []), *command]
                record = dict(command=argv, cwd=str(ROOT), started_utc=utc_stamp(), status="failed",
                    expected_reader_success=expected_success, exit_code=None, stdout="", stderr="",
                    stdout_base64="", stderr_base64="")
                try:
                    invoke(argv, dict(os.environ), ROOT, record, new_session=True)
                    if expected_success:
                        require(record["exit_code"] == 0 and not record["stderr"], "reader command failed")
                        value = parse_result(record["stdout"].encode())
                        require(value["status"] == "passed", "reader result failed")
                        values.append(value)
                    else:
                        require(type(record["exit_code"]) is int and record["exit_code"] != 0 and record["stderr"],
                                "failed capture incorrectly accepted")
                        values.append(dict(failed_capture=command[2], correctly_rejected=True))
                    record["status"] = "passed"
                finally:
                    record["finished_utc"] = utc_stamp()
                    records.append(record)
            summaries.append(values)
        require(summaries[0] == summaries[1], "normal/-O verification differs")
        checks.paired(rows)  # Full records across Release scalar/boxes and SAN boxes.
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
        result = dict(schema="mhgp8_q34_indexed_readback_v1", status=status, error=error,
            started_utc=started, finished_utc=utc_stamp(), commands=len(records), records=records,
            native_reexecutions=0, captures=captured, historical_captures_promoted=False,
            paired_smoke_measurements=len(rows), paired_full_records=status == "passed",
            normal_optimized_identical=len(summaries) == 2 and summaries[0] == summaries[1],
            results=summaries[0] if summaries else [], full_contract_qualified=False, gcp_used=False,
            source_sha256=source_before, source_sha256_after=close("sources", lambda: pins(checks.SOURCES)),
            input_sha256=files, input_sha256_after=close("inputs", lambda: pins(inputs)),
            artifact_sha256=artifacts, artifact_sha256_after=close("artifacts", lambda: pins(artifacts)),
            closing_errors=errors)
        if errors or any(result[key] != result[key + "_after"]
                         for key in ("source_sha256", "input_sha256", "artifact_sha256")):
            result["status"] = "failed"
            result["error"] = error or "readback closure changed"
        write_json(output, result)
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(path=str(output), status=result["status"], error=result["error"], commands=len(records),
            sources=len(source_before), inputs=len(files), artifacts=len(artifacts))), flush=True)
    require(result["status"] == "passed", "readback closure failed")


if __name__ == "__main__":
    main()
