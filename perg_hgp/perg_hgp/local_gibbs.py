import torch
import numpy as np

def solve_local_gibbs_entropy(nbr_dists_sq, entropy_target, max_iter=48, tol=1e-6):
    """
    Solves for the Gibbs temperatures eta_i using vectorized bisection search in PyTorch.
    nbr_dists_sq: PyTorch tensor of shape (N, m) representing squared neighbor distances.
    entropy_target: target effective entropy value K_rho (log_kappa = log(K_rho)).
    """
    if nbr_dists_sq.ndim != 2 or nbr_dists_sq.shape[1] == 0:
        raise ValueError("nbr_dists_sq must have shape (N, m) with m >= 1")
    if not torch.isfinite(nbr_dists_sq).all():
        raise ValueError("nbr_dists_sq contains NaN or Inf")

    device = nbr_dists_sq.device
    N, m = nbr_dists_sq.shape
    dtype = nbr_dists_sq.dtype
    target = float(entropy_target)
    if not np.isfinite(target) or target < 1.0 or target > m:
        raise ValueError(f"entropy_target must satisfy 1 <= target <= {m}; got {target}")

    log_target = float(np.log(target))
    if target == 1.0:
        Q = torch.zeros_like(nbr_dists_sq)
        Q.scatter_(1, torch.argmin(nbr_dists_sq, dim=1, keepdim=True), 1.0)
        return Q, torch.zeros(N, dtype=dtype, device=device)

    if target == float(m):
        Q = torch.full_like(nbr_dists_sq, 1.0 / m)
        return Q, torch.full((N,), float('inf'), dtype=dtype, device=device)

    # Subtracting the row minimum and dividing by the row range makes the
    # numerical solve invariant to translations and changes of physical units.
    shifted = nbr_dists_sq - torch.min(nbr_dists_sq, dim=1, keepdim=True).values
    row_scale = torch.max(shifted, dim=1).values
    scale_floor = torch.finfo(dtype).tiny
    normalized = shifted / torch.clamp(row_scale, min=scale_floor)[:, None]

    def get_entropy(t):
        # Prevent division by zero
        t_clamped = torch.clamp(t, min=1e-12)
        exponent = -normalized / t_clamped[:, None]
        max_exp = torch.max(exponent, dim=1, keepdim=True).values
        exp_w = torch.exp(exponent - max_exp)
        sum_exp = torch.sum(exp_w, dim=1, keepdim=True)
        q = exp_w / sum_exp
        log_q = exponent - max_exp - torch.log(sum_exp)
        return -torch.sum(q * log_q, dim=1)

    low = torch.full((N,), max(torch.finfo(dtype).eps ** 2, 1e-12), dtype=dtype, device=device)
    high = torch.ones((N,), dtype=dtype, device=device)

    # If several sites have exactly the same minimum cost, a Gibbs law cannot
    # reach an entropy below log(tie_count). Uniform mass over the minimizers is
    # nevertheless an optimal feasible solution of the constrained problem.
    entropy_low = get_entropy(low)
    tied_minima = entropy_low > log_target + tol

    # Robust bracketing of high temperature bound
    entropy_high = get_entropy(high)
    for _ in range(32):
        too_small = entropy_high < log_target
        if not torch.any(too_small):
            break
        high = torch.where(too_small, 2.0 * high, high)
        entropy_high = get_entropy(high)

    for _ in range(max_iter):
        mid = 0.5 * (low + high)
        entropy = get_entropy(mid)

        too_small = entropy < log_target
        low = torch.where(too_small, mid, low)
        high = torch.where(too_small, high, mid)

    eta_normalized = 0.5 * (low + high)
    exponent = -normalized / eta_normalized[:, None]
    max_exp = torch.max(exponent, dim=1, keepdim=True).values
    exp_w = torch.exp(exponent - max_exp)
    Q = exp_w / torch.sum(exp_w, dim=1, keepdim=True)

    if torch.any(tied_minima):
        minima = shifted[tied_minima] == 0
        Q_ties = minima.to(dtype)
        Q_ties /= torch.sum(Q_ties, dim=1, keepdim=True)
        Q[tied_minima] = Q_ties
        eta_normalized[tied_minima] = 0.0

    eta = eta_normalized * row_scale

    return Q, eta


def compute_regularized_sites(X, nbr_indices, nbr_dists_sq, entropy_target, shift_cost=False):
    """
    Computes regularized sites (Z, a) from points X and neighbors in memory-safe chunks.
    X: shape (N, 3)
    nbr_indices: shape (N, m_local)
    nbr_dists_sq: shape (N, m_local)
    """
    device = X.device
    N, m = nbr_indices.shape

    chunk_size = 500000
    Z_list = []
    a_list = []
    eta_list = []

    for start in range(0, N, chunk_size):
        end = min(start + chunk_size, N)

        # Ensure neighbors and distances are on device for math operations
        nbr_idx_c = nbr_indices[start:end].to(device)
        nbr_dists_c = nbr_dists_sq[start:end].to(device)

        if shift_cost:
            rho = torch.sqrt(nbr_dists_c[:, 0:1])
            dists = torch.sqrt(nbr_dists_c)
            cost_c = (torch.clamp(dists - rho, min=0.0)) ** 2
        else:
            cost_c = nbr_dists_c

        Q_c, eta_c = solve_local_gibbs_entropy(cost_c, entropy_target)

        # Gather neighbor coordinates on device
        X_nbrs_c = X[nbr_idx_c] # (chunk_len, m, 3)

        Z_c = torch.sum(Q_c[:, :, None] * X_nbrs_c, dim=1)

        diff_c = X_nbrs_c - Z_c[:, None, :]
        dist_to_center_c = torch.sum(diff_c ** 2, dim=2)
        a_c = torch.sum(Q_c * dist_to_center_c, dim=1)

        # Accumulate results on CPU to save VRAM
        Z_list.append(Z_c.cpu())
        a_list.append(a_c.cpu())
        eta_list.append(eta_c.cpu())

    Z = torch.cat(Z_list, dim=0).to(device)
    a = torch.cat(a_list, dim=0).to(device)
    eta = torch.cat(eta_list, dim=0).to(device)

    return Z, a, eta
