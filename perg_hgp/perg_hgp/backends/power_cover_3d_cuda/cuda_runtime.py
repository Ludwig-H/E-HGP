"""Optional RAPIDS/CuPy runtime for NVIDIA GPUs.

Imports are intentionally lazy so the exact CPU reference and the normal
``perg_hgp`` package remain usable without CUDA or RAPIDS installed.
"""

from __future__ import annotations

from collections import Counter
import json
import math
import operator
import os
import platform
import subprocess
import sys
from dataclasses import dataclass
from typing import Any

import numpy as np


# RAPIDS 26.02 dispatches the low-dimensional RBC KNN kernel through fixed
# specializations ending at k=1024.  cuML only checks the independent
# sqrt(N)-landmark condition, so asking for k=1025 otherwise reaches RBC with
# an unsupported rank and can return the initialized sentinel output.
RBC_MAX_QUERY_K = 1_024
RBC_OVERFETCH_FACTOR = 4


_RBC_DISTANCE_RECOMPUTE_KERNEL: Any | None = None
_DIRECT_AUDIT_SQUARED_KERNEL: Any | None = None


class CUDABackendUnavailable(RuntimeError):
    pass


def require_cuda_stack():
    try:
        import cupy as cp  # type: ignore
        import cuml  # type: ignore
        from cuml.neighbors import NearestNeighbors  # type: ignore
    except Exception as error:  # pragma: no cover - exercised on Colab
        raise CUDABackendUnavailable(
            "power_cover_3d_cuda requires compatible CuPy and RAPIDS cuML "
            "packages; use the official RAPIDS Colab installer"
        ) from error
    if cp.cuda.runtime.getDeviceCount() < 1:
        raise CUDABackendUnavailable("no CUDA device is visible")
    return cp, cuml, NearestNeighbors


@dataclass(frozen=True)
class NeighborAudit:
    queries: int
    k: int
    values_match: bool
    maximum_absolute_error: float
    rbc_seconds: float
    brute_force_seconds: float
    comparison_tolerance: float = 0.0
    audit_scope: str = "sampled_query_value_comparison"
    is_numerical_proof: bool = False
    distance_values_match: bool | None = None
    identifiers_match: bool | None = None
    support_equivalent: bool | None = None
    support_equivalence_required: bool = True
    identifier_mismatch_rows: int = 0
    coordinate_equivalent_tie_rows: int = 0
    non_equivalent_support_rows: int = 0
    maximum_relative_error: float = 0.0
    reference_distance_source: str = "stored_float32_coordinates_recomputed_float64"
    library_reference_distances_used: bool = False
    rbc_reported_distances_recomputed: bool = True
    cuvs_reported_maximum_absolute_error: float | None = None
    cuvs_brute_force_seconds: float | None = None
    cuvs_support_equivalent_to_direct: bool | None = None
    cuvs_non_equivalent_support_rows: int | None = None

    def to_dict(self):
        return self.__dict__.copy()


def _scale_aware_squared_distance_tolerance(
    observed: np.ndarray, reference: np.ndarray, *, factor: float = 128.0
) -> np.ndarray:
    """Return a per-rank float32 tolerance without a unit-scale floor.

    Normalized 30-million-point clouds routinely have squared KNN distances of
    order ``1e-5``.  A tolerance proportional to ``max(1, distance_squared)``
    is therefore large enough to hide an order-one rank error.  Both backends
    operate on stored float32 geometry, so a relative/ULP envelope is the
    appropriate empirical comparison here.
    """

    left = np.asarray(observed, dtype=np.float32)
    right = np.asarray(reference, dtype=np.float32)
    if left.shape != right.shape:
        raise ValueError("observed and reference distances must have the same shape")
    if not np.isfinite(left).all() or not np.isfinite(right).all():
        raise ValueError("distance audit inputs must be finite")
    if np.any(left < 0.0) or np.any(right < 0.0):
        raise ValueError("squared distances must be non-negative")
    scale = np.maximum(
        np.abs(left).astype(np.float64), np.abs(right).astype(np.float64)
    )
    ulp = np.maximum(
        np.abs(np.spacing(left)).astype(np.float64),
        np.abs(np.spacing(right)).astype(np.float64),
    )
    relative = np.finfo(np.float32).eps * scale
    return float(factor) * np.maximum(relative, ulp)


def _tie_aware_support_summary(
    observed_indices: np.ndarray,
    reference_indices: np.ndarray,
    observed_points: np.ndarray,
    reference_points: np.ndarray,
) -> dict[str, int | bool]:
    """Compare KNN supports, accepting only geometry-identical ID aliases.

    Equal distance values alone are insufficient for entropy regularization:
    replacing a boundary neighbour by a different point on the same sphere can
    change the Gibbs barycentre.  Different identifiers are consequently safe
    only when they refer to exactly identical stored coordinates.  Reordering
    the same identifiers inside a tie is also harmless.
    """

    observed_ids = np.asarray(observed_indices, dtype=np.int64)
    reference_ids = np.asarray(reference_indices, dtype=np.int64)
    observed_xyz = np.asarray(observed_points, dtype=np.float32)
    reference_xyz = np.asarray(reference_points, dtype=np.float32)
    if observed_ids.shape != reference_ids.shape or observed_ids.ndim != 2:
        raise ValueError("neighbor identifier arrays must have the same 2-D shape")
    expected_points_shape = observed_ids.shape + (3,)
    if (
        observed_xyz.shape != expected_points_shape
        or reference_xyz.shape != expected_points_shape
    ):
        raise ValueError("gathered neighbor coordinates have inconsistent shapes")

    mismatch_rows = 0
    coordinate_equivalent_rows = 0
    non_equivalent_rows = 0
    for row in range(observed_ids.shape[0]):
        observed_row = observed_ids[row]
        reference_row = reference_ids[row]
        if np.unique(observed_row).size != observed_row.size or np.unique(
            reference_row
        ).size != reference_row.size:
            mismatch_rows += 1
            non_equivalent_rows += 1
            continue
        observed_set = set(int(value) for value in observed_row)
        reference_set = set(int(value) for value in reference_row)
        if observed_set == reference_set:
            continue

        mismatch_rows += 1
        observed_only = [
            position
            for position, identifier in enumerate(observed_row)
            if int(identifier) not in reference_set
        ]
        reference_only = [
            position
            for position, identifier in enumerate(reference_row)
            if int(identifier) not in observed_set
        ]
        observed_coordinates = Counter(
            tuple(float(value) for value in observed_xyz[row, position])
            for position in observed_only
        )
        reference_coordinates = Counter(
            tuple(float(value) for value in reference_xyz[row, position])
            for position in reference_only
        )
        if observed_coordinates == reference_coordinates:
            coordinate_equivalent_rows += 1
        else:
            non_equivalent_rows += 1

    return {
        "identifiers_match": mismatch_rows == 0,
        "support_equivalent": non_equivalent_rows == 0,
        "identifier_mismatch_rows": mismatch_rows,
        "coordinate_equivalent_tie_rows": coordinate_equivalent_rows,
        "non_equivalent_support_rows": non_equivalent_rows,
    }


def _recompute_returned_distances(
    cp: Any, queries: Any, points: Any, indices: Any
) -> Any:
    """Recompute Euclidean distances while retaining each returned site ID.

    The NumPy branch is used only by the dependency-free contract tests.  The
    production CuPy branch is fused so its live memory is Q x K rather than
    Q x K x 3.
    """

    if isinstance(queries, np.ndarray):
        query_values = np.asarray(queries, dtype=np.float32)
        point_values = np.asarray(points, dtype=np.float32)
        identifiers = np.asarray(indices, dtype=np.int64)
        squared = np.zeros(identifiers.shape, dtype=np.float32)
        scratch = np.empty_like(squared)
        for axis in range(3):
            np.take(point_values[:, axis], identifiers, out=scratch)
            np.subtract(query_values[:, axis, None], scratch, out=scratch)
            np.square(scratch, out=scratch)
            np.add(squared, scratch, out=squared)
        np.sqrt(squared, out=squared)
        return squared

    query_values = cp.ascontiguousarray(cp.asarray(queries, dtype=cp.float32))
    point_values = cp.ascontiguousarray(cp.asarray(points, dtype=cp.float32))
    identifiers = cp.ascontiguousarray(cp.asarray(indices, dtype=cp.int64))
    output = cp.empty(identifiers.shape, dtype=cp.float32)
    total = int(output.size)
    global _RBC_DISTANCE_RECOMPUTE_KERNEL
    if _RBC_DISTANCE_RECOMPUTE_KERNEL is None:  # pragma: no branch - GPU only
        _RBC_DISTANCE_RECOMPUTE_KERNEL = cp.RawKernel(
            r"""
            extern "C" __global__ void recompute_rbc_distances(
                const float* queries,
                const float* points,
                const long long* indices,
                float* output,
                unsigned long long total,
                int row_stride
            ) {
                unsigned long long linear =
                    (unsigned long long)blockDim.x * blockIdx.x + threadIdx.x;
                if (linear >= total) return;
                unsigned long long row = linear / (unsigned int)row_stride;
                long long point_id = indices[linear];
                float dx = queries[3ULL * row] - points[3LL * point_id];
                float dy = queries[3ULL * row + 1ULL] - points[3LL * point_id + 1LL];
                float dz = queries[3ULL * row + 2ULL] - points[3LL * point_id + 2LL];
                output[linear] = sqrtf(dx * dx + dy * dy + dz * dz);
            }
            """,
            "recompute_rbc_distances",
            options=("-std=c++11",),
        )
    threads = 256
    blocks = (total + threads - 1) // threads
    _RBC_DISTANCE_RECOMPUTE_KERNEL(
        (blocks,),
        (threads,),
        (
            query_values,
            point_values,
            identifiers,
            output,
            np.uint64(total),
            np.int32(identifiers.shape[1]),
        ),
    )
    return output


def _direct_bruteforce_sample_knn(
    cp: Any,
    points: Any,
    queries: Any,
    k: int,
    *,
    maximum_distance_matrix_bytes: int = 512 << 20,
) -> tuple[Any, Any]:
    """Independent direct-difference KNN for a small audit query sample.

    cuVS ``sqeuclidean`` is very fast, but on Blackwell its GEMM-style value
    path can suffer cancellation for near neighbours.  This audit authority
    evaluates ``(q_x-p_x)^2 + ...`` in binary64 and merges chunk-local top-Ks.
    It therefore scales to 30 M sites without allocating Q x N permanently.
    """

    requested = int(k)
    if isinstance(queries, np.ndarray):
        point_values = np.asarray(points, dtype=np.float32)
        query_values = np.asarray(queries, dtype=np.float32)
        differences = (
            query_values[:, None, :].astype(np.float64)
            - point_values[None, :, :].astype(np.float64)
        )
        squared = np.sum(differences * differences, axis=2)
        identifiers = np.broadcast_to(
            np.arange(point_values.shape[0], dtype=np.int64), squared.shape
        )
        order = np.lexsort((identifiers, squared), axis=1)[:, :requested]
        return (
            np.take_along_axis(squared, order, axis=1),
            np.take_along_axis(identifiers, order, axis=1),
        )

    point_values = cp.ascontiguousarray(cp.asarray(points, dtype=cp.float32))
    query_values = cp.ascontiguousarray(cp.asarray(queries, dtype=cp.float32))
    n_points = int(point_values.shape[0])
    n_queries = int(query_values.shape[0])
    per_point_bytes = max(1, n_queries) * np.dtype(np.float64).itemsize
    point_chunk = max(
        requested,
        min(n_points, int(maximum_distance_matrix_bytes) // per_point_bytes),
    )
    best_squared = None
    best_indices = None
    global _DIRECT_AUDIT_SQUARED_KERNEL
    if _DIRECT_AUDIT_SQUARED_KERNEL is None:  # pragma: no branch - GPU only
        _DIRECT_AUDIT_SQUARED_KERNEL = cp.RawKernel(
            r"""
            extern "C" __global__ void direct_audit_squared_distances(
                const float* queries,
                const float* points,
                double* output,
                unsigned long long total,
                unsigned long long point_offset,
                int chunk_points
            ) {
                unsigned long long linear =
                    (unsigned long long)blockDim.x * blockIdx.x + threadIdx.x;
                if (linear >= total) return;
                unsigned long long row = linear / (unsigned int)chunk_points;
                unsigned long long local =
                    linear - row * (unsigned int)chunk_points;
                unsigned long long point_id = point_offset + local;
                double dx = (double)queries[3ULL * row]
                    - (double)points[3ULL * point_id];
                double dy = (double)queries[3ULL * row + 1ULL]
                    - (double)points[3ULL * point_id + 1ULL];
                double dz = (double)queries[3ULL * row + 2ULL]
                    - (double)points[3ULL * point_id + 2ULL];
                output[linear] = dx * dx + dy * dy + dz * dz;
            }
            """,
            "direct_audit_squared_distances",
            options=("-std=c++11",),
        )

    for start in range(0, n_points, point_chunk):
        stop = min(start + point_chunk, n_points)
        width = stop - start
        squared = cp.empty((n_queries, width), dtype=cp.float64)
        total = int(squared.size)
        threads = 256
        blocks = (total + threads - 1) // threads
        _DIRECT_AUDIT_SQUARED_KERNEL(
            (blocks,),
            (threads,),
            (
                query_values,
                point_values,
                squared,
                np.uint64(total),
                np.uint64(start),
                np.int32(width),
            ),
        )
        take = min(requested, width)
        if take == width:
            local_indices = cp.broadcast_to(
                cp.arange(width, dtype=cp.int64), (n_queries, width)
            )
        else:
            local_indices = cp.argpartition(squared, take - 1, axis=1)[:, :take]
        local_squared = cp.take_along_axis(squared, local_indices, axis=1)
        local_indices = local_indices.astype(cp.int64, copy=False) + start
        del squared
        if best_squared is None:
            merged_squared = local_squared
            merged_indices = local_indices
        else:
            merged_squared = cp.concatenate((best_squared, local_squared), axis=1)
            merged_indices = cp.concatenate((best_indices, local_indices), axis=1)
        # The merged set contains at most 2K entries per audit query.  A full
        # distance sort here is tiny and avoids CuPy 13's non-NumPy lexsort API.
        order = cp.argsort(merged_squared, axis=1)[:, :requested]
        best_squared = cp.take_along_axis(merged_squared, order, axis=1)
        best_indices = cp.take_along_axis(merged_indices, order, axis=1)
        del merged_squared, merged_indices, local_squared, local_indices, order
    assert best_squared is not None and best_indices is not None
    return best_squared, best_indices


class RBCAuditedIndex:
    """3-D Euclidean KNN through RAPIDS Random Ball Cover.

    RBC prunes candidates with the triangle inequality and is restricted by
    cuML to Euclidean 2-D/3-D data.  The Colab workflow additionally audits a
    query sample against cuVS brute force before it permits a strict 30 M run.
    """

    # cuML's RBC implementation is intended to perform exact Euclidean KNN,
    # but this wrapper has no formal floating-point proof.  In particular, a
    # successful sample audit is evidence, not a certification of every query.
    exact = False
    numerically_proven = False
    euclidean_order_contract = True
    backend_name = "rapids_cuml_rbc_3d"

    def __init__(self, points: Any, *, max_k: int, audited: bool = False):
        cp, _, NearestNeighbors = require_cuda_stack()
        values = cp.ascontiguousarray(cp.asarray(points, dtype=cp.float32))
        if values.ndim != 2 or values.shape[1] != 3 or values.shape[0] < 1:
            raise ValueError("RBC points must have shape (N, 3), N >= 1")
        if not bool(cp.all(cp.isfinite(values)).item()):
            raise ValueError("RBC points contain NaN or Inf")
        self.points = values
        self.size = int(values.shape[0])
        self.max_k = _positive_integer(max_k, name="max_k")
        if self.max_k > self.size:
            raise ValueError(
                f"max_k={self.max_k} exceeds the number of indexed points "
                f"N={self.size}"
            )
        landmark_limit = math.isqrt(self.size)
        if self.max_k > landmark_limit:
            raise ValueError(
                f"cuML RBC requires max_k <= floor(sqrt(N))={landmark_limit} for "
                f"N={self.size}, but max_k={self.max_k} was requested. cuML "
                "may otherwise select a brute-force fallback; this wrapper "
                "refuses that hidden algorithm change. Reduce max_k, increase "
                "N, or select an explicit brute-force backend."
            )
        if self.max_k > RBC_MAX_QUERY_K:
            raise ValueError(
                f"RAPIDS RBC supports at most {RBC_MAX_QUERY_K} neighbors per "
                f"query, but max_k={self.max_k} was requested. Reserve one of "
                "those ranks for the global power guard (candidate_k_max <= "
                f"{RBC_MAX_QUERY_K - 1}) or select an explicit brute-force "
                "backend."
            )
        self.search_k = min(
            self.size,
            landmark_limit,
            RBC_MAX_QUERY_K,
            max(self.max_k, RBC_OVERFETCH_FACTOR * self.max_k),
        )
        if audited:
            raise ValueError(
                "audited=True cannot be declared at construction time; call "
                "audit_against_bruteforce so the empirical result is recorded"
            )
        self.model = NearestNeighbors(
            n_neighbors=self.search_k,
            algorithm="rbc",
            metric="euclidean",
            output_type="cupy",
        )
        self.model.fit(values)
        self.audited = False
        self.audit_status = "not_run"
        self.last_audit: NeighborAudit | None = None

    @property
    def status(self) -> dict[str, Any]:
        """Expose the actual evidence level without claiming a proof."""

        return {
            "backend": self.backend_name,
            "requested_algorithm": "rbc",
            "hidden_fallback_allowed": False,
            "empirical_audit_status": self.audit_status,
            "empirical_audit_passed": self.audited,
            "numerically_proven": self.numerically_proven,
            "internal_overfetch_factor": RBC_OVERFETCH_FACTOR,
            "internal_overfetch_max_k": self.search_k,
        }

    def query(self, queries: Any, k: int):
        cp, _, _ = require_cuda_stack()
        requested = _positive_integer(k, name="k")
        if not 1 <= requested <= self.max_k:
            raise ValueError(f"k must satisfy 1 <= k <= configured max_k={self.max_k}")
        values = cp.ascontiguousarray(cp.asarray(queries, dtype=cp.float32))
        _validate_queries(cp, values)
        search_requested = min(
            self.search_k, max(requested, RBC_OVERFETCH_FACTOR * requested)
        )
        _library_distances, indices = self.model.kneighbors(
            values, n_neighbors=search_requested, return_distance=True
        )
        # cuML 26.02/Blackwell was observed to return the exact RBC neighbour
        # identifiers while some associated distance values differed from the
        # stored-coordinate distance by up to 1.6%.  Those values feed both
        # the entropy scale and the global power guard, so accepting them would
        # invalidate the algorithm even though the support audit passes.
        # Treat RBC as a support oracle and recompute every returned distance
        # from the actual float32 query/site pair.  The fused CUDA kernel avoids
        # a Q x K x 3 temporary (several GiB at production scale).
        del _library_distances
        identifiers = cp.ascontiguousarray(cp.asarray(indices, dtype=cp.int64))
        expected = (int(values.shape[0]), search_requested)
        if identifiers.shape != expected:
            raise RuntimeError(
                f"RBC returned identifier shape {identifiers.shape}; expected {expected}"
            )
        if not bool(
            cp.all((identifiers >= 0) & (identifiers < self.size)).item()
        ):
            raise RuntimeError("RBC returned an out-of-range site identifier")
        distances = _recompute_returned_distances(
            cp, values, self.points, identifiers
        )

        # Normally RBC identifiers are already ordered.  Recomputed values can
        # expose a rare library-distance inversion, in which case restore the
        # public nondecreasing-distance contract while preserving associations.
        if search_requested > 1 and bool(
            cp.any(distances[:, 1:] < distances[:, :-1]).item()
        ):
            order = cp.argsort(distances, axis=1)
            distances = cp.take_along_axis(distances, order, axis=1)
            identifiers = cp.take_along_axis(identifiers, order, axis=1)
        return distances[:, :requested], identifiers[:, :requested]

    def audit_against_bruteforce(
        self,
        queries: Any,
        *,
        k: int = 10,
        require_support_equivalence: bool = True,
    ) -> NeighborAudit:
        """Empirically compare sampled RBC supports with direct brute force.

        Passing this check does not constitute a numerical proof for untested
        queries or for all floating-point inputs.  Rank values use a scale-aware
        float32 envelope.  Identifier differences are accepted only when the
        substituted sites have exactly identical stored coordinates; an
        arbitrary equal-distance substitution could change a Gibbs barycentre.
        Set ``require_support_equivalence=False`` only for consumers, such as
        the guarded power oracle, whose correctness depends on the Euclidean
        rank values but not on a canonical representative inside an exact tie.
        """

        cp, _, _ = require_cuda_stack()
        requested = _positive_integer(k, name="k")
        if requested > self.max_k:
            raise ValueError(f"k must satisfy 1 <= k <= configured max_k={self.max_k}")
        if not isinstance(require_support_equivalence, (bool, np.bool_)):
            raise TypeError("require_support_equivalence must be a bool")
        support_required = bool(require_support_equivalence)
        values = cp.ascontiguousarray(cp.asarray(queries, dtype=cp.float32))
        _validate_queries(cp, values)
        self.audited = False
        self.last_audit = None
        self.audit_status = "running"
        try:
            from cuvs.neighbors import brute_force  # type: ignore
        except Exception as error:  # pragma: no cover - Colab only
            self.audit_status = "unavailable"
            raise CUDABackendUnavailable(
                "the strict RBC audit requires cuVS from the matching RAPIDS release"
            ) from error
        try:
            start = cp.cuda.Event()
            stop = cp.cuda.Event()
            start.record()
            rbc_distances, rbc_indices = self.query(values, requested)
            stop.record()
            stop.synchronize()
            rbc_seconds = float(cp.cuda.get_elapsed_time(start, stop)) / 1_000.0

            start_bf = cp.cuda.Event()
            stop_bf = cp.cuda.Event()
            start_bf.record()
            exact_index = brute_force.build(self.points, metric="sqeuclidean")
            cuvs_squared, cuvs_indices = brute_force.search(
                exact_index, values, requested
            )
            stop_bf.record()
            stop_bf.synchronize()
            cuvs_seconds = float(cp.cuda.get_elapsed_time(start_bf, stop_bf)) / 1_000.0

            start_direct = cp.cuda.Event()
            stop_direct = cp.cuda.Event()
            start_direct.record()
            exact_squared, exact_indices = _direct_bruteforce_sample_knn(
                cp, self.points, values, requested
            )
            stop_direct.record()
            stop_direct.synchronize()
            brute_seconds = (
                float(cp.cuda.get_elapsed_time(start_direct, stop_direct)) / 1_000.0
            )
            exact_squared = cp.asarray(exact_squared, dtype=cp.float32)
            exact_indices = cp.asarray(exact_indices, dtype=cp.int64)
            cuvs_squared = cp.asarray(cuvs_squared, dtype=cp.float32)
            cuvs_indices = cp.asarray(cuvs_indices, dtype=cp.int64)

            # cuVS brute-force support is the independent reference, but its
            # reported sqeuclidean values can use a dot-product identity and
            # lose digits by cancellation for self/near-neighbour queries.
            # Recompute the geometry for both returned supports from stored
            # float32 coordinates in sampled CPU float64.  Separately compare
            # our public RBC distances with their own identifier pairs so a
            # distance/ID association bug cannot hide behind rank sorting.
            rbc_squared = cp.asarray(rbc_distances, dtype=cp.float32) ** 2
            rbc_returned_squared_cpu = cp.asnumpy(rbc_squared)
            cuvs_reported_squared_cpu = cp.asnumpy(cuvs_squared)
            rbc_indices_cpu = cp.asnumpy(rbc_indices).astype(np.int64, copy=False)
            exact_indices_cpu = cp.asnumpy(exact_indices).astype(np.int64, copy=False)
            cuvs_indices_cpu = cp.asnumpy(cuvs_indices).astype(np.int64, copy=False)
            rbc_points_cpu = cp.asnumpy(self.points[rbc_indices])
            exact_points_cpu = cp.asnumpy(self.points[exact_indices])
            cuvs_points_cpu = cp.asnumpy(self.points[cuvs_indices])
            queries_cpu = cp.asnumpy(values)
            queries64 = queries_cpu.astype(np.float64, copy=False)
            rbc_pair_squared = np.sum(
                (rbc_points_cpu.astype(np.float64) - queries64[:, None, :]) ** 2,
                axis=2,
            )
            exact_pair_squared = np.sum(
                (exact_points_cpu.astype(np.float64) - queries64[:, None, :]) ** 2,
                axis=2,
            )
            cuvs_pair_squared = np.sum(
                (cuvs_points_cpu.astype(np.float64) - queries64[:, None, :]) ** 2,
                axis=2,
            )

            rbc_order = np.argsort(rbc_pair_squared, axis=1, kind="stable")
            exact_order = np.argsort(exact_pair_squared, axis=1, kind="stable")
            rbc_sorted = np.take_along_axis(rbc_pair_squared, rbc_order, axis=1)
            exact_sorted = np.take_along_axis(exact_pair_squared, exact_order, axis=1)
            support_differences = np.abs(rbc_sorted - exact_sorted)
            support_tolerances = _scale_aware_squared_distance_tolerance(
                rbc_sorted, exact_sorted
            )
            association_differences = np.abs(
                rbc_returned_squared_cpu.astype(np.float64) - rbc_pair_squared
            )
            association_tolerances = _scale_aware_squared_distance_tolerance(
                rbc_returned_squared_cpu, rbc_pair_squared
            )
            distance_values_match = bool(
                np.all(support_differences <= support_tolerances)
                and np.all(association_differences <= association_tolerances)
            )
            maximum_error = float(
                max(
                    np.max(support_differences, initial=0.0),
                    np.max(association_differences, initial=0.0),
                )
            )
            tolerance = float(
                max(
                    np.max(support_tolerances, initial=0.0),
                    np.max(association_tolerances, initial=0.0),
                )
            )
            all_differences = np.concatenate(
                (support_differences.ravel(), association_differences.ravel())
            )
            all_scales = np.concatenate(
                (
                    np.maximum(np.abs(rbc_sorted), np.abs(exact_sorted)).ravel(),
                    np.maximum(
                        np.abs(rbc_returned_squared_cpu).astype(np.float64),
                        np.abs(rbc_pair_squared),
                    ).ravel(),
                )
            )
            relative_denominator = np.maximum(
                all_scales, float(np.nextafter(np.float32(0.0), np.float32(1.0)))
            )
            maximum_relative_error = float(
                np.max(all_differences / relative_denominator, initial=0.0)
            )
            cuvs_reported_error = float(
                np.max(
                    np.abs(
                        cuvs_reported_squared_cpu.astype(np.float64)
                        - cuvs_pair_squared
                    ),
                    initial=0.0,
                )
            )

            support = _tie_aware_support_summary(
                rbc_indices_cpu,
                exact_indices_cpu,
                rbc_points_cpu,
                exact_points_cpu,
            )
            cuvs_support = _tie_aware_support_summary(
                cuvs_indices_cpu,
                exact_indices_cpu,
                cuvs_points_cpu,
                exact_points_cpu,
            )
            values_match = bool(
                distance_values_match
                and (not support_required or support["support_equivalent"])
            )
            self.audited = values_match
            self.audit_status = "passed" if values_match else "failed"
            del (
                exact_index,
                exact_indices,
                cuvs_indices,
                rbc_indices,
                rbc_squared,
                exact_squared,
                cuvs_squared,
                rbc_points_cpu,
                exact_points_cpu,
                cuvs_points_cpu,
                queries_cpu,
            )
            cp.get_default_memory_pool().free_all_blocks()
        except Exception:
            self.audit_status = "error"
            raise
        result = NeighborAudit(
            queries=int(values.shape[0]),
            k=requested,
            values_match=bool(values_match),
            maximum_absolute_error=maximum_error,
            rbc_seconds=rbc_seconds,
            brute_force_seconds=brute_seconds,
            comparison_tolerance=float(tolerance),
            audit_scope=(
                "sampled_direct_float64_bruteforce_value_and_"
                "tie_aware_support_comparison_with_cuvs_diagnostic"
            ),
            distance_values_match=distance_values_match,
            identifiers_match=bool(support["identifiers_match"]),
            support_equivalent=bool(support["support_equivalent"]),
            support_equivalence_required=support_required,
            identifier_mismatch_rows=int(support["identifier_mismatch_rows"]),
            coordinate_equivalent_tie_rows=int(
                support["coordinate_equivalent_tie_rows"]
            ),
            non_equivalent_support_rows=int(
                support["non_equivalent_support_rows"]
            ),
            maximum_relative_error=maximum_relative_error,
            cuvs_reported_maximum_absolute_error=cuvs_reported_error,
            cuvs_brute_force_seconds=cuvs_seconds,
            cuvs_support_equivalent_to_direct=bool(
                cuvs_support["support_equivalent"]
            ),
            cuvs_non_equivalent_support_rows=int(
                cuvs_support["non_equivalent_support_rows"]
            ),
        )
        self.last_audit = result
        return result


class CuvsBruteForceIndex:
    """Explicit exhaustive 3-D Euclidean KNN for small CUDA workloads.

    This route is selected deliberately when cuML RBC's ``sqrt(N)`` neighbor
    limit cannot cover regularization support or a power-query guard.  It is
    slower but avoids both a hidden cuML fallback and the previous artificial
    lower bound on usable cloud sizes.
    """

    exact = True
    numerically_proven = False
    algorithmically_exhaustive = True
    euclidean_order_contract = True
    backend_name = "cuvs_brute_force_3d"

    def __init__(self, points: Any, *, max_k: int):
        cp, _, _ = require_cuda_stack()
        try:
            from cuvs.neighbors import brute_force  # type: ignore
        except Exception as error:  # pragma: no cover - depends on RAPIDS runtime
            raise CUDABackendUnavailable(
                "the explicit small-cloud CUDA route requires cuVS from the "
                "matching RAPIDS release"
            ) from error
        values = cp.ascontiguousarray(cp.asarray(points, dtype=cp.float32))
        if values.ndim != 2 or values.shape[1] != 3 or values.shape[0] < 1:
            raise ValueError("brute-force points must have shape (N, 3), N >= 1")
        if not bool(cp.all(cp.isfinite(values)).item()):
            raise ValueError("brute-force points contain NaN or Inf")
        self.points = values
        self.size = int(values.shape[0])
        self.max_k = _positive_integer(max_k, name="max_k")
        if self.max_k > self.size:
            raise ValueError(
                f"max_k={self.max_k} exceeds the number of indexed points "
                f"N={self.size}"
            )
        self._cp = cp
        self._brute_force = brute_force
        self.index = brute_force.build(values, metric="sqeuclidean")
        self.audited = False
        self.audit_status = "not_required_exhaustive"

    @property
    def status(self) -> dict[str, Any]:
        return {
            "backend": self.backend_name,
            "requested_algorithm": "brute_force",
            "hidden_fallback_allowed": False,
            "algorithmically_exhaustive": True,
            "empirical_audit_status": self.audit_status,
            "empirical_audit_passed": None,
            "numerically_proven": self.numerically_proven,
        }

    def query(self, queries: Any, k: int):
        requested = _positive_integer(k, name="k")
        if requested > self.max_k:
            raise ValueError(f"k must satisfy 1 <= k <= configured max_k={self.max_k}")
        values = self._cp.ascontiguousarray(
            self._cp.asarray(queries, dtype=self._cp.float32)
        )
        _validate_queries(self._cp, values)
        squared, indices = self._brute_force.search(self.index, values, requested)
        distances = self._cp.sqrt(self._cp.maximum(self._cp.asarray(squared), 0.0))
        return distances, self._cp.asarray(indices, dtype=self._cp.int64)


# Backward-compatible import for the first prototype.  The new name avoids
# suggesting that a sampled floating-point audit is a formal proof.
RBCExactIndex = RBCAuditedIndex


def _positive_integer(value: Any, *, name: str) -> int:
    if isinstance(value, (bool, np.bool_)):
        raise TypeError(f"{name} must be a positive integer, not bool")
    try:
        result = operator.index(value)
    except TypeError as error:
        raise TypeError(f"{name} must be a positive integer") from error
    if result < 1:
        raise ValueError(f"{name} must be positive")
    return int(result)


def _validate_queries(cp: Any, values: Any) -> None:
    if values.ndim != 2 or values.shape[1] != 3 or values.shape[0] < 1:
        raise ValueError("RBC queries must have shape (Q, 3), Q >= 1")
    if not bool(cp.all(cp.isfinite(values)).item()):
        raise ValueError("RBC queries contain NaN or Inf")


def configure_cupy_rmm_allocator() -> dict[str, Any]:
    """Route future CuPy allocations through the current RMM resource.

    Call this once, before creating any CuPy arrays.  The helper deliberately
    neither frees an existing CuPy pool nor calls ``rmm.reinitialize``: either
    action can invalidate or strand allocations in an active session.  A
    non-empty default CuPy pool therefore causes a hard failure with a restart
    instruction.
    """

    cp, _, _ = require_cuda_stack()
    try:
        import rmm  # type: ignore  # noqa: F401
        from rmm.allocators.cupy import rmm_cupy_allocator  # type: ignore
    except Exception as error:  # pragma: no cover - depends on RAPIDS runtime
        raise CUDABackendUnavailable(
            "configuring the shared allocator requires RMM from the matching "
            "RAPIDS release"
        ) from error

    pool = cp.get_default_memory_pool()
    used_bytes = int(pool.used_bytes())
    reserved_bytes = int(pool.total_bytes())
    if used_bytes or reserved_bytes:
        raise RuntimeError(
            "the default CuPy pool is already active "
            f"(used={used_bytes} bytes, reserved={reserved_bytes} bytes). "
            "Restart the runtime and call configure_cupy_rmm_allocator() "
            "before creating CuPy arrays; the helper will not reset a live "
            "allocator."
        )
    cp.cuda.set_allocator(rmm_cupy_allocator)
    return {
        "allocator": "rmm.rmm_cupy_allocator",
        "previous_cupy_pool_used_bytes": used_bytes,
        "previous_cupy_pool_reserved_bytes": reserved_bytes,
        "rmm_reinitialized": False,
    }


def free_device_bytes() -> tuple[int, int]:
    cp, _, _ = require_cuda_stack()
    free, total = cp.cuda.runtime.memGetInfo()
    return int(free), int(total)


def ensure_memory_budget(estimated_peak_bytes: int, *, fraction: float = 0.85) -> None:
    free, total = free_device_bytes()
    allowed = int(free * float(fraction))
    if estimated_peak_bytes > allowed:
        raise MemoryError(
            f"estimated peak {estimated_peak_bytes / 2**30:.2f} GiB exceeds "
            f"{fraction:.0%} of currently free VRAM ({free / 2**30:.2f} GiB); "
            "reduce chunk_size, candidate_k_max or grid_shape"
        )


def _command_output(command: list[str]) -> str | None:
    try:
        return subprocess.check_output(
            command, stderr=subprocess.STDOUT, text=True, timeout=30
        ).strip()
    except Exception:
        return None


def collect_hardware_manifest() -> dict[str, Any]:
    """Collect reproducibility metadata without assuming a specific G4 SKU."""

    manifest: dict[str, Any] = {
        "python": sys.version,
        "platform": platform.platform(),
        "cpu": platform.processor(),
        "nvidia_smi": _command_output(["nvidia-smi"]),
        "nvcc": _command_output(["nvcc", "--version"]),
        "git_head": _command_output(["git", "rev-parse", "HEAD"]),
    }
    try:
        cp, cuml, _ = require_cuda_stack()
        device_id = cp.cuda.Device().id
        properties = cp.cuda.runtime.getDeviceProperties(device_id)
        name = properties.get("name", b"unknown")
        if isinstance(name, bytes):
            name = name.decode("utf-8", errors="replace")
        free, total = cp.cuda.runtime.memGetInfo()
        manifest.update(
            {
                "cupy": cp.__version__,
                "cuml": getattr(cuml, "__version__", "unknown"),
                "cuda_runtime": int(cp.cuda.runtime.runtimeGetVersion()),
                "cuda_driver": int(cp.cuda.runtime.driverGetVersion()),
                "gpu_name": name,
                "gpu_compute_capability": [
                    int(properties.get("major", -1)),
                    int(properties.get("minor", -1)),
                ],
                "gpu_total_bytes": int(total),
                "gpu_free_bytes_at_manifest": int(free),
            }
        )
    except CUDABackendUnavailable as error:
        manifest["cuda_error"] = str(error)
    try:
        import psutil

        virtual = psutil.virtual_memory()
        manifest["host_ram_total_bytes"] = int(virtual.total)
        manifest["host_ram_available_bytes"] = int(virtual.available)
    except Exception:
        pass
    try:
        stat = os.statvfs(".")
        manifest["disk_free_bytes"] = int(stat.f_bavail * stat.f_frsize)
    except Exception:
        pass
    return manifest


def write_json_atomic(payload: Any, path: str) -> None:
    temporary = path + ".tmp"
    with open(temporary, "w", encoding="utf-8") as stream:
        json.dump(payload, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write("\n")
    os.replace(temporary, path)
