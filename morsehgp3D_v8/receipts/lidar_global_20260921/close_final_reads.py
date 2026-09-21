#!/usr/bin/env python3
"""Read closed LiDAR receipts in normal/-O modes; never rerun native tests.

Explicit small port of q4_window_20260920/close_reads.py. The initial global
v1 capture is read historically, not promoted to the current source snapshot.
"""
import json
import os
from pathlib import Path
import signal
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
from run_wspd_q34_lidar import SOURCES
from run_q4_family_checks import digest, pins, read_json
from run_p0_matrix import invoke, on_signal, parse_result, require, utc_stamp, write_json

CAPTURES = (
    ("edge_scale_gom3hgqp", "edge", True),
    ("gate_tb_fv9nw", "edge", True),
    ("edge_pilot12_d2jo8ey8", "edge", True),
    ("regression_s88mxc3a", "edge", True),
    ("global_3ml5xuwx", "global", True),
    ("global__6ze1yo7", "global", True),
    ("global__rlwn_g2", "global", True),
    ("global_vwtz76da", "global", False),
)
FAILED = "gate_kny8nlmd"


def main():
    output = HERE / "FINAL_READBACK.json"
    require(not output.exists(), "immutable readback already exists")
    readers = {"edge": ROOT / "morsehgp3D_v8/bench/run_q34_lidar_checks.py",
               "global": ROOT / "morsehgp3D_v8/bench/run_wspd_q34_lidar.py"}
    files = {str(Path(__file__).resolve().relative_to(ROOT))}
    artifacts, commands, captured = set(), [], []
    for name, reader, live in CAPTURES:
        path = HERE / name
        manifest, completion = read_json(path / "MANIFEST.json"), read_json(path / "COMPLETION.json")
        require(completion["status"] == "passed", "capture not closed successfully: " + name)
        files.update(str(p.relative_to(ROOT)) for p in path.rglob("*") if p.is_file())
        files.update(manifest["input_sha256"])
        artifacts.update(manifest["artifact_sha256"])
        captured.append(dict(path=str(path.relative_to(ROOT)), check_live=live,
            source_sha256=manifest["source_sha256"], artifact_sha256=manifest["artifact_sha256"],
            manifest_sha256=digest(path / "MANIFEST.json"), completion_sha256=digest(path / "COMPLETION.json")))
        commands.append(([str(readers[reader]), "read", str(path), "--compact",
                          *(["--check-live"] if live else [])], True))
    for name, reader in (("edge_scale_gom3hgqp", "edge"), ("global_3ml5xuwx", "global")):
        commands.append(([str(readers[reader]), "selftest", str(HERE / name)], True))
    failed = HERE / FAILED
    require(read_json(failed / "COMPLETION.json")["status"] == "failed", "preserved failure changed")
    files.update(str(p.relative_to(ROOT)) for p in failed.rglob("*") if p.is_file())
    commands.append(([str(readers["edge"]), "read", str(failed), "--compact"], False))
    source_before, input_before, artifact_before = pins(SOURCES), pins(files), pins(artifacts)
    started = utc_stamp()
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    records, summaries, status, error = [], [], "failed", None
    try:
        for optimized in (False, True):
            values = []
            for command, expected_pass in commands:
                argv = [sys.executable, "-B", *(["-O"] if optimized else []), *command]
                record = dict(command=argv, cwd=str(ROOT), started_utc=utc_stamp(), status="failed",
                    expected_reader_success=expected_pass, exit_code=None, stdout="", stderr="",
                    stdout_base64="", stderr_base64="")
                try:
                    invoke(argv, dict(os.environ), ROOT, record, new_session=True)
                    if expected_pass:
                        require(record["exit_code"] == 0, "reader command failed")
                        result = parse_result(record["stdout"].encode())
                        require(result["status"] == "passed", "reader result failed")
                        values.append(result)
                    else:
                        require(type(record["exit_code"]) is int and record["exit_code"] != 0 and
                                record["stderr"], "failed capture incorrectly accepted")
                        values.append(dict(preserved_failed_capture=FAILED, correctly_rejected=True))
                    record["status"] = "passed"
                finally:
                    record["finished_utc"] = utc_stamp()
                    records.append(record)
            summaries.append(values)
        require(summaries[0] == summaries[1], "normal/-O result differs")
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
        result = dict(schema="mhgp8_lidar_final_readback_v1", status=status, error=error,
            started_utc=started, finished_utc=utc_stamp(), commands=len(records), records=records,
            native_reexecutions=0, capture_count=len(CAPTURES), captures=captured,
            normal_optimized_identical=len(summaries) == 2 and summaries[0] == summaries[1],
            results=summaries[0] if summaries else [],
            source_sha256=source_before, source_sha256_after=close("sources", lambda: pins(SOURCES)),
            input_sha256=input_before, input_sha256_after=close("inputs", lambda: pins(files)),
            artifact_sha256=artifact_before, artifact_sha256_after=close("artifacts", lambda: pins(artifacts)),
            closing_errors=errors, full_contract_qualified=False, gcp_used=False,
            scope="receipt verification only; historical global v1 is not a current-source native qualification")
        if errors or any(result[key] != result[key + "_after"]
                         for key in ("source_sha256", "input_sha256", "artifact_sha256")):
            result["status"] = "failed"
            result["error"] = error or "verification closure changed"
        write_json(output, result)
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(path=str(output), status=result["status"], error=result["error"],
                             commands=len(records), sources=len(SOURCES))), flush=True)
    require(result["status"] == "passed", "verification closure failed")


if __name__ == "__main__":
    main()
