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
        
    return beta_sorted, sorted_neighbors, dists[:, -1]


def get_certified_lambda(i, neigh_list, beta_list, D_max_list, L_current, n):
    if L_current[i] >= n:
        return np.inf
    idx = min(L_current[i], len(beta_list[i]) - 1)
    lambda_raw = beta_list[i][idx]
    return min(lambda_raw, 0.5 * D_max_list[i])


def get_beta_val(i, j, neigh_list, beta_list):
    idx = np.where(neigh_list[i] == j)[0]
    if len(idx) > 0:
        return beta_list[i][idx[0]]
    return np.inf


def expand_site_neighbors(i, target_beta, Z, a, tree, n, neigh_list, beta_list, D_max_list, L_current):
    while L_current[i] < n:
        lambda_cert = get_certified_lambda(i, neigh_list, beta_list, D_max_list, L_current, n)
        if lambda_cert > target_beta:
            break
            
        L_new = min(n, L_current[i] * 2)
        if L_new <= L_current[i]:
            break
            
        dists, indices = tree.query(Z[i], k=L_new)
        D2 = dists ** 2
        D = dists
        a_j = a[indices]
        a_i = a[i]
        
        denom = 2.0 * D
        mask_zero = D < 1e-12
        denom[mask_zero] = 1.0
        
        u = (D2 + a_j - a_i) / denom
        u = np.clip(u, 0.0, D)
        u[mask_zero] = 0.0
        
        beta2 = np.maximum(u**2 + a_i, (D - u)**2 + a_j)
        beta_local = np.sqrt(beta2)
        
        sort_idx = np.argsort(beta_local)
        neigh_list[i] = indices[sort_idx]
        beta_list[i] = beta_local[sort_idx]
        D_max_list[i] = dists[-1]
        L_current[i] = L_new


def check_gabriel_local_tau(center, r2, sigma, Z, a, union_support_tau, tol=1e-8):
    check_set = list(union_support_tau - set(sigma))
    if not check_set:
        return True
        
    diff = Z[check_set] - center
    phi = np.sum(diff ** 2, axis=1) + a[check_set]
    if np.any(phi < r2 - tol):
        return False
    return True


def is_internal_coface(sigma, root, face_to_id, uf):
    for v in sigma:
        facet = tuple(sorted(set(sigma) - {v}))
        if facet not in face_to_id:
            return False
        if uf.find(face_to_id[facet]) != root:
            return False
    return True


def lazy_cut_boruvka(Z, a, K, L_initial=30, max_iter=20, tol=1e-6):
    """
    Lazy Cut-Borůvka algorithm for finding the certified K-MST.
    """
    n, p = Z.shape
    L_initial = min(n, L_initial)
    
    # 1. Build beta lists locally
    L_max = max(30, L_initial + 10)
    L_max = min(n, L_max)
    beta_sorted, sorted_neighbors, D_max = compute_pairwise_beta(Z, a, L_max=L_max)
    
    # Represent neighbor lists as lists of 1D arrays to support local variable-size expansions
    beta_list = [beta_sorted[i] for i in range(n)]
    neigh_list = [sorted_neighbors[i] for i in range(n)]
    D_max_list = list(D_max)
    L_current = np.full(n, L_initial, dtype=np.int32)
    
    tree = KDTree(Z)
    
    # 2. Discover initial K-faces
    discovered_faces = set()
    for i in range(n):
        face = tuple(sorted(neigh_list[i][:K]))
        discovered_faces.add(face)
        
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
    gabriel_global_tests_done = 0
    cut_certificates_missing = 0
    
    # Main Borůvka loop
    for iteration in range(max_iter):
        components = {}
        for fid in id_to_face:
            root = uf.find(fid)
            if root not in components:
                components[root] = []
            components[root].append(fid)
            
        num_components = len(components)
        if num_components <= 1:
            break
            
        best_edges = {} # component_root -> (weight, u_id, v_id, coface)
        component_certified = {root: True for root in components}
        
        for root, fids in components.items():
            local_candidates = []
            
            # 1. Collect all valid candidates locally across all boundary faces of component
            for fid in fids:
                tau = id_to_face[fid]
                
                # Intersection of prefixes
                ext_candidates = set(neigh_list[tau[0]][:L_current[tau[0]]])
                for v in tau[1:]:
                    ext_candidates.intersection_update(neigh_list[v][:L_current[v]])
                    
                valid_ext = []
                for j in ext_candidates:
                    if j in tau:
                        continue
                    is_valid = True
                    for i in tau:
                        if get_beta_val(i, j, neigh_list, beta_list) >= np.inf:
                            is_valid = False
                            break
                    if is_valid:
                        valid_ext.append(j)
                        
                if not valid_ext:
                    continue
                    
                # Generate and filter candidate cofaces (check if internal to component early)
                cofaces_cand = []
                for j in valid_ext:
                    sigma = tuple(sorted(tau + (j,)))
                    if is_internal_coface(sigma, root, face_to_id, uf):
                        continue
                    cofaces_cand.append(sigma)
                    
                if not cofaces_cand:
                    continue
                    
                cofaces_arr = np.array(cofaces_cand, dtype=np.int32)
                centers, radii_sq = compute_weighted_miniball_batch(Z, a, cofaces_arr, tol=tol)
                
                union_support_tau = set()
                for i in tau:
                    union_support_tau.update(neigh_list[i][:L_current[i]])
                    
                for idx, sigma in enumerate(cofaces_cand):
                    rho = np.sqrt(max(radii_sq[idx], 0.0))
                    if not check_gabriel_local_tau(centers[idx], radii_sq[idx], sigma, Z, a, union_support_tau, tol=tol):
                        continue
                    local_candidates.append((rho, fid, sigma, centers[idx], radii_sq[idx]))
                    
            if not local_candidates:
                # No candidates found locally. If any boundary face has finite certification bound, the component is not certified.
                for fid in fids:
                    tau = id_to_face[fid]
                    min_lambda = min(get_certified_lambda(i, neigh_list, beta_list, D_max_list, L_current, n) for i in tau)
                    if min_lambda < np.inf:
                        component_certified[root] = False
                        break
                continue
                
            # 2. Sort local candidates by rho ascending
            local_candidates.sort(key=lambda x: x[0])
            
            # 3. Find the best edge by checking Gabriel globally in order of rho
            best_edge = None
            for rho, fid, sigma, center, r2 in local_candidates:
                # Perform global Gabriel test ONLY on this specific candidate
                centers_single = np.array([center])
                radii_sq_single = np.array([r2])
                cofaces_arr_single = np.array([sigma], dtype=np.int32)
                
                is_gabriel, gaps = gabriel_global_test_batch(centers_single, radii_sq_single, Z, a, cofaces_arr_single, tol=tol)
                gabriel_global_tests_done += 1
                
                if not is_gabriel[0]:
                    continue
                    
                facets = []
                for v in sigma:
                    facets.append(tuple(sorted(set(sigma) - {v})))
                    
                all_in_same = True
                facet_fids = []
                for f in facets:
                    if f not in face_to_id:
                        all_in_same = False
                        facet_fids.append(get_face_id(f))
                    else:
                        facet_fids.append(face_to_id[f])
                        if uf.find(face_to_id[f]) != root:
                            all_in_same = False
                            
                if all_in_same:
                    continue
                    
                other_fid = None
                for ffid in facet_fids:
                    if uf.find(ffid) != root:
                        other_fid = ffid
                        break
                if other_fid is None:
                    other_fid = facet_fids[1]
                    
                best_edge = (rho, fid, other_fid, sigma)
                break
                
            # 4. Check certification and expand locally across ALL boundary faces if needed
            if best_edge is not None:
                rho, fid, other_fid, sigma = best_edge
                
                # We must audit all boundary faces of the component to ensure no omitted coface is smaller than rho
                for f_id_check in fids:
                    if not component_certified[root]:
                        break
                    tau_check = id_to_face[f_id_check]
                    min_lambda = min(get_certified_lambda(i, neigh_list, beta_list, D_max_list, L_current, n) for i in tau_check)
                    if rho >= min_lambda:
                        # Expand neighbor lists
                        for i in tau_check:
                            expand_site_neighbors(i, rho, Z, a, tree, n, neigh_list, beta_list, D_max_list, L_current)
                        min_lambda = min(get_certified_lambda(i, neigh_list, beta_list, D_max_list, L_current, n) for i in tau_check)
                        if rho >= min_lambda:
                            component_certified[root] = False
                best_edges[root] = best_edge
            else:
                # No edge passed global Gabriel. If any boundary face has finite certification bound, the component is not certified.
                for fid in fids:
                    tau = id_to_face[fid]
                    min_lambda = min(get_certified_lambda(i, neigh_list, beta_list, D_max_list, L_current, n) for i in tau)
                    if min_lambda < np.inf:
                        component_certified[root] = False
                        break
                        
        num_merges = 0
        for root, edge in best_edges.items():
            rho, u_id, v_id, sigma = edge
            if not component_certified[root]:
                cut_certificates_missing += 1
                
            if uf.union(u_id, v_id):
                mst_edges.append((id_to_face[u_id], id_to_face[v_id], float(rho), sigma))
                num_merges += 1
                
        if num_merges == 0:
            break
            
    hgp_hierarchy_complete = (cut_certificates_missing == 0) and (num_components <= 1)
    
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
