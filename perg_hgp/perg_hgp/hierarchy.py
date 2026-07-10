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



def _eom_select(children, stability):
    """Select a disjoint set of clusters by excess of mass."""
    n_clusters = len(children)
    if n_clusters == 0:
        return [], np.empty(0, dtype=np.int32)

    parent_cluster = np.full(n_clusters, -1, dtype=np.int32)
    best_stability = np.asarray(stability, dtype=np.float64).copy()
    keep_self = np.ones(n_clusters, dtype=bool)
    for parent_id, child_ids in enumerate(children):
        for child_id in child_ids:
            parent_cluster[child_id] = parent_id
        descendant_stability = sum(best_stability[child_id] for child_id in child_ids)
        if descendant_stability > best_stability[parent_id]:
            best_stability[parent_id] = descendant_stability
            keep_self[parent_id] = False

    selected = []
    stack = np.where(parent_cluster == -1)[0].tolist()
    while stack:
        cluster_id = stack.pop()
        if keep_self[cluster_id] or not children[cluster_id]:
            selected.append(cluster_id)
        else:
            stack.extend(children[cluster_id])
    return sorted(selected), parent_cluster


def condense_tree(W_nodes, u_mst, v_mst, w_mst, min_cluster_size, merge_tolerance=0.0):
    """Build a weighted, multiway condensed tree from a minimum spanning forest.

    ``W_nodes`` are normalized facet masses. Edges with equal weights are handled
    as one multiway event, and facets that first cross ``min_cluster_size`` after
    several sub-threshold merges are retained through ``initial_membership``.
    """
    W_nodes = np.asarray(W_nodes, dtype=np.float64)
    u_mst = np.asarray(u_mst, dtype=np.int64)
    v_mst = np.asarray(v_mst, dtype=np.int64)
    w_mst = np.asarray(w_mst, dtype=np.float64)
    N = W_nodes.shape[0]
    M = w_mst.shape[0]

    if u_mst.shape != (M,) or v_mst.shape != (M,):
        raise ValueError("u_mst, v_mst and w_mst must have the same length")
    if np.any(~np.isfinite(W_nodes)) or np.any(W_nodes < 0):
        raise ValueError("W_nodes must be finite and non-negative")
    if np.any(~np.isfinite(w_mst)) or np.any(w_mst < 0):
        raise ValueError("MST weights must be finite and non-negative")
    if M and (np.min(u_mst) < 0 or np.min(v_mst) < 0 or np.max(u_mst) >= N or np.max(v_mst) >= N):
        raise ValueError("MST endpoint outside the facet range")
    if min_cluster_size <= 0:
        raise ValueError("min_cluster_size must be positive")

    order = np.argsort(w_mst, kind='stable')
    u_mst = u_mst[order]
    v_mst = v_mst[order]
    w_mst = w_mst[order]

    parent = np.arange(N, dtype=np.int64)
    component_weight = W_nodes.copy()
    component_cluster = np.full(N, -1, dtype=np.int64)

    # Linked lists contain only facets that have not entered any eligible cluster.
    head = np.arange(N, dtype=np.int64)
    tail = np.arange(N, dtype=np.int64)
    next_node = np.full(N, -1, dtype=np.int64)

    children = []
    birth_r = []
    death_r = []
    stability = []
    size_at_birth = []
    mass_in_cluster = []
    sum_join_lambda = []
    initial_membership = np.full(N, -1, dtype=np.int32)
    join_r = np.full(N, np.inf, dtype=np.float32)

    def find(node):
        root = node
        while parent[root] != root:
            root = parent[root]
        while parent[node] != node:
            nxt = parent[node]
            parent[node] = root
            node = nxt
        return root

    def assign_unclustered(root, cluster_id, radius):
        added_mass = 0.0
        node = head[root]
        while node != -1:
            initial_membership[node] = cluster_id
            join_r[node] = radius
            added_mass += W_nodes[node]
            node = next_node[node]
        head[root] = -1
        tail[root] = -1
        return added_mass

    edge_index = 0
    eps = 1e-12
    while edge_index < M:
        batch_end = edge_index + 1
        batch_start_weight = w_mst[edge_index]
        while batch_end < M and w_mst[batch_end] - batch_start_weight <= merge_tolerance:
            batch_end += 1

        radius = w_mst[batch_end - 1]
        lam = 1.0 / (radius + eps) if radius > 0 else 1e12
        tracked_clusters = {}
        touched_roots = set()

        for pos in range(edge_index, batch_end):
            root_u = find(int(u_mst[pos]))
            root_v = find(int(v_mst[pos]))
            if root_u == root_v:
                continue

            for root in (root_u, root_v):
                cluster_id = component_cluster[root]
                if cluster_id != -1:
                    tracked_clusters.setdefault(root, []).append(int(cluster_id))
                    component_cluster[root] = -1

            if (component_weight[root_u] < component_weight[root_v] or
                    (component_weight[root_u] == component_weight[root_v] and root_u > root_v)):
                root_u, root_v = root_v, root_u

            clusters_u = tracked_clusters.pop(root_u, [])
            clusters_v = tracked_clusters.pop(root_v, [])
            parent[root_v] = root_u
            component_weight[root_u] += component_weight[root_v]

            if head[root_v] != -1:
                if head[root_u] == -1:
                    head[root_u], tail[root_u] = head[root_v], tail[root_v]
                else:
                    next_node[tail[root_u]] = head[root_v]
                    tail[root_u] = tail[root_v]
                head[root_v] = tail[root_v] = -1

            merged_clusters = list(dict.fromkeys(clusters_u + clusters_v))
            if merged_clusters:
                tracked_clusters[root_u] = merged_clusters
            touched_roots.add(root_u)

        final_roots = sorted({find(root) for root in touched_roots})
        for root in final_roots:
            child_clusters = tracked_clusters.pop(root, [])
            if component_weight[root] < min_cluster_size:
                if child_clusters:
                    raise RuntimeError("eligible cluster became sub-threshold after a union")
                continue

            if not child_clusters:
                cluster_id = len(children)
                children.append([])
                birth_r.append(radius)
                death_r.append(np.nan)
                stability.append(0.0)
                size_at_birth.append(component_weight[root])
                mass_in_cluster.append(component_weight[root])
                sum_join_lambda.append(component_weight[root] * lam)
                assign_unclustered(root, cluster_id, radius)
                component_cluster[root] = cluster_id
            elif len(child_clusters) == 1:
                cluster_id = child_clusters[0]
                added_mass = assign_unclustered(root, cluster_id, radius)
                mass_in_cluster[cluster_id] += added_mass
                sum_join_lambda[cluster_id] += added_mass * lam
                component_cluster[root] = cluster_id
            else:
                child_clusters = sorted(set(child_clusters))
                for child_id in child_clusters:
                    if np.isnan(death_r[child_id]):
                        death_r[child_id] = radius
                        stability[child_id] += (
                            sum_join_lambda[child_id] - mass_in_cluster[child_id] * lam
                        )

                cluster_id = len(children)
                children.append(child_clusters)
                birth_r.append(radius)
                death_r.append(np.nan)
                stability.append(0.0)
                added_mass = assign_unclustered(root, cluster_id, radius)
                parent_mass = sum(mass_in_cluster[child_id] for child_id in child_clusters) + added_mass
                size_at_birth.append(parent_mass)
                mass_in_cluster.append(parent_mass)
                sum_join_lambda.append(parent_mass * lam)
                component_cluster[root] = cluster_id

        edge_index = batch_end

    for cluster_id in range(len(children)):
        if np.isnan(death_r[cluster_id]):
            stability[cluster_id] += sum_join_lambda[cluster_id]

    birth_r = np.asarray(birth_r, dtype=np.float32)
    death_r_arr = np.asarray(death_r, dtype=np.float32)
    stability_arr = np.asarray(stability, dtype=np.float64)
    lambda_birth = np.divide(1.0, birth_r + eps, out=np.full_like(birth_r, 1e12), where=birth_r > 0)
    lambda_death = np.zeros_like(death_r_arr)
    finite_death = np.isfinite(death_r_arr)
    lambda_death[finite_death] = 1.0 / (death_r_arr[finite_death] + eps)
    selected_clusters, parent_cluster = _eom_select(children, stability_arr)

    return {
        'children': children,
        'r': birth_r,
        'stability': stability_arr,
        'initial_membership': initial_membership,
        'join_r': join_r,
        'size': np.asarray(size_at_birth, dtype=np.float64),
        'lambda_birth': lambda_birth,
        'lambda_death': lambda_death,
        'parent_cluster': parent_cluster,
        'selected_clusters': selected_clusters,
        'N': N,
        'M': M,
        'U': u_mst.astype(np.int32, copy=False),
        'V': v_mst.astype(np.int32, copy=False),
        'W': w_mst.astype(np.float32, copy=False),
    }


def extract_labels(Z_tree, unique_facets, n_points, facet_birth_weights):
    """
    Extracts point labels by voting from the condensed tree.
    """
    N = Z_tree['N']
    selected = Z_tree.get('selected_clusters', [])
    num_clusters = len(selected)
    if num_clusters == 0:
        return np.full(n_points, -1, dtype=np.int32)

    selected_to_idx = {cluster_id: idx for idx, cluster_id in enumerate(selected)}
    labels_facets = np.full(N, -1, dtype=np.int32)

    if 'initial_membership' in Z_tree:
        initial_membership = Z_tree['initial_membership']
        parent_cluster = Z_tree['parent_cluster']
        for facet_id in range(N):
            cluster_id = int(initial_membership[facet_id])
            while cluster_id != -1:
                if cluster_id in selected_to_idx:
                    labels_facets[facet_id] = selected_to_idx[cluster_id]
                    break
                cluster_id = int(parent_cluster[cluster_id])
    else:
        # Backward-compatible reader for pre-audit tree checkpoints.
        parent_tree = Z_tree['parent_tree']
        for facet_id in range(N):
            node_id = facet_id
            while node_id != -1:
                if node_id in selected_to_idx:
                    labels_facets[facet_id] = selected_to_idx[node_id]
                    break
                node_id = parent_tree[node_id]

    # Voting from facets to points
    K = unique_facets.shape[1]
    labels_expanded = np.repeat(labels_facets, K)
    S_facets_expanded = np.repeat(facet_birth_weights, K)
    flat_facets = unique_facets.flatten()

    mask = labels_expanded >= 0
    if not np.any(mask):
        return np.full(n_points, -1, dtype=np.int32)

    rows = flat_facets[mask]
    cols = labels_expanded[mask]
    data = S_facets_expanded[mask]

    mat = coo_matrix((data, (rows, cols)), shape=(n_points, num_clusters), dtype=np.float32).tocsr()

    best_clusters = np.asarray(mat.argmax(axis=1)).flatten()
    has_votes = mat.getnnz(axis=1) > 0

    final_labels = np.full(n_points, -1, dtype=np.int32)
    final_labels[has_votes] = best_clusters[has_votes]

    return final_labels
