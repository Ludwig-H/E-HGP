#!/usr/bin/env python3
"""Four posthoc readers; no native execution and no mutation of compiled evidence."""
from pathlib import Path
import base64
import json
import subprocess
import sys

HERE = Path(__file__).resolve().parent
V8 = HERE.parents[2]
sys.path.insert(0, str(V8 / "tests"))
import float32_ball_mutations as gate


def main():
    capture = HERE / "compiled_r2"
    target = HERE / "MUTANTS_R2_READBACK.json"
    gate.require(not target.exists(), "readback output must be fresh")
    manifest = gate.load(capture / "MANIFEST.json")
    completion = gate.load(capture / "COMPLETION.json")
    files = {Path(__file__).resolve(), Path(sys.executable).resolve(),
             *(p for p in capture.rglob("*") if p.is_file()),
             *(Path(p) for p in manifest["source_sha256"]),
             *(Path(p) for p in completion["available_build_artifacts"])}
    for detail in completion["compiled"].values():
        files.update(map(Path, detail["dependencies_closed"]))
        files.update(map(Path, detail["artifacts_closed"]))
    before = gate.pins(files)
    result = dict(schema="mhgp8_float32_ball_mutations_readback_v1", status="failed",
                  started_utc=gate.stamp(), inputs_before=before, commands=[],
                  native_reexecuted=False, closing_errors=[])
    try:
        for optimized in (False, True):
            for live in (False, True):
                command = [sys.executable, "-B", *( ["-O"] if optimized else []),
                           str(V8 / "tests/float32_ball_mutations.py"), "read", "--path", str(capture),
                           *( ["--check-live"] if live else [])]
                row = dict(command=command, cwd=str(V8.parent), environment=gate.ENVIRONMENT,
                           started_utc=gate.stamp(), optimized=optimized, check_live=live)
                process = subprocess.run(command, cwd=V8.parent, env=gate.ENVIRONMENT, capture_output=True)
                row.update(exit_code=process.returncode, finished_utc=gate.stamp(),
                           stdout=process.stdout.decode(), stderr=process.stderr.decode(),
                           stdout_base64=base64.b64encode(process.stdout).decode(),
                           stderr_base64=base64.b64encode(process.stderr).decode())
                result["commands"].append(row)
                gate.require(type(process.returncode) is int and process.returncode == 0 and not process.stderr,
                             "posthoc reader failed")
                row["result"] = gate.oracle.strict_json(row["stdout"])
                gate.require(row["result"]["status"] == "passed" and
                             row["result"]["check_live"] is live and
                             row["result"]["native_reexecuted"] is False,
                             "posthoc reader result differs")
        normalized = [{k: v for k, v in row["result"].items() if k != "check_live"}
                      for row in result["commands"]]
        gate.require(all(row == normalized[0] for row in normalized), "normal/optimized/live readers differ")
        result["status"] = "passed"
    except BaseException as error:
        result["error"] = f"{type(error).__name__}: {error}"
    finally:
        try:
            result["inputs_after"] = gate.pins(files)
            gate.require(result["inputs_after"] == before, "inputs changed during readback")
        except BaseException as error:
            result["closing_errors"].append(f"{type(error).__name__}: {error}")
            result["status"] = "failed"
        result["finished_utc"] = gate.stamp()
        gate.write_json(target, result)
    gate.require(result["status"] == "passed", "readback failed; evidence preserved")
    print(json.dumps(dict(status="passed", commands=len(result["commands"]),
                          inputs=len(before), sha256=gate.digest(target), path=str(target)), sort_keys=True))


if __name__ == "__main__":
    main()
