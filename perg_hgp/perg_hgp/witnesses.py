import torch
import numpy as np
from .grid import SpatialGrid3D
from .rank_field import compute_rank_soft_dp

class WitnessPool:
    """
    Manages a pool of witnesses.
    Each witness is represented by its coordinates, its rank, and its signature.
    """
    def __init__(self, coords, rank, scores=None, signatures=None):
        self.coords = coords # (M, 3) PyTorch tensor
        self.rank = rank
        self.scores = scores if scores is not None else torch.zeros(coords.shape[0], device=coords.device)
        self.signatures = signatures # (M, K+1) signatures (indices of top-K closest sites)

def refine_witness_pool(witness_pool, Z, a, grid_z, cfg):
    """
    Runs fixed-point iterations on witness coordinates to find critical witnesses of rank k.
    """
    device = Z.device
    W_coords = witness_pool.coords.clone()
    M = W_coords.shape[0]
    rank = witness_pool.rank
    
    # Run fixed point iterations
    # w = (1-gamma)*w + gamma*T_k(w)
    gamma = cfg.gamma
    
    for it in range(cfg.fixed_point_iters_per_temp):
        # 1. Query local neighbors of witnesses on the grid Z
        nbr_indices, nbr_dists_sq = grid_z.query_knn_grid(W_coords, m_local=cfg.m_active)
        
        # 2. Compute energies E_i = ||w - z_i||^2 + a_i
        # X_nbrs has shape (M, m_active, 3)
        Z_nbrs = Z[nbr_indices]
        a_nbrs = a[nbr_indices]
        
        diff = Z_nbrs - W_coords[:, None, :]
        dist_sq = torch.sum(diff ** 2, dim=2)
        E = dist_sq + a_nbrs # (M, m_active)
        
        # We run the rank DP for each witness
        # To keep it simple, we do it in a loop or vectorise if we can.
        # Since M is large, let's write a batch-friendly or loop-friendly update.
        # Note: If M is very large (e.g. millions), we chunk it.
        chunk_size = 5000
        for start in range(0, M, chunk_size):
            end = min(start + chunk_size, M)
            E_chunk = E[start:end]
            Z_nbrs_chunk = Z_nbrs[start:end]
            
            for i in range(end - start):
                idx = start + i
                Q_val, b_val = compute_rank_soft_dp(E_chunk[i], rank, cfg.rank_eps_schedule[-1])
                
                # Barycenter T = sum_j b_val[rank-1, j] * Z_nbrs[j]
                b_weights = b_val[rank - 1] # shape (m_active,)
                T = torch.sum(b_weights[:, None] * Z_nbrs_chunk[i], dim=0)
                
                # Update coordinates
                W_coords[idx] = (1.0 - gamma) * W_coords[idx] + gamma * T
                
    # Re-evaluate final scores and signatures
    nbr_indices, nbr_dists_sq = grid_z.query_knn_grid(W_coords, m_local=cfg.m_active)
    Z_nbrs = Z[nbr_indices]
    a_nbrs = a[nbr_indices]
    diff = Z_nbrs - W_coords[:, None, :]
    dist_sq = torch.sum(diff ** 2, dim=2)
    E = dist_sq + a_nbrs
    
    scores = torch.zeros(M, device=device)
    signatures = torch.zeros((M, rank + 1), dtype=torch.int64, device=device)
    
    for i in range(M):
        # Signature is the indices of top-(rank+1) closest sites
        sorted_idx = torch.argsort(E[i])
        signatures[i] = nbr_indices[i, sorted_idx[:rank + 1]]
        
        # Score measures how close it is to a critical point
        # e.g., negative distance to its update
        Q_val, b_val = compute_rank_soft_dp(E[i], rank, cfg.rank_eps_schedule[-1])
        b_weights = b_val[rank - 1]
        T = torch.sum(b_weights[:, None] * Z_nbrs[i], dim=0)
        dist_to_update = torch.sum((W_coords[i] - T) ** 2)
        scores[i] = -dist_to_update
        
    return WitnessPool(W_coords, rank, scores, signatures)

def lift_witness_pool(witness_pool, Z, a, eta, grid_z, cfg):
    """
    Lifts witnesses of rank k to rank k+1.
    Generates candidate seeds using Shell Lift and active supports.
    """
    device = Z.device
    W_coords = witness_pool.coords
    M = W_coords.shape[0]
    rank = witness_pool.rank
    next_rank = rank + 1
    
    seeds = []
    
    # 1. Generator A: Continuation
    seeds.append(W_coords.clone())
    
    # 2. Generator B: Shell lift & Generator C: Support centers
    # For each witness, we look at its active set shell
    nbr_indices, nbr_dists_sq = grid_z.query_knn_grid(W_coords, m_local=cfg.m_active)
    Z_nbrs = Z[nbr_indices]
    a_nbrs = a[nbr_indices]
    diff = Z_nbrs - W_coords[:, None, :]
    dist_sq = torch.sum(diff ** 2, dim=2)
    E = dist_sq + a_nbrs
    
    shell_lift_seeds = []
    support_seeds = []
    
    for i in range(M):
        # Sort neighbor sites by energy
        sorted_idx = torch.argsort(E[i])
        top_indices = nbr_indices[i, sorted_idx]
        top_Z = Z[top_indices]
        
        # Shell lift: take the top-rank sites and mix with shell site at rank+1
        # w' = average(top-rank sites + top-next_rank site)
        if len(top_Z) >= next_rank:
            mix_Z = top_Z[:next_rank]
            w_prime = torch.mean(mix_Z, dim=0)
            shell_lift_seeds.append(w_prime)
            
            # Support centers: take subsets of size 2, 3, or 4 from the top sites
            # and compute their Euclidean barycenters as seeds
            for size in range(2, min(5, next_rank + 1)):
                subset_Z = top_Z[:size]
                c_S = torch.mean(subset_Z, dim=0)
                support_seeds.append(c_S)
                
    if shell_lift_seeds:
        seeds.append(torch.stack(shell_lift_seeds))
    if support_seeds:
        seeds.append(torch.stack(support_seeds))
        
    all_seeds = torch.cat(seeds, dim=0)
    
    # 3. Rank capacity filter (Generator D)
    # Filter seeds to keep only those whose rank capacity is close to next_rank
    # Capacity is the number of sites whose energy is smaller than the local radius
    # To do this fast, we can sample or query the grid
    nbr_indices_s, nbr_dists_sq_s = grid_z.query_knn_grid(all_seeds, m_local=cfg.m_active)
    Z_nbrs_s = Z[nbr_indices_s]
    a_nbrs_s = a[nbr_indices_s]
    diff_s = Z_nbrs_s - all_seeds[:, None, :]
    dist_sq_s = torch.sum(diff_s ** 2, dim=2)
    E_s = dist_sq_s + a_nbrs_s
    
    # We estimate the rank at each seed
    # By counting how many points are within a threshold
    # For a seed, local radius at next_rank is approximately the next_rank-th energy
    # We count how many energies are smaller than the (next_rank + 2)-th energy
    # to see if it is in the target rank range.
    valid_idx = []
    for i in range(all_seeds.shape[0]):
        sorted_E = torch.sort(E_s[i]).values
        # Local radius estimate
        r_est = sorted_E[min(next_rank - 1, len(sorted_E) - 1)]
        # Count how many points are below r_est + epsilon
        # If count is close to next_rank, it's a good seed
        count = torch.sum(E_s[i] <= r_est + 1e-3).item()
        if abs(count - next_rank) <= 2:
            valid_idx.append(i)
            
    if not valid_idx:
        # Fallback to a random selection of seeds to avoid empty pool
        valid_idx = list(range(min(1000, all_seeds.shape[0])))
        
    filtered_seeds = all_seeds[valid_idx]
    
    # Limit to the budget_per_rank
    if filtered_seeds.shape[0] > cfg.budget_per_rank:
        idx = torch.randperm(filtered_seeds.shape[0])[:cfg.budget_per_rank]
        filtered_seeds = filtered_seeds[idx]
        
    return WitnessPool(filtered_seeds, next_rank)
