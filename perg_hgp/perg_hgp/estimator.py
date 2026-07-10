import os
import json
import hashlib
import torch
import numpy as np
from sklearn.base import BaseEstimator, ClusterMixin

def _save_atomic(data, path):
    tmp_path = path + ".tmp"
    torch.save(data, tmp_path)
    os.replace(tmp_path, path)


_CHECKPOINT_SCHEMA_VERSION = 3


def _algorithm_fingerprint():
    """Hash the Python implementation that defines checkpoint semantics."""
    digest = hashlib.sha256()
    package_dir = os.path.dirname(__file__)
    algorithm_files = []
    for root, directories, files in os.walk(package_dir):
        directories[:] = sorted(
            name for name in directories if name != '__pycache__'
        )
        for name in sorted(files):
            if name.endswith(('.py', '.cu', '.cuh', '.cpp', '.hpp')):
                path = os.path.join(root, name)
                algorithm_files.append((os.path.relpath(path, package_dir), path))
    for relative, path in sorted(algorithm_files):
        digest.update(relative.encode('utf-8'))
        with open(path, 'rb') as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b''):
                digest.update(chunk)
    return digest.hexdigest()


def _checkpoint_manifest(X_tensor, cfg):
    X_cpu = X_tensor.detach().to('cpu').contiguous()
    X_array = X_cpu.numpy()
    digest = hashlib.sha256(memoryview(X_array).cast('B')).hexdigest()
    config = cfg.to_dict()
    config.pop('checkpoint_dir', None)
    config.pop('device', None)
    config = json.loads(json.dumps(config, sort_keys=True))
    return {
        'schema_version': _CHECKPOINT_SCHEMA_VERSION,
        'algorithm_sha256': _algorithm_fingerprint(),
        'data_shape': list(X_array.shape),
        'data_dtype': str(X_array.dtype),
        'data_sha256': digest,
        'config': config,
    }


def _validate_checkpoint_dir(checkpoint_dir, manifest):
    manifest_path = os.path.join(checkpoint_dir, 'manifest.json')
    artifacts = [name for name in os.listdir(checkpoint_dir) if name.endswith('.pt')]
    if os.path.exists(manifest_path):
        with open(manifest_path, 'r', encoding='utf-8') as stream:
            existing = json.load(stream)
        if existing != manifest:
            if not artifacts:
                tmp_path = manifest_path + '.tmp'
                with open(tmp_path, 'w', encoding='utf-8') as stream:
                    json.dump(manifest, stream, sort_keys=True, indent=2)
                os.replace(tmp_path, manifest_path)
                return
            raise ValueError(
                "checkpoint_dir contains artifacts for different data or parameters; "
                "use an empty checkpoint directory"
            )
        return
    if artifacts:
        raise ValueError(
            "checkpoint_dir contains legacy artifacts without a manifest; "
            "use an empty checkpoint directory"
        )
    tmp_path = manifest_path + '.tmp'
    with open(tmp_path, 'w', encoding='utf-8') as stream:
        json.dump(manifest, stream, sort_keys=True, indent=2)
    os.replace(tmp_path, manifest_path)


def _compute_face_scores(facet_ids, coface_radii, num_facets, exp_z):
    """Thesis score S_tau = sum_{sigma superset tau} rho(sigma)^(-exp_z)."""
    facet_ids_cpu = facet_ids.detach().cpu().numpy()
    radii = coface_radii.detach().cpu().numpy().astype(np.float64, copy=False)
    coface_scores = 1.0 / np.power(radii + 1e-12, exp_z)
    scores = np.bincount(
        facet_ids_cpu.reshape(-1),
        weights=np.repeat(coface_scores, facet_ids_cpu.shape[1]),
        minlength=num_facets,
    )
    return scores.astype(np.float32, copy=False)

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
                 beam_per_bucket=4, rank_eps_schedule=(1.0, 0.5, 0.25, 0.125),
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

        # Validate input shapes and values
        if not isinstance(X, (np.ndarray, torch.Tensor)):
            raise ValueError("Input X must be a numpy array or PyTorch tensor.")

        N_in = X.shape[0]
        if N_in <= 0:
            raise ValueError(f"Input X must contain at least 1 point. Got N = {N_in}")

        if len(X.shape) != 2 or X.shape[1] != 3:
            raise ValueError(f"Input X must have shape (N, 3). Got {X.shape}")

        if isinstance(X, np.ndarray):
            if not np.isfinite(X).all():
                raise ValueError("Input X contains non-finite values (NaN or Inf).")
        else:
            if not torch.isfinite(X).all():
                raise ValueError("Input X contains non-finite values (NaN or Inf).")

        # Validate hyperparameters
        if self.K < 1 or self.K >= N_in:
            raise ValueError(f"Hyperparameter K must satisfy 1 <= K < N. Got K = {self.K}, N = {N_in}")

        if self.m_active < self.K + 1:
            raise ValueError(f"m_active ({self.m_active}) must be >= K + 1 ({self.K + 1})")

        if self.m_local < 1:
            raise ValueError(f"m_local must be >= 1. Got {self.m_local}")

        if self.grid_resolution < 1:
            raise ValueError(f"grid_resolution must be >= 1. Got {self.grid_resolution}")

        if not self.rank_eps_schedule or len(self.rank_eps_schedule) == 0:
            raise ValueError("rank_eps_schedule must not be empty.")

        if any(eps <= 0 for eps in self.rank_eps_schedule):
            raise ValueError("rank_eps_schedule temperatures must be strictly positive.")

        if self.max_ram_facets < 1:
            raise ValueError(f"max_ram_facets must be >= 1. Got {self.max_ram_facets}")
        if not np.isfinite(self.gabriel_tol) or self.gabriel_tol <= 0:
            raise ValueError(f"gabriel_tol must be positive. Got {self.gabriel_tol}")
        if not 0.0 < cfg.gamma <= 1.0:
            raise ValueError(f"gamma must satisfy 0 < gamma <= 1. Got {cfg.gamma}")
        if self.W1_budget < 1 or self.budget_per_rank < 1:
            raise ValueError("witness budgets must be positive")

        valid_modes = {'soft_only', 'atlas_exact', 'global_gabriel_certified', 'cut_certified'}
        if self.exactness_mode not in valid_modes:
            raise ValueError(f"exactness_mode must be one of {sorted(valid_modes)}")
        if self.exactness_mode == 'cut_certified':
            raise NotImplementedError(
                "cut_certified is not implemented: the current code has no cut-completeness audit"
            )

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

        # Effective fitted parameters are capped without mutating sklearn's
        # constructor parameters, so repeated fits remain independent.
        self.m_local_ = min(self.m_local, N)
        self.m_active_ = min(self.m_active, N)
        if self.K_rho is None:
            self.K_rho_ = min(max(32, 3 * self.K), self.m_local_)
        else:
            if not np.isfinite(self.K_rho) or self.K_rho < 1 or self.K_rho > self.m_local_:
                raise ValueError(
                    f"K_rho must satisfy 1 <= K_rho <= effective m_local ({self.m_local_}); "
                    f"got {self.K_rho}"
                )
            self.K_rho_ = self.K_rho
        cfg.m_local = self.m_local_
        cfg.m_active = self.m_active_
        cfg.K_rho = self.K_rho_

        checkpoint_dir = self.checkpoint_dir
        if checkpoint_dir is not None:
            os.makedirs(checkpoint_dir, exist_ok=True)
            _validate_checkpoint_dir(checkpoint_dir, _checkpoint_manifest(X_tensor, cfg))

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
                indices_chunk, dists_chunk = grid.query_knn_grid(X_tensor[start_idx:end_idx], m_local=self.m_local_)
                Z_chunk, a_chunk, _ = compute_regularized_sites(X_tensor, indices_chunk, dists_chunk, entropy_target=cfg.K_rho)
                # Move to CPU immediately to keep RAM/VRAM bounded
                Z_list.append(Z_chunk.cpu())
                a_list.append(a_chunk.cpu())
            Z = torch.cat(Z_list, dim=0).to(device)
            a = torch.cat(a_list, dim=0).to(device)
            if sites_path:
                _save_atomic({'Z': Z, 'a': a}, sites_path)

        self.Z_ = Z
        self.a_ = a
        displacement_sq = torch.sum((X_tensor - Z) ** 2, dim=1) + a
        self.regularization_interleaving_radius_ = float(
            torch.sqrt(torch.clamp(torch.max(displacement_sq), min=0.0)).item()
        )

        # 3. Build Spatial Grid on Z
        print("[PERG-HGP] Building spatial grid on regularized sites...")
        grid_z = SpatialGrid3D(Z, grid_resolution=self.grid_resolution, device=self.device)

        # Propagate configuration flags based on exactness_mode
        self.local_gabriel_ = self.local_gabriel
        self.global_gabriel_ = self.global_gabriel
        if cfg.exactness_mode == 'atlas_exact':
            # extract_top_cofaces already performs an exact global top-(K+1)
            # consistency check, which implies the strict Gabriel condition.
            self.local_gabriel_ = False
            self.global_gabriel_ = 'none'
        elif cfg.exactness_mode == 'global_gabriel_certified':
            self.local_gabriel_ = False
            self.global_gabriel_ = 'all'

        if cfg.exactness_mode == 'soft_only':
            print("[PERG-HGP] Running in soft_only mode (no cofaces, direct sites MST)...")
            # Build MST directly on regularized sites Z
            # Each site is connected to its nearest *other* power site.
            nbr_indices_z, nbr_dists_sq_z_power = grid_z.query_power_grid(Z, a, m_local=2)
            edge_u = torch.arange(N, device=device)
            is_self = nbr_indices_z == edge_u[:, None]
            external_power = torch.where(is_self, float('inf'), nbr_dists_sq_z_power)
            external_col = torch.argmin(external_power, dim=1)
            edge_v = torch.gather(nbr_indices_z, 1, external_col[:, None]).squeeze(1)

            # Exact two-site additive weighted miniball radius.
            d2 = torch.sum((Z[edge_v] - Z) ** 2, dim=1)
            ai, aj = a, a[edge_v]
            safe_d2 = torch.clamp(d2, min=torch.finfo(d2.dtype).tiny)
            alpha = 0.5 + (aj - ai) / (2.0 * safe_d2)
            pair_r2 = alpha ** 2 * d2 + ai
            pair_r2 = torch.where(d2 + aj <= ai, ai, pair_r2)
            pair_r2 = torch.where(d2 + ai <= aj, aj, pair_r2)
            pair_r2 = torch.where(d2 == 0, torch.maximum(ai, aj), pair_r2)
            edge_w = torch.sqrt(torch.clamp(pair_r2, min=0.0))

            # Since K=1 (each site is a facet of size 1)
            unique_facets = torch.arange(N, device=device).unsqueeze(1)
            facet_ids = torch.arange(N, device=device).unsqueeze(1)

            # Run MST on sites
            mst_u, mst_v, mst_w, mst_cof = dual_graph_mst(
                edge_u, edge_v, edge_w, torch.zeros_like(edge_u), N
            )

            site_birth_radii = np.sqrt(np.maximum(a.cpu().numpy(), 0.0))
            S_faces = 1.0 / ((site_birth_radii + 1e-12) ** self.expZ)

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
                "result_status": "heuristic",
                "model_exact_for_chosen_supports": True,
                "regularization_model": "localized_exact_knn",
                "regularization_support_size": self.m_local_,
                "regularization_interleaving_radius": self.regularization_interleaving_radius_,
                "accepted_cofaces_are_true_gabriel": False,
                "atlas_hierarchy_exact": False,
                "hgp_hierarchy_complete": False,
                "full_power_hgp_complete": False,
                "cut_certified": False,
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
            # Expose artifact properties as fitted attributes in soft_only mode
            self.cofaces_ = None
            self.coface_weights_ = None
            self.facets_ = unique_facets_cpu
            self.msf_ = {
                "mst_u": mst_u.cpu().numpy() if isinstance(mst_u, torch.Tensor) else mst_u,
                "mst_v": mst_v.cpu().numpy() if isinstance(mst_v, torch.Tensor) else mst_v,
                "mst_w": mst_w.cpu().numpy() if isinstance(mst_w, torch.Tensor) else mst_w,
                "mst_cof": mst_cof.cpu().numpy() if isinstance(mst_cof, torch.Tensor) else mst_cof
            }
            self.merge_tree_ = self.Z_tree_

            print("[PERG-HGP] Complete.")
            return self

        # 4 & 5. Witnesses pool (progressive rank loop or load from checkpoint)
        w_pool = None
        pool_is_refined = False

        if checkpoint_dir:
            for k_check in range(self.K + 1, 0, -1):
                w_path = os.path.join(checkpoint_dir, f"witnesses_rank_{k_check}.pt")
                if os.path.exists(w_path):
                    print(f"[PERG-HGP] Resuming from witnesses rank {k_check} checkpoint {w_path}...")
                    checkpoint_w = torch.load(w_path, map_location=device, weights_only=False)
                    if int(checkpoint_w['rank']) != k_check:
                        raise ValueError(f"Invalid witness checkpoint rank in {w_path}")
                    w_pool = WitnessPool(
                        checkpoint_w['coords'],
                        checkpoint_w['rank'],
                        checkpoint_w['scores'],
                        checkpoint_w['signatures']
                    )
                    pool_is_refined = True
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
                    nbr_indices, _ = grid_z.query_power_grid(W_chunk, a, m_local=1)
                    closest_idx = nbr_indices[:, 0]
                    W_chunk = (1 - cfg.gamma) * W_chunk + cfg.gamma * Z[closest_idx]

                # Compute final energy
                nbr_indices, nbr_dists_sq_power = grid_z.query_power_grid(W_chunk, a, m_local=1)
                closest_idx = nbr_indices[:, 0]
                energy = nbr_dists_sq_power[:, 0]

                # Copy only the small refined chunk results to CPU
                closest_cpu = closest_idx.cpu().numpy()
                energy_cpu = energy.cpu().numpy()
                W_chunk_cpu = W_chunk.cpu().numpy()

                # Select the minimum-energy proposal for every target site
                # without a Python loop over all 30M input points.
                order = np.lexsort((energy_cpu, closest_cpu))
                sorted_sites = closest_cpu[order]
                unique_sites, first = np.unique(sorted_sites, return_index=True)
                chosen = order[first]
                better = energy_cpu[chosen] < best_energies_cpu[unique_sites]
                update_sites = unique_sites[better]
                update_rows = chosen[better]
                best_energies_cpu[update_sites] = energy_cpu[update_rows]
                best_coords_cpu[update_sites] = W_chunk_cpu[update_rows]

            active_mask = best_energies_cpu < np.inf
            active_idx = np.where(active_mask)[0]

            W_coords_cpu = best_coords_cpu[active_idx]
            W_scores_cpu = best_energies_cpu[active_idx]

            if len(W_coords_cpu) > self.W1_budget:
                top_idx = np.argpartition(W_scores_cpu, self.W1_budget - 1)[:self.W1_budget]
                W_coords_cpu = W_coords_cpu[top_idx]

            W_coords = torch.from_numpy(W_coords_cpu).to(device)
            w_pool = WitnessPool(W_coords, rank=1)

        def save_witness_checkpoint(pool):
            if checkpoint_dir:
                w_path = os.path.join(checkpoint_dir, f"witnesses_rank_{pool.rank}.pt")
                _save_atomic({
                    'coords': pool.coords,
                    'rank': pool.rank,
                    'scores': pool.scores,
                    'signatures': pool.signatures,
                }, w_path)

        print(f"[PERG-HGP] Running progressive rank loop through K+1 = {self.K + 1}...")
        if not pool_is_refined:
            print("  - Refining rank 1...")
            w_pool = refine_witness_pool(w_pool, Z, a, grid_z, cfg)
            save_witness_checkpoint(w_pool)

        while w_pool.rank < self.K + 1:
            target_rank = w_pool.rank + 1
            print(f"  - Lifting and refining rank {target_rank} / {self.K + 1}...")
            w_pool = lift_witness_pool(w_pool, Z, a, grid_z, cfg)
            if w_pool.rank != target_rank:
                raise RuntimeError("witness lift returned an inconsistent rank")
            w_pool = refine_witness_pool(w_pool, Z, a, grid_z, cfg)
            save_witness_checkpoint(w_pool)

        w_pool_final = w_pool
        self.final_witness_rank_ = w_pool_final.rank

        # 6 & 7. Extract & Certify Cofaces
        certified_cofaces = None
        certified_centers = None
        certified_weights = None
        candidate_count = 0

        cofaces_path = os.path.join(checkpoint_dir, "certified_cofaces.pt") if checkpoint_dir else None
        if cofaces_path and os.path.exists(cofaces_path):
            print(f"[PERG-HGP] Loading certified cofaces from checkpoint {cofaces_path}...")
            checkpoint_cof = torch.load(cofaces_path, map_location=device, weights_only=False)
            certified_cofaces = checkpoint_cof['cofaces']
            certified_centers = checkpoint_cof['centers']
            certified_weights = checkpoint_cof['weights']
            candidate_count = int(checkpoint_cof.get('candidate_count', certified_cofaces.shape[0]))
        else:
            print("[PERG-HGP] Extracting candidate cofaces from final witnesses...")
            eta_val = torch.mean(a).item()
            cofaces, centers, weights = extract_top_cofaces(w_pool_final, Z, a, eta_val, grid_z, self.K, cfg)
            candidate_count = int(cofaces.shape[0])

            # Gabriel Certification
            weights_64 = weights.to(torch.float64)
            radii_sq = torch.nextafter(
                weights_64 ** 2, torch.full_like(weights_64, float('inf'))
            )
            print(f"[PERG-HGP] Certifying {cofaces.shape[0]} unique candidate cofaces...")

            valid_mask = torch.ones(cofaces.shape[0], dtype=torch.bool, device=device)

            if self.local_gabriel_:
                nbr_indices_z, _ = grid_z.query_power_grid(Z, a, m_local=cfg.m_active)
                local_passes = local_gabriel_filter(cofaces, Z, a, centers, radii_sq, nbr_indices_z, tol=self.gabriel_tol)
                valid_mask &= local_passes
                print(f"  - Local filter: {torch.sum(local_passes).item()} / {cofaces.shape[0]} passed.")

            if self.global_gabriel_ in ['all', 'selective']:
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
                    "result_status": "empty_certified_atlas",
                    "model_exact_for_chosen_supports": True,
                    "regularization_model": "localized_exact_knn",
                    "regularization_support_size": self.m_local_,
                    "regularization_interleaving_radius": self.regularization_interleaving_radius_,
                    "power_knn_certified_for_stored_float32_sites": True,
                    "rank_tail_certified": False,
                    "accepted_cofaces_pass_top_consistency": None,
                    "accepted_cofaces_are_true_gabriel": None,
                    "numerical_certificate": "no_accepted_cofaces",
                    "atlas_hierarchy_exact": False,
                    "edge_induced_connectivity_exact_for_float32_edge_weights": True,
                    "condensed_flat_selection_heuristic": True,
                    "hgp_hierarchy_complete": False,
                    "full_power_hgp_complete": False,
                    "cut_certified": False,
                    "facet_births_exact": False,
                    "candidate_cofaces": candidate_count,
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
                self.cofaces_ = np.empty((0, self.K + 1), dtype=np.int64)
                self.coface_weights_ = np.empty(0, dtype=np.float64)
                self.facets_ = np.empty((0, self.K), dtype=np.int64)
                self.msf_ = {
                    "mst_u": np.empty(0, dtype=np.int32),
                    "mst_v": np.empty(0, dtype=np.int32),
                    "mst_w": np.empty(0, dtype=np.float32),
                    "mst_cof": np.empty(0, dtype=np.int32),
                }
                self.Z_tree_ = None
                self.merge_tree_ = None
                return self

            certified_cofaces = cofaces[certified_idx]
            certified_centers = centers[certified_idx]
            certified_weights = weights[certified_idx]
            if certified_cofaces.shape[0] > cfg.max_cofaces:
                raise MemoryError(
                    f"certified coface budget exceeded before dual construction: "
                    f"{certified_cofaces.shape[0]} > {cfg.max_cofaces}"
                )

            if cofaces_path:
                _save_atomic({
                    'cofaces': certified_cofaces,
                    'centers': certified_centers,
                    'weights': certified_weights,
                    'candidate_count': candidate_count,
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
            projected_edges = certified_cofaces.shape[0] * self.K
            if projected_edges > cfg.max_dual_edges:
                raise MemoryError(
                    f"dual edge budget exceeded before allocation: "
                    f"{projected_edges} > {cfg.max_dual_edges}"
                )
            facet_ids, unique_facets = compute_facet_ids(certified_cofaces, self.K, max_ram_facets=cfg.max_ram_facets)
            if unique_facets.shape[0] > cfg.max_unique_facets:
                raise MemoryError(
                    f"unique facet budget exceeded: "
                    f"{unique_facets.shape[0]} > {cfg.max_unique_facets}"
                )
            edges = build_dual_edges(certified_cofaces, facet_ids, certified_weights)

            print("[PERG-HGP] Computing dual MST...")
            num_facets = unique_facets.shape[0]
            mst_u, mst_v, mst_w, mst_cof = dual_graph_mst(lambda: edges.chunks(1000000), num_facets)
            num_dual_edges = certified_cofaces.shape[0] * self.K

            if mst_path:
                _save_atomic({
                    'mst_u': mst_u,
                    'mst_v': mst_v,
                    'mst_w': mst_w,
                    'mst_cof': mst_cof,
                    'facet_ids': facet_ids,
                    'unique_facets': unique_facets,
                    'num_dual_edges': num_dual_edges
                }, mst_path)

        # 10. Thesis face score and weighted condensed hierarchy
        num_facets = unique_facets.shape[0]
        S_faces = _compute_face_scores(facet_ids, certified_weights, num_facets, self.expZ)
        self.face_scores_ = S_faces
        unique_facets_cpu = unique_facets.cpu().numpy()

        tree_path = os.path.join(checkpoint_dir, "Z_tree.pt") if checkpoint_dir else None
        if tree_path and os.path.exists(tree_path):
            print(f"[PERG-HGP] Loading condensed tree from checkpoint {tree_path}...")
            self.Z_tree_ = torch.load(tree_path, map_location='cpu', weights_only=False)
        else:
            flat_facets = unique_facets_cpu.flatten()
            S_faces_expanded = np.repeat(S_faces, self.K)
            T_points = np.bincount(flat_facets, weights=S_faces_expanded, minlength=N).astype(np.float32)

            inv_T_points = np.divide(1.0, T_points, out=np.zeros_like(T_points), where=(T_points > 0.0))
            sum_inv_Tp_face = np.sum(inv_T_points[unique_facets_cpu], axis=1)
            W_nodes = S_faces * sum_inv_Tp_face

            print("[PERG-HGP] Condensing tree hierarchy...")
            self.Z_tree_ = condense_tree(W_nodes, mst_u, mst_v, mst_w, self.min_cluster_size)

            if tree_path:
                _save_atomic(self.Z_tree_, tree_path)

        # 11. Extract Point Labels via voting
        print("[PERG-HGP] Extracting final labels...")
        self.labels_ = extract_labels(self.Z_tree_, unique_facets_cpu, N, S_faces)

        W_coords_len = w_pool_final.coords.shape[0] if w_pool_final is not None else 0
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
            "result_status": "generated_atlas_only",
            "model_exact_for_chosen_supports": True,
            "regularization_model": "localized_exact_knn",
            "regularization_support_size": self.m_local_,
            "regularization_interleaving_radius": self.regularization_interleaving_radius_,
            "power_knn_certified_for_stored_float32_sites": True,
            "rank_tail_certified": False,
            "accepted_cofaces_pass_top_consistency": True,
            "accepted_cofaces_are_true_gabriel": None,
            "gabriel_certificate": "floating_point_top_k_plus_1_consistency",
            "numerical_certificate": "no_interval_bounds",
            "global_gabriel_audit": self.global_gabriel_ == 'all',
            "atlas_hierarchy_exact": False,
            "edge_induced_connectivity_exact_for_float32_edge_weights": True,
            "condensed_flat_selection_heuristic": True,
            "hgp_hierarchy_complete": False,
            "full_power_hgp_complete": False,
            "cut_certified": False,
            "facet_births_exact": False,
            "face_score": "incident_coface_sum",
            "candidate_cofaces": candidate_count,
            "certified_cofaces": cof_len,
            "num_facets": num_facets,
            "witness_budget_exceeded": (W_coords_len > cfg.max_witnesses_per_rank),
            "cofaces_budget_exceeded": (candidate_count > cfg.max_cofaces),
            "facets_budget_exceeded": (num_facets > cfg.max_unique_facets),
            "edges_budget_exceeded": (num_dual_edges > cfg.max_dual_edges),
            "total_knn_queries": total_queries,
            "total_fallback_queries": total_fallbacks,
            "global_fallback_rate": total_fallbacks / max(1, total_queries)
        }

        # Expose artifact properties as fitted attributes
        self.cofaces_ = certified_cofaces.cpu().numpy() if certified_cofaces is not None else None
        self.coface_weights_ = certified_weights.cpu().numpy() if certified_weights is not None else None
        self.facets_ = unique_facets.cpu().numpy()
        self.msf_ = {
            "mst_u": mst_u.cpu().numpy() if isinstance(mst_u, torch.Tensor) else mst_u,
            "mst_v": mst_v.cpu().numpy() if isinstance(mst_v, torch.Tensor) else mst_v,
            "mst_w": mst_w.cpu().numpy() if isinstance(mst_w, torch.Tensor) else mst_w,
            "mst_cof": mst_cof.cpu().numpy() if isinstance(mst_cof, torch.Tensor) else mst_cof
        }
        self.merge_tree_ = self.Z_tree_

        print("[PERG-HGP] Complete.")
        return self

    def fit_predict(self, X, y=None):
        self.fit(X)
        return self.labels_
