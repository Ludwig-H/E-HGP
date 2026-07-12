import sys
import types

import numpy as np
import pytest

from perg_hgp.backends.power_cover_3d_cuda import cuda_runtime


class _FakePool:
    def __init__(self, *, used=0, reserved=0):
        self._used = used
        self._reserved = reserved

    def used_bytes(self):
        return self._used

    def total_bytes(self):
        return self._reserved


class _FakeCuda:
    def __init__(self):
        self.allocator = None

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

    all = staticmethod(np.all)
    isfinite = staticmethod(np.isfinite)

    def get_default_memory_pool(self):
        return self._pool


class _FakeNearestNeighbors:
    last_parameters = None

    def __init__(self, **parameters):
        type(self).last_parameters = parameters

    def fit(self, points):
        self.points = points
        return self

    def kneighbors(self, queries, *, n_neighbors, return_distance):
        assert return_distance
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
