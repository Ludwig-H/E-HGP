import torch
import numpy as np

def solve_local_gibbs_entropy(nbr_dists_sq, entropy_target, max_iter=32, tol=1e-5):
    """
    Solves for the Gibbs temperatures eta_i using vectorized bisection search in PyTorch.
    nbr_dists_sq: PyTorch tensor of shape (N, m) representing squared neighbor distances.
    entropy_target: target effective entropy value K_rho (log_kappa = log(K_rho)).
    """
    device = nbr_dists_sq.device
    N, m = nbr_dists_sq.shape
    dtype = nbr_dists_sq.dtype

    log_target = np.log(entropy_target)
    log_m = np.log(m)
    effective_target = min(log_target, log_m - 1e-6)

    def get_entropy(t):
        # Prevent division by zero
        t_clamped = torch.clamp(t, min=1e-12)
        exponent = -nbr_dists_sq / t_clamped[:, None]
        max_exp = torch.max(exponent, dim=1, keepdim=True).values
        exp_w = torch.exp(exponent - max_exp)
        sum_exp = torch.sum(exp_w, dim=1, keepdim=True)
        q = exp_w / sum_exp
        log_q = exponent - max_exp - torch.log(sum_exp)
        return -torch.sum(q * log_q, dim=1)

    # Bisection search bounds
    low = torch.full((N,), 1e-8, dtype=dtype, device=device)
    high = 100.0 * torch.max(nbr_dists_sq, dim=1).values + 1e-3

    # Robust bracketing of high temperature bound
    entropy_high = get_entropy(high)
    for _ in range(8):
        too_small = entropy_high < effective_target
        if not torch.any(too_small):
            break
        high = torch.where(too_small, 2.0 * high, high)
        entropy_high = get_entropy(high)

    for _ in range(max_iter):
        mid = 0.5 * (low + high)
        entropy = get_entropy(mid)

        # Check convergence
        if torch.max(high - low).item() < tol:
            break

        too_small = entropy < effective_target
        low = torch.where(too_small, mid, low)
        high = torch.where(too_small, high, mid)

    eta = 0.5 * (low + high)
    exponent = -nbr_dists_sq / eta[:, None]
    max_exp = torch.max(exponent, dim=1, keepdim=True).values
    exp_w = torch.exp(exponent - max_exp)
    Q = exp_w / torch.sum(exp_w, dim=1, keepdim=True)

    # If target entropy exceeds log(m), set to uniform weights
    if log_target >= log_m:
        Q.fill_(1.0 / m)
        eta.fill_(float('inf'))

    return Q, eta


def compute_regularized_sites(X, nbr_indices, nbr_dists_sq, entropy_target, shift_cost=True):
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
