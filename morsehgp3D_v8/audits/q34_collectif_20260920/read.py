#!/usr/bin/env python3
"""Verify the closed model observations; never import a product engine."""
import hashlib
import json
from pathlib import Path

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    capture = BASE / "capture"
    completion = json.loads((capture / "COMPLETION.json").read_text())
    require(completion["status"] == "pass" and completion["failures"] == [], "failed capture")
    for name, value in completion["sha256"].items():
        require(sha(capture / name) == value, "closed file changed: " + name)
    manifest = json.loads((capture / "MANIFEST.json").read_text())
    for name, value in manifest["scripts_sha256"].items():
        require(sha(ROOT / name) == value, "model source changed: " + name)
    for model in ("collective_model", "chord_bound_gate", "experiment"):
        records = [json.loads((capture / f"{model}_{mode}.json").read_text())
                   for mode in ("normal", "optimized")]
        require(all(r["returncode"] == 0 and not r["stderr"] for r in records), "model failed")
        require(records[0]["stdout"] == records[1]["stdout"] ==
                (capture / f"{model}_RESULT.json").read_text(), "normal/-O disagreement")
        for mode, record in zip(([], ["-O"]), records):
            expected = ["python3", "-B"] + mode + [str(BASE.relative_to(ROOT) / (model + ".py"))]
            require(record["command"] == expected, "model command mismatch")
    result = json.loads((capture / "experiment_RESULT.json").read_text())
    for name, value in result["input_sha256"].items():
        require(sha(ROOT / name) == value, "LiDAR input changed")
    rows = result["results"]
    require(len(rows) == 17, "missing model observation")
    for r in rows:
        require(r["baseline_scan"] == r["covered"] * r["seeds"] and
                r["pool_forms"] == r["pool_size"] * r["seeds"], "work identity")
        for v in r["methods"].values():
            require(0 <= v["both_rejected"] <= v["q4_rejected"] <= r["seeds"], "rejection counts")
            require(v["residual_scan_upper_bound"] ==
                    (r["seeds"] - v["both_rejected"]) * r["covered"], "residual bound")
    lidar = [r for r in rows if r["case"].startswith("lidar")]
    require(len(lidar) == 9 and sum(r["seeds"] for r in lidar) == 54, "LiDAR cases changed")
    require(all(all(v == r["methods"]["old_universal"] for v in r["methods"].values()) for r in lidar),
            "new rejection on the selected LiDAR edges")
    print(json.dumps({"status": "pass", "models": 3, "commands": 6,
                      "normal_optimized_identical": True, "observations": 17,
                      "lidar_edges": 9, "lidar_faces": 54,
                      "additional_rejections_on_selected_lidar_edges": 0,
                      "scope": "independent mathematical models; no product qualification"}, sort_keys=True))


if __name__ == "__main__":
    main()
