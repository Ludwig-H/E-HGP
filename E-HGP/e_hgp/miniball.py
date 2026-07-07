import numpy as np

try:
    from ._core import weighted_miniball_active_set_batch_cython
    HAS_CYTHON = True
except ImportError:
    HAS_CYTHON = False
def solve_linear_system_batch(G, h):
    """
    Solves a batch of linear systems G x = h.
    G: shape (B, d, d), h: shape (B, d).
    Returns x of shape (B, d).
    """
    # Reshape h to (B, d, 1) to make the batch dimension alignment explicit
    h_3d = h[:, :, np.newaxis]
    try:
        sol = np.linalg.solve(G, h_3d)
        return sol.squeeze(axis=2)
    except np.linalg.LinAlgError:
        # Fallback for singular or near-singular matrices using pseudo-inverse
        # Pinv shape is (B, d, d)
        pinv = np.linalg.pinv(G)
        return np.einsum('bki,bi->bk', pinv, h)


def miniball_support_enum_batch(Z, a, cofaces, tol=1e-6):
    """
    Batch weighted miniball solver using support enumeration.
    Recommended for K <= 5.
    
    Parameters
    ----------
    Z : np.ndarray
        Regularized centers, shape (n, p).
    a : np.ndarray
        Power weights, shape (n,).
    cofaces : np.ndarray
        Batch of cofaces, shape (B, K+1).
    tol : float
        Numerical tolerance.
        
    Returns
    -------
    centers : np.ndarray
        Miniball centers, shape (B, p).
    radii_sq : np.ndarray
        Miniball radii squared, shape (B,).
    """
    B, K_plus_1 = cofaces.shape
    p = Z.shape[1]
    
    # We want to find the optimal support subset for each coface in the batch.
    # Pre-generate all non-empty subsets of indices {0, ..., K_plus_1 - 1}
    # To keep it ordered by size, we generate subsets of size 1, then 2, etc.
    subsets = []
    for size in range(1, K_plus_1 + 1):
        # Generate all combinations of given size
        import itertools
        for combo in itertools.combinations(range(K_plus_1), size):
            subsets.append(list(combo))
            
    # For each coface, we track the best valid support (the one with the largest radius squared)
    best_centers = np.zeros((B, p), dtype=Z.dtype)
    best_radii_sq = np.full(B, -1.0, dtype=Z.dtype)
    
    # Pre-extract all coordinates and weights for the cofaces to avoid repeated indexing
    # coface_Z: shape (B, K+1, p)
    # coface_a: shape (B, K+1)
    coface_Z = Z[cofaces]
    coface_a = a[cofaces]
    
    for sub in subsets:
        sub_size = len(sub)
        
        # Extract subset elements
        # sub_Z: (B, sub_size, p)
        # sub_a: (B, sub_size)
        sub_Z = coface_Z[:, sub, :]
        sub_a = coface_a[:, sub]
        
        if sub_size == 1:
            # Single vertex support
            centers = sub_Z[:, 0, :]
            radii_sq = sub_a[:, 0]
            lambdas = np.ones((B, 1), dtype=Z.dtype)
            
        else:
            # Anchor is vertex 0
            # v: shape (B, sub_size-1, p)
            v = sub_Z[:, 1:, :] - sub_Z[:, 0:1, :]
            
            # Gram matrix G: shape (B, sub_size-1, sub_size-1)
            G = np.einsum('bki,bli->bkl', v, v)
            
            # Right-hand side h: shape (B, sub_size-1)
            # h_u = 0.5 * (||v_u||^2 + a_u - a_0)
            v_norm_sq = np.sum(v ** 2, axis=2)
            h = 0.5 * (v_norm_sq + sub_a[:, 1:] - sub_a[:, 0:1])
            
            # Solve G alpha = h
            # alpha: shape (B, sub_size-1)
            alpha = solve_linear_system_batch(G, h)
            
            # Center: c_S = z_0 + sum_u alpha_u v_u
            centers = sub_Z[:, 0, :] + np.einsum('bk,bki->bi', alpha, v)
            
            # Radius squared: t_S = a_0 + alpha^T G alpha
            alpha_G_alpha = np.einsum('bk,bk->b', alpha, np.einsum('bkl,bl->bk', G, alpha))
            radii_sq = sub_a[:, 0] + alpha_G_alpha
            
            # Barycentric coordinates
            lambdas = np.zeros((B, sub_size), dtype=Z.dtype)
            lambdas[:, 1:] = alpha
            lambdas[:, 0] = 1.0 - np.sum(alpha, axis=1)
            
        # Check validity:
        # 1. lambdas >= -tol
        valid_lambda = np.all(lambdas >= -tol, axis=1)
        
        # 2. All points in the coface are inside the ball:
        # phi_i(center) <= radius_sq + tol
        # Compute distances from all coface vertices to center
        # coface_Z: (B, K+1, p)
        # centers[:, np.newaxis, :]: (B, 1, p)
        diff = coface_Z - centers[:, np.newaxis, :]
        dist_sq = np.sum(diff ** 2, axis=2) # (B, K+1)
        phi = dist_sq + coface_a # (B, K+1)
        
        valid_containment = np.all(phi <= radii_sq[:, np.newaxis] + tol, axis=1)
        
        valid = valid_lambda & valid_containment
        
        # Update best support: if valid and radius is larger than current best
        update = valid & (radii_sq > best_radii_sq)
        best_centers[update] = centers[update]
        best_radii_sq[update] = radii_sq[update]
        
    return best_centers, best_radii_sq


def weighted_miniball_active_set_single(Z_coface, a_coface, tol=1e-8, max_iter=100):
    """
    Active-set solver for a single coface.
    Z_coface: shape (K+1, p)
    a_coface: shape (K+1,)
    """
    K_plus_1, p = Z_coface.shape
    
    # Active set S stores local indices in {0, ..., K}
    S = [0]
    
    for _ in range(max_iter):
        sub_size = len(S)
        sub_Z = Z_coface[S]
        sub_a = a_coface[S]
        
        # Solve for current active set
        if sub_size == 1:
            center = sub_Z[0]
            radius_sq = sub_a[0]
            lambdas = np.array([1.0])
        else:
            # Solve system G alpha = h
            v = sub_Z[1:] - sub_Z[0]
            G = np.dot(v, v.T)
            h = 0.5 * (np.sum(v**2, axis=1) + sub_a[1:] - sub_a[0])
            try:
                alpha = np.linalg.solve(G, h)
            except np.linalg.LinAlgError:
                alpha = np.dot(np.linalg.pinv(G), h)
                
            center = sub_Z[0] + np.dot(alpha, v)
            radius_sq = sub_a[0] + np.dot(alpha, np.dot(G, alpha))
            
            lambdas = np.zeros(sub_size)
            lambdas[1:] = alpha
            lambdas[0] = 1.0 - np.sum(alpha)
            
        # Check if any active point has negative weight
        if np.any(lambdas < -tol):
            # Remove the point with the most negative weight (excluding the anchor if we want to be simple,
            # or just drop it and pick a new anchor)
            neg_idx = np.argmin(lambdas)
            S.pop(neg_idx)
            if not S:
                S = [0]
            continue
            
        # Check containment for all points in coface
        diff = Z_coface - center
        phi = np.sum(diff**2, axis=1) + a_coface
        
        # Find the point that violates the boundary condition the most
        violations = phi - radius_sq
        max_viol_idx = np.argmax(violations)
        max_viol = violations[max_viol_idx]
        
        if max_viol <= tol:
            # Optimal miniball found!
            return center, radius_sq
            
        # Otherwise, add violating point to active set
        if max_viol_idx not in S:
            S.append(max_viol_idx)
        else:
            # Numerically stuck: return current
            return center, radius_sq
            
    return center, radius_sq


def miniball_active_set_batch(Z, a, cofaces, tol=1e-6):
    """
    Batch weighted miniball solver using active set method.
    Works for any K.
    """
    if HAS_CYTHON:
        # Explicitly cast to the types expected by the Cython extension
        Z_64 = np.ascontiguousarray(Z, dtype=np.float64)
        a_64 = np.ascontiguousarray(a, dtype=np.float64)
        cof_32 = np.ascontiguousarray(cofaces, dtype=np.int32)
        return weighted_miniball_active_set_batch_cython(Z_64, a_64, cof_32, tol=tol)
        
    B, K_plus_1 = cofaces.shape
    p = Z.shape[1]
    
    centers = np.zeros((B, p), dtype=Z.dtype)
    radii_sq = np.zeros(B, dtype=Z.dtype)
    
    # We solve sequentially for each coface in the batch, which is fine if B is not too huge,
    # or if we want exact active-set results.
    # Note: For GPU execution, this would be written as a parallel active-set kernel.
    for b in range(B):
        cof_idx = cofaces[b]
        Z_cof = Z[cof_idx]
        a_cof = a[cof_idx]
        c, r2 = weighted_miniball_active_set_single(Z_cof, a_cof, tol=tol)
        centers[b] = c
        radii_sq[b] = r2
        
    return centers, radii_sq


def compute_weighted_miniball_batch(Z, a, cofaces, tol=1e-6):
    """
    Wrapper that selects the appropriate solver based on K.
    """
    K = cofaces.shape[1] - 1
    if K <= 5:
        return miniball_support_enum_batch(Z, a, cofaces, tol=tol)
    else:
        return miniball_active_set_batch(Z, a, cofaces, tol=tol)
