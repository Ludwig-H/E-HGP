#!/usr/bin/env python3
"""Capture spatial-preparation gates and full-scan preparations, never a benchmark."""
from __future__ import annotations

import argparse
import base64
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile

# Explicit reuse of the already exercised byte-preserving collector, not of
# any geometry verdict. Its only deadline is cancellation escalation.
from run_p0_matrix import invoke as collect_process, on_signal

ROOT = Path(__file__).resolve().parents[2]
PREPARER = ROOT / "morsehgp3D_v8/bench/prepare_lidar_spatial.py"
GATE = ROOT / "morsehgp3D_v8/tests/lidar_spatial_gate.py"
COLLECTOR = ROOT / "morsehgp3D_v8/bench/run_p0_matrix.py"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + "\n",
                    encoding="utf-8")


def expected_preparation_artifacts(destination, source, read_result, before):
    """Bind the whole closed directory to the hashes actually checked by read."""
    if read_result.get("status") != "passed" or read_result.get("datasets") != 7 or \
            read_result.get("path") != str(destination.resolve()):
        raise RuntimeError("preparation reader identity differs")
    manifest_path = destination / "MANIFEST.json"
    manifest_bytes = manifest_path.read_bytes()
    if hashlib.sha256(manifest_bytes).hexdigest() != read_result["manifest_sha256"]:
        raise RuntimeError("manifest changed after successful readers")
    manifest = json.loads(manifest_bytes)
    if manifest["raw"]["path"] != str(source) or \
            manifest["raw"]["sha256"] != before[str(source)] or \
            manifest["script"]["sha256"] != before[str(PREPARER)]:
        raise RuntimeError("preparation source/script differs from capture pins")
    expected = {"MANIFEST.json": read_result["manifest_sha256"],
                "COMPLETION.json": read_result["completion_sha256"]}

    def add(name, value):
        if not isinstance(name, str) or Path(name).name != name or name in expected:
            raise RuntimeError("invalid/duplicate prepared artifact name")
        expected[name] = value

    add(manifest["raw_to_full"]["file"], manifest["raw_to_full"]["sha256"])
    for dataset in manifest["datasets"].values():
        add(dataset["points_file"], dataset["points_sha256"])
        add(dataset["site_ids_file"], dataset["site_ids_sha256"])
    if len(expected) != 17 or {p.name for p in destination.iterdir()} != set(expected):
        raise RuntimeError("prepared artifact inventory differs")
    return expected


def capture(output, inputs):
    inputs = [path.resolve(strict=True) for path in inputs]
    if len(set(inputs)) != len(inputs):
        raise ValueError("duplicate raw scans")
    output.mkdir(parents=True, exist_ok=True)
    target = Path(tempfile.mkdtemp(prefix="spatial_", dir=output.resolve()))
    sources = [PREPARER, GATE, COLLECTOR, Path(__file__).resolve()]
    before = {str(path): digest(path) for path in [*sources, *inputs]}
    state = {"schema": "mhgp8_lidar_spatial_checks_v1", "status": "running",
             "scope": "input_preparation_only_not_engine_or_full",
             "started_utc": datetime.now(timezone.utc).isoformat(),
             "launch": [sys.executable, *sys.argv], "pins_before": before,
             "commands": [], "prepared": [], "closing_errors": [],
             "expected_artifact_sha256": {}}
    environment = dict(os.environ)
    for name, arguments in (("git_head", ["git", "rev-parse", "HEAD"]),
                            ("git_status", ["git", "status", "--porcelain=v1"])):
        state[name] = subprocess.run(arguments, cwd=ROOT, text=True, check=True,
                                     capture_output=True).stdout
    frozen = target / "sources"
    frozen.mkdir()
    for path in sources:
        (frozen / path.name).write_bytes(path.read_bytes())
        state["expected_artifact_sha256"][f"sources/{path.name}"] = before[str(path)]
    write_json(target / "MANIFEST.json", state)
    state["expected_artifact_sha256"]["MANIFEST.json"] = digest(target / "MANIFEST.json")

    def invoke(command):
        number = len(state["commands"])
        entry = {"command": list(map(str, command)), "exit_code": None,
                 "stdout": f"command_{number:03}.stdout",
                 "stderr": f"command_{number:03}.stderr"}
        state["commands"].append(entry)
        write_json(target / "COMPLETION.json", state)
        raw = {"exit_code": None, "stdout_base64": "", "stderr_base64": ""}
        try:
            # One private group per command includes the gate's CLI children.
            # On interruption collection cancels the group and joins its leader
            # before restoring raising handlers; no wall-time cap is imposed.
            collect_process(entry["command"], environment, ROOT, raw, new_session=True)
        finally:
            entry["exit_code"] = raw["exit_code"]
            for stream in ("stdout", "stderr"):
                data = base64.b64decode(raw[stream + "_base64"], validate=True)
                (target / entry[stream]).write_bytes(data)
                entry[stream + "_sha256"] = hashlib.sha256(data).hexdigest()
                state["expected_artifact_sha256"][entry[stream]] = entry[stream + "_sha256"]
            write_json(target / "COMPLETION.json", state)
        if entry["exit_code"]:
            raise RuntimeError(f"command {number} failed with {entry['exit_code']}")
        return (target / entry["stdout"]).read_text(encoding="utf-8")

    handlers = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    try:
        invoke([sys.executable, GATE])
        invoke([sys.executable, "-O", GATE])
        for number, source in enumerate(inputs):
            destination = target / f"scene_{number:02}_{source.stem}"
            invoke([sys.executable, PREPARER, "prepare", "--input", source,
                    "--output", destination])
            first = invoke([sys.executable, PREPARER, "read", "--path", destination])
            second = invoke([sys.executable, "-O", PREPARER, "read", "--path", destination])
            if json.loads(first) != json.loads(second):
                raise RuntimeError("normal/optimized reader results differ")
            expected = expected_preparation_artifacts(destination, source, json.loads(first), before)
            state["expected_artifact_sha256"].update(
                (f"{destination.name}/{name}", value) for name, value in expected.items())
            state["prepared"].append({"path": destination.name,
                                      "source": str(source), "read": json.loads(first),
                                      "artifact_sha256": expected})
        state["status"] = "completed"
    except BaseException as error:
        state["status"] = "failed"
        state["error"] = f"{type(error).__name__}: {error}"
        raise
    finally:
        # Once cancellation has joined the owned child, further signals cannot
        # interrupt the failure receipt or replace its first cause.
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        state["pins_after"] = {}
        for name, original in before.items():
            try:
                after = digest(Path(name))
            except OSError as error:
                state["closing_errors"].append(f"{name}: {error}")
                continue
            state["pins_after"][name] = after
            if original != after:
                state["closing_errors"].append(f"changed pin: {name}")
        state["artifacts"] = {}
        for path in sorted(target.rglob("*")):
            if not path.is_file() or path == target / "COMPLETION.json":
                continue
            name = str(path.relative_to(target))
            try:
                if path.is_symlink():
                    raise ValueError("linked capture artifact")
                state["artifacts"][name] = digest(path)
            except (OSError, ValueError) as error:
                state["closing_errors"].append(f"{name}: {error}")
        expected = state["expected_artifact_sha256"]
        for name, value in expected.items():
            if state["artifacts"].get(name) != value:
                state["closing_errors"].append(f"changed/missing validated artifact: {name}")
        if state["status"] == "completed" and state["artifacts"].keys() != expected.keys():
            state["closing_errors"].append("successful capture has unexpected artifacts")
        if state["closing_errors"]:
            state["status"] = "failed"
        state["finished_utc"] = datetime.now(timezone.utc).isoformat()
        write_json(target / "COMPLETION.json", state)
        print(json.dumps({"path": str(target), "status": state["status"],
                          "commands": len(state["commands"]),
                          "prepared": len(state["prepared"]),
                          "closing_errors": state["closing_errors"]}, sort_keys=True))
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
    if state["status"] != "completed":
        raise RuntimeError("spatial preparation capture did not close")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--inputs", type=Path, nargs="+", required=True)
    args = parser.parse_args()
    capture(args.output, args.inputs)
