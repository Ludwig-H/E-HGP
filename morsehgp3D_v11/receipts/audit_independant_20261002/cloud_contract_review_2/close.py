#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
if (HERE / "SHA256SUMS").exists():
    raise RuntimeError("already closed")
before = json.loads((HERE / "SOURCE_BEFORE.json").read_text())
after = json.loads((HERE / "SOURCE_AFTER.json").read_text())
if not after["stable"] or not after["snapshot_intact"] or before["sources"] != after["sources"]:
    raise RuntimeError("source drift; state must be explained before closure")
for item in before["sources"]:
    if item.get("missing"):
        raise RuntimeError("source missing")
    if hashlib.sha256((HERE / "sources" / item["path"]).read_bytes()).hexdigest() != item["sha256"]:
        raise RuntimeError("snapshot hash")
supplement = json.loads((HERE / "DEPENDENCY_SUPPLEMENT.json").read_text())
source_name = supplement["path"]
supplement_hash = hashlib.sha256((HERE / "sources" / source_name).read_bytes()).hexdigest()
supplement_live_hash = hashlib.sha256((HERE.parents[3] / source_name).read_bytes()).hexdigest()
if supplement["sha256"] != supplement_hash or supplement_hash != supplement_live_hash:
    raise RuntimeError("dependency drift")
commands = []
for name, flags in (("capacity_normal", []), ("capacity_optimized", ["-O"])):
    argv = [sys.executable, "-B"] + flags + [str(HERE / "capacity.py")]
    result = subprocess.run(argv, cwd=HERE, capture_output=True)
    (HERE / (name + ".json")).write_bytes(result.stdout)
    (HERE / (name + ".stderr.txt")).write_bytes(result.stderr)
    commands.append({"argv": argv, "exit_code": result.returncode,
                     "stdout_sha256": hashlib.sha256(result.stdout).hexdigest(),
                     "stderr_sha256": hashlib.sha256(result.stderr).hexdigest()})
    if result.returncode != 0:
        raise RuntimeError("scalar calculation failed")
if (HERE / "capacity_normal.json").read_bytes() != (HERE / "capacity_optimized.json").read_bytes():
    raise RuntimeError("normal/-O outputs differ")
(HERE / "CLOSURE.json").write_text(json.dumps({"status": "CLOSED", "source_files": len(before["sources"]) + 1,
    "sources_stable": True, "snapshot_intact": True, "supplement_stable": True,
    "product_execution": False, "static_audit_only": True, "scalar_commands": commands,
    "normal_optimized_identical": True, "no_global_qualification": True}, indent=2) + "\n")
files = sorted(p for p in HERE.rglob("*") if p.is_file() and p.name != "SHA256SUMS")
manifest = "".join(hashlib.sha256(p.read_bytes()).hexdigest() + "  " + str(p.relative_to(HERE)) + "\n" for p in files)
(HERE / "SHA256SUMS").write_text(manifest)
print(json.dumps({"files": len(files), "manifest_sha256": hashlib.sha256(manifest.encode()).hexdigest()}))
