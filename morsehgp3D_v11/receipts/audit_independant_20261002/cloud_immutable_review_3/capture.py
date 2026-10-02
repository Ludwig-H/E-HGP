#!/usr/bin/env python3
import datetime
import hashlib
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
LIVE = Path("/workspaces/E-HGP/build/v11-development-20261002")
COMMIT = "6a22a9118a9dc692d3b5a34b8fd97673746697c4"
PREFIXES = ["morsehgp3D_v11/src/cloud", "morsehgp3D_v11/src/core", "morsehgp3D_v11/tests/cloud"]
EXTRA = ["AGENTS.md", "morsehgp3D_v11/README.md", "morsehgp3D_v11/docs/ARCHITECTURE.md", "morsehgp3D_v11/docs/PROVENANCE.md", "morsehgp3D_v11/docs/CONCEPTION_MOTEUR.md", "morsehgp3D_v11/docs/DEVELOPPEMENT.md", "morsehgp3D_v11/audits/REPONSE_CLAUDE_OUVERTURE_ET_FONDATIONS_20261002.md"]
names = subprocess.run(["git", "ls-tree", "-r", "--name-only", COMMIT, "--"] + PREFIXES, cwd=LIVE, capture_output=True, text=True, check=True).stdout.splitlines() + EXTRA
entries = []
for name in sorted(set(names)):
    data = subprocess.run(["git", "show", COMMIT + ":" + name], cwd=LIVE, capture_output=True, check=True).stdout
    path = HERE / "sources" / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    live = LIVE / name
    entries.append({"path": name, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest(),
                    "live_sha256_before": hashlib.sha256(live.read_bytes()).hexdigest() if live.is_file() else None})
result = {"time_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "commit": COMMIT,
          "live_head": subprocess.run(["git", "rev-parse", "HEAD"], cwd=LIVE, capture_output=True, text=True, check=True).stdout.strip(),
          "source_kind": "git_commit", "sources": entries, "product_execution": False}
(HERE / "SOURCE_BEFORE.json").write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps({"source_files": len(entries), "commit": COMMIT, "live_identical": all(i["sha256"] == i["live_sha256_before"] for i in entries)}))
