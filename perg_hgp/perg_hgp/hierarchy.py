import torch
import numpy as np
from scipy.sparse import csr_matrix, coo_matrix
from scipy.sparse.csgraph import minimum_spanning_tree

def dual_graph_mst(edge_u, edge_v, edge_w, edge_coface, num_facets):
    """
    Computes the MST on the dual graph of facets.
    Uses SciPy's highly optimized sparse minimum spanning tree algorithm.
    """
    # Move to CPU for SciPy
    u = edge_u.cpu().numpy()
    v = edge_v.cpu().numpy()
    w = edge_w.cpu().numpy()
    cof = edge_coface.cpu().numpy()
    
    # Construct sparse matrix
    # Since MST is undirected, we insert both directions.
    # SciPy's MST expects a directed matrix (typically upper triangular or symmetric),
    # and returns a directed MST.
    row = np.concatenate([u, v])
    col = np.concatenate([v, u])
    data = np.concatenate([w, w])
    
    # Store coface index mapping
    cof_mapped = np.concatenate([cof, cof])
    
    # Remove duplicate edges keeping the minimum weight
    # We do this by sorting by weight and building the CSR matrix
    order = np.argsort(data)
    row_s = row[order]
    col_s = col[order]
    data_s = data[order]
    cof_s = cof_mapped[order]
    
    # Build CSR to deduplicate (CSR construction will keep only one edge if duplicates are resolved,
    # but to be safe we can use a dict or simple coo construction)
    # Actually, SciPy's minimum_spanning_tree works fine with duplicate entries in coo
    mat = coo_matrix((data_s, (row_s, col_s)), shape=(num_facets, num_facets))
    
    # Compute MST
    mst_mat = minimum_spanning_tree(mat.tocsr())
    
    # Extract MST edges
    mst_coo = mst_mat.tocoo()
    mst_u = mst_coo.row
    mst_v = mst_coo.col
    mst_w = mst_coo.data
    
    # Map back to cofaces
    # Find which coface produced each MST edge
    # We can do this by matching (u, v) pairs
    # Create unique keys
    edge_to_cof = {}
    for i in range(len(row_s)):
        edge_to_cof[(row_s[i], col_s[i])] = cof_s[i]
        edge_to_cof[(col_s[i], row_s[i])] = cof_s[i]
        
    mst_cof = np.zeros(len(mst_u), dtype=np.int32)
    for i in range(len(mst_u)):
        mst_cof[i] = edge_to_cof[(mst_u[i], mst_v[i])]
        
    return mst_u, mst_v, mst_w, mst_cof


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
