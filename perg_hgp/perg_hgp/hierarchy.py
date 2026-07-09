import torch
import numpy as np
from scipy.sparse import coo_matrix

def dual_graph_mst(edge_u, edge_v, edge_w, edge_coface, num_facets):
    """
    Computes the MST on the dual graph of facets.
    Uses Kruskal's algorithm on edge list to avoid SciPy sparse matrix overhead.
    """
    # Move to CPU
    u = edge_u.cpu().numpy()
    v = edge_v.cpu().numpy()
    w = edge_w.cpu().numpy()
    cof = edge_coface.cpu().numpy()
    
    # Sort edges by weight ascending
    order = np.argsort(w)
    u_sorted = u[order]
    v_sorted = v[order]
    w_sorted = w[order]
    cof_sorted = cof[order]
    
    # Union-Find representing the merging components of facets
    parent = np.arange(num_facets, dtype=np.int32)
    
    def find(i):
        path = []
        while parent[i] != i:
            path.append(i)
            parent[i] = parent[parent[i]]
            i = parent[i]
        for node in path:
            parent[node] = i
        return i
        
    mst_u = []
    mst_v = []
    mst_w = []
    mst_cof = []
    
    num_edges_found = 0
    for i in range(len(w_sorted)):
        ru = find(u_sorted[i])
        rv = find(v_sorted[i])
        if ru != rv:
            parent[rv] = ru
            mst_u.append(u_sorted[i])
            mst_v.append(v_sorted[i])
            mst_w.append(w_sorted[i])
            mst_cof.append(cof_sorted[i])
            num_edges_found += 1
            if num_edges_found == num_facets - 1:
                break
                
    return (np.array(mst_u, dtype=np.int32), 
            np.array(mst_v, dtype=np.int32), 
            np.array(mst_w, dtype=np.float32), 
            np.array(mst_cof, dtype=np.int32))



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
