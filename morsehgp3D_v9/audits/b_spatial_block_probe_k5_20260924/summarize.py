#!/usr/bin/env python3
"""Audit the bounded BVH pilot and join its sparse decisions to pinned S2/F."""

import csv
import hashlib
import json
import statistics
import sys
from pathlib import Path

TRACE_DIR = Path(__file__).resolve().parent.parent / "b_s2_trace_k5_20260924"
sys.path.insert(0, str(TRACE_DIR))
import join_shadow  # noqa: E402


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_tsv(path):
    with path.open(newline="") as stream:
        return list(csv.DictReader(stream, delimiter="\t"))


def need(ok, message):
    if not ok:
        raise ValueError(message)


def summarize(detail_path, shadow_path):
    trace = read_tsv(TRACE_DIR / "quarter_1288_k5.trace.tsv")
    receipt = json.loads((TRACE_DIR / "RESULT.json").read_text())
    need(len(trace) == receipt["survivors"] and
         sum(int(row["F"]) for row in trace) == receipt["sum_F"], "trace ledger")
    need(sum(int(row["F"]) for row in trace if int(row["post_core_mask"]) == 0) ==
         receipt["core_closed_F"], "core-closed F ledger")
    rows = read_tsv(detail_path)
    shadow = join_shadow.join(shadow_path)
    need(len(rows) == len(set(int(row["s2_ordinal"]) for row in rows)) ==
         shadow["decision_rows"], "detail/shadow row count")
    decisions = join_shadow.read_decisions(shadow_path, len(trace))
    cost_fields = ("cells", "outside_cells", "disk_tests", "center_membership_tests",
                   "node_tests", "corner_tests",
                   "corner_evals", "lb_tests", "credited_nodes", "credited_sites",
                   "excluded_nodes", "leaf_ambiguous", "splits", "frontier_copies",
                   "point_tests", "fixed_centers", "fixed_refutations",
                   "gate_nodes", "gate_sites", "exclude_gate_nodes", "exclude_gate_sites")
    costs = {key: 0 for key in cost_fields}
    class_counts = {"block_and_core": 0, "block_only": 0,
                    "core_only": 0, "neither": 0}
    class_f = {key: 0 for key in class_counts}
    lane_counts = {"q3": dict.fromkeys(class_counts, 0),
                   "q4": dict.fromkeys(class_counts, 0)}
    lane_f = {"q3": dict.fromkeys(class_counts, 0),
              "q4": dict.fromkeys(class_counts, 0)}
    complete = core_closed = block_closed = both_closed = block_only_closed = 0
    core_closed_f = block_closed_f = both_closed_f = block_only_closed_f = 0
    sample_f = 0
    strata = {}
    by_stratum = {}
    walls = []
    cpus = []
    for row in rows:
        j = int(row["s2_ordinal"])
        ref = trace[j]
        mask = int(ref["s2_mask"])
        f = int(ref["F"])
        post = int(ref["post_core_mask"])
        proved = int(row["proved_mask"])
        need(proved == decisions[j] and int(row["s2_mask"]) == mask and
             int(row["post_core_mask"]) == post and int(row["F"]) == f,
             f"detail/trace mismatch at {j}")
        need(proved & ~mask == 0, f"widened lane at {j}")
        complete += int(row["complete"])
        sample_f += f
        f_bin = "F_le_8" if f <= 8 else "F_9_32" if f <= 32 else "F_33_128" if f <= 128 else "F_gt_128"
        stratum = f"mask_{mask}_{f_bin}_core_{'some' if post != mask else 'none'}"
        strata[stratum] = strata.get(stratum, 0) + 1
        bucket = by_stratum.setdefault(stratum, {
            "edges": 0, "F": 0, "full_closed_edges": 0, "eligible_F": 0,
            "node_tests": 0, "corner_evals": 0, "lb_tests": 0,
            "disk_tests": 0, "center_membership_tests": 0,
            "point_tests": 0, "direct_gate_form_evals": 0, "total_form_evals": 0,
            "wall_ms": 0.0, "gate_ms": 0.0})
        bucket["edges"] += 1
        bucket["F"] += f
        bucket["full_closed_edges"] += proved == mask
        bucket["eligible_F"] += f if proved == mask else 0
        for key in ("node_tests", "corner_evals", "lb_tests", "point_tests",
                    "disk_tests", "center_membership_tests"):
            bucket[key] += int(row[key])
        gate_form_evals = 4 * (int(row["gate_sites"]) + int(row["exclude_gate_sites"]))
        bucket["direct_gate_form_evals"] += gate_form_evals
        bucket["total_form_evals"] += int(row["corner_evals"]) + int(row["point_tests"]) + gate_form_evals
        bucket["wall_ms"] += float(row["wall_ms"])
        bucket["gate_ms"] += float(row["gate_ms"])
        walls.append(float(row["wall_ms"]))
        cpus.append(float(row["cpu_ms"]))
        for key in cost_fields:
            costs[key] += int(row[key])
        core = mask & ~post
        for name, lane in (("q3", 2), ("q4", 4)):
            if not mask & lane:
                continue
            block_yes = bool(proved & lane)
            core_yes = bool(core & lane)
            label = ("block_and_core" if block_yes and core_yes else
                     "block_only" if block_yes else "core_only" if core_yes else "neither")
            lane_counts[name][label] += 1
            lane_f[name][label] += f
        # These classes are per edge for the union of all requested bits.
        block_any = proved != 0
        core_any = core != 0
        label = ("block_and_core" if block_any and core_any else
                 "block_only" if block_any else "core_only" if core_any else "neither")
        class_counts[label] += 1
        class_f[label] += f
        if post == 0:
            core_closed += 1
            core_closed_f += f
        if proved == mask:
            block_closed += 1
            block_closed_f += f
        if post == 0 and proved == mask:
            both_closed += 1
            both_closed_f += f
        if post != 0 and proved == mask:
            block_only_closed += 1
            block_only_closed_f += f
    need(complete == len(rows), "incomplete edge in closed pilot")
    need(costs["corner_evals"] == 32 * costs["corner_tests"] ==
         32 * costs["node_tests"], "32-corner work ledger")
    need(costs["gate_nodes"] == costs["credited_nodes"] and
         costs["gate_sites"] == costs["credited_sites"] and costs["gate_nodes"] > 0,
         "not every credited node/site passed direct enumeration")
    need(costs["exclude_gate_nodes"] > 0 and costs["exclude_gate_sites"] > 0,
         "exclusion gate unexercised")
    need(block_closed_f == shadow["eligible_core_skip_F"] and
         block_closed == shadow["full_closed_edges"], "join mismatch")
    total_gate_forms = 4 * (costs["gate_sites"] + costs["exclude_gate_sites"])
    total_forms = costs["corner_evals"] + costs["point_tests"] + total_gate_forms
    need(sum(value["total_form_evals"] for value in by_stratum.values()) == total_forms,
         "per-stratum exact evaluation ledger")
    return {
        "schema": "mhgp9_b_spatial_block_bounded_pilot_k5_v1",
        "scope": "audit_only_no_ground_quarter_08_000200_K5_s8",
        "selection": "splitmix64_deterministic_24_strata_mask_F_core_status_first10_then_fill256",
        "selected": 256, "visited": len(rows), "complete": complete,
        "visited_strata": dict(sorted(strata.items())),
        "by_stratum": {key: {**value, "wall_ms": round(value["wall_ms"], 3),
                             "gate_ms": round(value["gate_ms"], 3)}
                       for key, value in sorted(by_stratum.items())},
        "sample_F": sample_f,
        "trace_sum_F": receipt["sum_F"],
        "trace_core_closed_F": receipt["core_closed_F"],
        "work": costs,
        "exact_form_evaluations": {
            "spatial_corners": costs["corner_evals"],
            "fixed_center_sites": costs["point_tests"],
            "direct_gate_sites_x4": total_gate_forms,
            "total": total_forms,
        },
        "wall_ms_sum_edges": round(sum(walls), 3),
        "cpu_ms_sum_edges": round(sum(cpus), 3),
        "gate_ms_sum_edges": round(sum(float(row["gate_ms"]) for row in rows), 3),
        "wall_ms_median_edge": round(statistics.median(walls), 3),
        "full_closed": {
            "block_edges": block_closed, "block_F": block_closed_f,
            "core_edges": core_closed, "core_F": core_closed_f,
            "both_edges": both_closed, "both_F": both_closed_f,
            "block_only_edges": block_only_closed, "block_only_F": block_only_closed_f,
        },
        "any_bit_classes_edges": class_counts,
        "any_bit_classes_F": class_f,
        "lane_classes_edges": lane_counts,
        "lane_classes_F": lane_f,
        "shadow_join": shadow,
        "sha256": {"detail": sha256(detail_path), "shadow": sha256(shadow_path)},
        "limits": "single_1288_site_S2_trace_256_selected_no_G4_no_full_frame_no_product_speedup",
    }


def main():
    if len(sys.argv) != 4:
        print("usage: summarize.py detail.tsv shadow.tsv summary.json", file=sys.stderr)
        return 2
    try:
        out = Path(sys.argv[3])
        need(not out.exists(), "summary output exists")
        summary = summarize(Path(sys.argv[1]), Path(sys.argv[2]))
        out.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n")
        print(json.dumps({"full_closed": summary["full_closed"],
                          "work": summary["work"], "visited": summary["visited"]},
                         sort_keys=True))
    except (OSError, ValueError, KeyError) as exc:
        print(f"b_spatial_block_summary: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
