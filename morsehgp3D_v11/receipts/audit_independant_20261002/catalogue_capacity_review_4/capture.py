#!/usr/bin/env python3
import datetime
import hashlib
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
LIVE = HERE.parents[3]
COMMIT = "f391bf13e1a9a982025bde86fc9219b5b7430afc"
prefixes = ["morsehgp3D_v11/src/catalogue", "morsehgp3D_v11/src/cloud", "morsehgp3D_v11/src/core", "morsehgp3D_v11/src/num", "morsehgp3D_v11/tests/catalogue"]
names = subprocess.run(["git", "ls-tree", "-r", "--name-only", COMMIT, "--"] + prefixes, cwd=LIVE, capture_output=True, text=True, check=True).stdout.splitlines()
names += ["AGENTS.md", "morsehgp3D_v11/docs/ARCHITECTURE.md", "morsehgp3D_v11/docs/CONCEPTION_MOTEUR.md", "morsehgp3D_v11/tests/support/test.hpp"]
items = []
for name in sorted(set(names)):
    data = subprocess.run(["git", "show", COMMIT + ":" + name], cwd=LIVE, capture_output=True, check=True).stdout
    path = HERE / "sources" / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    src = LIVE / name
    items.append({"path": name, "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest(),
                  "live_sha256_before": hashlib.sha256(src.read_bytes()).hexdigest() if src.is_file() else None})
result = {"time_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "commit": COMMIT, "source_kind": "git_commit", "sources": items,
          "product_execution": False, "live_head": subprocess.run(["git", "rev-parse", "HEAD"], cwd=LIVE, capture_output=True, text=True, check=True).stdout.strip()}
(HERE / "SOURCE_BEFORE.json").write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps({"files": len(items), "commit": COMMIT, "live_matches_commit": all(i["sha256"] == i["live_sha256_before"] for i in items)}))
