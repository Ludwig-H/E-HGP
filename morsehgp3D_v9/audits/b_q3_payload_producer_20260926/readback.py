#!/usr/bin/env python3
"""Check the CPU prototype receipt; no assert (also runs under python -O)."""
import argparse
import hashlib
import json
from pathlib import Path


def require(okay, cause):
    if not okay:
        raise ValueError(cause)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    output = args.output.resolve(strict=True)
    root = Path(__file__).resolve().parents[3]
    manifest = json.loads((output / "MANIFEST.json").read_text())
    results = json.loads((output / "RESULTS.json").read_text())
    require(manifest["status"] == "completed" and not manifest["gcp_used"], "capture status")
    require(manifest["scope"] == "cpu_portable_q3_range_not_L15_not_full", "scope")
    require(manifest["source_before"] == manifest["source_after"], "source closing")
    for name, expected in manifest["source_before"].items():
        path = Path(name) if Path(name).is_absolute() else root / name
        require(digest(path) == expected, "source drift: " + name)
    for name, expected in manifest["outputs"].items():
        require(digest(output / name) == expected, "output drift: " + name)
    for command in manifest["commands"]:
        require(command["status"] == "completed" and command["expected"] == command["exit_code"], "command status")
        require(command["argv"], "missing argv")
    for name, expected in manifest["binaries"].items():
        require(digest(Path(name)) == expected, "binary drift")
    for name, expected in manifest["input_files"].items():
        require(digest(Path(name)) == expected, "input drift")
    for mutant in manifest["mutants"]:
        require(mutant["cause"] == "cause=interior_ids_differ" and digest(Path(mutant["binary"])) == mutant["sha256"], "causal mutant")
    gate = results["selftest"]
    require(gate["status"] == "pass" and gate["calls"] == 484 and gate["records"] == 233, "gate size")
    require(gate["interior_ids"] == 422 and gate["consumer_BallData_compared"] == 227, "payload/consumer coverage")
    require(gate["extra_shell_records"] == 6 and gate["discarded_prefix_ids"] == 15 and gate["max_depth"] == 8, "boundary coverage")
    rows = results["measurements"]
    require(len(rows) == 52, "measurement count")
    for row in rows:
        require(row["status"] == "equal" and row["scope"] == "128_stratified_adjacent_rank_edges_not_full", "measurement scope")
        require(row["old_census_ms"] >= 0 and row["payload_census_ms"] >= 0, "clock")
        require(row["seeds"] == row["records"] + row["rejected"], "census partition")
        require(row["output_bytes"] == 32 * row["records"], "sidecar size")
    grouped = {}
    for row in rows:
        grouped.setdefault((row["label"], row["n"], row["k"], row.get("scene")), []).append(row)
    work = ("cover_sites", "seeds", "point_tests_both", "rejected", "records", "scratch_id_writes", "discarded_prefix_ids", "extra_syncs", "output_bytes")
    for group in grouped.values():
        require([row["repeat"] for row in group] == [0, 1, 2, 3], "repeat identities")
        require([row["producer_first"] for row in group] == [False, True, True, False], "ABBA order")
        for key in work:
            require(len({row[key] for row in group}) == 1, "repeat work drift: " + key)
    print(json.dumps({"status": "pass", "scope": manifest["scope"], "measurements": len(rows), "cases": len(grouped), "mutants": len(manifest["mutants"])}))


if __name__ == "__main__":
    main()
