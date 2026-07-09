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
        self.signatures = signatures if signatures is not None else torch.zeros((coords.shape[0], rank + 1), dtype=torch.int64, device=coords.device)


def refine_witness_pool(witness_pool, Z, a, grid_z, cfg):
    """
    Runs fixed-point iterations on witness coordinates to find critical witnesses of rank k.
    This function is strictly chunked to bound VRAM and RAM memory usage under 1.5 GB.
    """
    device = Z.device
    W_coords = witness_pool.coords.clone()
    M = W_coords.shape[0]
    rank = witness_pool.rank
    gamma = cfg.gamma

    chunk_size = 50000 # Strictly bounds maximum VRAM allocations

    for it in range(cfg.fixed_point_iters_per_temp):
        for start in range(0, M, chunk_size):
            end = min(start + chunk_size, M)
            W_chunk = W_coords[start:end]

            # Local grid KNN query and neighborhood energy computation
            nbr_indices_chunk, _ = grid_z.query_knn_grid(W_chunk, m_local=cfg.m_active)
            Z_nbrs_chunk = Z[nbr_indices_chunk] # (chunk_size, m_active, 3)
            a_nbrs_chunk = a[nbr_indices_chunk] # (chunk_size, m_active)

            diff = Z_nbrs_chunk - W_chunk[:, None, :]
            dist_sq = torch.sum(diff ** 2, dim=2)
            E_chunk = dist_sq + a_nbrs_chunk # (chunk_size, m_active)

            Q_val, b_val = compute_rank_soft_dp_batched(E_chunk, rank, cfg.rank_eps_schedule[-1])
            b_weights = b_val[:, rank - 1] # (chunk_size, m_active)

            # Barycenter update
            T = torch.sum(b_weights[:, :, None] * Z_nbrs_chunk, dim=1) # (chunk_size, 3)
            W_coords[start:end] = (1.0 - gamma) * W_chunk + gamma * T

    # Final evaluation of scores and signatures in a strictly chunked manner
    scores = torch.zeros(M, device=device)
    signatures = torch.zeros((M, rank + 1), dtype=torch.int64, device=device)

    for start in range(0, M, chunk_size):
        end = min(start + chunk_size, M)
        W_chunk = W_coords[start:end]

        nbr_indices_chunk, _ = grid_z.query_knn_grid(W_chunk, m_local=cfg.m_active)
        Z_nbrs_chunk = Z[nbr_indices_chunk]
        a_nbrs_chunk = a[nbr_indices_chunk]

        diff = Z_nbrs_chunk - W_chunk[:, None, :]
        dist_sq = torch.sum(diff ** 2, dim=2)
        E_chunk = dist_sq + a_nbrs_chunk

        sorted_idx = torch.argsort(E_chunk, dim=1)
        row_indices = torch.arange(end - start, device=device).unsqueeze(1)
        signatures[start:end] = nbr_indices_chunk[row_indices, sorted_idx[:, :rank + 1]]

        Q_val, b_val = compute_rank_soft_dp_batched(E_chunk, rank, cfg.rank_eps_schedule[-1])
        b_weights = b_val[:, rank - 1]
        T = torch.sum(b_weights[:, :, None] * Z_nbrs_chunk, dim=1)
        dist_to_update = torch.sum((W_chunk - T) ** 2, dim=1)
        scores[start:end] = -dist_to_update

    return WitnessPool(W_coords, rank, scores, signatures)


def lift_witness_pool(witness_pool, Z, a, eta, grid_z, cfg):
    """
    Lifts witnesses of rank k to rank k+1.
    Strictly chunked to bound VRAM/RAM allocations.
    """
    device = Z.device
    W_coords = witness_pool.coords
    M = W_coords.shape[0]
    rank = witness_pool.rank
    next_rank = rank + 1

    chunk_size = 50000
    valid_seeds_list = []
    valid_E_closest_list = []

    for start in range(0, M, chunk_size):
        end = min(start + chunk_size, M)
        W_chunk = W_coords[start:end]
        M_c = W_chunk.shape[0]

        # Generate seed candidates for this chunk
        seeds_chunk = []
        seeds_chunk.append(W_chunk.clone()) # Generator A (Continuation)

        # Query local neighborhood
        nbr_indices_chunk, _ = grid_z.query_knn_grid(W_chunk, m_local=cfg.m_active)
        diff_chunk = Z[nbr_indices_chunk] - W_chunk[:, None, :]
        E_chunk = torch.sum(diff_chunk ** 2, dim=2) + a[nbr_indices_chunk]
        sorted_idx = torch.argsort(E_chunk, dim=1)

        row_indices = torch.arange(M_c, device=device).unsqueeze(1)
        top_k_indices = nbr_indices_chunk[row_indices, sorted_idx[:, :next_rank]]
        top_k_coords = Z[top_k_indices]

        # Generator B: Shell lift
        shell_lift = torch.mean(top_k_coords, dim=1)
        seeds_chunk.append(shell_lift)

        # Generator C: Support centers of size 2, 3, 4
        for size in range(2, min(5, next_rank + 1)):
            seeds_chunk.append(torch.mean(top_k_coords[:, :size, :], dim=1))

        all_seeds_chunk = torch.cat(seeds_chunk, dim=0)
        S_num_c = all_seeds_chunk.shape[0]

        # Generator D: Rank capacity filter on the chunk's seeds in smaller sub-chunks
        filtered_seeds_chunk_list = []
        filtered_E_chunk_list = []

        sub_chunk_size = 50000
        for s_start in range(0, S_num_c, sub_chunk_size):
            s_end = min(s_start + sub_chunk_size, S_num_c)
            sub_seeds = all_seeds_chunk[s_start:s_end]

            nbr_indices_s, _ = grid_z.query_knn_grid(sub_seeds, m_local=cfg.m_active)
            diff_s = Z[nbr_indices_s] - sub_seeds[:, None, :]
            E_s = torch.sum(diff_s ** 2, dim=2) + a[nbr_indices_s]

            sorted_E, _ = torch.sort(E_s, dim=1)
            r_est = sorted_E[:, min(next_rank - 1, cfg.m_active - 1)].unsqueeze(1)
            count = torch.sum(E_s <= r_est + 1e-3, dim=1)

            valid_mask = torch.abs(count - next_rank) <= 2
            valid_idx = torch.where(valid_mask)[0]

            if len(valid_idx) == 0:
                valid_idx = torch.arange(min(1000, sub_seeds.shape[0]), device=device)

            filtered_seeds_chunk_list.append(sub_seeds[valid_idx])
            filtered_E_chunk_list.append(E_s[valid_idx])

        filtered_seeds_chunk = torch.cat(filtered_seeds_chunk_list, dim=0)
        filtered_E_chunk = torch.cat(filtered_E_chunk_list, dim=0)

        valid_seeds_list.append(filtered_seeds_chunk)
        valid_E_closest_list.append(filtered_E_chunk[:, 0])

    # Concatenate filtered seeds from all chunks
    if valid_seeds_list:
        all_valid_seeds = torch.cat(valid_seeds_list, dim=0)
        all_valid_E = torch.cat(valid_E_closest_list, dim=0)
    else:
        all_valid_seeds = torch.empty((0, 3), device=device)
        all_valid_E = torch.empty((0,), device=device)

    # Limit to the budget_per_rank deterministically by energy to the closest site
    if all_valid_seeds.shape[0] > cfg.budget_per_rank:
        top_idx = torch.argsort(all_valid_E)[:cfg.budget_per_rank]
        all_valid_seeds = all_valid_seeds[top_idx]

    return WitnessPool(all_valid_seeds, next_rank)
