#!/usr/bin/env python3
"""Capture coarse/donated front q2 work with independent scheduling ledgers.

Explicit protocol port of run_wspd_q2_parallel_matrix.py at 4c0bfe1e.
Previous schemas, results and timings are not reinterpreted by this reader.
"""

from __future__ import annotations

import argparse
import base64
from collections import defaultdict
from datetime import datetime
import itertools
import json
import os
from pathlib import Path
import platform
import re
import signal
import statistics
import subprocess
import sys
from typing import Any

from check_paired_campaign import PROVENANCE_POLICY, provenance, sha256
from paired_receipts import close, finite_times, fingerprint
from run_p0_matrix import digest, invoke, on_signal, parse_result, require, uint, utc_stamp, write_json



# Field contracts reused, not prior results or timings. Their transitive sources are pinned.
from run_wspd_q2_matrix import (CENSUS_FIELDS, CLOUD_FIELDS, INDEX_FIELDS, GENERATION_FIELDS,
    FRONT_FIELDS, FRONT_ARRAYS, CALLBACK_FIELDS, SIBLING_FIELDS, ORDER_FIELDS, JOINT_FIELDS,
    POOL_FIELDS, RECIPES, counters, array, timestamp)


ROOT = Path(__file__).resolve().parents[2]
RUNNER_SOURCE = "morsehgp3D_v8/bench/run_wspd_q2_dynamic_matrix.py"
SCHEMA = "mhgp8_wspd_q2_dynamic_campaign_v1"
SCOPE = "q2_all_cloud_supports_not_full"
FAMILIES = ("uniform", "terrain", "clusters", "rows")
MODES, CENSUS_MODES = ("samples",), ("shared",)
KEYS = ("family", "n", "kmax", "s", "seed", "threads", "jobs_per_worker", "schedule",
        "queue_capacity", "donation_interval", "pool_min_factor")
MATRIX_KEYS = ("families", "sizes", "kmax", "separations", "seeds", "thread_counts",
               "jobs_per_worker", "schedules", "queue_capacities", "donation_intervals", "pool_min_factors")
RECEIPT_FILES = ("MANIFEST.json", "MEASURES.jsonl", "COMPLETION.json")
MEMORY_FIELDS = ("input_capacity_bytes", "cloud_retained_bytes", "index_retained_bytes",
                 "callback_buffers_capacity_bytes", "callback_state_bytes", "callback_slots")
TIMES = ("generation_ms", "load_ms", "cloud_ms", "index_ms", "pipeline_and_callback_ms", "validation_ms",
         "destruction_ms", "total_ms", "pipeline_wall_ms", "partition_ms", "worker_ms_sum", "payload_ms_sum")
PARALLEL_FIELDS = ("requested_workers", "started_workers", "target_jobs", "jobs", "completed_jobs",
                   "terminal_jobs", "prefix_product_visits", "job_storage_bytes", "pool_peak_bytes_sum", "queue_storage_bytes")
DISPATCH_FIELDS = ("seeds_started", "seeds_completed", "donations", "donor_checks", "offer_attempts",
                   "offer_full", "offer_busy", "offer_no_demand", "stolen_started", "stolen_completed",
                   "waits", "wakes", "max_queue_size", "max_local_stack_size")
WORKER_FIELDS = ("slot", "jobs", "front_products", "input_rectangles", "count_node_visits",
                 "supports", "pool_peak_bytes", "callback_buffers_capacity_bytes")
POOL_TIMES = ("preparation_ms_sum", "selected_total_ms_sum")
DISCRETE_FIELDS = ("recipe", "seed_affects_input", "input_hash", "input_kind", "input_path", "input_bytes",
                   "input_work", "total_unordered_pairs", "active_lane_mask",
                   "generation_work", "cloud_work", "index_work", "front_work", "census_work",
                   "sibling_work", "order_work", "joint_work", "callback_work", "digest",
                   "input_rectangles", "anchor_queries", "candidate_pairs", "accepted_pairs", "rejected_pairs")
FIXED = dict(schema="mhgp8_wspd_q2_dynamic_probe_v1", status="completed", scope=SCOPE,
    phase="exploration_v8_hors_registre", backend="cpu_reference", profile="quantized_u16_input_only",
    mode="implementation_v8_p0", public_status="not_claimed", separation_convention="box_gap_diameter_v1",
    front_mode="samples", census_mode="shared", sibling_mode="sibling", witness_order="complement",
    anchor_mode="anchors", clock_contract="wall_and_worker_sums_no_subtraction_v1")


def sources() -> dict[str, str]:
    source = ROOT / "morsehgp3D_v8"
    bench = source / "bench"
    # Includes all local Python helpers and both explicit port sources. No
    # caller-selected pin subset, external audit code, or mutable build import.
    paths = [source / "CMakeLists.txt", *sorted((source / "src").rglob("*.hpp")),
             *sorted((source / "src").rglob("*.cpp")), *sorted((source / "cmake").rglob("*.cmake")),
             *sorted(bench.glob("*.hpp")), *sorted(bench.glob("*.py")),
             bench / "wspd_q2_census_probe.cpp", bench / "wspd_q2_dynamic_probe.cpp",
             source / "tests/wspd_q2_dynamic_receipts_gate.py"]
    return {str(path.relative_to(ROOT)): digest(path) for path in paths}


def command_for(probe: str, key: tuple[Any, ...]) -> list[str]:
    family, n, kmax, separation, seed, threads, jobs, schedule, capacity, interval, pool = key
    return [probe, str(n), family, str(kmax), str(separation), str(seed), str(threads), str(jobs),
            schedule, str(capacity), str(interval), str(pool)]


def is_file(family: str) -> bool:
    return type(family) is str and family.startswith("file:")


def input_pin(family: str) -> dict[str, Any]:
    require(is_file(family), "not a file input")
    path = Path(family[5:])
    require(path.is_absolute() and all(ord(char) >= 32 for char in str(path)), "invalid input path")
    data = path.read_bytes()
    require(len(data) >= 12 and len(data) % 6 == 0, "input file is not complete xyz_u16le")
    n = len(data) // 6
    require(n <= 1 << 48, "file exceeds unique u16 physical domain")
    hashed = 14695981039346656037
    mask = (1 << 64) - 1
    def word(value: int) -> None:
        nonlocal hashed
        for _ in range(8):
            hashed = ((hashed ^ (value & 255)) * 1099511628211) & mask
            value >>= 8
    word(1)
    word(n)
    for offset in range(0, len(data), 2):
        word(data[offset] | (data[offset + 1] << 8))
    import hashlib
    return dict(path=str(path), n=n, bytes=len(data), sha256=hashlib.sha256(data).hexdigest(), input_hash=format(hashed, "x"))


def input_pins(families: list[str]) -> dict[str, dict]:
    return {family: input_pin(family) for family in families if is_file(family)}


def input_hashes(pins: dict[str, dict]) -> dict[str, str]:
    return {family: digest(Path(pin["path"])) for family, pin in pins.items()}


def matrix(manifest: dict[str, Any]) -> list[tuple[Any, ...]]:
    require(uint(manifest.get("repeats"), "repeats") >= 1, "empty repetitions")
    for name in MATRIX_KEYS:
        values = manifest.get(name)
        require(type(values) is list and bool(values), f"{name}: empty matrix")
        for value in values:
            if name == "families":
                require(type(value) is str and (value in FAMILIES or is_file(value)), "invalid family")
                if is_file(value):
                    require(Path(value[5:]).is_absolute() and all(ord(char) >= 32 for char in value), "invalid file family")
            elif name == "schedules":
                require(type(value) is str and value in ("coarse", "donate"), "invalid schedule")
            else:
                uint(value, name)
                minimum = 2 if name == "sizes" else 0 if name in ("seeds", "thread_counts", "pool_min_factors") else 1
                require(value >= minimum and (name != "kmax" or value <= 10) and
                        (name != "separations" or value < 1 << 32), f"{name}: invalid value")
        require(len(values) == len(set(values)), f"{name}: duplicate value")
    for workers, jobs in itertools.product(manifest["thread_counts"], manifest["jobs_per_worker"]):
        require(workers * jobs <= (1 << 64) - 1, "job target overflow")
    files = [family for family in manifest["families"] if is_file(family)]
    if files:
        require(len(files) == len(manifest["families"]) and manifest["seeds"] == [0],
                "file and synthetic inputs cannot share one campaign; file seed is zero")
        sizes = manifest.get("file_sizes")
        require(type(sizes) is dict and set(sizes) == set(files), "missing per-file sizes")
        for family, n in sizes.items():
            require(2 <= uint(n, "file size") <= 1 << 48, "invalid file size")
        require(set(manifest["sizes"]) == set(sizes.values()), "file size axis differs from inputs")
        cases = [(family, sizes[family], *tail) for family in files for tail in
                 itertools.product(*(manifest[name] for name in MATRIX_KEYS[2:]), range(manifest["repeats"]))]
    else:
        require(manifest.get("file_sizes", {}) == {}, "synthetic campaign has file sizes")
        cases = list(itertools.product(*(manifest[name] for name in MATRIX_KEYS), range(manifest["repeats"])))
    for family, n, *_ in cases:
        if is_file(family):
            continue
        limit = dict(uniform=1 << 48, terrain=1 << 40, clusters=1 << 33, rows=131072)[family]
        require(n <= limit and (family != "rows" or n % 2 == 0), "fixture outside physical domain")
    return cases


def validate_digest(value: Any, callback: Any, name: str, kmax: int, n: int) -> None:
    require(type(value) is dict and set(value) == {"encoding", "supports", "interior_ids", "shell_ids", "sum", "xor"}
            and value["encoding"] == "canonical_q2_support_v2", f"{name}: wrong digest")
    for key in ("supports", "interior_ids", "shell_ids"):
        uint(value[key], name + key)
    fingerprint(value["sum"], name + ".sum")
    fingerprint(value["xor"], name + ".xor")
    counters(callback, CALLBACK_FIELDS, name + ".callback")
    supports, interiors, shells = (value[key] for key in ("supports", "interior_ids", "shell_ids"))
    require(interiors <= (kmax - 1) * supports and 2 * supports <= shells <= n * supports,
            f"{name}: bad payload cardinalities")
    payload = interiors + shells
    require(callback["copied_ids"] == callback["validation_ids"] == payload and
            callback["sort_calls"] == 2 * supports and callback["support_key_axis_checks"] == 3 * supports and
            callback["hash_words"] == 9 * supports + payload and
            payload - 2 * supports <= callback["adjacent_tests"] <= payload - supports and
            callback["cross_set_comparisons"] <= payload, f"{name}: unpaid/inconsistent callback")


def validate_dispatch(row: dict[str, Any]) -> None:
    active = row["threads"] > 0 and row["schedule"] == "donate"
    parallel, total, workers = row["parallel_work"], row["dispatch_work"], row["workers"]
    for work in [total, *(worker["dispatch_work"] for worker in workers)]:
        counters(work, DISPATCH_FIELDS, "dispatch_work")
        require(work["seeds_started"] == work["seeds_completed"] and
                work["stolen_started"] == work["stolen_completed"] and
                work["donor_checks"] == work["offer_attempts"] + work["offer_no_demand"] and
                work["offer_attempts"] == work["donations"] + work["offer_full"] + work["offer_busy"] and
                work["waits"] == work["wakes"] and work["max_queue_size"] <= row["queue_capacity"] and
                work["max_local_stack_size"] <= 97, "dispatcher conservation or capacity mismatch")
        if not active:
            require(all(value == 0 for value in work.values()), "inactive dispatcher paid work")
    for key in DISPATCH_FIELDS:
        values = [worker["dispatch_work"][key] for worker in workers]
        require(total[key] == (max(values, default=0) if key.startswith("max_") else sum(values)),
                "dispatcher worker reduction lost " + key)
    if active:
        require(total["seeds_completed"] == parallel["jobs"] and
                total["donations"] == total["stolen_completed"] and
                parallel["queue_storage_bytes"] >= row["queue_capacity"] and
                all(worker["jobs"] == worker["dispatch_work"]["seeds_completed"] for worker in workers),
                "dispatcher lost seeds/donations/queue memory")
        if parallel["started_workers"] == 1:
            require(all(total[key] == 0 for key in DISPATCH_FIELDS[2:13]),
                    "single worker performed donation/wait work")
    else:
        require(parallel["queue_storage_bytes"] == 0, "inactive dispatcher allocated a queue")


def validate_result(row: dict[str, Any], command: list[str]) -> None:
    expected_fields = {*FIXED, *KEYS, *DISCRETE_FIELDS, "gcp_used", "execution", "parallel_work", "dispatch_work",
                       "pool_work", "workers", "memory", "timings"}
    require(set(row) == expected_fields, "missing or unknown parallel probe fields")
    for key, value in FIXED.items():
        require(row.get(key) == value, f"{key}: unsupported contract")
    require(row["gcp_used"] is False and len(command) == 12, "wrong cloud use or command")
    for key in KEYS:
        if key not in ("family", "schedule"):
            uint(row[key], key)
    expected = (command[2], int(command[1]), int(command[3]), int(command[4]), int(command[5]),
                int(command[6]), int(command[7]), command[8], int(command[9]), int(command[10]), int(command[11]))
    require(tuple(row[key] for key in KEYS) == expected, "command/result tuple mismatch")
    matrix(dict(zip(MATRIX_KEYS, ([row[key]] for key in KEYS)), repeats=1,
                file_sizes={row["family"]: row["n"]} if is_file(row["family"]) else {}))
    require(row["execution"] == ("mono_reference" if row["threads"] == 0 else "parallel_front"),
            "wrong reference/parallel execution mode")
    for key in ("total_unordered_pairs", "active_lane_mask"):
        uint(row[key], key)
    pool = row["pool_work"]
    require(type(pool) is dict and set(pool) == {*POOL_FIELDS, *POOL_TIMES}, "wrong Pool fields")
    counters({key: pool[key] for key in POOL_FIELDS}, POOL_FIELDS, "pool_work")
    finite_times(pool, POOL_TIMES)
    validate_geometry(row, pool)
    require(pool["selected_rectangles"] <= row["input_rectangles"] and
            pool["passthrough_rectangles"] <= pool["selected_rectangles"] and
            pool["passthrough_anchors"] <= pool["original_selected_anchors"] <= row["anchor_queries"] and
            pool["passthrough_pairs"] <= pool["residual_pairs"] <= pool["selected_pairs"] and
            pool["selected_pairs"] <= row["front_work"]["residual_pair_mass"][0] and
            pool["filtered_pairs"] == pool["selected_pairs"] - pool["residual_pairs"] and
            pool["pair_roots"] == pool["residual_pairs"] - pool["passthrough_pairs"],
            "Pool selected or passthrough mass mismatch")
    fsites = pool["factor_sites"]
    require(pool["factor_read_visits"] == pool["grouping_visits"] == 2 * fsites and
            pool["prefix_class_visits"] == (row["kmax"] + 1) * pool["selected_rectangles"] and
            pool["bands"] <= row["kmax"] * pool["selected_rectangles"] and
            pool["bands"] <= pool["selected_anchors"] <= pool["original_selected_anchors"] - pool["passthrough_anchors"] and
            pool["selected_anchors"] <= pool["pair_roots"] and
            (pool["selected_anchors"] == 0) == (pool["pair_roots"] == 0) and
            pool["selection_tests"] <= (row["kmax"] + 1) * fsites and
            pool["witness_attempts"] <= (row["kmax"] + 1) * fsites and
            pool["universal_queries"] <= pool["witness_attempts"] and
            pool["q2_axis_terms"] == 3 * pool["universal_queries"] and
            pool["pool_insertions"] <= fsites and
            pool["pool_shifted_entries"] <= (row["kmax"] + 1) * pool["pool_insertions"],
            "Pool work or band accounting mismatch")
    if row["pool_min_factor"] == 0 or pool["selected_rectangles"] == 0:
        require(all(value == 0 for value in pool.values()), "disabled/unselected Pool performed work")
    census, sibling, order = row["census_work"], row["sibling_work"], row["order_work"]
    counters(sibling, SIBLING_FIELDS, "sibling_work")
    counters(order, ORDER_FIELDS, "order_work")
    counters(row["joint_work"], JOINT_FIELDS, "joint_work")
    require(all(value == 0 for value in row["joint_work"].values()), "fixed individual policy used joint work")
    require(sibling["proposals"] == 2 * census["query_splits"] and
            sibling["cardinality_skips"] + sibling["bound_tests"] == sibling["proposals"] and
            sibling["rejected_tasks"] <= sibling["bound_tests"] and
            sibling["rejected_tasks"] <= sibling["rejected_pairs"] <= row["rejected_pairs"] and
            sibling["rejected_after_credit"] <= sibling["rejected_tasks"] and
            (sibling["rejected_tasks"] == 0) == (sibling["rejected_pairs"] == 0), "sibling accounting mismatch")
    require(order["structural_splits"] <= 96 * census["query_tasks"] and
            all(order[key] <= census["query_tasks"] for key in ORDER_FIELDS[1:]), "order accounting mismatch")
    parallel = row["parallel_work"]
    counters(parallel, PARALLEL_FIELDS, "parallel_work")
    require(parallel["requested_workers"] == row["threads"] and
            parallel["completed_jobs"] == parallel["jobs"], "requested workers or completed jobs mismatch")
    workers = row["workers"]
    require(type(workers) is list and len(workers) == parallel["started_workers"], "missing worker slots")
    if row["threads"] == 0:
        require(parallel["started_workers"] == 1 and all(parallel[key] == 0 for key in
                ("target_jobs", "jobs", "completed_jobs", "terminal_jobs", "prefix_product_visits", "job_storage_bytes")),
                "mono reference reported scheduler work")
    else:
        require(parallel["target_jobs"] == row["threads"] * row["jobs_per_worker"] and
                1 <= parallel["jobs"] <= parallel["target_jobs"] + 2 and
                parallel["started_workers"] == min(row["threads"], parallel["jobs"]) and
                parallel["terminal_jobs"] <= parallel["jobs"] and
                parallel["terminal_jobs"] <= parallel["prefix_product_visits"] <= row["front_work"]["product_visits"] and
                parallel["job_storage_bytes"] >= parallel["jobs"], "invalid parallel partition")
    for i, worker in enumerate(workers):
        require(type(worker) is dict and set(worker) == {*WORKER_FIELDS, "elapsed_ms", "payload_ms", "callback_work", "digest", "dispatch_work"},
                "wrong worker fields")
        counters({key: worker[key] for key in WORKER_FIELDS}, WORKER_FIELDS, "worker")
        finite_times(worker, ("elapsed_ms", "payload_ms"))
        require(worker["slot"] == i and worker["elapsed_ms"] + 1e-6 >= worker["payload_ms"], "worker slot/clock mismatch")
        validate_digest(worker["digest"], worker["callback_work"], "worker.digest", row["kmax"], row["n"])
        require(worker["supports"] == worker["digest"]["supports"] and
                (worker["supports"] == 0 or worker["callback_buffers_capacity_bytes"] >= 16), "worker payload mismatch")
    for per_worker, expected_total in (("jobs", parallel["jobs"]), ("front_products",
            row["front_work"]["product_visits"] - parallel["prefix_product_visits"]),
            ("input_rectangles", row["input_rectangles"]), ("count_node_visits", census["count_node_visits"]),
            ("supports", row["accepted_pairs"]), ("pool_peak_bytes", parallel["pool_peak_bytes_sum"]),
            ("callback_buffers_capacity_bytes", row["memory"]["callback_buffers_capacity_bytes"])):
        require(sum(worker[per_worker] for worker in workers) == expected_total, "worker reduction lost " + per_worker)
    require(max((worker["pool_peak_bytes"] for worker in workers), default=0) == pool["plan_peak_bytes"],
            "maximum plan storage differs from worker maxima")
    for key in CALLBACK_FIELDS:
        require(sum(worker["callback_work"][key] for worker in workers) == row["callback_work"][key],
                "callback worker reduction lost " + key)
    for key in ("supports", "interior_ids", "shell_ids"):
        require(sum(worker["digest"][key] for worker in workers) == row["digest"][key], "digest reduction lost " + key)
    summed, xored = 0, 0
    for worker in workers:
        summed = (summed + int(worker["digest"]["sum"], 16)) & ((1 << 64) - 1)
        xored ^= int(worker["digest"]["xor"], 16)
    require(summed == int(row["digest"]["sum"], 16) and xored == int(row["digest"]["xor"], 16),
            "worker canonical digest reduction mismatch")
    require(row["memory"]["callback_slots"] == max(1, row["threads"]) and
            row["memory"]["callback_state_bytes"] >= row["memory"]["callback_slots"], "callback state storage missing")
    timings = row["timings"]
    require(type(timings) is dict and set(timings) == set(TIMES), "wrong parallel timing fields")
    finite_times(timings, TIMES)
    close(timings["total_ms"], sum(timings[key] for key in TIMES[:7]), "outer clock")
    close(timings["worker_ms_sum"], sum(worker["elapsed_ms"] for worker in workers), "worker intervals")
    close(timings["payload_ms_sum"], sum(worker["payload_ms"] for worker in workers), "worker payload")
    require(timings["pipeline_and_callback_ms"] + 1e-6 >= timings["pipeline_wall_ms"] >= timings["partition_ms"] and
            all(worker["elapsed_ms"] <= timings["pipeline_wall_ms"] + 1e-6 for worker in workers) and
            pool["preparation_ms_sum"] <= pool["selected_total_ms_sum"] + 1e-6 and
            pool["selected_total_ms_sum"] <= timings["worker_ms_sum"] + 1e-6,
            "worker/partition/Pool clocks escaped enclosing execution")
    if row["threads"] == 0:
        require(timings["partition_ms"] == 0, "mono paid a parallel partition")
        close(timings["pipeline_wall_ms"], timings["worker_ms_sum"], "mono elapsed interval")
    require(timings["generation_ms" if is_file(row["family"]) else "load_ms"] == 0,
            "file loading and synthetic generation clocks confused")
    validate_dispatch(row)


def cross_check(row: dict[str, Any], identities: dict, signatures: dict) -> None:
    identity = row["family"], row["n"], row["seed"] if row["seed_affects_input"] else None
    value = {name: row[name] for name in ("input_hash", "total_unordered_pairs", "generation_work", "cloud_work", "index_work")}
    value["memory"] = {name: row["memory"][name] for name in MEMORY_FIELDS[:3]}
    require(("input", *identity) not in identities or identities[("input", *identity)] == value,
            "same fixture changed across threads, K, s or repetition")
    identities[("input", *identity)] = value
    support_key = ("support", *identity, row["kmax"])
    require(support_key not in identities or identities[support_key] == row["digest"],
            "canonical complete supports changed across threads, s or Pool")
    identities[support_key] = row["digest"]
    key = (*identity, row["kmax"], row["s"], row["pool_min_factor"])
    signature = {name: row[name] for name in DISCRETE_FIELDS}
    signature["pool_work"] = {name: row["pool_work"][name] for name in POOL_FIELDS}
    require(key not in signatures or signatures[key] == signature,
            "integer geometric/callback work changed across threads, granularity or repeat")
    signatures[key] = signature


def validate_geometry(row: dict[str, Any], pool: dict[str, Any]) -> None:
    # Explicit port of the mono v5 integer ledgers, without its time contract.
    joint = None
    n, kmax, family = row["n"], row["kmax"], row["family"]
    require(n >= 2 and 1 <= kmax <= 10 and 1 <= row["s"] <= (1 << 32) - 1 and
            (family in FAMILIES or is_file(family)) and row["front_mode"] in MODES and row["census_mode"] in CENSUS_MODES, "unsupported tuple")
    file = is_file(family)
    require(row.get("recipe") == ("xyz_u16le_file_v1" if file else RECIPES[family]) and
            row.get("seed_affects_input") is (not file and family != "rows"),
            "wrong fixture recipe or seed contract")
    require(row["input_kind"] == ("file_u16le" if file else "synthetic") and
            row["input_path"] == (family[5:] if file else "") and
            uint(row["input_bytes"], "input_bytes") == (6 * n if file else 0), "input identity/length mismatch")
    counters(row["input_work"], ("bytes_read", "decoded_points", "hash_words"), "input_work")
    require(row["input_work"] == (dict(bytes_read=6 * n, decoded_points=n, hash_words=2 + 3 * n)
                                  if file else dict(bytes_read=0, decoded_points=0, hash_words=0)),
            "file input decoding/hashing work mismatch")
    if family == "rows":
        require(n % 2 == 0 and n <= 131072, "rows fixture outside its exact domain")
    fingerprint(row.get("input_hash"), "input_hash")
    total = n * (n - 1) // 2
    active = 1
    require(row["total_unordered_pairs"] == total and row["active_lane_mask"] == active,
            "wrong total pair count or active lanes")
    generation = row.get("generation_work")
    counters(generation, GENERATION_FIELDS, "generation_work")
    require(generation["accepted_points"] == (0 if file else n) and
            generation["proposed_points"] == (0 if file else n) + generation["duplicate_rejections"],
            "fixture generation lost its work or sites")
    if file:
        require(row["seed"] == 0 and all(value == 0 for value in generation.values()), "file input paid synthetic generation")
    elif family == "rows":
        require(generation["rng_calls"] == generation["duplicate_rejections"] ==
                generation["ordered_set_comparisons"] == 0, "deterministic rows paid random generation")
    else:
        require(generation["rng_calls"] == (4 if family == "clusters" else 3) * generation["proposed_points"] and
                generation["ordered_set_comparisons"] > 0, "random generation lost RNG or uniqueness work")
    cloud, index = row.get("cloud_work"), row.get("index_work")
    counters(cloud, CLOUD_FIELDS, "cloud_work")
    counters(index, INDEX_FIELDS, "index_work")
    require(cloud["coordinate_copies"] == cloud["validation_points"] == cloud["range_tree_leaf_visits"] == n and
            cloud["range_tree_nodes"] == 2 * n - 1 and
            cloud["range_tree_merges"] == cloud["uniqueness_adjacent_tests"] == n - 1 and
            cloud["uniqueness_comparisons"] > 0, "shared cloud work changed")
    require(index["nodes"] == index["escape_links"] == 2 * n - 1 and
            1 <= index["max_depth"] <= 48 and n <= index["point_visits"] <= 97 * n,
            "global index outside its representation bounds")
    work = row.get("front_work")
    require(type(work) is dict and set(work) == {*FRONT_FIELDS, *FRONT_ARRAYS}, "wrong front work fields")
    for field in FRONT_FIELDS:
        uint(work[field], f"front_work.{field}")
    for field, length in FRONT_ARRAYS.items():
        array(work[field], length, field)
    rectangles, splits, killed = work["emitted_rectangles"], work["disjoint_splits"], work["fully_rejected_products"]
    require(work["diagonal_splits"] == n - 1 and work["diagonal_leaves"] == n and
            work["product_visits"] == 1 + 3 * (n - 1) + 2 * splits and
            rectangles + killed == n - 1 + splits and
            work["separation_tests"] == rectangles + splits, "front partition work is inconsistent")
    require(work["max_product_depth"] <= 96 and
            1 <= work["max_stack_size"] <= 2 * work["max_product_depth"] + 1,
            "front stack or product depth outside structural bounds")
    searches = work["witness_searches"]
    require(searches <= splits + killed + rectangles and
            searches <= work["witness_descent_steps"] <= 48 * searches and
            work["witness_box_distance_tests"] == 2 * work["witness_descent_steps"] and
            searches <= work["proposed_sites"] <= min(kmax, n) * searches and
            work["h_bound_tests"] + work["proposals_in_factors"] == work["proposed_sites"] and
            work["xi_bound_tests"] <= work["h_bound_tests"] and
            work["witness_lane_credits"] <= work["h_bound_tests"],
            "witness proposal or certification work outside its declared bound")
    require(work["xi_bound_tests"] == 0, "q2-only front paid Xi")
    if row["front_mode"] == "pure":
        require(all(work[field] == 0 for field in FRONT_FIELDS[5:14]) and
                work["rejected_pair_mass"] == [0, 0, 0], "pure front performed rejection work")
    require(sum(work["size_class_rectangles"]) == rectangles and
            work["size_class_rectangles"][0] == work["leaf_pair_rectangles"] and
            work["size_class_pair_mass"][0] == work["leaf_pair_rectangles"],
            "size classes lost rectangles or singleton pair mass")
    union_mass = sum(work["size_class_pair_mass"])
    require(rectangles <= union_mass <= total and 2 * rectangles <= work["emitted_factor_sites"] <= n * rectangles and
            (rectangles == 0) == (work["max_factor_size"] == 0) and work["max_factor_size"] < n,
            "residual union or factor cardinality mismatch")
    for count, mass, minimum, maximum in zip(work["size_class_rectangles"], work["size_class_pair_mass"],
            (1, 2, 8, 64, 1024), (1, 7, 63, 1023, n - 1)):
        require((count == 0) == (mass == 0) and minimum * count <= mass <= maximum * maximum * count,
                "size class mass outside possible factor cardinalities")
    for lane in range(3):
        rejected, residual, count = (work[field][lane] for field in
            ("rejected_pair_mass", "residual_pair_mass", "lane_rectangles"))
        require(rejected + residual == (total if active & (1 << lane) else 0) and
                count <= rectangles and count <= residual <= union_mass and (count == 0) == (residual == 0),
                "lane mass ledger or rectangle count mismatch")
        if row["front_mode"] == "pure" and active & (1 << lane):
            require(count == rectangles, "pure front lost an active lane mask")
    require(rectangles <= sum(work["lane_rectangles"]) <= rectangles,
            "terminal mask population mismatch")
    for field in ("input_rectangles", "anchor_queries", "candidate_pairs", "accepted_pairs", "rejected_pairs"):
        uint(row.get(field), field)
    anchors, candidates, accepted = row["anchor_queries"], row["candidate_pairs"], row["accepted_pairs"]
    filtered = pool["filtered_pairs"] if pool is not None else 0
    require(row["input_rectangles"] == rectangles and rectangles <= anchors <= union_mass == candidates + filtered ==
            work["residual_pair_mass"][0] and accepted + row["rejected_pairs"] == candidates,
            "front/census mass and descriptor accounting mismatch")
    census = row.get("census_work")
    counters(census, CENSUS_FIELDS, "census_work")
    require(census["input_descriptors"] == rectangles and all(census[field] == 0 for field in
            ("query_build_point_visits", "query_build_nodes", "query_build_max_depth",
             "query_cover_visits", "frontier_restarts")), "census rebuilt local index or changed its descriptor contract")
    require(census["count_node_visits"] == census["count_bound_tests"] + census["count_point_tests"] and
            census["payload_node_visits"] == census["payload_bound_tests"] + census["payload_point_tests"],
            "census node classifications lost work")
    shared = row["census_mode"] == "shared"
    pool_roots = pool["pair_roots"] if pool is not None else 0
    selected_rectangles = pool["selected_rectangles"] - pool["passthrough_rectangles"] if pool is not None else 0
    selected_anchors = pool["original_selected_anchors"] - pool["passthrough_anchors"] if pool is not None else 0
    expected_roots = (rectangles - selected_rectangles + pool_roots if joint is not None else
                      anchors - selected_anchors + pool_roots if shared else candidates)
    expected_starts = joint["singleton_handoffs"] + pool_roots if joint is not None else expected_roots
    require(census["count_root_starts"] == expected_roots and
            census["query_tasks"] == expected_starts + 2 * census["query_splits"] and
            census["cursor_reuses"] == 2 * census["query_splits"] and
            census["shared_splits_after_credit"] <= census["query_splits"] and
            (joint is not None or census["count_node_visits"] >= census["count_root_starts"]),
            "census roots or inherited continuation accounting mismatch")
    require(census["uniform_rejected_pairs"] <= row["rejected_pairs"] and
            census["uniform_accepted_pairs"] <= accepted, "uniform decisions exceed census decisions")
    if not shared:
        require(all(census[field] == 0 for field in
                ("query_splits", "shared_splits_after_credit", "cursor_reuses", "cursor_advances",
                 "uniform_credited_pairs", "uniform_rejected_pairs", "uniform_accepted_pairs")),
                "pairwise mode reported shared decisions")
    output = row.get("digest")
    require(type(output) is dict and set(output) ==
            {"encoding", "supports", "interior_ids", "shell_ids", "sum", "xor"} and
            output["encoding"] == "canonical_q2_support_v2", "wrong canonical support digest fields")
    for field in ("supports", "interior_ids", "shell_ids"):
        uint(output[field], field)
    fingerprint(output["sum"], "digest.sum")
    fingerprint(output["xor"], "digest.xor")
    require(output["supports"] == accepted == census["payload_supports"] and
            output["interior_ids"] == census["payload_interior_sites"] <= (kmax - 1) * accepted and
            output["shell_ids"] == census["payload_shell_sites"] and
            2 * accepted <= output["shell_ids"] <= n * accepted and
            census["payload_node_visits"] >= accepted,
            "materialized q2 supports or complete shell accounting mismatch")
    callback = row.get("callback_work")
    counters(callback, CALLBACK_FIELDS, "callback_work")
    payload_ids = output["interior_ids"] + output["shell_ids"]
    require(callback["copied_ids"] == callback["validation_ids"] == payload_ids and
            callback["sort_calls"] == 2 * accepted and
            callback["support_key_axis_checks"] == 3 * accepted and
            callback["hash_words"] == 9 * accepted + payload_ids and
            payload_ids - 2 * accepted <= callback["adjacent_tests"] <= payload_ids - accepted and
            callback["cross_set_comparisons"] <= payload_ids,
            "canonical callback omitted materialization, checks or hashing")
    memory = row.get("memory")
    counters(memory, MEMORY_FIELDS, "memory")
    require(6 * n <= memory["input_capacity_bytes"] and 6 * n <= memory["cloud_retained_bytes"] <= 128 * n and
            8 * n <= memory["index_retained_bytes"] <= 256 * n and
            (accepted == 0 or memory["callback_buffers_capacity_bytes"] >= 16),
            "declared retained vector memory outside bounds")


def campaign_directories(root: Path) -> list[Path]:
    require(root.is_dir(), "receipt root is not a directory")
    directories = sorted({path.parent for name in RECEIPT_FILES for path in root.rglob(name)})
    require(bool(directories), "no parallel q2 campaigns")
    require(not any(parent in child.parents for parent in directories for child in directories), "nested campaign roots")
    for directory in directories:
        require(all((directory / name).is_file() for name in RECEIPT_FILES), f"incomplete campaign triplet: {directory}")
    return directories


def capture(args: argparse.Namespace) -> int:
    if args.input_files:
        require(args.families is None and args.sizes is None and args.seeds in (None, [0]),
                "--input-files excludes --families/--sizes and requires seed zero")
        families = ["file:" + str(path.resolve()) for path in args.input_files]
        try:
            file_inputs = input_pins(families)
        except (OSError, ValueError) as cause:
            # Preserve pre-invocation input failures as rejected orphan
            # closures, like missing binary/compiler startup evidence.
            args.output.mkdir(parents=True, exist_ok=False)
            write_json(args.output / "COMPLETION.json", dict(status="invalid", error=str(cause),
                       runs=0, attempts=0, finished_utc=utc_stamp(), input_hashes_unchanged=False))
            print(json.dumps(dict(status="invalid", runs=0, attempts=0, error=str(cause))))
            return 1
        sizes, seeds = sorted({pin["n"] for pin in file_inputs.values()}), [0]
    else:
        families, sizes, seeds = args.families or list(FAMILIES), args.sizes or [8000, 16000, 32000], args.seeds or [3]
        file_inputs = {}
    parameters = dict(families=families, sizes=sizes, kmax=args.kmax, separations=args.s,
                      seeds=seeds, thread_counts=args.threads, jobs_per_worker=args.jobs_per_worker,
                      schedules=args.schedules, queue_capacities=args.queue_capacities,
                      donation_intervals=args.donation_intervals,
                      file_sizes={family: pin["n"] for family, pin in file_inputs.items()},
                      pool_min_factors=args.pool_min_factors, repeats=args.repeats)
    cases = matrix(parameters)
    args.output.mkdir(parents=True, exist_ok=False)
    binary = args.probe.resolve()
    pins, binary_pin = {}, None
    attempts = completed = 0
    status, error, code = "failed", "campaign did not start", 1
    previous = {signum: signal.signal(signum, on_signal) for signum in (signal.SIGINT, signal.SIGTERM)}
    try:
        pins, binary_pin = sources(), digest(binary)
        require(input_hashes(file_inputs) == {family: pin["sha256"] for family, pin in file_inputs.items()},
                "input file changed before campaign")
        cache = (binary.parent / "CMakeCache.txt").read_text()
        compilers = [line.split("=", 1)[1] for line in cache.splitlines() if line.startswith("CMAKE_CXX_COMPILER:") and "=" in line]
        require(len(compilers) == 1 and bool(compilers[0]), "missing compiler provenance")
        manifest = {**parameters, "schema": SCHEMA, "scope": SCOPE, "public_status": "not_claimed",
            "separation_convention": "box_gap_diameter_v1", "started_utc": utc_stamp(),
            "threads": "per_measurement", "gcp_used": False, "processes_sequential": True, "warmup_runs": 0,
            "probe": str(binary), "probe_sha256": binary_pin, "source_sha256": pins, "cmake_cache": cache,
            "input_files": file_inputs,
            "compiler_version": subprocess.check_output([compilers[0], "--version"], text=True),
            "platform": platform.platform(), "logical_cpu_count": os.cpu_count(),
            "cpu_affinity": sorted(os.sched_getaffinity(0)),
            "cpuinfo": Path("/proc/cpuinfo").read_text().split("\n\n", 1)[0],
            "commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
            "worktree_status": subprocess.check_output(["git", "status", "--short"], cwd=ROOT, text=True),
            "runner_command": [sys.executable, *sys.argv]}
        write_json(args.output / "MANIFEST.json", manifest)
        environment = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
        identities, signatures = {}, {}
        with (args.output / "MEASURES.jsonl").open("x") as stream:
            for *key, repeat in cases:
                command = command_for(str(binary), tuple(key))
                record = dict(command=command, repeat=repeat, status="failed", exit_code=None,
                              stdout="", stderr="", stdout_base64="", stderr_base64="", started_utc=utc_stamp())
                attempts += 1
                try:
                    file_pin = file_inputs.get(key[0])
                    record["input_sha256_before"] = digest(Path(file_pin["path"])) if file_pin else None
                    require(record["input_sha256_before"] == (file_pin["sha256"] if file_pin else None),
                            "input changed before invocation")
                    record["probe_sha256_before"] = digest(binary)
                    require(record["probe_sha256_before"] == binary_pin, "binary changed before invocation")
                    print(json.dumps({"starting": command[1:], "repeat": repeat}), flush=True)
                    invoke(command, environment, ROOT, record)
                    record["input_sha256_after"] = digest(Path(file_pin["path"])) if file_pin else None
                    require(record["input_sha256_after"] == record["input_sha256_before"], "input changed during invocation")
                    record["probe_sha256_after"] = digest(binary)
                    require(record["probe_sha256_after"] == binary_pin, "binary changed during invocation")
                    if record["exit_code"] != 0:
                        raise RuntimeError(f"probe exited {record['exit_code']}")
                    require(not record["stderr_base64"], "probe wrote unexpected stderr")
                    row = parse_result(base64.b64decode(record["stdout_base64"], validate=True))
                    validate_result(row, command)
                    if file_pin:
                        require(row["input_hash"] == file_pin["input_hash"], "probe decoded different file coordinates")
                    cross_check(row, identities, signatures)
                    record.update(status="completed", result=row)
                    completed += 1
                except KeyboardInterrupt as cause:
                    record.update(status="interrupted", error=str(cause))
                    raise
                except Exception as cause:
                    record.update(status="invalid" if isinstance(cause, ValueError) else "failed", error=str(cause))
                    raise
                finally:
                    record["finished_utc"] = utc_stamp()
                    stream.write(json.dumps(record, allow_nan=False) + "\n")
                    stream.flush()
        status, error, code = "completed", "", 0
    except KeyboardInterrupt as cause:
        status, error, code = "interrupted", str(cause), 128 + getattr(cause, "signum", signal.SIGINT)
    except Exception as cause:
        status, error = "invalid" if isinstance(cause, ValueError) else "failed", str(cause)
    finally:
        closing = {}
        for name in pins:
            try:
                closing[name] = digest(ROOT / name)
            except OSError:
                closing[name] = None
        try:
            binary_closing = digest(binary)
        except OSError:
            binary_closing = None
        source_same, binary_same = bool(pins) and pins == closing, binary_pin is not None and binary_pin == binary_closing
        try:
            input_closing = input_hashes(file_inputs)
        except OSError:
            input_closing = None
        input_same = input_closing == {family: pin["sha256"] for family, pin in file_inputs.items()}
        if status == "completed" and not (source_same and binary_same and input_same):
            status, error, code = "invalid", "source, binary or input changed at closure", 1
        write_json(args.output / "COMPLETION.json", dict(status=status, error=error, runs=completed, attempts=attempts,
            finished_utc=utc_stamp(), source_hashes_unchanged=source_same, probe_hash_unchanged=binary_same,
            source_sha256_closing=closing, probe_sha256_closing=binary_closing,
            input_hashes_unchanged=input_same, input_sha256_closing=input_closing))
        for signum, handler in previous.items():
            signal.signal(signum, handler)
    print(json.dumps(dict(status=status, runs=completed, attempts=attempts, error=error)))
    return code


def check(args: argparse.Namespace) -> int:
    directories = campaign_directories(args.receipt)
    pins = sources()
    identities, signatures, groups = {}, {}, defaultdict(list)
    common_provenance = published = None
    measures = 0
    observed_input_pins: dict[str, dict] = {}
    for directory in directories:
        manifest = parse_result((directory / "MANIFEST.json").read_bytes())
        completion = parse_result((directory / "COMPLETION.json").read_bytes())
        require(manifest.get("schema") == SCHEMA and manifest.get("scope") == SCOPE and
                manifest.get("public_status") == "not_claimed" and manifest.get("gcp_used") is False and
                manifest.get("separation_convention") == "box_gap_diameter_v1" and
                manifest.get("threads") == "per_measurement" and manifest.get("processes_sequential") is True and
                uint(manifest.get("warmup_runs"), "warmup_runs") == 0, "wrong campaign scope")
        require(manifest.get("source_sha256") == pins, "source coverage/hash differs from current version")
        current_inputs = input_pins(manifest["families"])
        require(manifest.get("input_files") == current_inputs and
                manifest.get("file_sizes") == {family: pin["n"] for family, pin in current_inputs.items()},
                "input source coverage/hash/size differs")
        for family, pin in current_inputs.items():
            require(family not in observed_input_pins or observed_input_pins[family] == pin,
                    "input changed between campaigns")
            observed_input_pins[family] = pin
        sha256(manifest.get("probe_sha256"), "probe_sha256")
        require(type(manifest.get("probe")) is str and Path(manifest["probe"]).is_absolute(), "missing absolute binary path")
        require(type(manifest.get("commit")) is str and re.fullmatch(r"[0-9a-f]{40}", manifest["commit"]) is not None and
                type(manifest.get("worktree_status")) is str and type(manifest.get("runner_command")) is list and
                bool(manifest["runner_command"]) and all(type(arg) is str for arg in manifest["runner_command"]),
                "missing repository/capture provenance")
        build, machine, published = provenance(manifest)
        affinity = manifest.get("cpu_affinity")
        require(type(affinity) is list and bool(affinity), "missing CPU affinity")
        for cpu in affinity:
            uint(cpu, "cpu_affinity")
        require(affinity == sorted(set(affinity)), "invalid CPU affinity")
        machine = {**machine, "cpu_affinity": affinity}
        published = {**published, "cpu_affinity": affinity}
        require(common_provenance is None or common_provenance == (build, machine), "heterogeneous build/machine provenance")
        common_provenance = build, machine
        start, end = timestamp(manifest.get("started_utc"), "campaign start"), timestamp(completion.get("finished_utc"), "campaign end")
        require(start <= end and completion.get("status") == "completed" and completion.get("source_hashes_unchanged") is True and
                completion.get("probe_hash_unchanged") is True and completion.get("source_sha256_closing") == pins and
                completion.get("probe_sha256_closing") == manifest["probe_sha256"], "invalid campaign closure")
        require(completion.get("input_hashes_unchanged") is True and completion.get("input_sha256_closing") ==
                {family: pin["sha256"] for family, pin in current_inputs.items()}, "invalid input closure")
        expected = set(matrix(manifest))
        records = [parse_result(line) for line in (directory / "MEASURES.jsonl").read_bytes().splitlines()]
        require(uint(completion.get("runs"), "runs") == uint(completion.get("attempts"), "attempts") ==
                len(records) == len(expected), "incomplete matrix")
        seen, previous_end = set(), start
        for record in records:
            begun, finished = timestamp(record.get("started_utc"), "invocation start"), timestamp(record.get("finished_utc"), "invocation end")
            require(previous_end <= begun <= finished <= end, "invocations are not serial inside campaign timestamps")
            previous_end = finished
            require(record.get("status") == "completed" and uint(record.get("exit_code"), "exit_code") == 0 and
                    record.get("stderr") == record.get("stderr_base64") == "" and
                    record.get("probe_sha256_before") == record.get("probe_sha256_after") == manifest["probe_sha256"],
                    "invalid invocation or binary pin")
            raw = base64.b64decode(record["stdout_base64"], validate=True)
            row = parse_result(raw)
            key = tuple(row[name] for name in KEYS)
            command = command_for(manifest["probe"], key)
            require(raw.decode() == record.get("stdout") and row == record.get("result") and
                    record.get("command") == command, "raw/parsed/command mismatch")
            validate_result(row, command)
            file_pin = current_inputs.get(row["family"])
            require(record.get("input_sha256_before") == record.get("input_sha256_after") ==
                    (file_pin["sha256"] if file_pin else None), "input invocation pins differ")
            require("input_sha256_before" in record and "input_sha256_after" in record, "input pin columns omitted")
            if file_pin:
                require(row["input_hash"] == file_pin["input_hash"], "file coordinate fingerprint differs")
            identity = (*key, uint(record.get("repeat"), "repeat"))
            require(identity not in seen, "duplicate invocation")
            seen.add(identity)
            cross_check(row, identities, signatures)
            groups[key].append(row)
            measures += 1
        require(seen == expected, "missing or extra matrix tuple")
    require(sources() == pins, "sources changed while reading")
    require(input_hashes(observed_input_pins) == {family: pin["sha256"] for family, pin in observed_input_pins.items()},
            "input files changed while reading")
    result = dict(status="passed", campaigns=len(directories), measurements=measures,
                  configurations_including_threads_seed=len(groups), scope=SCOPE, full_contract_qualified=False,
                  gcp_used=False, provenance_policy=PROVENANCE_POLICY, provenance=published,
                  current_reader_sha256=pins[RUNNER_SOURCE], source_check="current_content_and_closing_pins_not_a_rebuild",
                  mode_comparison="same_input_supports_and_integer_geometry_across_threads_and_jobs",
                  excluded_schedule_fields=["pool_time_sums", "memory", "parallel_work", "dispatch_work", "workers", "timings"],
                  input_files=observed_input_pins)
    if args.summary:
        result["summary"] = []
        for key, rows in sorted(groups.items()):
            item = dict(zip(KEYS, key), repeats=len(rows), **{name: rows[0][name] for name in DISCRETE_FIELDS})
            item["median_timings"] = {field: statistics.median(row["timings"][field] for row in rows) for field in TIMES}
            result["summary"].append(item)
    print(json.dumps(result, indent=2, allow_nan=False))
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="operation", required=True)
    run = sub.add_parser("run")
    run.add_argument("--probe", required=True, type=Path)
    run.add_argument("--output", required=True, type=Path)
    run.add_argument("--sizes", nargs="+", type=int)
    run.add_argument("--families", nargs="+", choices=FAMILIES)
    run.add_argument("--input-files", nargs="+", type=Path)
    run.add_argument("--kmax", nargs="+", type=int, default=[5, 10])
    run.add_argument("--s", nargs="+", type=int, default=[8, 10, 12])
    run.add_argument("--seeds", nargs="+", type=int)
    run.add_argument("--threads", nargs="+", type=int, default=[0, 1, 2, 4])
    run.add_argument("--jobs-per-worker", nargs="+", type=int, default=[16])
    run.add_argument("--schedules", nargs="+", choices=("coarse", "donate"), default=["coarse", "donate"])
    run.add_argument("--queue-capacities", nargs="+", type=int, default=[64])
    run.add_argument("--donation-intervals", nargs="+", type=int, default=[64])
    run.add_argument("--pool-min-factors", nargs="+", type=int, default=[64])
    run.add_argument("--repeats", type=int, default=1)
    reader = sub.add_parser("check")
    reader.add_argument("receipt", type=Path)
    reader.add_argument("--summary", action="store_true")
    args = parser.parse_args()
    try:
        return capture(args) if args.operation == "run" else check(args)
    except (ValueError, OSError, KeyError, TypeError) as error:
        if args.operation == "run":
            parser.error(str(error))
        print(f"parallel q2 receipt rejected: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
