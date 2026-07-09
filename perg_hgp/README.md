# PERG-HGP: Progressive Entropic Rank-Gabriel HGP

`PERG-HGP` is a high-performance Python package for progressive, entropy-regularized hypergraph percolation clustering on massive 3D point clouds (up to 30M points, $K=10$ or $K=20$).

It implements a PyTorch-accelerated, grid-based, lazy-evaluated algorithm that scales to massive datasets by replacing complete Čech complex triangulations with entropic critical witnesses.
