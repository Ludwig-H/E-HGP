"""Observation-to-component coverage for a fitted cubical power hierarchy.

This module is deliberately CPU-reference code.  It computes the canonical
ball/cell intersection relation in bounded site slices; it never substitutes
the cell containing ``x_i`` or ``z_i`` for the power ball required by the HGP
definition.  A future massive backend can implement the same CSR contract with
a two-pass GPU ball/AABB kernel.
"""

from __future__ import annotations

from dataclasses import dataclass
import math
from typing import Any

import numpy as np
from scipy.spatial import cKDTree

from .contracts import GridSpec
from .cubical_msf import CubicalMSFResult
from .spatial_core import grid_centers

EXCLUDED = np.uint8(0)
CONFIRMED = np.uint8(1)
POSSIBLE = np.uint8(2)
STATUS_NAMES = {0: "excluded", 1: "confirmed", 2: "possible"}


@dataclass(frozen=True, slots=True)
class SiteComponentMembership:
    """CSR relation between a contiguous site slice and cubical components."""

    radius: float
    site_start: int
    site_stop: int
    inner_indptr: np.ndarray
    inner_labels: np.ndarray
    outer_indptr: np.ndarray
    outer_labels: np.ndarray
    status: np.ndarray

    def __post_init__(self) -> None:
        n_sites = int(self.site_stop) - int(self.site_start)
        if n_sites < 0:
            raise ValueError("site_stop must be >= site_start")
        for name, indptr, labels in (
            ("inner", self.inner_indptr, self.inner_labels),
            ("outer", self.outer_indptr, self.outer_labels),
        ):
            ptr = np.asarray(indptr, dtype=np.int64)
            ids = np.asarray(labels, dtype=np.int32)
            if ptr.shape != (n_sites + 1,) or ptr[0] != 0 or ptr[-1] != ids.size:
                raise ValueError(f"invalid {name} CSR relation")
            if np.any(ptr[1:] < ptr[:-1]):
                raise ValueError(f"{name} CSR indptr must be nondecreasing")
            object.__setattr__(self, f"{name}_indptr", np.ascontiguousarray(ptr))
            object.__setattr__(self, f"{name}_labels", np.ascontiguousarray(ids))
        states = np.asarray(self.status, dtype=np.uint8)
        if states.shape != (n_sites,) or np.any(states > POSSIBLE):
            raise ValueError("invalid membership status array")
        object.__setattr__(self, "status", np.ascontiguousarray(states))

    @property
    def n_sites(self) -> int:
        return int(self.site_stop - self.site_start)

    def inner_for_site(self, local_site: int) -> np.ndarray:
        start = int(self.inner_indptr[local_site])
        stop = int(self.inner_indptr[local_site + 1])
        return self.inner_labels[start:stop]

    def outer_for_site(self, local_site: int) -> np.ndarray:
        start = int(self.outer_indptr[local_site])
        stop = int(self.outer_indptr[local_site + 1])
        return self.outer_labels[start:stop]

    def summary(self) -> dict[str, Any]:
        counts = np.bincount(self.status, minlength=3)
        return {
            "radius": float(self.radius),
            "site_start": int(self.site_start),
            "site_stop": int(self.site_stop),
            "n_sites": self.n_sites,
            "status_counts": {
                STATUS_NAMES[index]: int(counts[index]) for index in range(3)
            },
            "inner_relations": int(self.inner_labels.size),
            "outer_relations": int(self.outer_labels.size),
        }


def _as_numpy(array: Any) -> np.ndarray:
    if type(array).__module__.split(".", 1)[0] == "cupy":
        import cupy as cp  # type: ignore

        return cp.asnumpy(array)
    return np.asarray(array)


def _cell_relation(
    centers: np.ndarray,
    additive: np.ndarray,
    radius: float,
    labels: np.ndarray,
    spec: GridSpec,
    *,
    max_candidate_cells_per_site: int,
    max_active_cells: int,
) -> tuple[list[np.ndarray], np.ndarray]:
    """Return one ball--cell-component relation.

    The one-component case is common at the larger cuts used by flat-label
    scans.  Enumerating every cell covered by every ball is needlessly
    quadratic there: after the candidate-count guard has been checked, one
    intersecting cell proves the complete component relation.  The nearest
    centre is only a fast witness; a failed witness always falls back to the
    exhaustive broad phase, so this does not approximate the ball--AABB
    predicate.
    """

    active_ids = np.flatnonzero(labels >= 0)
    if active_ids.size == 0:
        return [np.empty(0, dtype=np.int32) for _ in range(centers.shape[0])], labels
    if active_ids.size > int(max_active_cells):
        raise RuntimeError(
            f"CPU coverage has {active_ids.size:,} active cells, above "
            f"max_active_cells={int(max_active_cells):,}"
        )
    active_centers = grid_centers(spec, active_ids, xp=np, dtype=np.float64)
    tree = cKDTree(active_centers, leafsize=32, compact_nodes=True)
    half_widths = 0.5 * np.asarray(spec.cell_widths, dtype=np.float64)
    enclosing_half_diagonal = float(np.linalg.norm(half_widths))
    result: list[np.ndarray] = []
    radius_squared = float(radius) ** 2
    arithmetic_margin = (
        128.0
        * np.finfo(np.float64).eps
        * max(1.0, radius_squared, float(np.max(np.abs(centers))) ** 2)
    )

    active_component_labels = labels[active_ids]
    first_component = int(active_component_labels[0])
    if np.all(active_component_labels == first_component):
        singleton = np.asarray([first_component], dtype=np.int32)
        empty = np.empty(0, dtype=np.int32)
        result = [empty for _ in range(centers.shape[0])]
        remaining_by_site = radius_squared - additive
        valid_rows = np.flatnonzero(remaining_by_site >= -arithmetic_margin)
        if valid_rows.size == 0:
            return result, labels

        ball_radii = np.sqrt(np.maximum(remaining_by_site[valid_rows], 0.0))

        # Preserve the public resource-budget behaviour even though the fast
        # witness path below does not materialise the candidate lists.
        candidate_counts = np.asarray(
            tree.query_ball_point(
                centers[valid_rows],
                ball_radii + enclosing_half_diagonal,
                return_length=True,
            ),
            dtype=np.int64,
        ).reshape(-1)
        if np.any(candidate_counts > int(max_candidate_cells_per_site)):
            raise RuntimeError(
                "power ball intersects too many candidate cells for the CPU "
                "coverage budget; process a finer grid/smaller radius or use the GPU backend"
            )

        _, nearest_rows = tree.query(centers[valid_rows], k=1)
        nearest_rows = np.asarray(nearest_rows, dtype=np.int64).reshape(-1)
        nearest_delta = np.maximum(
            np.abs(active_centers[nearest_rows] - centers[valid_rows]) - half_widths,
            0.0,
        )
        nearest_intersects = np.sum(nearest_delta * nearest_delta, axis=1) <= (
            remaining_by_site[valid_rows] + arithmetic_margin
        )
        # Match the original KD-tree broad phase exactly.  Its search radius
        # does not include ``arithmetic_margin``; a nearest AABB accepted only
        # by that margin must therefore not bypass an empty candidate query.
        nearest_intersects &= candidate_counts > 0
        for local_row in valid_rows[nearest_intersects]:
            result[int(local_row)] = singleton

        # Euclidean-nearest cell centres do not in general minimise distance
        # to equal AABBs (axis clipping can change the order).  Therefore every
        # failed witness is resolved with the original exhaustive predicate.
        for offset in np.flatnonzero(~nearest_intersects):
            row_id = int(valid_rows[offset])
            candidates = tree.query_ball_point(
                centers[row_id],
                float(ball_radii[offset]) + enclosing_half_diagonal,
                return_sorted=True,
            )
            if not candidates:
                continue
            candidate_rows = np.asarray(candidates, dtype=np.int64)
            delta = np.maximum(
                np.abs(active_centers[candidate_rows] - centers[row_id]) - half_widths,
                0.0,
            )
            if np.any(
                np.sum(delta * delta, axis=1)
                <= remaining_by_site[row_id] + arithmetic_margin
            ):
                result[row_id] = singleton
        return result, labels

    for site, weight in zip(centers, additive):
        remaining = radius_squared - float(weight)
        if remaining < -arithmetic_margin:
            result.append(np.empty(0, dtype=np.int32))
            continue
        ball_radius = math.sqrt(max(remaining, 0.0))
        candidates = tree.query_ball_point(
            site, ball_radius + enclosing_half_diagonal, return_sorted=True
        )
        if len(candidates) > int(max_candidate_cells_per_site):
            raise RuntimeError(
                "power ball intersects too many candidate cells for the CPU "
                "coverage budget; process a finer grid/smaller radius or use the GPU backend"
            )
        if not candidates:
            result.append(np.empty(0, dtype=np.int32))
            continue
        candidate_rows = np.asarray(candidates, dtype=np.int64)
        delta = np.maximum(
            np.abs(active_centers[candidate_rows] - site[None, :]) - half_widths,
            0.0,
        )
        intersects = np.sum(delta * delta, axis=1) <= remaining + arithmetic_margin
        cell_ids = active_ids[candidate_rows[intersects]]
        component_ids = np.unique(labels[cell_ids]).astype(np.int32, copy=False)
        result.append(component_ids)
    return result, labels


def _nested_cell_relations(
    centers: np.ndarray,
    additive: np.ndarray,
    radius: float,
    labels_by_relation: tuple[np.ndarray, ...],
    spec: GridSpec,
    *,
    max_candidate_cells_per_site: int,
    max_active_cells: int,
) -> tuple[list[list[np.ndarray]], tuple[np.ndarray, ...]] | None:
    """Evaluate nested active-cell relations with one shared exact geometry pass.

    ``None`` means that no active mask contains all the others, in which case
    callers must use independent trees.  The hierarchy normally has nested
    inner/outer active cells, but retaining the fallback keeps this CPU
    reference correct for independently constructed test forests too.
    """

    active_masks = tuple(labels >= 0 for labels in labels_by_relation)
    active_counts = tuple(int(np.count_nonzero(mask)) for mask in active_masks)
    if any(count > int(max_active_cells) for count in active_counts):
        largest = max(active_counts, default=0)
        raise RuntimeError(
            f"CPU coverage has {largest:,} active cells, above "
            f"max_active_cells={int(max_active_cells):,}"
        )
    if not labels_by_relation:
        return [], labels_by_relation

    primary_order = np.argsort(np.asarray(active_counts))[::-1]
    primary_index: int | None = None
    for candidate in primary_order:
        mask = active_masks[int(candidate)]
        if all(np.all(~other | mask) for other in active_masks):
            primary_index = int(candidate)
            break
    if primary_index is None:
        return None

    primary_ids = np.flatnonzero(active_masks[primary_index])
    n_sites = centers.shape[0]
    if primary_ids.size == 0:
        empty_rows = [
            [np.empty(0, dtype=np.int32) for _ in range(n_sites)]
            for _ in labels_by_relation
        ]
        return empty_rows, labels_by_relation

    active_centers = grid_centers(spec, primary_ids, xp=np, dtype=np.float64)
    tree = cKDTree(active_centers, leafsize=32, compact_nodes=True)
    half_widths = 0.5 * np.asarray(spec.cell_widths, dtype=np.float64)
    enclosing_half_diagonal = float(np.linalg.norm(half_widths))
    radius_squared = float(radius) ** 2
    arithmetic_margin = (
        128.0
        * np.finfo(np.float64).eps
        * max(1.0, radius_squared, float(np.max(np.abs(centers))) ** 2)
    )
    results: list[list[np.ndarray]] = [[] for _ in labels_by_relation]
    for site, weight in zip(centers, additive):
        remaining = radius_squared - float(weight)
        if remaining < -arithmetic_margin:
            for rows in results:
                rows.append(np.empty(0, dtype=np.int32))
            continue
        ball_radius = math.sqrt(max(remaining, 0.0))
        candidates = tree.query_ball_point(
            site, ball_radius + enclosing_half_diagonal, return_sorted=True
        )
        if len(candidates) > int(max_candidate_cells_per_site):
            raise RuntimeError(
                "power ball intersects too many candidate cells for the CPU "
                "coverage budget; process a finer grid/smaller radius or use the GPU backend"
            )
        if not candidates:
            for rows in results:
                rows.append(np.empty(0, dtype=np.int32))
            continue
        candidate_rows = np.asarray(candidates, dtype=np.int64)
        delta = np.maximum(
            np.abs(active_centers[candidate_rows] - site[None, :]) - half_widths,
            0.0,
        )
        intersects = np.sum(delta * delta, axis=1) <= remaining + arithmetic_margin
        intersecting_ids = primary_ids[candidate_rows[intersects]]
        for rows, relation_labels in zip(results, labels_by_relation):
            component_ids = relation_labels[intersecting_ids]
            component_ids = component_ids[component_ids >= 0]
            rows.append(np.unique(component_ids).astype(np.int32, copy=False))
    return results, labels_by_relation


def _to_csr(rows: list[np.ndarray]) -> tuple[np.ndarray, np.ndarray]:
    lengths = np.fromiter((row.size for row in rows), dtype=np.int64, count=len(rows))
    indptr = np.empty(len(rows) + 1, dtype=np.int64)
    indptr[0] = 0
    np.cumsum(lengths, out=indptr[1:])
    labels = (
        np.concatenate(rows).astype(np.int32, copy=False)
        if int(indptr[-1])
        else np.empty(0, dtype=np.int32)
    )
    return indptr, labels


def site_component_membership_cpu(
    sites: Any,
    additive_weights: Any,
    spec: GridSpec,
    inner_forest: CubicalMSFResult,
    outer_forest: CubicalMSFResult,
    *,
    radius: float,
    site_start: int = 0,
    site_stop: int | None = None,
    max_sites: int = 1_000_000,
    max_candidate_cells_per_site: int = 100_000,
    max_active_cells: int = 5_000_000,
    max_total_cells: int = 5_000_000,
) -> SiteComponentMembership:
    """Compute exact power-ball/cubical-component relations for a site slice."""

    level = float(radius)
    if not math.isfinite(level) or level < 0.0:
        raise ValueError("radius must be finite and non-negative")
    if len(sites.shape) != 2 or int(sites.shape[1]) != 3:
        raise ValueError("sites must have shape (N, 3)")
    n_total = int(sites.shape[0])
    if tuple(spec.shape) != tuple(inner_forest.dims) or tuple(spec.shape) != tuple(
        outer_forest.dims
    ):
        raise ValueError("grid spec and inner/outer forests must have identical shapes")
    for name, value in (
        ("max_sites", max_sites),
        ("max_candidate_cells_per_site", max_candidate_cells_per_site),
        ("max_active_cells", max_active_cells),
        ("max_total_cells", max_total_cells),
    ):
        if (
            isinstance(value, (bool, np.bool_))
            or not isinstance(value, (int, np.integer))
            or int(value) < 1
        ):
            raise ValueError(f"{name} must be a positive integer")
    if inner_forest.n_vertices > int(max_total_cells):
        raise RuntimeError(
            f"CPU coverage grid has {inner_forest.n_vertices:,} cells, above "
            f"max_total_cells={int(max_total_cells):,}"
        )
    start = int(site_start)
    stop = n_total if site_stop is None else int(site_stop)
    if not 0 <= start <= stop <= n_total:
        raise ValueError("invalid site slice")
    if stop - start > int(max_sites):
        raise RuntimeError(
            f"CPU coverage slice has {stop-start:,} sites, above max_sites={int(max_sites):,}"
        )
    if stop == start:
        empty_ptr = np.zeros(1, dtype=np.int64)
        return SiteComponentMembership(
            radius=level,
            site_start=start,
            site_stop=stop,
            inner_indptr=empty_ptr,
            inner_labels=np.empty(0, dtype=np.int32),
            outer_indptr=empty_ptr.copy(),
            outer_labels=np.empty(0, dtype=np.int32),
            status=np.empty(0, dtype=np.uint8),
        )
    centers = np.ascontiguousarray(_as_numpy(sites[start:stop]), dtype=np.float64)
    additive = np.ascontiguousarray(
        _as_numpy(additive_weights[start:stop]), dtype=np.float64
    )
    if additive_weights.shape != (n_total,):
        raise ValueError("additive_weights must have shape (N,)")
    if not np.isfinite(centers).all() or not np.isfinite(additive).all():
        raise ValueError("sites and additive_weights must be finite")
    if additive.size and float(additive.min()) < 0.0:
        raise ValueError("additive_weights must be non-negative")
    inner_active_count = int(np.count_nonzero(inner_forest.births <= level))
    outer_active_count = int(np.count_nonzero(outer_forest.births <= level))
    if max(inner_active_count, outer_active_count) > int(max_active_cells):
        raise RuntimeError(
            "CPU coverage active-cell budget exceeded before constructing cuts: "
            f"inner={inner_active_count:,}, outer={outer_active_count:,}, "
            f"limit={int(max_active_cells):,}"
        )
    inner_cell_labels = inner_forest.components_at_cut(level)
    outer_cell_labels = outer_forest.components_at_cut(level)
    relation_options = {
        "max_candidate_cells_per_site": max_candidate_cells_per_site,
        "max_active_cells": max_active_cells,
    }
    identical_cuts = np.array_equal(inner_cell_labels, outer_cell_labels)
    if identical_cuts:
        # Identical cuts have identical canonical geometry and component IDs.
        # Reusing the immutable row arrays avoids constructing and traversing a
        # second spatial index without changing either CSR relation.
        inner_rows, _ = _cell_relation(
            centers,
            additive,
            level,
            inner_cell_labels,
            spec,
            **relation_options,
        )
        outer_rows = inner_rows
    else:
        nonempty_labels = (
            inner_cell_labels[inner_cell_labels >= 0],
            outer_cell_labels[outer_cell_labels >= 0],
        )
        singleton_relations = tuple(
            values.size > 0 and np.all(values == values[0])
            for values in nonempty_labels
        )
        if any(singleton_relations):
            # Let each singleton use its exact witness shortcut.  The remaining
            # relation (if any) is evaluated independently.
            inner_rows, _ = _cell_relation(
                centers,
                additive,
                level,
                inner_cell_labels,
                spec,
                **relation_options,
            )
            outer_rows, _ = _cell_relation(
                centers,
                additive,
                level,
                outer_cell_labels,
                spec,
                **relation_options,
            )
        else:
            shared = _nested_cell_relations(
                centers,
                additive,
                level,
                (inner_cell_labels, outer_cell_labels),
                spec,
                **relation_options,
            )
            if shared is None:
                inner_rows, _ = _cell_relation(
                    centers,
                    additive,
                    level,
                    inner_cell_labels,
                    spec,
                    **relation_options,
                )
                outer_rows, _ = _cell_relation(
                    centers,
                    additive,
                    level,
                    outer_cell_labels,
                    spec,
                    **relation_options,
                )
            else:
                (inner_rows, outer_rows), _ = shared

    if identical_cuts:
        # The relations are literally equal and every component maps to itself
        # with one preimage, so only emptiness distinguishes the two statuses.
        status = np.fromiter(
            (CONFIRMED if row.size else EXCLUDED for row in outer_rows),
            dtype=np.uint8,
            count=stop - start,
        )
    else:
        # Every active inner cell is active in the outer envelope.  Map inner
        # component IDs into outer component IDs before comparing relations.
        inner_to_outer: dict[int, int] = {}
        outer_preimages: dict[int, set[int]] = {}
        for cell_id in np.flatnonzero(inner_cell_labels >= 0):
            inner_label = int(inner_cell_labels[cell_id])
            outer_label = int(outer_cell_labels[cell_id])
            inner_to_outer.setdefault(inner_label, outer_label)
            outer_preimages.setdefault(outer_label, set()).add(inner_label)
        status = np.empty(stop - start, dtype=np.uint8)
        for row_id, (inner, outer) in enumerate(zip(inner_rows, outer_rows)):
            if outer.size == 0:
                status[row_id] = EXCLUDED
                continue
            mapped_inner = np.unique(
                np.fromiter(
                    (inner_to_outer[int(label)] for label in inner),
                    dtype=np.int32,
                    count=inner.size,
                )
            )
            one_to_one = all(
                len(outer_preimages.get(int(label), set())) == 1 for label in outer
            )
            status[row_id] = (
                CONFIRMED
                if one_to_one and np.array_equal(mapped_inner, outer)
                else POSSIBLE
            )

    inner_indptr, inner_labels = _to_csr(inner_rows)
    outer_indptr, outer_labels = _to_csr(outer_rows)
    return SiteComponentMembership(
        radius=level,
        site_start=start,
        site_stop=stop,
        inner_indptr=inner_indptr,
        inner_labels=inner_labels,
        outer_indptr=outer_indptr,
        outer_labels=outer_labels,
        status=status,
    )


__all__ = [
    "CONFIRMED",
    "EXCLUDED",
    "POSSIBLE",
    "STATUS_NAMES",
    "SiteComponentMembership",
    "site_component_membership_cpu",
]
