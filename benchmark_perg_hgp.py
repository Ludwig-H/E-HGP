import time
import sys
import numpy as np
import torch
from sklearn.datasets import make_blobs
from sklearn.metrics import adjusted_rand_score
from hdbscan import HDBSCAN

# Ensure perg_hgp is in python path
sys.path.append("/workspaces/E-HGP/perg_hgp")
from perg_hgp import PERGHGPClusterer

def make_concentric_spheres(n_samples=10000, noise=0.08):
    # Inner dense sphere
    n_inner = n_samples // 2
    r_inner = 1.0
    phi = np.random.uniform(0, 2 * np.pi, n_inner)
    theta = np.random.uniform(0, np.pi, n_inner)
    x = r_inner * np.sin(theta) * np.cos(phi) + np.random.normal(0, noise, n_inner)
    y = r_inner * np.sin(theta) * np.sin(phi) + np.random.normal(0, noise, n_inner)
    z = r_inner * np.cos(theta) + np.random.normal(0, noise, n_inner)
    X_inner = np.column_stack([x, y, z])
    y_inner = np.zeros(n_inner)
    
    # Outer sparse sphere
    n_outer = n_samples - n_inner
    r_outer = 4.0
    phi = np.random.uniform(0, 2 * np.pi, n_outer)
    theta = np.random.uniform(0, np.pi, n_outer)
    x = r_outer * np.sin(theta) * np.cos(phi) + np.random.normal(0, noise * 2, n_outer)
    y = r_outer * np.sin(theta) * np.sin(phi) + np.random.normal(0, noise * 2, n_outer)
    z = r_outer * np.cos(theta) + np.random.normal(0, noise * 2, n_outer)
    X_outer = np.column_stack([x, y, z])
    y_outer = np.ones(n_outer)
    
    return np.vstack([X_inner, X_outer]), np.concatenate([y_inner, y_outer])

def run_perg_hgp_benchmark():
    print("=" * 80)
    print("PERG-HGP VS HDBSCAN BENCHMARK")
    print("=" * 80)
    
    results = []
    
    # -------------------------------------------------------------------------
    # 1. 3D Blobs Dataset (10,000 points)
    # -------------------------------------------------------------------------
    print("\n--- Running Benchmark 1: 3D Blobs (10,000 points) ---")
    X_b, y_b = make_blobs(n_samples=10000, centers=3, n_features=3, cluster_std=1.0, random_state=42)
    X_b = X_b.astype(np.float32)
    
    # PERG-HGP
    t0 = time.time()
    perg = PERGHGPClusterer(K=10, min_cluster_size=100, device='cpu')
    labels_perg_b = perg.fit_predict(X_b)
    t_perg_b = time.time() - t0
    ari_perg_b = adjusted_rand_score(y_b, labels_perg_b)
    pct_perg_b = np.sum(labels_perg_b >= 0) / len(labels_perg_b) * 100
    n_cl_perg_b = len(np.unique(labels_perg_b[labels_perg_b >= 0]))
    
    print(f"PERG-HGP finished in {t_perg_b:.4f}s | ARI: {ari_perg_b:.4f} | Clusters: {n_cl_perg_b} | Clustered %: {pct_perg_b:.2f}%")
    print("Exactness Report PERG-HGP:")
    for k, v in perg.exactness_report_.items():
        print(f"  - {k}: {v}")
        
    # HDBSCAN
    t0 = time.time()
    hdb = HDBSCAN(min_cluster_size=100)
    labels_hdb_b = hdb.fit_predict(X_b)
    t_hdb_b = time.time() - t0
    ari_hdb_b = adjusted_rand_score(y_b, labels_hdb_b)
    pct_hdb_b = np.sum(labels_hdb_b >= 0) / len(labels_hdb_b) * 100
    n_cl_hdb_b = len(np.unique(labels_hdb_b[labels_hdb_b >= 0]))
    print(f"HDBSCAN finished in {t_hdb_b:.4f}s | ARI: {ari_hdb_b:.4f} | Clusters: {n_cl_hdb_b} | Clustered %: {pct_hdb_b:.2f}%")
    
    results.append(("3D Blobs", "PERG-HGP", t_perg_b, ari_perg_b, n_cl_perg_b, pct_perg_b))
    results.append(("3D Blobs", "HDBSCAN", t_hdb_b, ari_hdb_b, n_cl_hdb_b, pct_hdb_b))
    
    # -------------------------------------------------------------------------
    # 2. 3D Concentric Spheres (10,000 points - Non-Convex Geometry)
    # -------------------------------------------------------------------------
    print("\n--- Running Benchmark 2: 3D Concentric Spheres (10,000 points) ---")
    X_s, y_s = make_concentric_spheres(n_samples=10000, noise=0.08)
    X_s = X_s.astype(np.float32)
    
    # PERG-HGP
    t0 = time.time()
    perg = PERGHGPClusterer(K=2, min_cluster_size=100, device='cpu')
    labels_perg_s = perg.fit_predict(X_s)
    t_perg_s = time.time() - t0
    ari_perg_s = adjusted_rand_score(y_s, labels_perg_s)
    pct_perg_s = np.sum(labels_perg_s >= 0) / len(labels_perg_s) * 100
    n_cl_perg_s = len(np.unique(labels_perg_s[labels_perg_s >= 0]))
    
    print(f"PERG-HGP finished in {t_perg_s:.4f}s | ARI: {ari_perg_s:.4f} | Clusters: {n_cl_perg_s} | Clustered %: {pct_perg_s:.2f}%")
    print("Exactness Report PERG-HGP:")
    for k, v in perg.exactness_report_.items():
        print(f"  - {k}: {v}")
        
    # HDBSCAN
    t0 = time.time()
    hdb = HDBSCAN(min_cluster_size=100)
    labels_hdb_s = hdb.fit_predict(X_s)
    t_hdb_s = time.time() - t0
    ari_hdb_s = adjusted_rand_score(y_s, labels_hdb_s)
    pct_hdb_s = np.sum(labels_hdb_s >= 0) / len(labels_hdb_s) * 100
    n_cl_hdb_s = len(np.unique(labels_hdb_s[labels_hdb_s >= 0]))
    print(f"HDBSCAN finished in {t_hdb_s:.4f}s | ARI: {ari_hdb_s:.4f} | Clusters: {n_cl_hdb_s} | Clustered %: {pct_hdb_s:.2f}%")
    
    results.append(("Concentric Spheres", "PERG-HGP", t_perg_s, ari_perg_s, n_cl_perg_s, pct_perg_s))
    results.append(("Concentric Spheres", "HDBSCAN", t_hdb_s, ari_hdb_s, n_cl_hdb_s, pct_hdb_s))
    
    # -------------------------------------------------------------------------
    # Markdown Report Summary
    # -------------------------------------------------------------------------
    print("\n" + "=" * 80)
    print("PERG-HGP VS HDBSCAN FINAL COMPARISON REPORT")
    print("=" * 80)
    print("| Dataset | Method | Time (s) | ARI | Clusters Found | Clustered % |")
    print("| --- | --- | --- | --- | --- | --- |")
    for res in results:
        print(f"| {res[0]} | {res[1]} | {res[2]:.4f}s | {res[3]:.4f} | {res[4]} | {res[5]:.2f}% |")
    print("=" * 80)

if __name__ == "__main__":
    run_perg_hgp_benchmark()
