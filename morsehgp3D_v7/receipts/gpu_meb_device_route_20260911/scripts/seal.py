#!/usr/bin/env python3
"""Seal closed local evidence without ELF/vendor and without replacement."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CAPTURES = ("fixture_r1", "stub_r1", "stub_r2", "stub_r3", "nvcc_r1", "san_root_r1")


def main():
    files = [p for p in ROOT.iterdir() if p.is_file() and p.suffix in {".md", ".py", ".json"} and p.name != "manifest.json"]
    files += [p for p in (ROOT / "source").rglob("*") if p.is_file()]
    for name in ("originals", "patches"):
        files += [p for p in (ROOT / name).rglob("*") if p.is_file()]
    for name in CAPTURES:
        receipt = json.loads((ROOT / name / "receipt.json").read_text())
        if receipt["status"] not in {"passed", "failed"} or not receipt["sources_stable"]:
            raise RuntimeError("capture not closed/stable: " + name)
        files += [p for p in (ROOT / name).rglob("*") if p.is_file() and p.name not in {"generator", "stub_gate", "device_gate"}]
    entries = {}
    for path in sorted(files):
        if path.is_symlink():
            raise RuntimeError("symlink refused")
        data = path.read_bytes(); data.decode("utf-8")
        entries[path.relative_to(ROOT).as_posix()] = {"size": len(data), "sha256": hashlib.sha256(data).hexdigest()}
    with (ROOT / "manifest.json").open("x") as stream:
        json.dump({"scope": "local_meb_batch_only", "snapshot_commit": "ce842a3f1c0d55786250b1deba85e7bd92c8807a",
                   "device_executed": False, "gcp_used": False, "files": entries}, stream, indent=2, sort_keys=True)
        stream.write("\n")
    print(hashlib.sha256((ROOT / "manifest.json").read_bytes()).hexdigest())


if __name__ == "__main__":
    main()
