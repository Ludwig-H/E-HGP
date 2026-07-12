"""Deterministic, chunked synthetic point-cloud generators.

Every generator writes C-contiguous ``float32`` coordinates and optional
``int32`` ground-truth labels.  Components occupy deterministic contiguous
row blocks; this avoids a 240 MB permutation at N=30M and makes generation
independent of the requested chunk size.
"""

from __future__ import annotations

import hashlib
import json
import os
import time
import uuid
from dataclasses import asdict, dataclass
from numbers import Integral
from pathlib import Path
from typing import Callable, Iterator, Sequence

import numpy as np
from numpy.lib.format import open_memmap

try:  # Support both ``python -m benchmarks`` and direct script imports.
    from .io_utils import atomic_write_json
except ImportError:  # pragma: no cover - direct-script compatibility
    from io_utils import atomic_write_json


DATASET_NAMES = (
    "anisotropic_blobs",
    "two_density_corridor",
    "lidar_surfaces",
    "uniform",
    "mixture",
)
GENERATOR_VERSION = 1


@dataclass(frozen=True)
class DatasetSpec:
    name: str
    n: int
    seed: int = 0
    chunk_size: int = 1_000_000

    def __post_init__(self) -> None:
        normalized = self.name.strip().lower().replace("-", "_")
        if normalized == "lidar_like":
            normalized = "lidar_surfaces"
        if normalized not in DATASET_NAMES:
            raise ValueError(
                f"unknown dataset {self.name!r}; choose one of {DATASET_NAMES}"
            )
        if isinstance(self.n, bool) or not isinstance(self.n, Integral) or self.n < 1:
            raise ValueError("n must be a positive integer")
        if (
            isinstance(self.seed, bool)
            or not isinstance(self.seed, Integral)
            or self.seed < 0
        ):
            raise ValueError("seed must be a non-negative integer")
        if (
            isinstance(self.chunk_size, bool)
            or not isinstance(self.chunk_size, Integral)
            or self.chunk_size < 1
        ):
            raise ValueError("chunk_size must be a positive integer")
        object.__setattr__(self, "name", normalized)
        object.__setattr__(self, "n", int(self.n))
        object.__setattr__(self, "seed", int(self.seed))
        object.__setattr__(self, "chunk_size", int(self.chunk_size))

    def identity_dict(self) -> dict[str, int | str]:
        # Chunk size is operational and intentionally absent: generated values
        # are invariant to it and cache identity should be invariant as well.
        return {
            "generator_version": GENERATOR_VERSION,
            "name": self.name,
            "n": self.n,
            "seed": self.seed,
        }

    @property
    def fingerprint(self) -> str:
        payload = json.dumps(
            self.identity_dict(), sort_keys=True, separators=(",", ":")
        ).encode("utf-8")
        return hashlib.sha256(payload).hexdigest()


@dataclass(frozen=True)
class DatasetArtifact:
    metadata_path: Path
    points_path: Path
    labels_path: Path | None
    metadata: dict


def _counts(n: int, probabilities: Sequence[float]) -> list[int]:
    weights = np.asarray(probabilities, dtype=np.float64)
    if weights.ndim != 1 or weights.size == 0 or np.any(weights < 0):
        raise ValueError("probabilities must be a non-empty non-negative vector")
    weights /= weights.sum()
    raw = weights * n
    counts = np.floor(raw).astype(np.int64)
    remainder = n - int(counts.sum())
    if remainder:
        # Largest-remainder apportionment is deterministic for every N.
        order = np.argsort(-(raw - counts), kind="stable")
        counts[order[:remainder]] += 1
    return [int(value) for value in counts]


def _slices(start: int, count: int, chunk_size: int) -> Iterator[slice]:
    stop = start + count
    for left in range(start, stop, chunk_size):
        yield slice(left, min(left + chunk_size, stop))


def _rng(seed: int, family: int, component: int) -> np.random.Generator:
    return np.random.Generator(
        np.random.PCG64DXSM(np.random.SeedSequence([seed, family, component]))
    )


def _fill_gaussian(
    points: np.ndarray,
    labels: np.ndarray,
    *,
    start: int,
    count: int,
    center: Sequence[float],
    transform: np.ndarray,
    label: int,
    rng: np.random.Generator,
    chunk_size: int,
) -> None:
    center_array = np.asarray(center, dtype=np.float32)
    transform_array = np.asarray(transform, dtype=np.float32)
    for rows in _slices(start, count, chunk_size):
        size = rows.stop - rows.start
        standard = rng.standard_normal((size, 3), dtype=np.float32)
        # Spell out the three length-3 dot products.  BLAS is allowed to pick
        # size-dependent kernels whose final float32 bit can differ with the
        # operational chunk size; these fixed elementwise expressions cannot.
        block = np.empty((size, 3), dtype=np.float32)
        for axis in range(3):
            block[:, axis] = (
                standard[:, 0] * transform_array[axis, 0]
                + standard[:, 1] * transform_array[axis, 1]
                + standard[:, 2] * transform_array[axis, 2]
                + center_array[axis]
            )
        points[rows] = block
        labels[rows] = label


def _anisotropic_blobs(
    points: np.ndarray, labels: np.ndarray, spec: DatasetSpec
) -> dict:
    centers = np.asarray(
        [
            [-2.4, -1.6, -0.5],
            [2.0, -1.2, 0.8],
            [-0.8, 2.2, 1.2],
            [2.4, 2.0, -1.1],
        ],
        dtype=np.float32,
    )
    transforms = np.asarray(
        [
            [[0.75, 0.20, 0.00], [0.00, 0.18, 0.04], [0.00, 0.00, 0.09]],
            [[0.12, 0.00, 0.03], [0.35, 0.65, 0.00], [0.00, 0.10, 0.22]],
            [[0.42, -0.25, 0.00], [0.18, 0.38, 0.00], [0.00, 0.00, 0.11]],
            [[0.18, 0.04, 0.00], [0.00, 0.22, 0.10], [0.15, 0.00, 0.70]],
        ],
        dtype=np.float32,
    )
    counts = _counts(spec.n, [0.24, 0.23, 0.22, 0.21, 0.10])
    cursor = 0
    for component, count in enumerate(counts[:-1]):
        _fill_gaussian(
            points,
            labels,
            start=cursor,
            count=count,
            center=centers[component],
            transform=transforms[component],
            label=component,
            rng=_rng(spec.seed, 11, component),
            chunk_size=spec.chunk_size,
        )
        cursor += count
    noise_rng = _rng(spec.seed, 11, 99)
    for rows in _slices(cursor, counts[-1], spec.chunk_size):
        points[rows] = noise_rng.uniform(
            low=(-4.5, -3.8, -2.7),
            high=(4.5, 3.8, 2.7),
            size=(rows.stop - rows.start, 3),
        ).astype(np.float32)
        labels[rows] = -1
    return {
        "description": "four rotated anisotropic Gaussian clouds plus uniform noise",
        "component_counts": counts,
        "ground_truth_available": True,
        "noise_label": -1,
    }


def _two_density_corridor(
    points: np.ndarray, labels: np.ndarray, spec: DatasetSpec
) -> dict:
    dense_count, sparse_count, corridor_count, noise_count = _counts(
        spec.n, [0.50, 0.36, 0.10, 0.04]
    )
    dense_transform = np.diag(np.asarray([0.20, 0.16, 0.14], dtype=np.float32))
    sparse_transform = np.asarray(
        [[0.58, 0.10, 0.00], [0.00, 0.45, 0.08], [0.00, 0.00, 0.36]],
        dtype=np.float32,
    )
    _fill_gaussian(
        points,
        labels,
        start=0,
        count=dense_count,
        center=(-2.2, 0.0, 0.0),
        transform=dense_transform,
        label=0,
        rng=_rng(spec.seed, 23, 0),
        chunk_size=spec.chunk_size,
    )
    _fill_gaussian(
        points,
        labels,
        start=dense_count,
        count=sparse_count,
        center=(2.2, 0.0, 0.0),
        transform=sparse_transform,
        label=1,
        rng=_rng(spec.seed, 23, 1),
        chunk_size=spec.chunk_size,
    )
    corridor_start = dense_count + sparse_count
    corridor_x_rng = _rng(spec.seed, 23, 20)
    corridor_cross_rng = _rng(spec.seed, 23, 21)
    for rows in _slices(corridor_start, corridor_count, spec.chunk_size):
        size = rows.stop - rows.start
        block = np.empty((size, 3), dtype=np.float32)
        block[:, 0] = corridor_x_rng.uniform(-1.85, 1.65, size=size).astype(
            np.float32
        )
        block[:, 1:] = corridor_cross_rng.normal(
            0.0, 0.075, size=(size, 2)
        ).astype(np.float32)
        points[rows] = block
        # The bridge is deliberately ambiguous and excluded from the two
        # semantic clusters, which tests resistance to premature percolation.
        labels[rows] = -1
    noise_start = corridor_start + corridor_count
    noise_rng = _rng(spec.seed, 23, 3)
    for rows in _slices(noise_start, noise_count, spec.chunk_size):
        points[rows] = noise_rng.uniform(
            (-3.5, -2.1, -1.8),
            (3.5, 2.1, 1.8),
            size=(rows.stop - rows.start, 3),
        ).astype(np.float32)
        labels[rows] = -1
    return {
        "description": "dense and sparse 3-D clusters joined by a thin noise corridor",
        "component_counts": [dense_count, sparse_count, corridor_count, noise_count],
        "ground_truth_available": True,
        "noise_label": -1,
    }


def _fill_cylinder(
    points: np.ndarray,
    labels: np.ndarray,
    *,
    start: int,
    count: int,
    center: Sequence[float],
    radius: float,
    height: float,
    label: int,
    rngs: tuple[np.random.Generator, ...],
    chunk_size: int,
) -> None:
    if len(rngs) != 4:
        raise ValueError("cylinder generation requires four independent streams")
    theta_rng, radial_rng, height_rng, jitter_rng = rngs
    cx, cy, cz = center
    for rows in _slices(start, count, chunk_size):
        size = rows.stop - rows.start
        theta = theta_rng.uniform(0.0, 2.0 * np.pi, size=size)
        radial_noise = radial_rng.normal(0.0, 0.018, size=size)
        block = np.empty((size, 3), dtype=np.float32)
        block[:, 0] = cx + (radius + radial_noise) * np.cos(theta)
        block[:, 1] = cy + (radius + radial_noise) * np.sin(theta)
        block[:, 2] = cz + height_rng.uniform(0.0, height, size=size)
        block += jitter_rng.normal(0.0, 0.006, size=(size, 3)).astype(np.float32)
        points[rows] = block
        labels[rows] = label


def _fill_ellipsoid_shell(
    points: np.ndarray,
    labels: np.ndarray,
    *,
    start: int,
    count: int,
    center: Sequence[float],
    axes: Sequence[float],
    yaw: float,
    label: int,
    rngs: tuple[np.random.Generator, np.random.Generator],
    chunk_size: int,
) -> None:
    center_array = np.asarray(center, dtype=np.float32)
    axes_array = np.asarray(axes, dtype=np.float32)
    direction_rng, noise_rng = rngs
    cosine = np.float32(np.cos(yaw))
    sine = np.float32(np.sin(yaw))
    for rows in _slices(start, count, chunk_size):
        size = rows.stop - rows.start
        directions = direction_rng.standard_normal((size, 3), dtype=np.float32)
        norms = np.sqrt(
            directions[:, 0] * directions[:, 0]
            + directions[:, 1] * directions[:, 1]
            + directions[:, 2] * directions[:, 2]
        )[:, None]
        directions /= np.maximum(norms, np.finfo(np.float32).tiny)
        shell = directions * axes_array
        shell += noise_rng.normal(0.0, 0.012, size=(size, 3)).astype(np.float32)
        block = np.empty_like(shell)
        block[:, 0] = shell[:, 0] * cosine - shell[:, 1] * sine + center_array[0]
        block[:, 1] = shell[:, 0] * sine + shell[:, 1] * cosine + center_array[1]
        block[:, 2] = shell[:, 2] + center_array[2]
        points[rows] = block
        labels[rows] = label


def _lidar_surfaces(
    points: np.ndarray, labels: np.ndarray, spec: DatasetSpec
) -> dict:
    # Ground, four pole/trunk surfaces, two vehicle-like shells, outliers.
    counts = _counts(spec.n, [0.36, 0.09, 0.08, 0.08, 0.07, 0.13, 0.13, 0.06])
    cursor = 0
    ground_xy_rng = _rng(spec.seed, 37, 0)
    ground_z_rng = _rng(spec.seed, 37, 1)
    for rows in _slices(cursor, counts[0], spec.chunk_size):
        size = rows.stop - rows.start
        block = np.empty((size, 3), dtype=np.float32)
        block[:, :2] = ground_xy_rng.uniform(-12.0, 12.0, size=(size, 2)).astype(
            np.float32
        )
        block[:, 2] = (
            0.025 * block[:, 0]
            - 0.018 * block[:, 1]
            + ground_z_rng.normal(0.0, 0.025, size=size)
        )
        points[rows] = block
        labels[rows] = -1
    cursor += counts[0]
    cylinders = [
        ((-5.0, -2.4, -0.08), 0.22, 3.7),
        ((-1.0, 4.6, -0.10), 0.30, 4.4),
        ((4.4, 3.1, 0.02), 0.18, 3.1),
        ((6.2, -4.1, 0.12), 0.38, 5.0),
    ]
    for component, (geometry, count) in enumerate(zip(cylinders, counts[1:5])):
        center, radius, height = geometry
        _fill_cylinder(
            points,
            labels,
            start=cursor,
            count=count,
            center=center,
            radius=radius,
            height=height,
            label=component,
            rngs=tuple(
                _rng(spec.seed, 371, 10 * component + stream)
                for stream in range(4)
            ),
            chunk_size=spec.chunk_size,
        )
        cursor += count
    vehicles = [
        ((-3.2, 2.2, 0.75), (1.65, 0.78, 0.68), 0.35),
        ((3.0, -2.6, 0.82), (1.90, 0.86, 0.74), -0.48),
    ]
    for offset, (geometry, count) in enumerate(zip(vehicles, counts[5:7])):
        center, axes, yaw = geometry
        _fill_ellipsoid_shell(
            points,
            labels,
            start=cursor,
            count=count,
            center=center,
            axes=axes,
            yaw=yaw,
            label=4 + offset,
            rngs=(
                _rng(spec.seed, 372, 10 * offset),
                _rng(spec.seed, 372, 10 * offset + 1),
            ),
            chunk_size=spec.chunk_size,
        )
        cursor += count
    outlier_rng = _rng(spec.seed, 37, 99)
    for rows in _slices(cursor, counts[-1], spec.chunk_size):
        points[rows] = outlier_rng.uniform(
            (-12.0, -12.0, -0.4),
            (12.0, 12.0, 6.0),
            size=(rows.stop - rows.start, 3),
        ).astype(np.float32)
        labels[rows] = -1
    return {
        "description": "tilted LiDAR-like ground, pole surfaces, vehicle shells and outliers",
        "component_counts": counts,
        "ground_truth_available": True,
        "noise_label": -1,
    }


def _uniform(points: np.ndarray, labels: np.ndarray, spec: DatasetSpec) -> dict:
    del labels
    rng = _rng(spec.seed, 41, 0)
    for rows in _slices(0, spec.n, spec.chunk_size):
        points[rows] = rng.uniform(
            -1.0, 1.0, size=(rows.stop - rows.start, 3)
        ).astype(np.float32)
    return {
        "description": "uniform 3-D cube for pure throughput and null-structure tests",
        "component_counts": [spec.n],
        "ground_truth_available": False,
        "noise_label": None,
    }


def _mixture(points: np.ndarray, labels: np.ndarray, spec: DatasetSpec) -> dict:
    probabilities = [0.16, 0.15, 0.14, 0.13, 0.12, 0.11, 0.10, 0.06, 0.03]
    counts = _counts(spec.n, probabilities)
    cursor = 0
    for component, count in enumerate(counts[:-1]):
        angle = component * (2.0 * np.pi / 8.0)
        center = (
            4.0 * np.cos(angle),
            4.0 * np.sin(angle),
            -1.4 + 0.4 * component,
        )
        c, s = np.cos(0.7 * angle), np.sin(0.7 * angle)
        rotation = np.asarray(
            [[c, -s, 0.0], [s, c, 0.0], [0.0, 0.0, 1.0]], dtype=np.float32
        )
        scales = np.diag(
            np.asarray(
                [0.22 + 0.055 * component, 0.12 + 0.02 * (component % 3), 0.10 + 0.035 * (component % 4)],
                dtype=np.float32,
            )
        )
        _fill_gaussian(
            points,
            labels,
            start=cursor,
            count=count,
            center=center,
            transform=rotation @ scales,
            label=component,
            rng=_rng(spec.seed, 53, component),
            chunk_size=spec.chunk_size,
        )
        cursor += count
    noise_rng = _rng(spec.seed, 53, 99)
    for rows in _slices(cursor, counts[-1], spec.chunk_size):
        points[rows] = noise_rng.uniform(
            (-6.0, -6.0, -3.0),
            (6.0, 6.0, 3.5),
            size=(rows.stop - rows.start, 3),
        ).astype(np.float32)
        labels[rows] = -1
    return {
        "description": "scalable eight-component density/anisotropy mixture plus noise",
        "component_counts": counts,
        "ground_truth_available": True,
        "noise_label": -1,
    }


_GENERATORS: dict[str, Callable[[np.ndarray, np.ndarray, DatasetSpec], dict]] = {
    "anisotropic_blobs": _anisotropic_blobs,
    "two_density_corridor": _two_density_corridor,
    "lidar_surfaces": _lidar_surfaces,
    "uniform": _uniform,
    "mixture": _mixture,
}


def _fsync_file(path: Path) -> None:
    with open(path, "rb") as stream:
        os.fsync(stream.fileno())


def _array_sha256(array: np.ndarray, chunk_rows: int = 1_000_000) -> str:
    digest = hashlib.sha256()
    for start in range(0, int(array.shape[0]), chunk_rows):
        block = np.ascontiguousarray(array[start : start + chunk_rows])
        digest.update(memoryview(block).cast("B"))
    return digest.hexdigest()


def _coordinate_summary(points: np.ndarray, chunk_rows: int) -> dict:
    minima = np.full(3, np.inf, dtype=np.float64)
    maxima = np.full(3, -np.inf, dtype=np.float64)
    sums = np.zeros(3, dtype=np.float64)
    for start in range(0, int(points.shape[0]), chunk_rows):
        block = np.asarray(points[start : start + chunk_rows])
        if not np.all(np.isfinite(block)):
            raise RuntimeError("generator produced non-finite coordinates")
        minima = np.minimum(minima, np.min(block, axis=0))
        maxima = np.maximum(maxima, np.max(block, axis=0))
        sums += np.sum(block, axis=0, dtype=np.float64)
    return {
        "min": minima.tolist(),
        "max": maxima.tolist(),
        "mean": (sums / points.shape[0]).tolist(),
    }


def generate_dataset(
    spec: DatasetSpec,
    directory: str | os.PathLike[str],
    *,
    force: bool = False,
) -> DatasetArtifact:
    """Generate or reuse an atomic ``.npy`` dataset artifact."""

    output = Path(directory)
    output.mkdir(parents=True, exist_ok=True)
    stem = f"{spec.name}-n{spec.n}-s{spec.seed}-{spec.fingerprint[:12]}"
    points_path = output / f"{stem}.points.npy"
    labels_path = None if spec.name == "uniform" else output / f"{stem}.labels.npy"
    metadata_path = output / f"{stem}.json"

    if not force and metadata_path.exists() and points_path.exists() and (
        labels_path is None or labels_path.exists()
    ):
        with open(metadata_path, "r", encoding="utf-8") as stream:
            metadata = json.load(stream)
        if metadata.get("identity") == spec.identity_dict():
            return DatasetArtifact(metadata_path, points_path, labels_path, metadata)

    token = f"{os.getpid()}-{uuid.uuid4().hex}"
    temporary_points = output / f".{stem}.{token}.points.tmp.npy"
    temporary_labels = output / f".{stem}.{token}.labels.tmp.npy"
    generation_started = time.perf_counter()
    points = open_memmap(
        temporary_points, mode="w+", dtype=np.float32, shape=(spec.n, 3)
    )
    labels = (
        np.empty(0, dtype=np.int32)
        if labels_path is None
        else open_memmap(
            temporary_labels, mode="w+", dtype=np.int32, shape=(spec.n,)
        )
    )
    try:
        details = _GENERATORS[spec.name](points, labels, spec)
        point_hash = _array_sha256(points, spec.chunk_size)
        label_hash = None if labels_path is None else _array_sha256(labels, spec.chunk_size)
        coordinate_summary = _coordinate_summary(points, spec.chunk_size)
        points.flush()
        if labels_path is not None:
            labels.flush()
        del points, labels
        _fsync_file(temporary_points)
        os.replace(temporary_points, points_path)
        if labels_path is not None:
            _fsync_file(temporary_labels)
            os.replace(temporary_labels, labels_path)
        metadata = {
            "schema": "perg_hgp.benchmark_dataset",
            "schema_version": 1,
            "identity": spec.identity_dict(),
            "generation": asdict(spec),
            "generation_wall_seconds": time.perf_counter() - generation_started,
            "software": {"numpy": np.__version__},
            "points": {
                "file": points_path.name,
                "shape": [spec.n, 3],
                "dtype": "float32",
                "sha256": point_hash,
                "bytes": points_path.stat().st_size,
                "coordinates": coordinate_summary,
            },
            "labels": None
            if labels_path is None
            else {
                "file": labels_path.name,
                "shape": [spec.n],
                "dtype": "int32",
                "sha256": label_hash,
                "bytes": labels_path.stat().st_size,
            },
            **details,
            "ordering": "deterministic_contiguous_component_blocks",
        }
        atomic_write_json(metadata_path, metadata)
        return DatasetArtifact(metadata_path, points_path, labels_path, metadata)
    except BaseException:
        for path in (temporary_points, temporary_labels):
            try:
                path.unlink()
            except FileNotFoundError:
                pass
        raise


def load_dataset(
    metadata_path: str | os.PathLike[str], *, mmap_mode: str | None = "r"
) -> tuple[np.ndarray, np.ndarray | None, dict]:
    path = Path(metadata_path)
    with open(path, "r", encoding="utf-8") as stream:
        metadata = json.load(stream)
    if metadata.get("schema") != "perg_hgp.benchmark_dataset":
        raise ValueError(f"not a benchmark dataset manifest: {path}")
    points = np.load(path.parent / metadata["points"]["file"], mmap_mode=mmap_mode)
    label_metadata = metadata.get("labels")
    labels = (
        None
        if label_metadata is None
        else np.load(path.parent / label_metadata["file"], mmap_mode=mmap_mode)
    )
    if points.dtype != np.float32 or points.shape != (
        metadata["identity"]["n"],
        3,
    ):
        raise ValueError("point artifact does not match its manifest")
    if labels is not None and (
        labels.dtype != np.int32 or labels.shape != (points.shape[0],)
    ):
        raise ValueError("label artifact does not match its manifest")
    return points, labels, metadata
