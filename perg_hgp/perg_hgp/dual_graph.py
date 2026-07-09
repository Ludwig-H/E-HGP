import torch
import numpy as np

def compute_facet_ids(cofaces, K):
    """
    Computes unique IDs for each K-facet of the cofaces.
    Runs 100% exact row-wise deduplication on CPU using NumPy, guaranteeing zero collision risk.
    """
    device = cofaces.device
    U = cofaces.shape[0]
    
    # 1. Generate all K-facets for all cofaces
    facets_list = []
    for r in range(K + 1):
        mask = torch.ones(K + 1, dtype=torch.bool, device=device)
        mask[r] = False
        facet = cofaces[:, mask]
        facets_list.append(facet)
        
    facets_all = torch.cat(facets_list, dim=0) # shape ((K+1)*U, K)
    
    # 2. Sort indices of each facet for canonical representation
    facets_all_sorted, _ = torch.sort(facets_all, dim=1)
    
    # 3. Direct exact row-wise deduplication on CPU
    facets_all_sorted_cpu = facets_all_sorted.cpu().numpy()
    unique_facets_cpu, unique_idx_cpu, inverse_indices_cpu = np.unique(
        facets_all_sorted_cpu, axis=0, return_index=True, return_inverse=True
    )
    
    unique_idx = torch.from_numpy(unique_idx_cpu).to(device)
    inverse_indices = torch.from_numpy(inverse_indices_cpu).to(device)
    
    unique_facets = facets_all_sorted[unique_idx]
    facet_ids = inverse_indices.view(K + 1, U).t()
    
    return facet_ids, unique_facets


def build_dual_edges(cofaces, facet_ids, weights):
    """
    Builds the dual graph edges using star expansion.
    """
    U = cofaces.shape[0]
    K = cofaces.shape[1] - 1
    device = cofaces.device
    
    edge_u = torch.zeros((U, K), dtype=torch.int64, device=device)
    edge_v = torch.zeros((U, K), dtype=torch.int64, device=device)
    edge_w = torch.zeros((U, K), dtype=torch.float32, device=device)
    edge_coface = torch.zeros((U, K), dtype=torch.int64, device=device)
    
    for j in range(1, K + 1):
        edge_u[:, j - 1] = facet_ids[:, 0]
        edge_v[:, j - 1] = facet_ids[:, j]
        edge_w[:, j - 1] = weights
        edge_coface[:, j - 1] = torch.arange(U, device=device)
        
    return edge_u.flatten(), edge_v.flatten(), edge_w.flatten(), edge_coface.flatten()
