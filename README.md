# E-HGP & PERG-HGP Repository

This repository contains the implementations of **Entropy-Regularized Hypergraph Percolation (E-HGP)** and its PyTorch-accelerated extension **Progressive Entropic Rank-Gabriel HGP (PERG-HGP)**.

---

## 📌 Project Status: Research Prototype

> [!IMPORTANT]
> **PERG-HGP** is currently a research prototype in active auditing and development.
> Following the technical and mathematical audit of July 2026, the codebase is undergoing a structured alignment process to bridge gaps between candidate witness complex generation and exact Čech/K-MST hierarchies.

### Current Implementation Characteristics & Limits
1. **Hierarchical Label Extraction**: Currently, `condense_tree` and `extract_labels` identify final connected components of the dual graph of certified facets.
2. **K-NN Power Search**: K-NN queries use a spatial grid on regularized sites to perform fast local search. A dynamic capping mechanism guarantees that neighborhood sizes do not exceed dataset dimensions, preventing out-of-bounds queries for small $N$.
3. **Out-of-Core Facet Deduplication**: Employs a dynamique bucket-sorting strategy to deduplicate massive collections of facets on disk. A second-level in-memory partition protects against bucket skew under highly non-uniform distributions.
4. **Exposed Fitted Attributes**: The estimator exposes Scikit-Learn compliant fitted attributes: `cofaces_`, `coface_weights_`, `facets_`, `msf_` (minimum spanning forest), and `merge_tree_`.

---

## 🛠️ Installation & Dependencies

To install the package in editable mode along with its dependencies:

```bash
pip install -e perg_hgp/
```

### Core Dependencies
* `numpy >= 1.20`
* `scipy >= 1.7`
* `torch >= 2.0`
* `scikit-learn >= 1.0`
* `pyyaml`
* `tqdm`

---

## 🧪 Running Unit Tests

The test suite covers exactness mode runs, checkpoint resuming, out-of-core deduplication, lazy Borůvka MST solvers, and regression testing for small datasets.

To execute the entire test suite:

```bash
pytest perg_hgp/tests
```

To run E-HGP base tests:

```bash
python -m unittest discover -s E-HGP/tests -v
```

---

## 🗺️ Roadmap & Implementation Plan

Following the July 2026 audit, the codebase is transitioning through these milestones:

* **PR 0**: Secure API assertions, dynamic neighborhood capping, Scikit-Learn compliant attributes, dependency declaration, and license clarification (Completed).
* **PR 1**: Implement an exact CPU-only reference oracle for small $N$ to validate HGP math and serve as a correctness boundary.
* **PR 2 & 3**: Standardize canonical Gibbs regularization, unify top-K power neighborhood search, and implement the complete progressive witness rank annealing loop.
* **PR 4 & 5**: Implement exact active-set weighted miniball solvers, Gabriel certification, and deterministically condensed hierarchical trees.
* **PR 6 & 7**: Out-of-core robust checkpoints, recursive disk bucket-sorting under extreme skew, and profiled CUDA/Triton kernels for bottlenecks.
* **PR 8**: Instrumented end-to-end 30M point cloud scalability validation report.
