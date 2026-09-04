#!/usr/bin/env python3
"""Replay the pinned CPU audit using permanent inputs and an isolated audit copy."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise RuntimeError(reason)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--prepare-only", action="store_true",
        help="verify permanent inputs, create source copies and apply instrumentation only",
    )
    args = parser.parse_args()
    receipts = Path(__file__).resolve().parent
    lineage = receipts.parent.parent
    assets = json.loads((receipts / "memory_budget_replay_assets.json").read_text())
    for name, expected in assets["permanent_inputs_sha256"].items():
        require(digest(receipts / name) == expected, f"changed permanent input: {name}")
    receipt = json.loads((receipts / "memory_budget_current.json").read_text())
    pinned = receipt["source_snapshot"]["copy"]
    for relative, expected in pinned.items():
        require(digest(lineage / relative) == expected, f"source no longer at audited hash: {relative}")

    workspace = lineage / "audits" / ".work_residence2"
    workspace.mkdir(exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix="replay_", dir=workspace))
    for directory in ("source", "instrumented"):
        for relative, expected in pinned.items():
            destination = scratch / directory / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(lineage / relative, destination)
            require(digest(destination) == expected, f"source changed while copying: {relative}")
    for relative, expected in pinned.items():
        require(digest(lineage / relative) == expected, f"source changed after copying: {relative}")
    patch_command = [
        "patch", "--batch", "--forward", "-p1", "-d", str(scratch / "instrumented"),
        "-i", str(receipts / "memory_budget_instrumentation.patch"),
    ]
    patch_result = subprocess.run(patch_command, text=True, capture_output=True, check=False)
    require(patch_result.returncode == 0, f"instrumentation failed: {patch_result.stdout}{patch_result.stderr}")
    expected_instrumented = dict(pinned)
    expected_instrumented["src/pipeline/expand.hpp"] = receipt["instrumentation"]["instrumented_expand_sha256"]
    for relative, expected in expected_instrumented.items():
        require(digest(scratch / "instrumented" / relative) == expected, f"unexpected instrumentation: {relative}")
    for source_name, destination_name in (
        ("memory_budget_gate.cpp", "budget_gate.cpp"),
        ("memory_budget_proxy_probe.cpp", "proxy_probe.cpp"),
    ):
        shutil.copyfile(receipts / source_name, scratch / destination_name)
    (scratch / "tmp").mkdir()
    environment = dict(os.environ, TMPDIR=str(scratch / "tmp"), PYTHONDONTWRITEBYTECODE="1")
    result = {
        "status": "prepared", "scratch": str(scratch),
        "source_files_verified": len(pinned), "patch_command": patch_command,
        "patch_stdout": patch_result.stdout, "patch_stderr": patch_result.stderr,
        "runs": [],
    }
    if not args.prepare_only:
        for name in ("proxy_probe", "budget_gate"):
            command = [
                "c++", "-std=c++20", "-O2", "-DNDEBUG", "-Wall", "-Wextra", "-Wpedantic",
                "-Werror", "-pthread", str(scratch / f"{name}.cpp"), "-o", str(scratch / name),
            ]
            compiled = subprocess.run(command, env=environment, text=True, capture_output=True, check=False)
            result["runs"].append({"command": command, "returncode": compiled.returncode,
                                   "stdout": compiled.stdout, "stderr": compiled.stderr})
            if compiled.returncode:
                result["status"] = "compile_failed"
                break
            executed = subprocess.run([str(scratch / name)], env=environment, text=True,
                                      capture_output=True, timeout=30, check=False)
            result["runs"].append({"command": [str(scratch / name)], "returncode": executed.returncode,
                                   "stdout": executed.stdout, "stderr": executed.stderr})
            if executed.returncode:
                result["status"] = "failed"
                break
        else:
            result["status"] = "completed"
    (scratch / "replay_result.json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if result["status"] in ("prepared", "completed") else 1


if __name__ == "__main__":
    raise SystemExit(main())
