#!/usr/bin/env python3
"""Short lifecycle check for the deliberately unsealed Phase 14R prefix."""

from __future__ import annotations

import json
import subprocess
import sys


def fail(message: str) -> None:
    raise SystemExit(f"massive sparse pair prefix smoke: {message}")


def main() -> None:
    if len(sys.argv) != 2:
        fail("expected the smoke executable")
    completed = subprocess.run(
        [
            sys.argv[1],
            "--point-count",
            "257",
            "--K",
            "10",
            "--prefix-advances",
            "1",
            "--work-budget",
            "64",
            "--spatial-backend",
            "reference_cpu",
        ],
        check=False,
        capture_output=True,
        text=True,
        timeout=10,
    )
    if completed.returncode != 0:
        fail(
            f"executable returned {completed.returncode}: "
            f"{completed.stderr.strip()}"
        )
    try:
        report = json.loads(completed.stdout)
    except json.JSONDecodeError as error:
        fail(f"invalid JSON: {error}")

    if report.get("schema") != (
        "morsehgp3d.phase14.massive_sparse_pair_prefix_smoke.v1"
    ):
        fail("unexpected schema")
    if report.get("spatial_backend_used") != "reference_cpu":
        fail("the host fixture did not use the requested backend")
    for key in (
        "pipeline_complete",
        "pair_authority_sealed",
        "scientific_result_materialized",
        "massive_product_path_claimed",
        "ten_million_point_capacity_claimed",
        "public_status_claimed",
    ):
        if report.get(key) is not False:
            fail(f"{key} must remain false")
    for key in (
        "input_released_before_spatial_build",
        "builder_released_before_pair_prefix",
    ):
        if report.get(key) is not True:
            fail(f"{key} was not certified")

    prefix = report.get("prefix", {})
    if prefix.get("executed_advance_count") != 1:
        fail("the prefix did not execute exactly one bounded advance")
    if prefix.get("session_complete") is not False:
        fail("the fixture unexpectedly completed the pair session")
    if prefix.get("retained_record_count") != 0:
        fail("the unsealed prefix retained a scientific record")
    forbidden = report.get("forbidden_materialization_counts", {})
    if not forbidden or any(value != 0 for value in forbidden.values()):
        fail("a forbidden downstream/global structure was reported")


if __name__ == "__main__":
    main()
