#!/usr/bin/env python3
"""Persist four mutation-receipt rereads with unchanged input hashes.

This closure helper is outside the frozen196 product/test inventory; its own
hash is nevertheless included among the observed inputs. It never recompiles
or changes product sources, the build, or the compiled-mutation capture.
"""
import argparse
import json
import os
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
from run_p0_matrix import digest, invoke, parse_result, require, utc_stamp, write_json  # noqa: E402


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    capture, output = args.capture.resolve(), args.output.resolve()
    require(not output.exists(), "readback output already exists; choose a new file")
    manifest = json.loads((capture / "MANIFEST.json").read_text())
    completion = json.loads((capture / "COMPLETION.json").read_text())
    paths = {ROOT / name for name in manifest["source_sha256"]}
    paths.update(ROOT / name for name in manifest["artifact_sha256"])
    paths.update(path for path in capture.iterdir() if path.is_file())
    paths.update(Path(item[field]) for item in completion["evidence"] for field in ("object", "binary"))
    paths.add(Path(manifest["compiler"]))
    paths.add(Path(__file__).resolve())
    def hashes():
        return {str(path): digest(path) for path in sorted(paths)}
    before = hashes()
    result = dict(schema="mhgp8_wspd_q34_mutant_readback_v1", status="failed",
                  capture=str(capture), started_utc=utc_stamp(), input_sha256_before=before,
                  records=[], error=None, closing_errors=[])
    helper = str(ROOT / "morsehgp3D_v8/tests/wspd_q34_mutations.py")
    try:
        for optimized in (False, True):
            for live in (False, True):
                command = [sys.executable, "-B", *( ["-O"] if optimized else []),
                           helper, "read", str(capture), *( ["--check-live"] if live else [])]
                record = dict(command=command, cwd=str(ROOT), optimized=optimized, live=live,
                              started_utc=utc_stamp(), exit_code=None, stdout="", stderr="",
                              stdout_base64="", stderr_base64="", status="failed")
                try:
                    invoke(command, dict(os.environ), ROOT, record, new_session=True)
                    require(record["exit_code"] == 0 and type(record["exit_code"]) is int,
                            "mutation readback command failed")
                    row = parse_result(record["stdout"].encode())
                    require(row["status"] == "passed" and row["live_checked"] is live,
                            "mutation reader did not confirm intended scope")
                    record["status"] = "passed"
                finally:
                    record["finished_utc"] = utc_stamp()
                    result["records"].append(record)
        for offset in (0, 1):
            require(result["records"][offset]["stdout"] == result["records"][offset+2]["stdout"],
                    "normal and optimized mutation readers disagree")
        result["status"] = "passed"
    except BaseException as cause:
        result["error"] = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        result["input_sha256_after"] = hashes()
        if result["input_sha256_after"] != before:
            result["closing_errors"].append("observed mutation inputs changed")
            result["status"] = "failed"
        result["finished_utc"] = utc_stamp()
        write_json(output, result)
    require(result["status"] == "passed", "mutation readback closure failed")
    print(json.dumps(dict(status=result["status"], path=str(output), commands=len(result["records"]),
                          inputs=len(before), closing_errors=result["closing_errors"])))


if __name__ == "__main__":
    main()
