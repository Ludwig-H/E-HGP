import torch

def compute_facet_ids(cofaces, K):
    """
    Computes unique IDs for each K-facet of the cofaces.
    cofaces: shape (U, K+1)
    Returns:
        facet_ids: shape (U, K+1) of unique face IDs
        unique_facets: shape (NumFacets, K) of unique facet indices
    """
    device = cofaces.device
    U = cofaces.shape[0]
    
    # 1. Generate all K-facets for all cofaces
    # For each coface, there are K+1 facets (obtained by dropping one index)
    facets_list = []
    for r in range(K + 1):
        # Drop position r
        mask = torch.ones(K + 1, dtype=torch.bool, device=device)
        mask[r] = False
        facet = cofaces[:, mask] # shape (U, K)
        facets_list.append(facet)
        
    facets_all = torch.cat(facets_list, dim=0) # shape ((K+1)*U, K)
    
    # 2. Sort indices of each facet to ensure canonical representation
    facets_all_sorted, _ = torch.sort(facets_all, dim=1)
    
    # 3. Deduplicate facets using unique
    unique_facets, inverse_indices = torch.unique(facets_all_sorted, dim=0, return_inverse=True)
    
    # 4. Reshape inverse_indices back to (K+1, U) and transpose to (U, K+1)
    facet_ids = inverse_indices.view(K + 1, U).t()
    
    return facet_ids, unique_facets


def build_dual_edges(cofaces, facet_ids, weights):
    """
    Builds the dual graph edges.
    For each coface, we add edges between its K-facets.
    Using star expansion: choose facet 0 as pivot, and add edges to other facets 1..K.
    This creates K edges per coface.
    Returns:
        edge_u: (U * K,)
        edge_v: (U * K,)
        edge_w: (U * K,) weight of the coface (rho)
        edge_coface: (U * K,) coface index
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
