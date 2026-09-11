#!/usr/bin/env python3
"""Create-only relocation of qualified test helpers; never patch product code."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]
SEAM = REPO / "build/v7_static_batch_seam_20260911"
PINS = {"cpu_geometry.hpp": "c81fba952428c1237549e6d17610e247cf1c917ca32e28c1ddb114933fd0b5a8",
    "cpu_owner.hpp": "4d0797b43a7d32fe3538abd76baf8b9b5eec0d0b77aa1a9a5268d9eb99392b20",
    "qualification.cpp": "f2b68162ad581ab7b4d54de95330bc88f4ac94e1a8159e288fa94ebd172d45fe"}
HEADER_PIN = "83f1c78e0656f08cd42522e4cd36d153ce283a6082246a36fe5225b3790c6366"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x") as stream:
        stream.write(text)


def main():
    if (ROOT / "origin.json").exists():
        raise RuntimeError("create-only preparation exists")
    for name, pin in PINS.items():
        if sha(SEAM / name) != pin:
            raise RuntimeError("qualified harness source changed")
        write(ROOT / "originals" / name, (SEAM / name).read_text())
    geometry = (SEAM / "cpu_geometry.hpp").read_text().replace(
        "#include MHGP7_FULL_HEADER", '#include "../src/forest/full_ball_tower.hpp"').replace(
        "namespace batch_cpu", "namespace mhgp7::batch_test")
    owner = (SEAM / "cpu_owner.hpp").read_text().replace(
        '#include "cpu_geometry.hpp"', '#include "full_ball_batch_geometry.hpp"').replace(
        "namespace batch_cpu", "namespace mhgp7::batch_test")
    write(ROOT / "candidate/morsehgp3D_v7/tests/full_ball_batch_geometry.hpp", geometry)
    write(ROOT / "candidate/morsehgp3D_v7/tests/full_ball_batch_owner.hpp", owner)
    header = SEAM / "source/morsehgp3D_v7/src/forest/full_ball_tower.hpp"
    if sha(header) != HEADER_PIN:
        raise RuntimeError("authorized header changed")
    target = ROOT / "overlay/morsehgp3D_v7/src/forest/full_ball_tower.hpp"
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(header, target)
    baseline = {}
    active = REPO / "morsehgp3D_v7"
    for name in ("CMakeLists.txt", "cmake/run_expect.cmake", "tests/full_ball_tower_gate.cpp",
        "oracle/local_plateau_oracle.hpp", "src/forest/full_ball_tower.hpp"):
        baseline[name] = sha(active / name)
        write(ROOT / "baseline" / name, (active / name).read_text())
    write(ROOT / "origin.json", json.dumps({"status": "prepared_not_compiled", "source_helper_pins": PINS,
        "baseline_source_pins": baseline, "inherited_results": False, "gcp_used": False}, indent=2, sort_keys=True) + "\n")
    write(ROOT / "overlay.json", json.dumps({"source": str(header.relative_to(REPO)),
        "files": {"src/forest/full_ball_tower.hpp": HEADER_PIN}, "product_instrumentation": False,
        "callback_supplied": True, "new_product_helpers": [], "results_inherited": False}, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"status": "prepared", "compiled": False, "header_sha256": HEADER_PIN}))


if __name__ == "__main__":
    main()
