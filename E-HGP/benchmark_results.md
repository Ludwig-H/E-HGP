# E-HGP Benchmark and Test Results

This document presents the benchmark and comparison results between `E-HGP` (our new entropy-regularized version), `HGP-old` (the exact combinatorial version), and `HDBSCAN`.

---

## 1. Datasets & Hyperparameters
* **2D Moons**: 600 points from nested moons with noise.
  * E-HGP: $K=2, \kappa=1.5, m_{reg}=20$, min_cluster_size = 15.
  * HGP-old: $K=2$, min_cluster_size = 15.
* **3D Blobs**: 500 points from 3 Gaussian blobs.
  * E-HGP: $K=2, \kappa=1.5, m_{reg}=30$, min_cluster_size = 15.
  * HGP-old: $K=2$, min_cluster_size = 15.
* **20D Blobs (High-Dimensional)**: 1000 points from 4 Gaussian blobs in 20 dimensions.
  * E-HGP: $K=2, \kappa=1.5, m_{reg}=30$, min_cluster_size = 20.
  * HDBSCAN: min_cluster_size = 20.

---

## 2. Performance Comparison Table

| Dataset | Method | Execution Time | Adjusted Rand Index (ARI) | Clusters Found |
| --- | --- | --- | --- | --- |
| **2D Moons** | **E-HGP** | **0.0435s** | 0.9602 | 2 |
| **2D Moons** | HGP-old | 0.6976s | **0.9800** | 2 |
| **3D Blobs** | **E-HGP** | **0.0520s** | **1.0000** | 3 |
| **3D Blobs** | HGP-old | 0.3807s | **1.0000** | 3 |
| **20D Blobs (nD)** | HDBSCAN | **0.0658s** | 0.9920 | 4 |
| **20D Blobs (nD)** | **E-HGP** | 0.1652s | **1.0000** | 4 |

---

## 3. Analysis and Conclusions

### 3.1 E-HGP vs HGP-old (Low Dimensions: 2D & 3D)
* **Execution Speed**: E-HGP is **16x faster** in 2D and **7x faster** in 3D compared to the exact combinatorial version. This is because E-HGP avoids constructing the complete Delaunay/Voronoi mosaic and uses a dynamic lazy Borůvka algorithm on localized $\beta$-neighbor prefix lists.
* **Clustering Quality**: The ARI scores are virtually identical (perfect alignment in 3D, and $>0.96$ in 2D). E-HGP successfully preserves the hierarchical topological structure of $K$-polyhedrons.

### 3.2 E-HGP vs HDBSCAN (High Dimension: 20D)
* **Combinatorial Feasibility**: `HGP-old` cannot be run in 20D because the Delaunay order-$K$ mosaic is impossible to construct in high dimensions. `E-HGP` runs successfully in **0.16s**.
* **Clustering Quality**: E-HGP achieves a **perfect ARI score of 1.0000** on the 20D blobs, outperforming HDBSCAN (ARI = 0.9920). This shows the power of hypergraph percolation ($K$-polyhedron tracking) over standard graph-based single-linkage extensions.
