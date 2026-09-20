#!/usr/bin/env python3
"""Read closed evidence and compare streams. A digest is not an exact oracle."""
import copy
import hashlib
import json
from pathlib import Path
import statistics
import subprocess

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate_row(row):
    f, c, p = (row[key] for key in ["front_work", "census_work", "pool_work"])
    i, e, k = row["inheritance_work"], row["extension_work"], row["kmax"]
    require(row["schema"] == "mhgp8_audit_front_options_lidar_v1" and
            row["status"] == "completed" and row["threads"] == 0 and
            row["kmax"] == 10 and row["s"] == 8 and row["pool_min_factor"] == 64,
            "capture scope mismatch")
    require(row["total_unordered_pairs"] == row["n"] * (row["n"] - 1) // 2 and
            row["total_unordered_pairs"] == f["rejected_pair_mass"][0] + f["residual_pair_mass"][0] and
            f["residual_pair_mass"][0] == row["candidate_pairs"] + p["filtered_pairs"] and
            row["candidate_pairs"] == row["accepted_pairs"] + row["rejected_pairs"], "pair partition")
    require(row["digest"]["supports"] == row["accepted_pairs"] == c["payload_supports"] and
            row["digest"]["interior_ids"] == c["payload_interior_sites"] and
            row["digest"]["shell_ids"] == c["payload_shell_sites"], "payload totals")
    require(f["proposed_sites"] == f["proposals_in_factors"] + i["inherited_duplicates"] + f["h_bound_tests"],
            "proposal ledger")
    if row["inherit_witnesses"]:
        require(i["inherited_credits"] % 2 == 0 and
                f["witness_lane_credits"] + i["inherited_credits"] // 2 ==
                k * f["fully_rejected_products"] + i["emitted_witness_credits"], "inheritance ledger")
    else:
        require(all(value == 0 for value in i.values()), "disabled inheritance")
    if row["window_factor"] == 1:
        require(all(value == 0 for value in e.values()), "disabled extension")


def main():
    capture = BASE / "capture"
    manifest = json.loads((capture / "MANIFEST.json").read_text())
    completion = json.loads((capture / "COMPLETION.json").read_text())
    require(completion["status"] == "completed", "unclosed capture")
    for name, value in completion["sha256"].items():
        require(sha(capture / name) == value, "closed record changed: " + name)
    for name, value in manifest["pins"].items():
        require(sha(ROOT / name) == value, "capture dependency changed: " + name)
    sources = json.loads((BASE / "SOURCE_PINS.json").read_text())
    for name, value in sources["source_sha256"].items():
        data = subprocess.check_output(["git", "show", sources["commit"] + ":" + name], cwd=ROOT)
        require(hashlib.sha256(data).hexdigest() == value, "source commit mismatch")
    groups, all_rows = {}, []
    require(len(manifest["planned"]) == len(completion["records"]) == 40, "incomplete campaign")
    expected_cases = [(0, 8000), (100, 8000), (200, 8000), (0, 16000), (0, 32000),
                      (0, 50000), (100, 50000), (200, 50000), (0, 50000), (0, 50000)]
    for number, plan in enumerate(manifest["planned"]):
        stem = f"record_{number:03d}"
        meta = json.loads((capture / (stem + ".json")).read_text())
        row = json.loads((capture / (stem + ".stdout")).read_text())
        require(meta["command"] == plan["command"] and meta["returncode"] == 0 and
                meta["status"] == "completed" and not (capture / (stem + ".stderr")).read_text(),
                "command failed or mismatched")
        for suffix in ["stdout", "stderr"]:
            require(sha(capture / (stem + "." + suffix)) == meta[suffix + "_sha256"], "raw output changed")
        command = plan["command"]
        require(row["family"] == command[2] and row["n"] == int(command[1]) and
                row["window_factor"] == int(command[-3]) and
                row["small_factor_limit"] == (None if command[-2] == "all" else int(command[-2])) and
                row["inherit_witnesses"] == bool(int(command[-1])) and
                (plan["scan"], plan["n"]) == expected_cases[plan["group"]], "command/result options")
        validate_row(row)
        groups.setdefault(plan["group"], []).append((plan, row))
        all_rows.append(row)
    comparisons = []
    discrete = ["front_work", "extension_work", "inheritance_work", "census_work", "pool_work",
                "sibling_work", "order_work", "joint_work", "callback_work", "digest", "candidate_pairs"]
    repeats = {}
    for group in range(10):
        entries = groups[group]
        by_option = {(r["window_factor"], r["small_factor_limit"], r["inherit_witnesses"]): r for _, r in entries}
        require(set(by_option) == {(1, None, False), (2, 16, False), (2, 16, True), (4, None, True)},
                "missing/duplicate option")
        reference = by_option[(1, None, False)]
        two = by_option[(2, 16, False)]
        inherited = by_option[(2, 16, True)]
        require(inherited["front_work"]["residual_pair_mass"][0] <= two["front_work"]["residual_pair_mass"][0],
                "inheritance front domination")
        for plan, row in entries:
            require(row["digest"] == reference["digest"], "stream digest differs")
            require(row["input_hash"] == reference["input_hash"], "input digest differs")
            key = (plan["scan"], plan["n"], row["window_factor"], row["inherit_witnesses"])
            values = {field: row[field] for field in discrete}
            if key in repeats:
                require(values == repeats[key], "repeated work differs")
            repeats[key] = values
            comparisons.append({"group": group, "scan": plan["scan"], "n": row["n"],
                  "window": row["window_factor"], "inherit": row["inherit_witnesses"],
                  "pipeline_s": row["timings"]["pipeline_wall_ms"] / 1000,
                  "total_s": row["timings"]["total_ms"] / 1000,
                  "time_vs_default": row["timings"]["pipeline_wall_ms"] / reference["timings"]["pipeline_wall_ms"],
                  "candidates": row["candidate_pairs"], "census_visits": row["census_work"]["count_node_visits"],
                  "front_h": row["front_work"]["h_bound_tests"], "supports": row["digest"]["supports"]})
    mutants = []
    for field, target, expected in [("accepted_pairs", None, "pair partition"),
                                    ("payload_shell_sites", "census_work", "payload totals"),
                                    ("proposed_sites", "front_work", "proposal ledger"),
                                    ("inherited_credits", "inheritance_work", "inheritance ledger")]:
        row = copy.deepcopy(next(r for r in all_rows if r["inherit_witnesses"]))
        (row if target is None else row[target])[field] += 1
        try:
            validate_row(row)
        except ValueError as error:
            require(str(error) == expected, "mutant rejected for wrong reason")
            mutants.append(field)
        else:
            raise ValueError("reader mutant survived: " + field)
    medians = {}
    for option in [(1, False), (2, False), (2, True), (4, True)]:
        rows = [r for r in comparisons if r["scan"] == 0 and r["n"] == 50000 and
                (r["window"], r["inherit"]) == option]
        medians[str(option)] = {"pipeline_s": statistics.median(r["pipeline_s"] for r in rows),
                               "times_s": [r["pipeline_s"] for r in rows]}
    print(json.dumps({"status": "pass", "measurements": len(all_rows), "paired_groups": 10,
                      "digest_agreement": True, "reader_mutants_rejected": mutants,
                      "scan0_50k_medians": medians, "observations": comparisons}, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
