import numpy as np
from itertools import combinations
from scipy.sparse import coo_matrix

def exact_local_gibbs_entropy(nbr_dists_sq, entropy_target, max_iter=100, tol=1e-8):
    """
    Exact Gibbs temperatures and weights on CPU using bisection.
    """
    N, m = nbr_dists_sq.shape
    log_target = np.log(entropy_target)
    log_m = np.log(m)
    effective_target = min(log_target, log_m - 1e-9)

    low = np.full(N, 1e-9)
    high = 100.0 * np.max(nbr_dists_sq, axis=1) + 1e-4

    for _ in range(max_iter):
        mid = 0.5 * (low + high)
        # exponent: -D / mid
        exponent = -nbr_dists_sq / mid[:, None]
        max_exp = np.max(exponent, axis=1, keepdims=True)
        exp_w = np.exp(exponent - max_exp)
        sum_exp = np.sum(exp_w, axis=1, keepdims=True)
        q = exp_w / sum_exp

        # Entropy: H(q) = - sum(q * log_q)
        log_q = exponent - max_exp - np.log(sum_exp)
        entropy = -np.sum(q * log_q, axis=1)

        too_small = entropy < effective_target
        low = np.where(too_small, mid, low)
        high = np.where(too_small, high, mid)

    eta = 0.5 * (low + high)
    exponent = -nbr_dists_sq / eta[:, None]
    max_exp = np.max(exponent, axis=1, keepdims=True)
    exp_w = np.exp(exponent - max_exp)
    Q = exp_w / np.sum(exp_w, axis=1, keepdims=True)

    if log_target >= log_m:
        Q.fill(1.0 / m)
        eta.fill(float('inf'))

    return Q, eta

def exact_regularized_sites(X, entropy_target, shift_cost=True):
    # X: (N, 3)
    N = X.shape[0]
    # Compute full pairwise distances
    diff = X[:, None, :] - X[None, :, :]
    nbr_dists_sq = np.sum(diff ** 2, axis=2) # (N, N)

    if shift_cost:
        # Sort distances to find closest neighbor
        sorted_dists = np.sort(nbr_dists_sq, axis=1)
        rho = np.sqrt(sorted_dists[:, 0:1]) # nearest neighbor distance
        dists = np.sqrt(nbr_dists_sq)
        cost = (np.clip(dists - rho, 0.0, None)) ** 2
    else:
        cost = nbr_dists_sq

    Q, eta = exact_local_gibbs_entropy(cost, entropy_target)

    Z = np.sum(Q[:, :, None] * X[None, :, :], axis=1)

    diff_z = X[None, :, :] - Z[:, None, :]
    dist_to_center = np.sum(diff_z ** 2, axis=2)
    a = np.sum(Q * dist_to_center, axis=1)

    return Z, a

def exact_weighted_miniball_bf(pts, weights):
    c = len(pts)
    best_R2 = float('inf')
    best_p = np.zeros(3)

    for sz in range(1, min(4, c) + 1):
        for indices in combinations(range(c), sz):
            sub_pts = pts[list(indices)]
            sub_w = weights[list(indices)]

            if sz == 1:
                p = sub_pts[0]
                R2 = sub_w[0]
            elif sz == 2:
                v = sub_pts[1] - sub_pts[0]
                d2 = np.sum(v**2)
                if d2 < 1e-12:
                    continue
                t = 0.5 + (sub_w[1] - sub_w[0]) / (2.0 * d2)
                p = sub_pts[0] + t * v
                R2 = t**2 * d2 + sub_w[0]
            elif sz == 3:
                v2 = sub_pts[1] - sub_pts[0]
                v3 = sub_pts[2] - sub_pts[0]
                A = np.zeros((2, 2))
                A[0, 0] = np.sum(v2**2)
                A[0, 1] = np.dot(v2, v3)
                A[1, 0] = A[0, 1]
                A[1, 1] = np.sum(v3**2)

                det = A[0, 0] * A[1, 1] - A[0, 1] * A[1, 0]
                if abs(det) < 1e-12:
                    continue

                rhs = np.zeros(2)
                rhs[0] = 0.5 * A[0, 0] + 0.5 * (sub_w[1] - sub_w[0])
                rhs[1] = 0.5 * A[1, 1] + 0.5 * (sub_w[2] - sub_w[0])

                t2 = (rhs[0] * A[1, 1] - rhs[1] * A[0, 1]) / det
                t3 = (A[0, 0] * rhs[1] - A[1, 0] * rhs[0]) / det

                p = sub_pts[0] + t2 * v2 + t3 * v3
                R2 = np.sum((p - sub_pts[0])**2) + sub_w[0]
            else:
                A = np.zeros((3, 3))
                rhs = np.zeros(3)
                z1 = sub_pts[0]
                w1 = sub_w[0]
                for i in range(1, 4):
                    zi = sub_pts[i]
                    wi = sub_w[i]
                    A[i-1] = 2.0 * (zi - z1)
                    rhs[i-1] = np.sum(zi**2) - np.sum(z1**2) + wi - w1
                try:
                    p = np.linalg.solve(A, rhs)
                    R2 = np.sum((p - z1)**2) + w1
                except np.linalg.LinAlgError:
                    continue

            # Check if this sphere encloses all pts in the subset
            encloses = True
            for i in range(c):
                dist2 = np.sum((p - pts[i])**2) + weights[i]
                if dist2 > R2 + 1e-6:
                    encloses = False
                    break

            if encloses and R2 < best_R2:
                best_R2 = R2
                best_p = p

    return best_p, best_R2

def exact_cech_gabriel_complex(Z, a, K):
    """
    Computes exact Čech/Gabriel complex of order K.
    Returns:
      certified_cofaces: list of tuples of size K+1
      certified_weights: list of floats (radius squared)
    """
    N = Z.shape[0]
    certified_cofaces = []
    certified_weights = []

    # Enumerate all subsets of size K+1
    for subset in combinations(range(N), K + 1):
        pts = Z[list(subset)]
        w = a[list(subset)]

        # 1. Solve weighted miniball
        p, R2 = exact_weighted_miniball_bf(pts, w)

        # 2. Check Gabriel property globally against all other sites in Z
        is_gabriel = True
        for i in range(N):
            if i in subset:
                continue
            dist2 = np.sum((p - Z[i])**2) + a[i]
            if dist2 < R2 - 1e-6:
                is_gabriel = False
                break

        if is_gabriel:
            certified_cofaces.append(sorted(list(subset)))
            certified_weights.append(R2)

    return np.array(certified_cofaces, dtype=np.int64), np.array(certified_weights, dtype=np.float32)

def exact_dual_mst(cofaces, weights, K):
    """
    Builds the dual graph of cofaces and computes exact Kruskal MST.
    """
    num_cofaces = cofaces.shape[0]
    if num_cofaces == 0:
        return np.empty(0), np.empty(0), np.empty(0), np.empty(0), np.empty(0), np.empty((0, K))

    # 1. Compile unique K-facets
    unique_facets_set = set()
    for cof in cofaces:
        for facet in combinations(cof, K):
            unique_facets_set.add(tuple(sorted(facet)))

    unique_facets_list = sorted(list(unique_facets_set))
    facet_to_idx = {f: idx for idx, f in enumerate(unique_facets_list)}
    unique_facets_arr = np.array(unique_facets_list, dtype=np.int64)

    num_facets = len(unique_facets_list)

    # 2. Build dual edges
    edge_u = []
    edge_v = []
    edge_w = []
    edge_coface = []

    for i, cof in enumerate(cofaces):
        facets = [tuple(sorted(f)) for f in combinations(cof, K)]
        for f1, f2 in combinations(facets, 2):
            u = facet_to_idx[f1]
            v = facet_to_idx[f2]
            edge_u.append(u)
            edge_v.append(v)
            edge_w.append(weights[i])
            edge_coface.append(i)

    edge_u = np.array(edge_u, dtype=np.int64)
    edge_v = np.array(edge_v, dtype=np.int64)
    edge_w = np.array(edge_w, dtype=np.float32)
    edge_coface = np.array(edge_coface, dtype=np.int64)

    # 3. Kruskal's MST on dual graph
    parent = np.arange(num_facets, dtype=np.int64)
    def find(i):
        path = []
        while parent[i] != i:
            path.append(i)
            parent[i] = parent[parent[i]]
            i = parent[i]
        for node in path:
            parent[node] = i
        return i

    order = np.argsort(edge_w)
    mst_u = []
    mst_v = []
    mst_w = []
    mst_cof = []

    for idx in order:
        ru = find(edge_u[idx])
        rv = find(edge_v[idx])
        if ru != rv:
            parent[rv] = ru
            mst_u.append(edge_u[idx])
            mst_v.append(edge_v[idx])
            mst_w.append(edge_w[idx])
            mst_cof.append(edge_coface[idx])

    return np.array(mst_u, dtype=np.int64), \
           np.array(mst_v, dtype=np.int64), \
           np.array(mst_w, dtype=np.float32), \
           np.array(mst_cof, dtype=np.int64), \
           num_facets, \
           unique_facets_arr

def exact_condense_tree(W_nodes, u_mst, v_mst, w_mst, min_cluster_size):
    """
    Exact condense tree on CPU.
    """
    N = W_nodes.shape[0]
    M = u_mst.shape[0]

    order = np.argsort(w_mst)
    u_mst = u_mst[order]
    v_mst = v_mst[order]
    w_mst = w_mst[order]

    parent = np.arange(N)
    size = np.ones(N, dtype=np.int32)

    next_label = N
    tree_list = []
    facet_cluster = np.arange(N)

    def find(i):
        path = []
        while parent[i] != i:
            path.append(i)
            parent[i] = parent[parent[i]]
            i = parent[i]
        for node in path:
            parent[node] = i
        return i

    for i in range(M):
        u = u_mst[i]
        v = v_mst[i]
        w = w_mst[i]

        root_u = find(u)
        root_v = find(v)

        if root_u == root_v:
            continue

        r_val = w
        lam = 1.0 / (r_val + 1e-12) if r_val > 0 else 1e12

        size_u = size[root_u]
        size_v = size[root_v]

        c_u = facet_cluster[root_u]
        c_v = facet_cluster[root_v]

        parent[root_v] = root_u
        size[root_u] = size_u + size_v

        if size_u + size_v >= min_cluster_size:
            if c_u < N and c_v < N:
                parent_cluster = next_label
                next_label += 1
                tree_list.append((parent_cluster, c_u, lam, size_u))
                tree_list.append((parent_cluster, c_v, lam, size_v))
                facet_cluster[root_u] = parent_cluster
            elif c_u >= N and c_v < N:
                tree_list.append((c_u, c_v, lam, size_v))
                facet_cluster[root_u] = c_u
            elif c_v >= N and c_u < N:
                tree_list.append((c_v, c_u, lam, size_u))
                facet_cluster[root_u] = c_v
            else:
                parent_cluster = next_label
                next_label += 1
                tree_list.append((parent_cluster, c_u, lam, size_u))
                tree_list.append((parent_cluster, c_v, lam, size_v))
                facet_cluster[root_u] = parent_cluster

    num_clusters = next_label - N
    children = [[] for _ in range(num_clusters)]
    sizes = np.zeros(num_clusters)
    lambda_births = np.zeros(num_clusters)
    parent_tree = np.full(next_label, -1)

    for row in tree_list:
        p, c, lam, sz = row
        parent_tree[c] = p
        if p >= N:
            children[p - N].append(c)
        if c >= N:
            lambda_births[c - N] = lam

    # Compute stabilities
    stabilities = np.zeros(num_clusters)
    for row in tree_list:
        p, c, lam, sz = row
        if p >= N:
            birth = lambda_births[p - N]
            stabilities[p - N] += (lam - birth) * sz

    # Recursive selection
    subtree_stability = np.zeros(num_clusters)
    selected_at_node = {}

    for i in range(num_clusters):
        node_id = N + i
        c_children = children[i]
        self_stability = stabilities[i]

        desc_stability = 0.0
        desc_selected = set()
        for child in c_children:
            if child >= N:
                desc_stability += subtree_stability[child - N]
                desc_selected.update(selected_at_node[child])

        if self_stability >= desc_stability:
            subtree_stability[i] = self_stability
            selected_at_node[node_id] = {node_id}
        else:
            subtree_stability[i] = desc_stability
            selected_at_node[node_id] = desc_selected

    roots = [i for i in range(N, next_label) if parent_tree[i] == -1]
    selected_clusters = set()
    for r in roots:
        if r in selected_at_node:
            selected_clusters.update(selected_at_node[r])

    return {
        'children': children,
        'size': sizes,
        'lambda_birth': lambda_births,
        'N': N,
        'next_label': next_label,
        'parent_tree': parent_tree,
        'tree_list': tree_list,
        'selected_clusters': list(selected_clusters)
    }

def exact_extract_labels(Z_tree, unique_facets, n_points, S_faces, K):
    """
    Exact label voting on CPU.
    """
    N = Z_tree['N']
    next_label = Z_tree['next_label']
    parent_tree = Z_tree['parent_tree']

    selected = Z_tree.get('selected_clusters', [])
    if len(selected) == 0:
        # Fallback to root clusters
        selected = [i for i in range(N, next_label) if parent_tree[i] == -1]

    num_clusters = len(selected)
    if num_clusters == 0:
        return np.full(n_points, -1, dtype=np.int32)

    selected_to_idx = {c: idx for idx, c in enumerate(selected)}
    labels_facets = np.full(N, -1, dtype=np.int32)

    for f in range(N):
        curr = f
        while curr != -1:
            if curr in selected_to_idx:
                labels_facets[f] = selected_to_idx[curr]
                break
            curr = parent_tree[curr]

    labels_expanded = np.repeat(labels_facets, K)
    mask = labels_expanded >= 0
    if not np.any(mask):
        return np.full(n_points, -1, dtype=np.int32)

    flat_facets = unique_facets.flatten()
    rows = flat_facets[mask]
    cols = labels_expanded[mask]

    S_faces_expanded = np.repeat(S_faces, K)
    data = S_faces_expanded[mask]

    mat = coo_matrix((data, (rows, cols)), shape=(n_points, num_clusters), dtype=np.float32).tocsr()
    best_clusters = np.asarray(mat.argmax(axis=1)).flatten()
    has_votes = mat.getnnz(axis=1) > 0

    final_labels = np.full(n_points, -1, dtype=np.int32)
    final_labels[has_votes] = best_clusters[has_votes]
    return final_labels
