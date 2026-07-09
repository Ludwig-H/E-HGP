import torch
import numpy as np
from .grid import SpatialGrid3D
from .rank_field import compute_rank_soft_dp_batched

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
    This function is fully batched and vectorised.
    """
    device = Z.device
    W_coords = witness_pool.coords.clone()
    M = W_coords.shape[0]
    rank = witness_pool.rank
    gamma = cfg.gamma
    
    for it in range(cfg.fixed_point_iters_per_temp):
        nbr_indices, nbr_dists_sq = grid_z.query_knn_grid(W_coords, m_local=cfg.m_active)
        Z_nbrs = Z[nbr_indices] # shape (M, m_active, 3)
        a_nbrs = a[nbr_indices] # shape (M, m_active)
        
        diff = Z_nbrs - W_coords[:, None, :]
        dist_sq = torch.sum(diff ** 2, dim=2)
        E = dist_sq + a_nbrs # (M, m_active)
        
        chunk_size = 5000
        for start in range(0, M, chunk_size):
            end = min(start + chunk_size, M)
            E_chunk = E[start:end]
            Z_nbrs_chunk = Z_nbrs[start:end]
            
            Q_val, b_val = compute_rank_soft_dp_batched(E_chunk, rank, cfg.rank_eps_schedule[-1])
            b_weights = b_val[:, rank - 1] # shape (chunk, m_active)
            
            # Barycenter T
            T = torch.sum(b_weights[:, :, None] * Z_nbrs_chunk, dim=1) # shape (chunk, 3)
            W_coords[start:end] = (1.0 - gamma) * W_coords[start:end] + gamma * T
            
    # Re-evaluate final scores and signatures in batch
    nbr_indices, nbr_dists_sq = grid_z.query_knn_grid(W_coords, m_local=cfg.m_active)
    Z_nbrs = Z[nbr_indices]
    a_nbrs = a[nbr_indices]
    diff = Z_nbrs - W_coords[:, None, :]
    dist_sq = torch.sum(diff ** 2, dim=2)
    E = dist_sq + a_nbrs
    
    scores = torch.zeros(M, device=device)
    signatures = torch.zeros((M, rank + 1), dtype=torch.int64, device=device)
    
    chunk_size = 5000
    for start in range(0, M, chunk_size):
        end = min(start + chunk_size, M)
        E_chunk = E[start:end]
        Z_nbrs_chunk = Z_nbrs[start:end]
        nbr_indices_chunk = nbr_indices[start:end]
        
        sorted_idx = torch.argsort(E_chunk, dim=1)
        row_indices = torch.arange(end - start, device=device).unsqueeze(1)
        signatures[start:end] = nbr_indices_chunk[row_indices, sorted_idx[:, :rank + 1]]
        
        Q_val, b_val = compute_rank_soft_dp_batched(E_chunk, rank, cfg.rank_eps_schedule[-1])
        b_weights = b_val[:, rank - 1]
        T = torch.sum(b_weights[:, :, None] * Z_nbrs_chunk, dim=1)
        dist_to_update = torch.sum((W_coords[start:end] - T) ** 2, dim=1)
        scores[start:end] = -dist_to_update
        
    return WitnessPool(W_coords, rank, scores, signatures)


def lift_witness_pool(witness_pool, Z, a, eta, grid_z, cfg):
    """
    Lifts witnesses of rank k to rank k+1.
    Fully vectorised seed generation and rank capacity filtering.
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
    nbr_indices, nbr_dists_sq = grid_z.query_knn_grid(W_coords, m_local=cfg.m_active)
    
    # Sort neighbor sites by energy
    diff = Z[nbr_indices] - W_coords[:, None, :]
    E = torch.sum(diff ** 2, dim=2) + a[nbr_indices]
    sorted_idx = torch.argsort(E, dim=1)
    
    row_indices = torch.arange(M, device=device).unsqueeze(1)
    top_k_indices = nbr_indices[row_indices, sorted_idx[:, :next_rank]] # (M, next_rank)
    top_k_coords = Z[top_k_indices] # (M, next_rank, 3)
    
    # Generator B: Shell lift (mean of top next_rank sites)
    shell_lift_seeds = torch.mean(top_k_coords, dim=1) # (M, 3)
    seeds.append(shell_lift_seeds)
    
    # Generator C: Support centers (mean of subsets of size 2, 3, 4)
    for size in range(2, min(5, next_rank + 1)):
        seeds.append(torch.mean(top_k_coords[:, :size, :], dim=1))
        
    all_seeds = torch.cat(seeds, dim=0)
    S_num = all_seeds.shape[0]
    
    # 3. Rank capacity filter (Generator D) - Fully Vectorised
    nbr_indices_s, nbr_dists_sq_s = grid_z.query_knn_grid(all_seeds, m_local=cfg.m_active)
    diff_s = Z[nbr_indices_s] - all_seeds[:, None, :]
    E_s = torch.sum(diff_s ** 2, dim=2) + a[nbr_indices_s] # (S_num, m_active)
    
    sorted_E, _ = torch.sort(E_s, dim=1)
    r_est = sorted_E[:, min(next_rank - 1, cfg.m_active - 1)].unsqueeze(1) # (S_num, 1)
    count = torch.sum(E_s <= r_est + 1e-3, dim=1) # (S_num,)
    
    valid_mask = torch.abs(count - next_rank) <= 2
    valid_idx = torch.where(valid_mask)[0]
    
    if len(valid_idx) == 0:
        # Deterministic fallback to first elements
        valid_idx = torch.arange(min(1000, S_num), device=device)
        
    filtered_seeds = all_seeds[valid_idx]
    
    # Limit to the budget_per_rank deterministically by sorting by distance to closest site
    if filtered_seeds.shape[0] > cfg.budget_per_rank:
        filtered_E = E_s[valid_idx]
        top_scores = filtered_E[:, 0] # energy to closest site
        top_idx = torch.argsort(top_scores)[:cfg.budget_per_rank]
        filtered_seeds = filtered_seeds[top_idx]
        
    return WitnessPool(filtered_seeds, next_rank)
