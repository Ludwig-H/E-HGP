"""End-to-end API for the spatial, entropy-regularized PERG-HGP backend."""

from __future__ import annotations

from dataclasses import dataclass
import json
import os
import time
from typing import Any, Iterable, Sequence

import numpy as np

from .contracts import (
    AccuracyReport,
    GridSpec,
    MemoryEstimate,
    PowerCoverConfig,
    PowerQueryDiagnostics,
    RegularizationDiagnostics,
    SpatialTransform,
    estimate_memory,
)
from .cubical_msf import CubicalMSFResult, build_cubical_msf
from .spatial_core import (
    CertifiedPowerKNN,
    CubicalField,
    ExactKDTreeIndex,
    RegularizedSites,
    build_grid_spec,
    evaluate_cubical_field,
    grid_centers,
    regularize_sites_streaming,
    solve_entropy_gibbs,
)


@dataclass
class KappaPilotRow:
    kappa: float
    s_max: float
    s_q50: float
    s_q90: float
    s_q99: float
    entropy_residual_max: float

    def to_dict(self):
        return self.__dict__.copy()


@dataclass
class PowerCoverHierarchy:
    """Primary output: two compact cubical H0 hierarchies and their contract."""

    config: PowerCoverConfig
    transform: SpatialTransform
    sites: RegularizedSites
    field: CubicalField
    base_forest: CubicalMSFResult
    inner_forest: CubicalMSFResult
    outer_forest: CubicalMSFResult
    report: AccuracyReport
    memory_estimate: MemoryEstimate
    timings: dict[str, float]
    neighbor_audits: dict[str, dict[str, Any]]

    def fusion_intervals(self, *, relative_to_original_knn: bool = True) -> np.ndarray:
        """Return lower/upper radius bounds for every compact MSF fusion.

        The correspondence is between the two uniform-grid forests.  It is not
        a pointwise correspondence between flat cluster labels.
        """

        if self.field.diagnostics.uncertain_queries:
            raise RuntimeError(
                "fusion intervals require every power query to pass the "
                "declared envelope; "
                "this hierarchy contains uncertified spatial queries"
            )

        lower = self.outer_forest.weights.astype(np.float64, copy=False)
        upper = self.inner_forest.weights.astype(np.float64, copy=False)
        if relative_to_original_knn:
            s = float(self.sites.diagnostics.s_max)
            lower = lower - s
            upper = upper + s
        return np.column_stack((np.maximum(lower, 0.0), upper))

    def components_at_cut(self, radius: float) -> tuple[np.ndarray, np.ndarray]:
        """Return inner then outer cubical components at a requested radius."""

        return (
            self.inner_forest.components_at_cut(radius),
            self.outer_forest.components_at_cut(radius),
        )

    def save(self, directory: str, *, include_sites: bool = False) -> None:
        """Persist compact artifacts and JSON metadata on local scratch storage."""

        os.makedirs(directory, exist_ok=True)
        hierarchy_path = os.path.join(directory, "cubical_hierarchy.npz")
        _savez_atomic(
            hierarchy_path,
            compressed=True,
            schema_version=np.asarray(1, dtype=np.int32),
            births=self.base_forest.births,
            edges=self.base_forest.edges,
            weights=self.base_forest.weights,
            dims=np.asarray(self.base_forest.dims, dtype=np.int32),
            half_diagonal=np.asarray(self.field.spec.half_diagonal),
            numerical_radius_error=np.asarray(self.field.numerical_radius_error),
            field_certified=_to_numpy(self.field.certified).astype(np.bool_, copy=False),
            inner_shift=np.asarray(
                self.field.spec.half_diagonal + self.field.numerical_radius_error
            ),
            outer_shift=np.asarray(
                -(self.field.spec.half_diagonal + self.field.numerical_radius_error)
            ),
        )
        artifacts = ["cubical_hierarchy.npz"]
        if include_sites:
            centers = _to_numpy(self.sites.centers)
            additive = _to_numpy(self.sites.additive_weights)
            distortions = _to_numpy(self.sites.distortions)
            _savez_atomic(
                os.path.join(directory, "power_sites.npz"),
                compressed=False,
                schema_version=np.asarray(1, dtype=np.int32),
                centers=centers,
                additive_weights=additive,
                distortions=distortions,
            )
            artifacts.append("power_sites.npz")
        metadata = {
            "schema": "perg_hgp.power_cover_run",
            "schema_version": 1,
            "artifacts": artifacts,
            "config": self.config.to_dict(),
            "transform": self.transform.to_dict(),
            "grid": self.field.spec.to_dict(),
            "regularization": self.sites.diagnostics.to_dict(),
            "power_queries": self.field.diagnostics.to_dict(),
            "accuracy": self.report.to_dict(),
            "memory_estimate": self.memory_estimate.to_dict(),
            "timings_seconds": self.timings,
            "neighbor_audits": self.neighbor_audits,
        }
        path = os.path.join(directory, "run_report.json")
        temporary = path + ".tmp"
        with open(temporary, "w", encoding="utf-8") as stream:
            json.dump(metadata, stream, indent=2, sort_keys=True, allow_nan=False)
            stream.write("\n")
        os.replace(temporary, path)


def _to_numpy(array: Any) -> np.ndarray:
    if type(array).__module__.split(".", 1)[0] == "cupy":
        import cupy as cp  # type: ignore

        return cp.asnumpy(array)
    return np.asarray(array)


def _savez_atomic(path: str, *, compressed: bool, **arrays: Any) -> None:
    """Write an NPZ beside its destination, then atomically publish it."""

    temporary = path + ".tmp.npz"
    writer = np.savez_compressed if compressed else np.savez
    writer(temporary, **arrays)
    os.replace(temporary, path)


def _sample_rows(values: Any, count: int):
    n_rows = int(values.shape[0])
    count = min(n_rows, max(0, int(count)))
    if count == 0:
        return values[:0]
    ids = np.linspace(0, n_rows - 1, count, dtype=np.int64)
    if type(values).__module__.split(".", 1)[0] == "cupy":
        import cupy as cp  # type: ignore

        ids = cp.asarray(ids)
    return values[ids]


def pilot_kappa_calibration(
    points: Any,
    index: Any,
    *,
    m_reg: int,
    candidates: Sequence[float],
    sample_size: int = 8_192,
    distortion_budget: float | None = None,
) -> tuple[float, list[KappaPilotRow]]:
    """Pilot several entropy levels without claiming a global distortion bound.

    The selected value must still be checked against the full-run ``s_max``.
    This routine exists to avoid the unjustified historical rule
    ``kappa=max(32, 3*K)``.
    """

    module = type(points).__module__.split(".", 1)[0]
    if module == "cupy":
        import cupy as xp  # type: ignore
    else:
        xp = np
    sample = _sample_rows(points, sample_size)
    support = min(int(m_reg), int(index.size))
    distances, indices = index.query(sample, support)
    distances = xp.asarray(distances, dtype=points.dtype)
    indices = xp.asarray(indices)
    neighbors = points[indices]
    rows: list[KappaPilotRow] = []
    valid_candidates = sorted({float(value) for value in candidates})
    if not valid_candidates or valid_candidates[0] < 1 or valid_candidates[-1] > support:
        raise ValueError("pilot kappa candidates must lie in [1, m_reg]")

    for kappa in valid_candidates:
        result = solve_entropy_gibbs(distances * distances, kappa)
        q = result.probabilities
        z = xp.sum(q[:, :, None] * neighbors, axis=1)
        offsets = neighbors - z[:, None, :]
        a = xp.sum(q * xp.sum(offsets * offsets, axis=2), axis=1)
        s = xp.sqrt(xp.maximum(xp.sum((sample - z) ** 2, axis=1) + a, 0.0))
        quantiles = xp.quantile(s, xp.asarray((0.5, 0.9, 0.99)))
        rows.append(
            KappaPilotRow(
                kappa=kappa,
                s_max=float(xp.max(s).item()),
                s_q50=float(quantiles[0].item()),
                s_q90=float(quantiles[1].item()),
                s_q99=float(quantiles[2].item()),
                entropy_residual_max=float(xp.max(result.constraint_residual).item()),
            )
        )
    if distortion_budget is None:
        chosen = rows[0].kappa
    else:
        admissible = [row.kappa for row in rows if row.s_max <= distortion_budget]
        if not admissible:
            raise RuntimeError("no pilot kappa satisfies the requested distortion budget")
        chosen = max(admissible)
    return chosen, rows


class PowerCover3D:
    """Build the entropy-regularized K-cover H0 hierarchy on CPU or CUDA.

    CPU mode is an exact small-data reference.  CUDA mode uses RAPIDS RBC with
    a mandatory empirical cuVS audit, a global power guard under the declared
    float32 envelope, and the implicit CuPy Boruvka implementation for the
    26-connected grid.
    """

    def __init__(
        self,
        config: PowerCoverConfig | None = None,
        *,
        backend: str = "cpu",
        distortion_budget: float | None = None,
    ):
        self.config = config or PowerCoverConfig()
        normalized = backend.lower().replace("-", "_")
        if normalized not in {"cpu", "cuda", "cupy", "gpu"}:
            raise ValueError("backend must be cpu or cuda/cupy/gpu")
        self.backend = "cpu" if normalized == "cpu" else "cuda"
        self.distortion_budget = distortion_budget

    def fit(self, points: Any) -> PowerCoverHierarchy:
        config = self.config
        timings: dict[str, float] = {}
        audits: dict[str, dict[str, Any]] = {}
        start_total = time.perf_counter()

        if self.backend == "cpu":
            original_values = np.ascontiguousarray(np.asarray(points))
            transform = _spatial_transform(original_values)
            values = _normalize_points(original_values, transform)
            raw_index = ExactKDTreeIndex(values)
            neighbor_support = raw_index.backend_name
            hardware_manifest: dict[str, Any] = {
                "backend": "cpu_reference",
                "numpy": np.__version__,
            }
        else:
            from .cuda_runtime import (
                RBCAuditedIndex,
                collect_hardware_manifest,
                ensure_memory_budget,
                require_cuda_stack,
            )

            cp, _, _ = require_cuda_stack()
            original_values = cp.ascontiguousarray(cp.asarray(points))
            transform = _spatial_transform(original_values)
            values = _normalize_points(original_values, transform)
            memory = estimate_memory(int(values.shape[0]), config)
            ensure_memory_budget(
                memory.estimated_peak_bytes, fraction=config.max_vram_fraction
            )
            raw_index = RBCAuditedIndex(
                values, max_k=min(config.m_reg, values.shape[0])
            )
            hardware_manifest = collect_hardware_manifest()
            if config.neighbor_audit_queries:
                audit = raw_index.audit_against_bruteforce(
                    _sample_rows(values, config.neighbor_audit_queries),
                    k=min(config.m_reg, int(values.shape[0])),
                )
                audits["raw_rbc_vs_cuvs_bruteforce"] = audit.to_dict()
            audits["raw_rbc_status"] = raw_index.status
            if config.require_neighbor_audit and not raw_index.audited:
                raise RuntimeError("raw-site RBC audit failed; strict run aborted")
            raw_rbc_audited = bool(raw_index.audited)
            neighbor_support = "rapids_rbc_empirical_audit_pending_power_queries"

        n_points = int(values.shape[0])
        if n_points < config.K:
            raise ValueError(f"K={config.K} requires at least K input points")
        memory = estimate_memory(n_points, config)

        start = time.perf_counter()
        sites = regularize_sites_streaming(
            values,
            raw_index,
            m_reg=config.m_reg,
            kappa=config.kappa,
            chunk_size=config.chunk_size,
        )
        _synchronize(self.backend)
        timings["regularization"] = time.perf_counter() - start
        s_original = sites.diagnostics.s_max * transform.scale
        if self.distortion_budget is not None and s_original > self.distortion_budget:
            raise RuntimeError(
                f"full-run distortion s={s_original:.6g} exceeds "
                f"budget {self.distortion_budget:.6g}; lower kappa and rerun"
            )
        del raw_index, values, original_values
        _release_cached_gpu_memory(self.backend)

        spec = build_grid_spec(sites.centers, config.grid_shape)
        start = time.perf_counter()
        if self.backend == "cpu":
            site_index = ExactKDTreeIndex(sites.centers)
        else:
            from .cuda_runtime import RBCAuditedIndex

            site_index = RBCAuditedIndex(
                sites.centers,
                max_k=min(n_points, config.candidate_k_max + 1),
            )
            if config.neighbor_audit_queries:
                audit_count = min(config.neighbor_audit_queries, spec.n_cubes)
                flat = cp.linspace(
                    0, spec.n_cubes - 1, audit_count, dtype=cp.int64
                )
                audit_queries = grid_centers(
                    spec, flat, xp=cp, dtype=sites.centers.dtype
                )
                audit = site_index.audit_against_bruteforce(
                    audit_queries,
                    k=min(config.candidate_k_max + 1, n_points),
                )
                audits["power_grid_centers_rbc_vs_cuvs_bruteforce"] = audit.to_dict()
                del flat, audit_queries
            audits["power_rbc_status"] = site_index.status
            if config.require_neighbor_audit and not site_index.audited:
                raise RuntimeError("power-site RBC audit failed; strict run aborted")
            neighbor_support = (
                "rapids_rbc_empirically_audited_on_raw_and_grid_samples"
                if raw_rbc_audited and site_index.audited
                else "rapids_rbc_empirical_audit_incomplete_or_failed"
            )
        timings["power_index"] = time.perf_counter() - start

        oracle = CertifiedPowerKNN(
            sites.centers,
            sites.additive_weights,
            site_index,
            K=config.K,
            candidate_k_initial=config.candidate_k_initial,
            candidate_k_max=config.candidate_k_max,
            numerical_margin_factor=config.numerical_margin_factor,
            strict=config.strict_certification,
        )
        start = time.perf_counter()
        field = evaluate_cubical_field(
            oracle, spec, chunk_size=config.power_chunk_size
        )
        _synchronize(self.backend)
        timings["power_field"] = time.perf_counter() - start
        del oracle, site_index
        _release_cached_gpu_memory(self.backend)

        start = time.perf_counter()
        base_forest_normalized = build_cubical_msf(
            field.radii,
            dims=spec.shape,
            backend="cpu" if self.backend == "cpu" else "cuda",
        )
        # Uniform cubes have a constant H.  The two certified filtrations have
        # identical topology and differ only by this radius translation.
        sites = _denormalize_sites(sites, transform)
        field = _denormalize_field(field, transform)
        base_forest = _rescale_forest(base_forest_normalized, transform.scale)
        effective_half_width = (
            field.spec.half_diagonal + field.numerical_radius_error
        )
        inner_forest = base_forest.shifted(effective_half_width)
        outer_forest = base_forest.shifted(-effective_half_width)
        _synchronize(self.backend)
        timings["cubical_msf"] = time.perf_counter() - start

        spatial_enveloped = field.diagnostics.uncertain_queries == 0
        report = AccuracyReport(
            mode="spatial_enveloped" if spatial_enveloped else "spatial_uncertain",
            neighbor_support=neighbor_support,
            entropy_residual_max=sites.diagnostics.entropy_residual_max,
            regularization_interleaving_radius=sites.diagnostics.s_max,
            power_knn_certified_fraction=field.diagnostics.certified_fraction,
            power_knn_uncertain_count=field.diagnostics.uncertain_queries,
            numerical_radius_error=field.numerical_radius_error,
            spatial_interleaving_radius=(
                2.0 * effective_half_width if spatial_enveloped else None
            ),
            total_interleaving_radius=(
                sites.diagnostics.s_max + 2.0 * effective_half_width
                if spatial_enveloped
                else None
            ),
            cubical_connectivity=26,
            cubical_msf_exact_for_stored_float32_values=True,
            flat_selection="none",
            atlas_completeness="not_used",
            full_power_hgp_complete=False,
            K_fixed_finite_sample_resolution=True,
            hardware_manifest=hardware_manifest,
            notes=[
                "Primary output is the inner/outer cubical H0 hierarchy.",
                "s+2(H+delta_num) is the published radius/H0 interleaving "
                "bound, not a flat-label guarantee.",
                "Power top-K certification is conditional on the declared "
                "float32 error envelope; directed interval arithmetic remains future work.",
                "Canonical site-to-component coverage is not replaced by cell lookup.",
                "K fixed at finite sample size is not an asymptotic consistency claim.",
                "Uniform-grid inner and outer forests share one MSF topology.",
                "Geometry was isotropically recentered/scaled; public radii use input units.",
                "The cubical envelope includes an explicit float32 radius margin.",
                *(
                    []
                    if spatial_enveloped
                    else [
                        "Uncertified power queries remain: the stored forests are "
                        "diagnostic approximations, not certified interleaving bounds."
                    ]
                ),
            ],
        )
        timings["total"] = time.perf_counter() - start_total
        return PowerCoverHierarchy(
            config=config,
            transform=transform,
            sites=sites,
            field=field,
            base_forest=base_forest,
            inner_forest=inner_forest,
            outer_forest=outer_forest,
            report=report,
            memory_estimate=memory,
            timings=timings,
            neighbor_audits=audits,
        )


def _spatial_transform(values: Any) -> SpatialTransform:
    if values.ndim != 2 or values.shape[1] != 3 or values.shape[0] < 1:
        raise ValueError("points must have shape (N, 3), N >= 1")
    if values.dtype.kind not in "fiu":
        raise TypeError("points must contain real numeric coordinates")
    module = type(values).__module__.split(".", 1)[0]
    if module == "cupy":
        import cupy as xp  # type: ignore
    else:
        xp = np
    if not bool(xp.all(xp.isfinite(values)).item()):
        raise ValueError("points contain NaN or Inf")
    lower = xp.min(values, axis=0)
    upper = xp.max(values, axis=0)
    lower_values = tuple(float(item.item()) for item in lower)
    upper_values = tuple(float(item.item()) for item in upper)
    scale = max(high - low for low, high in zip(lower_values, upper_values))
    if scale == 0.0:
        scale = 1.0
    return SpatialTransform(
        offset=lower_values,
        scale=scale,
    )


def _normalize_points(values: Any, transform: SpatialTransform):
    module = type(values).__module__.split(".", 1)[0]
    if module == "cupy":
        import cupy as xp  # type: ignore
    else:
        xp = np
    # Recenter in at least the precision of the input, then cast the bounded
    # normalized geometry to float32.  Casting a float64 cloud around 1e9
    # before subtraction would irreversibly collapse small spatial offsets.
    use_float64 = values.dtype.kind in "iu" or values.dtype.itemsize > 4
    working_dtype = xp.float64 if use_float64 else xp.float32
    working = xp.asarray(values, dtype=working_dtype)
    offset = xp.asarray(transform.offset, dtype=working_dtype)
    normalized = (working - offset[None, :]) / transform.scale
    return xp.ascontiguousarray(normalized, dtype=xp.float32)


def _denormalize_sites(
    sites: RegularizedSites, transform: SpatialTransform
) -> RegularizedSites:
    module = type(sites.centers).__module__.split(".", 1)[0]
    if module == "cupy":
        import cupy as xp  # type: ignore
    else:
        xp = np
    # Public absolute centers need float64: a float32 array cannot represent
    # sub-unit offsets around a translation of order 1e9.
    offset = xp.asarray(transform.offset, dtype=xp.float64)
    centers = (
        xp.asarray(sites.centers, dtype=xp.float64) * transform.scale
        + offset[None, :]
    )
    additive = sites.additive_weights * (transform.scale**2)
    distortions = sites.distortions * transform.scale
    diagnostics = RegularizationDiagnostics(
        entropy_residual_max=sites.diagnostics.entropy_residual_max,
        entropy_target_deviation_max=sites.diagnostics.entropy_target_deviation_max,
        simplex_residual_max=sites.diagnostics.simplex_residual_max,
        s_max=sites.diagnostics.s_max * transform.scale,
        s_quantiles={
            key: value * transform.scale
            for key, value in sites.diagnostics.s_quantiles.items()
        },
        query_count=sites.diagnostics.query_count,
        s_quantile_sample_size=sites.diagnostics.s_quantile_sample_size,
    )
    return RegularizedSites(centers, additive, distortions, diagnostics)


def _denormalize_field(field: CubicalField, transform: SpatialTransform) -> CubicalField:
    offset = transform.offset
    spec = GridSpec(
        bbox_min=tuple(
            offset[axis] + transform.scale * field.spec.bbox_min[axis]
            for axis in range(3)
        ),
        bbox_max=tuple(
            offset[axis] + transform.scale * field.spec.bbox_max[axis]
            for axis in range(3)
        ),
        shape=field.spec.shape,
        cell_widths=tuple(width * transform.scale for width in field.spec.cell_widths),
        half_diagonal=field.spec.half_diagonal * transform.scale,
    )
    diagnostics = PowerQueryDiagnostics(
        total_queries=field.diagnostics.total_queries,
        certified_queries=field.diagnostics.certified_queries,
        uncertain_queries=field.diagnostics.uncertain_queries,
        candidate_histogram=field.diagnostics.candidate_histogram.copy(),
        min_certificate_gap=(
            None
            if field.diagnostics.min_certificate_gap is None
            else field.diagnostics.min_certificate_gap * (transform.scale**2)
        ),
        radius_error_max=field.diagnostics.radius_error_max * transform.scale,
        numerical_certificate=field.diagnostics.numerical_certificate,
    )
    return CubicalField(
        spec=spec,
        radii=field.radii * transform.scale,
        numerical_radius_error=field.numerical_radius_error * transform.scale,
        inner_activation=field.inner_activation * transform.scale,
        outer_activation=field.outer_activation * transform.scale,
        certified=field.certified,
        diagnostics=diagnostics,
    )


def _rescale_forest(forest: CubicalMSFResult, scale: float) -> CubicalMSFResult:
    return CubicalMSFResult(
        births=forest.births * scale,
        edges=forest.edges,
        weights=forest.weights * scale,
        dims=forest.dims,
    )


def _synchronize(backend: str) -> None:
    if backend == "cuda":
        import cupy as cp  # type: ignore

        cp.cuda.get_current_stream().synchronize()


def _release_cached_gpu_memory(backend: str) -> None:
    if backend == "cuda":
        import cupy as cp  # type: ignore

        cp.get_default_memory_pool().free_all_blocks()


__all__ = [
    "KappaPilotRow",
    "PowerCover3D",
    "PowerCoverHierarchy",
    "pilot_kappa_calibration",
]
