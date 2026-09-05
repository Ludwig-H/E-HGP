#!/usr/bin/env python3
"""Read the closed E/F s8 pair captured during F qualification, no engine run."""
import json
from pathlib import Path
import re

from verify_receipts import decode, require, sha

HERE = Path(__file__).resolve().parent
folder = HERE / "pair_s8"
capture = decode((folder / "capture.json").read_bytes())
for name, pin in capture.items():
    require(Path(name).name == name, "safe captured name")
    data = (folder / name).read_bytes()
    require(sha(data) == pin["sha256"] and len(data) == pin["bytes"], "capture integrity")
hashes = decode((folder / "hashes.json").read_bytes())
for name, pin in hashes.items():
    require(sha((folder / name).read_bytes()) == pin, "constructor manifest")
runs = decode((folder / "runs.json").read_bytes())
summary = decode((folder / "summary.json").read_bytes())
metadata = decode((folder / "metadata.json").read_bytes())
require(metadata["snapshot_before"] == decode((folder / "snapshot_after.json").read_bytes()), "stable source/binary snapshot")
for kind, expected_sha in (("baseline", "df75153326f7bbf4ce0a412031a365205559cb68155d4304adc9301461f505f6"), ("candidate", "ee29d3d5cfb49a728fa9dfa44fdb85a5a6043c941b1f61d4a6d9531ea4671f85")):
    binary = metadata[kind]["record"]["binary"]
    require(binary["sha256"] == expected_sha and metadata["snapshot_before"]["files"][binary["path"]] == expected_sha, "E/F pinned binary identity")
require(len(runs) == 2 and [run["role"] for run in runs] == ["E", "F"], "two fresh ordered arms")
require(summary["status"] == "paired_equal" and summary["error"] is None and summary["source_stable"] is True, "terminal pair")
parsed = []
for run in runs:
    require(all(flag in run["command"] for flag in ("--s=8", "--n=8000", "--coord=65536", "--seed=3", "--smax=11", "--complete-incidences", "--digest")), "pair input and payload flags")
    require(run["returncode"] == 0 and run["timed_out"] is False and run["status"] == "engine_completed", "successful arm")
    require(run["started_utc"] < run["ended_utc"], "timestamp order")
    raw = (folder / run["stdout"]).read_text()
    require(not (folder / run["stderr"]).read_bytes(), "empty engine stderr")
    require("Exit status: 0" in (folder / run["usage"]).read_text(), "raw time exit zero")
    lines = raw.splitlines()
    for exact in (
        "payload=mhgp7-forests-horizontal-v1 authority=status_terminal callbacks=provisional vertical_maps=none",
        "backend=cpu_reference",
        "forest_semantics=normalized_horizontal_h0_candidate public_status=not_claimed require_exact=false",
        "tower_scope=profile_complete_k10 smax_requested=11 smax_effective=11",
    ):
        require(lines.count(exact) == 1, "raw scope " + exact)
    digests = dict(re.findall(r"^(digest_forest_K\d+|digest_all)=([0-9a-f]{64})$", raw, re.M))
    require(len(digests) == 11 and digests == run["digests"], "eleven raw output digests")
    cards = {}
    for order, fields in re.findall(r"^cardinalites K=(\d+) (.+)$", raw, re.M):
        require(order not in cards, "unique order")
        cards[order] = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+)", fields)}
    require(set(cards) == {str(k) for k in range(1, 11)} and cards == run["cardinalities"], "ten raw cardinalities")
    require(cards["1"]["facettes"] == 8000 and all(v["deltas"] > 0 for v in cards.values()), "pair nonvacuum")
    semantic = [line for line in lines if line.startswith(("cardinalites ", "silent_K", "silent_limits ", "stockage_foret ", "famille=", "digest_", "forest_layout=", "memory_budget_scope="))]
    require(len([line for line in semantic if line.startswith("silent_K")]) == 9, "nine silent orders")
    parsed.append({"digests": digests, "cardinalities": cards, "semantic_lines": semantic})
require(parsed[0] == parsed[1], "raw semantic equality E/F")
require(runs[0]["ended_utc"] < runs[1]["started_utc"], "sequential E then F")
print(json.dumps({"status": "closed_s8_raw_semantics_equal", "scope": "one completed E/F constructor pair read independently; no performance conclusion", "auditor_engine_runs": 0, "public_status": "not_claimed", "started_utc": summary["started_utc"], "ended_utc": summary["ended_utc"], "digest_all": parsed[0]["digests"]["digest_all"], "orders": 10, "raw_semantic_lines_equal": len(parsed[0]["semantic_lines"]), "cardinalities": parsed[0]["cardinalities"], "capture_sha256": sha((folder / "capture.json").read_bytes())}, indent=2, sort_keys=True))
