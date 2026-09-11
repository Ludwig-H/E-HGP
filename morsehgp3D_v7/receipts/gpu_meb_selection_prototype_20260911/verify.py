#!/usr/bin/env python3
"""Portable evidence-only reader: no compiler, executable payload or GCP."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def digest(content):
    return hashlib.sha256(content).hexdigest()


def safe_name(name):
    return bool(name) and not Path(name).is_absolute() and ".." not in Path(name).parts


def main():
    manifest = read_json(ROOT / "manifest.json")
    need(manifest["device_executed"] is False and manifest["gcp_used"] is False,
         "public scope mismatch")
    for name, entry in manifest["files"].items():
        need(safe_name(name), "unsafe public path")
        path = ROOT / name
        need(not path.is_symlink() and path.is_file(), "missing public file " + name)
        data = path.read_bytes()
        need(len(data) == entry["size"] and digest(data) == entry["sha256"],
             "public file mismatch " + name)
        data.decode("utf-8")
    captured_bytes = (ROOT / "capture_manifest.json").read_bytes()
    need(digest(captured_bytes) == "aa7e0ece18a0e72ea3007f10a30544a4c3edd2ed4cca6f3f34b0cf4f05e78a0b",
         "private manifest changed")
    captured = json.loads(captured_bytes)
    mapping = read_json(ROOT / "storage_map.json")
    need(set(mapping) == set(captured["files"]), "logical mapping incomplete")
    with tempfile.TemporaryDirectory(prefix="mhgp7-gpu-meb-evidence-") as temporary:
        restored = Path(temporary)
        for name, original in captured["files"].items():
            entry = mapping[name]
            need(safe_name(name) and safe_name(entry["storage"]), "unsafe logical path")
            need(entry["storage"] in manifest["files"], "unsealed storage")
            need(entry["sha256"] == original["sha256"] and entry["size"] == original["size"],
                 "mapped metadata changed " + name)
            data = (ROOT / entry["storage"]).read_bytes()
            need(digest(data) == original["sha256"] and len(data) == original["size"],
                 "mapped data changed " + name)
            target = restored / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        (restored / "manifest.json").write_bytes(captured_bytes)
        flags = ["-B"] + (["-O"] if sys.flags.optimize else [])
        result = subprocess.run([sys.executable, *flags, str(restored / "verify.py")],
                                check=False, capture_output=True, text=True)
        need(result.returncode == 0, "private reader failed: " + result.stdout + result.stderr)
        report = json.loads(result.stdout)
        need(report["status"] == "passed" and report["files"] == 381 and
             report["san_passed"] is False and report["device_executed"] is False,
             "private scope mismatch")
    print(json.dumps({"status": "passed", "logical_files": len(mapping),
                      "public_files": len(manifest["files"]), "san_passed": False,
                      "device_executed": False, "gcp_used": False,
                      "manifest_sha256": digest((ROOT / "manifest.json").read_bytes())}))


if __name__ == "__main__":
    main()
