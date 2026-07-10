import importlib.util
import math

import numpy as np
import pytest

from perg_hgp.backends.power_cover_3d_cuda.cubical_msf import (
    build_cubical_msf,
    cubical_msf_cpu,
    cubical_msf_gpu,
    estimate_cubical_msf_memory,
)


OFFSETS_26 = tuple(
    offset
    for offset in np.ndindex(3, 3, 3)
    if offset != (1, 1, 1)
)
OFFSETS_26 = tuple(tuple(coordinate - 1 for coordinate in offset) for offset in OFFSETS_26)
OFFSETS_6 = tuple(
    offset for offset in OFFSETS_26 if sum(abs(coordinate) for coordinate in offset) == 1
)


def _dims3(shape):
    return tuple(shape) + (1,) * (3 - len(shape))


def _brute_force_components(field, threshold, *, connectivity=26):
    values = np.asarray(field)
    shape = values.shape
    nx, ny, nz = _dims3(shape)
    flat = values.reshape(-1)
    active = flat <= threshold
    parent = np.arange(flat.size, dtype=np.int32)

    def find(vertex):
        while parent[vertex] != vertex:
            parent[vertex] = parent[parent[vertex]]
            vertex = int(parent[vertex])
        return vertex

    def union(u, v):
        root_u = find(u)
        root_v = find(v)
        if root_u == root_v:
            return
        low, high = sorted((root_u, root_v))
        parent[high] = low

    offsets = OFFSETS_26 if connectivity == 26 else OFFSETS_6
    yz = ny * nz
    for u in np.flatnonzero(active):
        x = int(u) // yz
        remainder = int(u) - x * yz
        y = remainder // nz
        z = remainder - y * nz
        for dx, dy, dz in offsets:
            vx, vy, vz = x + dx, y + dy, z + dz
            if 0 <= vx < nx and 0 <= vy < ny and 0 <= vz < nz:
                v = (vx * ny + vy) * nz + vz
                if active[v]:
                    union(int(u), v)

    labels = np.full(flat.size, -1, dtype=np.int32)
    for vertex in np.flatnonzero(active):
        labels[vertex] = find(int(vertex))
    return labels.reshape(shape)


def _same_partition(first, second):
    first = np.asarray(first).reshape(-1)
    second = np.asarray(second).reshape(-1)
    assert np.array_equal(first < 0, second < 0)
    active = np.flatnonzero(first >= 0)
    if active.size:
        assert np.array_equal(
            first[active, None] == first[None, active],
            second[active, None] == second[None, active],
        )


@pytest.mark.parametrize(
    "shape",
    [
        (7,),
        (3, 5),
        (2, 3, 4),
        (1, 6),
        (1, 4, 1),
        (3, 1, 2),
    ],
)
def test_msf_components_match_implicit_graph_at_every_threshold(shape):
    rng = np.random.default_rng(sum(shape) * 101)
    # Integer-valued float32 fields deliberately create multiway equal events.
    field = rng.integers(-2, 4, size=shape).astype(np.float32)
    result = cubical_msf_cpu(field)

    assert result.dims == shape
    assert result.edges.shape == (field.size - 1, 2)
    np.testing.assert_array_equal(
        result.weights,
        np.maximum(result.births[result.edges[:, 0]], result.births[result.edges[:, 1]]),
    )

    unique_births = np.unique(field)
    thresholds = [np.nextafter(unique_births[0], -np.inf, dtype=np.float32)]
    thresholds.extend(unique_births.tolist())
    thresholds.append(np.nextafter(unique_births[-1], np.inf, dtype=np.float32))
    for threshold in thresholds:
        expected = _brute_force_components(field, threshold, connectivity=26)
        actual = result.components_at_cut(threshold, reshape=True)
        np.testing.assert_array_equal(actual, expected)


def test_26_connectivity_joins_corner_touching_cells_but_6_does_not():
    field = np.full((2, 2, 2), 10.0, dtype=np.float32)
    field[0, 0, 0] = 0.0
    field[1, 1, 1] = 0.0
    result = cubical_msf_cpu(field)

    labels_26 = result.cut(0.0, reshape=True)
    brute_26 = _brute_force_components(field, 0.0, connectivity=26)
    brute_6 = _brute_force_components(field, 0.0, connectivity=6)

    np.testing.assert_array_equal(labels_26, brute_26)
    assert labels_26[0, 0, 0] == labels_26[1, 1, 1]
    assert brute_6[0, 0, 0] != brute_6[1, 1, 1]


def test_equal_events_are_bitwise_deterministic_and_axis_permutation_invariant():
    field = np.array(
        [
            [[0, 0, 2], [1, 0, 2]],
            [[1, 1, 2], [0, 0, 2]],
        ],
        dtype=np.float32,
    )
    contiguous = cubical_msf_cpu(field)
    fortran = cubical_msf_cpu(np.asfortranarray(field))

    np.testing.assert_array_equal(contiguous.births, fortran.births)
    np.testing.assert_array_equal(contiguous.edges, fortran.edges)
    np.testing.assert_array_equal(contiguous.weights, fortran.weights)

    permutation = (2, 0, 1)
    inverse = tuple(np.argsort(permutation))
    transposed = cubical_msf_cpu(field.transpose(permutation))
    for threshold in np.unique(field):
        original_labels = contiguous.cut(float(threshold), reshape=True)
        permuted_labels = transposed.cut(float(threshold), reshape=True).transpose(inverse)
        _same_partition(original_labels, permuted_labels)


@pytest.mark.parametrize("shape", [(1,), (1, 1), (1, 1, 1), (1, 5, 1), (4, 1, 1)])
def test_degenerate_dims_and_explicit_flat_dims(shape):
    field = np.linspace(-1.0, 1.0, math.prod(shape), dtype=np.float64).reshape(shape)
    shaped_result = cubical_msf_cpu(field)
    flat_result = cubical_msf_cpu(field.reshape(-1), dims=shape)

    assert shaped_result.dims == shape
    assert flat_result.dims == shape
    np.testing.assert_array_equal(shaped_result.births, flat_result.births)
    np.testing.assert_array_equal(shaped_result.edges, flat_result.edges)
    np.testing.assert_array_equal(shaped_result.weights, flat_result.weights)
    assert shaped_result.cut(np.inf, reshape=True).shape == shape


def test_input_validation_is_explicit():
    with pytest.raises(TypeError, match="floating-point"):
        cubical_msf_cpu(np.arange(8).reshape(2, 2, 2))
    with pytest.raises(ValueError, match="finite"):
        cubical_msf_cpu(np.array([0.0, np.nan], dtype=np.float32))
    with pytest.raises(ValueError, match="requires"):
        cubical_msf_cpu(np.zeros(4, dtype=np.float32), dims=(2, 3))
    with pytest.raises(ValueError, match="1-D, 2-D, or 3-D"):
        cubical_msf_cpu(np.zeros((1, 1, 1, 1), dtype=np.float32))
    with pytest.raises(ValueError, match="NaN"):
        cubical_msf_cpu(np.zeros(2, dtype=np.float32)).cut(np.nan)


def test_dispatcher_uses_the_cpu_reference():
    field = np.array([[2.0, 0.0], [1.0, 1.0]], dtype=np.float32)
    direct = cubical_msf_cpu(field)
    dispatched = build_cubical_msf(field, backend="reference")
    np.testing.assert_array_equal(direct.edges, dispatched.edges)
    np.testing.assert_array_equal(direct.weights, dispatched.weights)


def test_constant_shift_reuses_topology_for_inner_and_outer_fields():
    field = np.array(
        [[[0.5, -1.0], [2.0, 0.5]], [[3.0, 1.0], [-1.0, 4.0]]],
        dtype=np.float32,
    )
    result = cubical_msf_cpu(field)
    for delta in (-0.125, 0.125):
        shifted = result.shifted(delta)
        np.testing.assert_array_equal(shifted.edges, result.edges)
        np.testing.assert_allclose(shifted.births, result.births + delta)
        np.testing.assert_allclose(shifted.weights, result.weights + delta)
        for threshold in np.unique(field):
            np.testing.assert_array_equal(
                shifted.cut(float(threshold) + delta), result.cut(float(threshold))
            )

    with pytest.raises(ValueError, match="finite"):
        result.shifted(np.inf)


def test_memory_estimator_counts_edges_and_avoids_their_materialization():
    tiny = estimate_cubical_msf_memory((2, 2, 2), backend="gpu")
    assert tiny["n_vertices"] == 8
    assert tiny["n_tree_edges"] == 7
    # Every pair of cells in a 2x2x2 block is 26-adjacent.
    assert tiny["n_implicit_edges"] == 28
    assert tiny["materializes_implicit_edge_list"] is False
    assert tiny["explicit_edge_list_bytes_avoided"] == 28 * 16

    production = estimate_cubical_msf_memory((256, 256, 256), backend="gpu")
    assert production["n_vertices"] == 256**3
    assert production["n_tree_edges"] == 256**3 - 1
    assert production["device_peak_bytes"] > production["result_bytes"]
    assert production["explicit_edge_list_bytes_avoided"] > production["device_peak_bytes"]

    cpu64 = estimate_cubical_msf_memory(
        (8, 7), backend="cpu", activation_dtype=np.float64
    )
    assert cpu64["device_peak_bytes"] == 0
    assert cpu64["activation_dtype"] == "float64"


def test_cupy_dependency_is_lazy_and_failure_is_explicit_when_unavailable():
    if importlib.util.find_spec("cupy") is not None:
        pytest.skip("CuPy is installed; GPU execution belongs to the Colab test")
    with pytest.raises(ImportError, match="requires CuPy"):
        cubical_msf_gpu(np.zeros(2, dtype=np.float32))
