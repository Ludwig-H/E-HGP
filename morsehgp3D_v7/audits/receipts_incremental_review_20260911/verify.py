"""Recompute net journal storage from sealed historical inputs; no engine."""
from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
from typing import Any


BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
HISTORICAL = BASE.parent / "receipts_tower_cost_review_20260910"
RUNS = ROOT / "morsehgp3D_v7/receipts/full_ball_runs_20260910/local"


def need(condition: object, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path: Path) -> Any:
    return json.loads(path.read_text())


def check_manifest(base: Path) -> None:
    for line in (base / "SHA256SUMS").read_text().splitlines():
        digest, name = line.split("  ", 1)
        need(sha(base / name) == digest, "packet pin: " + str(base / name))


def compute() -> dict[str, Any]:
    pins = read(BASE / "input_pins.json")
    for name, digest in pins["files"].items():
        need(sha(ROOT / name) == digest, "immutable input pin: " + name)
    check_manifest(HISTORICAL)
    specification = importlib.util.spec_from_file_location(
        "incremental_historical_cost_reader", HISTORICAL / "verify.py")
    need(specification is not None and specification.loader is not None,
         "historical reader is importable")
    historical = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(historical)
    checked = historical.compute()
    need(checked == read(HISTORICAL / "review.json"), "historical review reproduced")
    abi = checked["abi"]
    output = []
    for previous in checked["runs"]:
        row = read(RUNS / f"n{previous['n']}.stdout")
        need(row["extra_records"] == 0 and row["orders"] == 10,
             "regular historical ten-order corpus")
        n, nodes = row["n"], row["nodes"]
        parents, contributions = row["parent_refs"], row["contributions"]
        batches_min = previous["draft_batches_lower_bound"]
        batches_max = nodes - n + 1
        need(0 < batches_min <= batches_max, "nonempty batch interval")
        need(batches_max - batches_min == row["lot_dsu_slots"] - row["grouped_lots"],
             "maximum packing savings of grouped lots")
        # Regular geometry gives T=0, actions=N, and contributions=births.
        # This hypothesis is NOT extended to arbitrary structural journals.
        final_bytes = ((abi["FullNode"] + abi["u64"]) * nodes +
                       abi["u64"] * parents + abi["FullDatedContribution"] * contributions)
        draft_bytes = [abi["FullCoverageBatch"] * batches +
                       abi["FullCoverageAction"] * nodes + abi["u64"] * parents +
                       abi["FullCoverageRef"] * contributions
                       for batches in (batches_min, batches_max)]
        net = [amount - final_bytes for amount in draft_bytes]
        need(net == [80 * batches - 24 * nodes - 64 * contributions
                     for batches in (batches_min, batches_max)],
             "expanded and simplified net storage agree")
        output.append(dict(n=n, orders=10, nodes=nodes, parent_refs=parents,
                           contributions=contributions, birth_nodes=contributions,
                           published_continuations=0, published_actions=nodes,
                           draft_batches_interval=[batches_min, batches_max],
                           draft_logical_bytes_interval=draft_bytes,
                           final_arena_logical_bytes=final_bytes,
                           net_draft_minus_final_logical_bytes_interval=net))
    need(len(output) == 3 and [row["n"] for row in output] == [8000, 16000, 32000],
         "historical triplet is complete")
    return dict(
        schema="mhgp7-incremental-journal-review-v1",
        status="passed",
        verification_scope="sealed_inputs_and_net_logical_storage_recalculation_only",
        reviewed_design=pins["reviewed_design"],
        historical_measurements="full_ball_runs_20260910; d188e3de source lineage",
        historical_integrity_reader="receipts_tower_cost_review_20260910/verify.py::compute",
        abi=abi,
        runs=output,
        excluded_storage=["bank", "vertical_refs", "histories", "shared_catalogue", "index",
                          "live", "scratchs", "vector_slack", "allocator_metadata",
                          "reallocation_overlap"],
        comparison_boundary="after last order, before immutable population bank copy",
        temporal_stability="proved_in_README_not_an_executed_incremental_gate",
        proposed_gate="prefix_vs_append_at_old_open_and_closed_cuts",
        incremental_implementation_executed=False,
        new_oracle_executed=False,
        new_engine_measurement=False,
        rss_reduction_measured=False,
        public_status="not_claimed",
        gcp_used=False,
    )


def main() -> None:
    check_manifest(BASE)
    report = compute()
    need(report == read(BASE / "review.json"), "net storage review differs")
    print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
