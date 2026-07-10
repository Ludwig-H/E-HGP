import torch
import numpy as np
import itertools


def _validated_relative_tolerance(tol):
    value = float(tol)
    if not np.isfinite(value) or value <= 0:
        raise ValueError(f"tol must be finite and positive; got {tol}")
    return max(value, 64.0 * torch.finfo(torch.float64).eps)


def _power_tolerance(Z, a, rtol):
    """Translation-invariant tolerance in squared-distance units."""
    Z64 = Z.to(torch.float64)
    a64 = a.to(torch.float64)
    if Z64.ndim == 2:
        spatial = torch.max(torch.sum((Z64 - Z64[:1]) ** 2, dim=1))
        weight = torch.max(torch.abs(a64 - a64[:1]))
    else:
        spatial = torch.amax(torch.sum((Z64 - Z64[:, :1]) ** 2, dim=2), dim=1)
        weight = torch.amax(torch.abs(a64 - a64[:, :1]), dim=1)
    scale = torch.maximum(spatial, weight)
    return (rtol + 128.0 * torch.finfo(torch.float64).eps) * scale


def _outward_feasible_radius(Z, a, center):
    """Return max power rounded upward in float64."""
    if Z.ndim == 2:
        powers = torch.sum((Z - center) ** 2, dim=1) + a
        radius_sq = torch.max(powers)
    else:
        powers = torch.sum((Z - center[:, None]) ** 2, dim=2) + a
        radius_sq = torch.amax(powers, dim=1)
    return torch.nextafter(radius_sq, torch.full_like(radius_sq, float('inf')))


def _outward_float32_radius(radius_sq):
    """Store sqrt(radius_sq) in float32 without rounding below the float64 value."""
    radius_64 = torch.sqrt(torch.clamp(radius_sq.to(torch.float64), min=0.0))
    radius_32 = radius_64.to(torch.float32)
    rounded_down = radius_32.to(torch.float64) < radius_64
    radius_32_up = torch.nextafter(radius_32, torch.full_like(radius_32, float('inf')))
    return torch.where(rounded_down, radius_32_up, radius_32)


def _solve_support_ball_batched_64(sub_Z, sub_a):
    """Solve equality-constrained support balls after power-unit scaling."""
    sub_Z = sub_Z.to(torch.float64)
    sub_a = sub_a.to(torch.float64)
    U, size, _ = sub_Z.shape
    device = sub_Z.device

    if size == 1:
        alpha = torch.empty((U, 0), dtype=torch.float64, device=device)
        return sub_Z[:, 0], sub_a[:, 0], alpha

    v = sub_Z[:, 1:] - sub_Z[:, :1]
    delta_a = sub_a[:, 1:] - sub_a[:, :1]
    v_norm_sq = torch.sum(v ** 2, dim=2)
    scale_sq = torch.maximum(
        torch.amax(v_norm_sq, dim=1),
        torch.amax(torch.abs(delta_a), dim=1),
    )
    safe_scale_sq = torch.where(scale_sq > 0, scale_sq, torch.ones_like(scale_sq))
    scale = torch.sqrt(safe_scale_sq)

    v_scaled = v / scale[:, None, None]
    delta_a_scaled = delta_a / safe_scale_sq[:, None]
    G = torch.matmul(v_scaled, v_scaled.transpose(1, 2))
    h = 0.5 * (torch.sum(v_scaled ** 2, dim=2) + delta_a_scaled)

    if size == 2:
        denominator = G[:, 0, 0:1]
        safe_denominator = torch.where(
            denominator != 0, denominator, torch.ones_like(denominator)
        )
        alpha = h / safe_denominator
        alpha = torch.where(denominator != 0, alpha, torch.zeros_like(alpha))
    else:
        alpha, info = torch.linalg.solve_ex(G, h)
        residual = torch.amax(
            torch.abs(torch.bmm(G, alpha.unsqueeze(2)).squeeze(2) - h), dim=1
        )
        reference = 1.0 + torch.amax(torch.abs(h), dim=1)
        bad = (
            (info != 0)
            | ~torch.isfinite(alpha).all(dim=1)
            | (residual > 256.0 * torch.finfo(torch.float64).eps * reference)
        )
        if torch.any(bad):
            pinv = torch.linalg.pinv(
                G[bad], hermitian=True, rtol=256.0 * torch.finfo(torch.float64).eps
            )
            alpha[bad] = torch.bmm(pinv, h[bad].unsqueeze(2)).squeeze(2)

    center_offset = torch.bmm(alpha.unsqueeze(1), v_scaled).squeeze(1)
    center = sub_Z[:, 0] + scale[:, None] * center_offset
    G_alpha = torch.bmm(G, alpha.unsqueeze(2)).squeeze(2)
    radius_sq = sub_a[:, 0] + safe_scale_sq * torch.sum(alpha * G_alpha, dim=1)

    zero_scale = scale_sq == 0
    if torch.any(zero_scale):
        center[zero_scale] = sub_Z[zero_scale, 0]
        radius_sq[zero_scale] = sub_a[zero_scale, 0]
        alpha[zero_scale] = 0.0

    return center, radius_sq, alpha


def solve_support_ball_batched(sub_Z, sub_a):
    """
    Solves the support ball problem for a batch of U subsets of size <= 4.
    sub_Z: shape (U, size, 3)
    sub_a: shape (U, size)
    """
    dtype_orig = sub_Z.dtype
    center, radius_sq, _ = _solve_support_ball_batched_64(sub_Z, sub_a)
    return center.to(dtype_orig), radius_sq.to(dtype_orig)


def solve_weighted_miniball_batched(Z_cofaces, a_cofaces, tol=1e-6):
    """
    Batched solver for weighted miniballs in 3D.

    Candidate supports have size at most four, regardless of the coface size.
    Z_cofaces: shape (U, Nc, 3)
    a_cofaces: shape (U, Nc)
    """
    Z_cofaces_64 = Z_cofaces.to(torch.float64)
    a_cofaces_64 = a_cofaces.to(torch.float64)

    U, Nc, _ = Z_cofaces_64.shape
    device = Z_cofaces_64.device
    rtol = _validated_relative_tolerance(tol)
    power_tol = _power_tolerance(Z_cofaces_64, a_cofaces_64, rtol)

    subsets = []
    for r in range(1, min(5, Nc + 1)):
        for S in itertools.combinations(range(Nc), r):
            subsets.append(list(S))

    best_r2 = torch.full((U,), float('inf'), dtype=torch.float64, device=device)
    best_centers = torch.zeros((U, 3), dtype=torch.float64, device=device)

    for S in subsets:
        sz = len(S)
        sub_Z = Z_cofaces_64[:, S]
        sub_a = a_cofaces_64[:, S]

        c, r2, alpha = _solve_support_ball_batched_64(sub_Z, sub_a)

        if sz == 1:
            lambdas = torch.ones((U, 1), dtype=torch.float64, device=device)
        else:
            lambdas = torch.zeros((U, sz), dtype=torch.float64, device=device)
            lambdas[:, 1:] = alpha
            lambdas[:, 0] = 1.0 - torch.sum(alpha, dim=1)

        dual_ok = torch.all(lambdas >= -rtol, dim=1)

        diff = Z_cofaces_64 - c.unsqueeze(1)
        a_reference = torch.amin(a_cofaces_64, dim=1)
        phi_relative = torch.sum(diff ** 2, dim=2) + a_cofaces_64 - a_reference[:, None]
        r2_relative = r2 - a_reference
        primal_ok = torch.all(
            phi_relative <= r2_relative.unsqueeze(1) + power_tol.unsqueeze(1), dim=1
        )

        feasible_r2 = torch.amax(phi_relative, dim=1) + a_reference
        valid = dual_ok & primal_ok & torch.isfinite(feasible_r2) & torch.isfinite(c).all(dim=1)

        better = valid & (feasible_r2 < best_r2)
        best_r2 = torch.where(better, feasible_r2, best_r2)
        best_centers = torch.where(better.unsqueeze(1), c, best_centers)

    failed = best_r2 == float('inf')
    if failed.any():
        for row in torch.where(failed)[0].tolist():
            center, radius_sq = solve_weighted_miniball_active_set_3d(
                Z_cofaces_64[row], a_cofaces_64[row], tol=rtol
            )
            best_centers[row] = center.to(torch.float64)
            best_r2[row] = radius_sq.to(torch.float64)

    best_r2 = _outward_feasible_radius(Z_cofaces_64, a_cofaces_64, best_centers)
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
    dtype_orig = S_Z.dtype
    center, radius_sq, _ = _solve_support_ball_batched_64(
        S_Z.unsqueeze(0), S_a.unsqueeze(0)
    )
    center = center[0]
    radius_sq = radius_sq[0]
    return center.to(dtype_orig), radius_sq.to(dtype_orig)


def solve_weighted_miniball_brute_force_3d(Z_coface, a_coface, tol=1e-6):
    """
    Exhaustive support oracle for the weighted 3D miniball, up to tolerance.
    """
    Z_coface_64 = Z_coface.to(torch.float64)
    a_coface_64 = a_coface.to(torch.float64)

    n = Z_coface_64.shape[0]
    device = Z_coface_64.device
    rtol = _validated_relative_tolerance(tol)
    power_tol = _power_tolerance(Z_coface_64, a_coface_64, rtol)

    best_r2 = np.inf
    best_center = None

    # Try all subsets of size 1, 2, 3, 4
    for r in range(1, min(5, n + 1)):
        for S in itertools.combinations(range(n), r):
            S = list(S)
            c_batch, r2_batch, alpha_batch = _solve_support_ball_batched_64(
                Z_coface_64[S].unsqueeze(0), a_coface_64[S].unsqueeze(0)
            )
            c = c_batch[0]
            r2 = r2_batch[0]
            alpha = alpha_batch[0]

            # Compute multipliers
            if len(S) == 1:
                lambdas = torch.tensor([1.0], dtype=torch.float64, device=device)
            else:
                lambdas = torch.zeros(len(S), dtype=torch.float64, device=device)
                lambdas[1:] = alpha
                lambdas[0] = 1.0 - torch.sum(alpha)

            # Check dual feasibility
            dual_ok = torch.all(lambdas >= -rtol)

            # Check containment of all coface points
            diff = Z_coface_64 - c
            a_reference = torch.min(a_coface_64)
            phi_relative = torch.sum(diff ** 2, dim=1) + a_coface_64 - a_reference
            r2_relative = r2 - a_reference
            primal_ok = torch.all(phi_relative <= r2_relative + power_tol)

            if dual_ok and primal_ok:
                r2_val = (torch.max(phi_relative) + a_reference).item()
                if r2_val < best_r2:
                    best_r2 = r2_val
                    best_center = c

    if best_center is None:
        raise RuntimeError("weighted miniball support enumeration found no feasible support")

    best_r2_outward = _outward_feasible_radius(Z_coface_64, a_coface_64, best_center)
    return best_center, best_r2_outward


def solve_weighted_miniball_active_set_3d(Z_coface, a_coface, max_iter=100, tol=1e-6):
    """
    Active-set weighted miniball solver with exhaustive support fallback.
    """
    Z_coface_64 = Z_coface.to(torch.float64)
    a_coface_64 = a_coface.to(torch.float64)

    device = Z_coface_64.device
    rtol = _validated_relative_tolerance(tol)
    power_tol = _power_tolerance(Z_coface_64, a_coface_64, rtol)

    S = [0]
    c, r2 = Z_coface_64[0], a_coface_64[0]

    converged = False
    for _ in range(max_iter):
        sub_Z = Z_coface_64[S]
        sub_a = a_coface_64[S]

        # 1. Solve equality-constrained problem on S
        c_batch, r2_batch, alpha_batch = _solve_support_ball_batched_64(
            sub_Z.unsqueeze(0), sub_a.unsqueeze(0)
        )
        c = c_batch[0]
        r2 = r2_batch[0]
        alpha = alpha_batch[0]

        # Compute multipliers (lambdas)
        if len(S) == 1:
            lambdas = torch.tensor([1.0], dtype=torch.float64, device=device)
        else:
            lambdas = torch.zeros(len(S), dtype=torch.float64, device=device)
            lambdas[1:] = alpha
            lambdas[0] = 1.0 - torch.sum(alpha)

        # Check if any multiplier is negative
        if not torch.isfinite(c).all() or not torch.isfinite(r2) or not torch.isfinite(lambdas).all():
            return solve_weighted_miniball_brute_force_3d(Z_coface, a_coface, tol=rtol)

        min_lambda_idx = torch.argmin(lambdas).item()
        min_lambda = lambdas[min_lambda_idx].item()

        if min_lambda < -rtol:
            # Drop the point with the negative multiplier
            S.pop(min_lambda_idx)
            continue

        # 2. Check containment of all points in the coface
        diff = Z_coface_64 - c
        a_reference = torch.min(a_coface_64)
        phi_relative = torch.sum(diff ** 2, dim=1) + a_coface_64 - a_reference
        r2_relative = r2 - a_reference
        violations = phi_relative - r2_relative

        max_viol_idx = torch.argmax(violations).item()
        max_viol = violations[max_viol_idx].item()

        if max_viol <= power_tol.item():
            # Feasible and optimal!
            converged = True
            break

        # 3. Add most violating point
        if max_viol_idx in S:
            return solve_weighted_miniball_brute_force_3d(Z_coface, a_coface, tol=rtol)
        S.append(max_viol_idx)

        if len(S) > 4:
            # S size is 5, we must drop one point.
            # We brute-force the 5 subsets of size 4 of S to find the one that covers S
            best_sub_r2 = float('inf')
            best_sub_c = None
            best_sub_S = None
            for idx_to_remove in range(5):
                candidate_S = [S[j] for j in range(5) if j != idx_to_remove]
                c_sub, r2_sub = solve_support_ball_3d(Z_coface_64[candidate_S], a_coface_64[candidate_S])
                c_sub = c_sub.to(torch.float64)
                r2_sub = r2_sub.to(torch.float64)

                rem_pt_idx = S[idx_to_remove]
                dist_rem_relative = (
                    torch.sum((Z_coface_64[rem_pt_idx] - c_sub) ** 2)
                    + a_coface_64[rem_pt_idx]
                    - a_reference
                )
                if dist_rem_relative <= r2_sub - a_reference + power_tol:
                    if r2_sub < best_sub_r2:
                        best_sub_r2 = r2_sub
                        best_sub_c = c_sub
                        best_sub_S = candidate_S
            if best_sub_S is not None:
                S = best_sub_S
                c = best_sub_c
                r2 = best_sub_r2
            else:
                # Fallback to brute force
                return solve_weighted_miniball_brute_force_3d(Z_coface, a_coface, tol=rtol)

    if not converged:
        return solve_weighted_miniball_brute_force_3d(Z_coface, a_coface, tol=rtol)

    # Double check feasibility after convergence.
    diff = Z_coface_64 - c
    a_reference = torch.min(a_coface_64)
    phi_relative = torch.sum(diff ** 2, dim=1) + a_coface_64 - a_reference
    if torch.any(phi_relative > r2 - a_reference + power_tol):
        return solve_weighted_miniball_brute_force_3d(Z_coface, a_coface, tol=rtol)

    feasible_r2 = _outward_feasible_radius(Z_coface_64, a_coface_64, c)
    return c, feasible_r2


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

        nbr_indices_chunk, nbr_dists_sq_chunk_power = grid_z.query_power_grid(W_chunk, a, m_local=cfg.m_active)
        cofaces_chunk = nbr_indices_chunk[:, :K + 1]

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
            centers_chunk = torch.zeros((chunk_len, 3), dtype=torch.float64, device=device)
            r2_chunk = torch.zeros(chunk_len, dtype=torch.float64, device=device)
            for i in range(chunk_len):
                c, r2 = solve_weighted_miniball_active_set_3d(Z_cof[i], a_cof[i], tol=cfg.gabriel_tol)
                centers_chunk[i] = c
                r2_chunk[i] = r2

        # Batched top-consistency check: Top_{K+1}(c) == cof_idx
        nbr_idx, _ = grid_z.query_power_grid(centers_chunk, a, m_local=cfg.m_active) # (chunk_len, m_active)
        top_k_plus_1 = nbr_idx[:, :K + 1] # (chunk_len, K+1)
        top_k_plus_1_sorted, _ = torch.sort(top_k_plus_1, dim=1)

        matches = torch.all(top_k_plus_1_sorted == cof_idx_chunk, dim=1) # (chunk_len,)

        valid_idx = torch.where(matches)[0]
        if valid_idx.shape[0] > 0:
            valid_cofaces.append(cof_idx_chunk[valid_idx])
            centers_list.append(centers_chunk[valid_idx])
            weights_list.append(_outward_float32_radius(r2_chunk[valid_idx]))

    if len(valid_cofaces) == 0:
        return torch.empty((0, K + 1), dtype=torch.int64, device=device), \
               torch.empty((0, 3), dtype=torch.float32, device=device), \
               torch.empty((0,), dtype=torch.float32, device=device)

    return torch.cat(valid_cofaces, dim=0), torch.cat(centers_list, dim=0), torch.cat(weights_list, dim=0)
