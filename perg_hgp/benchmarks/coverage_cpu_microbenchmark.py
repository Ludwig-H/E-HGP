#!/usr/bin/env python3
"""Reproducible before/after microbenchmark for CPU ball--AABB coverage.

The legacy function is the pre-optimization implementation copied verbatim in
structure: one tree and exhaustive candidate materialization for each of the
inner and outer relations.  The optimized measurement uses the production
``site_component_membership_cpu`` path, including the two MSF cuts and status
construction, so its timing is conservative.  Both paths are checked against
the same expected one-component relation before a speedup is reported.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import statistics
import sys
import time

import numpy as np
import scipy
from scipy.spatial import cKDTree

REPOSITORY_PACKAGE = Path(__file__).resolve().parents[1]
if str(REPOSITORY_PACKAGE) not in sys.path:
    sys.path.insert(0, str(REPOSITORY_PACKAGE))

from perg_hgp.backends.power_cover_3d_cuda.contracts import GridSpec
from perg_hgp.backends.power_cover_3d_cuda.coverage import (
    site_component_membership_cpu,
)
from perg_hgp.backends.power_cover_3d_cuda.cubical_msf import cubical_msf_cpu
from perg_hgp.backends.power_cover_3d_cuda.spatial_core import grid_centers


def _legacy_cell_relation(centers, additive, radius, labels, spec):
    active_ids = np.flatnonzero(labels >= 0)
    active_centers = grid_centers(spec, active_ids, xp=np, dtype=np.float64)
    tree = cKDTree(active_centers, leafsize=32, compact_nodes=True)
    half_widths = 0.5 * np.asarray(spec.cell_widths, dtype=np.float64)
    enclosing_half_diagonal = float(np.linalg.norm(half_widths))
    radius_squared = float(radius) ** 2
    arithmetic_margin = (
        128.0
        * np.finfo(np.float64).eps
        * max(1.0, radius_squared, float(np.max(np.abs(centers))) ** 2)
    )
    rows = []
    for site, weight in zip(centers, additive):
        remaining = radius_squared - float(weight)
        if remaining < -arithmetic_margin:
            rows.append(np.empty(0, dtype=np.int32))
            continue
        candidates = tree.query_ball_point(
            site,
            math.sqrt(max(remaining, 0.0)) + enclosing_half_diagonal,
            return_sorted=True,
        )
        candidate_rows = np.asarray(candidates, dtype=np.int64)
        delta = np.maximum(
            np.abs(active_centers[candidate_rows] - site[None, :]) - half_widths,
            0.0,
        )
        intersects = np.sum(delta * delta, axis=1) <= (
            remaining + arithmetic_margin
        )
        rows.append(
            np.unique(labels[active_ids[candidate_rows[intersects]]]).astype(
                np.int32, copy=False
            )
        )
    return rows


def _timed(callable_, repeats):
    samples = []
    value = None
    for _ in range(repeats):
        started = time.perf_counter()
        value = callable_()
        samples.append(time.perf_counter() - started)
    return value, samples


def _sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--grid", type=int, default=32)
    parser.add_argument("--sites", type=int, default=256)
    parser.add_argument("--radius", type=float, default=2.0)
    parser.add_argument("--seed", type=int, default=20260712)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.grid < 1 or args.sites < 1 or args.repeats < 1 or args.radius < 0:
        parser.error("grid, sites, repeats must be positive and radius non-negative")

    shape = (args.grid,) * 3
    widths = (1.0 / args.grid,) * 3
    spec = GridSpec(
        bbox_min=(0.0, 0.0, 0.0),
        bbox_max=(1.0, 1.0, 1.0),
        shape=shape,
        cell_widths=widths,
        half_diagonal=math.sqrt(3.0) / (2.0 * args.grid),
    )
    rng = np.random.default_rng(args.seed)
    sites = rng.uniform(0.0, 1.0, size=(args.sites, 3))
    additive = np.zeros(args.sites, dtype=np.float64)
    forest = cubical_msf_cpu(np.zeros(np.prod(shape)), dims=shape)
    labels = forest.components_at_cut(args.radius)

    # The old public path built both trees and repeated the same geometry pass.
    legacy, legacy_samples = _timed(
        lambda: (
            _legacy_cell_relation(sites, additive, args.radius, labels, spec),
            _legacy_cell_relation(sites, additive, args.radius, labels, spec),
        ),
        args.repeats,
    )
    optimized, optimized_samples = _timed(
        lambda: site_component_membership_cpu(
            sites,
            additive,
            spec,
            forest,
            forest,
            radius=args.radius,
            max_sites=args.sites,
            max_candidate_cells_per_site=int(np.prod(shape)),
            max_active_cells=int(np.prod(shape)),
            max_total_cells=int(np.prod(shape)),
        ),
        args.repeats,
    )

    expected = np.zeros(args.sites, dtype=np.int32)
    legacy_ok = all(
        row.shape == (1,) and int(row[0]) == 0
        for relation in legacy
        for row in relation
    )
    optimized_ok = (
        np.array_equal(optimized.inner_indptr, np.arange(args.sites + 1))
        and np.array_equal(optimized.outer_indptr, np.arange(args.sites + 1))
        and np.array_equal(optimized.inner_labels, expected)
        and np.array_equal(optimized.outer_labels, expected)
    )
    if not legacy_ok or not optimized_ok:
        raise RuntimeError("legacy/optimized relation parity check failed")

    legacy_median = statistics.median(legacy_samples)
    optimized_median = statistics.median(optimized_samples)
    report = {
        "schema": "perg_hgp.coverage_cpu_microbenchmark",
        "seed": args.seed,
        "grid_shape": list(shape),
        "sites": args.sites,
        "radius": args.radius,
        "repeats": args.repeats,
        "candidate_cells_per_site": int(np.prod(shape)),
        "legacy_seconds": legacy_samples,
        "optimized_seconds": optimized_samples,
        "legacy_median_seconds": legacy_median,
        "optimized_median_seconds": optimized_median,
        "speedup_median": legacy_median / optimized_median,
        "relation_parity": True,
        "runtime": {
            "python": platform.python_version(),
            "numpy": np.__version__,
            "scipy": scipy.__version__,
            "platform": platform.platform(),
            "machine": platform.machine(),
            "logical_cpu_count": os.cpu_count(),
        },
        "source_sha256": {
            "coverage.py": _sha256(
                REPOSITORY_PACKAGE
                / "perg_hgp/backends/power_cover_3d_cuda/coverage.py"
            ),
            "coverage_cpu_microbenchmark.py": _sha256(Path(__file__).resolve()),
        },
        "note": (
            "legacy times two exhaustive relations; optimized times the full "
            "membership path including two MSF cuts and status construction"
        ),
    }
    payload = json.dumps(report, indent=2, sort_keys=True)
    print(payload)
    if args.output is not None:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(payload + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
