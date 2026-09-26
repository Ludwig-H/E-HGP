#!/usr/bin/env python3
"""Derive FULL work/output and phase-window tables from raw R24-B JSON."""
import argparse
import hashlib
import json
from pathlib import Path
from statistics import median

from event_model import check, need

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
RECEIPT = ROOT / "morsehgp3D_v9/receipts/g4_tower_r24b_20260926"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    def unique(items):
        result = {}
        for key, value in items:
            need(key not in result, "duplicate JSON key")
            result[key] = value
        return result
    return json.loads(path.read_text(), object_pairs_hook=unique)


def build():
    package = read(RECEIPT / "PACKAGE.json")
    rows = []
    input_hashes = {}
    identities = {}
    for index, case in enumerate(package["cases"]):
        path = RECEIPT / "vm" / ("probe_%d.stdout" % index)
        command = read(RECEIPT / "vm" / ("probe_%d.command.json" % index))
        need(command["exit_code"] == 0 and sha(path) == command["stdout_sha256"], "raw R24-B command pin")
        probe = read(path)
        need(probe["status"] == "complete_relative" and probe["options"]["levers"] == case["levers"], "case identity")
        input_hashes[str(path.relative_to(ROOT))] = sha(path)
        orders, work, phase = probe["orders"], probe["tower_work"], probe["tower_phases_ms"]
        need([order["K"] for order in orders] == list(range(1, case["k"] + 1)), "explicit complete K tower")
        totals = {name: sum(order[name] for order in orders)
                  for name in ("nodes", "births", "merges", "parents", "contributions")}
        need(totals["nodes"] == totals["births"] + totals["merges"] and
             all(totals[name] == work[name] for name in ("births", "merges", "contributions")), "output identities")
        need(totals["parents"] == totals["nodes"] - case["k"], "one root per full order")
        identity = (probe["tower_digest"], probe["catalogue_digest"], totals)
        key = (case["scene"], case["k"])
        if key in identities:
            need(identities[key] == identity, "output differs across R24-B levers or s")
        identities[key] = identity
        lever = case["levers"]
        arm = ("engine" if not lever["q34_batch_filter"] else "C_no_overlap" if not lever["tower_overlap_static"]
               else "B_no_tail" if not lever["tower_pipelined_tail"] else "A_full_overlap")
        # These are nominal ready times from recorded phase-0 durations, not
        # timestamps. Scheduling overhead/contention can change under a port.
        ready = [sum(phase["static_by_k"][k:]) for k in range(case["k"])]
        ends = [ready[k] + phase["lots_by_k"][k] for k in range(case["k"])]
        nominal_window = max(ends)
        eliminate_top = max(ends[:-1] + [ready[-1]])
        all_half = max(ready[k] + phase["lots_by_k"][k] / 2 for k in range(case["k"]))
        timed = {name: sum(phase[name]) for name in ("static_collect_by_k", "static_sort_by_k",
                                                   "static_groups_by_k", "static_resolve_by_k")}
        rows.append(dict(index=index, scene=case["scene"], Kmax=case["k"], s=case["s"], repeat=case["repeat"],
                         arm=arm, sites=case["n"], chain_ms=probe["times_ms"]["chain_total"],
                         tower_ms=probe["times_ms"]["tower"], phases=phase, static_parts_sum_ms=timed,
                         output_totals=totals, output_by_k=orders, work=work,
                         regular_ball_count=work["records"]-work["extra_records"],
                         representative_per_output_node=work["representatives"] / totals["nodes"],
                         # Only these three explicit u64 arrays, not total output.
                         parent_successor_vertical_bytes=8 * (totals["parents"] + 2 * totals["nodes"]),
                         anchor_dense_current_logical_bytes=4 * work["records"] * case["k"],
                         overlap_model=(dict(ready_nominal_by_k=ready, A_end_nominal_by_k=ends,
                             nominal_window_ms=nominal_window, measured_static_plus_exposed_A_ms=phase["static"]+phase["lots"],
                             eliminate_top_A_only_fixed_ready_gain_ms=nominal_window-eliminate_top,
                             halve_all_A_fixed_ready_gain_ms=nominal_window-all_half)
                             if lever["tower_overlap_static"] else None)))
    groups = []
    for scene, kmax in sorted(identities):
        for arm in ("A_full_overlap", "B_no_tail", "C_no_overlap"):
            selected = [r for r in rows if r["scene"] == scene and r["Kmax"] == kmax and r["arm"] == arm
                        and r["s"] == 8]
            if selected:
                need(len(selected) == 2 and {r["repeat"] for r in selected} == {0, 1}, "paired lever repetitions")
                groups.append(dict(scene=scene, Kmax=kmax, arm=arm, count=len(selected),
                    chain_ms=[r["chain_ms"] for r in selected], tower_ms=[r["tower_ms"] for r in selected],
                    tower_median_ms=median(r["tower_ms"] for r in selected),
                    phase_A_by_k=[r["phases"]["lots_by_k"] for r in selected],
                    phase_zero_ms=[r["phases"]["static"] for r in selected],
                    output_totals=selected[0]["output_totals"], work=selected[0]["work"]))
    sources = ["src/tower/forest/full_ball_tower.hpp", "src/tower/forest/full_coverage_certificate.hpp",
               "src/tower/forest/local_plateau.hpp", "src/tower/forest/anchor_meb.hpp"]
    return dict(schema="mhgp9_audit_b_full_phase_a_work_v1", public_status="not_claimed", GCP_used=False,
                # Fixed audit base, not the caller's HEAD after publication.
                # Current product hashes remain explicit below.
                source_commit="52ff41802a9c77e8495aee57ab732da34d1a225e",
                audited_source_hashes={name: sha(ROOT / "morsehgp3D_v9" / name) for name in sources},
                raw_receipt_hashes=input_hashes, R24B_source_commit=package["commit"], rows=rows, groups=groups,
                model=check(), complexity_scope="event sidecar after exact static terminal resolution only",
                output_bytes_scope="u64 parents, successors, vertical arrays only; excludes nodes/levels/contributions/bank",
                warning="fixed-ready overlap counterfactuals are not predicted timings or measured speedups")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    data = json.dumps(build(), sort_keys=True, indent=2) + "\n"
    if args.output:
        with args.output.open("x") as stream:
            stream.write(data)
    else:
        print(data, end="")
