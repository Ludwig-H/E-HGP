"""Public contracts for the spatial PERG-HGP backend.

The objects in this module deliberately contain no CUDA dependency.  They are
used by the exact CPU reference, the optional RAPIDS/CuPy implementation and
the Colab benchmark notebook.
"""

from __future__ import annotations

from dataclasses import asdict, dataclass, field
from math import isfinite, prod, ulp
from numbers import Integral, Real
from typing import Any, Dict, Sequence, Tuple, cast


def _integer(name: str, value: Any, *, minimum: int) -> int:
    """Return a validated integer without silently accepting bools/floats."""

    if isinstance(value, bool) or not isinstance(value, Integral):
        raise TypeError(f"{name} must be an integer")
    result = int(value)
    if result < minimum:
        qualifier = "positive" if minimum == 1 else f">= {minimum}"
        raise ValueError(f"{name} must be {qualifier}")
    return result


def _real(name: str, value: Any) -> float:
    """Return a finite real scalar without treating booleans as numbers."""

    if isinstance(value, bool) or not isinstance(value, Real):
        raise TypeError(f"{name} must be a real number")
    result = float(value)
    if not isfinite(result):
        raise ValueError(f"{name} must be finite")
    return result


def _boolean(name: str, value: Any) -> bool:
    if not isinstance(value, bool):
        raise TypeError(f"{name} must be a bool")
    return value


def _optional_real(
    name: str, value: Any, *, minimum: float | None = None, strict: bool = False
) -> float | None:
    """Validate an optional finite real without silently accepting booleans."""

    if value is None:
        return None
    result = _real(name, value)
    if minimum is not None:
        invalid = result <= minimum if strict else result < minimum
        if invalid:
            relation = ">" if strict else ">="
            raise ValueError(f"{name} must be {relation} {minimum}")
    return result


def _real3(name: str, value: Sequence[float]) -> Tuple[float, float, float]:
    try:
        items = tuple(value)
    except TypeError as error:
        raise TypeError(f"{name} must contain three finite real numbers") from error
    if len(items) != 3:
        raise ValueError(f"{name} must have three coordinates")
    return cast(
        Tuple[float, float, float],
        tuple(_real(f"{name}[{axis}]", item) for axis, item in enumerate(items)),
    )


def _shape3(value: int | Sequence[int]) -> Tuple[int, int, int]:
    if isinstance(value, Integral) and not isinstance(value, bool):
        item = _integer("grid_shape", value, minimum=1)
        result = (item, item, item)
    else:
        try:
            items = tuple(value)  # type: ignore[arg-type]
        except TypeError as error:
            raise TypeError(
                "grid_shape must be an integer or three positive integers"
            ) from error
        if len(items) != 3:
            raise ValueError("grid_shape must be an integer or three positive integers")
        result = cast(
            Tuple[int, int, int],
            tuple(
                _integer(f"grid_shape[{axis}]", item, minimum=1)
                for axis, item in enumerate(items)
            ),
        )
    return result


@dataclass(frozen=True)
class PowerCoverConfig:
    """Configuration of the power-cover H0 pipeline.

    ``K``, ``kappa`` and ``m_reg`` are intentionally independent.  ``K`` is
    the order of the cover, ``kappa`` is the effective entropy support and
    ``m_reg`` is the hard local support on which the entropy problem is solved.
    """

    K: int = 10
    kappa: float = 4.0
    m_reg: int = 64
    grid_shape: Tuple[int, int, int] = (128, 128, 128)
    chunk_size: int = 262_144
    power_chunk_size: int = 65_536
    candidate_k_initial: int = 64
    candidate_k_max: int = 1_024
    strict_certification: bool = True
    neighbor_audit_queries: int = 32
    require_neighbor_audit: bool = True
    numerical_margin_factor: float = 64.0
    max_vram_fraction: float = 0.85
    regularization_mode: str = "fixed_kappa"
    distortion_budget_absolute: float | None = None
    distortion_budget_relative: float | None = None
    local_scale_k: int | None = None
    min_resolved_radius: float | None = None
    max_relative_spatial_error: float = 0.25
    max_grid_cells: int = 50_000_000

    def __post_init__(self) -> None:
        object.__setattr__(self, "K", _integer("K", self.K, minimum=1))
        object.__setattr__(self, "m_reg", _integer("m_reg", self.m_reg, minimum=1))
        object.__setattr__(self, "grid_shape", _shape3(self.grid_shape))
        object.__setattr__(
            self, "chunk_size", _integer("chunk_size", self.chunk_size, minimum=1)
        )
        object.__setattr__(
            self,
            "power_chunk_size",
            _integer("power_chunk_size", self.power_chunk_size, minimum=1),
        )
        object.__setattr__(
            self,
            "candidate_k_initial",
            _integer("candidate_k_initial", self.candidate_k_initial, minimum=1),
        )
        object.__setattr__(
            self,
            "candidate_k_max",
            _integer("candidate_k_max", self.candidate_k_max, minimum=1),
        )
        object.__setattr__(
            self,
            "neighbor_audit_queries",
            _integer("neighbor_audit_queries", self.neighbor_audit_queries, minimum=0),
        )
        object.__setattr__(
            self,
            "max_grid_cells",
            _integer("max_grid_cells", self.max_grid_cells, minimum=1),
        )
        object.__setattr__(
            self,
            "strict_certification",
            _boolean("strict_certification", self.strict_certification),
        )
        object.__setattr__(
            self,
            "require_neighbor_audit",
            _boolean("require_neighbor_audit", self.require_neighbor_audit),
        )
        kappa = _real("kappa", self.kappa)
        margin = _real("numerical_margin_factor", self.numerical_margin_factor)
        vram_fraction = _real("max_vram_fraction", self.max_vram_fraction)
        object.__setattr__(self, "kappa", kappa)
        object.__setattr__(self, "numerical_margin_factor", margin)
        object.__setattr__(self, "max_vram_fraction", vram_fraction)
        if not isinstance(self.regularization_mode, str):
            raise TypeError("regularization_mode must be a string")
        mode = self.regularization_mode.strip().lower().replace("-", "_")
        if mode not in {"fixed_kappa", "local_distortion"}:
            raise ValueError(
                "regularization_mode must be 'fixed_kappa' or 'local_distortion'"
            )
        object.__setattr__(self, "regularization_mode", mode)
        absolute_budget = _optional_real(
            "distortion_budget_absolute",
            self.distortion_budget_absolute,
            minimum=0.0,
        )
        relative_budget = _optional_real(
            "distortion_budget_relative",
            self.distortion_budget_relative,
            minimum=0.0,
            strict=True,
        )
        object.__setattr__(self, "distortion_budget_absolute", absolute_budget)
        object.__setattr__(self, "distortion_budget_relative", relative_budget)
        local_scale_k = (
            None
            if self.local_scale_k is None
            else _integer("local_scale_k", self.local_scale_k, minimum=1)
        )
        if relative_budget is not None and local_scale_k is None:
            local_scale_k = self.K
        object.__setattr__(self, "local_scale_k", local_scale_k)
        min_resolved_radius = _optional_real(
            "min_resolved_radius", self.min_resolved_radius, minimum=0.0, strict=True
        )
        relative_spatial_error = _real(
            "max_relative_spatial_error", self.max_relative_spatial_error
        )
        object.__setattr__(self, "min_resolved_radius", min_resolved_radius)
        object.__setattr__(self, "max_relative_spatial_error", relative_spatial_error)
        if not 1.0 <= kappa <= self.m_reg:
            raise ValueError("kappa must satisfy 1 <= kappa <= m_reg")
        if self.candidate_k_initial < self.K:
            raise ValueError("candidate_k_initial must be at least K")
        if self.candidate_k_max < self.candidate_k_initial:
            raise ValueError("candidate_k_max must be >= candidate_k_initial")
        if self.require_neighbor_audit and self.neighbor_audit_queries < 1:
            raise ValueError("strict neighbor audit requires at least one query")
        if margin < 1.0:
            raise ValueError("numerical_margin_factor must be at least one")
        if not 0.0 < vram_fraction <= 1.0:
            raise ValueError("max_vram_fraction must lie in (0, 1]")
        if (
            mode == "local_distortion"
            and absolute_budget is None
            and relative_budget is None
        ):
            raise ValueError(
                "local_distortion requires distortion_budget_absolute and/or "
                "distortion_budget_relative"
            )
        if mode == "fixed_kappa" and (
            absolute_budget is not None or relative_budget is not None
        ):
            raise ValueError(
                "distortion budgets require regularization_mode='local_distortion'"
            )
        if relative_budget is not None and relative_budget >= 0.5:
            raise ValueError(
                "distortion_budget_relative must be < 0.5 for the local "
                "multiplicative K-NN guarantee"
            )
        if not 0.0 < relative_spatial_error <= 1.0:
            raise ValueError("max_relative_spatial_error must lie in (0, 1]")

    @property
    def n_cubes(self) -> int:
        return int(prod(self.grid_shape))

    def to_dict(self) -> Dict[str, Any]:
        result = asdict(self)
        result["grid_shape"] = list(self.grid_shape)
        return result


@dataclass(frozen=True)
class GridSpec:
    """Axis-aligned cubical discretisation of the site bounding box."""

    bbox_min: Tuple[float, float, float]
    bbox_max: Tuple[float, float, float]
    shape: Tuple[int, int, int]
    cell_widths: Tuple[float, float, float]
    half_diagonal: float
    policy: str = "fixed_shape"
    requested_half_diagonal_limit: float | None = None

    def __post_init__(self) -> None:
        bbox_min = _real3("bbox_min", self.bbox_min)
        bbox_max = _real3("bbox_max", self.bbox_max)
        shape = _shape3(self.shape)
        cell_widths = _real3("cell_widths", self.cell_widths)
        half_diagonal = _real("half_diagonal", self.half_diagonal)
        if not isinstance(self.policy, str):
            raise TypeError("policy must be a string")
        policy = self.policy.strip().lower().replace("-", "_")
        if policy not in {"fixed_shape", "local_scale"}:
            raise ValueError("policy must be 'fixed_shape' or 'local_scale'")
        requested_limit = _optional_real(
            "requested_half_diagonal_limit",
            self.requested_half_diagonal_limit,
            minimum=0.0,
            strict=True,
        )
        if any(high < low for low, high in zip(bbox_min, bbox_max)):
            raise ValueError("bbox_max must be coordinatewise >= bbox_min")
        if any(width < 0.0 for width in cell_widths):
            raise ValueError("cell_widths must be non-negative")
        if half_diagonal < 0.0:
            raise ValueError("half_diagonal must be non-negative")
        expected_widths = tuple(
            (high - low) / cells for low, high, cells in zip(bbox_min, bbox_max, shape)
        )
        width_scale = max(1.0, *expected_widths, *cell_widths)
        base_consistency_tolerance = 128.0 * ulp(width_scale)
        coordinate_tolerances = tuple(
            max(
                base_consistency_tolerance,
                4.0 * (ulp(abs(low)) + ulp(abs(high))) / cells,
            )
            for low, high, cells in zip(bbox_min, bbox_max, shape)
        )
        if any(
            abs(actual - expected) > tolerance
            for actual, expected, tolerance in zip(
                cell_widths, expected_widths, coordinate_tolerances
            )
        ):
            raise ValueError("cell_widths are inconsistent with bbox and shape")
        expected_half_diagonal = (
            0.5 * sum(width * width for width in cell_widths) ** 0.5
        )
        if abs(half_diagonal - expected_half_diagonal) > 128.0 * ulp(
            max(1.0, half_diagonal, expected_half_diagonal)
        ):
            raise ValueError("half_diagonal is inconsistent with cell_widths")
        object.__setattr__(self, "bbox_min", bbox_min)
        object.__setattr__(self, "bbox_max", bbox_max)
        object.__setattr__(self, "shape", shape)
        object.__setattr__(self, "cell_widths", cell_widths)
        object.__setattr__(self, "half_diagonal", half_diagonal)
        object.__setattr__(self, "policy", policy)
        object.__setattr__(self, "requested_half_diagonal_limit", requested_limit)
        if (
            requested_limit is not None
            and half_diagonal > requested_limit + 32.0 * ulp(requested_limit)
        ):
            raise ValueError("half_diagonal exceeds requested_half_diagonal_limit")

    @property
    def n_cubes(self) -> int:
        return int(prod(self.shape))

    def to_dict(self) -> Dict[str, Any]:
        result = asdict(self)
        for key in ("bbox_min", "bbox_max", "shape", "cell_widths"):
            result[key] = list(result[key])
        return result


@dataclass(frozen=True)
class SpatialTransform:
    """Isotropic normalization used by all GPU geometry predicates."""

    offset: Tuple[float, float, float]
    scale: float

    def __post_init__(self) -> None:
        offset = _real3("offset", self.offset)
        scale = _real("scale", self.scale)
        if scale <= 0.0:
            raise ValueError("scale must be strictly positive")
        object.__setattr__(self, "offset", offset)
        object.__setattr__(self, "scale", scale)

    def to_dict(self) -> Dict[str, Any]:
        return {"offset": list(self.offset), "scale": float(self.scale)}


@dataclass
class RegularizationDiagnostics:
    mode: str = "fixed_kappa"
    solver_backend: str = "vectorized_gibbs"
    entropy_residual_max: float = 0.0
    entropy_target_deviation_max: float = 0.0
    simplex_residual_max: float | None = 0.0
    s_max: float = 0.0
    s_quantiles: Dict[str, float] = field(default_factory=dict)
    effective_kappa_quantiles: Dict[str, float] = field(default_factory=dict)
    local_scale_quantiles: Dict[str, float] = field(default_factory=dict)
    distortion_budget_violation_max: float = 0.0
    relative_distortion_ratio_max: float | None = None
    relative_contract_status: str = "not_requested"
    relative_zero_scale_violations: int = 0
    local_scale_below_resolution_count: int | None = None
    local_scale_below_resolution_fraction: float | None = None
    entropy_degenerate_rows: int = 0
    query_count: int = 0
    s_quantile_sample_size: int = 0

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)


@dataclass
class PowerQueryDiagnostics:
    total_queries: int = 0
    certified_queries: int = 0
    uncertain_queries: int = 0
    candidate_histogram: Dict[str, int] = field(default_factory=dict)
    min_certificate_gap: float | None = None
    radius_error_max: float = 0.0
    numerical_certificate: str = "float64_distance_guard"

    @property
    def certified_fraction(self) -> float:
        return self.certified_queries / max(1, self.total_queries)

    @property
    def conditional_guard_pass_fraction(self) -> float:
        """Compatibility-neutral name for the conditional RBC guard metric."""

        return self.certified_fraction

    def to_dict(self) -> Dict[str, Any]:
        result = asdict(self)
        result["certified_fraction"] = self.certified_fraction
        result["conditional_guard_pass_fraction"] = self.conditional_guard_pass_fraction
        return result


@dataclass(frozen=True)
class MemoryEstimate:
    """Conservative byte budget for a single-GPU run."""

    n_points: int
    n_cubes: int
    persistent_bytes: int
    regularization_chunk_bytes: int
    power_query_chunk_bytes: int
    cubical_msf_bytes_per_forest: int
    estimated_peak_bytes: int
    assumptions: Tuple[str, ...]

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)


@dataclass
class AccuracyReport:
    """Machine-readable statement of what a run did and did not certify."""

    mode: str = "spatial_enveloped"
    neighbor_support: str = "unknown"
    entropy_residual_max: float | None = None
    input_quantization_radius: float | None = None
    regularization_interleaving_radius: float | None = None
    regularization_multiplicative_lower: float | None = None
    regularization_multiplicative_upper: float | None = None
    power_knn_certified_fraction: float | None = None
    conditional_guard_pass_fraction: float | None = None
    power_knn_uncertain_count: int | None = None
    numerical_radius_error: float | None = None
    base_spatial_interleaving_radius: float | None = None
    spatial_interleaving_radius: float | None = None
    base_total_interleaving_radius: float | None = None
    total_interleaving_radius: float | None = None
    min_resolved_radius: float | None = None
    relative_spatial_error_at_min_radius: float | None = None
    grid_policy: str = "fixed_shape"
    inner_outer_merge_disagreement_count: int | None = None
    cubical_connectivity: int = 26
    cubical_msf_exact_for_stored_float32_values: bool = False
    cubical_msf_exact_for_stored_values: bool = False
    field_dtype: str = "unknown"
    power_oracle_exact: bool = False
    flat_selection: str = "none"
    atlas_completeness: str = "not_used"
    full_power_hgp_complete: bool = False
    K_fixed_finite_sample_resolution: bool = True
    hardware_manifest: Dict[str, Any] = field(default_factory=dict)
    notes: list[str] = field(default_factory=list)

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)


def estimate_memory(
    n_points: int,
    config: PowerCoverConfig,
    *,
    index_bytes_per_point: int = 64,
    input_coordinate_bytes: int = 4,
    n_cubes: int | None = None,
) -> MemoryEstimate:
    """Estimate the peak device memory without claiming a measured value.

    The estimate includes a deliberately conservative allowance for an RBC or
    spatial index.  The notebook compares it with NVML measurements; it must
    never be presented as a benchmark result.
    """

    n_points = _integer("n_points", n_points, minimum=1)
    index_bytes_per_point = _integer(
        "index_bytes_per_point", index_bytes_per_point, minimum=0
    )
    input_coordinate_bytes = _integer(
        "input_coordinate_bytes", input_coordinate_bytes, minimum=1
    )
    if n_cubes is None:
        n_cubes = config.n_cubes
    else:
        n_cubes = _integer("n_cubes", n_cubes, minimum=1)
    q_regularization = min(config.chunk_size, n_points)
    q_power = min(config.power_chunk_size, n_cubes)

    # Caller-owned X, normalized X, Z, a and s can coexist during streaming.
    # The allowance also includes the active spatial index.  A float64 caller
    # adds another 12 bytes/point beyond this float32 target configuration.
    # Local-distortion mode stores one effective perplexity per point.  The
    # local KNN radius is streamed and summarized, not retained globally.
    local_regularization_bytes = (
        4 + (4 if config.distortion_budget_relative is not None else 0)
        if config.regularization_mode == "local_distortion"
        else 0
    )
    caller_dtype_extra = 3 * max(0, input_coordinate_bytes - 4)
    persistent = n_points * (
        11 * 4 + caller_dtype_extra + local_regularization_bytes + index_bytes_per_point
    )

    # Query indices are conservatively counted as int64.  The 60-byte factor
    # covers distances, Gibbs probabilities, gathered coordinates and the
    # largest elementwise temporaries that coexist in the current loop.
    effective_m_reg = min(config.m_reg, n_points)
    raw_query_support = min(
        n_points,
        max(
            effective_m_reg,
            (
                config.local_scale_k
                if config.regularization_mode == "local_distortion"
                and config.distortion_budget_relative is not None
                and config.local_scale_k is not None
                else effective_m_reg
            ),
        ),
    )
    # The 60-byte Gibbs/geometry allowance applies to m_reg entries; any
    # additional KNN distances used only to read R_K need distance+id storage.
    regularization_chunk = q_regularization * effective_m_reg * 60
    regularization_chunk += (
        q_regularization * max(0, raw_query_support - effective_m_reg) * 12
    )
    regularization_chunk += q_regularization * 64

    candidates = min(n_points, config.candidate_k_max + 1)
    # Fused CUDA power evaluation retains distances (4), indices (8), powers
    # (4), validation masks and a partition-workspace allowance (4) per
    # candidate.  It does not create Q x C x 3 coordinate tensors.
    power_chunk = q_power * candidates * 20
    power_chunk += q_power * 64

    # values, parent, best-key, N-1 compact edges and sort scratch allowance.
    msf_one = n_cubes * (4 + 4 + 8 + 12 + 8)
    peak = persistent + max(regularization_chunk, power_chunk, 2 * msf_one)

    return MemoryEstimate(
        n_points=n_points,
        n_cubes=n_cubes,
        persistent_bytes=int(persistent),
        regularization_chunk_bytes=int(regularization_chunk),
        power_query_chunk_bytes=int(power_chunk),
        cubical_msf_bytes_per_forest=int(msf_one),
        estimated_peak_bytes=int(peak),
        assumptions=(
            "float32 input/normalized compute geometry; float64 public centers",
            f"{input_coordinate_bytes}-byte caller coordinate dtype",
            "int64 neighbor outputs (conservative)",
            f"{index_bytes_per_point} bytes/point spatial-index allowance",
            "caller input and normalized coordinates may coexist",
            "separate 65536-default chunks for cubical power queries",
            "fused power kernel; no Q x C x 3 candidate tensors",
            "implicit 26-neighbor grid; no explicit grid edge list",
            "one dominant streaming scratch allocation at a time",
        ),
    )
