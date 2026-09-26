#!/usr/bin/env python3
"""Capture the offline S1/S2 protocol suite; never invokes real GCP."""
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT, text=True).strip()


def hashes():
    paths = git("ls-files", "gcp-migration").splitlines()
    return {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
            for name in paths if (ROOT / name).is_file()}


def main():
    output = HERE / "receipt.json"
    if output.exists() or any((HERE / (mode + ".stdout")).exists()
                             for mode in ("normal", "optimized")):
        raise SystemExit("refusing to overwrite a prior protocol capture")
    before = hashes()
    record = {"schema": "mhgp9_audit_b_local_protocol_replay_v1",
              "started_utc": datetime.now(timezone.utc).isoformat(),
              "commit": git("rev-parse", "HEAD"),
              "protocol_source_hashes_before": before,
              "python": sys.version, "GCP_used": False,
              "scope": "offline fake-cloud protocol, not CUDA geometry or performance",
              "runs": []}
    for mode, flags in (("normal", ["-B"]), ("optimized", ["-B", "-O"])):
        command = [sys.executable, *flags, "gcp-migration/gpu_filter_selftest_v9.py", "-v"]
        started = time.monotonic()
        with (HERE / (mode + ".stdout")).open("xb") as stdout, \
                (HERE / (mode + ".stderr")).open("xb") as stderr:
            result = subprocess.run(command, cwd=ROOT, stdout=stdout, stderr=stderr, check=False)
        elapsed = time.monotonic() - started
        error = (HERE / (mode + ".stderr")).read_text()
        tests = re.search(r"Ran (\d+) tests? in ", error)
        row = {"mode": mode, "command": command, "exit_code": result.returncode,
               "wall_seconds": elapsed, "test_count": int(tests.group(1)) if tests else None,
               "passed": result.returncode == 0 and tests is not None
               and int(tests.group(1)) == 13 and error.rstrip().endswith("OK")}
        record["runs"].append(row)
        output.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n")
        print(json.dumps(row), flush=True)
    record["protocol_source_hashes_after"] = hashes()
    record["sources_unchanged"] = before == record["protocol_source_hashes_after"]
    record["finished_utc"] = datetime.now(timezone.utc).isoformat()
    record["passed"] = record["sources_unchanged"] and all(row["passed"] for row in record["runs"])
    output.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n")
    return 0 if record["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
