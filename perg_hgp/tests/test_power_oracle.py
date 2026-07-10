from __future__ import annotations

from itertools import combinations

import numpy as np

from perg_hgp.backends.power_cover_3d_cuda.spatial_core import (
    ExactKDTreeIndex,
    regularize_sites_streaming,
)
from perg_hgp.reference.power_oracle import (
    blocked_power_topk,
    build_component_hierarchy,
    components_at_threshold,
    enumerate_power_complex,
    solve_additive_miniball_3d,
)


def _partition_as_site_facets(complex_, threshold):
    state = components_at_threshold(complex_, threshold)
    return frozenset(
        frozenset(tuple(int(value) for value in complex_.facets[index]) for index in part)
        for part in state.components
    )


def test_blocked_power_topk_is_stable_across_blocks_and_ties():
    sites = np.asarray(
        [
            [-1.0, 0.0, 0.0],
            [1.0, 0.0, 0.0],
            [0.0, -1.0, 0.0],
            [0.0, 1.0, 0.0],
            [3.0, 0.0, 0.0],
        ]
    )
    queries = np.asarray([[0.0, 0.0, 0.0], [2.0, 0.0, 0.0]])
    weights = np.zeros(sites.shape[0])

    values_one, ids_one = blocked_power_topk(
        queries, sites, weights, 4, query_block=1, site_block=1
    )
    values_many, ids_many = blocked_power_topk(
        queries, sites, weights, 4, query_block=9, site_block=9
    )

    np.testing.assert_array_equal(values_one, values_many)
    np.testing.assert_array_equal(ids_one, ids_many)
    # Four exact ties at the origin must be decided only by the site id.
    np.testing.assert_array_equal(ids_one[0], [0, 1, 2, 3])
    np.testing.assert_array_equal(values_one[0], np.ones(4))


def test_additive_miniball_has_float64_primal_dual_certificate():
    tetrahedron = np.asarray(
        [
            [1.0, 1.0, 1.0],
            [1.0, -1.0, -1.0],
            [-1.0, 1.0, -1.0],
            [-1.0, -1.0, 1.0],
        ],
        dtype=np.float64,
    )
    result = solve_additive_miniball_3d(tetrahedron, np.zeros(4))

    assert result.feasible
    assert result.center.dtype == np.float64
    assert result.multipliers.dtype == np.float64
    assert len(result.support) == 4
    np.testing.assert_allclose(result.center, 0.0, atol=1e-13)
    np.testing.assert_allclose(result.power_radius, 3.0, rtol=1e-13)
    np.testing.assert_allclose(result.radius, np.sqrt(3.0), rtol=1e-13)
    assert result.primal_residual <= 1e-13
    assert result.dual_residual <= 1e-12
    assert result.active_residual <= 1e-12
    assert result.stationarity_residual <= 1e-13
    assert result.simplex_residual <= 1e-13
    assert result.minimum_multiplier >= 0.0


def test_additive_miniball_handles_dominant_weight_and_duplicates():
    sites = np.asarray(
        [
            [0.0, 0.0, 0.0],
            [0.0, 0.0, 0.0],
            [1.0, 0.0, 0.0],
            [1.0, 0.0, 0.0],
        ]
    )
    weights = np.asarray([0.0, 4.0, 0.0, 0.0])
    result = solve_additive_miniball_3d(sites, weights)

    assert result.feasible
    assert len(result.support) <= 4
    np.testing.assert_allclose(result.center, [0.0, 0.0, 0.0], atol=1e-13)
    np.testing.assert_allclose(result.power_radius, 4.0, atol=1e-13)
    assert result.support == (1,)
    np.testing.assert_allclose(result.multipliers, [1.0])


def test_kappa_one_recovers_unregularized_sites_and_zero_weights():
    points = np.asarray(
        [
            [0.0, 0.0, 0.0],
            [0.5, 0.2, 0.1],
            [1.1, -0.3, 0.4],
            [-0.4, 0.8, -0.2],
            [0.2, -0.6, 1.0],
        ],
        dtype=np.float64,
    )
    regularized = regularize_sites_streaming(
        points,
        ExactKDTreeIndex(points),
        m_reg=4,
        kappa=1.0,
        chunk_size=2,
    )

    np.testing.assert_allclose(regularized.centers, points, atol=0.0, rtol=0.0)
    np.testing.assert_allclose(regularized.additive_weights, 0.0, atol=0.0, rtol=0.0)
    np.testing.assert_allclose(regularized.distortions, 0.0, atol=0.0, rtol=0.0)


def test_complete_six_point_k2_oracle_does_not_apply_gabriel_filter():
    # Site 3 lies strictly inside the unweighted miniball of triangle (0,1,2).
    # A Gabriel oracle could reject that triangle; the complete oracle must not.
    sites = np.asarray(
        [
            [-1.0, 0.0, 0.0],
            [1.0, 0.0, 0.0],
            [0.0, 2.0, 0.0],
            [0.0, 0.0, 0.0],
            [0.0, 0.0, 1.0],
            [0.3, -0.8, -0.5],
        ]
    )
    complex_ = enumerate_power_complex(sites, np.zeros(6), K=2)

    assert complex_.n_facets == 15
    assert complex_.n_cofaces == 20
    assert complex_.n_edges == 60
    assert (0, 1, 2) in {tuple(row) for row in complex_.cofaces.tolist()}
    assert all(ball.feasible for ball in complex_.facet_miniballs)
    assert all(ball.feasible for ball in complex_.coface_miniballs)

    facet_lookup = {tuple(row): i for i, row in enumerate(complex_.facets.tolist())}
    for coface_index, coface in enumerate(complex_.cofaces.tolist()):
        for facet in combinations(coface, 2):
            facet_index = facet_lookup[tuple(facet)]
            assert (
                complex_.facet_births[facet_index]
                <= complex_.coface_births[coface_index] + 1e-11
            )


def test_equal_events_are_atomic_and_isolated_facets_are_present():
    # Three edges incident to site 0 have exactly the same birth radius 1.
    sites = np.asarray(
        [
            [0.0, 0.0, 0.0],
            [2.0, 0.0, 0.0],
            [-2.0, 0.0, 0.0],
            [0.0, 2.0, 0.0],
        ]
    )
    complex_ = enumerate_power_complex(sites, np.zeros(4), K=1)

    before = components_at_threshold(complex_, 0.5)
    assert before.components == ((0,), (1,), (2,), (3,))

    at_event = components_at_threshold(complex_, 1.0)
    assert at_event.components == ((0, 1, 2, 3),)

    hierarchy = build_component_hierarchy(complex_, event_rtol=0.0, event_atol=1e-13)
    equal_level = next(level for level in hierarchy if abs(level.radius - 1.0) <= 1e-13)
    assert set(equal_level.born_cofaces) == {0, 1, 2}
    assert equal_level.components == ((0, 1, 2, 3),)


def test_k1_is_exact_single_linkage_with_pair_distance_over_two():
    sites = np.asarray(
        [
            [0.0, 0.0, 0.0],
            [1.0, 0.0, 0.0],
            [2.6, 0.0, 0.0],
            [0.0, 3.0, 0.0],
            [0.0, 3.8, 0.0],
        ]
    )
    complex_ = enumerate_power_complex(sites, np.zeros(5), K=1)

    np.testing.assert_allclose(complex_.facet_births, 0.0, atol=0.0)
    for coface, birth in zip(complex_.cofaces, complex_.coface_births):
        left, right = (int(coface[0]), int(coface[1]))
        expected = np.linalg.norm(sites[left] - sites[right]) / 2.0
        np.testing.assert_allclose(birth, expected, rtol=1e-13, atol=1e-13)

    for threshold in (0.0, 0.45, 0.6, 0.85, 1.5, 10.0):
        parent = np.arange(sites.shape[0])

        def find(item):
            while parent[item] != item:
                parent[item] = parent[parent[item]]
                item = parent[item]
            return int(item)

        def union(left, right):
            left_root, right_root = find(left), find(right)
            if left_root != right_root:
                parent[max(left_root, right_root)] = min(left_root, right_root)

        for left, right in combinations(range(sites.shape[0]), 2):
            if np.linalg.norm(sites[left] - sites[right]) <= 2.0 * threshold:
                union(left, right)
        expected_groups = {}
        for site_index in range(sites.shape[0]):
            expected_groups.setdefault(find(site_index), []).append(site_index)
        expected = tuple(
            tuple(group)
            for group in sorted(expected_groups.values(), key=lambda group: group[0])
        )
        assert components_at_threshold(complex_, threshold).components == expected


def test_complex_births_and_components_are_permutation_equivariant():
    sites = np.asarray(
        [
            [-0.7, 0.1, 0.3],
            [0.4, -0.2, 0.8],
            [1.2, 0.5, -0.4],
            [-0.1, 1.4, 0.6],
            [0.8, 1.1, 1.5],
        ]
    )
    weights = np.asarray([0.05, 0.2, 0.0, 0.4, 0.1])
    permutation = np.asarray([3, 0, 4, 1, 2])  # new id -> original id
    original = enumerate_power_complex(sites, weights, K=2)
    permuted = enumerate_power_complex(
        sites[permutation], weights[permutation], K=2
    )

    original_facet_births = {
        tuple(row): birth
        for row, birth in zip(original.facets.tolist(), original.facet_births)
    }
    permuted_facet_births = {
        tuple(sorted(int(permutation[index]) for index in row)): birth
        for row, birth in zip(permuted.facets.tolist(), permuted.facet_births)
    }
    original_coface_births = {
        tuple(row): birth
        for row, birth in zip(original.cofaces.tolist(), original.coface_births)
    }
    permuted_coface_births = {
        tuple(sorted(int(permutation[index]) for index in row)): birth
        for row, birth in zip(permuted.cofaces.tolist(), permuted.coface_births)
    }

    assert original_facet_births.keys() == permuted_facet_births.keys()
    assert original_coface_births.keys() == permuted_coface_births.keys()
    for facet in original_facet_births:
        np.testing.assert_allclose(
            original_facet_births[facet], permuted_facet_births[facet], atol=2e-12
        )
    for coface in original_coface_births:
        np.testing.assert_allclose(
            original_coface_births[coface], permuted_coface_births[coface], atol=2e-12
        )

    threshold = float(np.median(original.coface_births))
    original_partition = _partition_as_site_facets(original, threshold)
    permuted_state = components_at_threshold(permuted, threshold, atol=2e-12)
    mapped_partition = frozenset(
        frozenset(
            tuple(sorted(int(permutation[index]) for index in permuted.facets[facet]))
            for facet in component
        )
        for component in permuted_state.components
    )
    assert mapped_partition == original_partition
