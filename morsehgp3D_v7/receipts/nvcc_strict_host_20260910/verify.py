#!/usr/bin/env python3
"""Verify portable local NVCC receipts under ordinary Python and -O."""
from pathlib import Path
import hashlib
import json
import sys


def need(value: bool, reason: str) -> None:
    if not value:
        raise RuntimeError(reason)


def main() -> int:
    root = Path(sys.argv[1]).resolve() if len(sys.argv) == 2 else Path(__file__).resolve().parent
    manifest = json.loads((root / "manifest.json").read_text())
    files = {str(path.relative_to(root)) for path in root.rglob("*") if path.is_file()}
    need(files == set(manifest) | {"manifest.json"}, "manifest inventory")
    for relative, item in manifest.items():
        path = Path(relative)
        need(not path.is_absolute() and ".." not in path.parts, "manifest path")
        raw = (root / relative).read_bytes()
        need(not raw.startswith(b"\x7fELF"), "ELF forbidden in packet")
        need(len(raw) == item["bytes"] and hashlib.sha256(raw).hexdigest() == item["sha256"], "manifest digest")
    receipt = json.loads((root / "receipt.json").read_text())
    need(receipt["status"] == "completed" and receipt["device_executed"] is False, "receipt scope")
    need(receipt["sources_stable"] and receipt["snapshot_stable"], "source stability")
    need(receipt["sources_before"] == receipt["sources_after"], "before/after")
    need(receipt["binaries"] == receipt["binaries_after"], "binary stability")
    for path, digest in receipt["sources_before"].items():
        need(manifest["source/" + path]["sha256"] == digest, "source manifest binding")
    names = set()
    for command in receipt["commands"]:
        need(command["name"] not in names, "command duplicate")
        names.add(command["name"])
        need(command["returncode"] == command["expected"], "command failure")
        for stream in ("stdout", "stderr"):
            item = command[stream]
            need(manifest[item["path"]] == {"bytes": item["bytes"], "sha256": item["sha256"]}, "stream binding")
    need({"compile_link_gate", "compile_link_probe", "minimal_compile_link", "include_define_host_run",
          "reject_strict_bad.cu", "reject_strict_bad_pp.cu", "reject_unknown_phase", "reject_truncated_output",
          "reject_truncated_language"} <= names, "nonvacuity command floor")
    print(f"PASS local_NVCC_compile_link_only files={len(manifest)} commands={len(names)} device_executed=false")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print("REFUS", str(error), file=sys.stderr)
        raise SystemExit(1)
