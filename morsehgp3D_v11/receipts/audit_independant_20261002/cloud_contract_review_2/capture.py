#!/usr/bin/env python3
import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
LIVE = HERE.parents[2]
CHECKOUT = LIVE.parent
if len(sys.argv) != 2 or sys.argv[1] not in ("before", "after"):
    raise RuntimeError("usage: capture.py before|after")
mode = sys.argv[1]
if mode == "before":
    names = [str(p.relative_to(CHECKOUT)) for prefix in ("src/cloud", "src/io", "tests/cloud", "tests/io")
             for p in sorted((LIVE / prefix).rglob("*")) if p.is_file()]
    names += ["morsehgp3D_v11/" + n for n in ("src/core/buffer.hpp", "src/core/buffer.cpp", "src/core/types.hpp", "src/core/status.hpp", "src/core/reasons.def", "src/core/ledger.hpp", "README.md", "docs/ARCHITECTURE.md", "docs/PROVENANCE.md")]
    names += ["AGENTS.md"]
else:
    names = [item["path"] for item in json.loads((HERE / "SOURCE_BEFORE.json").read_text())["sources"]]
entries = []
for name in names:
    source = CHECKOUT / name
    if not source.is_file():
        entries.append({"path": name, "missing": True})
        continue
    data = source.read_bytes()
    sha = hashlib.sha256(data).hexdigest()
    if mode == "before":
        dest = HERE / "sources" / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(data)
    entries.append({"path": name, "bytes": len(data), "sha256": sha})
head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=CHECKOUT, capture_output=True, text=True, check=True).stdout.strip()
status = subprocess.run(["git", "status", "--short", "--", "morsehgp3D_v11/src/cloud", "morsehgp3D_v11/src/io", "morsehgp3D_v11/tests/cloud", "morsehgp3D_v11/tests/io"], cwd=CHECKOUT, capture_output=True, text=True, check=True).stdout
result = {"time_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "head": head, "git_status_named_scope": status, "sources": entries, "no_execution": True}
if mode == "after":
    before = json.loads((HERE / "SOURCE_BEFORE.json").read_text())
    result["stable"] = before["sources"] == entries
    result["snapshot_intact"] = all(item.get("missing") or hashlib.sha256((HERE / "sources" / item["path"]).read_bytes()).hexdigest() == item["sha256"] for item in before["sources"])
(HERE / ("SOURCE_" + mode.upper() + ".json")).write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n")
print(json.dumps({"files": len(entries), "head": head, "stable": result.get("stable"), "snapshot_intact": result.get("snapshot_intact")}))
