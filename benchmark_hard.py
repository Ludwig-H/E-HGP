import time
import numpy as np
from sklearn.datasets import make_blobs
from sklearn.metrics import adjusted_rand_score
from e_hgp import EHGPClusterer
from hdbscan import HDBSCAN

def make_concentric_spheres(n_samples=30000, noise=0.1):
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

def run_hard_benchmarks():
    print("="*80)
    print("RUNNING CHALLENGING CLUSTERING BENCHMARKS")
    print("="*80)
    
    # -------------------------------------------------------------------------
    # Scenario A: Concentric 3D Spheres (Complex Non-Convex Geometry)
    # -------------------------------------------------------------------------
    print("\n--- Scenario A: Concentric 3D Spheres (30,000 points) ---")
    X_a, y_a = make_concentric_spheres(n_samples=30000, noise=0.08)
    
    # E-HGP
    t0 = time.time()
    ehgp = EHGPClusterer(K=2, kappa=1.2, m_reg=20, min_cluster_size=100)
    labels_ehgp_a = ehgp.fit_predict(X_a)
    t_ehgp_a = time.time() - t0
    ari_ehgp_a = adjusted_rand_score(y_a, labels_ehgp_a)
    pct_ehgp_a = np.sum(labels_ehgp_a >= 0) / len(labels_ehgp_a) * 100
    print(f"E-HGP   : Time = {t_ehgp_a:.2f}s | ARI = {ari_ehgp_a:.4f} | Clustered % = {pct_ehgp_a:.2f}%")
    
    # HDBSCAN
    t0 = time.time()
    hdb = HDBSCAN(min_cluster_size=100)
    labels_hdb_a = hdb.fit_predict(X_a)
    t_hdb_a = time.time() - t0
    ari_hdb_a = adjusted_rand_score(y_a, labels_hdb_a)
    pct_hdb_a = np.sum(labels_hdb_a >= 0) / len(labels_hdb_a) * 100
    print(f"HDBSCAN : Time = {t_hdb_a:.2f}s | ARI = {ari_hdb_a:.4f} | Clustered % = {pct_hdb_a:.2f}%")
    
    # -------------------------------------------------------------------------
    # Scenario B: Variable Density Blobs with 10% Background Noise
    # -------------------------------------------------------------------------
    print("\n--- Scenario B: Variable Density & Noise (50,000 points, 3D) ---")
    N = 50000
    N_blobs = int(N * 0.9)
    N_noise = N - N_blobs
    
    # 3 blobs with highly different standard deviations (0.3, 1.2, 3.5)
    X_b1, y_b1 = make_blobs(n_samples=N_blobs // 3, centers=[[-5, -5, 0]], cluster_std=0.3, n_features=3, random_state=42)
    X_b2, y_b2 = make_blobs(n_samples=N_blobs // 3, centers=[[0, 5, 5]], cluster_std=1.2, n_features=3, random_state=42)
    X_b3, y_b3 = make_blobs(n_samples=N_blobs - 2*(N_blobs // 3), centers=[[5, -5, -5]], cluster_std=3.5, n_features=3, random_state=42)
    
    X_blobs = np.vstack([X_b1, X_b2, X_b3])
    y_blobs = np.concatenate([y_b1, y_b2 + 1, y_b3 + 2])
    
    # Background uniform noise
    min_x, max_x = X_blobs.min(axis=0), X_blobs.max(axis=0)
    X_noise = np.random.uniform(min_x, max_x, size=(N_noise, 3))
    y_noise = np.full(N_noise, -1)
    
    X_b = np.vstack([X_blobs, X_noise])
    y_b = np.concatenate([y_blobs, y_noise])
    
    # E-HGP
    t0 = time.time()
    ehgp = EHGPClusterer(K=2, kappa=1.5, m_reg=25, min_cluster_size=150)
    labels_ehgp_b = ehgp.fit_predict(X_b)
    t_ehgp_b = time.time() - t0
    ari_ehgp_b = adjusted_rand_score(y_b, labels_ehgp_b)
    pct_ehgp_b = np.sum(labels_ehgp_b >= 0) / len(labels_ehgp_b) * 100
    print(f"E-HGP   : Time = {t_ehgp_b:.2f}s | ARI = {ari_ehgp_b:.4f} | Clustered % = {pct_ehgp_b:.2f}%")
    
    # HDBSCAN
    t0 = time.time()
    hdb = HDBSCAN(min_cluster_size=150)
    labels_hdb_b = hdb.fit_predict(X_b)
    t_hdb_b = time.time() - t0
    ari_hdb_b = adjusted_rand_score(y_b, labels_hdb_b)
    pct_hdb_b = np.sum(labels_hdb_b >= 0) / len(labels_hdb_b) * 100
    print(f"HDBSCAN : Time = {t_hdb_b:.2f}s | ARI = {ari_hdb_b:.4f} | Clustered % = {pct_hdb_b:.2f}%")
    
    # -------------------------------------------------------------------------
    # Scenario C: High-Dimensional Hubness (50,000 points, 100D, 8 clusters)
    # -------------------------------------------------------------------------
    print("\n--- Scenario C: High-Dimensional Hubness (50,000 points, 100D) ---")
    X_c, y_c = make_blobs(n_samples=50000, centers=8, n_features=100, cluster_std=3.0, random_state=42)
    
    # E-HGP
    t0 = time.time()
    ehgp = EHGPClusterer(K=2, kappa=1.5, m_reg=30, min_cluster_size=100)
    labels_ehgp_c = ehgp.fit_predict(X_c)
    t_ehgp_c = time.time() - t0
    ari_ehgp_c = adjusted_rand_score(y_c, labels_ehgp_c)
    pct_ehgp_c = np.sum(labels_ehgp_c >= 0) / len(labels_ehgp_c) * 100
    print(f"E-HGP   : Time = {t_ehgp_c:.2f}s | ARI = {ari_ehgp_c:.4f} | Clustered % = {pct_ehgp_c:.2f}%")
    
    # HDBSCAN
    t0 = time.time()
    hdb = HDBSCAN(min_cluster_size=100)
    labels_hdb_c = hdb.fit_predict(X_c)
    t_hdb_c = time.time() - t0
    ari_hdb_c = adjusted_rand_score(y_c, labels_hdb_c)
    pct_hdb_c = np.sum(labels_hdb_c >= 0) / len(labels_hdb_c) * 100
    print(f"HDBSCAN : Time = {t_hdb_c:.2f}s | ARI = {ari_hdb_c:.4f} | Clustered % = {pct_hdb_c:.2f}%")
    
    print("\n" + "="*80)
    print("HARD BENCHMARKS FINAL REPORT")
    print("="*80)
    print("| Scenario | Method | Time (s) | ARI | % Clustered |")
    print("| --- | --- | --- | --- | --- |")
    print(f"| A (Spheres 3D) | E-HGP | {t_ehgp_a:.2f}s | {ari_ehgp_a:.4f} | {pct_ehgp_a:.2f}% |")
    print(f"| A (Spheres 3D) | HDBSCAN | {t_hdb_a:.2f}s | {ari_hdb_a:.4f} | {pct_hdb_a:.2f}% |")
    print(f"| B (Var Density + Noise) | E-HGP | {t_ehgp_b:.2f}s | {ari_ehgp_b:.4f} | {pct_ehgp_b:.2f}% |")
    print(f"| B (Var Density + Noise) | HDBSCAN | {t_hdb_b:.2f}s | {ari_hdb_b:.4f} | {pct_hdb_b:.2f}% |")
    print(f"| C (Hubness 100D) | E-HGP | {t_ehgp_c:.2f}s | {ari_ehgp_c:.4f} | {pct_ehgp_c:.2f}% |")
    print(f"| C (Hubness 100D) | HDBSCAN | {t_hdb_c:.2f}s | {ari_hdb_c:.4f} | {pct_hdb_c:.2f}% |")
    print("="*80 + "\n")

if __name__ == "__main__":
    run_hard_benchmarks()
