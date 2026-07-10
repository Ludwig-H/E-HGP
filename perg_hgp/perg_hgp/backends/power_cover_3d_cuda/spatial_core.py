"""CPU-testable core of the 3-D power-cover backend.

The CUDA implementation supplies a RAPIDS Random Ball Cover index and CuPy
arrays, but all mathematical decisions live here and are exercised on CPU.
No function in this module materialises an ``N x m_reg`` array globally.
"""

from __future__ import annotations

from dataclasses import dataclass
import time
from typing import Any, Protocol, Sequence

import numpy as np
from scipy.spatial import cKDTree

from .contracts import (
    GridSpec,
    PowerQueryDiagnostics,
    RegularizationDiagnostics,
    _shape3,
)
from .progress import ProgressCallback, emit_progress


class EuclideanKNNIndex(Protocol):
    """Minimal interface shared by SciPy cKDTree and RAPIDS RBC."""

    size: int
    euclidean_order_contract: bool

    def query(self, queries: Any, k: int) -> tuple[Any, Any]:
        """Return Euclidean distances then integer indices, both shaped Q x k."""


class ExactKDTreeIndex:
    """Exact low-dimensional CPU oracle used by the test suite."""

    exact = True
    euclidean_order_contract = True
    backend_name = "scipy_ckdtree_exact"

    def __init__(self, points: np.ndarray, *, leafsize: int = 32):
        values = np.asarray(points)
        if values.ndim != 2 or values.shape[1] != 3 or values.shape[0] < 1:
            raise ValueError("points must have shape (N, 3), N >= 1")
        if not np.isfinite(values).all():
            raise ValueError("points contain NaN or Inf")
        self.points = np.asarray(values, dtype=np.float64)
        self.size = int(values.shape[0])
        self._tree = cKDTree(self.points, leafsize=leafsize, compact_nodes=True)

    def query(self, queries: np.ndarray, k: int) -> tuple[np.ndarray, np.ndarray]:
        if not 1 <= int(k) <= self.size:
            raise ValueError(f"k must satisfy 1 <= k <= {self.size}")
        values = np.asarray(queries, dtype=np.float64)
        distances, indices = self._tree.query(values, k=int(k), workers=1)
        if int(k) == 1:
            distances = distances[:, None]
            indices = indices[:, None]
        return np.asarray(distances), np.asarray(indices, dtype=np.int64)


@dataclass
class GibbsResult:
    probabilities: Any
    temperatures: Any
    entropy: Any
    constraint_residual: Any


@dataclass
class RegularizedSites:
    centers: Any
    additive_weights: Any
    distortions: Any
    diagnostics: RegularizationDiagnostics


@dataclass
class PowerQueryResult:
    power_values: Any
    radii: Any
    radius_errors: Any
    certified: Any
    candidates_used: Any
    diagnostics: PowerQueryDiagnostics


@dataclass
class CubicalField:
    spec: GridSpec
    radii: Any
    numerical_radius_error: float
    inner_activation: Any
    outer_activation: Any
    certified: Any
    diagnostics: PowerQueryDiagnostics


class PowerKNNCertificationError(RuntimeError):
    """Raised when the candidate cap is reached before the global certificate."""

    def __init__(self, uncertain_count: int, total_count: int, candidate_cap: int):
        super().__init__(
            f"{uncertain_count}/{total_count} power queries remain uncertified "
            f"at candidate cap {candidate_cap}"
        )
        self.uncertain_count = int(uncertain_count)
        self.total_count = int(total_count)
        self.candidate_cap = int(candidate_cap)


def _array_module(array: Any):
    module = type(array).__module__.split(".", 1)[0]
    if module == "cupy":
        import cupy as cp  # type: ignore

        return cp
    return np


def _scalar(value: Any) -> float:
    return float(value.item() if hasattr(value, "item") else value)


def solve_entropy_gibbs(
    costs: Any,
    kappa: float,
    *,
    max_iter: int = 48,
    tolerance: float = 1e-6,
) -> GibbsResult:
    """Solve the canonical local entropy-constrained linear problem.

    The returned Gibbs law minimises ``<q, costs>`` on the supplied hard
    support under ``H(q) >= log(kappa)``.  Rows with tied minima are handled
    explicitly: uniform mass on all *exact* minimisers is optimal and may have
    entropy strictly larger than the requested lower bound.  Every other row
    is solved in log inverse-temperature coordinates after scaling by its
    smallest strictly positive cost gap.  Consequently, a representable gap
    such as ``1e-20`` is never mistaken for a tie merely because a fixed
    temperature floor cannot resolve it.
    """

    xp = _array_module(costs)
    values = xp.asarray(costs)
    if values.ndim != 2 or values.shape[1] < 1:
        raise ValueError("costs must have shape (N, m), m >= 1")
    if not _scalar(xp.all(xp.isfinite(values))):
        raise ValueError("costs contain NaN or Inf")
    n_rows, support = values.shape
    target = float(kappa)
    if not 1.0 <= target <= support:
        raise ValueError(f"kappa must satisfy 1 <= kappa <= {support}")
    if int(max_iter) < 1:
        raise ValueError("max_iter must be positive")
    if not np.isfinite(tolerance) or float(tolerance) <= 0.0:
        raise ValueError("tolerance must be finite and positive")

    dtype = values.dtype
    if not np.issubdtype(dtype, np.floating):
        raise TypeError("costs must have a real floating-point dtype")
    log_target = float(np.log(target))
    shifted = values - xp.min(values, axis=1, keepdims=True)
    minima = shifted == 0
    tie_count = xp.sum(minima, axis=1)

    if target == 1.0:
        probabilities = xp.zeros_like(values)
        first = xp.argmin(values, axis=1)
        probabilities[xp.arange(n_rows), first] = 1.0
        entropy = xp.zeros(n_rows, dtype=dtype)
        return GibbsResult(
            probabilities,
            xp.zeros(n_rows, dtype=dtype),
            entropy,
            xp.zeros(n_rows, dtype=dtype),
        )

    if target == float(support):
        probabilities = xp.full_like(values, 1.0 / support)
        entropy = xp.full(n_rows, np.log(support), dtype=dtype)
        return GibbsResult(
            probabilities,
            xp.full(n_rows, xp.inf, dtype=dtype),
            entropy,
            xp.zeros(n_rows, dtype=dtype),
        )

    finfo = xp.finfo(dtype)
    probabilities = xp.zeros_like(values)
    temperatures = xp.zeros(n_rows, dtype=dtype)
    entropy = xp.empty(n_rows, dtype=dtype)

    # This classification is combinatorial, not numerical: only entries equal
    # to the represented row minimum are ties.  Testing the entropy at an
    # arbitrary small temperature would incorrectly merge tiny positive gaps.
    tied_solution = tie_count.astype(dtype) >= target
    if _scalar(xp.any(tied_solution)):
        tie_probabilities = minima[tied_solution].astype(dtype)
        tie_probabilities /= xp.sum(tie_probabilities, axis=1, keepdims=True)
        probabilities[tied_solution] = tie_probabilities
        entropy[tied_solution] = xp.log(tie_count[tied_solution].astype(dtype))

    active = ~tied_solution
    if _scalar(xp.any(active)):
        active_shifted = shifted[active]
        positive = active_shifted > 0
        minimum_gap = xp.min(
            xp.where(positive, active_shifted, xp.asarray(xp.inf, dtype=dtype)),
            axis=1,
        )
        if not _scalar(xp.all(xp.isfinite(minimum_gap))):
            raise RuntimeError("an entropy-active row has no strictly positive cost gap")

        # log_ratio is log((c_j-c_min)/gap_min).  Forming the ratio directly
        # could overflow when a row contains both subnormal and very large
        # gaps, whereas its logarithm remains finite.
        safe_gap = xp.where(positive, active_shifted, xp.ones_like(active_shifted))
        log_ratio = xp.log(safe_gap) - xp.log(minimum_gap)[:, None]
        log_ratio = xp.where(positive, log_ratio, -xp.inf)
        max_log_ratio = xp.max(log_ratio, axis=1)

        # Once a dimensionless energy exceeds -log(tiny), its Gibbs weight is
        # below the normal range and can be capped without changing a result at
        # this dtype.  The cap also prevents exp(log_beta + log_ratio) overflow.
        maximum_energy = float(-np.log(float(finfo.tiny)))
        log_energy_cap = float(np.log(maximum_energy))

        def probabilities_and_entropy(log_beta: Any) -> tuple[Any, Any]:
            log_energy = log_beta[:, None] + log_ratio
            energy = xp.exp(xp.minimum(log_energy, log_energy_cap))
            logits = -energy
            logits -= xp.max(logits, axis=1, keepdims=True)
            weights = xp.exp(logits)
            normalizer = xp.sum(weights, axis=1, keepdims=True)
            probabilities_local = weights / normalizer
            log_probabilities = logits - xp.log(normalizer)
            entropy_local = -xp.sum(
                probabilities_local * log_probabilities,
                axis=1,
            )
            return probabilities_local, entropy_local

        # Entropy decreases monotonically with log(beta).  These scale-aware
        # initial bounds are already nearly uniform and nearly concentrated,
        # respectively; the short expansion loops validate rather than assume
        # that they bracket unusual finite-precision inputs.
        low = -max_log_ratio - log_energy_cap
        high = xp.full_like(low, log_energy_cap)
        _, entropy_low = probabilities_and_entropy(low)
        _, entropy_high = probabilities_and_entropy(high)
        bracket_slack = max(float(tolerance), 32.0 * float(finfo.eps))

        for _ in range(64):
            needs_hotter = entropy_low < log_target - bracket_slack
            if not _scalar(xp.any(needs_hotter)):
                break
            low = xp.where(needs_hotter, low - 4.0, low)
            _, entropy_low = probabilities_and_entropy(low)
        if _scalar(xp.any(entropy_low < log_target - bracket_slack)):
            raise RuntimeError("failed to bracket the Gibbs solution from above")

        for _ in range(64):
            needs_colder = entropy_high > log_target + bracket_slack
            if not _scalar(xp.any(needs_colder)):
                break
            high = xp.where(needs_colder, high + 4.0, high)
            _, entropy_high = probabilities_and_entropy(high)
        if _scalar(xp.any(entropy_high > log_target + bracket_slack)):
            raise RuntimeError("failed to bracket the Gibbs solution from below")

        for _ in range(int(max_iter)):
            middle = (low + high) * 0.5
            _, entropy_middle = probabilities_and_entropy(middle)
            feasible = entropy_middle >= log_target
            low = xp.where(feasible, middle, low)
            high = xp.where(feasible, high, middle)

        # The hot side of the final bracket is feasible by construction.  It
        # differs from the optimum only by the final log-temperature interval.
        active_probabilities, active_entropy = probabilities_and_entropy(low)
        probabilities[active] = active_probabilities
        entropy[active] = active_entropy

        log_temperature = xp.log(minimum_gap) - low
        active_temperatures = xp.exp(
            xp.minimum(log_temperature, float(np.log(float(finfo.max))))
        )
        active_temperatures = xp.where(
            log_temperature < float(np.log(float(finfo.tiny))),
            xp.asarray(0.0, dtype=dtype),
            active_temperatures,
        )
        active_temperatures = xp.where(
            log_temperature > float(np.log(float(finfo.max))),
            xp.asarray(xp.inf, dtype=dtype),
            active_temperatures,
        )
        temperatures[active] = active_temperatures

    constraint_residual = xp.maximum(log_target - entropy, 0.0)
    simplex_residual = xp.abs(xp.sum(probabilities, axis=1) - 1.0)
    validation_tolerance = max(float(tolerance), 32.0 * float(finfo.eps))
    if (
        not _scalar(xp.all(xp.isfinite(probabilities)))
        or not _scalar(xp.all(xp.isfinite(entropy)))
        or _scalar(xp.max(constraint_residual)) > validation_tolerance
        or _scalar(xp.max(simplex_residual)) > validation_tolerance
    ):
        raise RuntimeError(
            "Gibbs solve failed its entropy/simplex residual validation"
        )
    return GibbsResult(probabilities, temperatures, entropy, constraint_residual)


def regularize_sites_streaming(
    points: Any,
    index: EuclideanKNNIndex,
    *,
    m_reg: int,
    kappa: float,
    chunk_size: int,
    require_self_neighbor: bool = True,
    progress: ProgressCallback | None = None,
) -> RegularizedSites:
    """Compute ``(z_i, a_i, s_i)`` while releasing every neighbor block."""

    xp = _array_module(points)
    values = xp.asarray(points)
    if values.ndim != 2 or values.shape[1] != 3 or values.shape[0] < 1:
        raise ValueError("points must have shape (N, 3), N >= 1")
    if int(index.size) != int(values.shape[0]):
        raise ValueError("the KNN index must contain exactly the input points")
    if not _scalar(xp.all(xp.isfinite(values))):
        raise ValueError("points contain NaN or Inf")
    n_points = int(values.shape[0])
    support = min(int(m_reg), n_points)
    if not 1.0 <= float(kappa) <= support:
        raise ValueError("kappa must not exceed the effective regularization support")

    centers = xp.empty((n_points, 3), dtype=values.dtype)
    additive = xp.empty(n_points, dtype=values.dtype)
    distortions = xp.empty(n_points, dtype=values.dtype)
    max_entropy_violation = 0.0
    max_entropy_deviation = 0.0
    max_simplex_residual = 0.0
    progress_started = time.perf_counter()

    for start in range(0, n_points, int(chunk_size)):
        stop = min(start + int(chunk_size), n_points)
        queries = values[start:stop]
        distances, indices = index.query(queries, support)
        distances = xp.asarray(distances, dtype=values.dtype)
        indices = xp.asarray(indices)
        _validate_knn_response(
            xp,
            distances,
            indices,
            rows=stop - start,
            columns=support,
            index_size=n_points,
        )
        if require_self_neighbor:
            scale = max(1.0, _scalar(xp.max(xp.abs(queries))))
            threshold = 32.0 * float(xp.finfo(values.dtype).eps) * scale
            if _scalar(xp.max(distances[:, 0])) > threshold:
                raise RuntimeError(
                    "the KNN support does not include each observation itself; "
                    "this violates the thesis K-NN convention"
                )

        gibbs = solve_entropy_gibbs(distances * distances, kappa)
        probabilities = gibbs.probabilities
        neighbors = values[indices]
        centers_chunk = xp.sum(probabilities[:, :, None] * neighbors, axis=1)
        offsets = neighbors - centers_chunk[:, None, :]
        additive_chunk = xp.sum(probabilities * xp.sum(offsets * offsets, axis=2), axis=1)
        additive_chunk = xp.maximum(additive_chunk, 0.0)
        displacement = queries - centers_chunk
        distortion_chunk = xp.sqrt(
            xp.maximum(xp.sum(displacement * displacement, axis=1) + additive_chunk, 0.0)
        )

        centers[start:stop] = centers_chunk
        additive[start:stop] = additive_chunk
        distortions[start:stop] = distortion_chunk
        max_entropy_violation = max(
            max_entropy_violation, _scalar(xp.max(gibbs.constraint_residual))
        )
        max_entropy_deviation = max(
            max_entropy_deviation,
            _scalar(xp.max(xp.abs(gibbs.entropy - np.log(float(kappa))))),
        )
        max_simplex_residual = max(
            max_simplex_residual,
            _scalar(xp.max(xp.abs(xp.sum(probabilities, axis=1) - 1.0))),
        )

        del (
            distances,
            indices,
            gibbs,
            probabilities,
            neighbors,
            centers_chunk,
            offsets,
            additive_chunk,
            displacement,
            distortion_chunk,
            queries,
        )
        if progress is not None:
            emit_progress(
                progress,
                stage="regularization",
                kind="progress",
                completed=stop,
                total=n_points,
                unit="points",
                started_at=progress_started,
                details={
                    "chunk": int(start // int(chunk_size) + 1),
                    "chunks": int(
                        (n_points + int(chunk_size) - 1) // int(chunk_size)
                    ),
                },
            )

    quantile_levels = (0.0, 0.5, 0.9, 0.99, 1.0)
    quantile_sample_size = min(n_points, 1_000_000)
    if quantile_sample_size == n_points:
        quantile_sample = distortions
    else:
        quantile_ids = xp.linspace(
            0, n_points - 1, quantile_sample_size, dtype=xp.int64
        )
        quantile_sample = distortions[quantile_ids]
    quantile_values = xp.quantile(quantile_sample, xp.asarray(quantile_levels))
    diagnostics = RegularizationDiagnostics(
        entropy_residual_max=max_entropy_violation,
        entropy_target_deviation_max=max_entropy_deviation,
        simplex_residual_max=max_simplex_residual,
        s_max=_scalar(xp.max(distortions)),
        s_quantiles={
            str(level): _scalar(value)
            for level, value in zip(quantile_levels, quantile_values)
        },
        query_count=n_points,
        s_quantile_sample_size=quantile_sample_size,
    )
    return RegularizedSites(centers, additive, distortions, diagnostics)


_POWER_VALUES_KERNEL: Any | None = None


def _candidate_power_values(
    queries: Any,
    indices: Any,
    candidate_count: int,
    sites: Any,
    additive: Any,
):
    """Evaluate a Q x C power matrix without materialising Q x C x 3 arrays."""

    xp = _array_module(sites)
    count = int(candidate_count)
    if xp is np:
        query_values = np.asarray(queries)
        index_values = np.asarray(indices)
        output = np.zeros((query_values.shape[0], count), dtype=sites.dtype)
        scratch = np.empty_like(output)
        selected = index_values[:, :count]
        for axis in range(3):
            np.take(sites[:, axis], selected, out=scratch)
            np.subtract(query_values[:, axis, None], scratch, out=scratch)
            np.square(scratch, out=scratch)
            np.add(output, scratch, out=output)
        # NumPy 2.0 requires the source and ``out`` dtypes of ``take`` to
        # match, even when the conversion is otherwise safe.  Reuse the
        # geometric scratch when possible; otherwise replace it with a local
        # Q x C weight scratch instead of converting the global N-vector.
        if additive.dtype == scratch.dtype:
            weight_scratch = scratch
        else:
            del scratch
            weight_scratch = np.empty(output.shape, dtype=additive.dtype)
        np.take(additive, selected, out=weight_scratch)
        np.add(output, weight_scratch, out=output, casting="same_kind")
        return output

    # The massive CUDA path stores normalized sites in float32 and RBC emits
    # int64 identifiers.  One fused kernel avoids candidates, differences,
    # squared differences and gathered-weight matrices, which otherwise add
    # several GiB at the 1,024-candidate ceiling.
    import cupy as cp  # type: ignore

    if sites.dtype != cp.float32 or additive.dtype != cp.float32:
        raise TypeError("CUDA power evaluation requires float32 sites and weights")
    index_values = cp.ascontiguousarray(indices, dtype=cp.int64)
    query_values = cp.ascontiguousarray(queries, dtype=cp.float32)
    output = cp.empty((query_values.shape[0], count), dtype=cp.float32)
    total = int(output.size)
    global _POWER_VALUES_KERNEL
    if _POWER_VALUES_KERNEL is None:  # pragma: no branch - compiled once on GPU
        _POWER_VALUES_KERNEL = cp.RawKernel(
            r'''
            extern "C" __global__ void candidate_power_values(
                const float* queries,
                const float* sites,
                const float* additive,
                const long long* indices,
                float* output,
                unsigned long long total,
                int candidate_count,
                int index_stride
            ) {
                unsigned long long linear =
                    (unsigned long long)blockDim.x * blockIdx.x + threadIdx.x;
                if (linear >= total) return;
                unsigned long long row = linear / (unsigned int)candidate_count;
                int column = (int)(linear - row * (unsigned int)candidate_count);
                long long site_id = indices[row * (unsigned int)index_stride + column];
                float dx = queries[3ULL * row] - sites[3LL * site_id];
                float dy = queries[3ULL * row + 1ULL] - sites[3LL * site_id + 1LL];
                float dz = queries[3ULL * row + 2ULL] - sites[3LL * site_id + 2LL];
                output[linear] = dx * dx + dy * dy + dz * dz + additive[site_id];
            }
            ''',
            "candidate_power_values",
            options=("-std=c++11",),
        )
    threads = 256
    blocks = (total + threads - 1) // threads
    _POWER_VALUES_KERNEL(
        (blocks,),
        (threads,),
        (
            query_values,
            sites,
            additive,
            index_values,
            output,
            np.uint64(total),
            np.int32(count),
            np.int32(index_values.shape[1]),
        ),
    )
    return output


def _validate_knn_response(
    xp: Any,
    distances: Any,
    indices: Any,
    *,
    rows: int,
    columns: int,
    index_size: int,
) -> None:
    expected = (int(rows), int(columns))
    if distances.shape != expected or indices.shape != expected:
        raise RuntimeError(
            f"KNN backend returned shapes {distances.shape}/{indices.shape}; "
            f"expected {expected}"
        )
    if indices.dtype.kind not in "iu":
        raise RuntimeError("KNN backend returned non-integer identifiers")
    if not _scalar(xp.all(xp.isfinite(distances))):
        raise RuntimeError("KNN backend returned a non-finite distance")
    if _scalar(xp.min(distances)) < 0.0:
        raise RuntimeError("KNN backend returned a negative distance")
    if columns > 1 and _scalar(xp.any(distances[:, 1:] < distances[:, :-1])):
        raise RuntimeError("KNN backend distances are not nondecreasing")
    if _scalar(xp.min(indices)) < 0 or _scalar(xp.max(indices)) >= index_size:
        raise RuntimeError("KNN backend returned an out-of-range site identifier")


class CertifiedPowerKNN:
    """Top-K power query from ordered Euclidean candidates plus a global guard.

    If the first ``C`` Euclidean sites are candidates and the ``(C+1)``-st
    Euclidean distance is ``d_guard``, every omitted site has power at least
    ``d_guard**2 + min(a)``.  Comparing that lower bound with the candidate
    K-th power value certifies the global result.  Unresolved queries are
    reissued with a larger candidate set; there is no silent ANN fallback.
    """

    def __init__(
        self,
        sites: Any,
        additive_weights: Any,
        index: EuclideanKNNIndex,
        *,
        K: int,
        candidate_k_initial: int = 64,
        candidate_k_max: int = 1_024,
        numerical_margin_factor: float = 64.0,
        strict: bool = True,
    ):
        xp = _array_module(sites)
        centers = xp.asarray(sites)
        additive = xp.asarray(additive_weights)
        if centers.ndim != 2 or centers.shape[1] != 3:
            raise ValueError("sites must have shape (N, 3)")
        if centers.dtype.kind != "f" or additive.dtype.kind != "f":
            raise TypeError("sites and additive_weights must be floating point")
        if additive.shape != (centers.shape[0],):
            raise ValueError("additive_weights must have shape (N,)")
        if not _scalar(xp.all(xp.isfinite(centers))) or not _scalar(
            xp.all(xp.isfinite(additive))
        ):
            raise ValueError("sites or weights contain NaN or Inf")
        tolerance = 64.0 * float(xp.finfo(additive.dtype).eps)
        if _scalar(xp.min(additive)) < -tolerance:
            raise ValueError("additive power weights must be non-negative")
        self.sites = centers
        self.additive = xp.maximum(additive, 0.0)
        self.index = index
        self.size = int(centers.shape[0])
        if int(index.size) != self.size:
            raise ValueError("the Euclidean index and sites have different sizes")
        if not bool(getattr(index, "euclidean_order_contract", False)):
            raise TypeError(
                "the KNN backend must explicitly declare an ordered Euclidean "
                "candidate contract"
            )
        if isinstance(K, (bool, np.bool_)) or not isinstance(K, (int, np.integer)):
            raise TypeError("K must be an integer")
        self.K = int(K)
        if not 1 <= self.K <= self.size:
            raise ValueError("K must satisfy 1 <= K <= number of sites")
        for name, candidate_value in (
            ("candidate_k_initial", candidate_k_initial),
            ("candidate_k_max", candidate_k_max),
        ):
            if isinstance(candidate_value, (bool, np.bool_)) or not isinstance(
                candidate_value, (int, np.integer)
            ):
                raise TypeError(f"{name} must be an integer")
        self.candidate_k_initial = int(candidate_k_initial)
        self.candidate_k_max = int(candidate_k_max)
        if self.candidate_k_initial < self.K:
            raise ValueError("candidate_k_initial must be at least K")
        if self.candidate_k_max < self.candidate_k_initial:
            raise ValueError("candidate_k_max must be >= candidate_k_initial")
        self.margin_factor = float(numerical_margin_factor)
        if not np.isfinite(self.margin_factor) or self.margin_factor < 1.0:
            raise ValueError("numerical_margin_factor must be finite and >= 1")
        if not isinstance(strict, (bool, np.bool_)):
            raise TypeError("strict must be a bool")
        self.strict = bool(strict)
        self.min_additive = _scalar(xp.min(self.additive))
        required_capacity = min(self.size, self.candidate_k_max + 1)
        if hasattr(index, "max_k") and int(index.max_k) < required_capacity:
            raise ValueError(
                f"KNN index capacity {int(index.max_k)} is below the required "
                f"candidate/guard capacity {required_capacity}"
            )

    def query(self, queries: Any) -> PowerQueryResult:
        xp = _array_module(self.sites)
        values = xp.asarray(queries, dtype=self.sites.dtype)
        if values.ndim != 2 or values.shape[1] != 3 or values.shape[0] < 1:
            raise ValueError("queries must have shape (Q, 3), Q >= 1")
        if not _scalar(xp.all(xp.isfinite(values))):
            raise ValueError("queries contain NaN or Inf")
        n_queries = int(values.shape[0])
        powers_out = xp.full(n_queries, xp.nan, dtype=self.sites.dtype)
        certified = xp.zeros(n_queries, dtype=xp.bool_)
        used = xp.zeros(n_queries, dtype=xp.int32)
        unresolved = xp.arange(n_queries, dtype=xp.int64)
        histogram: dict[str, int] = {}
        minimum_gap: float | None = None
        candidate_count = min(self.size, self.candidate_k_initial)

        while int(unresolved.size) > 0:
            active_queries = values[unresolved]
            if candidate_count >= self.size:
                distances, indices = self.index.query(active_queries, self.size)
                distances = xp.asarray(distances, dtype=self.sites.dtype)
                indices = xp.asarray(indices)
                _validate_knn_response(
                    xp,
                    distances,
                    indices,
                    rows=int(unresolved.size),
                    columns=self.size,
                    index_size=self.size,
                )
                del distances
                candidate_powers = _candidate_power_values(
                    active_queries, indices, self.size, self.sites, self.additive
                )
                del indices
                candidate_powers.partition(self.K - 1, axis=1)
                kth = candidate_powers[:, self.K - 1].copy()
                del candidate_powers
                accepted = xp.ones(int(unresolved.size), dtype=xp.bool_)
                gaps = xp.full(int(unresolved.size), xp.inf, dtype=self.sites.dtype)
            else:
                request = min(self.size, candidate_count + 1)
                distances, indices = self.index.query(active_queries, request)
                distances = xp.asarray(distances, dtype=self.sites.dtype)
                indices = xp.asarray(indices)
                _validate_knn_response(
                    xp,
                    distances,
                    indices,
                    rows=int(unresolved.size),
                    columns=request,
                    index_size=self.size,
                )

                # The explicit envelope covers float32 arithmetic and possible
                # distance-order drift in the library query.  Near-boundary
                # cases remain uncertain and are escalated instead of guessed.
                outside_lower = distances[:, candidate_count] ** 2 + self.min_additive
                del distances
                candidate_powers = _candidate_power_values(
                    active_queries,
                    indices,
                    candidate_count,
                    self.sites,
                    self.additive,
                )
                del indices
                candidate_powers.partition(self.K - 1, axis=1)
                kth = candidate_powers[:, self.K - 1].copy()
                del candidate_powers
                scale = xp.maximum(xp.maximum(xp.abs(kth), xp.abs(outside_lower)), 1.0)
                envelope = self.margin_factor * xp.finfo(self.sites.dtype).eps * scale
                gaps = outside_lower - kth
                accepted = kth + envelope <= outside_lower - envelope

            accepted_indices = unresolved[accepted]
            powers_out[accepted_indices] = kth[accepted]
            certified[accepted_indices] = True
            used[accepted_indices] = candidate_count
            accepted_count = int(_scalar(xp.sum(accepted)))
            histogram[str(candidate_count)] = histogram.get(str(candidate_count), 0) + accepted_count
            if accepted_count:
                accepted_gap = _scalar(xp.min(gaps[accepted]))
                if np.isfinite(accepted_gap):
                    minimum_gap = (
                        accepted_gap
                        if minimum_gap is None
                        else min(minimum_gap, accepted_gap)
                    )

            unresolved = unresolved[~accepted]
            if int(unresolved.size) == 0 or candidate_count >= self.size:
                break
            next_count = min(self.size, self.candidate_k_max, candidate_count * 2)
            if next_count == candidate_count:
                # Preserve the last candidate estimate for diagnostics, but do
                # not turn it into a certified result.
                rejected_indices = unresolved
                rejected_kth = kth[~accepted]
                powers_out[rejected_indices] = rejected_kth
                used[rejected_indices] = candidate_count
                histogram[f"{candidate_count}:uncertain"] = int(unresolved.size)
                break
            del kth, gaps, accepted, active_queries
            candidate_count = next_count

        uncertain_count = int(_scalar(xp.sum(~certified)))
        power_envelope = (
            self.margin_factor
            * xp.finfo(self.sites.dtype).eps
            * xp.maximum(xp.abs(powers_out), 1.0)
        )
        radius_center = xp.sqrt(xp.maximum(powers_out, 0.0))
        radius_lower = xp.sqrt(xp.maximum(powers_out - power_envelope, 0.0))
        radius_upper = xp.sqrt(xp.maximum(powers_out + power_envelope, 0.0))
        radius_errors = xp.maximum(
            radius_center - radius_lower, radius_upper - radius_center
        )
        radius_error_max = _scalar(xp.max(radius_errors)) if n_queries else 0.0
        diagnostics = PowerQueryDiagnostics(
            total_queries=n_queries,
            certified_queries=n_queries - uncertain_count,
            uncertain_queries=uncertain_count,
            candidate_histogram=histogram,
            min_certificate_gap=minimum_gap,
            radius_error_max=radius_error_max,
            numerical_certificate="declared_float32_gap_envelope_not_directed",
        )
        if uncertain_count and self.strict:
            raise PowerKNNCertificationError(
                uncertain_count, n_queries, min(self.size, self.candidate_k_max)
            )
        return PowerQueryResult(
            powers_out,
            radius_center,
            radius_errors,
            certified,
            used,
            diagnostics,
        )


def build_grid_spec(sites: Any, requested_shape: int | Sequence[int]) -> GridSpec:
    """Build the site-bounding box grid, collapsing constant axes to one cell."""

    xp = _array_module(sites)
    values = xp.asarray(sites)
    if values.ndim != 2 or values.shape[1] != 3 or values.shape[0] < 1:
        raise ValueError("sites must have shape (N, 3), N >= 1")
    bbox_min_array = xp.min(values, axis=0)
    bbox_max_array = xp.max(values, axis=0)
    bbox_min = tuple(_scalar(item) for item in bbox_min_array)
    bbox_max = tuple(_scalar(item) for item in bbox_max_array)
    requested = _shape3(requested_shape)
    span = tuple(high - low for low, high in zip(bbox_min, bbox_max))
    global_scale = max(1.0, max(abs(item) for item in (*bbox_min, *bbox_max)))
    tolerance = 64.0 * np.finfo(values.dtype).eps * global_scale
    shape = tuple(1 if width <= tolerance else cells for width, cells in zip(span, requested))
    cell_widths = tuple(width / cells if cells > 1 or width > 0 else 0.0 for width, cells in zip(span, shape))
    half_diagonal = 0.5 * float(np.sqrt(sum(width * width for width in cell_widths)))
    return GridSpec(bbox_min, bbox_max, shape, cell_widths, half_diagonal)


def grid_centers(spec: GridSpec, flat_indices: Any, xp=np, dtype=np.float32):
    """Generate grid centres for a flat-index block without storing the grid."""

    indices = xp.asarray(flat_indices, dtype=xp.int64)
    nx, ny, nz = spec.shape
    yz = ny * nz
    ix = indices // yz
    remainder = indices - ix * yz
    iy = remainder // nz
    iz = remainder - iy * nz
    coordinates = xp.stack((ix, iy, iz), axis=1).astype(dtype)
    widths = xp.asarray(spec.cell_widths, dtype=dtype)
    lower = xp.asarray(spec.bbox_min, dtype=dtype)
    return lower[None, :] + (coordinates + 0.5) * widths[None, :]


def evaluate_cubical_field(
    oracle: CertifiedPowerKNN,
    spec: GridSpec,
    *,
    chunk_size: int,
    progress: ProgressCallback | None = None,
) -> CubicalField:
    """Evaluate the radius field and its inner/outer cube activations."""

    xp = _array_module(oracle.sites)
    n_cubes = spec.n_cubes
    radii = xp.empty(n_cubes, dtype=oracle.sites.dtype)
    certified = xp.empty(n_cubes, dtype=xp.bool_)
    total_certified = 0
    total_uncertain = 0
    histogram: dict[str, int] = {}
    minimum_gap: float | None = None
    numerical_radius_error = 0.0
    progress_started = time.perf_counter()

    for start in range(0, n_cubes, int(chunk_size)):
        stop = min(start + int(chunk_size), n_cubes)
        flat = xp.arange(start, stop, dtype=xp.int64)
        centers = grid_centers(spec, flat, xp=xp, dtype=oracle.sites.dtype)
        result = oracle.query(centers)
        radii[start:stop] = result.radii
        certified[start:stop] = result.certified
        total_certified += result.diagnostics.certified_queries
        total_uncertain += result.diagnostics.uncertain_queries
        for key, count in result.diagnostics.candidate_histogram.items():
            histogram[key] = histogram.get(key, 0) + count
        gap = result.diagnostics.min_certificate_gap
        if gap is not None:
            minimum_gap = gap if minimum_gap is None else min(minimum_gap, gap)
        numerical_radius_error = max(
            numerical_radius_error, result.diagnostics.radius_error_max
        )
        if progress is not None:
            emit_progress(
                progress,
                stage="power_field",
                kind="progress",
                completed=stop,
                total=n_cubes,
                unit="cubes",
                started_at=progress_started,
                details={
                    "chunk": int(start // int(chunk_size) + 1),
                    "chunks": int(
                        (n_cubes + int(chunk_size) - 1) // int(chunk_size)
                    ),
                    "certified": int(total_certified),
                    "uncertain": int(total_uncertain),
                },
            )

    # A global envelope preserves the common MSF topology of the two uniform
    # filtrations while accounting for centre-value roundoff explicitly.
    h = spec.half_diagonal + numerical_radius_error
    inner = radii + h
    outer = radii - h
    diagnostics = PowerQueryDiagnostics(
        total_queries=n_cubes,
        certified_queries=total_certified,
        uncertain_queries=total_uncertain,
        candidate_histogram=histogram,
        min_certificate_gap=minimum_gap,
        radius_error_max=numerical_radius_error,
        numerical_certificate="declared_float32_gap_envelope_not_directed",
    )
    return CubicalField(
        spec, radii, numerical_radius_error, inner, outer, certified, diagnostics
    )


def brute_force_power_topk(
    queries: np.ndarray,
    sites: np.ndarray,
    additive_weights: np.ndarray,
    K: int,
    *,
    query_block: int = 1_024,
    site_block: int = 4_096,
) -> tuple[np.ndarray, np.ndarray]:
    """Blocked deterministic CPU oracle for small validation problems.

    Ties are ordered by site identifier.  Only ``Q x site_block`` distances are
    materialised, never ``Q x N`` for the entire query set.
    """

    q = np.asarray(queries, dtype=np.float64)
    z = np.asarray(sites, dtype=np.float64)
    a = np.asarray(additive_weights, dtype=np.float64)
    if q.ndim != 2 or q.shape[1] != 3 or z.ndim != 2 or z.shape[1] != 3:
        raise ValueError("queries and sites must have three columns")
    if a.shape != (z.shape[0],) or not 1 <= K <= z.shape[0]:
        raise ValueError("invalid weights or K")
    values_out = np.empty((q.shape[0], K), dtype=np.float64)
    indices_out = np.empty((q.shape[0], K), dtype=np.int64)

    for q_start in range(0, q.shape[0], query_block):
        q_stop = min(q_start + query_block, q.shape[0])
        qb = q[q_start:q_stop]
        best_values = np.full((qb.shape[0], K), np.inf)
        best_indices = np.full((qb.shape[0], K), z.shape[0], dtype=np.int64)
        for z_start in range(0, z.shape[0], site_block):
            z_stop = min(z_start + site_block, z.shape[0])
            diff = qb[:, None, :] - z[None, z_start:z_stop, :]
            powers = np.sum(diff * diff, axis=2) + a[None, z_start:z_stop]
            ids = np.broadcast_to(
                np.arange(z_start, z_stop, dtype=np.int64)[None, :], powers.shape
            )
            merged_values = np.concatenate((best_values, powers), axis=1)
            merged_indices = np.concatenate((best_indices, ids), axis=1)
            for row in range(qb.shape[0]):
                order = np.lexsort((merged_indices[row], merged_values[row]))[:K]
                best_values[row] = merged_values[row, order]
                best_indices[row] = merged_indices[row, order]
        values_out[q_start:q_stop] = best_values
        indices_out[q_start:q_stop] = best_indices
    return values_out, indices_out
