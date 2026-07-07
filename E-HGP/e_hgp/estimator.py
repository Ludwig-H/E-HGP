from sklearn.base import BaseEstimator, ClusterMixin
import numpy as np

from .regularization import regularize_entropy_local
from .lazy_boruvka import lazy_cut_boruvka
from .clustering import compute_face_weights, condense_tree, extract_labels

class EHGPClusterer(BaseEstimator, ClusterMixin):
    """
    Entropy-Regularized Hypergraph Percolation (E-HGP) Clusterer.
    
    A Scikit-Learn compatible estimator that applies local entropy
    regularization to define power sites, finds a certified K-MST using
    lazy Borůvka, and extracts clusters from the hierarchy.
    """
    def __init__(self, K=2, kappa=2.0, m_reg=30, min_cluster_size=None, 
                 regularization='entropy_local', eps_sinkhorn=0.1, L_initial=30):
        self.K = K
        self.kappa = kappa
        self.m_reg = m_reg
        self.min_cluster_size = min_cluster_size
        self.regularization = regularization
        self.eps_sinkhorn = eps_sinkhorn
        self.L_initial = L_initial
        
    def fit(self, X, y=None):
        """
        Fits the E-HGP Clusterer to the input data X.
        
        Parameters
        ----------
        X : np.ndarray
            Input data points of shape (n, p).
        y : None
            Ignored.
            
        Returns
        -------
        self : EHGPClusterer
        """
        X = np.ascontiguousarray(X, dtype=np.float32)
        n = X.shape[0]
        if self.min_cluster_size is None:
            self.min_cluster_size = int(np.sqrt(n))
            
        # 1. Apply entropy regularization to get power sites (Z, a)
        Z, a, local_supports = regularize_entropy_local(
            X, m_reg=self.m_reg, kappa=self.kappa, 
            regularization=self.regularization, eps_sinkhorn=self.eps_sinkhorn
        )
        self.Z_ = Z
        self.a_ = a
        
        # 2. Build the certified K-MST using lazy Boruvka
        mst_edges, exactness_report = lazy_cut_boruvka(
            Z, a, K=self.K, L_initial=self.L_initial
        )
        self.mst_edges_ = mst_edges
        self.exactness_report_ = exactness_report
        
        if not mst_edges:
            self.labels_ = np.full(n, -1, dtype=np.int32)
            return self
            
        # 3. Compile unique K-faces in the MST
        unique_faces = set()
        for edge in mst_edges:
            tau_a, tau_b, weight, sigma = edge
            unique_faces.add(tau_a)
            unique_faces.add(tau_b)
            
        unique_faces = sorted(list(unique_faces))
        face_to_idx = {face: idx for idx, face in enumerate(unique_faces)}
        faces_unique_arr = np.array(unique_faces, dtype=np.int32)
        
        M = len(mst_edges)
        u_mst = np.zeros(M, dtype=np.int32)
        v_mst = np.zeros(M, dtype=np.int32)
        w_mst = np.zeros(M, dtype=np.float32)
        
        for idx, edge in enumerate(mst_edges):
            tau_a, tau_b, weight, sigma = edge
            u_mst[idx] = face_to_idx[tau_a]
            v_mst[idx] = face_to_idx[tau_b]
            w_mst[idx] = weight
            
        # 4. Compute face weights W_nodes
        W_nodes, S_faces = compute_face_weights(faces_unique_arr, Z, a)
        
        # 5. Condense tree hierarchy
        Z_tree = condense_tree(W_nodes, u_mst, v_mst, w_mst, self.min_cluster_size)
        self.Z_tree_ = Z_tree
        
        # 6. Extract point labels via sparse voting
        self.labels_ = extract_labels(Z_tree, faces_unique_arr, n, S_faces)
        
        return self
        
    def fit_predict(self, X, y=None):
        self.fit(X)
        return self.labels_
