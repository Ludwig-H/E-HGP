"""Optional RAPIDS/CuPy runtime for NVIDIA GPUs.

Imports are intentionally lazy so the exact CPU reference and the normal
``perg_hgp`` package remain usable without CUDA or RAPIDS installed.
"""

from __future__ import annotations

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
    audit_scope: str = "sampled_query_value_comparison"
    is_numerical_proof: bool = False

    def to_dict(self):
        return self.__dict__.copy()


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
        rbc_limit = math.isqrt(self.size)
        if self.max_k > rbc_limit:
            raise ValueError(
                f"cuML RBC requires max_k <= floor(sqrt(N))={rbc_limit} for "
                f"N={self.size}, but max_k={self.max_k} was requested. cuML "
                "may otherwise select a brute-force fallback; this wrapper "
                "refuses that hidden algorithm change. Reduce max_k, increase "
                "N, or select an explicit brute-force backend."
            )
        if audited:
            raise ValueError(
                "audited=True cannot be declared at construction time; call "
                "audit_against_bruteforce so the empirical result is recorded"
            )
        self.model = NearestNeighbors(
            n_neighbors=self.max_k,
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
        }

    def query(self, queries: Any, k: int):
        cp, _, _ = require_cuda_stack()
        requested = _positive_integer(k, name="k")
        if not 1 <= requested <= self.max_k:
            raise ValueError(f"k must satisfy 1 <= k <= configured max_k={self.max_k}")
        values = cp.ascontiguousarray(cp.asarray(queries, dtype=cp.float32))
        _validate_queries(cp, values)
        distances, indices = self.model.kneighbors(
            values, n_neighbors=requested, return_distance=True
        )
        return cp.asarray(distances), cp.asarray(indices, dtype=cp.int64)

    def audit_against_bruteforce(self, queries: Any, *, k: int = 10) -> NeighborAudit:
        """Empirically compare sampled RBC values with cuVS brute force.

        Passing this check does not constitute a numerical proof for untested
        queries or for all floating-point inputs.
        """

        cp, _, _ = require_cuda_stack()
        requested = _positive_integer(k, name="k")
        if requested > self.max_k:
            raise ValueError(f"k must satisfy 1 <= k <= configured max_k={self.max_k}")
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
            exact_squared, exact_indices = brute_force.search(
                exact_index, values, requested
            )
            stop_bf.record()
            stop_bf.synchronize()
            brute_seconds = float(cp.cuda.get_elapsed_time(start_bf, stop_bf)) / 1_000.0
            exact_squared = cp.asarray(exact_squared)

            # Compare values rather than IDs: duplicate or equidistant sites may
            # legitimately produce different stable representatives.
            rbc_squared = cp.asarray(rbc_distances) ** 2
            rbc_sorted = cp.sort(rbc_squared, axis=1)
            exact_sorted = cp.sort(exact_squared, axis=1)
            maximum_error = float(cp.max(cp.abs(rbc_sorted - exact_sorted)).item())
            tolerance = (
                128.0
                * np.finfo(np.float32).eps
                * max(1.0, float(cp.max(cp.abs(exact_sorted)).item()))
            )
            values_match = maximum_error <= tolerance
            self.audited = bool(values_match)
            self.audit_status = "passed" if values_match else "failed"
            del exact_index, exact_indices, rbc_indices
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
