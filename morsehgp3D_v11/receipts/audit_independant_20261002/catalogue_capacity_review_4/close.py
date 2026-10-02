#!/usr/bin/env python3
import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
LIVE = HERE.parents[3]
if (HERE / "SHA256SUMS").exists():
    raise RuntimeError("already closed")
before = json.loads((HERE / "SOURCE_BEFORE.json").read_text())
items = []
for item in before["sources"]:
    name = item["path"]
    snapshot_hash = hashlib.sha256((HERE / "sources" / name).read_bytes()).hexdigest()
    if snapshot_hash != item["sha256"]:
        raise RuntimeError("snapshot drift")
    source = LIVE / name
    items.append({"path": name, "snapshot_sha256": snapshot_hash,
                  "live_sha256_after": hashlib.sha256(source.read_bytes()).hexdigest() if source.is_file() else None})
(HERE / "SOURCE_AFTER.json").write_text(json.dumps({"time_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
    "reviewed_commit": before["commit"], "snapshot_intact": True,
    "live_head": subprocess.run(["git", "rev-parse", "HEAD"], cwd=LIVE, capture_output=True, text=True, check=True).stdout.strip(),
    "live_matches_snapshot": all(x["snapshot_sha256"] == x["live_sha256_after"] for x in items), "sources": items}, indent=2) + "\n")
commands = []
for name, flags in (("judge_normal", []), ("judge_optimized", ["-O"])):
    argv = [sys.executable, "-B"] + flags + [str(HERE / "judge.py")]
    result = subprocess.run(argv, cwd=HERE, capture_output=True)
    (HERE / (name + ".stdout.json")).write_bytes(result.stdout)
    (HERE / (name + ".stderr.txt")).write_bytes(result.stderr)
    commands.append({"argv": argv, "exit_code": result.returncode,
                     "stdout_sha256": hashlib.sha256(result.stdout).hexdigest(), "stderr_sha256": hashlib.sha256(result.stderr).hexdigest()})
    if result.returncode:
        raise RuntimeError("reader failure")
if (HERE / "judge_normal.stdout.json").read_bytes() != (HERE / "judge_optimized.stdout.json").read_bytes():
    raise RuntimeError("normal/-O mismatch")
(HERE / "CLOSURE.json").write_text(json.dumps({"status": "CLOSED", "snapshot_intact": True, "commands": commands,
    "normal_optimized_identical": True, "product_execution": False, "new_GCP_session": False,
    "native_qualification": False}, indent=2) + "\n")
files = sorted(p for p in HERE.rglob("*") if p.is_file() and p.name != "SHA256SUMS")
manifest = "".join(hashlib.sha256(p.read_bytes()).hexdigest() + "  " + str(p.relative_to(HERE)) + "\n" for p in files)
(HERE / "SHA256SUMS").write_text(manifest)
print(json.dumps({"files": len(files), "manifest_sha256": hashlib.sha256(manifest.encode()).hexdigest()}))
