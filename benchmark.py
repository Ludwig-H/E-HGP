import time
import numpy as np
from sklearn.datasets import make_moons, make_blobs
from sklearn.metrics import adjusted_rand_score
from hdbscan import HDBSCAN
from hgp_clusterer import HGPClusterer
from e_hgp import EHGPClusterer

def run_benchmark():
    results = []
    
    # -------------------------------------------------------------------------
    # 1. 2D Nesting Moons Dataset
    # -------------------------------------------------------------------------
    print("Running 2D benchmark (Nesting Moons)...")
    X_2d, y_2d = make_moons(n_samples=600, noise=0.08, random_state=42)
    
    # E-HGP
    t0 = time.time()
    ehgp = EHGPClusterer(K=2, kappa=1.5, m_reg=20, min_cluster_size=15)
    labels_ehgp_2d = ehgp.fit_predict(X_2d)
    t_ehgp_2d = time.time() - t0
    ari_ehgp_2d = adjusted_rand_score(y_2d, labels_ehgp_2d)
    n_cl_ehgp_2d = len(np.unique(labels_ehgp_2d[labels_ehgp_2d >= 0]))
    results.append(("2D Moons", "E-HGP", t_ehgp_2d, ari_ehgp_2d, n_cl_ehgp_2d))
    
    # HGP-old
    t0 = time.time()
    hgp_old = HGPClusterer(K=2, min_cluster_size=15)
    labels_hgp_old_2d = hgp_old.fit_predict(X_2d)
    t_hgp_old_2d = time.time() - t0
    ari_hgp_old_2d = adjusted_rand_score(y_2d, labels_hgp_old_2d)
    n_cl_hgp_old_2d = len(np.unique(labels_hgp_old_2d[labels_hgp_old_2d >= 0]))
    results.append(("2D Moons", "HGP-old", t_hgp_old_2d, ari_hgp_old_2d, n_cl_hgp_old_2d))
    
    # -------------------------------------------------------------------------
    # 2. 3D Blobs Dataset
    # -------------------------------------------------------------------------
    print("Running 3D benchmark (Blobs)...")
    X_3d, y_3d = make_blobs(n_samples=500, centers=3, n_features=3, random_state=42)
    
    # E-HGP
    t0 = time.time()
    ehgp = EHGPClusterer(K=2, kappa=1.5, m_reg=30, min_cluster_size=15)
    labels_ehgp_3d = ehgp.fit_predict(X_3d)
    t_ehgp_3d = time.time() - t0
    ari_ehgp_3d = adjusted_rand_score(y_3d, labels_ehgp_3d)
    n_cl_ehgp_3d = len(np.unique(labels_ehgp_3d[labels_ehgp_3d >= 0]))
    results.append(("3D Blobs", "E-HGP", t_ehgp_3d, ari_ehgp_3d, n_cl_ehgp_3d))
    
    # HGP-old
    t0 = time.time()
    hgp_old = HGPClusterer(K=2, min_cluster_size=15)
    labels_hgp_old_3d = hgp_old.fit_predict(X_3d)
    t_hgp_old_3d = time.time() - t0
    ari_hgp_old_3d = adjusted_rand_score(y_3d, labels_hgp_old_3d)
    n_cl_hgp_old_3d = len(np.unique(labels_hgp_old_3d[labels_hgp_old_3d >= 0]))
    results.append(("3D Blobs", "HGP-old", t_hgp_old_3d, ari_hgp_old_3d, n_cl_hgp_old_3d))
    
    # -------------------------------------------------------------------------
    # 3. nD (High-Dimensional 20D) Blobs Dataset
    # -------------------------------------------------------------------------
    print("Running 20D benchmark...")
    X_nd, y_nd = make_blobs(n_samples=1000, centers=4, n_features=20, random_state=42)
    
    # E-HGP
    t0 = time.time()
    ehgp = EHGPClusterer(K=2, kappa=1.5, m_reg=30, min_cluster_size=20)
    labels_ehgp_nd = ehgp.fit_predict(X_nd)
    t_ehgp_nd = time.time() - t0
    ari_ehgp_nd = adjusted_rand_score(y_nd, labels_ehgp_nd)
    n_cl_ehgp_nd = len(np.unique(labels_ehgp_nd[labels_ehgp_nd >= 0]))
    results.append(("20D Blobs", "E-HGP", t_ehgp_nd, ari_ehgp_nd, n_cl_ehgp_nd))
    
    # HDBSCAN
    t0 = time.time()
    hdb = HDBSCAN(min_cluster_size=20)
    labels_hdb_nd = hdb.fit_predict(X_nd)
    t_hdb_nd = time.time() - t0
    ari_hdb_nd = adjusted_rand_score(y_nd, labels_hdb_nd)
    n_cl_hdb_nd = len(np.unique(labels_hdb_nd[labels_hdb_nd >= 0]))
    results.append(("20D Blobs", "HDBSCAN", t_hdb_nd, ari_hdb_nd, n_cl_hdb_nd))
    
    # Print results in Markdown Table format
    print("\n" + "="*80)
    print("BENCHMARK RESULTS")
    print("="*80)
    print("| Dataset | Method | Time (s) | ARI | Clusters Found |")
    print("| --- | --- | --- | --- | --- |")
    for res in results:
        print(f"| {res[0]} | {res[1]} | {res[2]:.4f}s | {res[3]:.4f} | {res[4]} |")
    print("="*80 + "\n")

if __name__ == "__main__":
    run_benchmark()
