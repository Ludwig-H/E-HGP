#!/usr/bin/env python3
"""Create-only pinned import of the published K2..8 CUDA-terminal prototype."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]
PACKET = REPO / "morsehgp3D_v7/receipts/gpu_static_terminal_cuda_20260911"
PIN = "5b631b10cf329f549e473f59b73777f441c85fe55374af1c6af56311bcb1da29"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    if (ROOT / "source").exists() or (ROOT / "import.json").exists():
        raise RuntimeError("create-only source already exists")
    if sha(PACKET / "manifest.json") != PIN:
        raise RuntimeError("published packet pin mismatch")
    manifest = json.loads((PACKET / "manifest.json").read_text())
    for name, entry in manifest["files"].items():
        path = PACKET / name
        if path.is_symlink() or sha(path) != entry["sha256"] or path.stat().st_size != entry["size"]:
            raise RuntimeError("published source drift: " + name)
    copied = {}
    current = PACKET / "sources/current"
    for path in sorted(current.rglob("*")):
        if not path.is_file():
            continue
        relative = path.relative_to(current)
        # Keep old generated expectations as history, not an implicit fixture
        # input to new source captures.
        if relative.as_posix() == "cuda_trial/terminal_fixtures.inc":
            target = ROOT / "originals/k8_terminal_fixtures.inc"
        else:
            target = ROOT / relative
        if target.exists():
            # prepare.py belongs to this import, not the historical experiment.
            target = ROOT / "originals" / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
        copied[target.relative_to(ROOT).as_posix()] = sha(path)
    for name in ("export.cpp", "device_gate.cu", "record.py", "fixture_types.hpp", "reference.hpp", "provenance_gate.py"):
        target = ROOT / "originals/k8" / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(current / "cuda_trial" / name, target)
    for path in sorted((PACKET / "qualification").rglob("*")):
        if path.is_file():
            target = ROOT / "qualification" / path.relative_to(PACKET / "qualification")
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)
    # Reconstruct the shared headers of the six already-qualified snapshots,
    # checking against their immutable host capture authority before copying.
    host = json.loads((ROOT / "qualification/host_capture_manifest.json").read_text())
    for receipt_path in sorted((ROOT / "qualification").rglob("receipt.json")):
        data = json.loads(receipt_path.read_text())
        for name, pin in data["sources_before"].items():
            relative = Path(name)
            shared = name.startswith("source/") or (len(relative.parts) == 1 and
                (relative.suffix in (".cpp", ".cu", ".cuh", ".hpp") or name == "nvcc_strict_host.py"))
            if not shared:
                continue
            logical = "ownerfix/" + receipt_path.parent.relative_to(ROOT / "qualification").as_posix() + "/source_snapshot/" + name
            if host["files"][logical]["sha256"] != pin or sha(ROOT / name) != pin:
                raise RuntimeError("qualified shared snapshot mismatch: " + name)
            target = receipt_path.parent / "source_snapshot" / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT / name, target)
    t2 = REPO / "build/v7_t2_census_tower_20260911/t2_oracle.hpp"
    if sha(t2) != "57b615240896f4631434421fa2360d8e5b87db41b0cfc74a28afa0bd0727939c":
        raise RuntimeError("T2 oracle pin mismatch")
    shutil.copy2(t2, ROOT / "cuda_trial/t2_oracle.hpp")
    data = {"packet_manifest_sha256": PIN, "published_at": "dc5a36ba64d5f41027d6acb23a949a38e75d0d56",
        "product_reference_commit": "c03f6be8488453486b112811071827a96303ec86", "copied": copied,
        "terminal_sha256": sha(ROOT / "terminal.cuh"), "owner_sha256": sha(ROOT / "terminal_owner.hpp"),
        "t2_oracle_sha256": sha(t2), "inherited_results": False}
    (ROOT / "import.json").write_text(json.dumps(data, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"status": "imported", "files": len(copied), "terminal": data["terminal_sha256"], "owner": data["owner_sha256"]}))


if __name__ == "__main__":
    main()
