#!/usr/bin/env python3
import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
LIVE = Path("/workspaces/E-HGP/build/v11-development-20261002")
if (HERE / "SHA256SUMS").exists():
    raise RuntimeError("receipt already closed")
before = json.loads((HERE / "SOURCE_BEFORE.json").read_text())
supplement = json.loads((HERE / "SUPPLEMENT_BEFORE.json").read_text())
items = before["sources"] + supplement["sources"]
after = []
for item in items:
    name = item["path"]
    snapshot_hash = hashlib.sha256((HERE / "sources" / name).read_bytes()).hexdigest()
    if snapshot_hash != item["sha256"]:
        raise RuntimeError("snapshot mutated")
    live = LIVE / name
    after.append({"path": name, "snapshot_sha256": snapshot_hash,
                  "live_sha256_after": hashlib.sha256(live.read_bytes()).hexdigest() if live.is_file() else None})
(HERE / "SOURCE_AFTER.json").write_text(json.dumps({"time_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
    "reviewed_commit": before["commit"], "snapshot_intact": True,
    "live_head": subprocess.run(["git", "rev-parse", "HEAD"], cwd=LIVE, capture_output=True, text=True, check=True).stdout.strip(),
    "live_matches_snapshot": all(x["snapshot_sha256"] == x["live_sha256_after"] for x in after), "sources": after}, indent=2) + "\n")
commands = []
def run(name, argv, expected):
    result = subprocess.run(argv, cwd=HERE, capture_output=True)
    (HERE / (name + ".stdout.txt")).write_bytes(result.stdout)
    (HERE / (name + ".stderr.txt")).write_bytes(result.stderr)
    commands.append({"name": name, "argv": argv, "exit_code": result.returncode, "expected": expected,
                     "stdout_sha256": hashlib.sha256(result.stdout).hexdigest(), "stderr_sha256": hashlib.sha256(result.stderr).hexdigest()})
    if result.returncode != expected:
        raise RuntimeError("reader outcome " + name)
    return result
run("inspect_initial_failure", [sys.executable, "-B", str(HERE / "inspect_g4_initial.py")], 1)
run("inspect_corrected", [sys.executable, "-B", str(HERE / "inspect_g4.py")], 0)
diff = run("qualified_scope_diff", ["git", "-C", str(LIVE), "diff", "--name-only", "a971806679a1c68519249bb28c5dac9533a43f59", before["commit"], "--", "morsehgp3D_v11/src/cloud", "morsehgp3D_v11/src/core", "morsehgp3D_v11/tests/cloud", "morsehgp3D_v11/tests/support", "morsehgp3D_v11/cmake"], 0)
if diff.stdout:
    raise RuntimeError("qualified project scope changed")
normal = run("judge_normal", [sys.executable, "-B", str(HERE / "judge.py")], 0)
optimized = run("judge_optimized", [sys.executable, "-B", "-O", str(HERE / "judge.py")], 0)
if normal.stdout != optimized.stdout:
    raise RuntimeError("normal/-O differ")
(HERE / "CLOSURE.json").write_text(json.dumps({"status": "CLOSED", "source_kind": "git_commit", "snapshot_intact": True,
    "normal_optimized_identical": True, "commands": commands, "product_execution": False, "new_GCP_session": False,
    "no_FULL_performance_qualification": True}, indent=2) + "\n")
files = sorted(p for p in HERE.rglob("*") if p.is_file() and p.name != "SHA256SUMS")
manifest = "".join(hashlib.sha256(p.read_bytes()).hexdigest() + "  " + str(p.relative_to(HERE)) + "\n" for p in files)
(HERE / "SHA256SUMS").write_text(manifest)
print(json.dumps({"files": len(files), "manifest_sha256": hashlib.sha256(manifest.encode()).hexdigest()}))
