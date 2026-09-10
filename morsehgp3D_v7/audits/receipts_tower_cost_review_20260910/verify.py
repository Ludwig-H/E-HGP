"""Recompute storage bounds from sealed captures, without running the engine."""
from __future__ import annotations

import copy
import hashlib
import json
from pathlib import Path
import tarfile

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
RUNS = ROOT / "morsehgp3D_v7/receipts/full_ball_runs_20260910"
PARENTS = ROOT / "morsehgp3D_v7/receipts/coverage_parent_array_20260910"


def need(value: object, reason: str) -> None:
    if not value:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path: Path) -> object:
    return json.loads(path.read_text())


def derive(row: dict, abi: dict) -> dict:
    need(all(type(v) is int and v >= 0 for k, v in row.items()
             if k in ("n", "orders", "nodes", "parent_refs", "contributions", "balls",
                      "anchor_blocks", "representatives", "anchor_hits", "intruder_queries",
                      "resolver_meb_calls", "lower_edges_indexed", "lower_nodes_activated",
                      "lower_edges_activated", "grouped_lots", "lot_dsu_slots", "extra_records")),
         "integer counts")
    need(row["status"] == "completed_relative" and row["extra_records"] == 0 and
         row["orders"] == row["kmax"] == 10, "regular completed ten-order scope")
    n, nodes, edges = row["n"], row["nodes"], row["parent_refs"]
    need(edges == nodes - 10, "one final root per order")
    need(row["lower_nodes_activated"] == row["lower_edges_indexed"] + 9 and
         row["lower_edges_activated"] == row["lower_edges_indexed"], "complete lower histories")
    need(row["resolver_meb_calls"] == row["anchor_hits"] + row["intruder_queries"],
         "one anchor hit or intruder per MEB iteration")
    need(row["singleton_lots"] + row["lot_dsu_slots"] == row["anchor_blocks"] and
         row["lot_dsu_slots"] >= 2 * row["grouped_lots"], "lot partition")
    last_nodes = nodes - row["lower_nodes_activated"]
    need(0 < last_nodes <= nodes and n <= row["contributions"] < nodes,
         "nonvacuous births and last order")
    # Every emitted action makes a node in this regular corpus. Initial K1
    # packs n actions; each grouped lot with m blocks saves at most m-1 batches.
    batches_min = nodes - n - (row["lot_dsu_slots"] - row["grouped_lots"]) + 1
    need(0 < batches_min <= nodes - n + 1, "batch lower bound")
    # by_key + population_ids + lower_anchors, programs, last History and
    # compressed. All remain allocated at the end of the ten-order loop.
    dead = ((abi["u32"] + 2 * abi["u64"]) * row["balls"] +
            abi["u32"] * row["anchor_blocks"] +
            (abi["ExactLevel"] + 2 * abi["u64"]) * last_nodes)
    return dict(n=n, nodes=nodes, births=row["contributions"],
                k10_nodes=last_nodes, draft_batches_lower_bound=batches_min,
                draft_metadata_bytes_lower_bound=(abi["FullCoverageBatch"] * batches_min +
                                                 abi["FullCoverageAction"] * nodes),
                dead_working_arrays_logical_bytes=dead,
                birth_records_logical_bytes=abi["FullDatedContribution"] * row["contributions"],
                selected_output_logical_bytes=(abi["FullNode"] * nodes + abi["u64"] * edges +
                                              2 * abi["u64"] * nodes +
                                              abi["FullDatedContribution"] * row["contributions"]),
                engine_reexecuted=False, rss_reduction_measured=False)


def compute() -> dict:
    pins = read(BASE / "input_pins.json")
    for packet in (RUNS, PARENTS):
        manifest = packet / "manifest.json"
        need(sha(manifest) == pins[str(manifest.relative_to(ROOT))], "constructor manifest pin")
        for path, digest in read(manifest).items():
            need(sha(packet / path) == digest, "constructor capture: " + path)
    archived = {}
    with tarfile.open(RUNS / "snapshot/snapshot.tar.gz", "r:gz") as archive:
        for member in archive.getmembers():
            need(member.isfile() and member.name not in archived, "source archive shape")
            archived[member.name] = hashlib.sha256(archive.extractfile(member).read()).hexdigest()
    need(archived == read(RUNS / "snapshot/source_manifest.json"), "source archive hashes")
    for name, digest in pins.items():
        if "/receipts/" not in name:
            source = PARENTS / "snapshot" / name
            observed = sha(source) if source.is_file() else archived.get(name)
            need(observed == digest, "source authority: " + name)
    before = read(RUNS / "local/sources_before.json")
    need(before == read(RUNS / "local/sources_after.json") and
         all(archived.get(k) == v for k, v in before.items()), "measured source stability")
    layout = read(BASE / "layout_capture/receipt.json")
    for name, digest in layout["source_pins_before_after"].items():
        observed = sha(ROOT / name) if name.endswith("/layout.cpp") else archived.get(name)
        need(observed == digest, "layout source pin: " + name)
    commands = read(BASE / "layout_capture/commands.json")
    need([c["name"] for c in commands] == ["compiler", "compile", "run"] and
         all(c["exit_code"] == 0 and c["ended_ns"] >= c["started_ns"] for c in commands),
         "ABI probe closed")
    abi = read(BASE / "layout_capture/run.stdout")
    need(abi == dict(ExactLevel=48, FullNode=64, FullDatedContribution=80,
                     FullCoveragePopulation=48, FullCoverageBatch=80,
                     FullCoverageAction=48, FullCoverageRef=16, u64=8, u32=4), "qualified local ABI")
    rows = [read(RUNS / f"local/n{n}.stdout") for n in (8000, 16000, 32000)]
    results = [derive(row, abi) for row in rows]
    # Perturb decoded counters after integrity checks: these are data-reader
    # mutants, not forged sealed measurements and not C++ product mutants.
    killed = []
    for key in ("parent_refs", "lower_nodes_activated", "resolver_meb_calls", "lot_dsu_slots"):
        mutant = copy.deepcopy(rows[-1])
        mutant[key] += 1
        try:
            derive(mutant, abi)
        except ValueError:
            killed.append(key)
    need(len(killed) == 4, "counter mutants")
    need((PARENTS / "selftest_o2.stdout").read_bytes() ==
         (PARENTS / "selftest_san.stdout").read_bytes(), "parent gate O2/SAN equality")
    need((PARENTS / "selftest_parent_zero_mutant.stderr").read_text().strip() ==
         "FAIL arena.parent_value", "closed parent-array regression")
    return dict(schema="mhgp7-tower-cost-review-v1", abi=abi, runs=results,
                data_mutants_rejected=killed, parent_array_request="closed_by_d188e3de",
                authority="static_lifetimes_and_capture_counts_only",
                new_full_qualification=False, public_status="not_claimed", gcp_used=False)


def main() -> None:
    for line in (BASE / "SHA256SUMS").read_text().splitlines():
        digest, name = line.split("  ", 1)
        need(sha(BASE / name) == digest, "audit packet pin: " + name)
    report = compute()
    need(report == read(BASE / "review.json"), "recomputed review differs")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
