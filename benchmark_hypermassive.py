import time
import numpy as np
from sklearn.datasets import make_blobs
from sklearn.metrics import adjusted_rand_score
from e_hgp import EHGPClusterer
from hdbscan import HDBSCAN

def run_hypermassive_benchmark():
    N = 10000000  # 10 Million points
    centers = 100 # 100 clusters
    dim = 3
    
    print(f"Generating hypermassive dataset: {N} points in {dim}D with {centers} clusters...")
    t0 = time.time()
    X, y = make_blobs(n_samples=N, centers=centers, n_features=dim, random_state=42)
    print(f"Dataset generated in {time.time() - t0:.2f}s.")
    
    print("\n" + "="*80)
    print("RUNNING E-HGP (10 Million points)...")
    print("="*80)
    t0 = time.time()
    # K=2, L_initial=10 to keep lookahead very memory-efficient
    ehgp = EHGPClusterer(K=2, kappa=1.5, m_reg=20, min_cluster_size=1000, L_initial=10)
    labels_ehgp = ehgp.fit_predict(X)
    t_ehgp = time.time() - t0
    
    # Calculate ARI on a sample to avoid memory overhead of adjusted_rand_score
    sub_idx = np.random.choice(N, size=50000, replace=False)
    ari_ehgp = adjusted_rand_score(y[sub_idx], labels_ehgp[sub_idx])
    n_cl_ehgp = len(np.unique(labels_ehgp[labels_ehgp >= 0]))
    pct_ehgp = np.sum(labels_ehgp >= 0) / N * 100.0
    
    print(f"E-HGP finished in {t_ehgp:.2f}s | ARI (subsample 50k): {ari_ehgp:.4f} | Clusters found: {n_cl_ehgp} | Clustered: {pct_ehgp:.2f}%")
    
    print("\n" + "="*80)
    print("RUNNING HDBSCAN (10 Million points)...")
    print("="*80)
    try:
        t0 = time.time()
        hdb = HDBSCAN(min_cluster_size=1000, core_dist_n_jobs=-1)
        labels_hdb = hdb.fit_predict(X)
        t_hdb = time.time() - t0
        ari_hdb = adjusted_rand_score(y[sub_idx], labels_hdb[sub_idx])
        n_cl_hdb = len(np.unique(labels_hdb[labels_hdb >= 0]))
        pct_hdb = np.sum(labels_hdb >= 0) / N * 100.0
        print(f"HDBSCAN finished in {t_hdb:.2f}s | ARI (subsample 50k): {ari_hdb:.4f} | Clusters found: {n_cl_hdb} | Clustered: {pct_hdb:.2f}%")
    except Exception as e:
        print(f"HDBSCAN failed or ran out of memory: {e}")
        t_hdb = float('nan')
        ari_hdb = 0.0
        n_cl_hdb = 0
        pct_hdb = 0.0
        
    print("\n" + "="*80)
    print("HYPERMASSIVE BENCHMARK SUMMARY (10,000,000 points)")
    print("="*80)
    print(f"E-HGP   : Time = {t_ehgp:.2f}s | ARI = {ari_ehgp:.4f} | Clusters = {n_cl_ehgp} | Clustered = {pct_ehgp:.2f}%")
    if not np.isnan(t_hdb):
        print(f"HDBSCAN : Time = {t_hdb:.2f}s | ARI = {ari_hdb:.4f} | Clusters = {n_cl_hdb} | Clustered = {pct_hdb:.2f}%")
    else:
        print("HDBSCAN : FAILED (OOM/Error)")
    print("="*80 + "\n")

if __name__ == "__main__":
    run_hypermassive_benchmark()
