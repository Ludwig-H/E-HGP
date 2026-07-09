import torch
import numpy as np
from sklearn.base import BaseEstimator, ClusterMixin

from .config import PERGHGPConfig
from .grid import SpatialGrid3D
from .local_gibbs import compute_regularized_sites
from .witnesses import WitnessPool, refine_witness_pool, lift_witness_pool
from .cofaces import extract_top_cofaces
from .gabriel import local_gabriel_filter, global_gabriel_grid_test
from .dual_graph import compute_facet_ids, build_dual_edges
from .hierarchy import dual_graph_mst, condense_tree, extract_labels

class PERGHGPClusterer(BaseEstimator, ClusterMixin):
    """
    Progressive Entropic Rank-Gabriel HGP (PERG-HGP) Clusterer.
    An optimized, Scikit-Learn compatible GPU-friendly implementation.
    """
    def __init__(self, K=10, K_rho=None, alpha=0.0, m_local=128, m_active=128,
                 grid_resolution=64, W1_budget=10000, budget_per_rank=10000,
                 beam_per_bucket=4, rank_eps_schedule=[1.0, 0.5, 0.25, 0.125],
                 L_shell=4, support_cap=4, local_gabriel=True, global_gabriel='selective',
                 gabriel_tol=1e-6, min_cluster_size=100, expZ=2.0, device='cpu'):
        
        self.K = K
        self.K_rho = K_rho
        self.alpha = alpha
        self.m_local = m_local
        self.m_active = m_active
        self.grid_resolution = grid_resolution
        self.W1_budget = W1_budget
        self.budget_per_rank = budget_per_rank
        self.beam_per_bucket = beam_per_bucket
        self.rank_eps_schedule = rank_eps_schedule
        self.L_shell = L_shell
        self.support_cap = support_cap
        self.local_gabriel = local_gabriel
        self.global_gabriel = global_gabriel
        self.gabriel_tol = gabriel_tol
        self.min_cluster_size = min_cluster_size
        self.expZ = expZ
        self.device = device
        
    def fit(self, X, y=None):
        """
        Fits the PERG-HGP Clusterer to the input data X.
        """
        # Load config
        cfg = PERGHGPConfig(**self.get_params())
        
        if cfg.alpha != 0.0:
            raise NotImplementedError(
                "Multiplicatively weighted power miniball not implemented. "
                "Set alpha=0 for exact additive power geometry."
            )
            
        device = torch.device(self.device)
        
        # Ensure PyTorch tensor
        if isinstance(X, np.ndarray):
            X_tensor = torch.from_numpy(X).to(device, dtype=torch.float32)
        else:
            X_tensor = X.to(device, dtype=torch.float32)
            
        N = X_tensor.shape[0]
        
        # 1. Build Spatial Grid on X
        print("[PERG-HGP] Building spatial grid on raw points...")
        grid = SpatialGrid3D(X_tensor, grid_resolution=self.grid_resolution, device=self.device)
        
        # 2. Compute Nearest Neighbors and Solve local Gibbs weights
        print("[PERG-HGP] Solving local Gibbs and computing regularized sites...")
        nbr_indices, nbr_dists_sq = grid.query_knn_grid(X_tensor, m_local=self.m_local)
        Z, a, eta = compute_regularized_sites(X_tensor, nbr_indices, nbr_dists_sq, entropy_target=cfg.K_rho)
        self.Z_ = Z
        self.a_ = a
        
        # 3. Build Spatial Grid on Z
        print("[PERG-HGP] Building spatial grid on regularized sites...")
        grid_z = SpatialGrid3D(Z, grid_resolution=self.grid_resolution, device=self.device)
        
        # 4. Initialize witnesses of rank 1
        # Stream points as seeds
        print("[PERG-HGP] Initializing witness pool...")
        # Since N can be massive (30M), we subsample for the witness pool initialization to respect W1_budget
        if N > self.W1_budget:
            idx = torch.randperm(N, device=device)[:self.W1_budget]
            W_coords = Z[idx].clone()
        else:
            W_coords = Z.clone()
            
        w_pool = WitnessPool(W_coords, rank=1)
        
        # 5. Progressive Rank Refinement & Lifting Loop
        print(f"[PERG-HGP] Running progressive rank loop up to K = {self.K}...")
        for k in range(1, self.K + 1):
            print(f"  - Rank {k} / {self.K}...")
            # Refine witnesses
            w_pool = refine_witness_pool(w_pool, Z, a, grid_z, cfg)
            
            # Lift to rank k+1
            if k < self.K:
                w_pool = lift_witness_pool(w_pool, Z, a, eta, grid_z, cfg)
                
        # Final witnesses at rank K+1
        print("[PERG-HGP] Refining final witness pool of rank K+1...")
        w_pool_final = refine_witness_pool(w_pool, Z, a, grid_z, cfg)
        
        # 6. Extract Candidate Cofaces
        print("[PERG-HGP] Extracting candidate cofaces from final witnesses...")
        cofaces, centers, weights = extract_top_cofaces(w_pool_final, Z, a, eta, grid_z, self.K, cfg)
        
        # 7. Gabriel Certification
        radii_sq = weights ** 2
        print(f"[PERG-HGP] Certifying {cofaces.shape[0]} unique candidate cofaces...")
        
        valid_mask = torch.ones(cofaces.shape[0], dtype=torch.bool, device=device)
        
        if self.local_gabriel:
            local_passes = local_gabriel_filter(cofaces, Z, a, centers, radii_sq, nbr_indices, tol=self.gabriel_tol)
            valid_mask &= local_passes
            print(f"  - Local filter: {torch.sum(local_passes).item()} / {cofaces.shape[0]} passed.")
            
        if self.global_gabriel in ['all', 'selective']:
            # Select cofaces to check globally: selective tests only active ones
            # For simplicity, we check all candidate cofaces that passed local filter
            cand_idx = torch.where(valid_mask)[0]
            if len(cand_idx) > 0:
                global_passes = global_gabriel_grid_test(cofaces[cand_idx], Z, a, centers[cand_idx], radii_sq[cand_idx], grid_z, tol=self.gabriel_tol)
                
                global_mask = torch.zeros(cofaces.shape[0], dtype=torch.bool, device=device)
                global_mask[cand_idx] = global_passes
                valid_mask &= global_mask
                print(f"  - Global test: {torch.sum(global_passes).item()} / {len(cand_idx)} passed.")
                
        # Keep only certified cofaces
        certified_idx = torch.where(valid_mask)[0]
        if len(certified_idx) == 0:
            print("[PERG-HGP] Warning: No cofaces passed Gabriel certification. Hierarchy will be empty.")
            self.labels_ = np.full(N, -1, dtype=np.int32)
            return self
            
        certified_cofaces = cofaces[certified_idx]
        certified_centers = centers[certified_idx]
        certified_weights = weights[certified_idx]
        
        # 8. Dual Graph & Facets
        print("[PERG-HGP] Building dual graph of K-facets...")
        facet_ids, unique_facets = compute_facet_ids(certified_cofaces, self.K)
        edge_u, edge_v, edge_w, edge_coface = build_dual_edges(certified_cofaces, facet_ids, certified_weights)
        
        # 9. Compute MST on dual graph using SciPy
        print("[PERG-HGP] Computing dual MST...")
        num_facets = unique_facets.shape[0]
        mst_u, mst_v, mst_w, mst_cof = dual_graph_mst(edge_u, edge_v, edge_w, edge_coface, num_facets)
        
        # 10. Condensed Tree Hierarchy
        # Compute facet birth weights (rho) and S_faces = 1.0 / (rho ** expZ)
        facet_birth_weights = np.full(num_facets, np.inf, dtype=np.float32)
        cof_w_cpu = certified_weights.cpu().numpy()
        facet_ids_cpu = facet_ids.cpu().numpy()
        for i in range(certified_cofaces.shape[0]):
            w_val = cof_w_cpu[i]
            for r in range(self.K + 1):
                f_id = facet_ids_cpu[i, r]
                if w_val < facet_birth_weights[f_id]:
                    facet_birth_weights[f_id] = w_val
                    
        facet_birth_weights[facet_birth_weights == np.inf] = 0.0
        
        # Compute S_faces
        S_faces = 1.0 / ((facet_birth_weights + 1e-12) ** self.expZ)
        
        # Compute T_points[p] = sum_{f contains p} S_faces[f]
        unique_facets_cpu = unique_facets.cpu().numpy()
        flat_facets = unique_facets_cpu.flatten()
        S_faces_expanded = np.repeat(S_faces, self.K)
        T_points = np.bincount(flat_facets, weights=S_faces_expanded, minlength=N).astype(np.float32)
        
        # Compute W_nodes[f] = S_faces[f] * sum_{p in f} (1.0 / T_points[p])
        inv_T_points = 1.0 / T_points
        inv_T_points[~np.isfinite(inv_T_points)] = 0.0
        sum_inv_Tp_face = np.sum(inv_T_points[unique_facets_cpu], axis=1)
        W_nodes = S_faces * sum_inv_Tp_face
        
        print("[PERG-HGP] Condensing tree hierarchy...")
        self.Z_tree_ = condense_tree(W_nodes, mst_u, mst_v, mst_w, self.min_cluster_size)
        
        # 11. Extract Point Labels via voting
        print("[PERG-HGP] Extracting final labels...")
        self.labels_ = extract_labels(self.Z_tree_, unique_facets_cpu, N, S_faces)
        
        # Save diagnostics
        # Budget audits
        if len(W_coords) > cfg.max_witnesses_per_rank:
            print(f"[PERG-HGP] Warning: witnesses count {len(W_coords)} exceeds max_witnesses_per_rank budget {cfg.max_witnesses_per_rank}.")
        if cofaces.shape[0] > cfg.max_cofaces:
            print(f"[PERG-HGP] Warning: candidate cofaces count {cofaces.shape[0]} exceeds max_cofaces budget {cfg.max_cofaces}.")
        if num_facets > cfg.max_unique_facets:
            print(f"[PERG-HGP] Warning: unique facets count {num_facets} exceeds max_unique_facets budget {cfg.max_unique_facets}.")
        if len(edge_u) > cfg.max_dual_edges:
            print(f"[PERG-HGP] Warning: dual edges count {len(edge_u)} exceeds max_dual_edges budget {cfg.max_dual_edges}.")

        self.exactness_report_ = {
            "exactness_mode": cfg.exactness_mode,
            "model_exact_for_chosen_supports": (cfg.alpha == 0.0),
            "accepted_cofaces_are_true_gabriel": (cfg.exactness_mode in ["global_gabriel_certified", "cut_certified"]),
            "hgp_hierarchy_complete": (cfg.exactness_mode == "cut_certified"),
            "candidate_cofaces": cofaces.shape[0],
            "certified_cofaces": len(certified_idx),
            "num_facets": num_facets,
            "witness_budget_exceeded": (len(W_coords) > cfg.max_witnesses_per_rank),
            "cofaces_budget_exceeded": (cofaces.shape[0] > cfg.max_cofaces),
            "facets_budget_exceeded": (num_facets > cfg.max_unique_facets),
            "edges_budget_exceeded": (len(edge_u) > cfg.max_dual_edges)
        }
        
        print("[PERG-HGP] Complete.")
        return self
        
    def fit_predict(self, X, y=None):
        self.fit(X)
        return self.labels_
