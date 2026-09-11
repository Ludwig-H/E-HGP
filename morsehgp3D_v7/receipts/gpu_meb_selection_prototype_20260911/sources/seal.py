#!/usr/bin/env python3
"""Seal only closed private textual evidence; never overwrite a manifest."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CAPTURES = ("o2_r1", "o2_r2", "nvcc_r1", "san_r1")


def main():
    files = [p for p in ROOT.iterdir() if p.is_file() and
             p.suffix in {".py", ".cpp", ".cu", ".cuh", ".source", ".md"}]
    files += [p for p in (ROOT / "source").rglob("*") if p.is_file()]
    for name in CAPTURES:
        receipt = json.loads((ROOT / name / "receipt.json").read_text())
        if receipt["status"] not in {"passed", "failed"} or not receipt["sources_stable"]:
            raise RuntimeError("unclosed or unstable capture: " + name)
        files += [p for p in (ROOT / name).rglob("*") if p.is_file() and
                  p.name not in {"oracle", "transport", "kernel"}]
    entries = {}
    for path in sorted(files):
        if path.is_symlink():
            raise RuntimeError("symlink refused")
        content = path.read_bytes()
        content.decode("utf-8")
        entries[path.relative_to(ROOT).as_posix()] = {
            "sha256": hashlib.sha256(content).hexdigest(), "size": len(content)}
    with (ROOT / "manifest.json").open("x") as stream:
        json.dump({"scope": "private_host_stub_and_nvcc_compile_only",
                   "gcp_used": False, "device_executed": False,
                   "files": entries}, stream, indent=2, sort_keys=True)
        stream.write("\n")
    print(hashlib.sha256((ROOT / "manifest.json").read_bytes()).hexdigest())


if __name__ == "__main__":
    main()
