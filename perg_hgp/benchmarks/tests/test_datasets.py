from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np
import pytest

BENCHMARK_PARENT = Path(__file__).resolve().parents[2]
if str(BENCHMARK_PARENT) not in sys.path:
    sys.path.insert(0, str(BENCHMARK_PARENT))

from benchmarks.datasets import DATASET_NAMES, DatasetSpec, generate_dataset, load_dataset


@pytest.mark.parametrize("name", DATASET_NAMES)
def test_generators_are_finite_float32_and_have_exact_size(tmp_path: Path, name: str):
    artifact = generate_dataset(
        DatasetSpec(name=name, n=257, seed=19, chunk_size=31), tmp_path / name
    )
    points, labels, metadata = load_dataset(artifact.metadata_path)

    assert points.shape == (257, 3)
    assert points.dtype == np.float32
    assert np.isfinite(points).all()
    assert sum(metadata["component_counts"]) == 257
    if name == "uniform":
        assert labels is None
        assert metadata["ground_truth_available"] is False
    else:
        assert labels is not None
        assert labels.shape == (257,)
        assert labels.dtype == np.int32
        assert metadata["ground_truth_available"] is True


@pytest.mark.parametrize("name", ["anisotropic_blobs", "lidar_surfaces", "mixture"])
def test_generation_is_byte_identical_across_chunk_sizes(
    tmp_path: Path, name: str
):
    first = generate_dataset(
        DatasetSpec(name=name, n=1003, seed=123, chunk_size=17), tmp_path / "a"
    )
    second = generate_dataset(
        DatasetSpec(name=name, n=1003, seed=123, chunk_size=251), tmp_path / "b"
    )
    x_first, y_first, metadata_first = load_dataset(first.metadata_path)
    x_second, y_second, metadata_second = load_dataset(second.metadata_path)

    assert np.array_equal(x_first, x_second)
    assert metadata_first["points"]["sha256"] == metadata_second["points"]["sha256"]
    assert (y_first is None) == (y_second is None)
    if y_first is not None:
        assert np.array_equal(y_first, y_second)


def test_cache_identity_ignores_operational_chunk_size(tmp_path: Path):
    first = generate_dataset(
        DatasetSpec("two_density_corridor", 311, seed=7, chunk_size=13), tmp_path
    )
    second = generate_dataset(
        DatasetSpec("two_density_corridor", 311, seed=7, chunk_size=101), tmp_path
    )

    assert first.metadata_path == second.metadata_path
    with open(second.metadata_path, "r", encoding="utf-8") as stream:
        metadata = json.load(stream)
    # The original generation parameters remain evidence of the reused bytes.
    assert metadata["generation"]["chunk_size"] == 13


def test_seed_changes_values(tmp_path: Path):
    first = generate_dataset(DatasetSpec("uniform", 128, seed=1), tmp_path / "a")
    second = generate_dataset(DatasetSpec("uniform", 128, seed=2), tmp_path / "b")
    x_first, _, _ = load_dataset(first.metadata_path)
    x_second, _, _ = load_dataset(second.metadata_path)
    assert not np.array_equal(x_first, x_second)

