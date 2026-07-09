import torch
import numpy as np
from scipy.sparse import coo_matrix

def dual_graph_mst(edge_u, edge_v, edge_w, edge_coface, num_facets):
    """
    Computes the MST on the dual graph of facets using a vectorized Boruvka algorithm in PyTorch.
    Supports both GPU (CUDA) and CPU.
    """
    device = edge_u.device
    num_edges = edge_u.shape[0]
    
    if num_edges == 0:
        return (np.empty(0, dtype=np.int32),
                np.empty(0, dtype=np.int32),
                np.empty(0, dtype=np.float32),
                np.empty(0, dtype=np.int32))
                
    # Union-Find parent pointers on device
    parent = torch.arange(num_facets, device=device)
    
    mst_edges_indices = []
    
    # We loop for at most ceil(log2(num_facets)) phases.
    for phase in range(25):
        # 1. Pointer jumping to find representative roots for all components
        while True:
            p = parent[parent]
            if torch.equal(p, parent):
                break
            parent = p
            
        # 2. Find roots of endpoints for all edges
        ru = parent[edge_u]
        rv = parent[edge_v]
        
        # Filter out self-loops (edges within the same component)
        active_edges_mask = ru != rv
        if not active_edges_mask.any():
            break
            
        # 3. For each active edge, we pack its weight and its index to select deterministically
        w_min = edge_w.min()
        shift = 0.0
        if w_min < 0:
            shift = -w_min + 1.0
        w_shifted = edge_w + shift
        
        w_int = w_shifted.view(torch.int32).to(torch.int64)
        edge_idx = torch.arange(num_edges, device=device)
        
        packed = (w_int << 32) | edge_idx
        
        # 4. Find the cheapest edge for each component using scatter_reduce
        cheapest = torch.full((num_facets,), torch.iinfo(torch.int64).max, dtype=torch.int64, device=device)
        
        active_idx = torch.where(active_edges_mask)[0]
        if active_idx.shape[0] == 0:
            break
            
        cheapest.scatter_reduce_(0, ru[active_idx], packed[active_idx], 'amin', include_self=False)
        cheapest.scatter_reduce_(0, rv[active_idx], packed[active_idx], 'amin', include_self=True)
        
        # 5. Add cheapest edges to the MST and merge components
        valid_mask = cheapest < torch.iinfo(torch.int64).max
        valid_roots = torch.where(valid_mask)[0]
        
        if valid_roots.shape[0] == 0:
            break
            
        cheapest_edges_idx = cheapest[valid_roots] & 0xFFFFFFFF
        unique_edges_idx = torch.unique(cheapest_edges_idx)
        
        # Union-Find union steps on CPU to avoid parallel write conflicts
        mst_u_cpu = edge_u[unique_edges_idx].cpu().numpy()
        mst_v_cpu = edge_v[unique_edges_idx].cpu().numpy()
        
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
                
        parent = torch.from_numpy(parent_cpu).to(device)
        if len(merged_edges_idx) > 0:
            mst_edges_indices.append(torch.tensor(merged_edges_idx, dtype=torch.int64, device=device))
        
    if len(mst_edges_indices) == 0:
        return (np.empty(0, dtype=np.int32),
                np.empty(0, dtype=np.int32),
                np.empty(0, dtype=np.float32),
                np.empty(0, dtype=np.int32))
                
    final_edges_idx = torch.cat(mst_edges_indices)
    
    # Run a clean step to filter out any potential cycle or redundant edges
    u_mst = edge_u[final_edges_idx].cpu().numpy()
    v_mst = edge_v[final_edges_idx].cpu().numpy()
    w_mst = edge_w[final_edges_idx].cpu().numpy()
    cof_mst = edge_coface[final_edges_idx].cpu().numpy()
    
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
