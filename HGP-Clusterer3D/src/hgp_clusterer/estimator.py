"""Scikit-learn estimator for HGP clustering of 3D point clouds."""

from __future__ import annotations

import math
import numbers

import numpy as np
from scipy.sparse import coo_matrix
from sklearn.base import BaseEstimator, ClusterMixin
from sklearn.utils.validation import check_array, check_is_fitted

from ._hierarchy import condense_tree, kruskal
from .hierarchy import select_clusters
from .hypergraph import MAX_ORDER, build_hypergraph

MAX_EXPZ = 20.0


def _is_integer(value):
    return isinstance(value, numbers.Integral) and not isinstance(value, bool)


def _is_positive_real(value):
    return (
        isinstance(value, numbers.Real)
        and not isinstance(value, bool)
        and math.isfinite(value)
        and value > 0
    )


class HGPClusterer(ClusterMixin, BaseEstimator):
    """Hypergraph percolation (HGP) clustering of 3D point clouds.

    The (K-1)-faces of the K-simplices of the order-k Delaunay filtration are
    linked through these simplices, filtered by the radius of their minimum
    enclosing ball. The minimum spanning tree of this dual graph (K-MST) yields
    an HDBSCAN-like condensed tree from which clusters are selected, then
    transferred to the points.

    Parameters
    ----------
    K : int, default=2
        Order of the simplices: 1 for pairs (single linkage), 2 for triangles,
        3 for tetrahedra, and so on.
    min_cluster_size : float, default=None
        Minimal mass of a cluster, in number of points. Defaults to
        ``round(sqrt(n_samples))``.
    method : {"eom", "leaf"} or float, default="eom"
        Cluster selection: excess of mass, leaves of the condensed tree, or a
        positive radius (in the units of ``X``) at which the hierarchy is cut.
    splitting : callable, default=None
        ``splitting(parent_points, children_points) -> bool``. Called on each
        selected cluster that has children, with the points carried by the
        faces of the leaves below it and their partition among its children;
        returning ``True`` replaces the cluster by its children, recursively.
    expZ : float, default=2.0
        Exponent of the density scale, between 0 (excluded) and 20: a simplex of
        enclosing radius ``r`` has weight ``(r / h) ** -expZ``, where ``h`` is the
        median radius of the simplices; clusters are born and die at the scale
        ``lambda = (r / h) ** -expZ``.
    verbose : bool, default=False
        Print the size of the intermediate structures.

    Attributes
    ----------
    labels_ : ndarray of shape (n_samples,)
        Cluster of each point, ``-1`` for noise.
    min_cluster_size_ : float
        Effective minimal cluster size.
    faces_unique_ : ndarray of shape (n_faces, K)
        Distinct (K-1)-faces, i.e. the nodes of the hierarchy.
    S_faces_ : ndarray of shape (n_faces,)
        Sum of the weights ``(r / h) ** -expZ`` of the simplices containing each
        face.
    T_points_ : ndarray of shape (n_samples,)
        Sum of ``S_faces_`` over the faces containing each point.
    W_nodes_ : ndarray of shape (n_faces,)
        Mass of each face, ``S_faces_[f] * sum(1 / T_points_[x] for x in f)``.
    forest_ : list of dict
        Condensed tree (``"tree"``) and face indices (``"nodes"``) of each
        connected component of the dual graph.
    """

    def __init__(
        self,
        K=2,
        min_cluster_size=None,
        method="eom",
        splitting=None,
        expZ=2.0,
        verbose=False,
    ):
        self.K = K
        self.min_cluster_size = min_cluster_size
        self.method = method
        self.splitting = splitting
        self.expZ = expZ
        self.verbose = verbose

    def fit(self, X, y=None):
        """Build the hierarchy of ``X`` and extract its clusters.

        Parameters
        ----------
        X : array-like of shape (n_samples, 3)
            Point cloud.
        y : ignored

        Returns
        -------
        self : HGPClusterer
        """
        X = check_array(X, dtype=np.float64, ensure_min_samples=1)
        if X.shape[1] != 3:
            raise ValueError(f"X must have 3 columns, got {X.shape[1]}")
        if not _is_integer(self.K) or not 1 <= self.K <= MAX_ORDER:
            raise ValueError(f"K must be an integer between 1 and {MAX_ORDER}")
        if not _is_positive_real(self.expZ) or self.expZ > MAX_EXPZ:
            raise ValueError(f"expZ must be a real number in (0, {MAX_EXPZ:g}]")
        self._check_method(self.method)

        n_samples = X.shape[0]
        expZ = float(self.expZ)
        if self.min_cluster_size is None:
            min_cluster_size = float(round(math.sqrt(n_samples)))
        elif _is_positive_real(self.min_cluster_size):
            min_cluster_size = float(self.min_cluster_size)
        else:
            raise ValueError("min_cluster_size must be a positive real number")

        faces, u, v, w, S_faces, unit = build_hypergraph(X, int(self.K), expZ, self.verbose)

        width = faces.shape[1]
        T_points = np.bincount(faces.ravel(), weights=np.repeat(S_faces, width), minlength=n_samples)
        inverse_T = np.zeros(n_samples)
        np.divide(1.0, T_points, out=inverse_T, where=T_points > 0)
        W_nodes = (S_faces * inverse_T[faces].sum(axis=1)).astype(np.float32)

        order = np.argsort(w, kind="stable")
        U = np.minimum(u, v)[order]
        V = np.maximum(u, v)[order]
        w = w[order]

        forest = []
        for edges in kruskal(U, V, faces.shape[0]):
            if edges.shape[0] == 0:
                continue
            nodes, inverse = np.unique(np.concatenate((U[edges], V[edges])), return_inverse=True)
            inverse = inverse.astype(np.int32)
            tree = condense_tree(
                W_nodes[nodes],
                inverse[: edges.shape[0]],
                inverse[edges.shape[0]:],
                w[edges],
                min_cluster_size,
                expZ,
            )
            forest.append({"tree": tree, "nodes": nodes.astype(np.int32)})
        if self.verbose:
            print(f"[HGP] {len(forest)} connected components")

        self.n_features_in_ = 3
        self._n_samples = n_samples
        self._expZ = expZ
        self._unit = unit
        self.min_cluster_size_ = min_cluster_size
        self.faces_unique_ = faces
        self.S_faces_ = S_faces
        self.T_points_ = T_points
        self.W_nodes_ = W_nodes
        self.forest_ = forest
        self.labels_ = self._extract_labels(self.method, self.splitting)
        return self

    def refine_clusters(self, method="eom", splitting=None):
        """Extract clusters again from the fitted hierarchy, without recomputing it.

        Parameters
        ----------
        method : {"eom", "leaf"} or float, default="eom"
            See the ``method`` parameter.
        splitting : callable, default=None
            See the ``splitting`` parameter.

        Returns
        -------
        labels : ndarray of shape (n_samples,)
            The updated ``labels_``.
        """
        check_is_fitted(self, "forest_")
        self._check_method(method)
        self.labels_ = self._extract_labels(method, splitting)
        return self.labels_

    @staticmethod
    def _check_method(method):
        if isinstance(method, str):
            if method not in ("eom", "leaf"):
                raise ValueError("method must be 'eom', 'leaf' or a positive radius")
        elif not _is_positive_real(method):
            raise ValueError("method must be 'eom', 'leaf' or a positive radius")

    def _extract_labels(self, method, splitting):
        if isinstance(method, str):
            selection, level = method, None
        else:
            selection, level = "cut", float(method) / self._unit

        face_labels = np.full(self.faces_unique_.shape[0], -1, dtype=np.int32)
        n_clusters = 0
        for component in self.forest_:
            nodes = component["nodes"]
            clusters = select_clusters(
                component["tree"],
                selection,
                self.faces_unique_[nodes],
                self.S_faces_[nodes],
                splitting=splitting,
                level=level,
            )
            for cluster in clusters:
                face_labels[nodes[cluster]] = n_clusters
                n_clusters += 1

        labels = np.full(self._n_samples, -1, dtype=np.int32)
        if n_clusters == 0:
            return labels

        width = self.faces_unique_.shape[1]
        face_labels = np.repeat(face_labels, width)
        mask = face_labels >= 0
        votes = coo_matrix(
            (
                np.repeat(self.S_faces_, width)[mask],
                (self.faces_unique_.ravel()[mask], face_labels[mask]),
            ),
            shape=(self._n_samples, n_clusters),
            dtype=np.float64,
        ).tocsr()
        voted = np.diff(votes.indptr) > 0
        best = np.asarray(votes.argmax(axis=1)).ravel()
        labels[voted] = best[voted]

        present = np.unique(labels[voted])
        relabel = np.full(n_clusters, -1, dtype=np.int32)
        relabel[present] = np.arange(present.shape[0], dtype=np.int32)
        labels[voted] = relabel[labels[voted]]
        return labels
