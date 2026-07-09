import torch
import numpy as np
from sklearn.base import BaseEstimator, ClusterMixin

from .config import PERGHGPConfig
from .grid import SpatialGrid3D
from .local_gibbs import compute_regularized_sites
from .witnesses import WitnessPool, refine_witness_pool, lift_witness_pool
from .cofaces import extract_top_cofaces
from .gabriel import local_gabriel_filter, global_gabriel_grid_test
from .dual_graph import compute_facet_ids, build_dual_edges, get_edge_chunks
from .hierarchy import dual_graph_mst, condense_tree, extract_labels

class PERGHGPClusterer(BaseEstimator, ClusterMixin):
    """
    Progressive Entropic Rank-Gabriel HGP (PERG-HGP) Clusterer.
    An optimized, Scikit-Learn compatible GPU-friendly implementation.
    """
    __module__ = 'perg_hgp'
    def __init__(self, K=10, K_rho=None, alpha=0.0, m_local=128, m_active=128,
                 grid_resolution=64, W1_budget=3000000, budget_per_rank=2000000,
                 beam_per_bucket=4, rank_eps_schedule=[1.0, 0.5, 0.25, 0.125],
                 L_shell=4, support_cap=4, local_gabriel=True, global_gabriel='selective',
                 gabriel_tol=1e-6, min_cluster_size=100, expZ=2.0, device='cpu',
                 checkpoint_dir=None, exactness_mode='atlas_exact',
                 max_witnesses_per_rank=5000000, max_cofaces=20000000,
                 max_unique_facets=100000000, max_dual_edges=300000000,
                 max_ram_facets=2000000):

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
        self.checkpoint_dir = checkpoint_dir
        self.exactness_mode = exactness_mode
        self.max_witnesses_per_rank = max_witnesses_per_rank
        self.max_cofaces = max_cofaces
        self.max_unique_facets = max_unique_facets
        self.max_dual_edges = max_dual_edges
        self.max_ram_facets = max_ram_facets

    def fit(self, X, y=None):
        """
        Fits the PERG-HGP Clusterer to the input data X.
        Supports saving/loading intermediate checkpoints to checkpoint_dir.
        """
        import os
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

        checkpoint_dir = self.checkpoint_dir
        if checkpoint_dir is not None:
            os.makedirs(checkpoint_dir, exist_ok=True)

        # 1. Build Spatial Grid on X
        print("[PERG-HGP] Building spatial grid on raw points...")
        grid = SpatialGrid3D(X_tensor, grid_resolution=self.grid_resolution, device=self.device)

        # 2. Compute Nearest Neighbors and Solve local Gibbs weights (or load from checkpoint)
        Z = None
        a = None

        sites_path = os.path.join(checkpoint_dir, "sites.pt") if checkpoint_dir else None
        if sites_path and os.path.exists(sites_path):
            print(f"[PERG-HGP] Loading regularized sites from checkpoint {sites_path}...")
            checkpoint_sites = torch.load(sites_path, map_location=device, weights_only=False)
            Z = checkpoint_sites['Z']
            a = checkpoint_sites['a']
        else:
            print("[PERG-HGP] Solving local Gibbs and computing regularized sites...")
            # Query KNN and compute regularized sites in memory-safe chunks to prevent VRAM/RAM overflow
            chunk_size = 500000
            Z_list = []
            a_list = []
            for start_idx in range(0, N, chunk_size):
                end_idx = min(start_idx + chunk_size, N)
                indices_chunk, dists_chunk = grid.query_knn_grid(X_tensor[start_idx:end_idx], m_local=self.m_local)
                Z_chunk, a_chunk, _ = compute_regularized_sites(X_tensor, indices_chunk, dists_chunk, entropy_target=cfg.K_rho)
                # Move to CPU immediately to keep RAM/VRAM bounded
                Z_list.append(Z_chunk.cpu())
                a_list.append(a_chunk.cpu())
            Z = torch.cat(Z_list, dim=0).to(device)
            a = torch.cat(a_list, dim=0).to(device)
            if sites_path:
                torch.save({'Z': Z, 'a': a}, sites_path)

        self.Z_ = Z
        self.a_ = a

        # 3. Build Spatial Grid on Z
        print("[PERG-HGP] Building spatial grid on regularized sites...")
        grid_z = SpatialGrid3D(Z, grid_resolution=self.grid_resolution, device=self.device)

        # Propagate configuration flags based on exactness_mode
        if cfg.exactness_mode == 'atlas_exact':
            self.local_gabriel = True
            self.global_gabriel = 'none'
        elif cfg.exactness_mode == 'global_gabriel_certified':
            self.local_gabriel = True
            self.global_gabriel = 'selective'
        elif cfg.exactness_mode == 'cut_certified':
            self.local_gabriel = True
            self.global_gabriel = 'all'

        if cfg.exactness_mode == 'soft_only':
            print("[PERG-HGP] Running in soft_only mode (no cofaces, direct sites MST)...")
            # Build MST directly on regularized sites Z
            # Each site is connected to its 2 nearest neighbors in Z
            nbr_indices_z, nbr_dists_sq_z = grid_z.query_knn_grid(Z, m_local=2)

            # Edges: connect site i to site j = nbr_indices_z[i, 1]
            edge_u = torch.arange(N, device=device)
            edge_v = nbr_indices_z[:, 1]
            # weight = dist_sq + max(a_i, a_j)
            edge_w = nbr_dists_sq_z[:, 1] + torch.max(a, a[edge_v])

            # Since K=1 (each site is a facet of size 1)
            unique_facets = torch.arange(N, device=device).unsqueeze(1)
            facet_ids = torch.arange(N, device=device).unsqueeze(1)

            # Run MST on sites
            mst_u, mst_v, mst_w, mst_cof = dual_graph_mst(
                edge_u, edge_v, edge_w, torch.zeros_like(edge_u), N
            )

            # S_faces for sites is just 1.0 / (a_i ** expZ)
            S_faces = 1.0 / ((a.cpu().numpy() + 1e-12) ** self.expZ)

            unique_facets_cpu = unique_facets.cpu().numpy()
            flat_facets = unique_facets_cpu.flatten()
            S_faces_expanded = S_faces
            T_points = np.bincount(flat_facets, weights=S_faces_expanded, minlength=N).astype(np.float32)

            inv_T_points = 1.0 / T_points
            inv_T_points[~np.isfinite(inv_T_points)] = 0.0
            sum_inv_Tp_face = np.sum(inv_T_points[unique_facets_cpu], axis=1)
            W_nodes = S_faces * sum_inv_Tp_face

            print("[PERG-HGP] Condensing tree hierarchy...")
            self.Z_tree_ = condense_tree(W_nodes, mst_u, mst_v, mst_w, self.min_cluster_size)

            print("[PERG-HGP] Extracting final labels...")
            self.labels_ = extract_labels(self.Z_tree_, unique_facets_cpu, N, S_faces)

            total_queries = 0
            total_fallbacks = 0
            if 'grid' in locals() and grid is not None:
                total_queries += grid.total_queries_
                total_fallbacks += grid.total_fallbacks_
            if 'grid_z' in locals() and grid_z is not None:
                total_queries += grid_z.total_queries_
                total_fallbacks += grid_z.total_fallbacks_

            self.exactness_report_ = {
                "exactness_mode": cfg.exactness_mode,
                "model_exact_for_chosen_supports": False,
                "accepted_cofaces_are_true_gabriel": False,
                "hgp_hierarchy_complete": False,
                "candidate_cofaces": 0,
                "certified_cofaces": 0,
                "num_facets": N,
                "witness_budget_exceeded": False,
                "cofaces_budget_exceeded": False,
                "facets_budget_exceeded": False,
                "edges_budget_exceeded": False,
                "total_knn_queries": total_queries,
                "total_fallback_queries": total_fallbacks,
                "global_fallback_rate": total_fallbacks / max(1, total_queries)
            }
            print("[PERG-HGP] Complete.")
            return self

        # 4 & 5. Witnesses pool (progressive rank loop or load from checkpoint)
        w_pool = None
        start_rank = 1

        if checkpoint_dir:
            # Check if there is any witnesses_rank_{k}.pt starting from self.K down to 1
            for k_check in range(self.K, 0, -1):
                w_path = os.path.join(checkpoint_dir, f"witnesses_rank_{k_check}.pt")
                if os.path.exists(w_path):
                    print(f"[PERG-HGP] Resuming from witnesses rank {k_check} checkpoint {w_path}...")
                    checkpoint_w = torch.load(w_path, map_location=device, weights_only=False)
                    w_pool = WitnessPool(
                        checkpoint_w['coords'],
                        checkpoint_w['rank'],
                        checkpoint_w['scores'],
                        checkpoint_w['signatures']
                    )
                    start_rank = k_check + 1
                    break

        if w_pool is None:
            # Initialize witnesses of rank 1 by streaming all points
            print("[PERG-HGP] Initializing witness pool by streaming all points...")

            # Accumulate best witnesses for each site in Z entirely on CPU (numpy)
            # to avoid large GPU-CPU memory copies at each chunk.
            Z_cpu = Z.cpu().numpy()
            best_coords_cpu = Z_cpu.copy()
            best_energies_cpu = np.full((N,), np.inf, dtype=np.float32)

            chunk_size = 50000
            for start_idx in range(0, N, chunk_size):
                end_idx = min(start_idx + chunk_size, N)
                W_chunk = Z[start_idx:end_idx].clone()

                # Refine rank 1 on GPU
                for it in range(cfg.fixed_point_iters_per_temp):
                    nbr_indices, nbr_dists_sq = grid_z.query_knn_grid(W_chunk, m_local=1)
                    closest_idx = nbr_indices[:, 0]
                    W_chunk = (1 - cfg.gamma) * W_chunk + cfg.gamma * Z[closest_idx]

                # Compute final energy
                nbr_indices, nbr_dists_sq = grid_z.query_knn_grid(W_chunk, m_local=1)
                closest_idx = nbr_indices[:, 0]
                energy = nbr_dists_sq[:, 0] + a[closest_idx]

                # Copy only the small refined chunk results to CPU
                closest_cpu = closest_idx.cpu().numpy()
                energy_cpu = energy.cpu().numpy()
                W_chunk_cpu = W_chunk.cpu().numpy()

                # Update best_coords and best_energies on CPU
                for i in range(end_idx - start_idx):
                    c_idx = closest_cpu[i]
                    eng = energy_cpu[i]
                    if eng < best_energies_cpu[c_idx]:
                        best_energies_cpu[c_idx] = eng
                        best_coords_cpu[c_idx] = W_chunk_cpu[i]

            active_mask = best_energies_cpu < np.inf
            active_idx = np.where(active_mask)[0]

            W_coords_cpu = best_coords_cpu[active_idx]
            W_scores_cpu = best_energies_cpu[active_idx]

            if len(W_coords_cpu) > self.W1_budget:
                top_idx = np.argsort(W_scores_cpu)[:self.W1_budget]
                W_coords_cpu = W_coords_cpu[top_idx]

            W_coords = torch.from_numpy(W_coords_cpu).to(device)
            w_pool = WitnessPool(W_coords, rank=1)
            if checkpoint_dir:
                w_path = os.path.join(checkpoint_dir, "witnesses_rank_1.pt")
                torch.save({
                    'coords': w_pool.coords,
                    'rank': w_pool.rank,
                    'scores': w_pool.scores,
                    'signatures': w_pool.signatures
                }, w_path)

        # 5. Progressive Rank Refinement & Lifting Loop
        if start_rank <= self.K:
            print(f"[PERG-HGP] Running progressive rank loop up to K = {self.K}...")
            for k in range(start_rank, self.K + 1):
                print(f"  - Rank {k} / {self.K}...")
                w_pool = refine_witness_pool(w_pool, Z, a, grid_z, cfg)

                if checkpoint_dir:
                    w_path = os.path.join(checkpoint_dir, f"witnesses_rank_{k}.pt")
                    torch.save({
                        'coords': w_pool.coords,
                        'rank': w_pool.rank,
                        'scores': w_pool.scores,
                        'signatures': w_pool.signatures
                    }, w_path)

                if k < self.K:
                    eta_val = torch.mean(a).item()
                    w_pool = lift_witness_pool(w_pool, Z, a, eta_val, grid_z, cfg)

        # Final witnesses at rank K+1
        print("[PERG-HGP] Refining final witness pool of rank K+1...")
        w_pool_final = refine_witness_pool(w_pool, Z, a, grid_z, cfg)

        # 6 & 7. Extract & Certify Cofaces
        certified_cofaces = None
        certified_centers = None
        certified_weights = None

        cofaces_path = os.path.join(checkpoint_dir, "certified_cofaces.pt") if checkpoint_dir else None
        if cofaces_path and os.path.exists(cofaces_path):
            print(f"[PERG-HGP] Loading certified cofaces from checkpoint {cofaces_path}...")
            checkpoint_cof = torch.load(cofaces_path, map_location=device, weights_only=False)
            certified_cofaces = checkpoint_cof['cofaces']
            certified_centers = checkpoint_cof['centers']
            certified_weights = checkpoint_cof['weights']
        else:
            print("[PERG-HGP] Extracting candidate cofaces from final witnesses...")
            eta_val = torch.mean(a).item()
            cofaces, centers, weights = extract_top_cofaces(w_pool_final, Z, a, eta_val, grid_z, self.K, cfg)

            # Gabriel Certification
            radii_sq = weights ** 2
            print(f"[PERG-HGP] Certifying {cofaces.shape[0]} unique candidate cofaces...")

            valid_mask = torch.ones(cofaces.shape[0], dtype=torch.bool, device=device)

            if self.local_gabriel:
                nbr_indices_z, _ = grid_z.query_knn_grid(Z, m_local=cfg.m_active)
                local_passes = local_gabriel_filter(cofaces, Z, a, centers, radii_sq, nbr_indices_z, tol=self.gabriel_tol)
                valid_mask &= local_passes
                print(f"  - Local filter: {torch.sum(local_passes).item()} / {cofaces.shape[0]} passed.")

            if self.global_gabriel in ['all', 'selective']:
                cand_idx = torch.where(valid_mask)[0]
                if len(cand_idx) > 0:
                    global_passes = global_gabriel_grid_test(cofaces[cand_idx], Z, a, centers[cand_idx], radii_sq[cand_idx], grid_z, tol=self.gabriel_tol)

                    global_mask = torch.zeros(cofaces.shape[0], dtype=torch.bool, device=device)
                    global_mask[cand_idx] = global_passes
                    valid_mask &= global_mask
                    print(f"  - Global test: {torch.sum(global_passes).item()} / {len(cand_idx)} passed.")

            certified_idx = torch.where(valid_mask)[0]
            if len(certified_idx) == 0:
                print("[PERG-HGP] Warning: No cofaces passed Gabriel certification. Hierarchy will be empty.")
                self.labels_ = np.full(N, -1, dtype=np.int32)
                W_coords_len = w_pool.coords.shape[0] if w_pool is not None else 0

                total_queries = 0
                total_fallbacks = 0
                if 'grid' in locals() and grid is not None:
                    total_queries += grid.total_queries_
                    total_fallbacks += grid.total_fallbacks_
                if 'grid_z' in locals() and grid_z is not None:
                    total_queries += grid_z.total_queries_
                    total_fallbacks += grid_z.total_fallbacks_

                self.exactness_report_ = {
                    "exactness_mode": cfg.exactness_mode,
                    "model_exact_for_chosen_supports": (cfg.alpha == 0.0),
                    "accepted_cofaces_are_true_gabriel": (self.global_gabriel in ['all', 'selective']),
                    "hgp_hierarchy_complete": False,
                    "candidate_cofaces": cofaces.shape[0],
                    "certified_cofaces": 0,
                    "num_facets": 0,
                    "witness_budget_exceeded": (W_coords_len > cfg.max_witnesses_per_rank),
                    "cofaces_budget_exceeded": (cofaces.shape[0] > cfg.max_cofaces),
                    "facets_budget_exceeded": False,
                    "edges_budget_exceeded": False,
                    "total_knn_queries": total_queries,
                    "total_fallback_queries": total_fallbacks,
                    "global_fallback_rate": total_fallbacks / max(1, total_queries)
                }
                return self

            certified_cofaces = cofaces[certified_idx]
            certified_centers = centers[certified_idx]
            certified_weights = weights[certified_idx]

            if cofaces_path:
                torch.save({
                    'cofaces': certified_cofaces,
                    'centers': certified_centers,
                    'weights': certified_weights
                }, cofaces_path)

        # 8 & 9. Build Dual Graph & Compute MST
        mst_u, mst_v, mst_w, mst_cof = None, None, None, None
        facet_ids, unique_facets = None, None

        mst_path = os.path.join(checkpoint_dir, "dual_mst.pt") if checkpoint_dir else None
        if mst_path and os.path.exists(mst_path):
            print(f"[PERG-HGP] Loading dual MST from checkpoint {mst_path}...")
            checkpoint_mst = torch.load(mst_path, map_location=device, weights_only=False)
            mst_u = checkpoint_mst['mst_u']
            mst_v = checkpoint_mst['mst_v']
            mst_w = checkpoint_mst['mst_w']
            mst_cof = checkpoint_mst['mst_cof']
            facet_ids = checkpoint_mst['facet_ids']
            unique_facets = checkpoint_mst['unique_facets']
            num_dual_edges = checkpoint_mst.get('num_dual_edges', len(mst_u))
        else:
            print("[PERG-HGP] Building dual graph of K-facets...")
            facet_ids, unique_facets = compute_facet_ids(certified_cofaces, self.K, max_ram_facets=cfg.max_ram_facets)
            edges = build_dual_edges(certified_cofaces, facet_ids, certified_weights)

            print("[PERG-HGP] Computing dual MST...")
            num_facets = unique_facets.shape[0]
            mst_u, mst_v, mst_w, mst_cof = dual_graph_mst(lambda: edges.chunks(1000000), num_facets)
            num_dual_edges = certified_cofaces.shape[0] * self.K

            if mst_path:
                torch.save({
                    'mst_u': mst_u,
                    'mst_v': mst_v,
                    'mst_w': mst_w,
                    'mst_cof': mst_cof,
                    'facet_ids': facet_ids,
                    'unique_facets': unique_facets,
                    'num_dual_edges': num_dual_edges
                }, mst_path)

        # 10. Condensed Tree Hierarchy (or load from checkpoint)
        tree_path = os.path.join(checkpoint_dir, "Z_tree.pt") if checkpoint_dir else None
        if tree_path and os.path.exists(tree_path):
            print(f"[PERG-HGP] Loading condensed tree from checkpoint {tree_path}...")
            self.Z_tree_ = torch.load(tree_path, map_location='cpu', weights_only=False)
        else:
            num_facets = unique_facets.shape[0]
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
            S_faces = 1.0 / ((facet_birth_weights + 1e-12) ** self.expZ)

            unique_facets_cpu = unique_facets.cpu().numpy()
            flat_facets = unique_facets_cpu.flatten()
            S_faces_expanded = np.repeat(S_faces, self.K)
            T_points = np.bincount(flat_facets, weights=S_faces_expanded, minlength=N).astype(np.float32)

            inv_T_points = np.divide(1.0, T_points, out=np.zeros_like(T_points), where=(T_points > 0.0))
            sum_inv_Tp_face = np.sum(inv_T_points[unique_facets_cpu], axis=1)
            W_nodes = S_faces * sum_inv_Tp_face

            print("[PERG-HGP] Condensing tree hierarchy...")
            self.Z_tree_ = condense_tree(W_nodes, mst_u, mst_v, mst_w, self.min_cluster_size)

            if tree_path:
                torch.save(self.Z_tree_, tree_path)

        # 11. Extract Point Labels via voting
        print("[PERG-HGP] Extracting final labels...")
        num_facets = unique_facets.shape[0]
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
        S_faces = 1.0 / ((facet_birth_weights + 1e-12) ** self.expZ)
        unique_facets_cpu = unique_facets.cpu().numpy()

        self.labels_ = extract_labels(self.Z_tree_, unique_facets_cpu, N, S_faces)

        W_coords_len = w_pool.coords.shape[0] if w_pool is not None else 0
        cof_len = certified_cofaces.shape[0]

        # Budget audits & warnings
        if W_coords_len > cfg.max_witnesses_per_rank:
            print(f"[PERG-HGP] Warning: witnesses count {W_coords_len} exceeds budget {cfg.max_witnesses_per_rank}.")
        if cof_len > cfg.max_cofaces:
            print(f"[PERG-HGP] Warning: candidate cofaces count {cof_len} exceeds budget {cfg.max_cofaces}.")
        if num_facets > cfg.max_unique_facets:
            print(f"[PERG-HGP] Warning: unique facets count {num_facets} exceeds budget {cfg.max_unique_facets}.")
        if num_dual_edges > cfg.max_dual_edges:
            print(f"[PERG-HGP] Warning: dual edges count {num_dual_edges} exceeds budget {cfg.max_dual_edges}.")

        total_queries = 0
        total_fallbacks = 0
        if 'grid' in locals() and grid is not None:
            total_queries += grid.total_queries_
            total_fallbacks += grid.total_fallbacks_
        if 'grid_z' in locals() and grid_z is not None:
            total_queries += grid_z.total_queries_
            total_fallbacks += grid_z.total_fallbacks_

        self.exactness_report_ = {
            "exactness_mode": cfg.exactness_mode,
            "model_exact_for_chosen_supports": (cfg.alpha == 0.0),
            "accepted_cofaces_are_true_gabriel": (self.global_gabriel in ['all', 'selective'] and cof_len > 0),
            "hgp_hierarchy_complete": False, # True only if a strict geometric cut-audit is implemented and passes
            "candidate_cofaces": cof_len,
            "certified_cofaces": cof_len,
            "num_facets": num_facets,
            "witness_budget_exceeded": (W_coords_len > cfg.max_witnesses_per_rank),
            "cofaces_budget_exceeded": (cof_len > cfg.max_cofaces),
            "facets_budget_exceeded": (num_facets > cfg.max_unique_facets),
            "edges_budget_exceeded": (num_dual_edges > cfg.max_dual_edges),
            "total_knn_queries": total_queries,
            "total_fallback_queries": total_fallbacks,
            "global_fallback_rate": total_fallbacks / max(1, total_queries)
        }

        print("[PERG-HGP] Complete.")
        return self

    def fit_predict(self, X, y=None):
        self.fit(X)
        return self.labels_
