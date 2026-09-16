"""Order-k Delaunay simplices and their dual hypergraph."""

from __future__ import annotations

import numbers

import numpy as np
from sklearn.utils.validation import check_array

from ._geometry import order_k_delaunay
from ._hierarchy import dual_graph

JITTER = 1e-8
MAX_ORDER = 63
MAX_SPAN = 1e8


def normalize(X: np.ndarray) -> tuple[np.ndarray, float]:
    """Center and scale a 3D cloud robustly, then apply a deterministic perturbation.

    The cloud is centered on its median and divided by its largest
    interquartile range (or, for degenerate distributions, by the largest
    extent between its 1 % and 99 % quantiles, then of its bounding box). Each
    point is perturbed by a gaussian noise of relative amplitude ``JITTER``.

    Returns the transformed coordinates and the scale factor that converts
    lengths back to the units of ``X``.
    """
    X = check_array(X, dtype=np.float64, ensure_min_samples=0)
    if X.shape[1] != 3:
        raise ValueError(f"X must have 3 columns, got {X.shape[1]}")
    if X.shape[0] == 0:
        return X, 1.0
    center = np.median(X, axis=0)
    with np.errstate(over="ignore", invalid="ignore"):
        scale = 0.0
        for low, high in ((0.25, 0.75), (0.01, 0.99), (0.0, 1.0)):
            q_low, q_high = np.quantile(X, [low, high], axis=0)
            scale = float(np.max(q_high - q_low))
            if not np.isfinite(scale):
                raise ValueError("the extent of X is not finite")
            if scale > 0.0:
                break
        if scale == 0.0:
            scale = 1.0
        Y = (X - center) / scale
    if not np.isfinite(Y).all():
        raise ValueError("the extent of X is not finite")
    if np.abs(Y).max() > MAX_SPAN:
        raise ValueError("X spans too many orders of magnitude around its median")
    amplitude = JITTER * np.maximum(1.0, np.abs(Y).max(axis=1, keepdims=True))
    Y += amplitude * np.random.default_rng(42).standard_normal(Y.shape)
    return np.ascontiguousarray(Y), scale


def order_k_simplices(X: np.ndarray, K: int) -> tuple[np.ndarray, np.ndarray]:
    """K-simplices of the order-k Delaunay filtration of a 3D point cloud.

    Parameters
    ----------
    X : array-like of shape (n_samples, 3)
        Point cloud with finite coordinates.
    K : int
        Order of the simplices: 1 for pairs, 2 for triangles, 3 for tetrahedra.

    Returns
    -------
    simplices : ndarray of shape (n_simplices, K + 1), int32
        Sets of K + 1 points whose order-(K + 1) Voronoi cell is non-empty,
        sorted within each row and lexicographically across rows.
    radii : ndarray of shape (n_simplices,), float64
        Radius of the minimum enclosing ball of each simplex, in the units of ``X``.
    """
    if not isinstance(K, numbers.Integral) or isinstance(K, bool) or not 1 <= K <= MAX_ORDER:
        raise ValueError(f"K must be an integer between 1 and {MAX_ORDER}")
    Y, scale = normalize(X)
    simplices, squared_radii = order_k_delaunay(Y, int(K))
    return simplices, scale * np.sqrt(np.maximum(squared_radii, 0.0))


def build_hypergraph(X: np.ndarray, K: int, expZ: float, verbose: bool = False):
    """Dual graph of the (K-1)-faces of the order-k Delaunay K-simplices.

    Simplices are filtered by their relative radius ``r / h``, where ``r`` is the
    radius of their minimum enclosing ball and ``h`` the median of these radii;
    a simplex weighs ``(r / h) ** -expZ`` in the faces it contains.

    Returns
    -------
    faces, u, v, w, face_weights
        See :func:`hgp_clusterer._hierarchy.dual_graph`.
    unit : float
        The radius ``h``, in the units of ``X``.
    """
    Y, scale = normalize(X)
    simplices, squared_radii = order_k_delaunay(Y, int(K))
    if verbose:
        print(f"[HGP] {simplices.shape[0]} simplices of order {K}")
    radii = np.sqrt(np.maximum(squared_radii, 0.0))
    positive = radii[radii > 0.0]
    reference = float(np.median(positive)) if positive.size else 1.0
    levels = (radii / reference).astype(np.float32)
    faces, u, v, w, face_weights = dual_graph(simplices, levels, int(K), float(expZ))
    if verbose:
        print(f"[HGP] {faces.shape[0]} faces, {u.shape[0]} edges")
    return faces, u, v, w, face_weights, scale * reference
