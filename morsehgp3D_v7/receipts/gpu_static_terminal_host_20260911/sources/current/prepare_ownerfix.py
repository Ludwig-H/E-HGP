#!/usr/bin/env python3
"""Create-only mechanical import of the closed terminal r2 source tree."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]
OLD = REPO / "build/v7_gpu_static_terminal_20260911"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    if (ROOT / "ownerfix_import.json").exists() or (ROOT / "source").exists():
        raise RuntimeError("ownerfix import already exists")
    selected = sorted(path for directory in ("source", "originals", "upstream_intruder")
        for path in (OLD / directory).rglob("*") if path.is_file())
    selected += sorted(path for path in OLD.iterdir() if path.is_file() and
        path.name not in ("prepare.py", "README.md"))
    selected += sorted(path for directory in ("t2_trial", "guards_trial")
        for path in (OLD / directory).iterdir() if path.is_file() and path.name != "prepare.py")
    pins = {}
    for path in selected:
        relative = path.relative_to(OLD)
        target = ROOT / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        if target.exists():
            raise RuntimeError("create-only destination already exists: " + str(target))
        pins[str(relative)] = sha(path)
        shutil.copy2(path, target)
    shutil.copy2(OLD / "terminal_owner.hpp", ROOT / "guards_trial/terminal_owner.r2.hpp")
    shutil.copy2(OLD / "README.md", ROOT / "originals/terminal_readme.r2.md")
    receipts = {}
    for relative in ("o2_r1/receipt.json", "o2_r2/receipt.json", "san_root_r1/receipt.json",
        "t2_trial/o2_r1/receipt.json", "t2_trial/san_root_r1/receipt.json",
        "guards_trial/o2_r1/receipt.json", "guards_trial/san_root_r1/receipt.json"):
        path = OLD / relative
        data = json.loads(path.read_text())
        if data.get("status") != "passed" or data.get("sources_stable") is not True:
            raise RuntimeError("historical capture not closed: " + relative)
        receipts[relative] = sha(path)
    if pins != {relative: sha(OLD / relative) for relative in pins}:
        raise RuntimeError("historical sources changed during import")
    manifest = {"source_root": str(OLD.relative_to(REPO)), "source_pins": pins,
        "historical_receipt_pins": receipts, "prior_results_inherited": False,
        "old_owner_has_cross_type_token_collision": True,
        "old_owner_sha256": sha(OLD / "terminal_owner.hpp")}
    (ROOT / "ownerfix_import.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"status": "imported", "files": len(pins), "historical_receipts": len(receipts)}))


if __name__ == "__main__":
    main()
