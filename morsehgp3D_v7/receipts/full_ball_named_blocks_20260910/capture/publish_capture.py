#!/usr/bin/env python3
"""Copy an explicitly hashed sealed private packet; no Git/cloud operations."""
import hashlib
import json
from pathlib import Path
import shutil
import sys

source, destination = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()
expected_manifest = sys.argv[3]
manifest_file = source / "manifest.json"
if hashlib.sha256(manifest_file.read_bytes()).hexdigest() != expected_manifest:
    raise SystemExit("source manifest pin mismatch")
manifest = json.loads(manifest_file.read_text())
for name, digest in manifest.items():
    relative = Path(name)
    if relative.is_absolute() or ".." in relative.parts:
        raise SystemExit("unsafe manifest path")
    if hashlib.sha256((source / relative).read_bytes()).hexdigest() != digest:
        raise SystemExit("source file pin mismatch: " + name)
destination.mkdir(parents=True,exist_ok=False)
capture = destination / "capture"
capture.mkdir()
total = 0
for name in [*manifest, "manifest.json"]:
    target = capture / name
    target.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(source / name,target)
    total += target.stat().st_size
print(json.dumps({"destination":str(destination),"files":len(manifest)+1,"bytes":total,"manifest_sha256":expected_manifest}))
