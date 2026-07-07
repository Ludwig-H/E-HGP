import time
import numpy as np
from sklearn.datasets import make_blobs
from sklearn.metrics import adjusted_rand_score
from e_hgp import EHGPClusterer
from hdbscan import HDBSCAN

def run_large_benchmark():
    N = 1000000  # 1 Million points
    centers = 20
    dim = 3
    
    print(f"Generating large dataset: {N} points in {dim}D with {centers} clusters...")
    X, y = make_blobs(n_samples=N, centers=centers, n_features=dim, random_state=42)
    
    print("\nRunning E-HGP (1M points)...")
    t0 = time.time()
    # Vary m_reg and K as requested
    ehgp = EHGPClusterer(K=2, kappa=1.5, m_reg=20, min_cluster_size=1000)
    labels_ehgp = ehgp.fit_predict(X)
    t_ehgp = time.time() - t0
    
    # Calculate ARI on a subsample to avoid memory overhead of adjusted_rand_score on 1M points
    # (adjusted_rand_score on 1M points allocates large contingency tables and can exceed RAM)
    sub_idx = np.random.choice(N, size=50000, replace=False)
    ari_ehgp = adjusted_rand_score(y[sub_idx], labels_ehgp[sub_idx])
    n_cl_ehgp = len(np.unique(labels_ehgp[labels_ehgp >= 0]))
    
    print(f"E-HGP finished in {t_ehgp:.2f}s | ARI (subsample 50k): {ari_ehgp:.4f} | Clusters found: {n_cl_ehgp}")
    
    print("\nRunning HDBSCAN (1M points)...")
    t0 = time.time()
    hdb = HDBSCAN(min_cluster_size=1000, core_dist_n_jobs=-1)
    labels_hdb = hdb.fit_predict(X)
    t_hdb = time.time() - t0
    ari_hdb = adjusted_rand_score(y[sub_idx], labels_hdb[sub_idx])
    n_cl_hdb = len(np.unique(labels_hdb[labels_hdb >= 0]))
    
    print(f"HDBSCAN finished in {t_hdb:.2f}s | ARI (subsample 50k): {ari_hdb:.4f} | Clusters found: {n_cl_hdb}")
    
    print("\n" + "="*80)
    print("LARGE BENCHMARK SUMMARY (1,000,000 points)")
    print("="*80)
    print(f"E-HGP   : Time = {t_ehgp:.2f}s | ARI = {ari_ehgp:.4f} | Clusters = {n_cl_ehgp}")
    print(f"HDBSCAN : Time = {t_hdb:.2f}s | ARI = {ari_hdb:.4f} | Clusters = {n_cl_hdb}")
    print("="*80)

if __name__ == "__main__":
    run_large_benchmark()
