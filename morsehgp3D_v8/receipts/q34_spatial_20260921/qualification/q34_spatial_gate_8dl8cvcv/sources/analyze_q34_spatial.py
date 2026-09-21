#!/usr/bin/env python3
"""Six real parent/child comparisons per spatial scene; no native execution."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import sys

RELATIONS = (
    ("full", "half_x_neg"), ("full", "half_x_nonneg"),
    ("half_x_neg", "quarter_x_neg_y_neg"),
    ("half_x_neg", "quarter_x_neg_y_nonneg"),
    ("half_x_nonneg", "quarter_x_nonneg_y_neg"),
    ("half_x_nonneg", "quarter_x_nonneg_y_nonneg"),
)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def growth(parent_n, child_n, parent_work, child_work):
    for n in (parent_n, child_n):
        require(type(n) is int and n >= 0, "invalid size")
    require(parent_n >= child_n, "a spatial child cannot exceed its parent")
    for work in (parent_work, child_work):
        require(type(work) in (int, float) and math.isfinite(work) and work >= 0, "invalid work")
    result = dict(parent_n=parent_n, child_n=child_n, parent_work=parent_work,
                  child_work=child_work, size_ratio=None, work_ratio=None,
                  quadratic_ratio=None, empirical_exponent=None,
                  below_quadratic=None, quadratic_relation=None, non_estimable=None)
    if child_n == 0:
        result["non_estimable"] = "empty_child"
        return result
    result["size_ratio"] = parent_n / child_n
    result["quadratic_ratio"] = parent_n**2 / child_n**2
    if parent_n == child_n:
        result["non_estimable"] = "equal_sizes"
        return result
    if parent_work == 0 or child_work == 0:
        result["non_estimable"] = "zero_work"
        return result
    result["work_ratio"] = parent_work / child_work
    result["empirical_exponent"] = math.log(parent_work / child_work) / math.log(parent_n / child_n)
    # Exact for integer counters, never decide their quadratic boundary from
    # the rounded exponent or an assumed doubling of point count.
    left, right = parent_work * child_n**2, child_work * parent_n**2
    result["below_quadratic"] = left < right
    result["quadratic_relation"] = "below" if left < right else "above" if left > right else "equal"
    return result


def flatten(value, prefix):
    out = {}
    if type(value) is dict:
        for key, child in value.items():
            out.update(flatten(child, prefix + "." + key))
    elif type(value) is list:
        for index, child in enumerate(value):
            out.update(flatten(child, prefix + f"[{index}]"))
    else:
        require(type(value) is int and value >= 0, "noninteger native counter: " + prefix)
        out[prefix] = value
    return out


def metrics(row):
    result = {}
    for group in ("cloud_work", "index_work", "work", "parallel", "memory"):
        result.update(flatten(row[group], group))
    result.update(flatten(row["front"]["work"], "front.work"))
    for key in ("callbacks", "q3", "q4", "support_ids", "shell_ids"):
        result["output." + key] = row["output"][key]
    for key, value in row["timings_ms"].items():
        result["timings_ms." + key] = value
    return result


def kind(name):
    if name.startswith("timings_ms."):
        return "elapsed_time_not_deterministic_work"
    if "bytes" in name or name.startswith("memory."):
        return "capacity_not_rss_or_work"
    if "max_" in name or "peak_" in name:
        return "maximum_not_additive_work"
    if name.startswith("parallel."):
        return "scheduling_or_storage_not_geometry"
    if name.startswith("output."):
        return "output_volume"
    return "raw_counter_may_be_subset_do_not_sum_all_fields"


def analyze(entries):
    require(type(entries) is list and entries, "no spatial observations")
    by_repeat = {}
    for entry in entries:
        require(entry["status"] in ("completed", "empty"), "unfinished observation")
        slot = by_repeat.setdefault(entry["repeat"], {})
        require(entry["dataset"] not in slot, "duplicate piece within a repetition")
        slot[entry["dataset"]] = entry
    names = {name for pair in RELATIONS for name in pair}
    comparisons, totals = [], []
    for repetition, rows in sorted(by_repeat.items()):
        require(rows.keys() == names, "must retain all seven pieces, including empty ones")
        sizes = {name: entry["row"]["n"] if entry["row"] is not None else 0 for name, entry in rows.items()}
        require(sizes["full"] == sizes["half_x_neg"] + sizes["half_x_nonneg"] and
                all(sizes[parent] == sum(sizes[child] for p, child in RELATIONS if p == parent)
                    for parent in ("half_x_neg", "half_x_nonneg")), "spatial cardinalities do not partition")
        configurations = {tuple(entry["row"][key] for key in
            ("kmax", "s", "mask", "q4_backend", "workers", "front_mode", "output_mode", "witness_mode",
             "q3_census_mode", "witness_bounds_mode", "q4_seed_mode", "q4_seed_block_size"))
            for entry in rows.values() if entry["row"] is not None}
        require(len(configurations) == 1, "pieces use different algorithm configurations")
        for parent, child in RELATIONS:
            a, b = rows[parent], rows[child]
            na = a["row"]["n"] if a["row"] is not None else 0
            nb = b["row"]["n"] if b["row"] is not None else 0
            record = dict(repeat=repetition, parent=parent, child=child, parent_n=na, child_n=nb)
            if a["status"] == "empty" or b["status"] == "empty":
                record.update(metrics={}, non_estimable="empty_piece")
            else:
                ma, mb = metrics(a["row"]), metrics(b["row"])
                require(ma.keys() == mb.keys(), "different counter inventories")
                record["metrics"] = {name: dict(kind=kind(name), **growth(na, nb, ma[name], mb[name]))
                                     for name in sorted(ma)}
            comparisons.append(record)
        # Level sums describe partitioning overhead, not a size-growth series.
        level_metrics = {}
        for level, pieces in (("full", ("full",)), ("halves", ("half_x_neg", "half_x_nonneg")),
                              ("quarters", tuple(child for _, child in RELATIONS[2:]))):
            selected = [metrics(rows[name]["row"]) for name in pieces if rows[name]["row"] is not None]
            names_at_level = set().union(*selected)
            level_metrics[level] = {name: sum(values[name] for values in selected)
                for name in sorted(names_at_level)
                if kind(name) not in ("capacity_not_rss_or_work", "maximum_not_additive_work")}
        totals.append(dict(repeat=repetition, levels=level_metrics,
                           note="sum of separately executed pieces; no exponent inferred from these sums"))
    return dict(schema="mhgp8_q34_spatial_growth_v1", status="completed",
                scope="empirical_parent_child_work_not_asymptotic_proof_or_full",
                stable_timing_gain_claimed=False, comparisons=comparisons, level_sums=totals)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", type=Path)
    parser.add_argument("--check-live", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    import run_q34_spatial as capture
    proof = capture.read(args.path, args.check_live)
    result = analyze(proof["records"])
    result["capture"] = str(args.path.resolve())
    result["capture_completion_sha256"] = hashlib.sha256((args.path / "COMPLETION.json").read_bytes()).hexdigest()
    result["analysis_source_sha256"] = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    encoded = json.dumps(result, sort_keys=True, allow_nan=False)
    if args.output is None:
        print(encoded)
    else:
        with args.output.open("x", encoding="utf-8") as output:
            output.write(encoded + "\n")
        print(json.dumps(dict(status="completed", comparisons=len(result["comparisons"]),
                             output=str(args.output), sha256=hashlib.sha256((encoded + "\n").encode()).hexdigest()),
                         sort_keys=True))


if __name__ == "__main__":
    main()
