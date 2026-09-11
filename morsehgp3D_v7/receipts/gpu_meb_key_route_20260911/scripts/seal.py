#!/usr/bin/env python3
"""Create-only provenance and source closure, after all three local gates close."""
import difflib
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as stream:
        stream.write(data)


def main():
    subprocess.run([sys.executable, "-B", str(ROOT / "verify.py")], check=True)
    pins = json.loads((ROOT / "pins.json").read_text())
    origins = {}
    for relative, entry in pins["overlays"].items():
        package, logical = entry["origin"].split("/", 1)
        package_root = REPO / "morsehgp3D_v7/receipts" / package
        if package == "gpu_ball_key_device_20260911":
            manifest = json.loads((package_root / "manifest.json").read_text())
            objects = {row["path"]: row for row in manifest["files"]}
            data = (package_root / objects[logical]["object"]).read_bytes()
        else:
            data = (package_root / logical).read_bytes()
        if sha(data) != entry["sha256"]:
            raise RuntimeError("upstream pin changed")
        write(ROOT / "upstream" / relative, data)
        origins[relative] = data
    renamed = {"src/gpu/anchor_meb_key_route.cuh": "src/gpu/anchor_meb_route.cuh",
               "tests/anchor_meb_key_route_gate.cu": "tests/anchor_meb_route_device_gate.cu"}
    changed = []
    for relative in ("src/gpu/anchor_meb_selection_private.cuh", "src/gpu/anchor_meb_key.cuh",
                     "src/gpu/anchor_meb_key_route.cuh", "tests/anchor_meb_key_route_gate.cu"):
        prior = renamed.get(relative, relative)
        old = origins.get(prior, b"")
        new = (ROOT / "source/morsehgp3D_v7" / relative).read_bytes()
        patch = "".join(difflib.unified_diff(old.decode().splitlines(keepends=True),
            new.decode().splitlines(keepends=True), fromfile="upstream/" + prior,
            tofile="source/morsehgp3D_v7/" + relative))
        patch_name = relative.replace("/", "_") + ".patch"
        write(ROOT / "patches" / patch_name, patch.encode())
        changed.append({"source": relative, "prior": prior if old else None,
                        "sha256": sha(new), "patch": "patches/" + patch_name})
    binary_pins = {}
    for capture in sorted(ROOT.glob("*/receipt.json")):
        receipt = json.loads(capture.read_text())
        if "binary_sha256" not in receipt:
            continue
        binary = capture.parent / ("device_gate" if receipt["mode"] == "nvcc" else "stub_gate")
        data = binary.read_bytes()
        if data[:4] != b"\x7fELF" or sha(data) != receipt["binary_sha256"]:
            raise RuntimeError("binary differs from capture")
        binary_pins[str(binary.relative_to(ROOT))] = {"size": len(data), "sha256": sha(data),
            "published": False, "executed": receipt["mode"] != "nvcc"}
    write(ROOT / "binary_pins.json", (json.dumps(binary_pins, indent=2, sort_keys=True) + "\n").encode())
    write(ROOT / "changes.json", (json.dumps(changed, indent=2, sort_keys=True) + "\n").encode())
    files = {}
    for path in sorted(ROOT.rglob("*")):
        if not path.is_file():
            continue
        if path.is_symlink():
            raise RuntimeError("symlink in private closure")
        relative = path.relative_to(ROOT).as_posix()
        if relative in binary_pins:
            continue
        data = path.read_bytes()
        if data[:4] == b"\x7fELF":
            raise RuntimeError("unaccounted executable")
        data.decode("utf-8")
        files[relative] = {"size": len(data), "sha256": sha(data)}
    manifest = {"scope": "local_meb_device_key_only", "base_commit": pins["base_commit"],
                "device_executed": False, "gcp_used": False, "files": files}
    payload = (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode()
    write(ROOT / "manifest.json", payload)
    print(json.dumps({"manifest_sha256": sha(payload), "logical_files": len(files),
                      "elf_pins_not_payload": len(binary_pins)}))


if __name__ == "__main__":
    main()
