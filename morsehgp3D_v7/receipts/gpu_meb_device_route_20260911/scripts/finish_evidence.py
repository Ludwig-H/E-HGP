#!/usr/bin/env python3
"""Create textual originals, exact diffs and binary pins; never overwrite."""
import difflib
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]
PUBLISHED = REPO / "morsehgp3D_v7/receipts/gpu_meb_selection_prototype_20260911/sources"
SOURCE = ROOT / "source/morsehgp3D_v7"


def write(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("xb") as stream:
        stream.write(content)


def main():
    pairs = (
        (PUBLISHED / "q4_original.hpp.source", SOURCE / "src/lanes/q4.hpp", "q4_hd_annotation"),
        (PUBLISHED / "meb_selection.cuh", SOURCE / "src/gpu/anchor_meb_selection_private.cuh", "private_selection_route_and_flag"),
        (SOURCE / "tests/anchor_meb_gate.cpp", SOURCE / "tests/anchor_meb_fixture_generator.cpp", "fixture_generator"),
    )
    for original, changed, name in pairs:
        old, new = original.read_bytes(), changed.read_bytes()
        write(ROOT / "originals" / (name + ".source"), old)
        patch = "".join(difflib.unified_diff(old.decode().splitlines(keepends=True), new.decode().splitlines(keepends=True),
                                            fromfile="original/" + name, tofile=str(changed.relative_to(ROOT))))
        write(ROOT / "patches" / (name + ".patch"), patch.encode())
    for relative in ("src/gpu/anchor_meb_route.cuh", "tests/anchor_meb_route_device_gate.cu"):
        source = SOURCE / relative
        patch = "".join(difflib.unified_diff([], source.read_text().splitlines(keepends=True),
                                            fromfile="/dev/null", tofile="source/morsehgp3D_v7/" + relative))
        write(ROOT / "patches" / (source.name + ".patch"), patch.encode())
    binaries = {}
    for name in ("fixture_r1/generator", "stub_r2/stub_gate", "stub_r3/stub_gate", "san_root_r1/stub_gate", "nvcc_r1/device_gate"):
        path = ROOT / name
        data = path.read_bytes()
        if not data.startswith(b"\x7fELF"):
            raise RuntimeError("expected ELF " + name)
        binaries[name] = {"sha256": hashlib.sha256(data).hexdigest(), "size": len(data),
                          "executed": not name.startswith("nvcc_"), "distributed": False}
    write(ROOT / "binary_pins.json", (json.dumps(binaries, indent=2, sort_keys=True) + "\n").encode())


if __name__ == "__main__":
    main()
