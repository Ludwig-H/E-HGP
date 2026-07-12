import sys
import types

import numpy as np
import pytest

from perg_hgp.backends.power_cover_3d_cuda import cuda_runtime
from perg_hgp.backends.power_cover_3d_cuda.api import _cuda_raw_index_backend


class _FakePool:
    def __init__(self, *, used=0, reserved=0):
        self._used = used
        self._reserved = reserved

    def used_bytes(self):
        return self._used

    def total_bytes(self):
        return self._reserved

    def free_all_blocks(self):
        return None


class _FakeEvent:
    def record(self):
        return None

    def synchronize(self):
        return None


class _FakeCuda:
    def __init__(self):
        self.allocator = None

    Event = _FakeEvent

    @staticmethod
    def get_elapsed_time(start, stop):
        return 0.0

    def set_allocator(self, allocator):
        self.allocator = allocator


class _FakeCupy:
    float32 = np.float32
    int64 = np.int64

    def __init__(self, *, pool=None):
        self.cuda = _FakeCuda()
        self._pool = pool or _FakePool()

    @staticmethod
    def asarray(values, dtype=None):
        return np.asarray(values, dtype=dtype)

    @staticmethod
    def ascontiguousarray(values):
        return np.ascontiguousarray(values)

    @staticmethod
    def asnumpy(values):
        return np.asarray(values)

    all = staticmethod(np.all)
    any = staticmethod(np.any)
    argsort = staticmethod(np.argsort)
    take_along_axis = staticmethod(np.take_along_axis)
    isfinite = staticmethod(np.isfinite)

    def get_default_memory_pool(self):
        return self._pool


class _FakeNearestNeighbors:
    last_parameters = None
    last_query_neighbors = None

    def __init__(self, **parameters):
        type(self).last_parameters = parameters

    def fit(self, points):
        self.points = points
        return self

    def kneighbors(self, queries, *, n_neighbors, return_distance):
        assert return_distance
        type(self).last_query_neighbors = n_neighbors
        shape = (queries.shape[0], n_neighbors)
        return np.zeros(shape, dtype=np.float32), np.zeros(shape, dtype=np.int64)


def _install_fake_cuda(monkeypatch, *, pool=None):
    cp = _FakeCupy(pool=pool)
    monkeypatch.setattr(
        cuda_runtime,
        "require_cuda_stack",
        lambda: (cp, object(), _FakeNearestNeighbors),
    )
    return cp


def _install_fake_bruteforce(monkeypatch, *, squared, indices):
    cuvs = types.ModuleType("cuvs")
    neighbors = types.ModuleType("cuvs.neighbors")
    brute_force = types.SimpleNamespace(
        build=lambda points, metric: np.asarray(points),
        search=lambda index, queries, k: (
            np.asarray(squared, dtype=np.float32),
            np.asarray(indices, dtype=np.int64),
        ),
    )
    neighbors.brute_force = brute_force
    cuvs.neighbors = neighbors
    monkeypatch.setitem(sys.modules, "cuvs", cuvs)
    monkeypatch.setitem(sys.modules, "cuvs.neighbors", neighbors)


def test_rbc_refuses_cuml_hidden_brute_force_fallback(monkeypatch):
    _install_fake_cuda(monkeypatch)
    points = np.zeros((100, 3), dtype=np.float32)

    with pytest.raises(ValueError, match="refuses that hidden algorithm change"):
        cuda_runtime.RBCExactIndex(points, max_k=11)

    index = cuda_runtime.RBCExactIndex(points, max_k=10)
    assert _FakeNearestNeighbors.last_parameters["algorithm"] == "rbc"
    assert index.status == {
        "backend": "rapids_cuml_rbc_3d",
        "requested_algorithm": "rbc",
        "hidden_fallback_allowed": False,
        "empirical_audit_status": "not_run",
        "empirical_audit_passed": False,
        "numerically_proven": False,
        "internal_overfetch_factor": 4,
        "internal_overfetch_max_k": 10,
    }
    assert index.exact is False


def test_rbc_refuses_rank_above_rapids_kernel_capacity(monkeypatch):
    _install_fake_cuda(monkeypatch)
    points = np.zeros((1_050_625, 3), dtype=np.float32)

    index = cuda_runtime.RBCExactIndex(
        points, max_k=cuda_runtime.RBC_MAX_QUERY_K
    )
    assert index.max_k == 1_024
    with pytest.raises(ValueError, match="at most 1024 neighbors"):
        cuda_runtime.RBCExactIndex(points, max_k=1_025)


def test_rbc_overfetches_support_then_returns_only_requested_ranks(monkeypatch):
    _install_fake_cuda(monkeypatch)
    points = np.zeros((10_000, 3), dtype=np.float32)
    index = cuda_runtime.RBCAuditedIndex(points, max_k=10)

    distances, identifiers = index.query(points[:2], 3)

    assert index.search_k == 40
    assert _FakeNearestNeighbors.last_query_neighbors == 12
    assert distances.shape == (2, 3)
    assert identifiers.shape == (2, 3)


def test_rbc_validates_integer_parameters_and_cannot_be_preaudited(monkeypatch):
    _install_fake_cuda(monkeypatch)
    points = np.zeros((100, 3), dtype=np.float32)

    with pytest.raises(TypeError, match="max_k must be a positive integer"):
        cuda_runtime.RBCExactIndex(points, max_k=10.5)
    with pytest.raises(TypeError, match="not bool"):
        cuda_runtime.RBCExactIndex(points, max_k=True)
    with pytest.raises(ValueError, match="cannot be declared"):
        cuda_runtime.RBCExactIndex(points, max_k=10, audited=True)

    index = cuda_runtime.RBCExactIndex(points, max_k=10)
    with pytest.raises(TypeError, match="k must be a positive integer"):
        index.query(points[:1], 2.5)
    with pytest.raises(TypeError, match="not bool"):
        index.query(points[:1], True)
    with pytest.raises(ValueError, match="configured max_k=10"):
        index.query(points[:1], 11)
    with pytest.raises(TypeError, match="require_support_equivalence must be a bool"):
        index.audit_against_bruteforce(
            points[:1], k=2, require_support_equivalence="yes"
        )


@pytest.mark.parametrize(
    "queries, message",
    [
        (np.empty((0, 3), dtype=np.float32), "shape"),
        (np.zeros((2, 2), dtype=np.float32), "shape"),
        (np.array([[0.0, np.nan, 0.0]], dtype=np.float32), "NaN or Inf"),
    ],
)
def test_rbc_validates_queries_before_calling_cuml(monkeypatch, queries, message):
    _install_fake_cuda(monkeypatch)
    index = cuda_runtime.RBCExactIndex(np.zeros((100, 3)), max_k=10)
    with pytest.raises(ValueError, match=message):
        index.query(queries, 1)


def test_rbc_recomputes_and_sorts_distances_from_returned_identifier_pairs(
    monkeypatch,
):
    _install_fake_cuda(monkeypatch)
    points = np.array(
        [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [2.0, 0.0, 0.0], [3.0, 0.0, 0.0]],
        dtype=np.float32,
    )
    index = cuda_runtime.RBCAuditedIndex(points, max_k=2)
    index.model = types.SimpleNamespace(
        kneighbors=lambda queries, n_neighbors, return_distance: (
            np.array([[888.0, 999.0]], dtype=np.float32),
            np.array([[2, 0]], dtype=np.int64),
        )
    )

    distances, identifiers = index.query(points[:1], 2)

    np.testing.assert_array_equal(identifiers, np.array([[0, 2]], dtype=np.int64))
    np.testing.assert_array_equal(distances, np.array([[0.0, 2.0]], dtype=np.float32))


def test_neighbor_audit_labels_sample_evidence_as_not_a_proof():
    audit = cuda_runtime.NeighborAudit(
        queries=32,
        k=10,
        values_match=True,
        maximum_absolute_error=0.0,
        rbc_seconds=0.1,
        brute_force_seconds=0.2,
    )
    assert audit.audit_scope == "sampled_query_value_comparison"
    assert audit.is_numerical_proof is False
    assert audit.comparison_tolerance == 0.0
    assert audit.to_dict()["is_numerical_proof"] is False


def test_neighbor_audit_tolerance_scales_with_dense_knn_distances():
    exact = np.array([[1.0e-5, 2.0e-5]], dtype=np.float32)
    adjacent = np.nextafter(exact, np.float32(np.inf))
    tolerance = cuda_runtime._scale_aware_squared_distance_tolerance(
        adjacent, exact
    )

    assert np.all(np.abs(adjacent.astype(np.float64) - exact) <= tolerance)
    assert float(tolerance.max()) < 1.0e-8
    wrong_rank = exact * np.float32(1.5)
    wrong_tolerance = cuda_runtime._scale_aware_squared_distance_tolerance(
        wrong_rank, exact
    )
    assert np.any(np.abs(wrong_rank.astype(np.float64) - exact) > wrong_tolerance)


def test_tie_aware_support_accepts_only_coordinate_identical_id_aliases():
    reference_ids = np.array([[0, 1]], dtype=np.int64)
    observed_ids = np.array([[0, 2]], dtype=np.int64)
    reference_points = np.array([[[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]]])
    duplicate_points = np.array([[[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]]])
    distinct_tie_points = np.array([[[0.0, 0.0, 0.0], [-1.0, 0.0, 0.0]]])

    duplicate = cuda_runtime._tie_aware_support_summary(
        observed_ids, reference_ids, duplicate_points, reference_points
    )
    assert duplicate == {
        "identifiers_match": False,
        "support_equivalent": True,
        "identifier_mismatch_rows": 1,
        "coordinate_equivalent_tie_rows": 1,
        "non_equivalent_support_rows": 0,
    }

    distinct = cuda_runtime._tie_aware_support_summary(
        observed_ids, reference_ids, distinct_tie_points, reference_points
    )
    assert distinct["support_equivalent"] is False
    assert distinct["non_equivalent_support_rows"] == 1


def test_neighbor_audit_rejects_equal_distance_but_different_geometry(monkeypatch):
    _install_fake_cuda(monkeypatch)
    _install_fake_bruteforce(
        monkeypatch,
        squared=np.array([[0.0, 1.0]], dtype=np.float32),
        indices=np.array([[0, 1]], dtype=np.int64),
    )
    points = np.array(
        [
            [0.0, 0.0, 0.0],
            [1.0, 0.0, 0.0],
            [-1.0, 0.0, 0.0],
            [2.0, 0.0, 0.0],
        ],
        dtype=np.float32,
    )
    index = cuda_runtime.RBCAuditedIndex(points, max_k=2)
    index.query = lambda queries, k: (
        np.array([[0.0, 1.0]], dtype=np.float32),
        np.array([[0, 2]], dtype=np.int64),
    )

    audit = index.audit_against_bruteforce(points[:1], k=2)

    assert audit.distance_values_match is True
    assert audit.identifiers_match is False
    assert audit.support_equivalent is False
    assert audit.values_match is False
    assert audit.non_equivalent_support_rows == 1
    assert audit.support_equivalence_required is True
    assert index.audited is False
    assert index.audit_status == "failed"

    power_audit = index.audit_against_bruteforce(
        points[:1], k=2, require_support_equivalence=False
    )
    assert power_audit.distance_values_match is True
    assert power_audit.support_equivalent is False
    assert power_audit.support_equivalence_required is False
    assert power_audit.values_match is True
    assert index.audited is True
    assert index.audit_status == "passed"


def test_neighbor_audit_rejects_a_distance_inconsistent_with_returned_id(monkeypatch):
    _install_fake_cuda(monkeypatch)
    _install_fake_bruteforce(
        monkeypatch,
        squared=np.array([[0.0, 1.0]], dtype=np.float32),
        indices=np.array([[0, 1]], dtype=np.int64),
    )
    points = np.array(
        [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [2.0, 0.0, 0.0], [3.0, 0.0, 0.0]],
        dtype=np.float32,
    )
    index = cuda_runtime.RBCAuditedIndex(points, max_k=2)
    index.query = lambda queries, k: (
        np.array([[1.0e-3, 1.0]], dtype=np.float32),
        np.array([[0, 1]], dtype=np.int64),
    )

    audit = index.audit_against_bruteforce(points[:1], k=2)

    assert audit.values_match is False
    assert audit.distance_values_match is False
    assert audit.identifiers_match is True
    assert index.audited is False


@pytest.mark.parametrize(
    "n_points, query_k, expected",
    [
        (30_000_000, 1_024, "rbc"),
        (30_000_000, 1_025, "cuvs_brute_force"),
        (1_000, 32, "cuvs_brute_force"),
        (1_000, 31, "rbc"),
    ],
)
def test_raw_cuda_router_honors_both_rbc_capacity_limits(
    n_points, query_k, expected
):
    assert _cuda_raw_index_backend(n_points, query_k) == expected


def test_rmm_allocator_refuses_to_reset_a_live_cupy_pool(monkeypatch):
    cp = _install_fake_cuda(monkeypatch, pool=_FakePool(used=8, reserved=16))
    rmm = types.ModuleType("rmm")
    allocators = types.ModuleType("rmm.allocators")
    cupy_allocator = types.ModuleType("rmm.allocators.cupy")
    sentinel = object()
    cupy_allocator.rmm_cupy_allocator = sentinel
    monkeypatch.setitem(sys.modules, "rmm", rmm)
    monkeypatch.setitem(sys.modules, "rmm.allocators", allocators)
    monkeypatch.setitem(sys.modules, "rmm.allocators.cupy", cupy_allocator)

    with pytest.raises(RuntimeError, match="Restart the runtime"):
        cuda_runtime.configure_cupy_rmm_allocator()
    assert cp.cuda.allocator is None


def test_rmm_allocator_is_installed_only_for_an_empty_pool(monkeypatch):
    cp = _install_fake_cuda(monkeypatch)
    rmm = types.ModuleType("rmm")
    allocators = types.ModuleType("rmm.allocators")
    cupy_allocator = types.ModuleType("rmm.allocators.cupy")

    def allocator(size):
        return size

    cupy_allocator.rmm_cupy_allocator = allocator
    monkeypatch.setitem(sys.modules, "rmm", rmm)
    monkeypatch.setitem(sys.modules, "rmm.allocators", allocators)
    monkeypatch.setitem(sys.modules, "rmm.allocators.cupy", cupy_allocator)

    status = cuda_runtime.configure_cupy_rmm_allocator()
    assert cp.cuda.allocator is allocator
    assert status["rmm_reinitialized"] is False
