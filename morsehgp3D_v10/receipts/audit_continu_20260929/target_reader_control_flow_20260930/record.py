"""Capture two tiny AST-only probes; print receipt, do not write archives or execute native code."""
import datetime as dt
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent
NAMES = ("source/run_target.py", "source/valide_lib.py", "micro.py", "record.py")


def pins():
    return {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in NAMES}


before = pins()
commands = []
for mode in ("normal", "optimized"):
    argv = [sys.executable, "-B"] + (["-O"] if mode == "optimized" else []) + [str(ROOT / "micro.py")]
    start_utc = dt.datetime.now(dt.timezone.utc).isoformat()
    start_ns = time.monotonic_ns()
    out = subprocess.run(argv, cwd=ROOT, text=True, capture_output=True, check=False, timeout=10)
    end_ns = time.monotonic_ns()
    end_utc = dt.datetime.now(dt.timezone.utc).isoformat()
    commands.append({
        "name": mode, "argv": argv, "code": out.returncode, "stdout": out.stdout, "stderr": out.stderr,
        "start_utc": start_utc, "end_utc": end_utc, "start_ns": start_ns, "end_ns": end_ns,
    })
print(json.dumps({
    "schema": "mhgp10.target_reader_control_flow.v1",
    "scope": "AST-selected copied Python functions with explicit unit stubs, not native/geometry/statistics",
    "sources_before": before, "sources_after": pins(), "commands": commands,
    "status": "CAPTURED", "native_invocations": 0, "Gamma_invocations": 0, "GCP_used": False,
}, sort_keys=True, indent=2))
