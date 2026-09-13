#!/usr/bin/env python3
"""Check additive probe receipts/formulae; the C++ gate judges physical pairs."""

from __future__ import annotations

import argparse
import itertools
import json
import subprocess
import sys
from pathlib import Path
from typing import Any

from axis_probe_cli_gate import (fnv_bytes, sheet_candidates, sheet_dimensions,
                                 sheet_fingerprint, whole_factor_checksum)


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
from paired_receipts import ADDITIVE_FIELDS, validate_additive  # noqa: E402
from run_p0_matrix import parse_result  # noqa: E402


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def additive_sheet_count(n: int, need: int) -> int:
    """Disjoint coordinate-column count, not the complete q2 census."""
    width, height = sheet_dimensions(n)

    def population(length: int, credit: int) -> int:
        # Distances 0 and 1 both give zero STRICT interior witnesses.
        if credit == 0:
            return length + 2 * max(0, length - 1)
        return 2 * max(0, length - credit - 1)

    formula = sum(population(width, a) * population(height, b)
                  for a in range(need) for b in range(need - a))
    sites = list(itertools.product(range(width), range(height)))
    explicit = sum(max(0, abs(a[0] - b[0]) - 1) +
                   max(0, abs(a[1] - b[1]) - 1) < need for a in sites for b in sites)
    require(formula == explicit, "additive full-sheet formula disagrees with bounded pairs")
    return formula


def zero_local_checksum(n: int, need: int, strategy: str) -> str:
    """Literal credit_plan_v1: all credits zero, one full grouped product."""
    count = n // 2
    words = [1, 2, ("pool", "dual", "tubes").index(strategy), need, 0,
             count * count, count * count, 0, count, count, n]
    for entries in ([0] * count, [0] * count, range(count), range(count, n)):
        words.extend((count, *entries))
    words.extend((1, 0, count, 0, count))
    return fnv_bytes(b"".join(word.to_bytes(8, "little") for word in words))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--axis-probe", type=Path)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    probe = args.probe.resolve()
    axis_probe = (args.axis_probe or probe.with_name("mhgp8_axis_probe")).resolve()
    checks = literal_checksums = reduced_sheets = intersection_improvements = 0
    axis_rank_queries = restriction_queries = 0

    invalid = [
        [], ["--help"], ["-1", "pool", "grid", "10", "8", "independent-first", "additive"],
        ["8", "unknown", "grid", "10", "8", "independent-first", "additive"],
        ["8", "tubes", "unknown", "10", "8", "independent-first", "additive"],
        ["8", "pool", "grid", "0", "8", "independent-first", "additive"],
        ["8", "pool", "grid", "11", "8", "independent-first", "additive"],
        ["8", "pool", "grid", "10", "0", "independent-first", "additive"],
        ["8", "pool", "grid", "10", "8", "baseline-first", "additive"],
        ["8", "pool", "grid", "10", "8", "independent-first", "union"],
        ["8", "pool", "grid", "10", "8", "variant-first", "intersection", "extra"],
        ["8", "tubes", "rails", "10", "8", "variant-first", "intersection"],
        ["32000", "tubes", "tube", "10", "12", "variant-first", "intersection"],
        ["70000", "pool", "grid", "10", "8", "independent-first", "additive"],
        ["7", "tubes", "sheet_full", "10", "8", "variant-first", "intersection"],
        ["131074", "tubes", "sheet_full", "10", "8", "variant-first", "intersection"],
        ["9000000000", "tubes", "sheet_full", "10", "8", "variant-first", "intersection"],
    ]
    for arguments in invalid:
        result = subprocess.run([str(probe), *arguments], capture_output=True)
        require(result.returncode == 2 and not result.stdout and bool(result.stderr),
                f"bad additive refusal or partial success: {arguments}")
        checks += 1

    old_rows: dict[tuple[Any, ...], Any] = {}
    independent_signatures: dict[tuple[Any, ...], Any] = {}
    variant_signatures: dict[tuple[Any, ...], Any] = {}
    additive_counts: dict[tuple[Any, ...], int] = {}
    intersection_counts: list[tuple[tuple[Any, ...], int]] = []

    def measure(n: int, strategy: str, family: str, kmax: int, separation: int,
                order: str, variant: str) -> dict[str, Any]:
        nonlocal axis_rank_queries, restriction_queries
        command = [str(probe), str(n), strategy, family, str(kmax), str(separation),
                   order, variant]
        result = subprocess.run(command, capture_output=True)
        require(result.returncode == 0 and not result.stderr,
                f"additive probe failed: {command}: {result.stderr!r}")
        row = parse_result(result.stdout)
        validate_additive(row, command)
        require("plans_identical" not in row and row["need"] == kmax and
                row["independent_kind"] == "axis_q2_independent",
                "wrong need/baseline or false plan-equality claim")
        key = (family, n, kmax, separation, strategy)
        if key not in old_rows:
            old_command = [str(axis_probe), str(n), strategy, family, str(kmax),
                           str(separation), "baseline-first"]
            old = subprocess.run(old_command, capture_output=True)
            require(old.returncode == 0 and not old.stderr, "independent-axis control failed")
            old_rows[key] = parse_result(old.stdout)
        old = old_rows[key]
        require(row["fixture_version"] == old["fixture_version"] and
                row["input_fnv1a64_le_u16_xyz"] == old["input_fnv1a64_le_u16_xyz"] and
                row["preparation_work"] == old["preparation_work"] and
                row["independent_candidates"] == old["axis_candidates"] and
                row["independent_descriptors"] == old["axis_descriptors"] and
                row["independent_checksum"] == old["axis_checksum"] and
                {field: row["independent_work"][field] for field in old["axis_work"]} ==
                old["axis_work"] and
                all(row["independent_work"][field] == 0 for field in ADDITIVE_FIELDS),
                "Independent changed its input, result, descriptor identities or work")
        base_key = (family, n, kmax)
        independent_signature = tuple(row[field] for field in (
            "input_fnv1a64_le_u16_xyz", "independent_candidates", "independent_descriptors",
            "independent_checksum", "independent_work"))
        require(base_key not in independent_signatures or
                independent_signatures[base_key] == independent_signature,
                "strategy/variant/order/s changed the Independent baseline")
        independent_signatures[base_key] = independent_signature
        variant_key = (*base_key, variant, strategy if variant == "intersection" else None)
        variant_signature = tuple(row[field] for field in (
            "variant_candidates", "variant_descriptors", "variant_checksum", "variant_work"))
        require(variant_key not in variant_signatures or
                variant_signatures[variant_key] == variant_signature,
                "irrelevant strategy/order/s changed the additive object or work")
        variant_signatures[variant_key] = variant_signature
        if variant == "intersection":
            require(row["local_plan_present"] is True and
                    row["local_candidates"] == old["baseline_candidates"] and
                    row["local_work"] == old["baseline_work"] and
                    row["variant_work"]["restriction_credit_copies"] == n,
                    "restriction changed local q2 work or hid its private snapshot")
            intersection_counts.append((base_key, row["variant_candidates"]))
        else:
            require(row["local_plan_present"] is False and row["local_plan_ms"] == 0 and
                    all(row[field] is None for field in
                        ("local_candidates", "local_descriptors", "local_checksum", "local_work")) and
                    all(row["variant_work"][field] == 0 for field in ADDITIVE_FIELDS
                        if field.startswith("restriction_")),
                    "unrestricted arm fabricated or executed local work")
            additive_counts[base_key] = row["variant_candidates"]
        axis_rank_queries += row["variant_work"]["axis_count_queries"]
        restriction_queries += row["variant_work"]["restriction_bound_queries"]
        return row

    sizes = (8, 18, 34, 128)
    formulae = {(n, kmax): additive_sheet_count(n, kmax)
                for n, kmax in itertools.product(sizes, (1, 5, 10))}
    independent_formulae = {(n, kmax): sheet_candidates(n, kmax)
                            for n, kmax in itertools.product(sizes, (1, 5, 10))}
    for n, kmax, separation, strategy, variant, order in itertools.product(
            sizes, (1, 5, 10), (8, 10, 12), ("pool", "dual", "tubes"),
            ("additive", "intersection"), ("independent-first", "variant-first")):
        row = measure(n, strategy, "sheet_full", kmax, separation, order, variant)
        require(row["input_fnv1a64_le_u16_xyz"] == sheet_fingerprint(n) and
                row["fixture_version"] == 2 and
                row["independent_candidates"] == independent_formulae[(n, kmax)] and
                row["variant_candidates"] == formulae[(n, kmax)],
                "full-sheet coordinates or strict additive formula changed")
        if variant == "intersection":
            require(row["local_candidates"] == row["total_pairs"] and
                    row["local_descriptors"] == 1 and
                    row["local_checksum"] == zero_local_checksum(n, kmax, strategy),
                    "literal all-zero full-sheet local restriction changed")
            literal_checksums += 1
        if row["variant_candidates"] == row["total_pairs"]:
            require(row["variant_checksum"] == whole_factor_checksum(n, kmax) and
                    row["variant_descriptors"] == n // 2 and
                    row["variant_work"]["tree_nodes"] == 0,
                    "literal whole-factor additive checksum differs")
            literal_checksums += 1
        if row["variant_candidates"] < row["independent_candidates"]:
            reduced_sheets += 1
        checks += 1

    cases = [(family, 130) for family in ("grid", "sheet", "skew", "tube")]
    cases.append(("rails", 2718))
    for (family, n), kmax, separation, strategy, variant, order in itertools.product(
            cases, (5, 10), (8, 10, 12), ("pool", "dual", "tubes"),
            ("additive", "intersection"), ("independent-first", "variant-first")):
        row = measure(n, strategy, family, kmax, separation, order, variant)
        require(row["fixture_version"] == 1, "a legacy coordinate recipe was relabelled")
        checks += 1

    for key, candidates in intersection_counts:
        require(key in additive_counts and candidates <= additive_counts[key],
                "intersection enlarged the unrestricted additive residual")
        intersection_improvements += candidates < additive_counts[key]
    require(checks == 809 and len(old_rows) == 198 and literal_checksums > 0 and
            reduced_sheets > 0 and intersection_improvements > 0 and
            axis_rank_queries > 0 and restriction_queries > 0,
            "additive CLI non-vacuity floor")
    print(json.dumps({"status": "passed", "checks": checks,
                      "literal_checksums": literal_checksums,
                      "reduced_sheet_cases": reduced_sheets,
                      "intersection_improvements": intersection_improvements,
                      "independent_axis_controls": len(old_rows),
                      "python_optimized": bool(sys.flags.optimize),
                      "scope": "additive_probe_receipts_and_sheet_formula_not_full_hgp"}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
