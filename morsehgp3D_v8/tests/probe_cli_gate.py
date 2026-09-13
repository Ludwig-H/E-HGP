#!/usr/bin/env python3
"""Exact exit codes and JSON scope of the P0 rectangle probe; not a FULL gate."""

from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    checks = 0
    rejected = [
        [], ["--help"], ["-1", "pool", "2", "grid", "10", "8"],
        ["8", "pool", "256", "grid", "10", "8"],
        ["8", "unknown", "2", "grid", "10", "8"],
        ["8", "dual", "2", "unknown", "10", "8"],
        ["8", "dual", "2", "grid", "11", "8"],
        ["8", "dual", "2", "grid", "10", "0"],
        ["32000", "dual", "2", "tube", "10", "12"],
        ["70000", "pool", "2", "grid", "10", "8"],
    ]
    for arguments in rejected:
        result = subprocess.run([str(args.probe), *arguments], capture_output=True, text=True)
        require(result.returncode == 2, f"bad-argument exit code: {arguments}: {result}")
        require(not result.stdout.strip(), f"success output on refusal: {arguments}")
        require(bool(result.stderr.strip()), f"missing refusal diagnostic: {arguments}")
        checks += 1
    for family in ("grid", "sheet", "skew", "tube"):
        for strategy in ("pool", "dual", "tubes"):
            for lane in (2, 3, 4):
                comparable = []
                for separation in (8, 10, 12):
                    arguments = ["128", strategy, str(lane), family, "10", str(separation)]
                    result = subprocess.run(
                        [str(args.probe), *arguments], capture_output=True, text=True
                    )
                    require(result.returncode == 0, f"probe failed: {arguments}: {result}")
                    require(not result.stderr.strip(), f"unexpected stderr: {arguments}")
                    row = json.loads(result.stdout)
                    require(row["schema"] == "mhgp8_p0_probe_v1", "schema")
                    require(row["scope"] == "single_separated_rectangle_credits", "scope")
                    require(row["public_status"] == "not_claimed", "public status")
                    require(row["threads"] == 1, "mono probe")
                    require(row["candidates_expanded"] is False, "expansion scope")
                    require(row["downstream_measured"] is False, "downstream scope")
                    require(row["n_a"] + row["n_b"] == 128, "point count")
                    require(row["total_pairs"] == row["n_a"] * row["n_b"], "pair total")
                    require(0 <= row["candidate_pairs"] <= row["total_pairs"], "residual")
                    require(row["rejected_pairs"] + row["candidate_pairs"] == row["total_pairs"], "sum")
                    require(row["threshold"] == 12 - lane, "threshold")
                    for key in ("generation_ms", "prepare_ms", "plan_ms", "total_component_ms"):
                        require(row[key] >= 0, f"negative time: {key}")
                    require(row["preparation_work"]["validation_points"] == 128, "input validation")
                    comparable.append((row["input_fnv1a64_le_u16_xyz"], row["candidate_pairs"],
                                       row["plan_work"], row["preparation_work"]))
                    checks += 1
                require(comparable[0] == comparable[1] == comparable[2], "s is a precondition only")
    require(checks == 118, "non-vacuity floor")
    print(json.dumps({"status": "passed", "checks": checks, "scope": "p0_probe_cli"}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
