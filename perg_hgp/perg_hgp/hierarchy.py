import torch
import numpy as np
from scipy.sparse import coo_matrix

def dual_graph_mst(edge_u, edge_v=None, edge_w=None, edge_coface=None, num_facets=None):
    """
    Computes the MST on the dual graph of facets using a vectorized Boruvka algorithm in PyTorch.
    Supports both GPU (CUDA) and CPU.
    Accepts either:
      1. dual_graph_mst(edge_generator, num_facets) where edge_generator is a callable returning chunks
      2. dual_graph_mst(edge_u, edge_v, edge_w, edge_coface, num_facets) where arguments are tensors
    """
    # 1. Determine mode and wrap accordingly
    if callable(edge_u):
        get_chunks = edge_u
        num_facets = edge_v  # Second argument is num_facets in callable mode
        device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    else:
        # Tensor mode
        device = edge_u.device
        num_edges = edge_u.shape[0]
        if num_edges == 0:
            return (np.empty(0, dtype=np.int32),
                    np.empty(0, dtype=np.int32),
                    np.empty(0, dtype=np.float32),
                    np.empty(0, dtype=np.int32))

        is_cuda = (device.type == 'cuda')
        edge_u_cpu = edge_u.cpu() if is_cuda else edge_u
        edge_v_cpu = edge_v.cpu() if is_cuda else edge_v
        edge_w_cpu = edge_w.cpu() if is_cuda else edge_w
        edge_coface_cpu = edge_coface.cpu() if is_cuda else edge_coface

        def get_chunks():
            edge_chunk_size = 5000000
            for start in range(0, num_edges, edge_chunk_size):
                end = min(start + edge_chunk_size, num_edges)
                yield edge_u_cpu[start:end], edge_v_cpu[start:end], edge_w_cpu[start:end], edge_coface_cpu[start:end]

    # Union-Find parent pointers on device
    parent = torch.arange(num_facets, device=device)

    mst_edges_indices = []

    # We loop for at most ceil(log2(num_facets)) phases.
    max_phases = max(25, int(np.ceil(np.log2(max(2, num_facets)))) + 2)
    for phase in range(max_phases):
        # 1. Pointer jumping to find representative roots for all components
        while True:
            p = parent[parent]
            if torch.equal(p, parent):
                break
            parent = p

        # 2. Check if there are active edges and compute cheapest_w
        cheapest_w = torch.full((num_facets,), float('inf'), dtype=torch.float32, device=device)
        has_active = False

        for u_c_cpu, v_c_cpu, w_c_cpu, _ in get_chunks():
            u_c = u_c_cpu.to(device)
            v_c = v_c_cpu.to(device)
            w_c = w_c_cpu.to(device)

            ru_c = parent[u_c]
            rv_c = parent[v_c]
            mask_c = ru_c != rv_c

            if mask_c.any():
                has_active = True
                cheapest_w.scatter_reduce_(0, ru_c[mask_c], w_c[mask_c], 'amin', include_self=True)
                cheapest_w.scatter_reduce_(0, rv_c[mask_c], w_c[mask_c], 'amin', include_self=True)

        if not has_active:
            break

        # 3. Tie-breaker pass to compute cheapest_idx
        cheapest_idx = torch.full((num_facets,), 2**60, dtype=torch.int64, device=device)

        edge_offset = 0
        for u_c_cpu, v_c_cpu, w_c_cpu, _ in get_chunks():
            chunk_num_edges = u_c_cpu.shape[0]

            u_c = u_c_cpu.to(device)
            v_c = v_c_cpu.to(device)
            w_c = w_c_cpu.to(device)

            ru_c = parent[u_c]
            rv_c = parent[v_c]
            mask_c = ru_c != rv_c

            if mask_c.any():
                active_idx_c = torch.where(mask_c)[0] + edge_offset

                # Tie-breaker for ru
                match_ru = (w_c[mask_c] == cheapest_w[ru_c[mask_c]])
                if match_ru.any():
                    cheapest_idx.scatter_reduce_(0, ru_c[mask_c][match_ru], active_idx_c[match_ru], 'amin', include_self=True)

                # Tie-breaker for rv
                match_rv = (w_c[mask_c] == cheapest_w[rv_c[mask_c]])
                if match_rv.any():
                    cheapest_idx.scatter_reduce_(0, rv_c[mask_c][match_rv], active_idx_c[match_rv], 'amin', include_self=True)

            edge_offset += chunk_num_edges

        # 4. Add cheapest edges to the MST and merge components
        valid_mask = cheapest_idx < 2**60
        valid_roots = torch.where(valid_mask)[0]

        if valid_roots.shape[0] == 0:
            break

        cheapest_edges_idx = cheapest_idx[valid_roots]
        unique_edges_idx = torch.unique(cheapest_edges_idx)

        # Retrieve endpoints of unique_edges_idx in CPU NumPy format
        mst_u_list = []
        mst_v_list = []

        edge_offset = 0
        unique_edges_idx_cpu = unique_edges_idx.cpu()
        for u_c_cpu, v_c_cpu, w_c_cpu, _ in get_chunks():
            chunk_num_edges = u_c_cpu.shape[0]
            in_chunk_mask = (unique_edges_idx_cpu >= edge_offset) & (unique_edges_idx_cpu < edge_offset + chunk_num_edges)
            if in_chunk_mask.any():
                chunk_indices = unique_edges_idx_cpu[in_chunk_mask] - edge_offset
                mst_u_list.append(u_c_cpu[chunk_indices])
                mst_v_list.append(v_c_cpu[chunk_indices])
            edge_offset += chunk_num_edges

        mst_u_cpu = torch.cat(mst_u_list).numpy()
        mst_v_cpu = torch.cat(mst_v_list).numpy()

        parent_cpu = parent.cpu().numpy()

        def find_cpu(i):
            path = []
            while parent_cpu[i] != i:
                path.append(i)
                parent_cpu[i] = parent_cpu[parent_cpu[i]]
                i = parent_cpu[i]
            for node in path:
                parent_cpu[node] = i
            return i

        merged_edges_idx = []
        for idx_idx, edge_idx_val in enumerate(unique_edges_idx.cpu().numpy()):
            u_val = mst_u_cpu[idx_idx]
            v_val = mst_v_cpu[idx_idx]
            ru_val = find_cpu(u_val)
            rv_val = find_cpu(v_val)
            if ru_val != rv_val:
                parent_cpu[ru_val] = rv_val
                merged_edges_idx.append(edge_idx_val)

        if len(merged_edges_idx) == 0:
            break

        mst_edges_indices.append(torch.tensor(merged_edges_idx, dtype=torch.int64, device=device))
        parent = torch.from_numpy(parent_cpu).to(device)

    if len(mst_edges_indices) == 0:
        return (np.empty(0, dtype=np.int32),
                np.empty(0, dtype=np.int32),
                np.empty(0, dtype=np.float32),
                np.empty(0, dtype=np.int32))

    final_edges_idx = torch.cat(mst_edges_indices)

    # Retrieve u, v, w, coface details for final_edges_idx
    mst_u_list = []
    mst_v_list = []
    mst_w_list = []
    mst_cof_list = []

    edge_offset = 0
    final_edges_idx_cpu = final_edges_idx.cpu()
    for u_c_cpu, v_c_cpu, w_c_cpu, coface_c_cpu in get_chunks():
        chunk_num_edges = u_c_cpu.shape[0]
        in_chunk_mask = (final_edges_idx_cpu >= edge_offset) & (final_edges_idx_cpu < edge_offset + chunk_num_edges)
        if in_chunk_mask.any():
            chunk_indices = final_edges_idx_cpu[in_chunk_mask] - edge_offset
            mst_u_list.append(u_c_cpu[chunk_indices])
            mst_v_list.append(v_c_cpu[chunk_indices])
            mst_w_list.append(w_c_cpu[chunk_indices])
            mst_cof_list.append(coface_c_cpu[chunk_indices])
        edge_offset += chunk_num_edges

    u_mst = torch.cat(mst_u_list).numpy()
    v_mst = torch.cat(mst_v_list).numpy()
    w_mst = torch.cat(mst_w_list).numpy()
    cof_mst = torch.cat(mst_cof_list).numpy()

    order = np.argsort(w_mst)
    u_sorted = u_mst[order]
    v_sorted = v_mst[order]
    w_sorted = w_mst[order]
    cof_sorted = cof_mst[order]

    clean_parent = np.arange(num_facets, dtype=np.int32)
    def clean_find(i):
        path = []
        while clean_parent[i] != i:
            path.append(i)
            clean_parent[i] = clean_parent[clean_parent[i]]
            i = clean_parent[i]
        for node in path:
            clean_parent[node] = i
        return i

    mst_u_out = []
    mst_v_out = []
    mst_w_out = []
    mst_cof_out = []

    for i in range(len(w_sorted)):
        ru_cl = clean_find(u_sorted[i])
        rv_cl = clean_find(v_sorted[i])
        if ru_cl != rv_cl:
            clean_parent[rv_cl] = ru_cl
            mst_u_out.append(u_sorted[i])
            mst_v_out.append(v_sorted[i])
            mst_w_out.append(w_sorted[i])
            mst_cof_out.append(cof_sorted[i])

    return (np.array(mst_u_out, dtype=np.int32),
            np.array(mst_v_out, dtype=np.int32),
            np.array(mst_w_out, dtype=np.float32),
            np.array(mst_cof_out, dtype=np.int32))



def condense_tree(facet_birth_weights, u_mst, v_mst, w_mst, min_cluster_size):
    """
    Constructs the HDBSCAN-like condensed tree hierarchy.
    """
    N = len(facet_birth_weights)
    M = len(u_mst)

    # Sort MST edges by weight ascending
    order = np.argsort(w_mst)
    u_mst = u_mst[order]
    v_mst = v_mst[order]
    w_mst = w_mst[order]

    # Union-Find representing the merging components
    parent = np.arange(N)
    size = facet_birth_weights.copy()

    # Nodes in the condensed tree
    next_label = N

    # Track hierarchy: parent, child, lambda_birth, size
    tree_list = []

    # Track which cluster ID each facet belongs to
    facet_cluster = np.arange(N)

    def find(i):
        path = []
        while parent[i] != i:
            path.append(i)
            parent[i] = parent[parent[i]] # path compression
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

        # Merge roots in Union-Find
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

    # Finalize the root node
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

    return {
        'children': children,
        'size': sizes,
        'lambda_birth': lambda_births,
        'N': N,
        'next_label': next_label,
        'parent_tree': parent_tree,
        'tree_list': tree_list
    }


def extract_labels(Z_tree, unique_facets, n_points, facet_birth_weights):
    """
    Extracts point labels by voting from the condensed tree.
    """
    N = Z_tree['N']
    next_label = Z_tree['next_label']
    parent_tree = Z_tree['parent_tree']

    roots = [i for i in range(N, next_label) if parent_tree[i] == -1]
    labels_facets = np.full(N, -1, dtype=np.int32)
    root_to_idx = {root: idx for idx, root in enumerate(roots)}

    for f in range(N):
        curr = f
        while curr != -1:
            if curr in root_to_idx:
                labels_facets[f] = root_to_idx[curr]
                break
            curr = parent_tree[curr]

    num_clusters = len(roots)
    if num_clusters == 0:
        return np.full(n_points, -1, dtype=np.int32)

    # Voting from facets to points
    # Each facet unique_facets[f] (length K) votes for its vertices with weight facet_birth_weights[f]
    K = unique_facets.shape[1]
    labels_expanded = np.repeat(labels_facets, K)
    S_facets_expanded = np.repeat(facet_birth_weights, K)
    flat_facets = unique_facets.flatten()

    mask = labels_expanded >= 0
    rows = flat_facets[mask]
    cols = labels_expanded[mask]
    data = S_facets_expanded[mask]

    mat = coo_matrix((data, (rows, cols)), shape=(n_points, num_clusters), dtype=np.float32).tocsr()

    best_clusters = np.asarray(mat.argmax(axis=1)).flatten()
    has_votes = mat.getnnz(axis=1) > 0

    final_labels = np.full(n_points, -1, dtype=np.int32)
    final_labels[has_votes] = best_clusters[has_votes]

    return final_labels
