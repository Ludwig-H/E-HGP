# cython: language_level=3
# cython: boundscheck=False
# cython: nonecheck=False
# cython: initializedcheck=False
"""Cython/C++ core utilities for E-HGP."""

import numpy as np
cimport numpy as np
from libcpp.vector cimport vector
from libcpp cimport bool
from libc.math cimport fabs, isnan, NAN
from cython.parallel import prange

# -----------------------------------------------------------------------------
# Linear System Solver for Small Matrices (Gaussian elimination with pivoting)
# -----------------------------------------------------------------------------
cdef int solve_linear_system_small(double* G, double* h, double* alpha, int d) nogil:
    cdef int i, j, k, max_row
    cdef double max_val, temp, factor
    cdef double[32][32] A
    cdef double[32] b
    
    if d > 32:
        return -1 # Exceeds capacity
        
    for i in range(d):
        for j in range(d):
            A[i][j] = G[i * d + j]
        b[i] = h[i]
        
    # Gaussian elimination with partial pivoting
    for i in range(d):
        max_row = i
        max_val = fabs(A[i][i])
        for k in range(i + 1, d):
            if fabs(A[k][i]) > max_val:
                max_val = fabs(A[k][i])
                max_row = k
                
        # Swap rows
        if max_row != i:
            for j in range(i, d):
                temp = A[i][j]
                A[i][j] = A[max_row][j]
                A[max_row][j] = temp
            temp = b[i]
            b[i] = b[max_row]
            b[max_row] = temp
            
        if fabs(A[i][i]) < 1e-12:
            return -1 # Singular matrix
            
        for k in range(i + 1, d):
            factor = A[k][i] / A[i][i]
            for j in range(i, d):
                A[k][j] -= factor * A[i][j]
            b[k] -= factor * b[i]
            
    # Back substitution
    for i in range(d - 1, -1, -1):
        temp = b[i]
        for j in range(i + 1, d):
            temp -= A[i][j] * alpha[j]
        alpha[i] = temp / A[i][i]
        
    return 0


# -----------------------------------------------------------------------------
# C++ Active-Set Additive Weighted Miniball Single Coface Solver
# -----------------------------------------------------------------------------
cdef int weighted_miniball_active_set_single_cpp(
    const double* Z_data,
    const double* a_data,
    const int* coface_indices,
    int K_plus_1,
    int p,
    double* out_center,
    double* out_radius_sq,
    double tol
) nogil:
    # Active set S stores local indices in {0, ..., K}
    cdef int[32] S
    cdef int sub_size = 1
    S[0] = 0
    
    cdef double[32][32] sub_Z
    cdef double[32] sub_a
    
    cdef double[32][32] v
    cdef double[32][32] G
    cdef double[32] h
    cdef double[32] alpha
    cdef double[32] lambdas
    
    cdef double[32] center
    cdef double radius_sq = 0.0
    
    cdef int i, j, k, u, vv, sub_idx, node_idx, other_idx, neg_idx, max_viol_idx
    cdef double val, sum_alpha, temp, v_norm_sq, max_viol, viol, min_lambda
    cdef bool already_in
    
    cdef int max_iter = 100
    cdef int iter_count = 0
    
    while iter_count < max_iter:
        iter_count += 1
        
        # Load sub Z and a
        for i in range(sub_size):
            sub_idx = coface_indices[S[i]]
            for j in range(p):
                sub_Z[i][j] = Z_data[sub_idx * p + j]
            sub_a[i] = a_data[sub_idx]
            
        if sub_size == 1:
            for j in range(p):
                center[j] = sub_Z[0][j]
            radius_sq = sub_a[0]
            lambdas[0] = 1.0
        else:
            # Construct v_u = sub_Z[u+1] - sub_Z[0]
            for u in range(sub_size - 1):
                for j in range(p):
                    v[u][j] = sub_Z[u + 1][j] - sub_Z[0][j]
                    
            # Gram matrix G_uv = <v_u, v_v>
            for u in range(sub_size - 1):
                for vv in range(sub_size - 1):
                    val = 0.0
                    for j in range(p):
                        val += v[u][j] * v[vv][j]
                    G[u][vv] = val
                    
            # RHS h_u = 0.5 * (||v_u||^2 + a_u+1 - a_0)
            for u in range(sub_size - 1):
                v_norm_sq = 0.0
                for j in range(p):
                    v_norm_sq += v[u][j] * v[u][j]
                h[u] = 0.5 * (v_norm_sq + sub_a[u + 1] - sub_a[0])
                
            # Solve G alpha = h
            if solve_linear_system_small(&G[0][0], h, alpha, sub_size - 1) < 0:
                # Fallback in case of singularity: single vertex
                for j in range(p):
                    center[j] = sub_Z[0][j]
                radius_sq = sub_a[0]
                lambdas[0] = 1.0
            else:
                # Center: c_S = z_0 + sum_u alpha_u v_u
                for j in range(p):
                    val = 0.0
                    for u in range(sub_size - 1):
                        val += alpha[u] * v[u][j]
                    center[j] = sub_Z[0][j] + val
                    
                # Radius squared: t_S = a_0 + alpha^T G alpha
                val = 0.0
                for u in range(sub_size - 1):
                    temp = 0.0
                    for vv in range(sub_size - 1):
                        temp += G[u][vv] * alpha[vv]
                    val += alpha[u] * temp
                radius_sq = sub_a[0] + val
                
                # Barycentric coordinates
                sum_alpha = 0.0
                for u in range(sub_size - 1):
                    lambdas[u + 1] = alpha[u]
                    sum_alpha += alpha[u]
                lambdas[0] = 1.0 - sum_alpha
                
        # Check if any active points have negative weights
        neg_idx = -1
        min_lambda = 0.0
        for i in range(sub_size):
            if lambdas[i] < min_lambda:
                min_lambda = lambdas[i]
                neg_idx = i
                
        if min_lambda < -tol and neg_idx != -1:
            # Remove the point with the most negative weight from active set
            for i in range(neg_idx, sub_size - 1):
                S[i] = S[i + 1]
            sub_size -= 1
            if sub_size == 0:
                S[0] = 0
                sub_size = 1
            continue
            
        # Check containment for all points in the coface
        max_viol = -1.0
        max_viol_idx = -1
        for i in range(K_plus_1):
            other_idx = coface_indices[i]
            # Calculate distance squared from Z[other_idx] to center
            temp = 0.0
            for j in range(p):
                val = Z_data[other_idx * p + j] - center[j]
                temp += val * val
            viol = temp + a_data[other_idx] - radius_sq
            
            if viol > max_viol:
                max_viol = viol
                max_viol_idx = i
                
        if max_viol <= tol:
            # Optimal miniball found
            for j in range(p):
                out_center[j] = center[j]
            out_radius_sq[0] = radius_sq
            return 0
            
        # Add the violating point to active set
        already_in = False
        for i in range(sub_size):
            if S[i] == max_viol_idx:
                already_in = True
                break
        if not already_in and sub_size < 32:
            S[sub_size] = max_viol_idx
            sub_size += 1
        else:
            # Numerical limit reached, return current
            for j in range(p):
                out_center[j] = center[j]
            out_radius_sq[0] = radius_sq
            return 0
            
    # Max iterations reached, return current
    for j in range(p):
        out_center[j] = center[j]
    out_radius_sq[0] = radius_sq
    return 0


# -----------------------------------------------------------------------------
# Cython Wrapper for Batch Active-Set Miniball Solver
# -----------------------------------------------------------------------------
def weighted_miniball_active_set_batch_cython(
    const double[:, ::1] Z,
    const double[::1] a,
    const int[:, ::1] cofaces,
    double tol=1e-6
):
    cdef Py_ssize_t B = cofaces.shape[0]
    cdef int K_plus_1 = cofaces.shape[1]
    cdef int p = Z.shape[1]
    
    cdef np.ndarray[np.float64_t, ndim=2] centers = np.zeros((B, p), dtype=np.float64)
    cdef np.ndarray[np.float64_t, ndim=1] radii_sq = np.zeros(B, dtype=np.float64)
    
    cdef double[:, ::1] centers_view = centers
    cdef double[::1] radii_sq_view = radii_sq
    
    cdef Py_ssize_t b
    
    with nogil:
        for b in prange(B, schedule='static'):
            weighted_miniball_active_set_single_cpp(
                &Z[0, 0],
                &a[0],
                &cofaces[b, 0],
                K_plus_1,
                p,
                &centers_view[b, 0],
                &radii_sq_view[b],
                tol
            )
            
    return centers, radii_sq


# -----------------------------------------------------------------------------
# Cython Implementation of condensed tree builder
# -----------------------------------------------------------------------------
cdef inline int _min_i32(int a, int b) nogil:
    return a if a <= b else b

cdef inline int _max_i32(int a, int b) nogil:
    return a if a >= b else b

def condense_tree_cython(
    const float[::1] W_nodes,
    const int[::1] U_mst,
    const int[::1] V_mst,
    const float[::1] W_mst,
    int min_cluster_size,
    double epsilon=0.0
):
    cdef Py_ssize_t N = W_nodes.shape[0]
    cdef Py_ssize_t M = W_mst.shape[0]
    cdef Py_ssize_t i, j, k
    
    cdef vector[vector[int]] children
    cdef vector[float] birth_r
    cdef vector[float] death_r
    cdef vector[float] stability
    cdef vector[float] size_at_birth
    cdef vector[float] n_in_cluster
    cdef vector[float] sum_join_lambda
    
    cdef vector[int] parent = vector[int](N)
    cdef vector[float] comp_weight = vector[float](N)
    cdef vector[int] comp_cid = vector[int](N)
    cdef vector[vector[int]] comp_nodes = vector[vector[int]](N)
    
    cdef vector[vector[int]] tracked_cids = vector[vector[int]](N)

    cdef np.ndarray[int, ndim=1] initial_membership = np.full(N, -1, dtype=np.int32)
    cdef np.ndarray[np.float32_t, ndim=1] join_r = np.full(N, np.inf, dtype=np.float32)

    cdef double EPS = 1e-12
    cdef int u, v, ru, rv, cid, node_idx
    cdef float r, lam, n_in, w_start, w_current, added_weight, n_parent
    cdef Py_ssize_t j_node, c_idx, cid_new
    cdef vector[int] new_ch
    cdef bool has_clusters

    # Initialization
    for i in range(N):
        parent[i] = i
        comp_weight[i] = W_nodes[i]
        comp_cid[i] = -1
        comp_nodes[i].push_back(i)

    cdef vector[Py_ssize_t] last_seen_batch = vector[Py_ssize_t](N, -1)
    cdef vector[int] unique_roots
    
    i = 0
    while i < M:
        w_start = W_mst[i]
        
        j = i
        while j < M:
            w_current = W_mst[j]
            if w_current - w_start <= epsilon:
                j += 1
            else:
                break
        
        r = W_mst[j-1]
        lam = 1.0 / (r + EPS)
        
        unique_roots.clear()
        
        for k in range(i, j):
            u = U_mst[k]
            v = V_mst[k]
            
            ru = u
            while parent[ru] != ru: parent[ru] = parent[parent[ru]]; ru = parent[ru]
            rv = v
            while parent[rv] != rv: parent[rv] = parent[parent[rv]]; rv = parent[rv]
            
            if ru == rv:
                continue
                
            if comp_weight[ru] < comp_weight[rv]:
                ru, rv = rv, ru
            
            if comp_cid[ru] != -1:
                tracked_cids[ru].push_back(comp_cid[ru])
                comp_cid[ru] = -1
            if comp_cid[rv] != -1:
                tracked_cids[rv].push_back(comp_cid[rv])
                comp_cid[rv] = -1
            
            if not tracked_cids[rv].empty():
                tracked_cids[ru].insert(tracked_cids[ru].end(), tracked_cids[rv].begin(), tracked_cids[rv].end())
                tracked_cids[rv].clear()
            
            parent[rv] = ru
            comp_weight[ru] += comp_weight[rv]
            comp_nodes[ru].insert(comp_nodes[ru].end(), comp_nodes[rv].begin(), comp_nodes[rv].end())
            comp_nodes[rv].clear()
            
            if last_seen_batch[ru] != i:
                unique_roots.push_back(ru)
                last_seen_batch[ru] = i
                
        for k in range(unique_roots.size()):
            ru = unique_roots[k]
            
            if parent[ru] != ru:
                continue
                
            has_clusters = not tracked_cids[ru].empty()
            
            if comp_weight[ru] >= min_cluster_size:
                if not has_clusters:
                    cid = children.size()
                    children.push_back(vector[int]())
                    birth_r.push_back(r)
                    death_r.push_back(NAN)
                    stability.push_back(0.0)
                    size_at_birth.push_back(comp_weight[ru])
                    n_in_cluster.push_back(comp_weight[ru])
                    sum_join_lambda.push_back(comp_weight[ru] * lam)
                    
                    for j_node in range(comp_nodes[ru].size()):
                        node_idx = comp_nodes[ru][j_node]
                        initial_membership[node_idx] = cid
                        join_r[node_idx] = r
                    comp_nodes[ru].clear()
                    
                    comp_cid[ru] = cid
                    
                else:
                    if tracked_cids[ru].size() == 1:
                        cid = tracked_cids[ru][0]
                        n_in = comp_weight[ru]
                        
                        added_weight = 0.0
                        for j_node in range(comp_nodes[ru].size()):
                            node_idx = comp_nodes[ru][j_node]
                            initial_membership[node_idx] = cid
                            join_r[node_idx] = r
                            added_weight += W_nodes[node_idx]
                        comp_nodes[ru].clear()
                        
                        n_in_cluster[cid] += added_weight
                        sum_join_lambda[cid] += added_weight * lam
                        comp_cid[ru] = cid
                        
                    else:
                        new_ch.clear()
                        new_ch = tracked_cids[ru]
                        
                        n_parent = 0.0
                        for c_idx in range(new_ch.size()):
                            cid = new_ch[c_idx]
                            if isnan(death_r[cid]):
                                death_r[cid] = r
                                stability[cid] += sum_join_lambda[cid] - n_in_cluster[cid] * lam
                            n_parent += n_in_cluster[cid]
                        
                        added_weight = 0.0
                        cid_new = children.size()
                        children.push_back(new_ch)
                        birth_r.push_back(r)
                        death_r.push_back(NAN)
                        stability.push_back(0.0)
                        
                        for j_node in range(comp_nodes[ru].size()):
                            node_idx = comp_nodes[ru][j_node]
                            initial_membership[node_idx] = cid_new
                            join_r[node_idx] = r
                            added_weight += W_nodes[node_idx]
                        comp_nodes[ru].clear()
                        
                        n_parent += added_weight
                        size_at_birth.push_back(n_parent)
                        n_in_cluster.push_back(n_parent)
                        sum_join_lambda.push_back(n_parent * lam)
                        
                        comp_cid[ru] = cid_new
            
            tracked_cids[ru].clear()
        i = j

    cdef Py_ssize_t n_clusters = children.size()
    cdef np.ndarray[np.float32_t, ndim=1] lambda_birth_arr = np.empty(n_clusters, dtype=np.float32)
    cdef np.ndarray[np.float32_t, ndim=1] lambda_death_arr = np.empty(n_clusters, dtype=np.float32)
    
    for i in range(n_clusters):
        lambda_birth_arr[i] = 1.0 / (birth_r[i] + EPS)
        if isnan(death_r[i]):
            stability[i] += sum_join_lambda[i]
            lambda_death_arr[i] = 0.0
        else:
            lambda_death_arr[i] = 1.0 / (death_r[i] + EPS)
            
    py_children = []
    for i in range(n_clusters):
        py_children.append(children[i])
        
    return {
        'children': py_children,
        'r': np.asarray(birth_r, dtype=np.float32),
        'stability': np.asarray(stability, dtype=np.float32),
        'initial_membership': initial_membership,
        'join_r': join_r,
        'size': np.asarray(size_at_birth, dtype=np.float32),
        'lambda_birth': lambda_birth_arr,
        'lambda_death': lambda_death_arr,
        'U': np.asarray(U_mst, dtype=np.int32),
        'V': np.asarray(V_mst, dtype=np.int32),
        'W': np.asarray(W_mst, dtype=np.float32),
        'N': int(N),
        'M': int(M),
    }


# -----------------------------------------------------------------------------
# Fast Cython Face-to-Point Label Extraction via Sparse Voting
# -----------------------------------------------------------------------------
def extract_labels_cython(
    const int[:, ::1] faces_unique,
    const int[::1] labels_faces,
    const float[::1] S_faces,
    int n_points,
    int n_clusters
):
    """
    Fast Cython face-to-point label voting. Avoids scipy sparse matrix overheads.
    """
    cdef Py_ssize_t N = faces_unique.shape[0]
    cdef int K = faces_unique.shape[1]
    
    # Pre-allocate accumulator for voting
    cdef np.ndarray[np.float32_t, ndim=2] votes = np.zeros((n_points, n_clusters), dtype=np.float32)
    cdef float[:, ::1] votes_view = votes
    
    cdef Py_ssize_t i
    cdef int j, p, label
    cdef float w
    
    for i in range(N):
        label = labels_faces[i]
        if label == -1:
            continue
        w = S_faces[i]
        for j in range(K):
            p = faces_unique[i, j]
            if 0 <= p < n_points:
                votes_view[p, label] += w
                
    cdef np.ndarray[np.int32_t, ndim=1] final_labels = np.full(n_points, -1, dtype=np.int32)
    cdef int[::1] final_labels_view = final_labels
    
    cdef float max_val
    cdef int best_label
    
    for p in range(n_points):
        max_val = 0.0
        best_label = -1
        for label in range(n_clusters):
            if votes_view[p, label] > max_val:
                max_val = votes_view[p, label]
                best_label = label
        final_labels_view[p] = best_label
        
    return final_labels
