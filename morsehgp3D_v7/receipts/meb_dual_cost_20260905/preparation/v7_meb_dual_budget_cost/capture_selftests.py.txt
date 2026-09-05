#!/usr/bin/env python3
"""Capture only the two literal Python selftests; never compiles/runs C++."""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

BASE = Path(__file__).resolve().parent
OUTPUT = BASE / "protocol_selftests_20260905"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, value):
    with path.open("x", encoding="utf-8") as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write("\n")


def main():
    if OUTPUT.exists():
        raise RuntimeError("create_only_selftest_destination")
    os.sched_setaffinity(0, {0})
    if os.sched_getaffinity(0) != {0}:
        raise RuntimeError("CPU0_required")
    paths = [BASE / name for name in ("run_cost.py", "cost_harness.cpp", "selftest.py", "README.md", "capture_selftests.py")]
    before = {path.name: sha(path) for path in paths}
    OUTPUT.mkdir(exist_ok=False)
    started = datetime.now(timezone.utc).isoformat()
    records = []
    for label, flags in (("normal", []), ("optimized", ["-O"])):
        argv = ["python3", "-B", *flags, str(BASE / "selftest.py")]
        tick = time.monotonic()
        rc, error = None, None
        with (OUTPUT / (label + ".stdout")).open("xb") as out, (OUTPUT / (label + ".stderr")).open("xb") as err:
            try:
                rc = subprocess.run(argv, cwd=BASE.parents[1], stdout=out, stderr=err, timeout=30, check=False).returncode
            except subprocess.TimeoutExpired as exception:
                error = str(exception)
        record = dict(label=label, argv=argv, returncode=rc, error=error, elapsed_seconds=time.monotonic()-tick,
                      stdout_sha256=sha(OUTPUT / (label + ".stdout")), stderr_sha256=sha(OUTPUT / (label + ".stderr")))
        write(OUTPUT / (label + ".json"), record)
        records.append(record)
    after = {path.name: sha(path) for path in paths}
    status = "passed" if before == after and all(row["returncode"] == 0 and row["error"] is None for row in records) else "failed"
    artifacts = {path.name: dict(sha256=sha(path), bytes=path.stat().st_size) for path in sorted(OUTPUT.iterdir())}
    write(OUTPUT / "summary.json", dict(status=status, started_at=started, finished_at=datetime.now(timezone.utc).isoformat(),
          source_before=before, source_after=after, records=records, artifacts=artifacts, cpu_affinity=[0],
          scope="Python_synthetic_only_no_Cpp_build_or_measurement", gcp_used=False, public_status="not_claimed"))
    print(json.dumps(dict(status=status, summary=str(OUTPUT / "summary.json"), sha256=sha(OUTPUT / "summary.json"))))
    return 0 if status == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
