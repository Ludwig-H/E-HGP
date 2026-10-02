#!/usr/bin/env python3
"""Offline integrity reader; no build, subprocess, network, or live source."""
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent


def require(condition, message):
    if not condition:
        raise SystemExit(message)


def verify(root, row):
    relative = Path(row["path"])
    require(not relative.is_absolute() and ".." not in relative.parts,
            "unsafe manifest path")
    path = root / relative
    require(path.is_file() and not path.is_symlink(), f"missing file: {relative}")
    data = path.read_bytes()
    require(len(data) == row["bytes"], f"size mismatch: {relative}")
    require(hashlib.sha256(data).hexdigest() == row["sha256"],
            f"hash mismatch: {relative}")


def main():
    manifest = json.loads((ROOT / "closure_manifest.json").read_text())
    rows = manifest["files"]
    require(len(rows) >= 100, "unexpectedly small capture")
    require(len({row["path"] for row in rows}) == len(rows), "duplicate path")
    for row in rows:
        verify(ROOT, row)
    snapshot = json.loads((ROOT / "snapshot.json").read_text())
    require(not snapshot["changed_during_capture"], "capture was unstable")
    require(len(snapshot["files"]) == 72, "snapshot count changed")
    for row in snapshot["files"]:
        verify(ROOT / "snapshot", row)
    pairs = [
        ("points_review/normal.json", "points_review/optimized.json"),
        ("numeric_review/normal.json", "numeric_review/optimized.json"),
        ("tower_review/normal.stdout", "tower_review/optimized.stdout"),
    ]
    for first, second in pairs:
        require((ROOT / first).read_bytes() == (ROOT / second).read_bytes(),
                f"normal/-O mismatch: {first}")
    normal = json.loads((ROOT / "foundation_followup/normal.json").read_text())
    optimized = json.loads((ROOT / "foundation_followup/optimized.json").read_text())
    require(normal.pop("mode") == "normal", "wrong normal parser mode")
    require(optimized.pop("mode") == "optimized", "wrong optimized parser mode")
    require(normal == optimized, "parser results differ beyond mode marker")
    reconciliation = json.loads((ROOT / "commit_reconciliation.json").read_text())
    require(sum(row["status"] == "identical" for row in reconciliation["files"]) == 62,
            "commit reconciliation count changed")
    print(json.dumps({"status": "PASS", "scope": "archived_integrity_only",
                      "artifacts": len(rows), "snapshot_files": 72,
                      "byte_identical_normal_optimized_pairs": len(pairs),
                      "parser_pair_identical_except_mode": True,
                      "reconciled_identical_files": 62,
                      "native_or_g4_execution": False}, sort_keys=True))


if __name__ == "__main__":
    main()
