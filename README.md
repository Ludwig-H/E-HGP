# E-HGP & PERG-HGP Repository

This repository contains historical HGP/E-HGP implementations, the experimental Rank-Gabriel PERG-HGP atlas, and a new spatial power-cover backend for massive 3-D data.

---

## 📌 Project Status: Research Prototype

> [!IMPORTANT]
> **PERG-HGP remains research software.** The CPU correctness boundary is tested, but the CUDA kernels and the 30-million-point run still require execution of the Blackwell notebook. No unexecuted notebook is presented as a benchmark result.

### Current Implementation Characteristics & Limits
1. **`PowerCover3D` (30 M path)**: streaming entropy sites, RAPIDS RBC candidate search with a mandatory runtime audit, a top-K power guard certified under its declared float32 envelope, inner/outer cubical fields, and an implicit 26-connected Borůvka MSF. Its primary output is an H0 hierarchy with a measured radius bound.
2. **`PERGHGPClusterer` (atlas path)**: explicit Rank-Gabriel cofaces, facets, MSF, condensation and voting. It remains useful experimentally, but atlas completeness and exact facet births are unknown.
3. **Exact small CPU oracles**: stable blocked power top-K, additive miniballs with primal/dual diagnostics, exhaustive power-HGP without Gabriel pruning, and cubical H0 references.
4. **Honest boundary**: the implemented spatial bound is `s + 2 * (H + delta_num)`. It is not a guarantee on flat labels, and fixed `K=10` is not an asymptotic consistency claim.

The complete July 2026 review is available in [`perg_hgp/Rapport_revue_complete_PERG_HGP_2026-07-10.md`](perg_hgp/Rapport_revue_complete_PERG_HGP_2026-07-10.md).
The backend contract and API are documented in [`perg_hgp/POWER_COVER_3D.md`](perg_hgp/POWER_COVER_3D.md). GPU acceptance is scripted in [`PERG_HGP_Blackwell_30M.ipynb`](PERG_HGP_Blackwell_30M.ipynb).

---

## 🛠️ Installation & Dependencies

To install the package in editable mode along with its dependencies:

```bash
pip install -e 'perg_hgp[test]'
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

The test suite covers both the historical atlas and the new spatial CPU oracles.

To execute the entire test suite:

```bash
python -m pytest -q perg_hgp/tests
```

To run E-HGP base tests:

```bash
python -m unittest discover -s E-HGP/tests -v
```

---

## 🗺️ Remaining GPU validation

The notebook must still establish, on the declared Blackwell machine:

- RAPIDS RBC agreement with cuVS brute force for raw-site queries and sampled grid centres;
- CuPy/NVRTC compilation and CPU/GPU equivalence of the implicit cubical MSF;
- zero uncertified top-10 power queries in strict mode;
- synchronized stage timings, NVML peak VRAM, RSS, scratch-space use and artifact persistence;
- runs at 100 k, 1 M, 3 M, 10 M and 30 M points;
- whether the maximum persistence of the synthetic validation cloud exceeds the
  measured bound; scientific data still require a branch-by-branch analysis.
