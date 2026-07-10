import json

import numpy as np
import pytest

from perg_hgp.backends.power_cover_3d_cuda.api import (
    PowerCover3D,
    pilot_kappa_calibration,
)
from perg_hgp.backends.power_cover_3d_cuda.contracts import (
    GridSpec,
    PowerCoverConfig,
    SpatialTransform,
    estimate_memory,
)
from perg_hgp.backends.power_cover_3d_cuda.spatial_core import (
    CertifiedPowerKNN,
    ExactKDTreeIndex,
    PowerKNNCertificationError,
    brute_force_power_topk,
    build_grid_spec,
    evaluate_cubical_field,
    regularize_sites_streaming,
    solve_entropy_gibbs,
)


def test_config_keeps_cover_entropy_and_support_independent():
    config = PowerCoverConfig(
        K=10,
        kappa=3.5,
        m_reg=17,
        grid_shape=4,
        candidate_k_initial=10,
        candidate_k_max=32,
        neighbor_audit_queries=0,
        require_neighbor_audit=False,
    )
    assert config.K == 10
    assert config.kappa == 3.5
    assert config.m_reg == 17
    assert config.grid_shape == (4, 4, 4)


@pytest.mark.parametrize(
    "kwargs",
    [
        {"K": True},
        {"m_reg": 4.5},
        {"grid_shape": (4, 2.5, 3)},
        {"chunk_size": False},
        {"candidate_k_initial": 10.0},
        {"candidate_k_max": np.inf},
        {"neighbor_audit_queries": 1.0},
        {"strict_certification": 1},
        {"require_neighbor_audit": 0},
        {"kappa": np.nan},
        {"numerical_margin_factor": np.inf},
        {"max_vram_fraction": np.nan},
    ],
)
def test_config_rejects_bool_non_integer_and_non_finite_values(kwargs):
    with pytest.raises((TypeError, ValueError)):
        PowerCoverConfig(**kwargs)


def test_grid_and_transform_contracts_reject_non_finite_or_mistyped_values():
    with pytest.raises(TypeError):
        GridSpec((0, 0, 0), (1, 1, 1), (2, True, 2), (0.5, 0.5, 0.5), 1.0)
    with pytest.raises(ValueError):
        GridSpec((0, 0, 0), (1, np.inf, 1), (2, 2, 2), (0.5, 0.5, 0.5), 1.0)
    with pytest.raises(ValueError):
        GridSpec((0, 0, 0), (1, 1, 1), (2, 2, 2), (0.5, -0.5, 0.5), 1.0)
    with pytest.raises(ValueError):
        SpatialTransform((0.0, np.nan, 0.0), 1.0)
    with pytest.raises(TypeError):
        SpatialTransform((0.0, 0.0, 0.0), True)


def test_memory_plan_for_30m_has_no_global_neighbor_matrix():
    config = PowerCoverConfig(
        grid_shape=(256, 256, 256),
        neighbor_audit_queries=0,
        require_neighbor_audit=False,
    )
    plan = estimate_memory(30_000_000, config)
    assert plan.n_cubes == 256**3
    assert plan.regularization_chunk_bytes < 2**34
    assert plan.estimated_peak_bytes < 69 * 2**30
    assert plan.power_query_chunk_bytes < 2 * 2**30
    assert "fused power kernel; no Q x C x 3 candidate tensors" in plan.assumptions
    assert "implicit 26-neighbor grid; no explicit grid edge list" in plan.assumptions


def test_gibbs_limit_cases_and_tied_minima():
    costs = np.array([[0.0, 1.0, 4.0], [0.0, 0.0, 2.0]], dtype=np.float64)
    delta = solve_entropy_gibbs(costs, 1.0)
    np.testing.assert_array_equal(delta.probabilities[0], [1.0, 0.0, 0.0])
    uniform = solve_entropy_gibbs(costs, 3.0)
    np.testing.assert_allclose(uniform.probabilities, 1.0 / 3.0)
    tied = solve_entropy_gibbs(costs[1:], 1.5)
    np.testing.assert_allclose(tied.probabilities[0], [0.5, 0.5, 0.0])
    assert tied.entropy[0] >= np.log(1.5)
    assert tied.constraint_residual[0] == 0.0


def test_gibbs_strict_tiny_gap_is_not_misclassified_as_a_tie():
    costs = np.array([[0.0, 1e-20, 1.0]], dtype=np.float64)
    result = solve_entropy_gibbs(costs, 1.5, tolerance=1e-12)

    assert result.probabilities[0, 0] > result.probabilities[0, 1] > 0.0
    assert result.probabilities[0, 2] < 1e-100
    assert result.temperatures[0] > 0.0
    assert result.entropy[0] == pytest.approx(np.log(1.5), abs=1e-12)
    assert result.constraint_residual[0] <= 1e-12
    np.testing.assert_allclose(result.probabilities.sum(axis=1), 1.0, atol=1e-15)


def test_gibbs_exact_ties_and_strict_gaps_take_different_branches():
    costs = np.array(
        [[0.0, 0.0, 1.0], [0.0, np.nextafter(0.0, 1.0), 1.0]],
        dtype=np.float64,
    )
    result = solve_entropy_gibbs(costs, 1.5, tolerance=1e-12)

    np.testing.assert_array_equal(result.probabilities[0], [0.5, 0.5, 0.0])
    assert result.temperatures[0] == 0.0
    assert result.entropy[0] == pytest.approx(np.log(2.0))
    assert result.probabilities[1, 1] > 0.0
    assert result.temperatures[1] >= 0.0
    assert result.entropy[1] == pytest.approx(np.log(1.5), abs=1e-12)


def test_gibbs_probabilities_are_invariant_to_positive_cost_scaling():
    costs = np.array([[0.0, 1e-20, 1.0], [0.0, 0.25, 0.5]], dtype=np.float64)
    baseline = solve_entropy_gibbs(costs, 1.5, tolerance=1e-12)
    scaled = solve_entropy_gibbs(costs * 1e100, 1.5, tolerance=1e-12)

    np.testing.assert_allclose(
        scaled.probabilities, baseline.probabilities, rtol=1e-12, atol=1e-14
    )
    np.testing.assert_allclose(
        scaled.temperatures, baseline.temperatures * 1e100, rtol=1e-12
    )


def test_regularization_kappa_one_is_identity_and_counts_self():
    rng = np.random.default_rng(3)
    points = rng.normal(size=(31, 3)).astype(np.float32)
    index = ExactKDTreeIndex(points)
    result = regularize_sites_streaming(
        points, index, m_reg=9, kappa=1.0, chunk_size=7
    )
    np.testing.assert_allclose(result.centers, points, atol=0.0, rtol=0.0)
    np.testing.assert_allclose(result.additive_weights, 0.0, atol=1e-12)
    np.testing.assert_allclose(result.distortions, 0.0, atol=1e-12)


def test_regularization_is_chunk_invariant():
    rng = np.random.default_rng(4)
    points = rng.normal(size=(43, 3)).astype(np.float32)
    index = ExactKDTreeIndex(points)
    first = regularize_sites_streaming(
        points, index, m_reg=12, kappa=4.0, chunk_size=5
    )
    second = regularize_sites_streaming(
        points, index, m_reg=12, kappa=4.0, chunk_size=43
    )
    np.testing.assert_allclose(first.centers, second.centers)
    np.testing.assert_allclose(first.additive_weights, second.additive_weights)
    np.testing.assert_allclose(first.distortions, second.distortions)


def test_power_candidate_guard_matches_blocked_bruteforce():
    rng = np.random.default_rng(5)
    sites = rng.normal(size=(101, 3)).astype(np.float32)
    additive = rng.lognormal(mean=-2.0, sigma=1.0, size=101).astype(np.float32)
    queries = rng.normal(size=(19, 3)).astype(np.float32)
    index = ExactKDTreeIndex(sites)
    oracle = CertifiedPowerKNN(
        sites,
        additive,
        index,
        K=10,
        candidate_k_initial=16,
        candidate_k_max=101,
    )
    result = oracle.query(queries)
    exact, _ = brute_force_power_topk(queries, sites, additive, 10, site_block=13)
    np.testing.assert_allclose(result.power_values, exact[:, -1], rtol=2e-6, atol=2e-6)
    assert result.diagnostics.certified_fraction == 1.0


def test_power_guard_escalates_when_close_sites_have_large_weights():
    sites = np.column_stack(
        (np.arange(12, dtype=np.float32), np.zeros(12), np.zeros(12))
    )
    additive = np.array([100.0] * 5 + [0.0] * 7, dtype=np.float32)
    assert sites.dtype == np.float64 and additive.dtype == np.float32
    queries = np.array([[0.0, 0.0, 0.0]], dtype=np.float32)
    oracle = CertifiedPowerKNN(
        sites,
        additive,
        ExactKDTreeIndex(sites),
        K=2,
        candidate_k_initial=2,
        candidate_k_max=12,
    )
    assert oracle.sites.dtype == sites.dtype
    assert oracle.additive.dtype == additive.dtype
    result = oracle.query(queries)
    assert result.candidates_used.item() > 2
    exact, _ = brute_force_power_topk(queries, sites, additive, 2)
    np.testing.assert_allclose(result.power_values, exact[:, -1], rtol=1e-6)


@pytest.mark.parametrize(
    ("site_dtype", "additive_dtype"),
    [
        (np.float32, np.float32),
        (np.float64, np.float32),
        (np.float32, np.float64),
        (np.float64, np.float64),
    ],
)
def test_power_guard_supports_mixed_float_dtypes(
    monkeypatch, site_dtype, additive_dtype
):
    sites = np.column_stack(
        (
            np.arange(8, dtype=site_dtype),
            np.zeros(8, dtype=site_dtype),
            np.zeros(8, dtype=site_dtype),
        )
    )
    additive = np.array([9.0] * 3 + [0.0] * 5, dtype=additive_dtype)
    queries = np.array([[0.25, 0.0, 0.0]], dtype=additive_dtype)

    # Emulate NumPy 2.0's strict ``take(..., out=...)`` dtype contract even
    # when this suite runs on a newer NumPy release.
    original_take = np.take

    def strict_take(array, indices, *args, out=None, **kwargs):
        if out is not None and np.asarray(array).dtype != out.dtype:
            raise TypeError("mixed source/out dtype rejected")
        return original_take(array, indices, *args, out=out, **kwargs)

    monkeypatch.setattr(np, "take", strict_take)
    oracle = CertifiedPowerKNN(
        sites,
        additive,
        ExactKDTreeIndex(sites),
        K=2,
        candidate_k_initial=2,
        candidate_k_max=8,
    )
    result = oracle.query(queries)
    exact, _ = brute_force_power_topk(queries, sites, additive, 2)
    assert result.power_values.dtype == np.dtype(site_dtype)
    np.testing.assert_allclose(result.power_values, exact[:, -1], rtol=1e-6, atol=1e-6)


def test_uncertified_power_query_is_never_silently_accepted():
    sites = np.column_stack(
        (np.arange(20, dtype=np.float32), np.zeros(20), np.zeros(20))
    )
    additive = np.array([1_000.0] * 10 + [0.0] * 10, dtype=np.float32)
    oracle = CertifiedPowerKNN(
        sites,
        additive,
        ExactKDTreeIndex(sites),
        K=2,
        candidate_k_initial=2,
        candidate_k_max=2,
        strict=True,
    )
    with pytest.raises(PowerKNNCertificationError):
        oracle.query(np.zeros((1, 3), dtype=np.float32))


def test_grid_collapses_constant_axes_and_bounds_radius_not_power():
    sites = np.array([[0.0, 2.0, 3.0], [4.0, 2.0, 3.0]], dtype=np.float32)
    weights = np.zeros(2, dtype=np.float32)
    spec = build_grid_spec(sites, (4, 8, 8))
    assert spec.shape == (4, 1, 1)
    oracle = CertifiedPowerKNN(
        sites,
        weights,
        ExactKDTreeIndex(sites),
        K=1,
        candidate_k_initial=1,
        candidate_k_max=2,
    )
    field = evaluate_cubical_field(oracle, spec, chunk_size=2)
    effective = spec.half_diagonal + field.numerical_radius_error
    np.testing.assert_allclose(field.inner_activation - field.radii, effective)
    np.testing.assert_allclose(field.radii - field.outer_activation, effective)


def test_pilot_kappa_selection_obeys_requested_sample_budget():
    rng = np.random.default_rng(6)
    points = rng.normal(size=(80, 3)).astype(np.float32)
    index = ExactKDTreeIndex(points)
    rows_budget = 0.8
    chosen, rows = pilot_kappa_calibration(
        points,
        index,
        m_reg=16,
        candidates=(1, 2, 4, 8),
        sample_size=32,
        distortion_budget=rows_budget,
    )
    assert chosen == max(row.kappa for row in rows if row.s_max <= rows_budget)


def test_end_to_end_cpu_builds_two_shifted_hierarchies():
    rng = np.random.default_rng(7)
    points = np.concatenate(
        (
            rng.normal((-1.0, 0.0, 0.0), 0.15, size=(25, 3)),
            rng.normal((1.0, 0.0, 0.0), 0.15, size=(25, 3)),
        )
    ).astype(np.float32)
    config = PowerCoverConfig(
        K=3,
        kappa=2.0,
        m_reg=8,
        grid_shape=(5, 4, 3),
        chunk_size=11,
        candidate_k_initial=8,
        candidate_k_max=50,
        neighbor_audit_queries=0,
        require_neighbor_audit=False,
    )
    hierarchy = PowerCover3D(config, backend="cpu").fit(points)
    h = (
        hierarchy.field.spec.half_diagonal
        + hierarchy.field.numerical_radius_error
    )
    np.testing.assert_allclose(
        hierarchy.inner_forest.weights, hierarchy.base_forest.weights + h
    )
    np.testing.assert_allclose(
        hierarchy.outer_forest.weights, hierarchy.base_forest.weights - h
    )
    assert hierarchy.report.total_interleaving_radius == pytest.approx(
        hierarchy.sites.diagnostics.s_max + 2.0 * h
    )
    assert hierarchy.report.mode == "spatial_enveloped"
    assert hierarchy.report.flat_selection == "none"
    assert hierarchy.report.full_power_hgp_complete is False


def test_saved_hierarchy_is_atomic_versioned_and_strict_json(tmp_path):
    points = np.array(
        [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]],
        dtype=np.float32,
    )
    config = PowerCoverConfig(
        K=1,
        kappa=1.0,
        m_reg=1,
        grid_shape=2,
        chunk_size=2,
        power_chunk_size=2,
        candidate_k_initial=3,
        candidate_k_max=3,
        neighbor_audit_queries=0,
        require_neighbor_audit=False,
    )
    hierarchy = PowerCover3D(config, backend="cpu").fit(points)
    hierarchy.save(str(tmp_path), include_sites=True)
    raw = (tmp_path / "run_report.json").read_text(encoding="utf-8")
    assert "Infinity" not in raw and "NaN" not in raw
    metadata = json.loads(raw)
    assert metadata["schema"] == "perg_hgp.power_cover_run"
    assert metadata["schema_version"] == 1
    with np.load(tmp_path / "cubical_hierarchy.npz") as artifact:
        assert int(artifact["schema_version"]) == 1
        assert artifact["field_certified"].dtype == np.bool_
        assert artifact["field_certified"].all()
    assert not list(tmp_path.glob("*.tmp*"))


def test_end_to_end_is_translation_and_scale_covariant():
    rng = np.random.default_rng(8)
    points = rng.normal(size=(24, 3)).astype(np.float32)
    config = PowerCoverConfig(
        K=2,
        kappa=2.0,
        m_reg=6,
        grid_shape=3,
        chunk_size=7,
        candidate_k_initial=4,
        candidate_k_max=24,
        neighbor_audit_queries=0,
        require_neighbor_audit=False,
    )
    base = PowerCover3D(config, backend="cpu").fit(points)
    factor = 17.0
    translated = points * factor + np.array([100.0, -40.0, 7.0], dtype=np.float32)
    changed = PowerCover3D(config, backend="cpu").fit(translated)
    np.testing.assert_allclose(
        changed.base_forest.births, base.base_forest.births * factor, rtol=3e-5, atol=3e-5
    )
    np.testing.assert_array_equal(changed.base_forest.edges, base.base_forest.edges)
    assert changed.report.total_interleaving_radius == pytest.approx(
        base.report.total_interleaving_radius * factor, rel=3e-5
    )


def test_float64_large_offset_is_recentered_before_float32_cast():
    offsets = np.array(
        [
            [0.0, 0.0, 0.0],
            [0.25, 0.0, 0.0],
            [0.0, 0.5, 0.0],
            [0.0, 0.0, 0.75],
        ],
        dtype=np.float64,
    )
    points = offsets + np.array([1.0e9, -1.0e9, 2.0e9], dtype=np.float64)
    config = PowerCoverConfig(
        K=1,
        kappa=1.0,
        m_reg=1,
        grid_shape=2,
        chunk_size=3,
        candidate_k_initial=1,
        candidate_k_max=4,
        neighbor_audit_queries=0,
        require_neighbor_audit=False,
    )
    hierarchy = PowerCover3D(config, backend="cpu").fit(points)
    assert hierarchy.sites.centers.dtype == np.float64
    np.testing.assert_array_equal(hierarchy.sites.centers, points)
    assert np.unique(hierarchy.sites.centers, axis=0).shape[0] == points.shape[0]


def test_non_strict_uncertain_field_never_advertises_interleaving_bounds():
    rng = np.random.default_rng(0)
    points = np.concatenate(
        (
            rng.normal((0.0, 0.0, 0.0), 0.01, size=(8, 3)),
            rng.normal((3.0, 0.0, 0.0), 1.0, size=(8, 3)),
        )
    ).astype(np.float32)
    config = PowerCoverConfig(
        K=1,
        kappa=4.0,
        m_reg=8,
        grid_shape=4,
        chunk_size=8,
        candidate_k_initial=1,
        candidate_k_max=1,
        strict_certification=False,
        neighbor_audit_queries=0,
        require_neighbor_audit=False,
    )
    hierarchy = PowerCover3D(config, backend="cpu").fit(points)
    assert hierarchy.field.diagnostics.uncertain_queries > 0
    assert hierarchy.report.mode == "spatial_uncertain"
    assert hierarchy.report.spatial_interleaving_radius is None
    assert hierarchy.report.total_interleaving_radius is None
    with pytest.raises(RuntimeError, match="declared envelope"):
        hierarchy.fusion_intervals()
