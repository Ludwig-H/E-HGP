"""Rebuild the isolated adapter gate in a fresh audit-owned directory."""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import time


HERE = Path(__file__).resolve().parent


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--name", required=True, help="Fresh output basename in this packet")
    args = parser.parse_args()
    if not args.name.isidentifier():
        raise ValueError("output name must be an identifier")
    work = HERE / (".work_" + args.name)
    work.mkdir(exist_ok=False)
    with tarfile.open(HERE / "source_snapshot.tar.gz", "r:gz") as archive:
        for member in archive.getmembers():
            path = work / member.name
            if not member.isfile() or not path.resolve().is_relative_to(work):
                raise ValueError("invalid source member")
            data = archive.extractfile(member)
            if data is None:
                raise ValueError("missing source member")
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data.read())
    fixed_adapter = (work / "fixed_adapter.hpp").read_bytes()
    shutil.copytree(work / "original", work / "fixed")
    (work / "fixed/batch_adapter.hpp").write_bytes(fixed_adapter)
    pins = json.loads((HERE / "source_pins.json").read_text())
    for name, expected in pins["files"].items():
        if sha(work / "original" / name) != expected:
            raise ValueError("source pin: " + name)
    if sha(work / "fixed/batch_adapter.hpp") != pins["fixed_adapter"]["sha256"]:
        raise ValueError("fixed adapter pin")
    source_files = [p for p in work.rglob("*") if p.is_file()]
    before = {str(p.relative_to(work)): sha(p) for p in source_files}
    report = {"schema": "mhgp7-adapter-work-boundary-v1", "runs": [],
              "source_archive_sha256": sha(HERE / "source_snapshot.tar.gz"),
              "gate_sha256": sha(HERE / "adapter_gate.cpp"),
              "compiler": subprocess.check_output(["g++", "--version"], text=True),
              "sources_before": before, "geometry_executed": False,
              "device_executed": False, "gcp_used": False}
    output = HERE / (args.name + ".json")
    if output.exists():
        raise ValueError("output already exists")
    good = True
    for mode in ("O2", "SAN"):
        for variant in ("original", "fixed"):
            binary = work / (mode + "_" + variant)
            flags = ["-O2"] if mode == "O2" else [
                "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
            compile_command = ["g++", "-std=c++20", *flags, "-Wall", "-Wextra", "-Wpedantic",
                               "-Werror", "-pthread", "-I", str(work / variant),
                               str(HERE / "adapter_gate.cpp"), "-o", str(binary)]
            for kind, command, expected in [("compile", compile_command, 0),
                                             ("run", [str(binary), "--selftest"],
                                              1 if variant == "original" else 0)]:
                env = dict(os.environ)
                if mode == "SAN":
                    env.update(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
                start = time.time_ns()
                result = subprocess.run(command, capture_output=True, text=True, env=env,
                                        timeout=180, check=False)
                row = {"mode": mode, "variant": variant, "kind": kind, "argv": command,
                       "started_ns": start, "ended_ns": time.time_ns(),
                       "expected_exit_code": expected, "exit_code": result.returncode,
                       "stdout": result.stdout, "stderr": result.stderr}
                if mode == "SAN":
                    row["sanitizer_env"] = {k: env[k] for k in ("ASAN_OPTIONS", "UBSAN_OPTIONS")}
                if kind == "run":
                    row["binary_sha256"] = sha(binary)
                report["runs"].append(row)
                output.write_text(json.dumps(report, indent=2) + "\n")
                good &= result.returncode == expected and not result.stderr
                if kind == "compile" and result.returncode:
                    break
    report["sources_after"] = {str(p.relative_to(work)): sha(p) for p in source_files}
    good &= report["sources_after"] == before and len(report["runs"]) == 8
    report["status"] = "passed_expected_original_failure_and_fix" if good else "failed"
    output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"status": report["status"], "commands": len(report["runs"]),
                      "sources_stable": report["sources_after"] == before}))
    return 0 if good else 1


if __name__ == "__main__":
    raise SystemExit(main())
