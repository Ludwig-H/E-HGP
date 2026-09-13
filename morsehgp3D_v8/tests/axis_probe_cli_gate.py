#!/usr/bin/env python3
"""Check axis-probe scope, literal full-sheet counts, hashes and paired inputs."""

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
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
from paired_receipts import validate_axis


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def fnv_bytes(values: bytes) -> str:
    checksum = 14695981039346656037
    for value in values:
        checksum = ((checksum ^ value) * 1099511628211) & ((1 << 64) - 1)
    return format(checksum, "x")


def sheet_dimensions(n: int) -> tuple[int, int]:
    count = n // 2
    width = max(divisor for divisor in range(1, math.isqrt(count) + 1)
                if count % divisor == 0)
    return width, count // width


def sheet_fingerprint(n: int) -> str:
    width, _height = sheet_dimensions(n)
    data = bytearray()
    for x in (1000, 60000):
        for index in range(n // 2):
            for coordinate in (x, 1000 + index % width, 1000 + index // width):
                data.extend(coordinate.to_bytes(2, "little"))
    return fnv_bytes(data)


def sheet_candidates(n: int, need: int) -> int:
    """Axis-slab residual, not an exhaustive q2 census or a FULL hierarchy."""
    width, height = sheet_dimensions(n)

    def axis_sum(length: int) -> int:
        closed = (length * length if need >= length - 1 else
                  length * (2 * need + 1) - need * (need + 1))
        explicit = sum(abs(a - b) <= need for a in range(length) for b in range(length))
        require(closed == explicit, "closed axis formula disagrees with bounded enumeration")
        return closed

    formula = axis_sum(width) * axis_sum(height)
    sites = list(itertools.product(range(width), range(height)))
    exhaustive = sum(abs(a[0] - b[0]) <= need and abs(a[1] - b[1]) <= need
                     for a in sites for b in sites)
    require(formula == exhaustive, "full-sheet product formula disagrees with exhaustive pairs")
    return formula


def whole_factor_checksum(n: int, need: int) -> str:
    """No-index literal case: B keeps original IDs, one full block per A."""
    count = n // 2
    words = [1, need, count * count, count * count, 0, count, count, n,
             count, *range(count, n), count]
    for a_id in range(count):
        words.extend((a_id, 0, count))
    return fnv_bytes(b"".join(word.to_bytes(8, "little") for word in words))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--single-probe", type=Path)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    probe = args.probe.resolve()
    single_probe = (args.single_probe or probe.with_name("mhgp8_p0_probe")).resolve()
    checks = literal_checksums = reduced_sheets = 0

    def measure(arguments: list[str]) -> dict[str, Any]:
        command = [str(probe), *arguments]
        result = subprocess.run(command, capture_output=True, text=True)
        require(result.returncode == 0 and not result.stderr,
                f"axis probe failed: {arguments}: {result}")
        row = json.loads(result.stdout)
        validate_axis(row, command)
        require("plans_identical" not in row and
                row["checksum_kind"] == "fnv1a64_le_u64_axis_plan_v1",
                "axis must not claim equality to its different baseline")
        setup = row["generation_ms"] + row["prepare_ms"]
        require(row["paired_execution_ms"] + 1e-6 >=
                setup + row["baseline_ms"] + row["axis_ms"] + row["inspection_ms"],
                "paired durations overlap or hide inspection")
        return row

    invalid = [
        [], ["--help"], ["-1", "pool", "grid", "10", "8", "baseline-first"],
        ["8", "unknown", "grid", "10", "8", "baseline-first"],
        ["8", "tubes", "unknown", "10", "8", "baseline-first"],
        ["8", "pool", "grid", "0", "8", "baseline-first"],
        ["8", "pool", "grid", "11", "8", "baseline-first"],
        ["8", "pool", "grid", "10", "0", "baseline-first"],
        ["8", "pool", "grid", "10", "8", "batch-first"],
        ["8", "pool", "grid", "10", "8", "axis-first", "extra"],
        ["8", "tubes", "rails", "10", "8", "axis-first"],
        ["32000", "tubes", "tube", "10", "12", "axis-first"],
        ["70000", "pool", "grid", "10", "8", "baseline-first"],
        ["7", "tubes", "sheet_full", "10", "8", "axis-first"],
        ["131074", "tubes", "sheet_full", "10", "8", "axis-first"],
        ["9000000000", "tubes", "sheet_full", "10", "8", "axis-first"],
    ]
    for arguments in invalid:
        result = subprocess.run([str(probe), *arguments], capture_output=True, text=True)
        require(result.returncode == 2 and not result.stdout and bool(result.stderr),
                f"bad axis refusal or partial success: {arguments}")
        checks += 1

    # Axis work/result must not depend on the chosen reference strategy,
    # execution order, or s when it only checks this unchanged rectangle.
    axis_signatures: dict[tuple[Any, ...], Any] = {}

    def check_axis_signature(row: dict[str, Any]) -> None:
        key = (row["family"], row["n"], row["kmax"])
        signature = (row["input_fnv1a64_le_u16_xyz"], row["axis_candidates"],
                     row["axis_descriptors"], row["axis_checksum"], row["axis_work"])
        require(key not in axis_signatures or axis_signatures[key] == signature,
                "baseline choice/order/s changed the axis result or work")
        axis_signatures[key] = signature

    formulae = {(n, kmax): sheet_candidates(n, kmax)
                for n, kmax in itertools.product((8, 12, 18, 34, 128), (1, 5, 10))}
    for n, kmax, separation, strategy, order in itertools.product(
            (8, 12, 18, 34, 128), (1, 5, 10), (8, 10, 12),
            ("pool", "dual", "tubes"), ("baseline-first", "axis-first")):
        row = measure([str(n), strategy, "sheet_full", str(kmax), str(separation), order])
        require(row["fixture_version"] == 2 and
                row["input_fnv1a64_le_u16_xyz"] == sheet_fingerprint(n),
                "full-sheet recipe or coordinate checksum changed")
        require(row["axis_candidates"] == formulae[(n, kmax)] and
                row["baseline_candidates"] == row["total_pairs"],
                "full-sheet axis formula or zero-universal-credit baseline changed")
        check_axis_signature(row)
        width, height = sheet_dimensions(n)
        if kmax >= max(width, height) - 1:
            require(row["axis_checksum"] == whole_factor_checksum(n, kmax) and
                    row["axis_descriptors"] == n // 2 and
                    row["axis_work"]["whole_factor_accepts"] == n // 2 and
                    row["axis_work"]["tree_nodes"] == 0,
                    "literal original IDs/whole-factor descriptors/checksum disagree")
            literal_checksums += 1
        if row["axis_candidates"] < row["total_pairs"]:
            reduced_sheets += 1
        checks += 1

    old_rows: dict[tuple[Any, ...], Any] = {}
    cases = [(family, 130) for family in ("grid", "sheet", "skew", "tube")]
    cases.append(("rails", 2718))
    for (family, n), kmax, separation, strategy, order in itertools.product(
            cases, (5, 10), (8, 10, 12), ("pool", "dual", "tubes"),
            ("baseline-first", "axis-first")):
        row = measure([str(n), strategy, family, str(kmax), str(separation), order])
        key = (family, n, kmax, separation, strategy)
        if key not in old_rows:
            result = subprocess.run([str(single_probe), str(n), strategy, "2", family,
                                     str(kmax), str(separation)], capture_output=True, text=True)
            require(result.returncode == 0 and not result.stderr, "single-q2 control failed")
            old_rows[key] = json.loads(result.stdout)
        old = old_rows[key]
        require(row["fixture_version"] == old["fixture_version"] == 1 and
                row["input_fnv1a64_le_u16_xyz"] == old["input_fnv1a64_le_u16_xyz"] and
                row["baseline_candidates"] == old["candidate_pairs"] and
                row["baseline_work"] == old["plan_work"] and
                row["preparation_work"] == old["preparation_work"],
                "baseline recipes/credits/work differ from the original single-q2 API")
        check_axis_signature(row)
        checks += 1

    require(checks == 466 and len(old_rows) == 90 and
            literal_checksums > 0 and reduced_sheets > 0, "axis CLI non-vacuity floor")
    print(json.dumps({"status": "passed", "checks": checks,
                      "literal_checksums": literal_checksums,
                      "reduced_sheet_cases": reduced_sheets,
                      "single_q2_controls": len(old_rows),
                      "python_optimized": bool(sys.flags.optimize),
                      "scope": "axis_probe_receipts_and_full_sheet_formula_not_full_hgp"}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
