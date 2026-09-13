#!/usr/bin/env python3
"""Check paired three-lane probe receipts, shared work and literal plan hashes."""

from __future__ import annotations

import argparse
import itertools
import json
import math
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
SHARED_FIELDS = (
    "tube_records", "tube_cells", "tube_sort_comparisons", "tube_separation_fallbacks",
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def zero_work(work: dict[str, Any]) -> bool:
    return all(value == 0 for key, value in work.items() if key != "predicates") and all(
        value == 0 for value in work["predicates"].values())


def validate_row(row: dict[str, Any], arguments: list[str]) -> None:
    n, strategy, family, kmax, separation, order = arguments
    require(row["schema"] == "mhgp8_batch_probe_v1" and
            row["status"] == "completed" and
            row["scope"] == "single_rectangle_three_lane_credit_batch" and
            row["public_status"] == "not_claimed", "batch schema/status/scope")
    require(row["n"] == int(n) and row["strategy"] == strategy and
            row["family"] == family and row["kmax"] == int(kmax) and
            row["separation_s"] == int(separation) and row["order"] == order,
            "batch command/result tuple")
    require(row["backend"] == "cpu_reference" and row["threads"] == 1 and
            row["profile"] == "quantized_u16_input_only", "batch backend/profile")
    require(row["owner_preparations"] == 1 and row["same_owner"] is True and
            row["plans_identical"] is True and row["candidates_expanded"] is False and
            row["downstream_measured"] is False, "batch ownership/comparison scope")
    require(row["fixture_version"] == 1 and row["seed"] is None and
            row["checksum_kind"] == "fnv1a64_le_u64_plan_v1" and
            row["totals_kind"] == "common_setup_plus_one_arm_not_pair_wall_time" and
            row["s_role"] == "rectangle_precondition_not_wspd_generation",
            "fixture/checksum/timing scope")
    require(row["preparation_work"]["validation_points"] == int(n) and
            row["n_a"] + row["n_b"] == int(n), "one-owner validation count")
    for field in ("generation_ms", "prepare_ms", "baseline_shared_owner_ms", "batch_ms",
                  "baseline_total_ms", "batch_total_ms", "comparison_ms", "paired_execution_ms"):
        require(type(row[field]) in (int, float) and math.isfinite(row[field]) and
                row[field] >= 0, f"invalid batch time: {field}")
    setup = row["generation_ms"] + row["prepare_ms"]
    require(math.isclose(row["baseline_total_ms"], setup + row["baseline_shared_owner_ms"],
                         rel_tol=1e-12, abs_tol=1e-6) and
            math.isclose(row["batch_total_ms"], setup + row["batch_ms"],
                         rel_tol=1e-12, abs_tol=1e-6), "owner counted incorrectly in totals")
    require(row["paired_execution_ms"] + 1e-6 >=
            setup + row["baseline_shared_owner_ms"] + row["batch_ms"] + row["comparison_ms"],
            "overlapping or missing paired phase")
    require([lane["lane"] for lane in row["lanes"]] == [2, 3, 4], "three ordered lanes")
    shared = row["shared_work"]
    for lane in row["lanes"]:
        threshold = max(0, int(kmax) + 2 - lane["lane"])
        require(lane["threshold"] == threshold and lane["core_credit"] == 0,
                "lane activation/threshold/core")
        require(lane["total_pairs"] == row["n_a"] * row["n_b"] and
                0 <= lane["candidate_pairs"] <= lane["total_pairs"] and
                (lane["candidate_pairs"] == 0) == (lane["candidate_descriptors"] == 0),
                "lane pair/descriptor counts")
        require(lane["baseline_checksum"] == lane["batch_checksum"] and
                len(lane["batch_checksum"]) in range(1, 17), "full-plan checksum mismatch")
        old, new = lane["baseline_work"], lane["batch_work"]
        if threshold == 0:
            require(lane["candidate_pairs"] == 0 and zero_work(old) and zero_work(new),
                    "inactive lane did work")
        elif strategy == "tubes":
            require(shared["tube_records"] == int(n), "tube input must be prepared once")
            for field in SHARED_FIELDS:
                require(old[field] == shared[field] and new[field] == 0,
                        f"shared tube work charged incorrectly: {field}")
            require(all(old[field] == new[field] for field in old
                        if field not in SHARED_FIELDS), "lane query changed with shared preparation")
            require(new["tube_sweep_tests"] <= 2 * shared["tube_records"],
                    "shared tube sweep bound")
        else:
            require(old == new and zero_work(shared), "non-tube control work changed")
    if strategy == "tubes":
        require(all(value == 0 for field, value in shared.items()
                    if field not in (*SHARED_FIELDS, "predicates")) and
                all(value == 0 for value in shared["predicates"].values()),
                "lane work incorrectly charged to shared preparation")


def literal_tube_hash(kmax: int, lane: int, strategy: str) -> tuple[str, int, int]:
    """Independent n=8 collinear oracle: forward A suffixes and B prefixes.

    Reconstruct actual credit arrays, stable grouped original IDs and every
    descriptor. No production predicate or checksum function is consulted.
    """
    count = 4
    need = max(0, kmax + 2 - lane)
    a = [min(need, count - 1 - index) for index in range(count)]
    b = [min(need, index) for index in range(count)]
    a_order = sorted(range(count), key=lambda index: a[index]) if need else []
    b_order = sorted(range(count, 2 * count), key=lambda index: b[index - count]) if need else []
    a_groups, b_groups = [], []
    a_offset = b_offset = 0
    for credit in range(need + 1):
        a_next = a_offset + a.count(credit)
        b_next = b_offset + b.count(credit)
        a_groups.append((a_offset, a_next))
        b_groups.append((b_offset, b_next))
        a_offset, b_offset = a_next, b_next
    blocks = []
    for ai in range(need):
        for bi in range(need - ai):
            aa, bb = a_groups[ai], b_groups[bi]
            if aa[0] != aa[1] and bb[0] != bb[1]:
                blocks.append((*aa, *bb))
    candidates = sum((a1 - a0) * (b1 - b0) for a0, a1, b0, b1 in blocks)
    words = [1, lane, {"pool": 0, "dual": 1, "tubes": 2}[strategy], need, 0,
             count * count, candidates, 0, count, count, 2 * count]
    for values in (a, b, a_order, b_order):
        words.extend([len(values), *values])
    words.append(len(blocks))
    for block in blocks:
        words.extend(block)
    checksum = 14695981039346656037
    for word in words:
        for byte in word.to_bytes(8, "little"):
            checksum = ((checksum ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return format(checksum, "x"), candidates, len(blocks)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--single-probe", type=Path)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    probe = args.probe.resolve()
    single_probe = (args.single_probe or probe.with_name("mhgp8_p0_probe")).resolve()
    checks = 0

    def measure(arguments: list[str]) -> dict[str, Any]:
        result = subprocess.run([str(probe), *arguments], capture_output=True, text=True)
        require(result.returncode == 0 and not result.stderr,
                f"batch probe failed: {arguments}: {result}")
        row = json.loads(result.stdout)
        validate_row(row, arguments)
        return row

    invalid = [
        [], ["--help"], ["-1", "pool", "grid", "10", "8", "baseline-first"],
        ["8", "unknown", "grid", "10", "8", "baseline-first"],
        ["8", "tubes", "unknown", "10", "8", "baseline-first"],
        ["8", "pool", "grid", "0", "8", "baseline-first"],
        ["8", "pool", "grid", "11", "8", "baseline-first"],
        ["8", "pool", "grid", "10", "0", "baseline-first"],
        ["8", "pool", "grid", "10", "8", "unknown"],
        ["8", "pool", "grid", "10", "8", "baseline-first", "extra"],
        ["8", "tubes", "rails", "10", "8", "batch-first"],
        ["32000", "tubes", "tube", "10", "12", "batch-first"],
        ["70000", "pool", "grid", "10", "8", "baseline-first"],
    ]
    for arguments in invalid:
        result = subprocess.run([str(probe), *arguments], capture_output=True, text=True)
        require(result.returncode == 2 and not result.stdout and bool(result.stderr),
                f"bad refusal code or success output: {arguments}")
        checks += 1

    for kmax, strategy, order in itertools.product(
            (1, 2, 5, 10), ("pool", "dual", "tubes"), ("baseline-first", "batch-first")):
        row = measure(["8", strategy, "tube", str(kmax), "8", order])
        for lane in row["lanes"]:
            checksum, candidates, descriptors = literal_tube_hash(kmax, lane["lane"], strategy)
            require(lane["batch_checksum"] == checksum and
                    lane["candidate_pairs"] == candidates and
                    lane["candidate_descriptors"] == descriptors,
                    "independent literal credit/order/block checksum disagreement")
        checks += 1

    signatures: dict[tuple[Any, ...], Any] = {}
    for family, kmax, separation, strategy, order in itertools.product(
            ("grid", "sheet", "skew", "tube"), (1, 5, 10), (8, 10, 12),
            ("pool", "dual", "tubes"), ("baseline-first", "batch-first")):
        row = measure(["128", strategy, family, str(kmax), str(separation), order])
        key = (family, kmax, strategy)
        signature = (row["input_fnv1a64_le_u16_xyz"], row["preparation_work"],
                     row["shared_work"], row["lanes"])
        require(key not in signatures or signatures[key] == signature,
                "s/order changed paired identities, work or residual")
        signatures[key] = signature
        checks += 1

    for kmax, separation, strategy, order in itertools.product(
            (5, 10), (8, 10, 12), ("pool", "dual", "tubes"),
            ("baseline-first", "batch-first")):
        row = measure(["2718", strategy, "rails", str(kmax), str(separation), order])
        if kmax == 10:
            require(row["lanes"][2]["candidate_pairs"] ==
                    (1846881 if strategy == "pool" else 2916), "rails residual changed")
        checks += 1

    # Explicit data-only differential against the immutable r3 receipts:
    # this checks extraction of recipes v1, not old runtime qualification.
    receipts = ROOT / "morsehgp3D_v8/receipts/p0_local_credits_20260913"
    historical: dict[tuple[Any, ...], dict[str, Any]] = {}
    for campaign in ("mono_k10_s8", "rails_k10_s8_10_12"):
        for line in (receipts / campaign / "MEASURES.jsonl").read_text().splitlines():
            row = json.loads(line)["result"]
            if row["separation_s"] == 8:
                historical[(row["family"], row["n"], row["strategy"], row["lane"])] = row
    cases = [(family, n, "pool", 2) for family, n in itertools.product(
        ("grid", "sheet", "skew"), (8000, 16000, 32000))]
    cases.extend(("rails", 2718, strategy, 4) for strategy in ("pool", "dual", "tubes"))
    for family, n, strategy, lane in cases:
        result = subprocess.run([str(single_probe), str(n), strategy, str(lane), family, "10", "8"],
                                capture_output=True, text=True)
        require(result.returncode == 0 and not result.stderr, "extracted single probe failed")
        actual = json.loads(result.stdout)
        expected = historical[(family, n, strategy, lane)]
        for field in ("fixture_version", "input_fnv1a64_le_u16_xyz", "n_a", "n_b",
                      "candidate_pairs", "candidate_descriptors"):
            require(actual[field] == expected[field], f"recipe v1 extraction changed {field}")
        checks += 1

    require(checks == 301, "batch CLI non-vacuity floor")
    print(json.dumps({"status": "passed", "checks": checks,
                      "literal_checksum_plans": 72, "historical_recipe_differentials": 12,
                      "python_optimized": bool(sys.flags.optimize),
                      "scope": "paired_three_lane_probe_not_full_tower"}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
