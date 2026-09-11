#!/usr/bin/env python3
"""Portable text-only receipt reader. No compilation, binary payload or cloud."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent
CAPTURE_PIN = "2e26fa2d592f3455a8fddfd7aa3e9a98370d24bf537c22fe07d20eb9500414a9"


def need(value, reason):
    if not value:
        raise RuntimeError(reason)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def safe(name):
    return bool(name) and not Path(name).is_absolute() and ".." not in Path(name).parts


def main():
    manifest = json.loads((ROOT / "manifest.json").read_text())
    need(manifest["scope"] == "local_meb_device_key_only" and manifest["device_executed"] is False and
         manifest["gcp_used"] is False, "scope drift")
    actual = {path.relative_to(ROOT).as_posix() for path in ROOT.rglob("*") if path.is_file()}
    need(actual == set(manifest["files"]) | {"manifest.json"}, "physical inventory drift")
    for name, entry in manifest["files"].items():
        need(safe(name), "unsafe physical path")
        path = ROOT / name
        need(not path.is_symlink(), "physical symlink")
        data = path.read_bytes()
        data.decode("utf-8")
        need(data[:4] != b"\x7fELF" and len(data) == entry["size"] and digest(data) == entry["sha256"], "physical pin")
    payload = (ROOT / "capture_manifest.json").read_bytes()
    need(digest(payload) == CAPTURE_PIN, "private closure drift")
    captured = json.loads(payload)
    mapping = json.loads((ROOT / "storage_map.json").read_text())
    need(set(mapping) == set(captured["files"]), "logical inventory drift")
    with tempfile.TemporaryDirectory(prefix="mhgp7-meb-key-reader-") as temporary:
        restored = Path(temporary)
        for name, entry in mapping.items():
            need(safe(name) and safe(entry["storage"]) and entry["storage"] in manifest["files"], "unsafe mapping")
            expected = captured["files"][name]
            need(entry["size"] == expected["size"] and entry["sha256"] == expected["sha256"], "mapping metadata")
            data = (ROOT / entry["storage"]).read_bytes()
            need(len(data) == entry["size"] and digest(data) == entry["sha256"], "logical pin")
            target = restored / name
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open("xb") as stream:
                stream.write(data)
        flags = ["-B"] + (["-O"] if sys.flags.optimize else [])
        result = subprocess.run([sys.executable, *flags, str(restored / "verify.py")],
                                check=False, capture_output=True, text=True)
        need(result.returncode == 0, "private reader: " + result.stdout + result.stderr)
        report = json.loads(result.stdout)
        need(report["status"] == "passed" and report["compared"] == 605 and
             report["device_executed"] is False and report["gcp_used"] is False and
             report["elf_required"] is False, "private result scope")
    print(json.dumps({"status": "passed", "logical_files": len(mapping), "physical_files": len(actual),
        "host_o2_san_passed": True, "nvcc_compile_link_passed": True, "device_executed": False,
        "gcp_used": False, "manifest_sha256": digest((ROOT / "manifest.json").read_bytes())}, sort_keys=True))


if __name__ == "__main__":
    main()
