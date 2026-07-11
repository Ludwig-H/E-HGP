"""CPU-testable core of the 3-D power-cover backend.

The CUDA implementation supplies a RAPIDS Random Ball Cover index and CuPy
arrays, but all mathematical decisions live here and are exercised on CPU.
No function in this module materialises an ``N x m_reg`` array globally.
"""

from __future__ import annotations

from dataclasses import dataclass
import math
import time
from typing import Any, Protocol, Sequence, cast

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
    constraint_active: Any
    effective_kappa: Any


@dataclass
class RegularizedSites:
    centers: Any
    additive_weights: Any
    distortions: Any
    diagnostics: RegularizationDiagnostics
    effective_kappa: Any | None = None
    local_scales: Any | None = None


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


class GridResolutionBudgetError(RuntimeError):
    """Raised before field allocation when a local scale exceeds the cell budget."""

    def __init__(self, requested_cells: int, max_cells: int, shape: Sequence[int]):
        super().__init__(
            "the requested local grid resolution needs "
            f"{int(requested_cells):,} cells with shape {tuple(int(v) for v in shape)}, "
            f"above max_grid_cells={int(max_cells):,}; tile the domain, relax the "
            "resolved radius, or use an adaptive grid"
        )
        self.requested_cells = int(requested_cells)
        self.max_cells = int(max_cells)
        self.shape = tuple(int(v) for v in shape)


class GridCoordinateResolutionError(RuntimeError):
    """Raised when float32 grid centres cannot represent adjacent cells."""


def _validate_float32_grid_coordinates(
    bbox_min: Sequence[float],
    bbox_max: Sequence[float],
    shape: Sequence[int],
    cell_widths: Sequence[float],
) -> None:
    eps = np.finfo(np.float32).eps
    for axis, (low, high, cells, width) in enumerate(
        zip(bbox_min, bbox_max, shape, cell_widths)
    ):
        if int(cells) > 2**24:
            raise GridCoordinateResolutionError(
                f"grid axis {axis} has {int(cells):,} cells; float32 cannot "
                "represent every consecutive centre index"
            )
        coordinate_scale = max(1.0, abs(float(low)), abs(float(high)))
        if int(cells) > 1 and float(width) <= 8.0 * eps * coordinate_scale:
            raise GridCoordinateResolutionError(
                f"grid axis {axis} cell width {float(width):.6g} is below the "
                "declared float32 centre-resolution envelope"
            )


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
            xp.ones(n_rows, dtype=xp.bool_),
            xp.ones(n_rows, dtype=dtype),
        )

    if target == float(support):
        probabilities = xp.full_like(values, 1.0 / support)
        entropy = xp.full(n_rows, np.log(support), dtype=dtype)
        return GibbsResult(
            probabilities,
            xp.full(n_rows, xp.inf, dtype=dtype),
            entropy,
            xp.zeros(n_rows, dtype=dtype),
            xp.ones(n_rows, dtype=xp.bool_),
            xp.full(n_rows, float(support), dtype=dtype),
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
            raise RuntimeError(
                "an entropy-active row has no strictly positive cost gap"
            )

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
        raise RuntimeError("Gibbs solve failed its entropy/simplex residual validation")
    return GibbsResult(
        probabilities,
        temperatures,
        entropy,
        constraint_residual,
        active,
        xp.exp(entropy),
    )


def solve_entropy_gibbs_budget(
    costs: Any,
    budgets_squared: Any,
    *,
    max_iter: int = 32,
    tolerance: float = 1e-6,
) -> GibbsResult:
    """Maximise entropy under a per-row quadratic-distortion budget.

    For every row this solves ``max H(q)`` on the simplex subject to
    ``<q, costs> <= budgets_squared``.  The solution is uniform when the
    budget admits it, uniform on exact minimisers at the minimum feasible
    cost, and otherwise a Gibbs law.  This inverse formulation lets callers
    impose ``s_i <= min(B_abs, gamma * R_K(x_i))`` in one streaming pass
    instead of rerunning a global sequence of ``kappa`` values.
    """

    xp = _array_module(costs)
    values = xp.asarray(costs)
    if values.ndim != 2 or values.shape[1] < 1:
        raise ValueError("costs must have shape (N, m), m >= 1")
    if values.dtype.kind != "f":
        raise TypeError("costs must have a real floating-point dtype")
    if not _scalar(xp.all(xp.isfinite(values))):
        raise ValueError("costs contain NaN or Inf")
    if _scalar(xp.min(values)) < 0.0:
        raise ValueError("costs must be non-negative")
    if int(max_iter) < 1:
        raise ValueError("max_iter must be positive")
    if not np.isfinite(tolerance) or float(tolerance) <= 0.0:
        raise ValueError("tolerance must be finite and positive")

    n_rows, support = values.shape
    dtype = values.dtype
    raw_budgets = xp.asarray(budgets_squared, dtype=dtype)
    if raw_budgets.ndim == 0:
        budgets = xp.full(n_rows, raw_budgets, dtype=dtype)
    elif raw_budgets.shape == (n_rows,):
        budgets = raw_budgets
    else:
        raise ValueError("budgets_squared must be scalar or have shape (N,)")
    if not _scalar(xp.all(xp.isfinite(budgets))) or _scalar(xp.min(budgets)) < 0.0:
        raise ValueError("budgets_squared must be finite and non-negative")

    finfo = xp.finfo(dtype)
    row_minimum = xp.min(values, axis=1)
    feasibility_scale = xp.maximum(
        xp.maximum(xp.abs(row_minimum), xp.abs(budgets)), 1.0
    )
    feasibility_slack = (
        max(float(tolerance), 64.0 * float(finfo.eps)) * feasibility_scale
    )
    if _scalar(xp.any(budgets + feasibility_slack < row_minimum)):
        raise ValueError("a distortion budget lies below the minimum row cost")
    shifted = values - row_minimum[:, None]
    shifted_budget = xp.maximum(budgets - row_minimum, 0.0)
    minima = shifted == 0
    tie_count = xp.sum(minima, axis=1)
    uniform_cost = xp.mean(shifted, axis=1)

    probabilities = xp.zeros_like(values)
    temperatures = xp.zeros(n_rows, dtype=dtype)
    entropy = xp.empty(n_rows, dtype=dtype)
    expected_shifted = xp.empty(n_rows, dtype=dtype)

    minimum_solution = shifted_budget == 0
    uniform_solution = (~minimum_solution) & (shifted_budget >= uniform_cost)
    active = ~(minimum_solution | uniform_solution)

    if _scalar(xp.any(minimum_solution)):
        q_min = minima[minimum_solution].astype(dtype)
        q_min /= xp.sum(q_min, axis=1, keepdims=True)
        probabilities[minimum_solution] = q_min
        entropy[minimum_solution] = xp.log(tie_count[minimum_solution].astype(dtype))
        expected_shifted[minimum_solution] = 0.0

    if _scalar(xp.any(uniform_solution)):
        probabilities[uniform_solution] = 1.0 / support
        temperatures[uniform_solution] = xp.inf
        entropy[uniform_solution] = np.log(float(support))
        expected_shifted[uniform_solution] = uniform_cost[uniform_solution]

    if _scalar(xp.any(active)):
        active_shifted = shifted[active]
        active_budget = shifted_budget[active]
        positive = active_shifted > 0
        minimum_gap = xp.min(
            xp.where(positive, active_shifted, xp.asarray(xp.inf, dtype=dtype)),
            axis=1,
        )
        if not _scalar(xp.all(xp.isfinite(minimum_gap))):
            raise RuntimeError("a budget-active row has no positive cost gap")
        safe_gap = xp.where(positive, active_shifted, xp.ones_like(active_shifted))
        log_ratio = xp.log(safe_gap) - xp.log(minimum_gap)[:, None]
        log_ratio = xp.where(positive, log_ratio, -xp.inf)
        max_log_ratio = xp.max(log_ratio, axis=1)
        maximum_energy = float(-np.log(float(finfo.tiny)))
        log_energy_cap = float(np.log(maximum_energy))

        def evaluate(log_beta: Any) -> tuple[Any, Any, Any]:
            log_energy = log_beta[:, None] + log_ratio
            energy = xp.exp(xp.minimum(log_energy, log_energy_cap))
            logits = -energy
            logits -= xp.max(logits, axis=1, keepdims=True)
            weights = xp.exp(logits)
            normalizer = xp.sum(weights, axis=1, keepdims=True)
            q_local = weights / normalizer
            log_q = logits - xp.log(normalizer)
            entropy_local = -xp.sum(q_local * log_q, axis=1)
            expected_local = xp.sum(q_local * active_shifted, axis=1)
            return q_local, entropy_local, expected_local

        low = -max_log_ratio - log_energy_cap
        high = xp.full_like(low, log_energy_cap)
        _, _, cost_low = evaluate(low)
        _, _, cost_high = evaluate(high)
        bracket_slack = max(float(tolerance), 64.0 * float(finfo.eps))
        for _ in range(64):
            needs_hotter = cost_low < active_budget * (1.0 - bracket_slack)
            if not _scalar(xp.any(needs_hotter)):
                break
            low = xp.where(needs_hotter, low - 4.0, low)
            _, _, cost_low = evaluate(low)
        for _ in range(64):
            needs_colder = cost_high > active_budget * (1.0 + bracket_slack)
            if not _scalar(xp.any(needs_colder)):
                break
            high = xp.where(needs_colder, high + 4.0, high)
            _, _, cost_high = evaluate(high)
        if _scalar(xp.any(cost_low < active_budget * (1.0 - bracket_slack))) or _scalar(
            xp.any(cost_high > active_budget * (1.0 + bracket_slack))
        ):
            raise RuntimeError("failed to bracket a distortion-budget Gibbs solution")

        for _ in range(int(max_iter)):
            middle = (low + high) * 0.5
            _, _, cost_middle = evaluate(middle)
            feasible = cost_middle <= active_budget
            high = xp.where(feasible, middle, high)
            low = xp.where(feasible, low, middle)

        q_active, entropy_active, cost_active = evaluate(high)
        probabilities[active] = q_active
        entropy[active] = entropy_active
        expected_shifted[active] = cost_active
        log_temperature = xp.log(minimum_gap) - high
        temperatures[active] = xp.exp(
            xp.minimum(log_temperature, float(np.log(float(finfo.max))))
        )

    expected_cost = row_minimum + expected_shifted
    constraint_residual = xp.maximum(expected_cost - budgets, 0.0)
    simplex_residual = xp.abs(xp.sum(probabilities, axis=1) - 1.0)
    validation_scale = xp.maximum(
        xp.maximum(xp.abs(expected_cost), xp.abs(budgets)), 1.0
    )
    validation_tolerance = max(float(tolerance), 128.0 * float(finfo.eps))
    if (
        not _scalar(xp.all(xp.isfinite(probabilities)))
        or not _scalar(xp.all(xp.isfinite(entropy)))
        or _scalar(xp.max(constraint_residual / validation_scale))
        > validation_tolerance
        or _scalar(xp.max(simplex_residual)) > validation_tolerance
    ):
        raise RuntimeError("budgeted Gibbs solve failed its feasibility validation")
    return GibbsResult(
        probabilities=probabilities,
        temperatures=temperatures,
        entropy=entropy,
        constraint_residual=constraint_residual,
        constraint_active=~uniform_solution,
        effective_kappa=xp.exp(entropy),
    )


_BUDGET_REGULARIZATION_KERNEL: Any | None = None


def _cuda_regularize_budget_chunk(
    queries: Any,
    points: Any,
    distances: Any,
    indices: Any,
    budgets: Any,
) -> tuple[Any, Any, Any, Any, Any]:
    """Fused per-row CUDA solve and site accumulation for local budgets.

    One block owns a KNN row.  Costs stay in registers, reductions use a small
    shared workspace, and the final Gibbs law is accumulated directly into
    ``z, a, s, exp(H)``.  No Q x m probability or Q x m x 3 neighbour tensor
    is materialised.  The final float64 contract pass in ``api.py`` remains
    responsible for the outward ``s`` bound.
    """

    import cupy as cp  # type: ignore

    query_values = cp.ascontiguousarray(queries, dtype=cp.float32)
    point_values = cp.ascontiguousarray(points, dtype=cp.float32)
    distance_values = cp.ascontiguousarray(distances, dtype=cp.float32)
    index_values = cp.ascontiguousarray(indices, dtype=cp.int64)
    budget_values = cp.ascontiguousarray(budgets, dtype=cp.float32)
    rows, support = (int(value) for value in distance_values.shape)
    if support > 1_024:
        raise ValueError("fused CUDA local regularization supports m_reg <= 1024")
    threads = 1 << max(0, (support - 1).bit_length())
    centers = cp.empty((rows, 3), dtype=cp.float32)
    additive = cp.empty(rows, dtype=cp.float32)
    distortions = cp.empty(rows, dtype=cp.float32)
    entropy = cp.empty(rows, dtype=cp.float32)
    effective_kappa = cp.empty(rows, dtype=cp.float32)

    global _BUDGET_REGULARIZATION_KERNEL
    if _BUDGET_REGULARIZATION_KERNEL is None:  # pragma: no branch - GPU only
        _BUDGET_REGULARIZATION_KERNEL = cp.RawKernel(
            r"""
            extern "C" __global__ void regularize_budget_rows(
                const float* queries,
                const float* points,
                const float* distances,
                const long long* indices,
                const float* budgets,
                float* centers,
                float* additive,
                float* distortions,
                float* entropy_out,
                float* kappa_out,
                int rows,
                int support,
                int stride
            ) {
                int row = (int)blockIdx.x;
                int tid = (int)threadIdx.x;
                if (row >= rows) return;
                extern __shared__ float shared[];
                float* first = shared;
                float* second = shared + blockDim.x;
                bool valid = tid < support;
                float distance = valid ? distances[(long long)row * stride + tid] : 0.0f;
                float cost = distance * distance;

                first[tid] = valid ? cost : 0.0f;
                second[tid] = valid ? 1.0f : 0.0f;
                __syncthreads();
                for (int offset = blockDim.x / 2; offset > 0; offset >>= 1) {
                    if (tid < offset) {
                        first[tid] += first[tid + offset];
                        second[tid] += second[tid + offset];
                    }
                    __syncthreads();
                }
                float uniform_cost = first[0] / second[0];

                first[tid] = (valid && cost > 0.0f) ? cost : 3.402823466e+38F;
                second[tid] = (valid && cost == 0.0f) ? 1.0f : 0.0f;
                __syncthreads();
                for (int offset = blockDim.x / 2; offset > 0; offset >>= 1) {
                    if (tid < offset) {
                        first[tid] = fminf(first[tid], first[tid + offset]);
                        second[tid] += second[tid + offset];
                    }
                    __syncthreads();
                }
                float minimum_gap = first[0];
                float tie_count = second[0];
                float budget2 = budgets[row] * budgets[row];
                int regime = budget2 <= 0.0f ? 0 : (budget2 >= uniform_cost ? 2 : 1);
                float log_beta_low = -80.0f;
                float log_beta_high = regime == 1 ? logf(80.0f) - logf(minimum_gap) : 0.0f;

                if (regime == 1) {
                    # The high side is cost-feasible; 28 iterations exceed the
                    # useful resolution of a float32 log-temperature bracket.
                    for (int iteration = 0; iteration < 28; ++iteration) {
                        float middle = 0.5f * (log_beta_low + log_beta_high);
                        float weight = 0.0f;
                        if (valid) {
                            float energy = 0.0f;
                            if (cost > 0.0f) {
                                energy = expf(fminf(80.0f, middle + logf(cost)));
                            }
                            weight = expf(-energy);
                        }
                        first[tid] = weight;
                        second[tid] = weight * cost;
                        __syncthreads();
                        for (int offset = blockDim.x / 2; offset > 0; offset >>= 1) {
                            if (tid < offset) {
                                first[tid] += first[tid + offset];
                                second[tid] += second[tid + offset];
                            }
                            __syncthreads();
                        }
                        float expected = second[0] / first[0];
                        if (expected <= budget2) log_beta_high = middle;
                        else log_beta_low = middle;
                        __syncthreads();
                    }
                }

                float weight = 0.0f;
                float beta_cost = 0.0f;
                if (valid) {
                    if (regime == 0) {
                        weight = cost == 0.0f ? 1.0f : 0.0f;
                    } else if (regime == 2) {
                        weight = 1.0f;
                    } else {
                        if (cost > 0.0f) {
                            beta_cost = expf(fminf(80.0f, log_beta_high + logf(cost)));
                        }
                        weight = expf(-beta_cost);
                    }
                }
                first[tid] = weight;
                second[tid] = weight * cost;
                __syncthreads();
                for (int offset = blockDim.x / 2; offset > 0; offset >>= 1) {
                    if (tid < offset) {
                        first[tid] += first[tid + offset];
                        second[tid] += second[tid + offset];
                    }
                    __syncthreads();
                }
                float normalizer = first[0];
                float expected_cost = second[0] / normalizer;
                float probability = weight / normalizer;
                second[tid] = probability > 0.0f ? -probability * logf(probability) : 0.0f;
                __syncthreads();
                for (int offset = blockDim.x / 2; offset > 0; offset >>= 1) {
                    if (tid < offset) second[tid] += second[tid + offset];
                    __syncthreads();
                }
                float row_entropy = second[0];

                long long point_id = valid ? indices[(long long)row * stride + tid] : 0;
                for (int axis = 0; axis < 3; ++axis) {
                    float coordinate = valid ? points[3LL * point_id + axis] : 0.0f;
                    first[tid] = probability * coordinate;
                    __syncthreads();
                    for (int offset = blockDim.x / 2; offset > 0; offset >>= 1) {
                        if (tid < offset) first[tid] += first[tid + offset];
                        __syncthreads();
                    }
                    if (tid == 0) centers[3LL * row + axis] = first[0];
                    __syncthreads();
                }
                float centered_norm2 = 0.0f;
                if (valid) {
                    float px = points[3LL * point_id];
                    float py = points[3LL * point_id + 1];
                    float pz = points[3LL * point_id + 2];
                    float ox = px - centers[3LL * row];
                    float oy = py - centers[3LL * row + 1];
                    float oz = pz - centers[3LL * row + 2];
                    centered_norm2 = ox * ox + oy * oy + oz * oz;
                }
                first[tid] = probability * centered_norm2;
                __syncthreads();
                for (int offset = blockDim.x / 2; offset > 0; offset >>= 1) {
                    if (tid < offset) first[tid] += first[tid + offset];
                    __syncthreads();
                }
                if (tid == 0) {
                    float zx = centers[3LL * row];
                    float zy = centers[3LL * row + 1];
                    float zz = centers[3LL * row + 2];
                    float variance = fmaxf(first[0], 0.0f);
                    float dx = queries[3LL * row] - zx;
                    float dy = queries[3LL * row + 1] - zy;
                    float dz = queries[3LL * row + 2] - zz;
                    additive[row] = variance;
                    distortions[row] = sqrtf(fmaxf(dx*dx + dy*dy + dz*dz + variance, 0.0f));
                    entropy_out[row] = row_entropy;
                    kappa_out[row] = expf(row_entropy);
                }
            }
            """,
            "regularize_budget_rows",
            options=("-std=c++11",),
        )
    _BUDGET_REGULARIZATION_KERNEL(
        (rows,),
        (threads,),
        (
            query_values,
            point_values,
            distance_values,
            index_values,
            budget_values,
            centers,
            additive,
            distortions,
            entropy,
            effective_kappa,
            np.int32(rows),
            np.int32(support),
            np.int32(distance_values.shape[1]),
        ),
        shared_mem=2 * threads * np.dtype(np.float32).itemsize,
    )
    return centers, additive, distortions, entropy, effective_kappa


def regularize_sites_streaming(
    points: Any,
    index: EuclideanKNNIndex,
    *,
    m_reg: int,
    kappa: float,
    chunk_size: int,
    mode: str = "fixed_kappa",
    distortion_budget_absolute: float | None = None,
    distortion_budget_relative: float | None = None,
    local_scale_k: int | None = None,
    require_self_neighbor: bool = True,
    progress: ProgressCallback | None = None,
) -> RegularizedSites:
    """Compute ``(z_i, a_i, s_i)`` while releasing every neighbor block.

    ``fixed_kappa`` solves the historical minimum-cost problem at one entropy
    target.  ``local_distortion`` instead maximises entropy under
    ``s_i <= min(B_abs, gamma R_K(x_i))``.  The latter adapts smoothing to the
    local K-NN scale in one pass and retains the global additive ``s_max``
    certificate as well as the multiplicative theorem when ``local_scale_k=K``.
    """

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
    normalized_mode = str(mode).strip().lower().replace("-", "_")
    if normalized_mode not in {"fixed_kappa", "local_distortion"}:
        raise ValueError("mode must be 'fixed_kappa' or 'local_distortion'")
    if not 1.0 <= float(kappa) <= support:
        raise ValueError("kappa must not exceed the effective regularization support")
    scale_k = int(local_scale_k) if local_scale_k is not None else 1
    if not 1 <= scale_k <= n_points:
        raise ValueError("local_scale_k must lie in [1, number of points]")
    if normalized_mode == "local_distortion":
        if distortion_budget_absolute is None and distortion_budget_relative is None:
            raise ValueError("local_distortion requires an absolute or relative budget")
        if distortion_budget_absolute is not None and (
            not np.isfinite(distortion_budget_absolute)
            or float(distortion_budget_absolute) < 0.0
        ):
            raise ValueError(
                "distortion_budget_absolute must be finite and non-negative"
            )
        if distortion_budget_relative is not None and (
            not np.isfinite(distortion_budget_relative)
            or not 0.0 < float(distortion_budget_relative) < 0.5
        ):
            raise ValueError("distortion_budget_relative must lie in (0, 0.5)")

    centers = xp.empty((n_points, 3), dtype=values.dtype)
    additive = xp.empty(n_points, dtype=values.dtype)
    distortions = xp.empty(n_points, dtype=values.dtype)
    effective_kappa = (
        xp.empty(n_points, dtype=values.dtype)
        if normalized_mode == "local_distortion"
        else None
    )
    local_scales = (
        xp.empty(n_points, dtype=values.dtype)
        if normalized_mode == "local_distortion"
        and distortion_budget_relative is not None
        else None
    )
    max_entropy_violation = 0.0
    max_entropy_deviation = 0.0
    max_simplex_residual = 0.0
    max_budget_violation = 0.0
    max_relative_ratio: float | None = None
    zero_scale_violations = 0
    degenerate_rows = 0
    progress_started = time.perf_counter()

    for start in range(0, n_points, int(chunk_size)):
        stop = min(start + int(chunk_size), n_points)
        queries = values[start:stop]
        query_support = max(
            support,
            (
                scale_k
                if normalized_mode == "local_distortion"
                and distortion_budget_relative is not None
                else support
            ),
        )
        distances, indices = index.query(queries, query_support)
        distances = xp.asarray(distances, dtype=values.dtype)
        indices = xp.asarray(indices)
        _validate_knn_response(
            xp,
            distances,
            indices,
            rows=stop - start,
            columns=query_support,
            index_size=n_points,
        )
        representative_positions = None
        reported_scale_k = None
        reported_scale_previous = None
        if (
            normalized_mode == "local_distortion"
            and distortion_budget_relative is not None
        ):
            reported_scale_k = distances[:, scale_k - 1].copy()
            if scale_k > 1:
                reported_scale_previous = distances[:, scale_k - 2].copy()
        if require_self_neighbor:
            self_ids = xp.arange(start, stop, dtype=indices.dtype)
            self_matches = indices == self_ids[:, None]
            has_self = xp.any(self_matches, axis=1)
            representative_positions = xp.argmax(self_matches, axis=1).astype(
                xp.int64, copy=False
            )
            if _scalar(xp.any(~has_self)):
                missing = xp.flatnonzero(~has_self)
                returned = values[indices[missing]]
                coordinate_matches = xp.all(
                    returned == queries[missing, None, :], axis=2
                )
                has_representative = xp.any(coordinate_matches, axis=1)
                if not _scalar(xp.all(has_representative)):
                    raise RuntimeError(
                        "the KNN backend omitted the observation and every "
                        "coordinate-identical representative; the thesis "
                        "self-neighbour convention cannot be restored"
                    )
                representative_positions[missing] = xp.argmax(
                    coordinate_matches, axis=1
                )
                del returned, coordinate_matches, has_representative, missing

            # cuML RBC has returned a small positive self-distance on some
            # hardware/software combinations.  The observation ID (or an
            # exactly coordinate-identical duplicate) is stronger evidence
            # than that floating distance: canonicalize it to the exact
            # zero-cost atom required by the regularization contract.
            rows = xp.arange(stop - start, dtype=xp.int64)
            indices[rows, representative_positions] = self_ids
            distances[rows, representative_positions] = 0.0
            outside_support = representative_positions >= support
            if _scalar(xp.any(outside_support)):
                outside_rows = xp.flatnonzero(outside_support)
                indices[outside_rows, support - 1] = self_ids[outside_rows]
                distances[outside_rows, support - 1] = 0.0
                del outside_rows
            del rows, self_matches, has_self, outside_support

        support_distances = distances[:, :support]
        support_indices = indices[:, :support]
        cuda_fused_budget = xp is not np and normalized_mode == "local_distortion"
        budgets = None
        local_scale_chunk = None
        if normalized_mode == "local_distortion":
            if distortion_budget_relative is None:
                local_scale_chunk = None
            elif require_self_neighbor:
                assert representative_positions is not None
                assert reported_scale_k is not None
                if scale_k == 1:
                    local_scale_chunk = xp.zeros(stop - start, dtype=values.dtype)
                else:
                    assert reported_scale_previous is not None
                    # Insert the canonical zero in the already ordered list.
                    # If its reported position was before the requested rank,
                    # the original K-th value remains K-th; otherwise the
                    # original (K-1)-st value becomes K-th.
                    local_scale_chunk = xp.where(
                        representative_positions < scale_k - 1,
                        reported_scale_k,
                        reported_scale_previous,
                    )
            else:
                local_scale_chunk = distances[:, scale_k - 1]
            budgets = xp.full(stop - start, xp.inf, dtype=values.dtype)
            if distortion_budget_absolute is not None:
                budgets = xp.minimum(budgets, float(distortion_budget_absolute))
            if distortion_budget_relative is not None:
                assert local_scale_chunk is not None
                budgets = xp.minimum(
                    budgets,
                    float(distortion_budget_relative) * local_scale_chunk,
                )
            if local_scales is not None:
                assert local_scale_chunk is not None
                local_scales[start:stop] = local_scale_chunk

        if cuda_fused_budget:
            assert budgets is not None and effective_kappa is not None
            (
                centers_chunk,
                additive_chunk,
                distortion_chunk,
                entropy_chunk,
                effective_kappa_chunk,
            ) = _cuda_regularize_budget_chunk(
                queries,
                values,
                support_distances,
                support_indices,
                budgets,
            )
            effective_kappa[start:stop] = effective_kappa_chunk
            costs = gibbs = probabilities = neighbors = offsets = displacement = None
        else:
            costs = support_distances * support_distances
            if normalized_mode == "fixed_kappa":
                gibbs = solve_entropy_gibbs(costs, kappa)
            else:
                assert budgets is not None
                gibbs = solve_entropy_gibbs_budget(costs, budgets * budgets)
            if normalized_mode == "local_distortion":
                assert effective_kappa is not None
                effective_kappa[start:stop] = gibbs.effective_kappa
            probabilities = gibbs.probabilities
            neighbors = values[support_indices]
            centers_chunk = xp.sum(probabilities[:, :, None] * neighbors, axis=1)
            offsets = neighbors - centers_chunk[:, None, :]
            additive_chunk = xp.sum(
                probabilities * xp.sum(offsets * offsets, axis=2), axis=1
            )
            additive_chunk = xp.maximum(additive_chunk, 0.0)
            displacement = queries - centers_chunk
            distortion_chunk = xp.sqrt(
                xp.maximum(
                    xp.sum(displacement * displacement, axis=1) + additive_chunk,
                    0.0,
                )
            )
            entropy_chunk = gibbs.entropy
            effective_kappa_chunk = gibbs.effective_kappa

        centers[start:stop] = centers_chunk
        additive[start:stop] = additive_chunk
        distortions[start:stop] = distortion_chunk
        if normalized_mode == "fixed_kappa":
            assert gibbs is not None
            max_entropy_violation = max(
                max_entropy_violation, _scalar(xp.max(gibbs.constraint_residual))
            )
            active_target = gibbs.constraint_active
            if _scalar(xp.any(active_target)):
                max_entropy_deviation = max(
                    max_entropy_deviation,
                    _scalar(
                        xp.max(
                            xp.abs(gibbs.entropy[active_target] - np.log(float(kappa)))
                        )
                    ),
                )
            degenerate_rows += int(_scalar(xp.sum(~active_target)))
        else:
            assert budgets is not None
            max_budget_violation = max(
                max_budget_violation,
                _scalar(xp.max(xp.maximum(distortion_chunk - budgets, 0.0))),
            )
            if distortion_budget_relative is not None:
                assert local_scale_chunk is not None
                positive_scale = local_scale_chunk > 0.0
                zero_scale_violations += int(
                    _scalar(xp.sum((~positive_scale) & (distortion_chunk > 0.0)))
                )
                ratios = xp.where(
                    positive_scale,
                    distortion_chunk
                    / xp.maximum(local_scale_chunk, xp.finfo(values.dtype).tiny),
                    xp.where(distortion_chunk == 0.0, 0.0, xp.inf),
                )
                if zero_scale_violations == 0:
                    chunk_ratio = _scalar(xp.max(ratios))
                    max_relative_ratio = (
                        chunk_ratio
                        if max_relative_ratio is None
                        else max(max_relative_ratio, chunk_ratio)
                    )
        if not cuda_fused_budget:
            assert probabilities is not None
            max_simplex_residual = max(
                max_simplex_residual,
                _scalar(xp.max(xp.abs(xp.sum(probabilities, axis=1) - 1.0))),
            )

        del (
            distances,
            indices,
            support_distances,
            support_indices,
            costs,
            gibbs,
            probabilities,
            neighbors,
            centers_chunk,
            offsets,
            additive_chunk,
            displacement,
            distortion_chunk,
            entropy_chunk,
            effective_kappa_chunk,
            queries,
        )
        if normalized_mode == "local_distortion":
            del budgets, local_scale_chunk
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
                    "chunks": int((n_points + int(chunk_size) - 1) // int(chunk_size)),
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
    if effective_kappa is None:
        effective_quantiles = {str(level): float(kappa) for level in quantile_levels}
        local_scale_quantiles: dict[str, float] = {}
    else:
        if quantile_sample_size == n_points:
            kappa_sample = effective_kappa
        else:
            kappa_sample = effective_kappa[quantile_ids]
        kappa_quantile_values = xp.quantile(kappa_sample, xp.asarray(quantile_levels))
        effective_quantiles = {
            str(level): _scalar(value)
            for level, value in zip(quantile_levels, kappa_quantile_values)
        }
        if local_scales is None:
            local_scale_quantiles = {}
        else:
            scale_sample = (
                local_scales
                if quantile_sample_size == n_points
                else local_scales[quantile_ids]
            )
            scale_quantile_values = xp.quantile(
                scale_sample, xp.asarray(quantile_levels)
            )
            local_scale_quantiles = {
                str(level): _scalar(value)
                for level, value in zip(quantile_levels, scale_quantile_values)
            }
    diagnostics = RegularizationDiagnostics(
        mode=normalized_mode,
        solver_backend=(
            "cuda_fused_budget_v1"
            if xp is not np and normalized_mode == "local_distortion"
            else "vectorized_gibbs"
        ),
        entropy_residual_max=max_entropy_violation,
        entropy_target_deviation_max=max_entropy_deviation,
        simplex_residual_max=(
            None
            if xp is not np and normalized_mode == "local_distortion"
            else max_simplex_residual
        ),
        s_max=_scalar(xp.max(distortions)),
        s_quantiles={
            str(level): _scalar(value)
            for level, value in zip(quantile_levels, quantile_values)
        },
        effective_kappa_quantiles=effective_quantiles,
        local_scale_quantiles=local_scale_quantiles,
        distortion_budget_violation_max=max_budget_violation,
        relative_distortion_ratio_max=(
            None
            if max_relative_ratio is None or zero_scale_violations
            else float(np.nextafter(max_relative_ratio, np.inf))
        ),
        relative_contract_status=(
            "not_requested"
            if distortion_budget_relative is None
            else (
                "unbounded_zero_scale"
                if zero_scale_violations
                else "conditional_float_knn_verified"
            )
        ),
        relative_zero_scale_violations=zero_scale_violations,
        entropy_degenerate_rows=degenerate_rows,
        query_count=n_points,
        s_quantile_sample_size=quantile_sample_size,
    )
    return RegularizedSites(
        centers,
        additive,
        distortions,
        diagnostics,
        effective_kappa,
        local_scales,
    )


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
            r"""
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
            """,
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
        interval_radius_errors = xp.zeros(n_queries, dtype=self.sites.dtype)
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
            histogram[str(candidate_count)] = (
                histogram.get(str(candidate_count), 0) + accepted_count
            )
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
                # A strict rank separation is unnecessary when the candidate
                # and omitted lower-bound intervals merely overlap at machine
                # precision (notably large exact ties).  Certify the *value*
                # interval in that narrow case; materially lower omitted bounds
                # remain uncertain and still fail in strict mode.
                rejected_indices = unresolved
                rejected_kth = kth[~accepted]
                rejected_gap = gaps[~accepted]
                rejected_scale = xp.maximum(
                    xp.maximum(
                        xp.abs(rejected_kth),
                        xp.abs(rejected_kth + rejected_gap),
                    ),
                    1.0,
                )
                rejected_envelope = (
                    self.margin_factor * xp.finfo(self.sites.dtype).eps * rejected_scale
                )
                interval_ok = rejected_gap >= -2.0 * rejected_envelope
                interval_ids = rejected_indices[interval_ok]
                interval_kth = rejected_kth[interval_ok]
                if int(interval_ids.size):
                    outside = interval_kth + rejected_gap[interval_ok]
                    lower_power = xp.minimum(
                        interval_kth - rejected_envelope[interval_ok],
                        outside - rejected_envelope[interval_ok],
                    )
                    upper_power = interval_kth + rejected_envelope[interval_ok]
                    radius = xp.sqrt(xp.maximum(interval_kth, 0.0))
                    radius_lower = xp.sqrt(xp.maximum(lower_power, 0.0))
                    radius_upper = xp.sqrt(xp.maximum(upper_power, 0.0))
                    powers_out[interval_ids] = interval_kth
                    certified[interval_ids] = True
                    used[interval_ids] = candidate_count
                    interval_radius_errors[interval_ids] = xp.maximum(
                        radius - radius_lower, radius_upper - radius
                    )
                    histogram[f"{candidate_count}:value_interval"] = int(
                        interval_ids.size
                    )
                uncertain_ids = rejected_indices[~interval_ok]
                if int(uncertain_ids.size):
                    powers_out[uncertain_ids] = rejected_kth[~interval_ok]
                    used[uncertain_ids] = candidate_count
                    histogram[f"{candidate_count}:uncertain"] = int(uncertain_ids.size)
                unresolved = uncertain_ids
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
        radius_errors = xp.maximum(radius_errors, interval_radius_errors)
        radius_error_max = _scalar(xp.max(radius_errors)) if n_queries else 0.0
        diagnostics = PowerQueryDiagnostics(
            total_queries=n_queries,
            certified_queries=n_queries - uncertain_count,
            uncertain_queries=uncertain_count,
            candidate_histogram=histogram,
            min_certificate_gap=minimum_gap,
            radius_error_max=radius_error_max,
            numerical_certificate=(
                "conditional_float_gap_or_narrow_rank_value_interval_not_directed"
            ),
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


class ExactLiftedPowerKNN:
    """Exact CPU top-K power values via the Euclidean lifting to R4.

    ``sqrt(||y-z_i||^2 + a_i)`` is the Euclidean distance between ``(y, 0)``
    and ``(z_i, sqrt(a_i))``.  SciPy's exact cKDTree therefore removes both
    the Euclidean-candidate cap and the strict-gap failure on tied sites for
    small and medium CPU runs.
    """

    exact = True
    backend_name = "scipy_ckdtree_exact_lifted_power_4d"

    def __init__(self, sites: Any, additive_weights: Any, *, K: int):
        centers = np.asarray(sites, dtype=np.float64)
        additive = np.asarray(additive_weights, dtype=np.float64)
        if centers.ndim != 2 or centers.shape[1] != 3 or centers.shape[0] < 1:
            raise ValueError("sites must have shape (N, 3), N >= 1")
        if additive.shape != (centers.shape[0],):
            raise ValueError("additive_weights must have shape (N,)")
        if not np.isfinite(centers).all() or not np.isfinite(additive).all():
            raise ValueError("sites or additive_weights contain NaN or Inf")
        tolerance = (
            128.0 * np.finfo(np.float64).eps * max(1.0, float(np.max(np.abs(additive))))
        )
        if float(np.min(additive)) < -tolerance:
            raise ValueError("additive power weights must be non-negative")
        if isinstance(K, (bool, np.bool_)) or not isinstance(K, (int, np.integer)):
            raise TypeError("K must be an integer")
        self.K = int(K)
        self.size = int(centers.shape[0])
        if not 1 <= self.K <= self.size:
            raise ValueError("K must satisfy 1 <= K <= number of sites")
        self.sites = np.ascontiguousarray(centers)
        self.additive = np.maximum(additive, 0.0)
        lifted = np.column_stack((self.sites, np.sqrt(self.additive)))
        self._tree = cKDTree(lifted, leafsize=32, compact_nodes=True)

    def query(self, queries: Any) -> PowerQueryResult:
        values = np.asarray(queries, dtype=np.float64)
        if values.ndim != 2 or values.shape[1] != 3 or values.shape[0] < 1:
            raise ValueError("queries must have shape (Q, 3), Q >= 1")
        if not np.isfinite(values).all():
            raise ValueError("queries contain NaN or Inf")
        lifted_queries = np.column_stack(
            (values, np.zeros(values.shape[0], dtype=np.float64))
        )
        distances, _ = self._tree.query(lifted_queries, k=self.K, workers=1)
        if self.K == 1:
            radii = np.asarray(distances, dtype=np.float64)
        else:
            radii = np.asarray(distances[:, self.K - 1], dtype=np.float64)
        powers = radii * radii
        scale = np.maximum(np.abs(radii), 1.0)
        radius_errors = 64.0 * np.finfo(np.float64).eps * scale
        n_queries = int(values.shape[0])
        diagnostics = PowerQueryDiagnostics(
            total_queries=n_queries,
            certified_queries=n_queries,
            uncertain_queries=0,
            candidate_histogram={"lifted_exact": n_queries},
            min_certificate_gap=None,
            radius_error_max=float(np.max(radius_errors)),
            numerical_certificate="exact_ckdtree_search_on_float64_lifted_sites",
        )
        return PowerQueryResult(
            power_values=powers,
            radii=radii,
            radius_errors=radius_errors,
            certified=np.ones(n_queries, dtype=np.bool_),
            candidates_used=np.full(n_queries, self.K, dtype=np.int32),
            diagnostics=diagnostics,
        )


def build_grid_spec(sites: Any, requested_shape: int | Sequence[int]) -> GridSpec:
    """Build the site-bounding box grid, collapsing constant axes to one cell."""

    xp = _array_module(sites)
    values = xp.asarray(sites)
    if values.ndim != 2 or values.shape[1] != 3 or values.shape[0] < 1:
        raise ValueError("sites must have shape (N, 3), N >= 1")
    bbox_min_array = xp.min(values, axis=0)
    bbox_max_array = xp.max(values, axis=0)
    bbox_min = cast(
        tuple[float, float, float], tuple(_scalar(item) for item in bbox_min_array)
    )
    bbox_max = cast(
        tuple[float, float, float], tuple(_scalar(item) for item in bbox_max_array)
    )
    requested = _shape3(requested_shape)
    span = tuple(high - low for low, high in zip(bbox_min, bbox_max))
    global_scale = max(1.0, max(abs(item) for item in (*bbox_min, *bbox_max)))
    tolerance = 64.0 * np.finfo(values.dtype).eps * global_scale
    shape = cast(
        tuple[int, int, int],
        tuple(
            1 if width <= tolerance else cells for width, cells in zip(span, requested)
        ),
    )
    cell_widths = cast(
        tuple[float, float, float],
        tuple(
            width / cells if cells > 1 or width > 0 else 0.0
            for width, cells in zip(span, shape)
        ),
    )
    half_diagonal = 0.5 * float(np.sqrt(sum(width * width for width in cell_widths)))
    _validate_float32_grid_coordinates(bbox_min, bbox_max, shape, cell_widths)
    return GridSpec(bbox_min, bbox_max, shape, cell_widths, half_diagonal)


def build_grid_spec_for_local_scale(
    sites: Any,
    *,
    min_resolved_radius: float,
    max_relative_spatial_error: float,
    max_cells: int,
) -> GridSpec:
    """Derive an anisotropic uniform grid from a *local radius* contract.

    The shape is not obtained from a global side count.  Instead it enforces
    ``2 H <= eta * r_min`` so the geometric inner/outer shift is at most an
    ``eta`` fraction of every filtration radius ``r >= r_min`` (before the
    separately reported numerical margin).  The scene extent only determines
    how many such physically sized cells are necessary.  If that count is not
    affordable, the function refuses the run rather than silently coarsening
    the hierarchy.

    This remains a uniform anisotropic grid, not an adaptive-octree claim.  It
    is a conservative bridge to tiling/adaptive refinement with a sound H0
    contract.
    """

    radius = float(min_resolved_radius)
    relative = float(max_relative_spatial_error)
    if not np.isfinite(radius) or radius <= 0.0:
        raise ValueError("min_resolved_radius must be finite and positive")
    if not np.isfinite(relative) or not 0.0 < relative <= 1.0:
        raise ValueError("max_relative_spatial_error must lie in (0, 1]")
    if isinstance(max_cells, (bool, np.bool_)) or not isinstance(
        max_cells, (int, np.integer)
    ):
        raise TypeError("max_cells must be an integer")
    if int(max_cells) < 1:
        raise ValueError("max_cells must be positive")

    xp = _array_module(sites)
    values = xp.asarray(sites)
    if values.ndim != 2 or values.shape[1] != 3 or values.shape[0] < 1:
        raise ValueError("sites must have shape (N, 3), N >= 1")
    bbox_min = cast(
        tuple[float, float, float],
        tuple(_scalar(item) for item in xp.min(values, axis=0)),
    )
    bbox_max = cast(
        tuple[float, float, float],
        tuple(_scalar(item) for item in xp.max(values, axis=0)),
    )
    spans = tuple(high - low for low, high in zip(bbox_min, bbox_max))
    # A nonzero stored span is geometrically real at the requested local
    # scale, even when it lies below a global float32-relative heuristic.
    # Subdivide it and let the explicit coordinate-representability guard
    # decide whether the resulting centres are portable.
    half_diagonal_limit = 0.5 * relative * radius
    diameter_squared_budget = (2.0 * half_diagonal_limit) ** 2
    unresolved_axes = {axis for axis, width in enumerate(spans) if width > 0.0}
    one_cell_axes = {axis for axis, width in enumerate(spans) if width == 0.0}
    fixed_squared_width = 0.0
    width_limit = math.inf
    while unresolved_axes:
        remaining_squared_budget = diameter_squared_budget - fixed_squared_width
        if remaining_squared_budget <= 0.0:
            raise RuntimeError(
                "the one-cell grid axes already exceed the local scale "
                "diagonal budget"
            )
        width_limit = math.sqrt(remaining_squared_budget / len(unresolved_axes))
        newly_fixed = {axis for axis in unresolved_axes if spans[axis] <= width_limit}
        if not newly_fixed:
            break
        one_cell_axes.update(newly_fixed)
        fixed_squared_width = math.fsum(
            spans[axis] * spans[axis] for axis in one_cell_axes
        )
        unresolved_axes.difference_update(newly_fixed)

    if not unresolved_axes:
        shape = (1, 1, 1)
    else:
        shape = cast(
            tuple[int, int, int],
            tuple(
                (
                    1
                    if axis in one_cell_axes
                    else max(1, int(math.ceil(width / width_limit)))
                )
                for axis, width in enumerate(spans)
            ),
        )
    n_cells = math.prod(shape)
    if n_cells > int(max_cells):
        raise GridResolutionBudgetError(n_cells, int(max_cells), shape)
    cell_widths = cast(
        tuple[float, float, float],
        tuple(
            width / cells if width > 0.0 else 0.0 for width, cells in zip(spans, shape)
        ),
    )
    half_diagonal = 0.5 * math.sqrt(sum(width * width for width in cell_widths))
    # Guard against a one-ulp ceil/quotient anomaly without weakening the
    # requested contract.
    if half_diagonal > half_diagonal_limit + 32.0 * math.ulp(half_diagonal_limit):
        raise RuntimeError("failed to derive a grid within the local scale limit")
    _validate_float32_grid_coordinates(bbox_min, bbox_max, shape, cell_widths)
    return GridSpec(
        bbox_min=bbox_min,
        bbox_max=bbox_max,
        shape=shape,
        cell_widths=cell_widths,
        half_diagonal=half_diagonal,
        policy="local_scale",
        requested_half_diagonal_limit=half_diagonal_limit,
    )


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
    oracle: CertifiedPowerKNN | ExactLiftedPowerKNN,
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
    numerical_certificates: set[str] = set()
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
        numerical_certificates.add(result.diagnostics.numerical_certificate)
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
                    "chunks": int((n_cubes + int(chunk_size) - 1) // int(chunk_size)),
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
        numerical_certificate=(
            next(iter(numerical_certificates))
            if len(numerical_certificates) == 1
            else "mixed:" + ",".join(sorted(numerical_certificates))
        ),
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
