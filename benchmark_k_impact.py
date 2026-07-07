import time
import numpy as np
from sklearn.datasets import make_blobs
from sklearn.metrics import adjusted_rand_score
from e_hgp import EHGPClusterer

def benchmark_k_impact():
    N = 100000  # 100k points
    centers = 5
    dim = 3
    
    print(f"Generating dataset for K-impact: {N} points in {dim}D with {centers} clusters and 10% noise...")
    # Generate 90k points from blobs and 10k points from uniform noise
    N_blobs = int(N * 0.9)
    N_noise = N - N_blobs
    X_blobs, y_blobs = make_blobs(n_samples=N_blobs, centers=centers, n_features=dim, random_state=42)
    
    # Generate uniform noise over the bounding box of blobs
    min_x, max_x = X_blobs.min(axis=0), X_blobs.max(axis=0)
    X_noise = np.random.uniform(min_x, max_x, size=(N_noise, dim))
    y_noise = np.full(N_noise, -1)  # Label -1 for noise
    
    X = np.vstack([X_blobs, X_noise])
    y = np.concatenate([y_blobs, y_noise])
    
    # Vary K from 2 to 5
    k_values = [2, 3, 4, 5]
    results = []
    
    for K in k_values:
        print(f"\nRunning E-HGP with K = {K}...")
        t0 = time.time()
        ehgp = EHGPClusterer(K=K, kappa=1.5, m_reg=20, min_cluster_size=200)
        labels = ehgp.fit_predict(X)
        t_exec = time.time() - t0
        
        # Percentage of clustered points (labels >= 0)
        clustered_mask = labels >= 0
        pct_clustered = np.sum(clustered_mask) / N * 100.0
        
        # ARI score (on a subsample to avoid memory overhead of adjusted_rand_score)
        sub_idx = np.random.choice(N, size=20000, replace=False)
        ari = adjusted_rand_score(y[sub_idx], labels[sub_idx])
        
        n_clusters = len(np.unique(labels[clustered_mask]))
        
        results.append((K, t_exec, ari, pct_clustered, n_clusters))
        print(f"K={K} | Time = {t_exec:.2f}s | ARI = {ari:.4f} | Clustered % = {pct_clustered:.2f}% | Clusters = {n_clusters}")
        
    print("\n" + "="*80)
    print("EVOLUTION OF PERFORMANCE VS K (100,000 points in 3D)")
    print("="*80)
    print("| K | Execution Time (s) | ARI | % Clustered Points | Clusters Found |")
    print("| --- | --- | --- | --- | --- |")
    for res in results:
        print(f"| {res[0]} | {res[1]:.2f}s | {res[2]:.4f} | {res[3]:.2f}% | {res[4]} |")
    print("="*80 + "\n")

if __name__ == "__main__":
    benchmark_k_impact()
