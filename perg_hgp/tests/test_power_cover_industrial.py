import math
from decimal import Decimal, localcontext

import numpy as np
import pytest
from scipy.spatial import cKDTree

from perg_hgp.backends.power_cover_3d_cuda import (
    CertifiedPowerKNN,
    ExactKDTreeIndex,
    ExactLiftedPowerKNN,
    GridCoordinateResolutionError,
    GridResolutionBudgetError,
    GridSpec,
    PowerCover3D,
    PowerCoverConfig,
    brute_force_power_topk,
    build_grid_spec,
    build_grid_spec_for_local_scale,
    cubical_msf_cpu,
    regularize_sites_streaming,
    site_component_membership_cpu,
    solve_entropy_gibbs_budget,
)
from perg_hgp.backends.power_cover_3d_cuda.api import (
    _cuda_power_candidate_plan,
    _stored_site_contract_upper,
)
from perg_hgp.backends.power_cover_3d_cuda.contracts import RegularizationDiagnostics
from perg_hgp.backends.power_cover_3d_cuda.contracts import estimate_memory
from perg_hgp.backends.power_cover_3d_cuda.spatial_core import RegularizedSites


def _local_config(**overrides):
    values = dict(
        K=5,
        kappa=2.0,
        m_reg=12,
        grid_shape=5,
        chunk_size=17,
        power_chunk_size=19,
        candidate_k_initial=5,
        candidate_k_max=200,
        neighbor_audit_queries=0,
        require_neighbor_audit=False,
        regularization_mode="local_distortion",
        distortion_budget_absolute=0.25,
        distortion_budget_relative=0.1,
        local_scale_k=5,
    )
    values.update(overrides)
    return PowerCoverConfig(**values)


def test_budgeted_gibbs_covers_minimum_active_and_uniform_regimes():
    costs = np.array(
        [[0.0, 1.0, 4.0], [0.0, 0.0, 2.0], [0.0, 1.0, 2.0]],
        dtype=np.float64,
    )
    budgets = np.array([0.0, 0.25, 10.0], dtype=np.float64)
    result = solve_entropy_gibbs_budget(costs, budgets, tolerance=1e-12)
    expected = np.sum(result.probabilities * costs, axis=1)

    np.testing.assert_array_equal(result.probabilities[0], [1.0, 0.0, 0.0])
    assert expected[1] <= budgets[1] + 1e-12
    np.testing.assert_allclose(result.probabilities[2], 1.0 / 3.0)
    assert result.effective_kappa[1] > 2.0
    np.testing.assert_allclose(result.probabilities.sum(axis=1), 1.0, atol=1e-14)


def test_local_distortion_obeys_global_multiplicative_knn_theorem():
    rng = np.random.default_rng(20260711)
    points = rng.normal(size=(180, 3)).astype(np.float32)
    K = 5
    sites = regularize_sites_streaming(
        points,
        ExactKDTreeIndex(points),
        m_reg=16,
        kappa=2.0,
        chunk_size=31,
        mode="local_distortion",
        distortion_budget_relative=0.08,
        local_scale_k=K,
    )
    queries = rng.normal(size=(70, 3)).astype(np.float64)
    original_radii = cKDTree(points.astype(np.float64)).query(queries, k=K)[0][:, K - 1]
    powers, _ = brute_force_power_topk(
        queries, sites.centers, sites.additive_weights, K, site_block=37
    )
    regularized_radii = np.sqrt(powers[:, K - 1])
    gamma = float(sites.diagnostics.relative_distortion_ratio_max)

    assert gamma < 0.5
    assert np.all(regularized_radii >= (1.0 - 2.0 * gamma) * original_radii - 2e-6)
    assert np.all(regularized_radii <= (1.0 + 2.0 * gamma) * original_radii + 2e-6)
    assert sites.diagnostics.effective_kappa_quantiles["0.5"] > 1.0


def test_exact_lifted_power_oracle_handles_more_ties_than_old_candidate_cap():
    sites = np.zeros((1_100, 3), dtype=np.float32)
    additive = np.zeros(1_100, dtype=np.float32)
    oracle = ExactLiftedPowerKNN(sites, additive, K=10)
    result = oracle.query(np.zeros((3, 3), dtype=np.float32))

    np.testing.assert_array_equal(result.radii, 0.0)
    assert result.diagnostics.certified_fraction == 1.0
    assert result.diagnostics.candidate_histogram == {"lifted_exact": 3}


def test_conditional_candidate_guard_certifies_narrow_value_interval_on_large_tie():
    sites = np.zeros((1_100, 3), dtype=np.float32)
    additive = np.zeros(1_100, dtype=np.float32)
    oracle = CertifiedPowerKNN(
        sites,
        additive,
        ExactKDTreeIndex(sites),
        K=10,
        candidate_k_initial=64,
        candidate_k_max=1_024,
        strict=True,
    )
    result = oracle.query(np.zeros((1, 3), dtype=np.float32))

    assert bool(result.certified[0])
    assert result.radii[0] == 0.0
    assert result.radius_errors[0] > 0.0
    assert result.diagnostics.candidate_histogram["1024:value_interval"] == 1


def test_local_scale_grid_is_anisotropic_bounded_and_refuses_silent_coarsening():
    sites = np.array([[0.0, 0.0, 0.0], [10.0, 2.0, 0.0]], dtype=np.float32)
    spec = build_grid_spec_for_local_scale(
        sites,
        min_resolved_radius=1.0,
        max_relative_spatial_error=0.2,
        max_cells=10_000,
    )
    assert spec.policy == "local_scale"
    assert spec.shape[0] > spec.shape[1] > spec.shape[2]
    assert 2.0 * spec.half_diagonal <= 0.2 + 1e-12

    with pytest.raises(GridResolutionBudgetError):
        build_grid_spec_for_local_scale(
            sites,
            min_resolved_radius=0.01,
            max_relative_spatial_error=0.01,
            max_cells=100,
        )


def test_local_scale_grid_accounts_for_nonzero_collapsed_axis_in_h_budget():
    sites = np.array(
        [
            [0.0, 0.0, 0.0],
            [0.54032516, 2.6796645e-4, 2.3375544e-6],
        ],
        dtype=np.float32,
    )
    radius = 1.3558794678123514e-5
    relative_error = 0.8134569689610721

    spec = build_grid_spec_for_local_scale(
        sites,
        min_resolved_radius=radius,
        max_relative_spatial_error=relative_error,
        max_cells=10**18,
    )

    assert spec.shape[2] == 1
    assert spec.cell_widths[2] > 0.0
    assert 2.0 * spec.half_diagonal <= relative_error * radius


def test_local_scale_grid_subdivides_small_but_nonzero_resolvable_axes():
    sites = np.array([[0.0, 0.0, 0.0], [1.0, 0.0, 2.3375544e-6]], dtype=np.float32)
    radius = 2.0e-6
    spec = build_grid_spec_for_local_scale(
        sites,
        min_resolved_radius=radius,
        max_relative_spatial_error=1.0,
        max_cells=2_000_000,
    )

    assert spec.shape[2] > 1
    assert spec.n_cubes <= 2_000_000
    assert 2.0 * spec.half_diagonal <= radius


def test_local_scale_grid_water_fills_thin_one_cell_axes():
    sites = np.array([[0.0, 0.0, 0.0], [10.0, 1.0e-3, 0.0]], dtype=np.float32)
    spec = build_grid_spec_for_local_scale(
        sites,
        min_resolved_radius=1.0,
        max_relative_spatial_error=1.0,
        max_cells=12,
    )

    assert spec.shape == (11, 1, 1)
    assert 2.0 * spec.half_diagonal <= 1.0


def test_grid_rejects_float32_unrepresentable_consecutive_centres():
    sites = np.array([[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]], dtype=np.float32)
    with pytest.raises(GridCoordinateResolutionError):
        build_grid_spec(sites, (20_000_000, 1, 1))


def test_config_preserves_K_mreg_independence_and_validates_local_contract():
    fixed = PowerCoverConfig(
        K=10,
        kappa=2,
        m_reg=4,
        candidate_k_initial=10,
        candidate_k_max=10,
        neighbor_audit_queries=0,
        require_neighbor_audit=False,
    )
    assert fixed.K > fixed.m_reg and fixed.local_scale_k is None
    local = _local_config(K=10, m_reg=4, local_scale_k=10, candidate_k_initial=10)
    assert local.local_scale_k == 10
    with pytest.raises(ValueError, match="< 0.5"):
        _local_config(distortion_budget_relative=0.5)


def test_cuda_candidate_plan_routes_small_and_industrial_clouds_explicitly():
    tiny = _cuda_power_candidate_plan(
        100, K=10, candidate_k_initial=64, candidate_k_max=1_024
    )
    assert tiny["index_backend"] == "cuvs_brute_force"
    assert tiny["index_max_k"] == 100

    small = _cuda_power_candidate_plan(
        1_000, K=10, candidate_k_initial=64, candidate_k_max=1_024
    )
    assert small["index_backend"] == "cuvs_brute_force"
    assert small["candidate_k_initial"] == 64
    assert small["candidate_k_max"] == 1_000
    assert small["index_max_k"] == 1_000

    unsupported_rbc_rank = _cuda_power_candidate_plan(
        30_000_000, K=10, candidate_k_initial=64, candidate_k_max=1_024
    )
    assert unsupported_rbc_rank["index_backend"] == "cuvs_brute_force"
    assert unsupported_rbc_rank["candidate_k_max"] == 1_024
    assert unsupported_rbc_rank["index_max_k"] == 1_025
    assert unsupported_rbc_rank["rbc_max_k"] == 1_024
    assert unsupported_rbc_rank["rbc_kernel_max_k"] == 1_024
    assert unsupported_rbc_rank["rbc_landmark_max_k"] == math.isqrt(30_000_000)

    industrial = _cuda_power_candidate_plan(
        30_000_000, K=10, candidate_k_initial=64, candidate_k_max=1_023
    )
    assert industrial["index_backend"] == "rbc"
    assert industrial["candidate_k_initial"] == 64
    assert industrial["candidate_k_max"] == 1_023
    assert industrial["index_max_k"] == 1_024


def test_small_cloud_memory_plan_uses_effective_regularization_support():
    config = PowerCoverConfig(
        K=10,
        kappa=2.0,
        m_reg=1_000_000,
        grid_shape=2,
        chunk_size=2_000,
        candidate_k_initial=10,
        candidate_k_max=100,
        neighbor_audit_queries=0,
        require_neighbor_audit=False,
    )
    plan = estimate_memory(1_000, config)

    assert plan.regularization_chunk_bytes < 61_000_000
    float64_plan = estimate_memory(1_000, config, input_coordinate_bytes=8)
    assert float64_plan.persistent_bytes - plan.persistent_bytes == 12_000


def test_input_integer_beyond_float64_exact_range_is_rejected():
    points = np.array([[2**53, 0, 0], [2**53 + 1, 0, 0]], dtype=np.int64)
    config = PowerCoverConfig(
        K=1,
        kappa=1,
        m_reg=1,
        grid_shape=1,
        candidate_k_initial=1,
        candidate_k_max=2,
        neighbor_audit_queries=0,
        require_neighbor_audit=False,
    )
    with pytest.raises(ValueError, match=r"2\*\*53"):
        PowerCover3D(config, backend="cpu").fit(points)


def test_denormalized_power_sites_remain_finite_beyond_float32_square_range():
    points = np.array([[0.0, 0.0, 0.0], [1.0e20, 0.0, 0.0]], dtype=np.float64)
    config = PowerCoverConfig(
        K=1,
        kappa=2,
        m_reg=2,
        grid_shape=1,
        candidate_k_initial=1,
        candidate_k_max=2,
        neighbor_audit_queries=0,
        require_neighbor_audit=False,
    )
    hierarchy = PowerCover3D(config, backend="cpu").fit(points)

    assert hierarchy.sites.additive_weights.dtype == np.float64
    assert hierarchy.sites.distortions.dtype == np.float64
    assert np.all(np.isfinite(hierarchy.sites.additive_weights))
    assert np.all(np.isfinite(hierarchy.sites.distortions))
    assert hierarchy.sites.additive_weights.max() > np.finfo(np.float32).max


def test_unrepresentable_physical_squared_scale_is_rejected_explicitly():
    points = np.array([[0.0, 0.0, 0.0], [4.0e-162, 0.0, 0.0]])
    config = PowerCoverConfig(
        K=1,
        kappa=2,
        m_reg=2,
        grid_shape=1,
        candidate_k_initial=1,
        candidate_k_max=2,
        neighbor_audit_queries=0,
        require_neighbor_audit=False,
    )
    with pytest.raises(ValueError, match="power weights are subnormal"):
        PowerCover3D(config, backend="cpu").fit(points)


def test_large_scale_field_envelopes_survive_save_load_roundtrip(tmp_path):
    points = np.array(
        [
            [0.0, 0.0, 0.0],
            [1.0e20, 0.0, 0.0],
            [0.0, 1.0e20, 0.0],
            [0.0, 0.0, 1.0e20],
        ]
    )
    config = PowerCoverConfig(
        K=1,
        kappa=1,
        m_reg=1,
        grid_shape=3,
        candidate_k_initial=1,
        candidate_k_max=4,
        neighbor_audit_queries=0,
        require_neighbor_audit=False,
    )
    hierarchy = PowerCover3D(config, backend="cpu").fit(points)
    hierarchy.save(str(tmp_path))
    loaded = type(hierarchy).load(str(tmp_path))

    np.testing.assert_array_equal(
        hierarchy.field.inner_activation, hierarchy.inner_forest.births
    )
    np.testing.assert_array_equal(
        hierarchy.field.outer_activation, hierarchy.outer_forest.births
    )
    np.testing.assert_array_equal(
        loaded.field.inner_activation, hierarchy.field.inner_activation
    )
    np.testing.assert_array_equal(
        loaded.field.outer_activation, hierarchy.field.outer_activation
    )


def test_reported_s_is_upper_for_stored_float_sites_and_bounds_are_complete():
    points = np.array(
        [[0.0, 0.0, 0.0], [1e-20, 0.0, 0.0], [1.0, 0.0, 0.0]],
        dtype=np.float32,
    )
    config = PowerCoverConfig(
        K=1,
        kappa=2,
        m_reg=2,
        grid_shape=2,
        chunk_size=2,
        candidate_k_initial=1,
        candidate_k_max=3,
        neighbor_audit_queries=0,
        require_neighbor_audit=False,
    )
    hierarchy = PowerCover3D(config, backend="cpu").fit(points)
    centers = np.asarray(hierarchy.sites.centers, dtype=np.float64)
    additive = np.asarray(hierarchy.sites.additive_weights, dtype=np.float64)
    exact_s = np.sqrt(
        np.sum((points.astype(np.float64) - centers) ** 2, axis=1) + additive
    )

    assert hierarchy.report.regularization_interleaving_radius >= float(exact_s.max())
    assert hierarchy.report.input_quantization_radius > 0.0
    base_expected = (
        hierarchy.report.input_quantization_radius
        + hierarchy.report.regularization_interleaving_radius
        + hierarchy.field.spec.half_diagonal
        + hierarchy.field.numerical_radius_error
    )
    assert hierarchy.report.base_total_interleaving_radius == pytest.approx(
        base_expected
    )
    assert hierarchy.report.field_dtype == "float64"
    assert hierarchy.report.power_oracle_exact is True
    assert hierarchy.report.conditional_guard_pass_fraction is None


def test_stored_s_rounding_envelope_is_above_the_exact_binary_quantity():
    points = np.array([[0.00412506, -0.00466416, 0.00267505]], dtype=np.float32)
    centers = np.array(
        [[1.5500484e-08, 1.7172328e-08, -8.2851992e-09]], dtype=np.float32
    )
    additive = np.array([1.8046596e-14], dtype=np.float32)
    sites = RegularizedSites(
        centers=centers,
        additive_weights=additive,
        distortions=np.zeros(1, dtype=np.float32),
        diagnostics=RegularizationDiagnostics(),
    )

    upper, _, _, _, _ = _stored_site_contract_upper(
        points,
        sites,
        chunk_size=1,
        relative_requested=False,
        min_resolved_radius=None,
    )
    with localcontext() as context:
        context.prec = 100
        exact_squared = sum(
            (
                Decimal.from_float(float(points[0, axis]))
                - Decimal.from_float(float(centers[0, axis]))
            )
            ** 2
            for axis in range(3)
        ) + Decimal.from_float(float(additive[0]))
        exact = exact_squared.sqrt()

    assert Decimal.from_float(upper) >= exact


def test_stored_absolute_budget_violation_is_not_hidden_by_float32_sites():
    points = np.random.default_rng(0).normal(size=(30, 3)).astype(np.float32)
    budget = 1.180486145177713e-5
    hierarchy = PowerCover3D(
        PowerCoverConfig(
            K=2,
            kappa=2.0,
            m_reg=8,
            grid_shape=1,
            regularization_mode="local_distortion",
            distortion_budget_absolute=budget,
            candidate_k_initial=2,
            candidate_k_max=30,
            neighbor_audit_queries=0,
            require_neighbor_audit=False,
        ),
        backend="cpu",
    ).fit(points)
    centers = np.asarray(hierarchy.sites.centers, dtype=np.float64)
    additive = np.asarray(hierarchy.sites.additive_weights, dtype=np.float64)
    observed = np.sqrt(
        np.sum((points.astype(np.float64) - centers) ** 2, axis=1) + additive
    ).max()

    assert observed > budget
    assert hierarchy.sites.diagnostics.distortion_budget_violation_max > 0.0
    assert hierarchy.sites.diagnostics.distortion_budget_violation_max >= (
        observed - budget
    )


def test_ball_aabb_membership_keeps_multivalued_component_relation():
    spec = GridSpec(
        bbox_min=(0.0, 0.0, 0.0),
        bbox_max=(3.0, 0.0, 0.0),
        shape=(3, 1, 1),
        cell_widths=(1.0, 0.0, 0.0),
        half_diagonal=0.5,
    )
    forest = cubical_msf_cpu(np.array([0.0, 10.0, 0.0]), dims=(3, 1, 1))
    membership = site_component_membership_cpu(
        np.array([[1.5, 0.0, 0.0]]),
        np.array([0.0]),
        spec,
        forest,
        forest,
        radius=0.6,
    )

    np.testing.assert_array_equal(membership.inner_for_site(0), [0, 2])
    np.testing.assert_array_equal(membership.outer_for_site(0), [0, 2])
    assert membership.summary()["status_counts"]["confirmed"] == 1


def test_end_to_end_local_report_uses_observed_gamma_and_local_grid():
    rng = np.random.default_rng(9)
    points = rng.normal(size=(80, 3)).astype(np.float32)
    hierarchy = PowerCover3D(
        _local_config(
            min_resolved_radius=1.5,
            max_relative_spatial_error=0.5,
            max_grid_cells=100_000,
        ),
        backend="cpu",
    ).fit(points)

    gamma = hierarchy.sites.diagnostics.relative_distortion_ratio_max
    assert gamma is not None and gamma < 0.5
    assert hierarchy.report.regularization_multiplicative_lower == pytest.approx(
        1.0 - 2.0 * gamma
    )
    assert hierarchy.report.grid_policy == "local_scale"
    assert hierarchy.report.relative_spatial_error_at_min_radius <= 0.5 + 1e-10
    scale_report = hierarchy.accuracy_at_scale(1.5)
    assert scale_report["envelope_relative_radius"] > 0.0
