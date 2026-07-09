import torch
import numpy as np

def solve_support_ball_3d(S_Z, S_a):
    """
    Solves the weighted miniball problem exactly for a small support set of size <= 4 in 3D.
    S_Z: shape (size, 3)
    S_a: shape (size,)
    Returns:
        center: (3,)
        radius_sq: float
    """
    size = S_Z.shape[0]
    device = S_Z.device
    dtype = S_Z.dtype
    
    if size == 1:
        return S_Z[0], S_a[0]
        
    # Pivot is element 0
    v = S_Z[1:] - S_Z[0] # shape (size-1, 3)
    
    # Gram matrix G
    G = torch.matmul(v, v.t()) # shape (size-1, size-1)
    
    # RHS h_u = 0.5 * (||v_u||^2 + a_u - a_0)
    v_norm_sq = torch.sum(v ** 2, dim=1)
    h = 0.5 * (v_norm_sq + S_a[1:] - S_a[0])
    
    try:
        # Solve G alpha = h
        if size == 2:
            alpha = h / G[0, 0]
        else:
            alpha = torch.linalg.solve(G, h)
    except RuntimeError:
        # Fallback to pseudo-inverse
        pinv = torch.linalg.pinv(G)
        alpha = torch.matmul(pinv, h)
        
    center = S_Z[0] + torch.matmul(alpha, v)
    radius_sq = S_a[0] + torch.dot(alpha, torch.matmul(G, alpha))
    
    return center, radius_sq


def solve_weighted_miniball_active_set_3d(Z_coface, a_coface, max_iter=50, tol=1e-6):
    """
    3D active-set weighted miniball solver for a single coface.
    Z_coface: shape (K+1, 3)
    a_coface: shape (K+1,)
    """
    K_plus_1 = Z_coface.shape[0]
    device = Z_coface.device
    
    # Start with active set containing the first element
    S = [0]
    
    center = Z_coface[0]
    radius_sq = a_coface[0]
    
    for it in range(max_iter):
        sub_Z = Z_coface[S]
        sub_a = a_coface[S]
        
        # 1. Solve for current active support
        c, r2 = solve_support_ball_3d(sub_Z, sub_a)
        center = c
        radius_sq = r2
        
        # 2. Check containment and find the most violating point
        diff = Z_coface - center
        phi = torch.sum(diff ** 2, dim=1) + a_coface
        violations = phi - radius_sq
        
        max_viol_idx = torch.argmax(violations).item()
        max_viol = violations[max_viol_idx].item()
        
        if max_viol <= tol:
            # Optimal miniball found
            break
            
        # 3. Add violating point to active set
        if max_viol_idx not in S:
            S.append(max_viol_idx)
            # Active set size in 3D is at most 4
            if len(S) > 4:
                # Remove the oldest or least violating index to maintain size <= 4
                S.pop(0)
        else:
            break
            
    return center, radius_sq


def extract_top_cofaces(witness_pool, Z, a, eta, grid_z, K, cfg):
    """
    Extracts candidate cofaces from the witness pool.
    """
    device = Z.device
    W_coords = witness_pool.coords
    M = W_coords.shape[0]
    
    # 1. Query neighborhood of witnesses on Z
    nbr_indices, nbr_dists_sq = grid_z.query_knn_grid(W_coords, m_local=cfg.m_active)
    Z_nbrs = Z[nbr_indices]
    a_nbrs = a[nbr_indices]
    diff = Z_nbrs - W_coords[:, None, :]
    dist_sq = torch.sum(diff ** 2, dim=2)
    E = dist_sq + a_nbrs
    
    cofaces_list = []
    centers_list = []
    weights_list = []
    
    for i in range(M):
        # Sort neighbors by energy
        sorted_idx = torch.argsort(E[i])
        coface = nbr_indices[i, sorted_idx[:K + 1]]
        
        # Sort indices of the coface for deduplication
        coface_sorted, _ = torch.sort(coface)
        cofaces_list.append(coface_sorted)
        
    # Stack and deduplicate
    cofaces_tensor = torch.stack(cofaces_list)
    unique_cofaces = torch.unique(cofaces_tensor, dim=0)
    
    U = unique_cofaces.shape[0]
    
    # Solve weighted miniball for unique cofaces
    for i in range(U):
        cof_idx = unique_cofaces[i]
        Z_cof = Z[cof_idx]
        a_cof = a[cof_idx]
        
        c, r2 = solve_weighted_miniball_active_set_3d(Z_cof, a_cof, tol=cfg.gabriel_tol)
        centers_list.append(c)
        weights_list.append(torch.sqrt(torch.clamp(r2, min=0.0)))
        
    centers_tensor = torch.stack(centers_list)
    weights_tensor = torch.stack(weights_list)
    
    return unique_cofaces, centers_tensor, weights_tensor
