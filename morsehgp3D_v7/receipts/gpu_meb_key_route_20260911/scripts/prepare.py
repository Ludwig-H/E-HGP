#!/usr/bin/env python3
"""Create-only snapshot and explicit qualified overlays; never touches active code."""
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]
COMMIT = "c03f6be8488453486b112811071827a96303ec86"
MEB = REPO / "morsehgp3D_v7/receipts/gpu_meb_device_route_20260911"
KEY = REPO / "morsehgp3D_v7/receipts/gpu_ball_key_device_20260911"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as stream:
        stream.write(data)


def main():
    target = ROOT / "source"
    target.mkdir(exist_ok=False)
    archived = subprocess.run(["git", "archive", COMMIT, "morsehgp3D_v7/src"],
                              cwd=REPO, check=True, capture_output=True).stdout
    with tarfile.open(fileobj=io.BytesIO(archived)) as archive:
        for member in archive.getmembers():
            if not member.isfile():
                continue
            if Path(member.name).is_absolute() or ".." in Path(member.name).parts:
                raise RuntimeError("unsafe archive member")
            write(target / member.name, archive.extractfile(member).read())
    pins = {"base_commit": COMMIT, "base_sources": {str(path.relative_to(target)): sha(path.read_bytes())
            for path in sorted(target.rglob("*")) if path.is_file()}, "overlays": {}}
    meb_manifest = json.loads((MEB / "manifest.json").read_text())
    key_manifest_bytes = (KEY / "manifest.json").read_bytes()
    if sha(key_manifest_bytes) != "8c985fbef1ece33e0198d24c93f3f005013a4940c660580bc9121d91d8035671":
        raise RuntimeError("key package changed")
    key_manifest = {row["path"]: row for row in json.loads(key_manifest_bytes)["files"]}
    def overlay(relative, data, origin):
        path = target / "morsehgp3D_v7" / relative
        if path.exists():
            write(ROOT / "originals" / relative, path.read_bytes())
            path.unlink()  # exact newly-created private snapshot target only
        write(path, data)
        pins["overlays"][relative] = {"origin": origin, "sha256": sha(data)}
    for relative in ("src/lanes/q4.hpp", "src/gpu/anchor_meb_selection_private.cuh",
                     "src/gpu/anchor_meb_route.cuh", "tests/anchor_meb_route_device_gate.cu",
                     "tests/anchor_meb_fixtures.inc"):
        name = "sources/morsehgp3D_v7/" + relative
        data = (MEB / name).read_bytes()
        if sha(data) != meb_manifest["files"][name]["sha256"]:
            raise RuntimeError("MEB source changed")
        overlay(relative, data, "gpu_meb_device_route_20260911/" + name)
    for relative in ("src/core/intmath.hpp", "src/lanes/keys.hpp"):
        logical = "source/morsehgp3D_v7/" + relative
        entry = key_manifest[logical]
        data = (KEY / entry["object"]).read_bytes()
        if sha(data) != entry["sha256"]:
            raise RuntimeError("key source changed")
        overlay(relative, data, "gpu_ball_key_device_20260911/" + logical)
    data = (MEB / "scripts/nvcc_strict_host.py").read_bytes()
    write(ROOT / "nvcc_strict_host.py", data)
    (ROOT / "nvcc_strict_host.py").chmod(0o700)
    write(ROOT / "pins.json", (json.dumps(pins, indent=2, sort_keys=True) + "\n").encode())


if __name__ == "__main__":
    main()
