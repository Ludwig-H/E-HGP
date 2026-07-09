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
    
    # Bisection search bounds
    low = torch.full((N,), 1e-8, dtype=dtype, device=device)
    high = 100.0 * torch.max(nbr_dists_sq, dim=1).values + 1e-5
    
    for _ in range(max_iter):
        mid = 0.5 * (low + high)
        
        # Gibbs weights: q = exp(-D / mid) / sum(exp(-D / mid))
        exponent = -nbr_dists_sq / mid[:, None]
        max_exp = torch.max(exponent, dim=1, keepdim=True).values
        exp_w = torch.exp(exponent - max_exp)
        sum_exp = torch.sum(exp_w, dim=1, keepdim=True)
        q = exp_w / sum_exp
        
        # Entropy: H(q) = - sum(q * log_q)
        log_q = exponent - max_exp - torch.log(sum_exp)
        entropy = -torch.sum(q * log_q, dim=1)
        
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
    Computes regularized sites (Z, a) from points X and neighbors.
    X: shape (N, 3)
    nbr_indices: shape (N, m_local)
    nbr_dists_sq: shape (N, m_local)
    """
    device = X.device
    N, m = nbr_indices.shape
    
    # Shift cost like UMAP if shift_cost is True: c_ij = max(0, d_ij - rho_i)^2
    if shift_cost:
        rho = torch.sqrt(nbr_dists_sq[:, 0:1]) # closest neighbor distance
        dists = torch.sqrt(nbr_dists_sq)
        cost = (torch.clamp(dists - rho, min=0.0)) ** 2
    else:
        cost = nbr_dists_sq
        
    # Solve local Gibbs
    Q, eta = solve_local_gibbs_entropy(cost, entropy_target)
    
    # Compute centers z_i = sum_j Q_ij X_nbr_j
    # Indexing: X[nbr_indices] has shape (N, m, 3)
    # Weights: Q[:, :, None] has shape (N, m, 1)
    X_nbrs = X[nbr_indices]
    Z = torch.sum(Q[:, :, None] * X_nbrs, dim=1)
    
    # Compute variances a_i = sum_j Q_ij ||X_nbr_j - z_i||^2
    diff = X_nbrs - Z[:, None, :]
    dist_to_center = torch.sum(diff ** 2, dim=2) # (N, m)
    a = torch.sum(Q * dist_to_center, dim=1)
    
    return Z, a, eta
