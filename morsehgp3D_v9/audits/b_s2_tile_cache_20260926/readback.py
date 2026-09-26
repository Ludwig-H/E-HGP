#!/usr/bin/env python3
"""LIVE local reader of sampled work, never a FULL timing qualification."""
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def need(ok, reason):
    if not ok:
        raise RuntimeError(reason)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    out = HERE / "results"
    manifest = json.loads((out / "MANIFEST.json").read_text())
    need(manifest["status"] == "completed" and not manifest["gcp_used"], "scope")
    need(manifest["source_before"] == manifest["source_after"], "closing sources")
    for name, pin in manifest["source_before"].items():
        need(sha(ROOT / name) == pin, "source " + name)
    need(sha(Path(manifest["library"])) == manifest["library_sha256"] == manifest["library_after_sha256"], "library")
    need(sha(Path(manifest["binary"])) == manifest["binary_sha256"], "binary")
    for name, pin in manifest["output_sha256"].items():
        need(Path(name).name == name and sha(out / name) == pin, "output " + name)
    for command in manifest["commands"]:
        need(command["status"] == "completed" and command["exit_code"] == command["expected"], "command result")
    # A missing input would also return 2: check the causal message.
    for name in ("bad_separation", "bad_k"):
        need((out / (name + ".stderr")).read_text().strip() == "tile_cache: K/s domain", "refusal cause")
    rows = json.loads((out / "RESULTS.json").read_text())
    need(len(rows) == 6, "cases")
    counts = []
    for row in rows:
        name = "scene_" + row["scene"] + "_K" + str(row["K"]) + "_s" + str(row["s"])
        raw = json.loads((out / (name + ".stdout")).read_text())
        need(all(row[k] == value for k, value in raw.items()), "raw vs summary")
        need(row["scope"] == "stratified_raw_pair_mass_tiles_not_full" and row["mismatches"] == 0, "sample equality")
        need(sha(ROOT / row["input_path"]) == row["input_sha256"], "current input")
        need(row["n"] * 12 == (ROOT / row["input_path"]).stat().st_size, "whole input index")
        need(0 < row["pairs"] <= row["representatives"] * 32 and row["representatives"] > 0, "tile population")
        need(row["selected_tiles"] == row["representatives"] + row["rect_rejected_tiles"], "rectangle filtering")
        need(0 < row["max_trace_nodes"] <= 2 * row["K"] - 3, "trace bound")
        need(row["fully_cached"] > 0 and row["partly_cached"] > 0, "useful and partial cache exercised")
        need(row["fully_cached"] + row["partly_cached"] <= row["pairs"] - row["representatives"], "cache outcomes")
        candidate = row["tiled_search_nodes"] + row["cache_node_tests"]
        counts.append(dict(scene=row["scene"], K=row["K"], s=row["s"], pairs=row["pairs"],
                           baseline_nodes=row["baseline_nodes"], candidate_nodes_plus_cache_tests=candidate,
                           work_ratio=row["baseline_nodes"] / candidate))
    gates = HERE / "gates"
    gm = json.loads((gates / "MANIFEST.json").read_text())
    need(gm["status"] == "completed" and gm["library_sha256"] == manifest["library_sha256"], "gates provenance")
    need(sha(HERE / "gate.cpp") == gm["source_sha256"] and sha(HERE / "gates.py") == gm["runner_sha256"], "gate sources")
    for name, pin in gm["output_sha256"].items():
        need(sha(gates / name) == pin, "gate output")
    need(len(gm["commands"]) == 4 and all(c["returncode"] == c["expected"] for c in gm["commands"]), "gate outcomes")
    need((gates / "mutant.stderr").read_text().strip() == "cause=tile_cache.endpoint_retest", "causal mutant")
    print(json.dumps(dict(status="pass", scope="sampled_geometric_work_only_not_gpu_or_full_gain", rows=counts), indent=2))


if __name__ == "__main__":
    main()
