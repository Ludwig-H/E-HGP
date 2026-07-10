"""Public contracts for the spatial PERG-HGP backend.

The objects in this module deliberately contain no CUDA dependency.  They are
used by the exact CPU reference, the optional RAPIDS/CuPy implementation and
the Colab benchmark notebook.
"""

from __future__ import annotations

from dataclasses import asdict, dataclass, field
from math import isfinite, prod
from numbers import Integral, Real
from typing import Any, Dict, Sequence, Tuple


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


def _real3(name: str, value: Sequence[float]) -> Tuple[float, float, float]:
    try:
        items = tuple(value)
    except TypeError as error:
        raise TypeError(f"{name} must contain three finite real numbers") from error
    if len(items) != 3:
        raise ValueError(f"{name} must have three coordinates")
    return tuple(_real(f"{name}[{axis}]", item) for axis, item in enumerate(items))


def _shape3(value: int | Sequence[int]) -> Tuple[int, int, int]:
    if isinstance(value, Integral) and not isinstance(value, bool):
        item = _integer("grid_shape", value, minimum=1)
        result = (item, item, item)
    else:
        try:
            items = tuple(value)
        except TypeError as error:
            raise TypeError(
                "grid_shape must be an integer or three positive integers"
            ) from error
        if len(items) != 3:
            raise ValueError(
                "grid_shape must be an integer or three positive integers"
            )
        result = tuple(
            _integer(f"grid_shape[{axis}]", item, minimum=1)
            for axis, item in enumerate(items)
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

    def __post_init__(self) -> None:
        object.__setattr__(self, "K", _integer("K", self.K, minimum=1))
        object.__setattr__(
            self, "m_reg", _integer("m_reg", self.m_reg, minimum=1)
        )
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
            _integer(
                "neighbor_audit_queries", self.neighbor_audit_queries, minimum=0
            ),
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

    def __post_init__(self) -> None:
        bbox_min = _real3("bbox_min", self.bbox_min)
        bbox_max = _real3("bbox_max", self.bbox_max)
        shape = _shape3(self.shape)
        cell_widths = _real3("cell_widths", self.cell_widths)
        half_diagonal = _real("half_diagonal", self.half_diagonal)
        if any(high < low for low, high in zip(bbox_min, bbox_max)):
            raise ValueError("bbox_max must be coordinatewise >= bbox_min")
        if any(width < 0.0 for width in cell_widths):
            raise ValueError("cell_widths must be non-negative")
        if half_diagonal < 0.0:
            raise ValueError("half_diagonal must be non-negative")
        object.__setattr__(self, "bbox_min", bbox_min)
        object.__setattr__(self, "bbox_max", bbox_max)
        object.__setattr__(self, "shape", shape)
        object.__setattr__(self, "cell_widths", cell_widths)
        object.__setattr__(self, "half_diagonal", half_diagonal)

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
    entropy_residual_max: float = 0.0
    entropy_target_deviation_max: float = 0.0
    simplex_residual_max: float = 0.0
    s_max: float = 0.0
    s_quantiles: Dict[str, float] = field(default_factory=dict)
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

    def to_dict(self) -> Dict[str, Any]:
        result = asdict(self)
        result["certified_fraction"] = self.certified_fraction
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
    regularization_interleaving_radius: float | None = None
    power_knn_certified_fraction: float | None = None
    power_knn_uncertain_count: int | None = None
    numerical_radius_error: float | None = None
    spatial_interleaving_radius: float | None = None
    total_interleaving_radius: float | None = None
    inner_outer_merge_disagreement_count: int | None = None
    cubical_connectivity: int = 26
    cubical_msf_exact_for_stored_float32_values: bool = False
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
    n_cubes = config.n_cubes
    q_regularization = min(config.chunk_size, n_points)
    q_power = min(config.power_chunk_size, n_cubes)

    # Caller-owned X, normalized X, Z, a and s can coexist during streaming.
    # The allowance also includes the active spatial index.  A float64 caller
    # adds another 12 bytes/point beyond this float32 target configuration.
    persistent = n_points * (11 * 4 + index_bytes_per_point)

    # Query indices are conservatively counted as int64.  The 60-byte factor
    # covers distances, Gibbs probabilities, gathered coordinates and the
    # largest elementwise temporaries that coexist in the current loop.
    regularization_chunk = q_regularization * config.m_reg * 60
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
            "int64 neighbor outputs (conservative)",
            f"{index_bytes_per_point} bytes/point spatial-index allowance",
            "caller input and normalized coordinates may coexist",
            "separate 65536-default chunks for cubical power queries",
            "fused power kernel; no Q x C x 3 candidate tensors",
            "implicit 26-neighbor grid; no explicit grid edge list",
            "one dominant streaming scratch allocation at a time",
        ),
    )
