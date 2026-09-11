#!/usr/bin/env python3
"""Pin the separately authorized future CPU seam; no callback implementation."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]
RELATIVE = "src/forest/full_ball_tower.hpp"
SOURCE = REPO / "build/v7_static_batch_seam_20260911/source/morsehgp3D_v7" / RELATIVE
PIN = "83f1c78e0656f08cd42522e4cd36d153ce283a6082246a36fe5225b3790c6366"


def main():
    target = ROOT / "overlay/morsehgp3D_v7" / RELATIVE
    if target.exists() or (ROOT / "overlay.json").exists():
        raise RuntimeError("create-only overlay exists")
    if hashlib.sha256(SOURCE.read_bytes()).hexdigest() != PIN:
        raise RuntimeError("authorized source changed")
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(SOURCE, target)
    data = {"source": str(SOURCE.relative_to(REPO)), "files": {RELATIVE: PIN},
        "scope": "future_CPU_static_batch_seam_default_backend_only", "callback_supplied": False,
        "new_helpers": [], "results_inherited": False, "gcp_used": False}
    with (ROOT / "overlay.json").open("x") as output:
        output.write(json.dumps(data, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"status": "overlay_prepared", "header_sha256": PIN, "compiled": False}))


if __name__ == "__main__":
    main()
