import numpy as np
from scipy.sparse import coo_matrix

try:
    from ._core import condense_tree_cython, extract_labels_cython
    HAS_CYTHON = True
except ImportError:
    HAS_CYTHON = False
from .miniball import compute_weighted_miniball_batch

def compute_face_weights(faces_unique, Z, a, expZ=2.0):
    """
    Computes node weights for the K-faces based on point density.
    Matches the formula in section 4.4 / HGP-old.
    """
    N, K = faces_unique.shape
    n = Z.shape[0]
    
    # 1. Compute birth radius of each face to get S_faces = 1.0 / (rho ** expZ)
    centers, radii_sq = compute_weighted_miniball_batch(Z, a, faces_unique)
    rho = np.sqrt(np.maximum(radii_sq, 0.0))
    
    with np.errstate(divide='ignore'):
        S_faces = 1.0 / ((rho + 1e-12) ** expZ)
        
    # 2. Compute T_points[p] = sum_{f contains p} S_faces[f]
    flat_faces = faces_unique.flatten()
    S_faces_expanded = np.repeat(S_faces, K)
    T_points = np.bincount(flat_faces, weights=S_faces_expanded, minlength=n).astype(np.float32)
    
    # 3. Compute W_nodes[f] = S_faces[f] * sum_{p in f} (1.0 / T_points[p])
    with np.errstate(divide='ignore', invalid='ignore'):
        inv_T_points = 1.0 / T_points
        inv_T_points[~np.isfinite(inv_T_points)] = 0.0
        
    sum_inv_Tp_face = np.sum(inv_T_points[faces_unique], axis=1)
    W_nodes = S_faces * sum_inv_Tp_face
    
    return W_nodes, S_faces


def _condense_tree_pure(W_nodes, u_mst, v_mst, w_mst, min_cluster_size):
    """
    Pure Python/NumPy implementation of the condensed tree builder.
    Constructs a HDBSCAN-like tree hierarchy from the dual MST.
    """
    N = len(W_nodes)
    M = len(u_mst)
    
    # Sort MST edges by weight ascending
    order = np.argsort(w_mst)
    u_mst = u_mst[order]
    v_mst = v_mst[order]
    w_mst = w_mst[order]
    
    # Union-Find representing the merging components
    parent = np.arange(N)
    size = W_nodes.copy()
    
    # Nodes in the condensed tree
    # Track the current active cluster IDs
    # Leaf nodes are the initial faces: index 0 to N-1
    next_label = N
    
    # Track hierarchy: parent, child, lambda_birth, size
    # We will build a list of rows: (parent, child, lambda, size)
    tree_list = []
    
    # Track which cluster ID each face belongs to
    # Initially each face belongs to its own singleton cluster
    face_cluster = np.arange(N)
    
    def find(i):
        path = []
        while parent[i] != i:
            path.append(i)
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
        
        c_u = face_cluster[root_u]
        c_v = face_cluster[root_v]
        
        # Merge roots in Union-Find
        parent[root_v] = root_u
        size[root_u] = size_u + size_v
        
        if size_u >= min_cluster_size and size_v >= min_cluster_size:
            # Both components are eligible -> create a new parent cluster
            parent_cluster = next_label
            next_label += 1
            
            tree_list.append((parent_cluster, c_u, lam, size_u))
            tree_list.append((parent_cluster, c_v, lam, size_v))
            
            face_cluster[root_u] = parent_cluster
            
        elif size_u >= min_cluster_size:
            # Only component u is eligible -> component v falls out of u
            # The children of v fall out at current lambda
            # We assign all faces in root_v's cluster to c_u
            # Finding all faces in root_v:
            tree_list.append((c_u, c_v, lam, size_v))
            face_cluster[root_u] = c_u
            
        elif size_v >= min_cluster_size:
            # Only component v is eligible -> component u falls out of v
            tree_list.append((c_v, c_u, lam, size_u))
            face_cluster[root_u] = c_v
            
        else:
            # Neither component is eligible -> they simply merge silently
            # The face_cluster of the merged root remains c_u or c_v (doesn't matter)
            pass
            
    # Finalize the root node
    # Find all unique clusters that survived to the end
    surviving_roots = np.unique([face_cluster[find(i)] for i in range(N)])
    
    # We construct the Z dict representation
    # Z contains: children, stability, size, lambda_birth, etc.
    # Total clusters generated is next_label - N
    num_clusters = next_label - N
    
    # Initialize tree arrays
    children = [[] for _ in range(num_clusters)]
    sizes = np.zeros(num_clusters)
    lambda_births = np.zeros(num_clusters)
    lambda_deaths = np.zeros(num_clusters)
    stabilities = np.zeros(num_clusters)
    
    # parent mapping in the condensed tree
    parent_tree = np.full(next_label, -1)
    
    for row in tree_list:
        p, c, lam, sz = row
        parent_tree[c] = p
        if p >= N:
            children[p - N].append(c)
            
    # Compile stabilities and properties
    # stability[c] = sum_{x in c} (lambda_x_death - lambda_x_birth)
    # We can compute this for each node
    for i in range(num_clusters):
        cid = N + i
        sizes[i] = size_node(cid, N, children, W_nodes)
        
        # lambda_birth is the lambda where the cluster was created
        # Find the row where this cluster was a child
        row_birth = [r for r in tree_list if r[1] == cid]
        if row_birth:
            lambda_births[i] = row_birth[0][2]
        else:
            lambda_births[i] = 1e12 # Root of the whole tree
            
        # lambda_death is when it splits or fades
        # For simplicity, stability is computed as:
        # Sum of (lam_death - lam_birth) for all elements that fall out of this node
        # or split from this node.
        
    # Return Z representation
    return {
        'children': children,
        'size': sizes,
        'lambda_birth': lambda_births,
        'N': N,
        'next_label': next_label,
        'parent_tree': parent_tree,
        'tree_list': tree_list
    }


def condense_tree(W_nodes, u_mst, v_mst, w_mst, min_cluster_size):
    """
    Public wrapper that uses the fast Cython condensed tree builder if available.
    """
    if HAS_CYTHON:
        sort_idx = np.argsort(w_mst)
        u_mst_sorted = u_mst[sort_idx]
        v_mst_sorted = v_mst[sort_idx]
        w_mst_sorted = w_mst[sort_idx]
        
        W_nodes_32 = np.ascontiguousarray(W_nodes, dtype=np.float32)
        U_mst_32 = np.ascontiguousarray(u_mst_sorted, dtype=np.int32)
        V_mst_32 = np.ascontiguousarray(v_mst_sorted, dtype=np.int32)
        W_mst_32 = np.ascontiguousarray(w_mst_sorted, dtype=np.float32)
        
        Z = condense_tree_cython(W_nodes_32, U_mst_32, V_mst_32, W_mst_32, min_cluster_size)
        
        N = Z['N']
        children = Z['children']
        num_clusters = len(children)
        next_label = N + num_clusters
        
        parent_tree = np.full(next_label, -1, dtype=np.int32)
        for i in range(num_clusters):
            p = N + i
            for c in children[i]:
                parent_tree[N + c] = p
                
        # Set parent for leaf faces using initial_membership
        initial_mem = Z['initial_membership']
        for f in range(N):
            if initial_mem[f] != -1:
                parent_tree[f] = N + initial_mem[f]
                
        Z['next_label'] = next_label
        Z['parent_tree'] = parent_tree
        return Z
        
    return _condense_tree_pure(W_nodes, u_mst, v_mst, w_mst, min_cluster_size)


def size_node(cid, N, children, W_nodes):
    if cid < N:
        return W_nodes[cid]
    # Recursive sum of children sizes
    return sum(size_node(c, N, children, W_nodes) for c in children[cid - N])


def extract_labels(Z_tree, faces_unique, n_points, S_faces):
    """
    Extracts point labels by voting from the condensed tree.
    """
    N = Z_tree['N']
    next_label = Z_tree['next_label']
    parent_tree = Z_tree['parent_tree']
    
    # We assign cluster IDs to faces by walking up the parent_tree
    # to find if their ancestor is one of the selected roots.
    roots = [i for i in range(N, next_label) if parent_tree[i] == -1]
    labels_faces = np.full(N, -1, dtype=np.int32)
    root_to_idx = {root: idx for idx, root in enumerate(roots)}
    
    for f in range(N):
        curr = f
        while curr != -1:
            if curr in root_to_idx:
                labels_faces[f] = root_to_idx[curr]
                break
            curr = parent_tree[curr]
        
    num_clusters = len(roots)
    if num_clusters == 0:
        return np.full(n_points, -1, dtype=np.int32)
        
    if HAS_CYTHON:
        faces_unique_32 = np.ascontiguousarray(faces_unique, dtype=np.int32)
        labels_faces_32 = np.ascontiguousarray(labels_faces, dtype=np.int32)
        S_faces_32 = np.ascontiguousarray(S_faces, dtype=np.float32)
        return extract_labels_cython(faces_unique_32, labels_faces_32, S_faces_32, n_points, num_clusters)
        
    # Propagate Face labels to Points using sparse voting (exactly like HGP-old)
    # Each face tau votes for its vertices with weight S_face[tau]
    K = faces_unique.shape[1]
    labels_expanded = np.repeat(labels_faces, K)
    S_faces_expanded = np.repeat(S_faces, K)
    flat_faces = faces_unique.flatten()
    
    mask = labels_expanded >= 0
    rows = flat_faces[mask]
    cols = labels_expanded[mask]
    data = S_faces_expanded[mask]
    
    mat = coo_matrix((data, (rows, cols)), shape=(n_points, num_clusters), dtype=np.float32).tocsr()
    
    best_clusters = np.asarray(mat.argmax(axis=1)).flatten()
    has_votes = mat.getnnz(axis=1) > 0
    
    final_labels = np.full(n_points, -1, dtype=np.int32)
    final_labels[has_votes] = best_clusters[has_votes]
    
    return final_labels


def get_descendants(node, N, children):
    if node < N:
        return [node]
    res = []
    for c in children[node - N]:
        res.extend(get_descendants(c, N, children))
    return res
