#!/usr/bin/env python3
"""Verify immutable objects, then run the unchanged private read-only gate."""
import hashlib
import json
from pathlib import Path
import runpy
import tempfile

BASE = Path(__file__).resolve().parent
PIN = "27bcde2d6a58cd8c3eb892c165f4455f3c0ab920ea8921afb3a308d2a685aa5d"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


manifest_path = BASE / "logical_manifest.json"
if sha(manifest_path) != PIN:
    raise RuntimeError("rightmost logical manifest pin mismatch")
manifest = json.loads(manifest_path.read_text())
expected_objects = {digest + ".source" for digest in manifest.values()}
if {path.name for path in (BASE / "objects").iterdir() if path.is_file()} != expected_objects:
    raise RuntimeError("rightmost object inventory mismatch")
for name, digest in manifest.items():
    relative = Path(name)
    if relative.is_absolute() or ".." in relative.parts or relative.suffix == ".bin":
        raise RuntimeError("unsafe or binary rightmost artifact")
    if sha(BASE / "objects" / (digest + ".source")) != digest:
        raise RuntimeError("rightmost object pin mismatch: " + name)
for name, stored in (("README.md", "report.md.source"), ("rightmost.patch", "rightmost.patch"), ("summary.json", "summary.json")):
    if sha(BASE / stored) != manifest[name]:
        raise RuntimeError("rightmost readable artifact pin mismatch: " + stored)
# Content-addressed storage preserves the original names and every captured
# byte, but avoids duplicate source trees and treating historical Markdown
# as active navigation. Only verified artifacts are restored temporarily.
with tempfile.TemporaryDirectory(prefix="mhgp7_rightmost_receipt_") as spelling:
    logical = Path(spelling)
    for name, digest in manifest.items():
        destination = logical / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes((BASE / "objects" / (digest + ".source")).read_bytes())
    (logical / "manifest.json").write_bytes(manifest_path.read_bytes())
    runpy.run_path(str(logical / "verify.py"), run_name="__main__")
