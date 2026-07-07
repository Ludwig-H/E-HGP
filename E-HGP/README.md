# E-HGP: Entropy-Regularized GPU-friendly Hypergraph Percolation

`E-HGP` is a Python package for GPU-friendly, entropy-regularized hypergraph percolation clustering. It extends the traditional exact combinatorial Hypergraph Percolation (HGP) to be highly scalable, robust against noise/hubs, and compatible with GPU/TPU architectures.

## Key Features

1. **Entropy Regularization**: Projects data points into weighted power sites with controlled local entropy to reduce noise and hubness effects.
2. **Miniball Additive Power Solvers**: Computes exact Čech birth radii using vectorized support enumeration or active-set quadratic programming.
3. **Global Gabriel Certification**: Uses fast batch Matrix Multiplication (GEMM) to certify candidate simplexes directly.
4. **Lazy Borůvka MST**: Discovers the certified $K$-MST on the dual graph of faces using localized $\beta$-neighbor prefix lists, avoiding expensive global triangulation.
5. **Scikit-Learn Compatibility**: Implements the standard `BaseEstimator` and `ClusterMixin` API.

## Installation

```bash
pip install -e /workspaces/E-HGP/E-HGP
```

## Quick Start

```python
import numpy as np
from sklearn.datasets import make_blobs
from e_hgp import EHGPClusterer

# Generate data
X, y = make_blobs(n_samples=200, centers=3, n_features=2, random_state=42)

# Cluster using E-HGP
clusterer = EHGPClusterer(K=2, kappa=2.0, m_reg=30, min_cluster_size=10)
labels = clusterer.fit_predict(X)

print("Cluster labels:", np.unique(labels))
print("Exactness report:", clusterer.exactness_report_)
```
