import numpy as np

def gabriel_global_test_batch(centers, radii_sq, Z, a, cofaces, tol=1e-8):
    """
    Performs a global Gabriel certificate test on a batch of cofaces.
    Optimized for GPU/TPU/CPU using GEMM.
    
    Parameters
    ----------
    centers : np.ndarray
        Miniball centers of the cofaces, shape (B, p).
    radii_sq : np.ndarray
        Miniball radii squared, shape (B,).
    Z : np.ndarray
        Regularized site centers, shape (n, p).
    a : np.ndarray
        Power weights, shape (n,).
    cofaces : np.ndarray
        Batch of cofaces, shape (B, K+1).
    tol : float
        Gabriel gap tolerance.
        
    Returns
    -------
    is_gabriel : np.ndarray
        Boolean array of shape (B,) indicating which cofaces are globally Gabriel.
    gaps : np.ndarray
        Gabriel gap values for each coface, shape (B,).
    """
    B, K_plus_1 = cofaces.shape
    n, p = Z.shape
    
    # 1. Compute norms
    norm_C = np.sum(centers ** 2, axis=1) # (B,)
    norm_Z_a = np.sum(Z ** 2, axis=1) + a # (n,)
    
    # 2. Compute power distance matrix using GEMM
    # dist_matrix[b, j] = ||c_b - z_j||^2 + a_j
    dist_matrix = norm_C[:, np.newaxis] + norm_Z_a[np.newaxis, :] - 2.0 * np.dot(centers, Z.T)
    
    # 3. Mask out the points that belong to the coface itself
    # row_indices: [0, 0, ..., B-1, B-1] (each row repeated K+1 times)
    # col_indices: cofaces index values
    row_indices = np.repeat(np.arange(B), K_plus_1)
    col_indices = cofaces.ravel()
    
    # Set distance of coface elements to infinity so they are excluded from the minimum
    dist_matrix[row_indices, col_indices] = np.inf
    
    # 4. Find the minimum power distance for each coface
    min_dist = np.min(dist_matrix, axis=1)
    
    # 5. Compute Gabriel gap: min_{j notin sigma} phi_j(c_b) - rho^2
    gaps = min_dist - radii_sq
    
    is_gabriel = gaps >= -tol
    
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
