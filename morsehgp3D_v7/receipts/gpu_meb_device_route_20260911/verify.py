#!/usr/bin/env python3
"""Portable local route evidence reader, no compilation or executable payload."""
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


def safe(name):
    return bool(name) and not Path(name).is_absolute() and ".." not in Path(name).parts


def main():
    manifest = read_json(ROOT / "manifest.json")
    need(manifest["device_executed"] is False and manifest["gcp_used"] is False, "local scope")
    for name, metadata in manifest["files"].items():
        need(safe(name), "unsafe public path")
        path = ROOT / name
        need(path.is_file() and not path.is_symlink(), "missing public file " + name)
        data = path.read_bytes(); data.decode("utf-8")
        need(len(data) == metadata["size"] and digest(data) == metadata["sha256"], "public hash " + name)
    captured_bytes = (ROOT / "capture_manifest.json").read_bytes()
    need(digest(captured_bytes) == "69bdafe0ef0c828a283fe3daaa1691706e00b2c344eba828054aa277df757b72", "capture manifest changed")
    captured = json.loads(captured_bytes)
    mapping = read_json(ROOT / "storage_map.json")
    need(set(mapping) == set(captured["files"]), "mapping incomplete")
    with tempfile.TemporaryDirectory(prefix="mhgp7-meb-route-evidence-") as temporary:
        restored = Path(temporary)
        for name, metadata in captured["files"].items():
            entry = mapping[name]
            need(safe(name) and safe(entry["storage"]), "unsafe logical path")
            need(entry["storage"] in manifest["files"], "storage not sealed")
            need(entry["size"] == metadata["size"] and entry["sha256"] == metadata["sha256"], "mapping metadata changed")
            data = (ROOT / entry["storage"]).read_bytes()
            need(len(data) == metadata["size"] and digest(data) == metadata["sha256"], "mapping content changed")
            target = restored / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        (restored / "manifest.json").write_bytes(captured_bytes)
        flags = ["-B"] + (["-O"] if sys.flags.optimize else [])
        result = subprocess.run([sys.executable, *flags, str(restored / "verify.py")],
                                check=False, capture_output=True, text=True)
        need(result.returncode == 0, "private reader: " + result.stdout + result.stderr)
        report = json.loads(result.stdout)
        need(report["status"] == "passed" and report["captures"] == 6 and report["logical_files"] == 527 and
             report["host_o2_san_passed"] is True and report["device_executed"] is False, "private report scope")
    print(json.dumps({"status": "passed", "logical_files": len(mapping), "public_files": len(manifest["files"]),
                      "host_o2_san_passed": True, "nvcc_compile_link_passed": True, "device_executed": False,
                      "gcp_used": False, "manifest_sha256": digest((ROOT / "manifest.json").read_bytes())}))


if __name__ == "__main__":
    main()
