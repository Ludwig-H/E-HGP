#!/usr/bin/env python3
"""R2-d: profile the exact product runner across a point-count x tile-target grid.

This is a PROFILING harness, not a qualification one.  It exists because the
route to 50 000 points has never been measured at any intermediate size: the
only two anchors are n=32 (where the LBVH is too flat for bulk pruning, so
per-support cost is a worst case) and n=50 000 (where nothing has ever
completed).  A single jump between them cannot say which stage breaks, so
this harness walks the grid and publishes the ventilation the runner already
emits per stage, plus the exact BigInt mass accounting R2-c added.

It claims nothing.  No SLO, no scale gate, no public status, no
extrapolation.  A censored cell is published as censored with the exact
fraction of C(n,3)+C(n,4) it decided before the stop; it is never dropped,
never replaced by a partial duration, and never fed into a scaling slope.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
from fractions import Fraction
from typing import Any, NoReturn

MANIFEST_SCHEMA = "morsehgp3d.phase15.reduced_verification_r2_sweep.v1"
HARNESS_ROLE = "profile_only"
MAX_SESSION_SECONDS = 28_800
TIMEOUT_KILL_AFTER_SECONDS = 30

# The runner's own typed exit codes.
RC_COMPLETE = 0
RC_CERTIFICATION_FAILURE = 3
RC_INVALID_INPUT = 4
RC_OPERATIONAL_FAILURE = 5
RC_OPERATIONAL_DEADLINE = 6

# Every stage the runner ventilates, in pipeline order.  Kept explicit so a
# renamed or dropped stage is a harness error rather than a silent hole.
STAGE_KEYS = (
    "generation",
    "canonicalization",
    "lbvh",
    "pair_support",
    "higher_support",
    "terminal_facade",
    "event_journal",
    "saddle_seed_journal",
    "batch_plan",
    "reducer_setup",
    "reducer_stream",
    "forest_finish",
    "total",
)


class ContractError(RuntimeError):
    """The runner or its JSON escaped the profiling contract."""


def fail(message: str) -> NoReturn:
    raise ContractError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            fail(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def canonical_json(value: Any) -> bytes:
    return (
        json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)
        + "\n"
    ).encode("ascii")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def atomic_write(path: Path, payload: bytes) -> None:
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f".{path.name}.", suffix=".partial", dir=path.parent
    )
    temporary = Path(temporary_name)
    try:
        offset = 0
        while offset < len(payload):
            offset += os.write(descriptor, payload[offset:])
        os.fsync(descriptor)
        os.close(descriptor)
        descriptor = -1
        os.replace(temporary, path)
        directory = os.open(path.parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
        try:
            os.fsync(directory)
        finally:
            os.close(directory)
    finally:
        if descriptor >= 0:
            os.close(descriptor)
        temporary.unlink(missing_ok=True)


def exact_mass(value: Any, label: str) -> int:
    """A canonical decimal exact::BigInt string, parsed without loss."""
    require(isinstance(value, str) and value, f"{label} must be a decimal string")
    require(
        value == "0" or (value.lstrip("-").isdigit() and value.lstrip("-")[0] != "0"),
        f"{label} is not a canonical decimal integer",
    )
    return int(value)


def candidate_universe(point_count: int) -> int:
    """C(n,3) + C(n,4), the higher-support universe the audit must close."""
    from math import comb

    return comb(point_count, 3) + comb(point_count, 4)


def run_one_cell(
    *,
    binary: Path,
    point_count: int,
    tile_target: int,
    maximum_order: int,
    family: str,
    higher_backend: str,
    verification_basis: str,
    tile_roots: int,
    frontier_target: int,
    cell_deadline_ms: int,
    kill_after_seconds: int,
) -> dict[str, Any]:
    command = [
        str(binary),
        "--point-count",
        str(point_count),
        "--K",
        str(maximum_order),
        "--family",
        family,
        "--mode",
        "complete_resident_diagnostic",
        # SPECIFICATION_MORSEHGP3D.md 1.1: the exact industrial version
        # carries no configured budget, and no other profile may be
        # measured.  Note that this harness passes NO --*-budget option --
        # the runner rejects the combination, and rightly so: a cap that is
        # merely large is still a cap.
        "--budget-profile",
        "unbudgeted_industrial",
        "--operational-deadline-ms",
        str(cell_deadline_ms),
        "--higher-backend",
        higher_backend,
        "--higher-verification-basis",
        verification_basis,
        "--higher-tile-target",
        str(tile_target),
        # T1: bound how many frontier roots one transaction consumes, so a
        # censored cell still reports committed progress instead of zero.
        "--higher-tile-roots",
        str(tile_roots),
        # T2: the frontier is host state, so it is refined independently of
        # what a tile feeds the device -- and the G4 measurement showed the
        # refinement is what decides whether a tile terminates at all.
        "--higher-frontier-target",
        str(frontier_target),
    ]
    # The runner's cooperative deadline is the first line of defence; GNU
    # timeout is the second, and a cell killed by it is an observation, not
    # a missing row.
    wall_budget = cell_deadline_ms / 1000.0 + kill_after_seconds
    guarded = [
        "timeout",
        "--kill-after",
        f"{kill_after_seconds}s",
        f"{wall_budget:.3f}s",
        *command,
    ]
    started = time.monotonic()
    completed = subprocess.run(
        guarded, capture_output=True, check=False
    )  # noqa: S603
    elapsed = time.monotonic() - started

    cell: dict[str, Any] = {
        "point_count": point_count,
        "tile_target": tile_target,
        "maximum_order": maximum_order,
        "family": family,
        "higher_backend": higher_backend,
        "requested_verification_basis": verification_basis,
        "tile_roots": tile_roots,
        "frontier_target": frontier_target,
        "cell_deadline_ms": cell_deadline_ms,
        "command": command,
        "return_code": completed.returncode,
        "harness_wall_seconds": round(elapsed, 6),
    }
    if completed.returncode in (124, 137):
        cell["status"] = "killed_by_external_timeout"
        cell["report"] = None
        return cell
    try:
        report = json.loads(
            completed.stdout.decode("utf-8"), object_pairs_hook=reject_duplicate_keys
        )
    except (UnicodeDecodeError, json.JSONDecodeError, ContractError) as error:
        cell["status"] = "unparsable_report"
        cell["decode_error"] = str(error)
        cell["stderr_tail"] = completed.stderr.decode("utf-8", "replace")[-2000:]
        cell["report"] = None
        return cell
    require(isinstance(report, dict), "the runner report must be a JSON object")
    cell["report"] = report
    cell["status"] = {
        RC_COMPLETE: "complete",
        RC_CERTIFICATION_FAILURE: "certification_failure",
        RC_INVALID_INPUT: "invalid_input",
        RC_OPERATIONAL_FAILURE: "operational_failure",
        RC_OPERATIONAL_DEADLINE: "censored_operational_deadline",
    }.get(completed.returncode, "unexpected_return_code")
    return cell


def summarize_cell(cell: dict[str, Any]) -> dict[str, Any]:
    """Derive only what the report actually states.  No extrapolation."""
    summary: dict[str, Any] = {
        "point_count": cell["point_count"],
        "tile_target": cell["tile_target"],
        "status": cell["status"],
        "return_code": cell["return_code"],
        "harness_wall_seconds": cell["harness_wall_seconds"],
    }
    report = cell.get("report")
    if not isinstance(report, dict):
        return summary

    timings = report.get("timings_ms")
    require(isinstance(timings, dict), "the report must ventilate timings_ms")
    missing = [key for key in STAGE_KEYS if key not in timings]
    require(not missing, f"the report lost the stages {missing}")
    summary["timings_ms"] = {key: timings[key] for key in STAGE_KEYS}

    higher = report.get("higher_support")
    require(isinstance(higher, dict), "the report must publish higher_support")
    progress = higher.get("progress")
    require(isinstance(progress, dict), "higher_support must publish its progress")

    total = exact_mass(progress.get("total_support_mass"), "total_support_mass")
    resolved = exact_mass(
        progress.get("resolved_support_mass"), "resolved_support_mass"
    )
    remaining = exact_mass(
        progress.get("remaining_frontier_support_mass"),
        "remaining_frontier_support_mass",
    )
    universe = candidate_universe(cell["point_count"])
    # The anchored induction identity, rechecked outside the binary.  A
    # transcript that fails it is not a slow measurement, it is a wrong one.
    require(
        total == 0 or total == universe,
        "the reported universe is neither absent nor C(n,3)+C(n,4)",
    )
    require(
        total == 0 or resolved + remaining == total,
        "R + C(F) != C(n,3)+C(n,4) in the published transcript",
    )

    summary["higher"] = {
        "status": higher.get("status"),
        "stop_reason": higher.get("stop_reason"),
        "assembly_censure": higher.get("assembly_censure"),
        "verification_basis": higher.get("verification_basis"),
        "total_support_mass": progress.get("total_support_mass"),
        "resolved_support_mass": progress.get("resolved_support_mass"),
        "remaining_frontier_support_mass": progress.get(
            "remaining_frontier_support_mass"
        ),
        "minimal_leaves": progress.get("minimal_leaves"),
        "above_rank_leaves": progress.get("above_rank_leaves"),
        "leaf_support_analyses": progress.get("leaf_support_analyses"),
        "point_classifications": progress.get("point_classifications"),
        "closed_ball_node_visits": progress.get("closed_ball_node_visits"),
        "committed_transactions": progress.get("committed_transactions"),
        "committed_tile_transactions": progress.get("committed_tile_transactions"),
        "committed_expansion_transitions": progress.get(
            "committed_expansion_transitions"
        ),
        "committed_tile_slots": progress.get("committed_tile_slots"),
        "frontier_entries": progress.get("frontier_entries"),
        "chunks": higher.get("chunks"),
    }
    if total != 0:
        fraction = Fraction(resolved, total)
        summary["higher"]["resolved_fraction_numerator"] = str(fraction.numerator)
        summary["higher"]["resolved_fraction_denominator"] = str(
            fraction.denominator
        )
        # Lossy, for reading only; the exact ratio is the pair above.
        summary["higher"]["resolved_fraction_binary64_diagnostic"] = float(fraction)

    minimal = progress.get("minimal_leaves")
    higher_ms = timings.get("higher_support")
    if isinstance(minimal, int) and minimal > 0 and isinstance(higher_ms, (int, float)):
        summary["higher"]["us_per_minimal_leaf"] = round(
            higher_ms * 1000.0 / minimal, 4
        )
    tiles = progress.get("committed_tile_transactions")
    if isinstance(tiles, int) and tiles > 0 and isinstance(higher_ms, (int, float)):
        summary["higher"]["ms_per_tile_transaction"] = round(higher_ms / tiles, 6)
        expansions = progress.get("committed_expansion_transitions")
        if isinstance(expansions, int):
            # The ratio that decides whether raising --higher-tile-target
            # helps: expansion transitions are pure host commits and their
            # count is set by the tree, not by the tile target.
            summary["higher"]["expansion_transitions_per_tile"] = round(
                expansions / tiles, 4
            )
    return summary


def scaling_rows(summaries: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Consecutive-size ratios, over COMPLETE cells only.

    A censored cell's duration is its deadline, not its cost.  Feeding one
    into a slope would manufacture a scaling law out of a circuit breaker.
    """
    rows: list[dict[str, Any]] = []
    by_target: dict[int, list[dict[str, Any]]] = {}
    for summary in summaries:
        if summary["status"] != "complete":
            continue
        by_target.setdefault(summary["tile_target"], []).append(summary)
    for tile_target, cells in sorted(by_target.items()):
        cells.sort(key=lambda cell: cell["point_count"])
        for previous, current in zip(cells, cells[1:]):
            row: dict[str, Any] = {
                "tile_target": tile_target,
                "from_point_count": previous["point_count"],
                "to_point_count": current["point_count"],
                "point_count_ratio": current["point_count"] / previous["point_count"],
            }
            for stage in STAGE_KEYS:
                before = previous["timings_ms"][stage]
                after = current["timings_ms"][stage]
                row[f"{stage}_ratio"] = (
                    round(after / before, 6) if before > 0 else None
                )
            before_leaves = previous["higher"].get("minimal_leaves")
            after_leaves = current["higher"].get("minimal_leaves")
            if isinstance(before_leaves, int) and before_leaves > 0:
                row["minimal_leaf_ratio"] = round(after_leaves / before_leaves, 6)
            rows.append(row)
    return rows


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument(
        "--point-counts",
        default="32,512,4096,50000",
        help="comma-separated; the escalier between the two known anchors",
    )
    parser.add_argument("--tile-targets", default="16,256,1024")
    parser.add_argument("--maximum-order", type=int, default=10)
    parser.add_argument("--family", default="uniform_latin")
    parser.add_argument("--higher-backend", default="device_tiled_session")
    parser.add_argument("--verification-basis", default="tile_certified")
    parser.add_argument("--tile-roots", type=int, default=1024)
    parser.add_argument("--frontier-target", type=int, default=16)
    parser.add_argument(
        "--cell-deadline-ms",
        type=int,
        default=120_000,
        help="cooperative per-cell deadline handed to the runner",
    )
    parser.add_argument(
        "--deadline-epoch",
        type=float,
        required=True,
        help="hard session boundary; set it below the guest circuit breaker",
    )
    parser.add_argument(
        "--kill-after-seconds", type=int, default=TIMEOUT_KILL_AFTER_SECONDS
    )
    arguments = parser.parse_args()

    binary = arguments.binary.resolve()
    require(binary.is_file() and os.access(binary, os.X_OK), "binary is not executable")
    require(arguments.maximum_order >= 1, "maximum-order must be positive")
    require(arguments.cell_deadline_ms > 0, "cell-deadline-ms must be positive")
    remaining_session = arguments.deadline_epoch - time.time()
    require(
        0 < remaining_session <= MAX_SESSION_SECONDS,
        "deadline-epoch must be in the future and within the session bound",
    )

    point_counts = [int(text) for text in arguments.point_counts.split(",") if text]
    tile_targets = [int(text) for text in arguments.tile_targets.split(",") if text]
    require(point_counts and tile_targets, "the grid must be non-empty")
    require(all(count > 0 for count in point_counts), "point counts must be positive")
    require(
        all(1 <= target <= 1024 for target in tile_targets),
        "tile targets must be in [1, 1024]",
    )

    cells: list[dict[str, Any]] = []
    summaries: list[dict[str, Any]] = []
    skipped: list[dict[str, Any]] = []
    # Small first: a cheap cell that fails tells us the grid is wrong before
    # an expensive one burns the session.
    grid = [
        (count, target) for count in sorted(point_counts) for target in sorted(tile_targets)
    ]
    for point_count, tile_target in grid:
        budget_left = arguments.deadline_epoch - time.time()
        needed = arguments.cell_deadline_ms / 1000.0 + arguments.kill_after_seconds
        if budget_left <= needed:
            # Recorded, never silently dropped: a grid with holes must say so.
            skipped.append(
                {
                    "point_count": point_count,
                    "tile_target": tile_target,
                    "status": "not_run_session_deadline",
                    "session_seconds_left": round(budget_left, 3),
                }
            )
            continue
        cell = run_one_cell(
            binary=binary,
            point_count=point_count,
            tile_target=tile_target,
            maximum_order=arguments.maximum_order,
            family=arguments.family,
            higher_backend=arguments.higher_backend,
            verification_basis=arguments.verification_basis,
            tile_roots=arguments.tile_roots,
            frontier_target=arguments.frontier_target,
            cell_deadline_ms=arguments.cell_deadline_ms,
            kill_after_seconds=arguments.kill_after_seconds,
        )
        cells.append(cell)
        summary = summarize_cell(cell)
        summaries.append(summary)
        print(
            f"n={point_count:>7} tile={tile_target:>5} "
            f"status={summary['status']:<32} "
            f"wall={summary['harness_wall_seconds']:.3f}s",
            flush=True,
        )

    manifest = {
        "schema": MANIFEST_SCHEMA,
        "role": HARNESS_ROLE,
        "phase": 15,
        "increment": "R2-d",
        "deployment_status": "architecture_only",
        "public_status": "not_claimed",
        "slo_claimed": False,
        "scale_gate_claimed": False,
        "exactness_claimed": False,
        "extrapolation_published": False,
        "binary": {
            "path": str(binary),
            "sha256": sha256_file(binary),
        },
        "grid": {
            "point_counts": point_counts,
            "tile_targets": tile_targets,
            "maximum_order": arguments.maximum_order,
            "family": arguments.family,
            "higher_backend": arguments.higher_backend,
            "requested_verification_basis": arguments.verification_basis,
            "tile_roots": arguments.tile_roots,
            "frontier_target": arguments.frontier_target,
            "cell_deadline_ms": arguments.cell_deadline_ms,
        },
        "cells": cells,
        "summaries": summaries,
        "skipped": skipped,
        "scaling": scaling_rows(summaries),
        "notes": [
            "profiling only: no SLO, no scale gate, no public status",
            "censored cells keep their exact resolved fraction and are excluded "
            "from every scaling ratio, because their duration is the deadline",
            "cells not run because the session deadline arrived are listed in "
            "skipped, never omitted",
        ],
    }
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    atomic_write(arguments.output, canonical_json(manifest))
    complete = sum(1 for cell in summaries if cell["status"] == "complete")
    print(
        f"{complete}/{len(summaries)} cells complete, {len(skipped)} not run; "
        f"manifest at {arguments.output}",
        flush=True,
    )
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ContractError as error:
        print(f"contract error: {error}", file=sys.stderr)
        sys.exit(2)
