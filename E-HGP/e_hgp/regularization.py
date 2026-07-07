import numpy as np
from sklearn.neighbors import NearestNeighbors

try:
    import torch
    HAS_TORCH = True
except ImportError:
    HAS_TORCH = False

def solve_gibbs_entropy_pytorch(D, log_kappa, max_iter=40, tol=1e-5):
    n, m = D.shape
    log_m = np.log(m)
    effective_target = min(log_kappa, log_m - 1e-6)
    
    if effective_target <= 1e-9:
        Q = torch.zeros_like(D)
        Q[:, 0] = 1.0
        return Q, torch.zeros(n, dtype=D.dtype, device=D.device)
        
    low = torch.full((n,), 1e-8, dtype=D.dtype, device=D.device)
    high = 100.0 * torch.max(D, dim=1).values + 1e-5
    
    for _ in range(max_iter):
        mid = 0.5 * (low + high)
        exponent = -D / mid[:, None]
        max_exp = torch.max(exponent, dim=1, keepdim=True).values
        exp_w = torch.exp(exponent - max_exp)
        sum_exp = torch.sum(exp_w, dim=1, keepdim=True)
        q = exp_w / sum_exp
        
        log_q = exponent - max_exp - torch.log(sum_exp)
        entropy = -torch.sum(q * log_q, dim=1)
        
        too_small = entropy < effective_target
        low = torch.where(too_small, mid, low)
        high = torch.where(too_small, high, mid)
        
    eta = 0.5 * (low + high)
    exponent = -D / eta[:, None]
    max_exp = torch.max(exponent, dim=1, keepdim=True).values
    exp_w = torch.exp(exponent - max_exp)
    Q = exp_w / torch.sum(exp_w, dim=1, keepdim=True)
    
    if log_kappa >= log_m:
        Q.fill_(1.0 / m)
        eta.fill_(float('inf'))
        
    return Q, eta

def solve_gibbs_entropy_vectorized(D, log_kappa, max_iter=40, tol=1e-5):
    """
    Vectorized solver for Gibbs weights under local entropy constraints.
    """
    n, m = D.shape
    
    if HAS_TORCH and torch.cuda.is_available():
        D_t = torch.as_tensor(D, device='cuda')
        Q_t, eta_t = solve_gibbs_entropy_pytorch(D_t, log_kappa, max_iter, tol)
        return Q_t.cpu().numpy(), eta_t.cpu().numpy()
        
    Q = np.zeros_like(D)
    eta = np.zeros(n, dtype=D.dtype)
    
    # Target entropy cannot exceed log(m)
    log_m = np.log(m)
    effective_target = min(log_kappa, log_m - 1e-6)
    
    if effective_target <= 1e-9:
        # kappa = 1 case: delta distribution
        Q[:, 0] = 1.0
        return Q, eta
        
    # Bisection search bounds
    # Low temperature limit: entropy -> 0
    # High temperature limit: entropy -> log(m)
    low = np.full(n, 1e-8, dtype=D.dtype)
    # Estimate a reasonable upper bound based on maximum distance
    high = 100.0 * np.max(D, axis=1) + 1e-5
    
    # We run fixed number of iterations for vectorization efficiency
    for _ in range(max_iter):
        mid = 0.5 * (low + high)
        
        # Compute Gibbs weights safely with log-sum-exp trick
        # exponent: -D / mid
        # To avoid overflow, we subtract max along axis 1
        exponent = -D / mid[:, np.newaxis]
        max_exp = np.max(exponent, axis=1, keepdims=True)
        exp_w = np.exp(exponent - max_exp)
        sum_exp = np.sum(exp_w, axis=1, keepdims=True)
        q = exp_w / sum_exp
        
        # Compute entropy: H(q) = - sum(q * log(q))
        # Safely compute q * log(q) using log(q) = exponent - max_exp - log(sum_exp)
        log_q = exponent - max_exp - np.log(sum_exp)
        entropy = -np.sum(q * log_q, axis=1)
        
        # Bisection update
        too_small = entropy < effective_target
        low = np.where(too_small, mid, low)
        high = np.where(too_small, high, mid)
        
    # Final weights computation
    eta = 0.5 * (low + high)
    exponent = -D / eta[:, np.newaxis]
    max_exp = np.max(exponent, axis=1, keepdims=True)
    exp_w = np.exp(exponent - max_exp)
    Q = exp_w / np.sum(exp_w, axis=1, keepdims=True)
    
    # For rows where log_kappa exceeds log(m), set to uniform weights
    if log_kappa >= log_m:
        Q[:] = 1.0 / m
        eta[:] = np.inf
        
    return Q, eta


def regularize_entropy_local(X, m_reg, kappa, regularization='entropy_local', eps_sinkhorn=0.1):
    """
    Computes entropy-regularized power sites (Z, a) from data points X.
    
    Parameters
    ----------
    X : np.ndarray
        Input data points of shape (n, p).
    m_reg : int
        Size of local neighborhood support.
    kappa : float
        Entropy constraint parameter (kappa >= 1).
    regularization : {'entropy_local', 'sinkhorn_sparse'}
        Type of regularization.
    eps_sinkhorn : float
        Regularization strength for Sinkhorn.
        
    Returns
    -------
    Z : np.ndarray
        Regularized site centers of shape (n, p).
    a : np.ndarray
        Power weights (local variances) of shape (n,).
    support_indices : np.ndarray
        Indices of support points of shape (n, m_reg).
    """
    n, p = X.shape
    m_reg = min(m_reg, n)
    
    # 1. Compute Nearest Neighbors to form local support
    nn = NearestNeighbors(n_neighbors=m_reg, metric='euclidean')
    nn.fit(X)
    distances, indices = nn.kneighbors(X)
    D = distances ** 2 # Squared distances
    
    if regularization == 'entropy_local':
        log_kappa = np.log(kappa)
        Q, eta = solve_gibbs_entropy_vectorized(D, log_kappa)
        
    elif regularization == 'sinkhorn_sparse':
        # Sparse Sinkhorn regularization over neighbors
        # We solve a local Sinkhorn problem for each row using the cost D
        # cost: D of shape (n, m_reg)
        # target marginals: r = 1 (each site has total weight 1)
        # we want to regularize with eps_sinkhorn
        # P_ij = u_i * v_j * exp(-D_ij / eps_sinkhorn)
        # For simplicity, if we regularize independently per row, it's equivalent
        # to Gibbs. But if we couple across the dataset (so we don't have too many hubs),
        # we run Sinkhorn over the sparse neighborhood graph.
        # Let's run a global sparse Sinkhorn on the n x n sparse matrix.
        # Construct the sparse cost matrix
        from scipy.sparse import csr_matrix
        
        row_indices = np.repeat(np.arange(n), m_reg)
        col_indices = indices.ravel()
        data_d = D.ravel()
        
        # Exponential cost matrix K = exp(-D / eps_sinkhorn)
        data_k = np.exp(-data_d / eps_sinkhorn)
        K = csr_matrix((data_k, (row_indices, col_indices)), shape=(n, n))
        
        # Sinkhorn iterations: u = 1 / (K v), v = 1 / (K^T u)
        # We want row sums to be 1, and column sums to be <= c (e.g. c = 2.0 * m_reg/n or similar)
        # To keep it simple and stable, let's solve: row sum = 1, col sum = 1 (doubly stochastic)
        # using standard Sinkhorn scaling.
        u = np.ones(n)
        v = np.ones(n)
        
        for _ in range(20):
            # u = 1.0 / K.dot(v)
            Kv = K.dot(v)
            Kv[Kv < 1e-12] = 1e-12
            u = 1.0 / Kv
            
            # v = 1.0 / K.T.dot(u)
            KTu = K.T.dot(u)
            KTu[KTu < 1e-12] = 1e-12
            v = 1.0 / KTu
            
        # Reconstruct the sparse transport matrix P
        # P_ij = u_i * v_j * K_ij
        # Extract row-wise weights
        Q = np.zeros((n, m_reg), dtype=X.dtype)
        for i in range(n):
            cols = indices[i]
            ki = data_k[i*m_reg : (i+1)*m_reg]
            pi = u[i] * v[cols] * ki
            # Normalize to ensure row sum is exactly 1
            sum_pi = np.sum(pi)
            Q[i] = pi / (sum_pi if sum_pi > 1e-12 else 1.0)
            
    else:
        raise ValueError(f"Unknown regularization mode: {regularization}")
        
    # 2. Compute Z and a
    # Z_i = sum_j Q_ij X_j
    # We can do this efficiently using advanced indexing
    # indices: (n, m_reg)
    # X[indices]: (n, m_reg, p)
    # Q[:, :, np.newaxis]: (n, m_reg, 1)
    # Z: (n, p)
    # This is fully vectorized!
    Z = np.sum(Q[:, :, np.newaxis] * X[indices], axis=1)
    
    # a_i = sum_j Q_ij ||X_j - Z_i||^2
    # We compute the squared distances from X_j to Z_i
    # Diff: (n, m_reg, p)
    diff = X[indices] - Z[:, np.newaxis, :]
    sq_dist = np.sum(diff ** 2, axis=2) # (n, m_reg)
    a = np.sum(Q * sq_dist, axis=1) # (n,)
    
    return Z, a, indices
