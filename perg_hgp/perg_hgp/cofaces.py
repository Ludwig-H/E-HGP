import torch
import numpy as np
import itertools

def solve_support_ball_batched(sub_Z, sub_a):
    """
    Solves the support ball problem for a batch of U subsets of size <= 4.
    sub_Z: shape (U, size, 3)
    sub_a: shape (U, size)
    """
    U, size, _ = sub_Z.shape
    device = sub_Z.device

    if size == 1:
        return sub_Z[:, 0], sub_a[:, 0]

    v = sub_Z[:, 1:] - sub_Z[:, 0:1] # shape (U, size-1, 3)

    G = torch.matmul(v, v.transpose(1, 2)) # shape (U, size-1, size-1)

    v_norm_sq = torch.sum(v ** 2, dim=2)
    h = 0.5 * (v_norm_sq + sub_a[:, 1:] - sub_a[:, 0:1])

    if size == 2:
        alpha = h / (G[:, 0, 0:1] + 1e-12)
    else:
        ridge = 1e-6 * torch.eye(size-1, device=device).unsqueeze(0)
        try:
            alpha = torch.linalg.solve(G + ridge, h)
        except RuntimeError:
            pinv = torch.linalg.pinv(G)
            alpha = torch.bmm(pinv, h.unsqueeze(2)).squeeze(2)

    center = sub_Z[:, 0] + torch.bmm(alpha.unsqueeze(1), v).squeeze(1)
    G_alpha = torch.bmm(G, alpha.unsqueeze(2)).squeeze(2)
    radius_sq = sub_a[:, 0] + torch.sum(alpha * G_alpha, dim=1)

    return center, radius_sq


def solve_weighted_miniball_batched(Z_cofaces, a_cofaces, tol=1e-6):
    """
    Batched solver for weighted miniball in 3D for U cofaces of size Nc <= 4.
    Z_cofaces: shape (U, Nc, 3)
    a_cofaces: shape (U, Nc)
    """
    U, Nc, _ = Z_cofaces.shape
    device = Z_cofaces.device
    tol = max(tol, 1e-5)

    subsets = []
    for r in range(1, min(5, Nc + 1)):
        for S in itertools.combinations(range(Nc), r):
            subsets.append(list(S))

    best_r2 = torch.full((U,), float('inf'), device=device)
    best_centers = torch.zeros((U, 3), device=device)

    for S in subsets:
        sz = len(S)
        sub_Z = Z_cofaces[:, S]
        sub_a = a_cofaces[:, S]

        c, r2 = solve_support_ball_batched(sub_Z, sub_a)

        if sz == 1:
            lambdas = torch.ones((U, 1), device=device)
        else:
            v = sub_Z[:, 1:] - sub_Z[:, 0:1]
            G = torch.matmul(v, v.transpose(1, 2))
            v_norm_sq = torch.sum(v ** 2, dim=2)
            h = 0.5 * (v_norm_sq + sub_a[:, 1:] - sub_a[:, 0:1])

            ridge = 1e-6 * torch.eye(sz-1, device=device).unsqueeze(0)
            try:
                if sz == 2:
                    alpha = h / (G[:, 0, 0:1] + 1e-12)
                else:
                    alpha = torch.linalg.solve(G + ridge, h)
            except RuntimeError:
                pinv = torch.linalg.pinv(G)
                alpha = torch.bmm(pinv, h.unsqueeze(2)).squeeze(2)

            lambdas = torch.zeros((U, sz), device=device)
            lambdas[:, 1:] = alpha
            lambdas[:, 0] = 1.0 - torch.sum(alpha, dim=1)

        dual_ok = torch.all(lambdas >= -tol, dim=1)

        diff = Z_cofaces - c.unsqueeze(1)
        phi = torch.sum(diff ** 2, dim=2) + a_cofaces
        primal_ok = torch.all(phi <= r2.unsqueeze(1) + tol, dim=1)

        valid = dual_ok & primal_ok

        better = valid & (r2 < best_r2)
        best_r2 = torch.where(better, r2, best_r2)
        best_centers = torch.where(better.unsqueeze(1), c, best_centers)

    failed = best_r2 == float('inf')
    if failed.any():
        best_r2 = torch.where(failed, a_cofaces[:, 0], best_r2)
        best_centers = torch.where(failed.unsqueeze(1), Z_cofaces[:, 0], best_centers)

    return best_centers, best_r2


def solve_support_ball_3d(S_Z, S_a):
    """
    Solves the weighted miniball problem exactly for a small support set of size <= 4 in 3D.
    S_Z: shape (size, 3)
    S_a: shape (size,)
    Returns:
        center: (3,)
        radius_sq: float
    """
    size = S_Z.shape[0]
    device = S_Z.device

    if size == 1:
        return S_Z[0], S_a[0]

    # Pivot is element 0
    v = S_Z[1:] - S_Z[0] # shape (size-1, 3)

    # Gram matrix G
    G = torch.matmul(v, v.t()) # shape (size-1, size-1)

    # RHS h_u = 0.5 * (||v_u||^2 + a_u - a_0)
    v_norm_sq = torch.sum(v ** 2, dim=1)
    h = 0.5 * (v_norm_sq + S_a[1:] - S_a[0])

    try:
        # Solve G alpha = h
        if size == 2:
            alpha = h / G[0, 0]
        else:
            alpha = torch.linalg.solve(G, h)
    except RuntimeError:
        # Fallback to pseudo-inverse
        pinv = torch.linalg.pinv(G)
        alpha = torch.matmul(pinv, h)

    center = S_Z[0] + torch.matmul(alpha, v)
    radius_sq = S_a[0] + torch.dot(alpha, torch.matmul(G, alpha))

    return center, radius_sq


def solve_weighted_miniball_brute_force_3d(Z_coface, a_coface, tol=1e-6):
    """
    Brute-force weighted miniball solver in 3D (support size <= 4) acting as an oracle.
    Guarantees the exact global minimum.
    """
    n = Z_coface.shape[0]
    device = Z_coface.device
    tol = max(tol, 1e-5)

    best_r2 = np.inf
    best_center = None

    # Try all subsets of size 1, 2, 3, 4
    for r in range(1, min(5, n + 1)):
        for S in itertools.combinations(range(n), r):
            S = list(S)
            c, r2 = solve_support_ball_3d(Z_coface[S], a_coface[S])

            # Compute multipliers
            if len(S) == 1:
                lambdas = torch.tensor([1.0], device=device)
            else:
                sub_Z = Z_coface[S]
                sub_a = a_coface[S]
                v = sub_Z[1:] - sub_Z[0]
                G = torch.matmul(v, v.t())
                v_norm_sq = torch.sum(v ** 2, dim=1)
                h = 0.5 * (v_norm_sq + sub_a[1:] - sub_a[0])
                try:
                    if len(S) == 2:
                        alpha = h / G[0, 0]
                    else:
                        alpha = torch.linalg.solve(G, h)
                except RuntimeError:
                    pinv = torch.linalg.pinv(G)
                    alpha = torch.matmul(pinv, h)
                lambdas = torch.zeros(len(S), device=device)
                lambdas[1:] = alpha
                lambdas[0] = 1.0 - torch.sum(alpha)

            # Check dual feasibility
            dual_ok = torch.all(lambdas >= -tol)

            # Check containment of all coface points
            diff = Z_coface - c
            phi = torch.sum(diff ** 2, dim=1) + a_coface
            primal_ok = torch.all(phi <= r2 + tol)

            if dual_ok and primal_ok:
                r2_val = r2.item()
                if r2_val < best_r2:
                    best_r2 = r2_val
                    best_center = c

    if best_center is None:
        return Z_coface[0], a_coface[0]

    return best_center, torch.tensor(best_r2, device=device)


def solve_weighted_miniball_active_set_3d(Z_coface, a_coface, max_iter=100, tol=1e-6):
    """
    Exact active-set weighted miniball solver in 3D.
    Guarantees convergence to the global minimum of the coface.
    """
    n = Z_coface.shape[0]
    device = Z_coface.device
    tol = max(tol, 1e-5)

    S = [0]
    c, r2 = Z_coface[0], a_coface[0]

    for it in range(max_iter):
        sub_Z = Z_coface[S]
        sub_a = a_coface[S]

        # 1. Solve equality-constrained problem on S
        c, r2 = solve_support_ball_3d(sub_Z, sub_a)

        # Compute multipliers (lambdas)
        if len(S) == 1:
            lambdas = torch.tensor([1.0], device=device)
        else:
            v = sub_Z[1:] - sub_Z[0]
            G = torch.matmul(v, v.t())
            v_norm_sq = torch.sum(v ** 2, dim=1)
            h = 0.5 * (v_norm_sq + sub_a[1:] - sub_a[0])
            try:
                if len(S) == 2:
                    alpha = h / G[0, 0]
                else:
                    alpha = torch.linalg.solve(G, h)
            except RuntimeError:
                pinv = torch.linalg.pinv(G)
                alpha = torch.matmul(pinv, h)

            lambdas = torch.zeros(len(S), device=device)
            lambdas[1:] = alpha
            lambdas[0] = 1.0 - torch.sum(alpha)

        # Check if any multiplier is negative
        min_lambda_idx = torch.argmin(lambdas).item()
        min_lambda = lambdas[min_lambda_idx].item()

        if min_lambda < -tol:
            # Drop the point with the negative multiplier
            S.pop(min_lambda_idx)
            continue

        # 2. Check containment of all points in the coface
        diff = Z_coface - c
        phi = torch.sum(diff ** 2, dim=1) + a_coface
        violations = phi - r2

        max_viol_idx = torch.argmax(violations).item()
        max_viol = violations[max_viol_idx].item()

        if max_viol <= tol:
            # Feasible and optimal!
            break

        # 3. Add most violating point
        S.append(max_viol_idx)

        if len(S) > 4:
            # S size is 5, we must drop one point.
            # We brute-force the 5 subsets of size 4 of S to find the one that covers S
            best_sub_r2 = -1.0
            best_sub_c = None
            best_sub_S = None
            for idx_to_remove in range(5):
                candidate_S = [S[j] for j in range(5) if j != idx_to_remove]
                c_sub, r2_sub = solve_support_ball_3d(Z_coface[candidate_S], a_coface[candidate_S])

                rem_pt_idx = S[idx_to_remove]
                dist_rem_sq = torch.sum((Z_coface[rem_pt_idx] - c_sub) ** 2) + a_coface[rem_pt_idx]
                if dist_rem_sq <= r2_sub + tol:
                    if r2_sub > best_sub_r2:
                        best_sub_r2 = r2_sub
                        best_sub_c = c_sub
                        best_sub_S = candidate_S
            if best_sub_S is not None:
                S = best_sub_S
                c = best_sub_c
                r2 = best_sub_r2
            else:
                # Fallback to brute force
                return solve_weighted_miniball_brute_force_3d(Z_coface, a_coface, tol=tol)

    # Double check feasibility via brute-force if active set failed to converge
    diff = Z_coface - c
    phi = torch.sum(diff ** 2, dim=1) + a_coface
    if torch.any(phi > r2 + tol):
        return solve_weighted_miniball_brute_force_3d(Z_coface, a_coface, tol=tol)

    return c, r2


def extract_top_cofaces(witness_pool, Z, a, eta, grid_z, K, cfg):
    """
    Extracts candidate cofaces from the witness pool and filters them by top-consistency in chunks.
    """
    device = Z.device
    W_coords = witness_pool.coords
    M = W_coords.shape[0]

    cofaces_list = []

    # 1. Query neighborhood of witnesses in chunks to prevent VRAM spikes
    chunk_size = 10000
    for start in range(0, M, chunk_size):
        end = min(start + chunk_size, M)
        W_chunk = W_coords[start:end]

        nbr_indices_chunk, nbr_dists_sq_chunk = grid_z.query_knn_grid(W_chunk, m_local=cfg.m_active)
        Z_nbrs = Z[nbr_indices_chunk]
        a_nbrs = a[nbr_indices_chunk]
        diff = Z_nbrs - W_chunk[:, None, :]
        dist_sq = torch.sum(diff ** 2, dim=2)
        E = dist_sq + a_nbrs

        # Sort neighbors by energy
        sorted_idx = torch.argsort(E, dim=1)
        row_indices = torch.arange(end - start, device=device).unsqueeze(1)
        cofaces_chunk = nbr_indices_chunk[row_indices, sorted_idx[:, :K + 1]]

        # Sort indices of the coface for deduplication
        cofaces_chunk_sorted, _ = torch.sort(cofaces_chunk, dim=1)
        cofaces_list.append(cofaces_chunk_sorted)

    if len(cofaces_list) == 0:
        return torch.empty((0, K + 1), dtype=torch.int64, device=device), \
               torch.empty((0, 3), dtype=torch.float32, device=device), \
               torch.empty((0,), dtype=torch.float32, device=device)

    # Stack and deduplicate
    cofaces_tensor = torch.cat(cofaces_list, dim=0)
    unique_cofaces = torch.unique(cofaces_tensor, dim=0)

    U = unique_cofaces.shape[0]

    valid_cofaces = []
    centers_list = []
    weights_list = []

    # 2. Solve weighted miniball and check top-consistency in chunks
    cof_chunk_size = 5000
    for start in range(0, U, cof_chunk_size):
        end = min(start + cof_chunk_size, U)
        chunk_len = end - start

        cof_idx_chunk = unique_cofaces[start:end]
        Z_cof = Z[cof_idx_chunk] # (chunk_len, K+1, 3)
        a_cof = a[cof_idx_chunk] # (chunk_len, K+1)

        # Use batched solver for target ranks up to K=20, otherwise fallback
        if K + 1 <= 21:
            centers_chunk, r2_chunk = solve_weighted_miniball_batched(Z_cof, a_cof, tol=cfg.gabriel_tol)
        else:
            # Fallback to sequential active-set solver
            centers_chunk = torch.zeros((chunk_len, 3), device=device)
            r2_chunk = torch.zeros(chunk_len, device=device)
            for i in range(chunk_len):
                c, r2 = solve_weighted_miniball_active_set_3d(Z_cof[i], a_cof[i], tol=cfg.gabriel_tol)
                centers_chunk[i] = c
                r2_chunk[i] = r2

        # Batched top-consistency check: Top_{K+1}(c) == cof_idx
        nbr_idx, _ = grid_z.query_knn_grid(centers_chunk, m_local=cfg.m_active) # (chunk_len, m_active)

        diff_pts = Z[nbr_idx] - centers_chunk.unsqueeze(1) # (chunk_len, m_active, 3)
        phi_pts = torch.sum(diff_pts ** 2, dim=2) + a[nbr_idx] # (chunk_len, m_active)

        sorted_idx = torch.argsort(phi_pts, dim=1) # (chunk_len, m_active)
        row_idx = torch.arange(chunk_len, device=device).unsqueeze(1)
        top_k_plus_1 = nbr_idx[row_idx, sorted_idx[:, :K + 1]] # (chunk_len, K+1)
        top_k_plus_1_sorted, _ = torch.sort(top_k_plus_1, dim=1)

        matches = torch.all(top_k_plus_1_sorted == cof_idx_chunk, dim=1) # (chunk_len,)

        valid_idx = torch.where(matches)[0]
        if valid_idx.shape[0] > 0:
            valid_cofaces.append(cof_idx_chunk[valid_idx])
            centers_list.append(centers_chunk[valid_idx])
            weights_list.append(torch.sqrt(torch.clamp(r2_chunk[valid_idx], min=0.0)))

    if len(valid_cofaces) == 0:
        return torch.empty((0, K + 1), dtype=torch.int64, device=device), \
               torch.empty((0, 3), dtype=torch.float32, device=device), \
               torch.empty((0,), dtype=torch.float32, device=device)

    return torch.cat(valid_cofaces, dim=0), torch.cat(centers_list, dim=0), torch.cat(weights_list, dim=0)
