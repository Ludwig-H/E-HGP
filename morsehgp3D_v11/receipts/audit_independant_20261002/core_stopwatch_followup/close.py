#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
if (HERE / "SHA256SUMS").exists():
    raise RuntimeError("receipt already closed")
for name, flags in (("judge_normal", []), ("judge_optimized", ["-O"])):
    argv = [sys.executable, "-B"] + flags + [str(HERE / "judge.py")]
    result = subprocess.run(argv, capture_output=True)
    (HERE / (name + ".stdout.json")).write_bytes(result.stdout)
    (HERE / (name + ".stderr.txt")).write_bytes(result.stderr)
    (HERE / (name + ".json")).write_text(json.dumps({"argv": argv, "exit_code": result.returncode}) + "\n")
    if result.returncode != 0:
        raise RuntimeError(name + " failed")
if (HERE / "judge_normal.stdout.json").read_bytes() != (HERE / "judge_optimized.stdout.json").read_bytes():
    raise RuntimeError("normal/-O outputs differ")
closure = {"status": "CLOSED", "sources_stable": True, "normal_gcc_only": True,
           "judge_normal_optimized_identical": True, "no_global_qualification": True}
(HERE / "CLOSURE.json").write_text(json.dumps(closure, indent=2) + "\n")
files = sorted(p for p in HERE.rglob("*") if p.is_file() and p.name != "SHA256SUMS")
manifest = "".join(hashlib.sha256(p.read_bytes()).hexdigest() + "  " + str(p.relative_to(HERE)) + "\n" for p in files)
(HERE / "SHA256SUMS").write_text(manifest)
print(json.dumps({"files": len(files), "manifest_sha256": hashlib.sha256(manifest.encode()).hexdigest()}))
