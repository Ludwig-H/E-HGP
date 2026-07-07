# Design of E-HGP: Entropy-Regularized GPU-friendly Hypergraph Percolation

This document outlines the architecture, data structures, and mathematical algorithms for the new `E-HGP` package. The design is optimized for massive parallelization (GPU-friendly) and memory efficiency, avoiding full high-dimensional Delaunay triangulations.

---

## 1. Directory Structure

We will create a clean Python package structure inside the `E-HGP` directory:

```text
/workspaces/E-HGP/E-HGP/
├── README.md
├── DESIGN.md
├── pyproject.toml
├── setup.py
├── e_hgp/
│   ├── __init__.py
│   ├── regularization.py  # Local entropy and Sinkhorn regularization
│   ├── miniball.py        # Additive weighted miniball solvers (batch active-set / enum)
│   ├── gabriel.py         # Batch Gabriel certification using GEMM
│   ├── lazy_boruvka.py    # High-dimensional lazy Borůvka MST solver
│   └── clustering.py      # Condensed tree hierarchy extraction (HDBSCAN-like)
└── tests/
    └── test_e_hgp.py      # Unit and integration tests
```

---

## 2. Data Structures & GPU Compatibility

To run efficiently on both CPU (NumPy/SciPy) and GPU (PyTorch/CuPy), all core operations are written using **vectorized tensor operations**. We will use PyTorch as our primary engine if available, with a clean NumPy fallback, or write NumPy code structured in a way that maps directly to GPU tensor operations.

### 2.1 Site Representation
* **Coordinates ($Z$)**: Dense tensor of shape `(n, p)` representing regularized site centers.
* **Weights ($a$)**: Dense tensor of shape `(n,)` representing the power weights (local variances).
* **Norm Cache (`norm_z_a`)**: Dense tensor of shape `(n,)` containing $\|z_i\|^2 + a_i$. This enables extremely fast distance calculations via Matrix Multiplication (GEMM):
  $$
  \Phi(C, Z)_{b, j} = \|c_b - z_j\|^2 + a_j = \|c_b\|^2 + (\|z_j\|^2 + a_j) - 2 \langle c_b, z_j \rangle
  $$

### 2.2 Simplexes & Candidates
* **Cofaces**: Represented as a dense integer tensor of shape `(B, K+1)`. By keeping the cardinality fixed ($K+1$), we avoid ragged arrays and enable contiguous GPU memory execution.
* **Deduplication**: Radix sort or hashing on sorted tuples of size $K+1$.

### 2.3 $\beta$-Lists (Lazy Borůvka)
* **Voisinage ($\beta$-Neighbors)**: Integer tensor of shape `(n, L)` storing the indices of the $L$ closest neighbors for each site.
* **Weights ($\beta$-Weights)**: Float tensor of shape `(n, L)` storing $\beta_{ij} = \rho_\kappa(\{i, j\})$.
* **Tail bounds ($\lambda^L$)**: Float tensor of shape `(n,)` storing the birth radius of the first omitted neighbor.

---

## 3. Core Modules

### 3.1 `regularization.py`
Given $X \in \mathbb{R}^{n \times p}$ and target entropy threshold $\kappa \ge 1$:
1. Find local neighborhood $A_i$ (e.g. $m_{reg}$-NN).
2. Solve for Gibbs temperature $\eta_i$ such that $H(q_i) = \log \kappa$.
   * Done via vectorized bisection or Newton-Raphson.
3. Compute regularized centers $z_i$ and variances $a_i$:
   $$
   z_i = \sum_{j \in A_i} q_{ij} x_j, \quad a_i = \sum_{j \in A_i} q_{ij} \|x_j - z_i\|^2
   $$

### 3.2 `miniball.py` (Weighted Miniball)
Solves the additive weighted miniball problem:
$$
\rho_\kappa(\sigma)^2 = \min_{y \in \mathbb{R}^p} \max_{i \in \sigma} (\|y - z_i\|^2 + a_i)
$$
* **`support_enum`**: For small $K, p \le 5$. Evaluates all $2^{K+1}-1$ subsets of $\sigma$, solves the linear system $G\alpha = h$, computes the center/radius, and verifies validity. Fully vectorized over a batch of cofaces.
* **`active_set`**: For larger $K$. Iterative active-set method (similar to Welzl but for power distances). Can be batched by tracking active sets in a fixed-size buffer.

### 3.3 `gabriel.py`
Performs the global Gabriel certificate test on a batch of cofaces:
1. Compute the miniball center $c_b$ and radius squared $t_b$ for each coface $\sigma_b$.
2. Compute power distances $\Phi(C, Z)$ of shape `(B, n)`.
3. Verify that for all $j \notin \sigma_b$, $\Phi_{b, j} \ge t_b - \text{tol}$.

### 3.4 `lazy_boruvka.py`
Finds the $K$-MST on the dual graph:
1. Initialize components using a Union-Find data structure.
2. For each active component, intersect prefix lists of $N_i^L$ for vertices in boundary $K$-faces $\tau$ to find candidate $(K+1)$-cofaces.
3. Certify the best candidate with a global Gabriel test.
4. Verify if the best candidate is smaller than the tail bound $\lambda^L$. If not, expand lists.

### 3.5 `clustering.py`
* Takes the $K$-MST edges and weights.
* Implements a fast condensed tree builder and extracts labels/clusters using EOM.
