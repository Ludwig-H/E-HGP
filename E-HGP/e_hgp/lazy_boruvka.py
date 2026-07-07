import numpy as np
from scipy.spatial import KDTree
from .miniball import compute_weighted_miniball_batch
from .gabriel import gabriel_global_test_batch

class UnionFind:
    def __init__(self):
        self.parent = {}
        self.rank = {}
        
    def add(self, x):
        if x not in self.parent:
            self.parent[x] = x
            self.rank[x] = 0
            
    def find(self, i):
        self.add(i)
        if self.parent[i] == i:
            return i
        self.parent[i] = self.find(self.parent[i])
        return self.parent[i]
        
    def union(self, i, j):
        root_i = self.find(i)
        root_j = self.find(j)
        if root_i != root_j:
            if self.rank[root_i] < self.rank[root_j]:
                self.parent[root_i] = root_j
            elif self.rank[root_i] > self.rank[root_j]:
                self.parent[root_j] = root_i
            else:
                self.parent[root_j] = root_i
                self.rank[root_i] += 1
            return True
        return False


def compute_pairwise_beta(Z, a, L_max=150):
    r"""
    Computes the localized pairwise birth radius beta_ij using KDTree.
    This avoids O(N^2) memory footprint and scales to millions of points.
    """
    n, p = Z.shape
    L_max = min(n, L_max)
    
    # 1. Build KDTree and query L_max nearest neighbors
    tree = KDTree(Z)
    dists, indices = tree.query(Z, k=L_max, workers=-1)
    
    # 2. Compute D2, D, and a weights for local neighbors
    D2 = dists ** 2
    D = dists
    
    a_j = a[indices]
    a_i = a[:, np.newaxis]
    
    # 3. Compute u_ij
    denom = 2.0 * D
    mask_zero = D < 1e-12
    denom[mask_zero] = 1.0
    
    u = (D2 + a_j - a_i) / denom
    u = np.clip(u, 0.0, D)
    u[mask_zero] = 0.0
    
    # 4. beta_ij^2 = max(u_ij^2 + a_i, (D_ij - u_ij)^2 + a_j)
    term1 = u**2 + a_i
    term2 = (D - u)**2 + a_j
    beta2 = np.maximum(term1, term2)
    beta_local = np.sqrt(beta2)
    
    # 5. Sort local neighbors by beta_local for each site
    sort_local = np.argsort(beta_local, axis=1)
    
    # Construct output arrays
    sorted_neighbors = np.empty((n, L_max), dtype=np.int32)
    beta_sorted = np.empty((n, L_max), dtype=np.float64)
    
    # Fast row-wise gather
    for i in range(n):
        idx = sort_local[i]
        sorted_neighbors[i] = indices[i, idx]
        beta_sorted[i] = beta_local[i, idx]
        
    return beta_sorted, sorted_neighbors


def lazy_cut_boruvka(Z, a, K, L_initial=30, max_iter=20, tol=1e-6):
    """
    Lazy Cut-Borůvka algorithm for finding the certified K-MST.
    
    Parameters
    ----------
    Z : np.ndarray
        Regularized site centers, shape (n, p).
    a : np.ndarray
        Power weights, shape (n,).
    K : int
        Size of the faces (K-subsets).
    L_initial : int
        Initial size of prefix lists.
    max_iter : int
        Maximum number of Borůvka passes.
    tol : float
        Numerical tolerance.
        
    Returns
    -------
    mst_edges : list
        List of MST edges as tuples: (tau_a_tuple, tau_b_tuple, weight, coface_tuple)
    exactness_report : dict
        Report indicating completeness and counts of tests.
    """
    n, p = Z.shape
    
    # 1. Build beta lists locally
    L_max = max(150, L_initial + 50)
    beta, sorted_neighbors = compute_pairwise_beta(Z, a, L_max=L_max)
    
    # Neighbor lists
    N_L = sorted_neighbors[:, :L_initial]
    # Tail bounds: weight of the first omitted neighbor
    lambda_L = beta[:, min(L_initial, beta.shape[1]-1)]
    
    # 2. Discover initial K-faces
    # For each site i, we take the K-subset consisting of i and its K-1 closest beta-neighbors
    discovered_faces = set()
    for i in range(n):
        face = tuple(sorted(N_L[i, :K]))
        discovered_faces.add(face)
        
    # Mapping face tuples to integer IDs for Union-Find
    face_to_id = {}
    id_to_face = {}
    
    def get_face_id(face):
        if face not in face_to_id:
            fid = len(face_to_id)
            face_to_id[face] = fid
            id_to_face[fid] = face
            return fid
        return face_to_id[face]
        
    for face in discovered_faces:
        get_face_id(face)
        
    uf = UnionFind()
    for fid in id_to_face:
        uf.add(fid)
        
    mst_edges = []
    
    # Statistics
    gabriel_global_tests_done = 0
    cut_certificates_missing = 0
    
    # Main Borůvka loop
    for iteration in range(max_iter):
        # Group discovered faces by their current Union-Find component
        components = {}
        for fid in id_to_face:
            root = uf.find(fid)
            if root not in components:
                components[root] = []
            components[root].append(fid)
            
        num_components = len(components)
        if num_components <= 1:
            break
            
        # For each component, find the best certified outgoing edge
        best_edges = {} # component_root -> (weight, u_id, v_id, coface)
        component_certified = {root: True for root in components}
        
        for root, fids in components.items():
            U_C = np.inf
            best_edge = None
            
            # Boundary faces of component C (for simplicity, we scan all faces in C)
            for fid in fids:
                tau = id_to_face[fid]
                
                # Minimum tail bound for vertices in tau
                min_lambda = np.min(lambda_L[list(tau)])
                
                # Intersection of prefixes: j such that beta_ij < U_C for all i in tau
                # Start with all neighbors of the first vertex in tau
                ext_candidates = set(N_L[tau[0]])
                for v in tau[1:]:
                    ext_candidates.intersection_update(N_L[v])
                    
                # Filter candidates by beta_ij < U_C
                valid_ext = []
                for j in ext_candidates:
                    if j in tau:
                        continue
                    # Check if beta_ij < U_C for all i in tau
                    # Since beta is local of shape (n, L_max), lookup local index of j in sorted_neighbors[i]
                    is_valid = True
                    for i in tau:
                        idx = np.where(sorted_neighbors[i] == j)[0]
                        if len(idx) > 0:
                            if beta[i, idx[0]] >= U_C:
                                is_valid = False
                                break
                        else:
                            is_valid = False
                            break
                    if is_valid:
                        valid_ext.append(j)
                        
                if not valid_ext:
                    continue
                    
                # Form candidate cofaces
                cofaces_cand = []
                for j in valid_ext:
                    cofaces_cand.append(tuple(sorted(tau + (j,))))
                
                cofaces_cand = list(set(cofaces_cand)) # Deduplicate
                if not cofaces_cand:
                    continue
                    
                # Compute weighted miniball for candidates in batch
                cofaces_arr = np.array(cofaces_cand, dtype=np.int32)
                centers, radii_sq = compute_weighted_miniball_batch(Z, a, cofaces_arr, tol=tol)
                
                # Perform global Gabriel test
                is_gabriel, gaps = gabriel_global_test_batch(centers, radii_sq, Z, a, cofaces_arr, tol=tol)
                gabriel_global_tests_done += len(cofaces_cand)
                
                for idx, sigma in enumerate(cofaces_cand):
                    if not is_gabriel[idx]:
                        continue
                        
                    rho = np.sqrt(max(radii_sq[idx], 0.0))
                    if rho >= U_C:
                        continue
                        
                    # Find all K-facets of the coface
                    facets = []
                    for v in sigma:
                        facets.append(tuple(sorted(set(sigma) - {v})))
                        
                    # Check if all facets are already in the same component C
                    all_in_same = True
                    facet_fids = []
                    for f in facets:
                        if f not in face_to_id:
                            all_in_same = False
                            facet_fids.append(get_face_id(f)) # Discover new face
                        else:
                            facet_fids.append(face_to_id[f])
                            if uf.find(face_to_id[f]) != root:
                                all_in_same = False
                                
                    if all_in_same:
                        continue # Internal edge
                        
                    # Update best edge
                    # Find the facet that is not in C
                    other_fid = None
                    for ffid in facet_fids:
                        if uf.find(ffid) != root:
                            other_fid = ffid
                            break
                    if other_fid is None:
                        other_fid = facet_fids[1] # Fallback if discovering new ones
                        
                    U_C = rho
                    best_edge = (rho, fid, other_fid, sigma)
                    
                # Certification check for this face:
                # If the best edge weight exceeds the tail bound, we might have missed a better edge
                if U_C >= min_lambda:
                    component_certified[root] = False
                    
            if best_edge is not None:
                best_edges[root] = best_edge
                
        # Merge components using the selected best edges
        num_merges = 0
        for root, edge in best_edges.items():
            rho, u_id, v_id, sigma = edge
            
            # Check if this component was certified
            if not component_certified[root]:
                cut_certificates_missing += 1
                
            if uf.union(u_id, v_id):
                mst_edges.append((id_to_face[u_id], id_to_face[v_id], float(rho), sigma))
                num_merges += 1
                
        if num_merges == 0:
            break
            
    # Compile exactness report
    hgp_hierarchy_complete = (cut_certificates_missing == 0)
    
    exactness_report = {
        "model_exact_for_chosen_supports": True,
        "accepted_cofaces_are_true_gabriel": True,
        "full_mosaic_complete": False,
        "hgp_hierarchy_complete": hgp_hierarchy_complete,
        "cut_certificates_missing": cut_certificates_missing,
        "gabriel_global_tests_done": gabriel_global_tests_done,
        "mode_used": "highdim_lazy"
    }
    
    return mst_edges, exactness_report
