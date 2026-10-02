#!/usr/bin/env python3
"""Hash reader only, no native/product execution."""
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
before = json.loads((HERE / "SOURCE_BEFORE.json").read_text())
after = json.loads((HERE / "SOURCE_AFTER.json").read_text())
checks = 0
def require(ok, message):
    global checks
    checks += 1
    if not ok:
        raise RuntimeError(message)
require(before["commit"] == "f391bf13e1a9a982025bde86fc9219b5b7430afc", "scope")
require(len(before["sources"]) == 48 and after["snapshot_intact"], "capture closure")
for item in before["sources"]:
    require(hashlib.sha256((HERE / "sources" / item["path"]).read_bytes()).hexdigest() == item["sha256"], item["path"])
manifest = HERE / "SHA256SUMS"
if manifest.exists():
    for line in manifest.read_text().splitlines():
        sha, name = line.split("  ", 1)
        if hashlib.sha256((HERE / name).read_bytes()).hexdigest() != sha:
            raise RuntimeError("artifact hash " + name)
print(json.dumps({"status": "PASS", "checks": checks, "scope": "Frozen static capacity audit only; no native/product/GCP execution"}, sort_keys=True))
