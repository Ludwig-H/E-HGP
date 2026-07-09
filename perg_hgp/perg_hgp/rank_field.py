import torch

def compute_elementary_symmetric_polynomials_prefix_suffix(u, Kmax):
    """
    Computes prefix and suffix tables for elementary symmetric polynomials.
    u: shape (m,)
    Kmax: maximum polynomial degree
    """
    m = u.shape[0]
    device = u.device
    dtype = u.dtype
    
    # prefix[t, l] is e_l(u_0, ..., u_{t-1})
    # shape (m + 1, Kmax + 1)
    prefix = torch.zeros((m + 1, Kmax + 1), dtype=dtype, device=device)
    prefix[:, 0] = 1.0
    
    for t in range(1, m + 1):
        u_val = u[t - 1]
        for l in range(1, Kmax + 1):
            prefix[t, l] = prefix[t - 1, l] + u_val * prefix[t - 1, l - 1]
            
    # suffix[t, l] is e_l(u_{t}, ..., u_{m-1})
    # shape (m + 1, Kmax + 1)
    suffix = torch.zeros((m + 1, Kmax + 1), dtype=dtype, device=device)
    suffix[:, 0] = 1.0
    
    for t in range(m - 1, -1, -1):
        u_val = u[t]
        for l in range(1, Kmax + 1):
            suffix[t, l] = suffix[t + 1, l] + u_val * suffix[t + 1, l - 1]
            
    return prefix, suffix


def compute_rank_soft_dp(E, Kmax, eps):
    """
    Computes soft rank values and shell weights for a query.
    E: shape (m,) energies for the active set.
    Kmax: maximum rank needed.
    eps: temperature epsilon.
    """
    device = E.device
    dtype = E.dtype
    m = E.shape[0]
    
    # 1. Rescale energies to avoid exponent overflow
    E0 = torch.min(E)
    u = torch.exp(-(E - E0) / eps)
    
    # 2. Compute prefix/suffix DP tables
    prefix, suffix = compute_elementary_symmetric_polynomials_prefix_suffix(u, Kmax + 1)
    
    # e_k(u) is prefix[m, k]
    e_vals = prefix[m]
    
    # 3. Free energy and soft ranks
    # F_k = k * E0 - eps * log(e_k)
    # Handle log of extremely small values safely
    e_vals_safe = torch.clamp(e_vals, min=1e-35)
    F = torch.arange(Kmax + 2, device=device, dtype=dtype) * E0 - eps * torch.log(e_vals_safe)
    
    # soft rank Q_k = F_k - F_{k-1}
    Q = F[1:] - F[:-1] # length Kmax + 1
    
    # 4. Inclusion margins p_i^k = u_i * e_{k-1}(u_{-i}) / e_k(u)
    # We want to compute this for k = 1 to Kmax
    # e_{k-1}(u_{-i}) = sum_{l=0}^{k-1} prefix[i, l] * suffix[i+1, k-1-l]
    p = torch.zeros((Kmax + 2, m), dtype=dtype, device=device)
    # for k = 0, p_i^0 = 0.0

    
    for k in range(1, Kmax + 2):
        ek = e_vals_safe[k]
        for i in range(m):
            val = 0.0
            for l in range(k):
                val += prefix[i, l] * suffix[i + 1, k - 1 - l]
            p[k, i] = u[i] * val / ek
            
    # clamp margins to [0, 1]
    p = torch.clamp(p, 0.0, 1.0)
    
    # shell weights: b_i^k = p_i^k - p_i^{k-1}
    b = p[1:] - p[:-1] # shape (Kmax + 1, m)
    
    # Ensure shell weights are positive and normalize to sum to 1.0
    b = torch.clamp(b, min=0.0)
    b_sums = torch.sum(b, dim=1, keepdim=True)
    b_sums[b_sums < 1e-12] = 1.0
    b = b / b_sums
    
    return Q, b


def compute_rank_shell_weights(E, k, eps):
    """
    Convenience wrapper to get shell weights b_i^k for a single rank k.
    E: shape (m,)
    k: rank (1-indexed)
    eps: temperature
    """
    Q, b = compute_rank_soft_dp(E, k, eps)
    return b[k - 1]
