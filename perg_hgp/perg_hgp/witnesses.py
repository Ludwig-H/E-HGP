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

            # Local grid power search query and neighborhood energy computation
            nbr_indices_chunk, E_chunk = grid_z.query_power_grid(W_chunk, a, m_local=cfg.m_active)
            Z_nbrs_chunk = Z[nbr_indices_chunk] # (chunk_size, m_active, 3)

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

        nbr_indices_chunk, E_chunk = grid_z.query_power_grid(W_chunk, a, m_local=cfg.m_active)
        Z_nbrs_chunk = Z[nbr_indices_chunk]

        signatures[start:end] = nbr_indices_chunk[:, :rank + 1]

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

    # Running top-budget_per_rank witnesses to strictly bound memory at all times
    running_seeds = torch.empty((0, 3), dtype=torch.float32, device=device)
    running_E = torch.empty((0,), dtype=torch.float32, device=device)

    for start in range(0, M, chunk_size):
        end = min(start + chunk_size, M)
        W_chunk = W_coords[start:end]
        M_c = W_chunk.shape[0]

        # Generate seed candidates for this chunk
        seeds_chunk = []
        seeds_chunk.append(W_chunk.clone()) # Generator A (Continuation)

        # Query local neighborhood using power distance
        nbr_indices_chunk, _ = grid_z.query_power_grid(W_chunk, a, m_local=cfg.m_active)
        top_k_indices = nbr_indices_chunk[:, :next_rank]
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

            # Query power distance on sub_seeds (sorted_E is exactly E_s because query_power_grid returns sorted distances)
            _, sorted_E = grid_z.query_power_grid(sub_seeds, a, m_local=cfg.m_active)

            r_est = sorted_E[:, min(next_rank - 1, cfg.m_active - 1)].unsqueeze(1)
            count = torch.sum(sorted_E <= r_est + 1e-3, dim=1)

            valid_mask = torch.abs(count - next_rank) <= 2
            valid_idx = torch.where(valid_mask)[0]

            if len(valid_idx) == 0:
                valid_idx = torch.arange(min(1000, sub_seeds.shape[0]), device=device)

            filtered_seeds_chunk_list.append(sub_seeds[valid_idx])
            filtered_E_chunk_list.append(sorted_E[valid_idx][:, 0])

        filtered_seeds_chunk = torch.cat(filtered_seeds_chunk_list, dim=0)
        filtered_E_chunk = torch.cat(filtered_E_chunk_list, dim=0)

        # Merge with running top-budget_per_rank witnesses
        combined_seeds = torch.cat([running_seeds, filtered_seeds_chunk], dim=0)
        combined_E = torch.cat([running_E, filtered_E_chunk], dim=0)

        if combined_seeds.shape[0] > cfg.budget_per_rank:
            val, idx = torch.topk(combined_E, cfg.budget_per_rank, largest=False)
            running_seeds = combined_seeds[idx]
            running_E = val
        else:
            running_seeds = combined_seeds
            running_E = combined_E

    return WitnessPool(running_seeds, next_rank)
