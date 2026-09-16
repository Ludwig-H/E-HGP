import subprocess
import sys

import numpy as np
import pytest
from sklearn.base import clone
from sklearn.metrics import adjusted_rand_score

from hgp_clusterer import HGPClusterer


@pytest.fixture(scope="module")
def cloud():
    rng = np.random.default_rng(0)
    centers = np.array([[0.0, 0.0, 0.0], [6.0, 0.0, 0.0], [0.0, 6.0, 3.0]])
    blobs = [c + rng.normal(scale=0.6, size=(300, 3)) for c in centers]
    noise = rng.uniform(-4.0, 10.0, size=(90, 3))
    X = np.vstack(blobs + [noise])
    y = np.repeat([0, 1, 2, -1], [300, 300, 300, 90])
    return X, y


@pytest.fixture(scope="module")
def fitted(cloud):
    X, _ = cloud
    return HGPClusterer(K=2, min_cluster_size=30).fit(X)


def test_recovers_blobs(cloud, fitted):
    X, y = cloud
    labels = fitted.labels_
    assert labels.shape == (X.shape[0],)
    assert set(np.unique(labels)) == {-1, 0, 1, 2}
    core = y >= 0
    assert adjusted_rand_score(y[core], labels[core]) > 0.95
    np.testing.assert_allclose(fitted.W_nodes_.sum(), np.count_nonzero(fitted.T_points_), rtol=1e-4)


def test_refinement_reuses_hierarchy(cloud, fitted):
    X, _ = cloud
    model = clone(fitted).fit(X)
    forest = model.forest_
    eom = model.labels_.copy()

    leaf = model.refine_clusters("leaf")
    assert len(np.unique(leaf[leaf >= 0])) >= len(np.unique(eom[eom >= 0]))
    split_all = model.refine_clusters("eom", splitting=lambda parent, children: True)
    assert adjusted_rand_score(leaf, split_all) == 1.0
    keep_all = model.refine_clusters("eom", splitting=lambda parent, children: False)
    np.testing.assert_array_equal(keep_all, eom)

    coarse = model.refine_clusters(100.0)
    assert np.all(coarse == 0)
    fine = model.refine_clusters(1e-6)
    assert np.all(fine == -1)
    assert model.forest_ is forest

    cut = model.refine_clusters(0.8).copy()
    model.set_params(expZ=4.0)
    np.testing.assert_array_equal(model.refine_clusters(0.8), cut)


def test_invariance_to_similarity(cloud, fitted):
    X, _ = cloud
    moved = HGPClusterer(K=2, min_cluster_size=30).fit(3.5 * X + np.array([-120.0, 7.0, 1e3]))
    assert adjusted_rand_score(fitted.labels_, moved.labels_) > 0.999
    cut = fitted.refine_clusters(0.8)
    assert adjusted_rand_score(cut, moved.refine_clusters(3.5 * 0.8)) > 0.999


@pytest.mark.parametrize("K", [1, 3])
def test_other_orders(cloud, K):
    X, y = cloud
    labels = HGPClusterer(K=K, min_cluster_size=30).fit_predict(X)
    assert labels.shape == y.shape and labels.max() >= 1


def test_groups_at_the_size_threshold():
    rng = np.random.default_rng(3)
    centers = 10.0 * np.stack(np.meshgrid(np.arange(4), np.arange(2), np.arange(5)), axis=-1).reshape(-1, 3)
    X = np.vstack([c + rng.normal(scale=0.1, size=(5, 3)) for c in centers])
    for method in ("eom", "leaf"):
        labels = HGPClusterer(K=1, min_cluster_size=5, method=method).fit_predict(X)
        assert len(np.unique(labels)) == 40 and labels.min() == 0


def test_degenerate_inputs():
    rng = np.random.default_rng(0)
    assert np.all(HGPClusterer(K=5).fit_predict(rng.random((5, 3))) == -1)
    assert np.all(HGPClusterer(K=3).fit_predict(rng.random((4, 3))) == 0)
    grid = np.stack(np.meshgrid(*[np.arange(6.0)] * 3), axis=-1).reshape(-1, 3)
    assert HGPClusterer(K=2).fit_predict(np.vstack([grid, grid])).shape == (432,)
    with pytest.raises(ValueError):
        HGPClusterer().fit(np.zeros((10, 2)))
    with pytest.raises(ValueError):
        HGPClusterer(K=0).fit(np.zeros((10, 3)))
    for params in ({"method": "dbscan"}, {"K": True}, {"expZ": None}, {"min_cluster_size": np.inf}):
        with pytest.raises(ValueError):
            HGPClusterer(**params).fit(np.zeros((10, 3)))


def test_far_outlier():
    code = (
        "import numpy as np\n"
        "from hgp_clusterer import HGPClusterer\n"
        "rng = np.random.default_rng(1)\n"
        "X = np.vstack([rng.random((5000, 3)), [[1e3, 1e3, 1e3]]])\n"
        "print(HGPClusterer(K=3).fit_predict(X).max())\n"
    )
    result = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True, timeout=600)
    assert result.returncode == 0, result.stderr
