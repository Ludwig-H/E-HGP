import numpy as np

try:
    import torch
    HAS_TORCH = True
except ImportError:
    HAS_TORCH = False

def gabriel_global_test_pytorch(centers, radii_sq, Z, a, cofaces, tol=1e-8):
    B, K_plus_1 = cofaces.shape
    n, p = Z.shape
    
    # Target memory footprint: ~100MB max per chunk
    chunk_size = max(1, 25000000 // n)
    
    is_gabriel = torch.empty(B, dtype=torch.bool, device=centers.device)
    gaps = torch.empty(B, dtype=centers.dtype, device=centers.device)
    
    norm_Z_a = torch.sum(Z ** 2, dim=1) + a
    
    for start in range(0, B, chunk_size):
        end = min(start + chunk_size, B)
        c_chunk = centers[start:end]
        r_chunk = radii_sq[start:end]
        cof_chunk = cofaces[start:end]
        B_c = end - start
        
        norm_C = torch.sum(c_chunk ** 2, dim=1)
        dist_matrix = norm_C[:, None] + norm_Z_a[None, :] - 2.0 * torch.matmul(c_chunk, Z.t())
        
        row_indices = torch.repeat_interleave(torch.arange(B_c, device=cofaces.device), K_plus_1)
        col_indices = cof_chunk.ravel()
        dist_matrix[row_indices, col_indices] = float('inf')
        
        min_dist = torch.min(dist_matrix, dim=1).values
        g_chunk = min_dist - r_chunk
        
        is_gabriel[start:end] = g_chunk >= -tol
        gaps[start:end] = g_chunk
        
    return is_gabriel, gaps

def gabriel_global_test_batch(centers, radii_sq, Z, a, cofaces, tol=1e-8):
    """
    Performs a global Gabriel certificate test on a batch of cofaces.
    Optimized for GPU/TPU/CPU using GEMM with dynamic chunking for scalability.
    """
    if HAS_TORCH and torch.cuda.is_available():
        centers_t = torch.as_tensor(centers, device='cuda')
        radii_sq_t = torch.as_tensor(radii_sq, device='cuda')
        Z_t = torch.as_tensor(Z, device='cuda')
        a_t = torch.as_tensor(a, device='cuda')
        cofaces_t = torch.as_tensor(cofaces, device='cuda', dtype=torch.long)
        
        is_gabriel_t, gaps_t = gabriel_global_test_pytorch(centers_t, radii_sq_t, Z_t, a_t, cofaces_t, tol)
        return is_gabriel_t.cpu().numpy(), gaps_t.cpu().numpy()

    B, K_plus_1 = cofaces.shape
    n, p = Z.shape
    
    # Target memory footprint: ~100MB max per chunk
    chunk_size = max(1, 25000000 // n)
    
    is_gabriel = np.empty(B, dtype=bool)
    gaps = np.empty(B, dtype=centers.dtype)
    
    norm_Z_a = np.sum(Z ** 2, axis=1) + a
    
    for start in range(0, B, chunk_size):
        end = min(start + chunk_size, B)
        c_chunk = centers[start:end]
        r_chunk = radii_sq[start:end]
        cof_chunk = cofaces[start:end]
        B_c = end - start
        
        norm_C = np.sum(c_chunk ** 2, axis=1)
        dist_matrix = norm_C[:, np.newaxis] + norm_Z_a[np.newaxis, :] - 2.0 * np.dot(c_chunk, Z.T)
        
        row_indices = np.repeat(np.arange(B_c), K_plus_1)
        col_indices = cof_chunk.ravel()
        dist_matrix[row_indices, col_indices] = np.inf
        
        min_dist = np.min(dist_matrix, axis=1)
        g_chunk = min_dist - r_chunk
        
        is_gabriel[start:end] = g_chunk >= -tol
        gaps[start:end] = g_chunk
        
    return is_gabriel, gaps


def gabriel_local_filter_batch(centers, radii_sq, Z, a, cofaces, local_supports, tol=1e-8):
    """
    A local pre-filter to quickly reject cofaces that are clearly not Gabriel
    by checking only the local support of the coface vertices.
    
    Parameters
    ----------
    centers : np.ndarray
        Coface centers, shape (B, p).
    radii_sq : np.ndarray
        Coface radii squared, shape (B,).
    Z : np.ndarray
        Site centers, shape (n, p).
    a : np.ndarray
        Power weights, shape (n,).
    cofaces : np.ndarray
        Batch of cofaces, shape (B, K+1).
    local_supports : np.ndarray
        The nearest neighbor indices for each site, shape (n, m_reg).
    tol : float
        Tolerance.
        
    Returns
    -------
    passes_local : np.ndarray
        Boolean array of shape (B,) indicating which cofaces passed the local test.
    """
    B, K_plus_1 = cofaces.shape
    
    # Extract the union of local supports of all vertices in each coface.
    # For a coface, the union of local supports contains at most (K+1) * m_reg indices.
    # To keep it vectorized, we can compute the distances to all union neighbors.
    passes_local = np.ones(B, dtype=bool)
    
    for b in range(B):
        cof = cofaces[b]
        c = centers[b]
        r2 = radii_sq[b]
        
        # Get local support union of all vertices in the coface
        union_support = np.unique(local_supports[cof])
        
        # Exclude vertices of the coface itself
        check_set = np.setdiff1d(union_support, cof, assume_unique=True)
        
        if len(check_set) == 0:
            continue
            
        # Compute power distances to check_set
        diff = Z[check_set] - c
        phi = np.sum(diff ** 2, axis=1) + a[check_set]
        
        if np.any(phi < r2 - tol):
            passes_local[b] = False
            
    return passes_local
