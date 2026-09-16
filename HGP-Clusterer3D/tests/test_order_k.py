import itertools

import numpy as np
import pytest
from scipy.optimize import linprog

from hgp_clusterer import order_k_simplices
from hgp_clusterer._geometry import order_k_delaunay
from hgp_clusterer.hypergraph import normalize


def cell_depth(X, subset):
    inside = X[list(subset)]
    outside = np.delete(X, list(subset), axis=0)
    n_rows = inside.shape[0] * outside.shape[0]
    A = np.hstack([2.0 * (outside[None, :, :] - inside[:, None, :]).reshape(n_rows, 3), np.ones((n_rows, 1))])
    b = ((outside**2).sum(axis=1)[None, :] - (inside**2).sum(axis=1)[:, None]).ravel()
    res = linprog([0.0, 0.0, 0.0, -1.0], A_ub=A, b_ub=b, bounds=[(None, None)] * 3 + [(None, 1.0)], method="highs")
    assert res.status == 0
    return -res.fun


def enclosing_radius(P):
    best = np.inf
    for size in range(1, min(len(P), 4) + 1):
        for support in itertools.combinations(range(len(P)), size):
            S = P[list(support)]
            V = S[1:] - S[0]
            y = np.linalg.lstsq(V @ V.T, 0.5 * (V * V).sum(axis=1), rcond=None)[0] if size > 1 else np.zeros(0)
            center = S[0] + V.T @ y
            radius = np.linalg.norm(S[0] - center)
            if np.all(np.linalg.norm(P - center, axis=1) <= radius * (1 + 1e-9) + 1e-12):
                best = min(best, radius)
    return best


@pytest.mark.parametrize("K", [1, 2, 3, 4])
def test_simplices_match_brute_force(K):
    X = np.random.default_rng(K).random((14, 3))
    depths = {s: cell_depth(X, s) for s in itertools.combinations(range(len(X)), K + 1)}
    assert min(abs(d) for d in depths.values()) > 1e-8
    expected = sorted(s for s, d in depths.items() if d > 0)

    simplices, squared_radii = order_k_delaunay(X, K)
    assert simplices.dtype == np.int32 and simplices.shape == (len(expected), K + 1)
    assert [tuple(row) for row in simplices.tolist()] == expected

    radii = np.array([enclosing_radius(X[list(s)]) for s in expected])
    np.testing.assert_allclose(np.sqrt(squared_radii), radii, rtol=1e-9, atol=1e-12)


def test_public_helper_is_affine_invariant():
    X = np.random.default_rng(7).normal(size=(200, 3))
    simplices, radii = order_k_simplices(X, 3)
    moved_simplices, moved_radii = order_k_simplices(250.0 * X + np.array([1e4, -3e3, 42.0]), 3)
    np.testing.assert_array_equal(simplices, moved_simplices)
    np.testing.assert_allclose(moved_radii, 250.0 * radii, rtol=1e-6)


def test_duplicated_grid_radii():
    grid = np.stack(np.meshgrid(*[np.arange(8.0)] * 3), axis=-1).reshape(-1, 3)
    Y, scale = normalize(np.vstack([grid, grid]))
    simplices, squared_radii = order_k_delaunay(Y, 4)
    points = Y[simplices]
    diameters = np.sqrt(((points[:, :, None, :] - points[:, None, :, :]) ** 2).sum(axis=-1)).max(axis=(1, 2))
    widest = np.argsort(np.sqrt(squared_radii) / diameters)[-100:]
    sample = np.concatenate([widest, np.random.default_rng(0).choice(len(simplices), 400, replace=False)])
    expected = np.array([enclosing_radius(points[i]) for i in sample])
    np.testing.assert_allclose(np.sqrt(squared_radii[sample]), expected, rtol=1e-8)


def test_far_outlier_keeps_core_geometry():
    core = np.random.default_rng(2).random((2000, 3))
    reference, reference_radii = order_k_simplices(core, 1)
    for far in (1e3, 1e7):
        simplices, radii = order_k_simplices(np.vstack([core, [[far, far, far]]]), 1)
        inner = (simplices < len(core)).all(axis=1)
        np.testing.assert_array_equal(simplices[inner], reference)
        np.testing.assert_allclose(radii[inner], reference_radii, rtol=1e-6)


def test_small_and_invalid_inputs():
    rng = np.random.default_rng(0)
    for n in range(9):
        for K in range(1, 9):
            simplices, radii = order_k_simplices(rng.random((n, 3)), K)
            assert simplices.shape[1] == K + 1 and radii.shape == (simplices.shape[0],)
            if n < K + 1:
                assert simplices.shape[0] == 0
            elif n <= 3 or n == K + 1:
                assert simplices.shape[0] == len(list(itertools.combinations(range(n), K + 1)))
    X = rng.random((20, 3))
    for K in (0, 64, 2.5, True, "2"):
        with pytest.raises(ValueError):
            order_k_simplices(X, K)
    with pytest.raises(ValueError):
        order_k_simplices(np.vstack([X, [[1e12, 0.0, 0.0]]]), 2)
    for bad in (np.nan, np.inf):
        X[3, 1] = bad
        with pytest.raises(ValueError):
            order_k_simplices(X, 2)
