"""End-to-end API for the spatial, entropy-regularized PERG-HGP backend."""

from __future__ import annotations

from dataclasses import dataclass
import json
import math
import os
import time
from typing import Any, Iterable, Sequence, cast
import uuid

import numpy as np

from .cuda_runtime import RBC_MAX_QUERY_K
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
from .progress import ProgressCallback, emit_progress
from .spatial_core import (
    CertifiedPowerKNN,
    CubicalField,
    ExactKDTreeIndex,
    ExactLiftedPowerKNN,
    RegularizedSites,
    build_grid_spec,
    build_grid_spec_for_local_scale,
    evaluate_cubical_field,
    grid_centers,
    regularize_sites_streaming,
    solve_entropy_gibbs,
)

_FIT_PROGRESS_STAGES = (
    "input",
    "raw_index",
    "regularization",
    "power_index",
    "power_field",
    "cubical_msf",
)


class NeighborAuditError(RuntimeError):
    """Strict CUDA failure carrying the empirical evidence collected so far."""

    def __init__(self, stage: str, audits: Any):
        self.stage = str(stage)

        def _sanitize(val: Any) -> Any:
            if isinstance(val, dict):
                return {str(k): _sanitize(v) for k, v in val.items()}
            if isinstance(val, (list, tuple)):
                return [_sanitize(v) for v in val]
            if isinstance(val, np.ndarray):
                return _sanitize(val.tolist())
            if isinstance(val, np.integer):
                return int(val)
            if isinstance(val, np.floating):
                return float(val) if np.isfinite(val) else repr(float(val))
            if isinstance(val, np.bool_):
                return bool(val)
            if isinstance(val, float) and not np.isfinite(val):
                return repr(val)
            return val

        self.neighbor_audits = _sanitize(audits) if isinstance(audits, dict) else {"raw_audits": _sanitize(audits)}
        super().__init__(
            f"{self.stage}-site neighbor evidence is insufficient; strict run "
            "aborted; collected audits="
            + json.dumps(self.neighbor_audits, sort_keys=True)
        )


class _FitProgress:
    """Emit consistent stage events while keeping display policy in the caller."""

    def __init__(self, callback: ProgressCallback | None):
        self.callback = callback
        self.started: dict[str, float] = {}

    def _details(
        self, stage: str, extra: dict[str, Any] | None = None
    ) -> dict[str, Any]:
        details: dict[str, Any] = {
            "stage_index": int(_FIT_PROGRESS_STAGES.index(stage) + 1),
            "stage_count": int(len(_FIT_PROGRESS_STAGES)),
        }
        if extra:
            details.update(extra)
        return details

    def start(
        self,
        stage: str,
        *,
        total: int,
        unit: str,
        details: dict[str, Any] | None = None,
    ) -> float:
        started = time.perf_counter()
        self.started[stage] = started
        emit_progress(
            self.callback,
            stage=stage,
            kind="start",
            completed=0,
            total=total,
            unit=unit,
            started_at=started,
            details=self._details(stage, details),
        )
        return started

    def message(self, stage: str, operation: str) -> None:
        emit_progress(
            self.callback,
            stage=stage,
            kind="message",
            started_at=self.started.get(stage),
            details=self._details(stage, {"operation": str(operation)}),
        )

    @property
    def helper_callback(self) -> ProgressCallback | None:
        return self._forward_helper_event if self.callback is not None else None

    def _forward_helper_event(self, event: dict[str, Any]) -> None:
        """Align helper timing/details with the enclosing fit stage."""

        stage = str(event["stage"])
        forwarded = event.copy()
        started = self.started.get(stage)
        if started is not None:
            forwarded["elapsed_seconds"] = float(
                max(0.0, time.perf_counter() - started)
            )
        forwarded["details"] = self._details(stage, dict(event.get("details", {})))
        callback = self.callback
        if callback is not None:
            callback(forwarded)

    def done(
        self,
        stage: str,
        *,
        total: int,
        unit: str,
        details: dict[str, Any] | None = None,
    ) -> None:
        emit_progress(
            self.callback,
            stage=stage,
            kind="done",
            completed=total,
            total=total,
            unit=unit,
            started_at=self.started.get(stage),
            details=self._details(stage, details),
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
            shift = float(self.sites.diagnostics.s_max) + float(
                self.report.input_quantization_radius or 0.0
            )
            lower = lower - shift
            upper = upper + shift
        return np.column_stack((np.maximum(lower, 0.0), upper))

    def accuracy_at_scale(self, radius: float) -> dict[str, float | None]:
        """Return additive and relative H0 contracts at a physical radius.

        The relative values are simply the certified additive shifts divided
        by the requested local filtration scale; they do not turn an additive
        interleaving into a label guarantee.
        """

        scale = float(radius)
        if not math.isfinite(scale) or scale <= 0.0:
            raise ValueError("radius must be finite and strictly positive")
        base = self.report.base_total_interleaving_radius
        envelope = self.report.total_interleaving_radius
        return {
            "radius": scale,
            "base_additive_radius": base,
            "envelope_additive_radius": envelope,
            "base_relative_radius": None if base is None else base / scale,
            "envelope_relative_radius": (
                None if envelope is None else envelope / scale
            ),
            "regularization_multiplicative_lower": (
                self.report.regularization_multiplicative_lower
            ),
            "regularization_multiplicative_upper": (
                self.report.regularization_multiplicative_upper
            ),
        }

    def components_at_cut(self, radius: float) -> tuple[np.ndarray, np.ndarray]:
        """Return inner then outer cubical components at a requested radius."""

        return (
            self.inner_forest.components_at_cut(radius),
            self.outer_forest.components_at_cut(radius),
        )

    def site_components_at_cut(
        self,
        radius: float,
        *,
        site_start: int = 0,
        site_stop: int | None = None,
        max_sites: int = 1_000_000,
        max_candidate_cells_per_site: int = 100_000,
        max_active_cells: int = 5_000_000,
        max_total_cells: int = 5_000_000,
    ):
        """Return canonical power-ball/component relations for a site slice.

        This is the exact CPU reference for the stored inner/outer cubical
        unions.  Slicing makes the contract usable out of core; a full 30 M
        relation still requires the planned two-pass GPU implementation.
        """

        from .coverage import site_component_membership_cpu

        if int(self.sites.centers.shape[0]) == 0:
            raise RuntimeError(
                "site coverage requires an artifact saved with include_sites=True"
            )

        return site_component_membership_cpu(
            self.sites.centers,
            self.sites.additive_weights,
            self.field.spec,
            self.inner_forest,
            self.outer_forest,
            radius=radius,
            site_start=site_start,
            site_stop=site_stop,
            max_sites=max_sites,
            max_candidate_cells_per_site=max_candidate_cells_per_site,
            max_active_cells=max_active_cells,
            max_total_cells=max_total_cells,
        )

    def labels_at_cut(
        self,
        radius: float,
        min_cluster_size: int = 1,
        policy: str = "confirmed_unique",
        *,
        max_sites: int = 1_000_000,
        max_candidate_cells_per_site: int = 100_000,
        max_active_cells: int = 5_000_000,
        max_total_cells: int = 5_000_000,
    ) -> np.ndarray:
        """Return conservative flat labels for all stored sites at one cut.

        ``confirmed_unique`` assigns a site only when the canonical
        ball--AABB relation returned by :meth:`site_components_at_cut` has
        status ``CONFIRMED`` *and* its outer relation contains exactly one
        component.  All other sites are labelled ``-1``.  Components with
        fewer than ``min_cluster_size`` assigned sites are then discarded and
        the survivors are recoded contiguously in increasing component-ID
        order.

        This is strict, deterministic post-processing of the stored cubical
        inner/outer relations, not a certified flat-partition theorem or a
        persistence-based cluster selection.  It intentionally uses the full
        site set so that ``min_cluster_size`` is global.  The implementation
        inherits the bounded CPU coverage limits below; in particular, it is
        not a viable 30-million-site path without a GPU ball--AABB backend.
        """

        if isinstance(min_cluster_size, (bool, np.bool_)) or not isinstance(
            min_cluster_size, (int, np.integer)
        ):
            raise TypeError("min_cluster_size must be an integer")
        minimum = int(min_cluster_size)
        if minimum < 1:
            raise ValueError("min_cluster_size must be positive")
        if not isinstance(policy, str):
            raise TypeError("policy must be a string")
        normalized_policy = policy.strip().lower().replace("-", "_")
        if normalized_policy != "confirmed_unique":
            raise ValueError("policy must be 'confirmed_unique'")

        from .coverage import CONFIRMED

        membership = self.site_components_at_cut(
            radius,
            site_start=0,
            site_stop=None,
            max_sites=max_sites,
            max_candidate_cells_per_site=max_candidate_cells_per_site,
            max_active_cells=max_active_cells,
            max_total_cells=max_total_cells,
        )
        labels = np.full(membership.n_sites, -1, dtype=np.int32)
        outer_lengths = membership.outer_indptr[1:] - membership.outer_indptr[:-1]
        assigned_rows = np.flatnonzero(
            (membership.status == CONFIRMED) & (outer_lengths == 1)
        )
        if assigned_rows.size == 0:
            return labels

        component_ids = membership.outer_labels[
            membership.outer_indptr[assigned_rows]
        ]
        components, inverse, counts = np.unique(
            component_ids, return_inverse=True, return_counts=True
        )
        surviving_components = components[counts >= minimum]
        keep = counts[inverse] >= minimum
        if np.any(keep):
            labels[assigned_rows[keep]] = np.searchsorted(
                surviving_components, component_ids[keep]
            ).astype(np.int32, copy=False)
        return labels

    def save(self, directory: str, *, include_sites: bool = False) -> None:
        """Persist compact artifacts and JSON metadata on local scratch storage."""

        os.makedirs(directory, exist_ok=True)
        run_id = uuid.uuid4().hex
        hierarchy_path = os.path.join(directory, "cubical_hierarchy.npz")
        _savez_atomic(
            hierarchy_path,
            compressed=True,
            schema_version=np.asarray(3, dtype=np.int32),
            run_id=np.asarray(run_id),
            births=self.base_forest.births,
            edges=self.base_forest.edges,
            weights=self.base_forest.weights,
            dims=np.asarray(self.base_forest.dims, dtype=np.int32),
            half_diagonal=np.asarray(self.field.spec.half_diagonal),
            numerical_radius_error=np.asarray(self.field.numerical_radius_error),
            input_quantization_radius=np.asarray(
                self.report.input_quantization_radius or 0.0
            ),
            field_certified=_to_numpy(self.field.certified).astype(
                np.bool_, copy=False
            ),
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
                schema_version=np.asarray(3, dtype=np.int32),
                run_id=np.asarray(run_id),
                centers=centers,
                additive_weights=additive,
                distortions=distortions,
                effective_kappa=(
                    np.asarray([], dtype=np.float32)
                    if self.sites.effective_kappa is None
                    else _to_numpy(self.sites.effective_kappa)
                ),
            )
            artifacts.append("power_sites.npz")
        metadata = {
            "schema": "perg_hgp.power_cover_run",
            "schema_version": 3,
            "run_id": run_id,
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
        if not include_sites:
            # The JSON manifest is authoritative.  Remove an optional artifact
            # left by an earlier save only after publishing the new manifest,
            # so an interrupted update can at worst leave an ignored file.
            try:
                os.remove(os.path.join(directory, "power_sites.npz"))
            except FileNotFoundError:
                pass

    @classmethod
    def load(cls, directory: str) -> "PowerCoverHierarchy":
        """Load a schema-v2 or schema-v3 hierarchy saved by :meth:`save`.

        Runs saved without sites still support cuts, fusion intervals and
        accuracy queries.  Observation coverage explicitly requires the
        optional ``power_sites.npz`` artifact.
        """

        report_path = os.path.join(directory, "run_report.json")
        hierarchy_path = os.path.join(directory, "cubical_hierarchy.npz")
        with open(report_path, "r", encoding="utf-8") as stream:
            metadata = json.load(stream)
        if metadata.get("schema") != "perg_hgp.power_cover_run":
            raise ValueError("not a perg_hgp power-cover run directory")
        schema_version = int(metadata.get("schema_version", -1))
        if schema_version not in {2, 3}:
            raise ValueError("PowerCoverHierarchy.load requires schema version 2 or 3")
        run_id = metadata.get("run_id")
        if (
            not isinstance(run_id, str)
            or len(run_id) != 32
            or any(character not in "0123456789abcdef" for character in run_id)
        ):
            raise ValueError("run metadata has an invalid generation identifier")
        raw_artifacts = metadata.get("artifacts")
        if (
            not isinstance(raw_artifacts, list)
            or any(not isinstance(name, str) for name in raw_artifacts)
            or len(set(raw_artifacts)) != len(raw_artifacts)
        ):
            raise ValueError("run artifact manifest must be a list of unique names")
        artifacts = set(raw_artifacts)
        if "cubical_hierarchy.npz" not in artifacts:
            raise ValueError("run artifact manifest omits cubical_hierarchy.npz")
        if not os.path.isfile(hierarchy_path):
            raise FileNotFoundError("manifested cubical_hierarchy.npz is missing")

        config = PowerCoverConfig(**metadata["config"])
        transform = SpatialTransform(**metadata["transform"])
        spec = GridSpec(**metadata["grid"])
        regularization = RegularizationDiagnostics(**metadata["regularization"])
        power_payload = dict(metadata["power_queries"])
        power_payload.pop("certified_fraction", None)
        power_payload.pop("conditional_guard_pass_fraction", None)
        power_diagnostics = PowerQueryDiagnostics(**power_payload)
        accuracy = AccuracyReport(**metadata["accuracy"])
        memory_payload = dict(metadata["memory_estimate"])
        memory_payload["assumptions"] = tuple(memory_payload["assumptions"])
        memory = MemoryEstimate(**memory_payload)

        with np.load(hierarchy_path, allow_pickle=False) as artifact:
            if int(artifact["schema_version"]) != schema_version:
                raise ValueError("cubical hierarchy artifact has an unsupported schema")
            if str(np.asarray(artifact["run_id"]).item()) != run_id:
                raise ValueError(
                    "cubical hierarchy artifact belongs to a different save generation"
                )
            births = np.asarray(artifact["births"])
            edges = np.asarray(artifact["edges"])
            weights = np.asarray(artifact["weights"])
            dims = tuple(int(value) for value in artifact["dims"])
            certified = np.asarray(artifact["field_certified"], dtype=np.bool_)
            numerical_error = float(artifact["numerical_radius_error"])
            inner_shift = float(artifact["inner_shift"])
            outer_shift = float(artifact["outer_shift"])
            stored_half_diagonal = float(artifact["half_diagonal"])
            stored_quantization = float(artifact["input_quantization_radius"])

        expected_vertices = math.prod(spec.shape)
        expected_shift = spec.half_diagonal + numerical_error
        if (
            dims != spec.shape
            or births.shape != (expected_vertices,)
            or certified.shape != (expected_vertices,)
            or power_diagnostics.total_queries != expected_vertices
            or power_diagnostics.certified_queries + power_diagnostics.uncertain_queries
            != expected_vertices
            or not math.isclose(
                stored_half_diagonal, spec.half_diagonal, rel_tol=1e-15, abs_tol=0.0
            )
            or not math.isclose(inner_shift, expected_shift, rel_tol=1e-15, abs_tol=0.0)
            or not math.isclose(
                outer_shift, -expected_shift, rel_tol=1e-15, abs_tol=0.0
            )
            or accuracy.numerical_radius_error is None
            or not math.isclose(
                numerical_error,
                accuracy.numerical_radius_error,
                rel_tol=1e-15,
                abs_tol=0.0,
            )
            or accuracy.input_quantization_radius is None
            or not math.isclose(
                stored_quantization,
                accuracy.input_quantization_radius,
                rel_tol=1e-15,
                abs_tol=0.0,
            )
        ):
            raise ValueError("cubical artifact and JSON manifest are inconsistent")

        base_forest = CubicalMSFResult(births, edges, weights, dims)
        inner_forest = base_forest.shifted(inner_shift)
        outer_forest = base_forest.shifted(outer_shift)
        field = CubicalField(
            spec=spec,
            radii=births.copy(),
            numerical_radius_error=numerical_error,
            inner_activation=births + inner_shift,
            outer_activation=births + outer_shift,
            certified=certified,
            diagnostics=power_diagnostics,
        )

        sites_path = os.path.join(directory, "power_sites.npz")
        if "power_sites.npz" in artifacts:
            if not os.path.isfile(sites_path):
                raise FileNotFoundError("manifested power_sites.npz is missing")
            with np.load(sites_path, allow_pickle=False) as artifact:
                if int(artifact["schema_version"]) != schema_version:
                    raise ValueError("power-sites artifact has an unsupported schema")
                if str(np.asarray(artifact["run_id"]).item()) != run_id:
                    raise ValueError(
                        "power-sites artifact belongs to a different save generation"
                    )
                centers = np.asarray(artifact["centers"])
                additive = np.asarray(artifact["additive_weights"])
                distortions = np.asarray(artifact["distortions"])
                stored_kappa = np.asarray(artifact["effective_kappa"])
                effective_kappa = stored_kappa if stored_kappa.size else None
            n_sites = int(centers.shape[0]) if centers.ndim == 2 else -1
            if (
                centers.shape != (n_sites, 3)
                or additive.shape != (n_sites,)
                or distortions.shape != (n_sites,)
                or (effective_kappa is not None and effective_kappa.shape != (n_sites,))
                or not np.all(np.isfinite(centers))
                or not np.all(np.isfinite(additive))
                or not np.all(np.isfinite(distortions))
                or (
                    effective_kappa is not None
                    and not np.all(np.isfinite(effective_kappa))
                )
                or np.any(additive < 0.0)
                or np.any(distortions < 0.0)
                or n_sites != regularization.query_count
            ):
                raise ValueError("power-sites artifact is inconsistent or non-finite")
        else:
            centers = np.empty((0, 3), dtype=np.float64)
            additive = np.empty(0, dtype=np.float64)
            distortions = np.empty(0, dtype=np.float64)
            effective_kappa = None
        sites = RegularizedSites(
            centers=centers,
            additive_weights=additive,
            distortions=distortions,
            diagnostics=regularization,
            effective_kappa=effective_kappa,
            local_scales=None,
        )
        return cls(
            config=config,
            transform=transform,
            sites=sites,
            field=field,
            base_forest=base_forest,
            inner_forest=inner_forest,
            outer_forest=outer_forest,
            report=accuracy,
            memory_estimate=memory,
            timings={
                str(key): float(value)
                for key, value in metadata["timings_seconds"].items()
            },
            neighbor_audits=dict(metadata["neighbor_audits"]),
        )


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


class _AnalyticIdentityRegularizationIndex:
    """Size contract for the exact kappa=1 regularization fast path."""

    backend_name = "analytic_identity_kappa_one_no_raw_knn"

    def __init__(self, size: int):
        self.size = int(size)

    def query(self, queries: Any, k: int):  # pragma: no cover - defensive
        raise AssertionError("the analytic kappa=1 path must not query an index")


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
    if (
        not valid_candidates
        or valid_candidates[0] < 1
        or valid_candidates[-1] > support
    ):
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
            raise RuntimeError(
                "no pilot kappa satisfies the requested distortion budget"
            )
        chosen = max(admissible)
    return chosen, rows


class PowerCover3D:
    """Build the entropy-regularized K-cover H0 hierarchy on CPU or CUDA.

    CPU mode is an exact small-data reference.  CUDA mode uses RAPIDS RBC with
    a mandatory empirical cuVS audit, a global power guard under the declared
    float32 envelope, and the implicit CuPy Boruvka implementation for the
    26-connected grid.  The optional ``progress`` callback receives
    synchronous, JSON-safe event dictionaries; callback exceptions are
    intentionally propagated.
    """

    def __init__(
        self,
        config: PowerCoverConfig | None = None,
        *,
        backend: str = "cpu",
        distortion_budget: float | None = None,
        progress: ProgressCallback | None = None,
    ):
        self.config = config or PowerCoverConfig()
        normalized = backend.lower().replace("-", "_")
        if normalized not in {"cpu", "cuda", "cupy", "gpu"}:
            raise ValueError("backend must be cpu or cuda/cupy/gpu")
        self.backend = "cpu" if normalized == "cpu" else "cuda"
        if distortion_budget is not None:
            if isinstance(distortion_budget, (bool, np.bool_)) or not isinstance(
                distortion_budget, (int, float, np.integer, np.floating)
            ):
                raise TypeError("distortion_budget must be a finite real or None")
            if (
                not math.isfinite(float(distortion_budget))
                or float(distortion_budget) < 0.0
            ):
                raise ValueError("distortion_budget must be finite and non-negative")
            if self.config.regularization_mode == "local_distortion":
                raise ValueError(
                    "legacy full-run distortion_budget cannot be combined with "
                    "regularization_mode='local_distortion'; use the config budgets"
                )
        self.distortion_budget = (
            None if distortion_budget is None else float(distortion_budget)
        )
        if progress is not None and not callable(progress):
            raise TypeError("progress must be callable or None")
        self.progress = progress

    def fit(self, points: Any) -> PowerCoverHierarchy:
        config = self.config
        timings: dict[str, float] = {}
        audits: dict[str, dict[str, Any]] = {}
        start_total = time.perf_counter()
        progress = _FitProgress(self.progress)

        progress.start(
            "input",
            total=1,
            unit="stage",
            details={"backend": self.backend},
        )
        raw_index: Any
        if self.backend == "cpu":
            original_values = np.ascontiguousarray(np.asarray(points))
            transform = _spatial_transform(original_values)
            values = _normalize_points(original_values, transform)
            hardware_manifest: dict[str, Any] = {
                "backend": "cpu_reference",
                "numpy": np.__version__,
            }
        else:
            from .cuda_runtime import (
                CuvsBruteForceIndex,
                RBCAuditedIndex,
                collect_hardware_manifest,
                ensure_memory_budget,
                require_cuda_stack,
            )

            cp, _, _ = require_cuda_stack()
            original_values = cp.ascontiguousarray(cp.asarray(points))
            transform = _spatial_transform(original_values)
            values = _normalize_points(original_values, transform)

        n_points = int(values.shape[0])
        input_coordinate_bytes = int(original_values.dtype.itemsize)
        if n_points < config.K:
            raise ValueError(f"K={config.K} requires at least K input points")
        input_quantization_radius = _input_quantization_radius(
            original_values,
            values,
            transform,
            chunk_size=config.chunk_size,
        )
        if config.min_resolved_radius is None:
            preliminary_spec = build_grid_spec(values, config.grid_shape)
        else:
            preliminary_spec = build_grid_spec_for_local_scale(
                values,
                min_resolved_radius=config.min_resolved_radius / transform.scale,
                max_relative_spatial_error=config.max_relative_spatial_error,
                max_cells=config.max_grid_cells,
            )
        memory = estimate_memory(
            n_points,
            config,
            input_coordinate_bytes=input_coordinate_bytes,
            n_cubes=preliminary_spec.n_cubes,
        )
        if self.backend == "cuda":
            ensure_memory_budget(
                memory.estimated_peak_bytes, fraction=config.max_vram_fraction
            )
        progress.done(
            "input",
            total=1,
            unit="stage",
            details={"points": n_points},
        )

        progress.start(
            "raw_index",
            total=1,
            unit="stage",
            details={"points": n_points},
        )
        oracle: Any
        analytic_identity = (
            config.regularization_mode == "fixed_kappa" and config.kappa == 1.0
        )
        if analytic_identity:
            raw_index = _AnalyticIdentityRegularizationIndex(n_points)
            neighbor_support = raw_index.backend_name
            audits["raw_neighbor_status"] = {
                "backend": raw_index.backend_name,
                "analytically_exact": True,
                "knn_query_required": False,
                "reason": (
                    "entropy target log(kappa)=0 plus the self-neighbor "
                    "convention gives z=x and a=s=0"
                ),
            }
            if self.backend == "cuda":
                hardware_manifest = collect_hardware_manifest()
            raw_neighbor_supported = True
            progress.message("raw_index", "skip_analytic_identity_kappa_one")
        elif self.backend == "cpu":
            raw_index = ExactKDTreeIndex(values)
            neighbor_support = raw_index.backend_name
        else:
            raw_query_k = max(
                config.m_reg,
                (
                    config.local_scale_k
                    if config.regularization_mode == "local_distortion"
                    and config.distortion_budget_relative is not None
                    and config.local_scale_k is not None
                    else config.m_reg
                ),
            )
            raw_query_k = min(raw_query_k, n_points)
            raw_backend = _cuda_raw_index_backend(n_points, raw_query_k)
            progress.message(
                "raw_index",
                "build_rapids_rbc"
                if raw_backend == "rbc"
                else "build_cuvs_brute_force_index",
            )
            raw_index_class = (
                RBCAuditedIndex
                if raw_backend == "rbc"
                else CuvsBruteForceIndex
            )
            raw_index = raw_index_class(values, max_k=raw_query_k)
            hardware_manifest = collect_hardware_manifest()
            raw_exhaustive = bool(
                getattr(raw_index, "algorithmically_exhaustive", False)
            )
            if config.neighbor_audit_queries and not raw_exhaustive:
                progress.message("raw_index", "audit_against_cuvs_bruteforce")
                audit = raw_index.audit_against_bruteforce(
                    _sample_rows(values, config.neighbor_audit_queries),
                    k=min(raw_query_k, n_points),
                    require_support_equivalence=True,
                )
                audits["raw_rbc_vs_cuvs_bruteforce"] = audit.to_dict()
            audits["raw_neighbor_status"] = raw_index.status
            raw_neighbor_supported = raw_exhaustive or bool(raw_index.audited)
            if config.require_neighbor_audit and not raw_neighbor_supported:
                raise NeighborAuditError("raw", audits)
            neighbor_support = (
                "cuvs_bruteforce_exhaustive_raw_pending_power_queries"
                if raw_exhaustive
                else "rapids_rbc_empirical_audit_pending_power_queries"
            )
        progress.done(
            "raw_index",
            total=1,
            unit="stage",
            details={
                "audit_queries": (
                    0 if analytic_identity else int(config.neighbor_audit_queries)
                ),
                "analytic_identity": analytic_identity,
            },
        )

        start = progress.start("regularization", total=n_points, unit="points")
        sites = regularize_sites_streaming(
            values,
            raw_index,
            m_reg=config.m_reg,
            kappa=config.kappa,
            chunk_size=config.chunk_size,
            mode=config.regularization_mode,
            distortion_budget_absolute=(
                None
                if config.distortion_budget_absolute is None
                else config.distortion_budget_absolute / transform.scale
            ),
            distortion_budget_relative=config.distortion_budget_relative,
            local_scale_k=config.local_scale_k,
            progress=progress.helper_callback,
        )
        _synchronize(self.backend)
        (
            stored_s_upper,
            stored_gamma_upper,
            relative_contract_status,
            zero_scale_violations,
            local_scale_below_resolution_count,
        ) = _stored_site_contract_upper(
            values,
            sites,
            chunk_size=config.chunk_size,
            relative_requested=config.distortion_budget_relative is not None,
            min_resolved_radius=(
                None
                if config.min_resolved_radius is None
                else config.min_resolved_radius / transform.scale
            ),
        )
        sites.diagnostics.s_max = stored_s_upper
        if config.distortion_budget_absolute is not None:
            normalized_absolute_budget = (
                config.distortion_budget_absolute / transform.scale
            )
            stored_absolute_violation = max(
                0.0,
                stored_s_upper
                + input_quantization_radius / transform.scale
                - normalized_absolute_budget,
            )
            if stored_absolute_violation > 0.0:
                stored_absolute_violation = float(
                    np.nextafter(stored_absolute_violation, np.inf)
                )
            sites.diagnostics.distortion_budget_violation_max = max(
                sites.diagnostics.distortion_budget_violation_max,
                stored_absolute_violation,
            )
        if config.distortion_budget_relative is not None:
            sites.diagnostics.relative_distortion_ratio_max = stored_gamma_upper
            sites.diagnostics.relative_contract_status = relative_contract_status
            sites.diagnostics.relative_zero_scale_violations = zero_scale_violations
            relative_budget_tolerance = max(
                512.0 * np.finfo(np.float32).eps, 1.0e-6
            )
            if (
                stored_gamma_upper is None
                or stored_gamma_upper
                > config.distortion_budget_relative + relative_budget_tolerance
            ):
                raise RuntimeError(
                    "stored power-site distortion violates the requested local "
                    f"ratio: observed={stored_gamma_upper!r}, requested="
                    f"{config.distortion_budget_relative:.9g}, tolerance="
                    f"{relative_budget_tolerance:.3g}. The run is rejected "
                    "instead of publishing an invalid multiplicative contract."
                )
        sites.diagnostics.local_scale_below_resolution_count = (
            local_scale_below_resolution_count
        )
        sites.diagnostics.local_scale_below_resolution_fraction = (
            None
            if local_scale_below_resolution_count is None
            else local_scale_below_resolution_count / max(1, n_points)
        )
        # Local scales are only needed to close the observed ratio and compute
        # diagnostics; retaining 30 M additional values would serve no later
        # hierarchy operation.
        sites.local_scales = None
        timings["regularization"] = time.perf_counter() - start
        s_original = sites.diagnostics.s_max * transform.scale
        if self.distortion_budget is not None and s_original > self.distortion_budget:
            raise RuntimeError(
                f"full-run distortion s={s_original:.6g} exceeds "
                f"budget {self.distortion_budget:.6g}; lower kappa and rerun"
            )
        progress.done(
            "regularization",
            total=n_points,
            unit="points",
            details={"s_max_input_units": float(s_original)},
        )
        del raw_index, values, original_values
        _release_cached_gpu_memory(self.backend)

        progress.start(
            "power_index",
            total=1,
            unit="stage",
            details={
                "grid_policy": preliminary_spec.policy,
                "planned_grid_shape": [int(value) for value in preliminary_spec.shape],
            },
        )
        if config.min_resolved_radius is None:
            spec = build_grid_spec(sites.centers, config.grid_shape)
        else:
            spec = build_grid_spec_for_local_scale(
                sites.centers,
                min_resolved_radius=config.min_resolved_radius / transform.scale,
                max_relative_spatial_error=config.max_relative_spatial_error,
                max_cells=config.max_grid_cells,
            )
        if spec.n_cubes > preliminary_spec.n_cubes:
            raise RuntimeError(
                "the regularized-site grid unexpectedly exceeds the conservative "
                "raw-point grid plan"
            )
        memory = estimate_memory(
            n_points,
            config,
            input_coordinate_bytes=input_coordinate_bytes,
            n_cubes=spec.n_cubes,
        )
        del preliminary_spec
        start = time.perf_counter()
        if self.backend == "cpu":
            oracle = ExactLiftedPowerKNN(
                sites.centers, sites.additive_weights, K=config.K
            )
            neighbor_support = (
                "analytic_identity_kappa_one_and_exact_lifted_power_4d"
                if analytic_identity
                else "scipy_ckdtree_exact_raw_3d_and_exact_lifted_power_4d"
            )
        else:
            from .cuda_runtime import CuvsBruteForceIndex, RBCAuditedIndex

            candidate_plan = _cuda_power_candidate_plan(
                n_points,
                K=config.K,
                candidate_k_initial=config.candidate_k_initial,
                candidate_k_max=config.candidate_k_max,
            )
            site_index_class = (
                RBCAuditedIndex
                if candidate_plan["index_backend"] == "rbc"
                else CuvsBruteForceIndex
            )
            progress.message(
                "power_index", f"build_{candidate_plan['index_backend']}_index"
            )
            site_index: Any = site_index_class(
                sites.centers, max_k=candidate_plan["index_max_k"]
            )
            power_exhaustive = bool(
                getattr(site_index, "algorithmically_exhaustive", False)
            )
            if config.neighbor_audit_queries and not power_exhaustive:
                progress.message("power_index", "audit_grid_against_cuvs_bruteforce")
                audit_count = min(config.neighbor_audit_queries, spec.n_cubes)
                flat = cp.linspace(0, spec.n_cubes - 1, audit_count, dtype=cp.int64)
                audit_queries = grid_centers(
                    spec, flat, xp=cp, dtype=sites.centers.dtype
                )
                audit = site_index.audit_against_bruteforce(
                    audit_queries,
                    k=candidate_plan["index_max_k"],
                    require_support_equivalence=False,
                )
                audits["power_grid_centers_rbc_vs_cuvs_bruteforce"] = audit.to_dict()
                del flat, audit_queries
            audits["power_neighbor_status"] = site_index.status
            audits["power_candidate_plan"] = candidate_plan
            power_neighbor_supported = power_exhaustive or bool(site_index.audited)
            if config.require_neighbor_audit and not power_neighbor_supported:
                raise NeighborAuditError("power", audits)
            if raw_neighbor_supported and power_neighbor_supported:
                neighbor_support = (
                    "analytic_identity_kappa_one_and_cuda_"
                    "exhaustive_or_empirically_audited_power_candidates"
                    if analytic_identity
                    else "cuda_exhaustive_or_empirically_audited_"
                    "euclidean_candidates"
                )
            else:
                neighbor_support = "cuda_neighbor_evidence_incomplete_or_failed"
        timings["power_index"] = time.perf_counter() - start
        progress.done(
            "power_index",
            total=1,
            unit="stage",
            details={
                "cubes": int(spec.n_cubes),
                "audit_queries": int(config.neighbor_audit_queries),
            },
        )

        if self.backend == "cuda":
            oracle = CertifiedPowerKNN(
                sites.centers,
                sites.additive_weights,
                site_index,
                K=config.K,
                candidate_k_initial=candidate_plan["candidate_k_initial"],
                candidate_k_max=candidate_plan["candidate_k_max"],
                numerical_margin_factor=config.numerical_margin_factor,
                strict=config.strict_certification,
            )
        start = progress.start("power_field", total=spec.n_cubes, unit="cubes")
        field = evaluate_cubical_field(
            oracle,
            spec,
            chunk_size=config.power_chunk_size,
            progress=progress.helper_callback,
        )
        _synchronize(self.backend)
        timings["power_field"] = time.perf_counter() - start
        progress.done(
            "power_field",
            total=spec.n_cubes,
            unit="cubes",
            details={
                "certified": int(field.diagnostics.certified_queries),
                "uncertain": int(field.diagnostics.uncertain_queries),
            },
        )
        del oracle
        if self.backend == "cuda":
            del site_index
        _release_cached_gpu_memory(self.backend)

        msf_edge_count = max(0, int(spec.n_cubes) - 1)
        start = progress.start("cubical_msf", total=msf_edge_count, unit="edges")
        base_forest_normalized = build_cubical_msf(
            field.radii,
            dims=spec.shape,
            backend="cpu" if self.backend == "cpu" else "cuda",
            progress=progress.helper_callback,
        )
        # Uniform cubes have a constant H.  The two certified filtrations have
        # identical topology and differ only by this radius translation.
        sites = _denormalize_sites(sites, transform)
        field = _denormalize_field(field, transform)
        base_forest = _rescale_forest(base_forest_normalized, transform.scale)
        effective_half_width = field.spec.half_diagonal + field.numerical_radius_error
        inner_forest = base_forest.shifted(effective_half_width)
        outer_forest = base_forest.shifted(-effective_half_width)
        _synchronize(self.backend)
        timings["cubical_msf"] = time.perf_counter() - start
        progress.done(
            "cubical_msf",
            total=msf_edge_count,
            unit="edges",
            details={"vertices": int(spec.n_cubes)},
        )

        spatial_enveloped = field.diagnostics.uncertain_queries == 0
        effective_half_width = field.spec.half_diagonal + field.numerical_radius_error
        s_upper = float(sites.diagnostics.s_max)
        quantization_upper = float(input_quantization_radius)
        base_spatial = effective_half_width if spatial_enveloped else None
        envelope_spatial = 2.0 * effective_half_width if spatial_enveloped else None
        base_total = (
            quantization_upper + s_upper + effective_half_width
            if spatial_enveloped
            else None
        )
        envelope_total = (
            quantization_upper + s_upper + 2.0 * effective_half_width
            if spatial_enveloped
            else None
        )
        observed_gamma = sites.diagnostics.relative_distortion_ratio_max
        multiplicative_contract = (
            config.regularization_mode == "local_distortion"
            and config.distortion_budget_relative is not None
            and config.local_scale_k == config.K
            and observed_gamma is not None
            and observed_gamma < 0.5
        )
        relative_spatial_at_min = (
            None
            if config.min_resolved_radius is None or envelope_spatial is None
            else envelope_spatial / config.min_resolved_radius
        )
        report = AccuracyReport(
            mode="spatial_enveloped" if spatial_enveloped else "spatial_uncertain",
            neighbor_support=neighbor_support,
            entropy_residual_max=sites.diagnostics.entropy_residual_max,
            input_quantization_radius=quantization_upper,
            regularization_interleaving_radius=s_upper,
            regularization_multiplicative_lower=(
                1.0 - 2.0 * observed_gamma
                if multiplicative_contract and observed_gamma is not None
                else None
            ),
            regularization_multiplicative_upper=(
                1.0 + 2.0 * observed_gamma
                if multiplicative_contract and observed_gamma is not None
                else None
            ),
            power_knn_certified_fraction=field.diagnostics.certified_fraction,
            conditional_guard_pass_fraction=(
                field.diagnostics.certified_fraction if self.backend == "cuda" else None
            ),
            power_knn_uncertain_count=field.diagnostics.uncertain_queries,
            numerical_radius_error=field.numerical_radius_error,
            base_spatial_interleaving_radius=base_spatial,
            spatial_interleaving_radius=envelope_spatial,
            base_total_interleaving_radius=base_total,
            total_interleaving_radius=envelope_total,
            min_resolved_radius=config.min_resolved_radius,
            relative_spatial_error_at_min_radius=relative_spatial_at_min,
            grid_policy=field.spec.policy,
            cubical_connectivity=26,
            cubical_msf_exact_for_stored_float32_values=(self.backend == "cuda"),
            cubical_msf_exact_for_stored_values=True,
            field_dtype=str(field.radii.dtype),
            power_oracle_exact=(self.backend == "cpu"),
            flat_selection="none",
            atlas_completeness="not_used",
            full_power_hgp_complete=False,
            K_fixed_finite_sample_resolution=True,
            hardware_manifest=hardware_manifest,
            notes=[
                "Primary output is the inner/outer cubical H0 hierarchy.",
                "epsilon_X+s+(H+delta_num) is the base-forest radius/H0 "
                "interleaving bound; epsilon_X+s+2(H+delta_num) is the "
                "inner/outer envelope bound, never a flat-label guarantee.",
                *(
                    [
                        "CUDA power top-K is conditional on the sampled RBC order "
                        "and declared float32 envelope; directed intervals remain future work."
                    ]
                    if self.backend == "cuda"
                    else [
                        "CPU power top-K uses the exact 4-D Euclidean lifting with "
                        "SciPy cKDTree for the stored float64 sites."
                    ]
                ),
                "Canonical site-to-component coverage is not replaced by cell lookup.",
                "K fixed at finite sample size is not an asymptotic consistency claim.",
                "Uniform-grid inner and outer forests share one MSF topology.",
                "Geometry was isotropically recentered/scaled; public radii use input units.",
                "The cubical envelope includes an explicit float32 radius margin.",
                *(
                    [
                        "Local distortion uses the observed outward-rounded gamma; "
                        "the (1-2gamma, 1+2gamma) theorem is relative to the "
                        "quantized input K-NN field. epsilon_X remains additive."
                    ]
                    if multiplicative_contract
                    else []
                ),
                *(
                    [
                        "The anisotropic uniform grid is driven by a minimum local "
                        "radius and refuses an unaffordable cell budget; it is not "
                        "an adaptive-grid claim."
                    ]
                    if field.spec.policy == "local_scale"
                    else []
                ),
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


def _cuda_raw_index_backend(n_points: int, query_k: int) -> str:
    """Route raw KNN without relying on cuML's hidden fallback or rank sentinel."""

    n = int(n_points)
    requested = int(query_k)
    if not 1 <= requested <= n:
        raise ValueError("raw CUDA query_k must lie in [1, number of points]")
    rbc_capacity = min(math.isqrt(n), RBC_MAX_QUERY_K)
    return (
        "rbc"
        if requested < n and requested <= rbc_capacity
        else "cuvs_brute_force"
    )


def _cuda_power_candidate_plan(
    n_points: int,
    *,
    K: int,
    candidate_k_initial: int,
    candidate_k_max: int,
) -> dict[str, Any]:
    """Choose an explicit small/large CUDA KNN regime.

    RBC is kept only when it can honor the complete requested candidate cap
    plus its global guard.  Otherwise cuVS brute force is selected explicitly;
    the configuration is never silently weakened just because ``N`` is small.
    """

    n = int(n_points)
    cover_k = int(K)
    rbc_landmark_limit = math.isqrt(n)
    rbc_limit = min(rbc_landmark_limit, RBC_MAX_QUERY_K)
    effective_max = min(int(candidate_k_max), n)
    effective_initial = min(int(candidate_k_initial), effective_max)
    index_max = n if effective_max == n else effective_max + 1
    if index_max < n and index_max <= rbc_limit:
        backend = "rbc"
        reason = "rbc_honors_requested_candidate_and_guard_capacity"
    else:
        backend = "cuvs_brute_force"
        reason = "rbc_cannot_honor_requested_candidate_and_guard_capacity"
    if effective_initial < cover_k or effective_max < cover_k:
        raise ValueError("CUDA candidate planning requires K <= number of points")
    return {
        "index_backend": backend,
        "reason": reason,
        "rbc_max_k": int(rbc_limit),
        "rbc_landmark_max_k": int(rbc_landmark_limit),
        "rbc_kernel_max_k": int(RBC_MAX_QUERY_K),
        "requested_candidate_k_initial": int(candidate_k_initial),
        "requested_candidate_k_max": int(candidate_k_max),
        "candidate_k_initial": int(effective_initial),
        "candidate_k_max": int(effective_max),
        "index_max_k": int(index_max),
    }


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
    if values.dtype.kind in "iu":
        exact_limit = 2**53
        too_large = values > exact_limit
        if values.dtype.kind == "i":
            too_large = too_large | (values < -exact_limit)
        if bool(xp.any(too_large).item()):
            raise ValueError(
                "integer coordinates outside [-2**53, 2**53] cannot be "
                "represented exactly by the float64 public geometry contract"
            )
    if not bool(xp.all(xp.isfinite(values)).item()):
        raise ValueError("points contain NaN or Inf")
    lower = xp.min(values, axis=0)
    upper = xp.max(values, axis=0)
    lower_values = cast(
        tuple[float, float, float], tuple(float(item.item()) for item in lower)
    )
    upper_values = cast(
        tuple[float, float, float], tuple(float(item.item()) for item in upper)
    )
    scale = max(high - low for low, high in zip(lower_values, upper_values))
    if not np.isfinite(scale):
        raise ValueError("the coordinate range overflows the float64 geometry contract")
    if scale == 0.0:
        scale = 1.0
    minimum_squared_scale = math.sqrt(float(np.nextafter(0.0, 1.0)))
    if scale < minimum_squared_scale:
        raise ValueError(
            "the coordinate range is too small for nonzero float64 squared-power "
            "weights; rescale the input units before fitting"
        )
    if scale > math.sqrt(np.finfo(np.float64).max):
        raise ValueError(
            "the coordinate range is too large for finite float64 squared-power "
            "weights; rescale the input units before fitting"
        )
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


def _input_quantization_radius(
    original: Any,
    normalized: Any,
    transform: SpatialTransform,
    *,
    chunk_size: int,
) -> float:
    """Measure the maximum input displacement introduced by float32 geometry."""

    module = type(normalized).__module__.split(".", 1)[0]
    if module == "cupy":
        import cupy as xp  # type: ignore
    else:
        xp = np
    n_points = int(normalized.shape[0])
    offset = xp.asarray(transform.offset, dtype=xp.float64)
    maximum = 0.0
    for start in range(0, n_points, int(chunk_size)):
        stop = min(start + int(chunk_size), n_points)
        source = xp.asarray(original[start:stop], dtype=xp.float64)
        reconstructed = (
            xp.asarray(normalized[start:stop], dtype=xp.float64) * transform.scale
            + offset[None, :]
        )
        errors = xp.sqrt(xp.sum((source - reconstructed) ** 2, axis=1))
        maximum = max(maximum, float(xp.max(errors).item()))
        del source, reconstructed, errors
    coordinate_scale = max(
        float(transform.scale),
        *(abs(value) for value in transform.offset),
    )
    arithmetic_margin = 64.0 * np.finfo(np.float64).eps * coordinate_scale
    return float(np.nextafter(maximum + arithmetic_margin, np.inf))


def _stored_site_contract_upper(
    points: Any,
    sites: RegularizedSites,
    *,
    chunk_size: int,
    relative_requested: bool,
    min_resolved_radius: float | None,
) -> tuple[float, float | None, str, int, int | None]:
    """Recompute ``s_i`` in float64 and close the observed local ratio."""

    module = type(points).__module__.split(".", 1)[0]
    if module == "cupy":
        import cupy as xp  # type: ignore
    else:
        xp = np
    n_points = int(points.shape[0])
    s_max = 0.0
    gamma_max = 0.0
    zero_scale_violations = 0
    local_scales = sites.local_scales
    below_resolution_count = (
        0 if min_resolved_radius is not None and local_scales is not None else None
    )
    if relative_requested and local_scales is None:
        raise RuntimeError("relative distortion requested without retained K-NN scales")
    for start in range(0, n_points, int(chunk_size)):
        stop = min(start + int(chunk_size), n_points)
        x = xp.asarray(points[start:stop], dtype=xp.float64)
        z = xp.asarray(sites.centers[start:stop], dtype=xp.float64)
        a = xp.maximum(
            xp.asarray(sites.additive_weights[start:stop], dtype=xp.float64), 0.0
        )
        squared = xp.maximum(xp.sum((x - z) ** 2, axis=1) + a, 0.0)
        # The operands are stored float32 geometry promoted to float64.  A
        # single nextafter after sqrt is not enough to cover rounding in the
        # three subtractions, products and reductions.  Inflate the positive
        # squared quantity by a conservative gamma_n envelope first, then
        # round the square root outwards.  A computed zero is exact here:
        # distinct float32 operands cannot underflow a float64 square.
        float64_eps = np.finfo(np.float64).eps
        squared_factor = float(np.nextafter(1.0 + 64.0 * float64_eps, np.inf))
        squared_upper = xp.where(
            squared > 0.0,
            xp.nextafter(squared * squared_factor, xp.inf),
            0.0,
        )
        s = xp.where(
            squared_upper > 0.0,
            xp.nextafter(xp.sqrt(squared_upper), xp.inf),
            0.0,
        )
        s_max = max(s_max, float(xp.max(s).item()))
        if relative_requested:
            assert local_scales is not None
            raw_scale = xp.asarray(local_scales[start:stop], dtype=xp.float64)
            if below_resolution_count is not None:
                assert min_resolved_radius is not None
                below_resolution_count += int(
                    xp.sum(raw_scale < float(min_resolved_radius)).item()
                )
            raw_scale_abs = xp.abs(raw_scale)
            round_margin = 64.0 * np.finfo(np.float32).eps * raw_scale_abs
            round_margin = xp.where(
                raw_scale_abs > 0.0,
                xp.nextafter(round_margin, xp.inf),
                0.0,
            )
            scale_lower = xp.maximum(raw_scale - round_margin, 0.0)
            positive = scale_lower > 0.0
            zero_scale_violations += int(xp.sum((~positive) & (s > 0.0)).item())
            if bool(xp.any(positive).item()):
                gamma_max = max(
                    gamma_max, float(xp.max(s[positive] / scale_lower[positive]).item())
                )
        del x, z, a, squared, squared_upper, s
    # For kappa=1 with the canonical self atom, z_i=x_i and a_i=0 exactly in
    # the stored geometry.  Preserve that exact identity instead of turning a
    # computed zero into the smallest positive float64 through ``nextafter``.
    s_upper = 0.0 if s_max == 0.0 else float(np.nextafter(s_max, np.inf))
    if not relative_requested:
        return s_upper, None, "not_requested", 0, below_resolution_count
    if zero_scale_violations:
        return (
            s_upper,
            None,
            "unbounded_zero_scale",
            zero_scale_violations,
            below_resolution_count,
        )
    return (
        s_upper,
        0.0 if gamma_max == 0.0 else float(np.nextafter(gamma_max, np.inf)),
        "conditional_knn_order_float_enveloped",
        0,
        below_resolution_count,
    )


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
        xp.asarray(sites.centers, dtype=xp.float64) * transform.scale + offset[None, :]
    )
    normalized_additive = xp.asarray(sites.additive_weights, dtype=xp.float64)
    additive = normalized_additive * (transform.scale**2)
    distortions = xp.asarray(sites.distortions, dtype=xp.float64) * transform.scale
    if bool(
        xp.any(
            (normalized_additive > 0.0) & (additive < np.finfo(np.float64).tiny)
        ).item()
    ):
        raise ValueError(
            "the physical additive power weights are subnormal or underflow "
            "float64; rescale the input units before fitting"
        )
    if not bool(
        xp.all(xp.isfinite(centers)).item()
        and xp.all(xp.isfinite(additive)).item()
        and xp.all(xp.isfinite(distortions)).item()
    ):
        raise ValueError(
            "denormalized power sites exceed the finite float64 geometry contract"
        )
    diagnostics = RegularizationDiagnostics(
        mode=sites.diagnostics.mode,
        solver_backend=sites.diagnostics.solver_backend,
        entropy_residual_max=sites.diagnostics.entropy_residual_max,
        entropy_target_deviation_max=sites.diagnostics.entropy_target_deviation_max,
        simplex_residual_max=sites.diagnostics.simplex_residual_max,
        s_max=(
            0.0
            if sites.diagnostics.s_max == 0.0
            else float(
                np.nextafter(sites.diagnostics.s_max * transform.scale, np.inf)
            )
        ),
        s_quantiles={
            key: value * transform.scale
            for key, value in sites.diagnostics.s_quantiles.items()
        },
        effective_kappa_quantiles=sites.diagnostics.effective_kappa_quantiles.copy(),
        local_scale_quantiles={
            key: value * transform.scale
            for key, value in sites.diagnostics.local_scale_quantiles.items()
        },
        distortion_budget_violation_max=(
            sites.diagnostics.distortion_budget_violation_max * transform.scale
        ),
        relative_distortion_ratio_max=sites.diagnostics.relative_distortion_ratio_max,
        relative_contract_status=sites.diagnostics.relative_contract_status,
        relative_zero_scale_violations=(
            sites.diagnostics.relative_zero_scale_violations
        ),
        local_scale_below_resolution_count=(
            sites.diagnostics.local_scale_below_resolution_count
        ),
        local_scale_below_resolution_fraction=(
            sites.diagnostics.local_scale_below_resolution_fraction
        ),
        entropy_degenerate_rows=sites.diagnostics.entropy_degenerate_rows,
        query_count=sites.diagnostics.query_count,
        s_quantile_sample_size=sites.diagnostics.s_quantile_sample_size,
        stored_budget_projection_count=(
            sites.diagnostics.stored_budget_projection_count
        ),
        stored_budget_identity_fallback_count=(
            sites.diagnostics.stored_budget_identity_fallback_count
        ),
        stored_budget_projection_factor_min=(
            sites.diagnostics.stored_budget_projection_factor_min
        ),
    )
    return RegularizedSites(
        centers,
        additive,
        distortions,
        diagnostics,
        sites.effective_kappa,
        None,
    )


def _denormalize_field(
    field: CubicalField, transform: SpatialTransform
) -> CubicalField:
    module = type(field.radii).__module__.split(".", 1)[0]
    if module == "cupy":
        import cupy as xp  # type: ignore
    else:
        xp = np
    offset = transform.offset
    spec = GridSpec(
        bbox_min=cast(
            tuple[float, float, float],
            tuple(
                offset[axis] + transform.scale * field.spec.bbox_min[axis]
                for axis in range(3)
            ),
        ),
        bbox_max=cast(
            tuple[float, float, float],
            tuple(
                offset[axis] + transform.scale * field.spec.bbox_max[axis]
                for axis in range(3)
            ),
        ),
        shape=field.spec.shape,
        cell_widths=cast(
            tuple[float, float, float],
            tuple(width * transform.scale for width in field.spec.cell_widths),
        ),
        half_diagonal=field.spec.half_diagonal * transform.scale,
        policy=field.spec.policy,
        requested_half_diagonal_limit=(
            None
            if field.spec.requested_half_diagonal_limit is None
            else field.spec.requested_half_diagonal_limit * transform.scale
        ),
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
        power_arithmetic_radius_error=(
            field.diagnostics.power_arithmetic_radius_error * transform.scale
        ),
        grid_center_quantization_radius=(
            field.diagnostics.grid_center_quantization_radius * transform.scale
        ),
    )
    radii = xp.asarray(field.radii, dtype=xp.float64) * transform.scale
    numerical_radius_error = field.numerical_radius_error * transform.scale
    effective_half_width = spec.half_diagonal + numerical_radius_error
    return CubicalField(
        spec=spec,
        radii=radii,
        numerical_radius_error=numerical_radius_error,
        inner_activation=radii + effective_half_width,
        outer_activation=radii - effective_half_width,
        certified=field.certified,
        diagnostics=diagnostics,
    )


def _rescale_forest(forest: CubicalMSFResult, scale: float) -> CubicalMSFResult:
    return CubicalMSFResult(
        births=np.asarray(forest.births, dtype=np.float64) * scale,
        edges=forest.edges,
        weights=np.asarray(forest.weights, dtype=np.float64) * scale,
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
