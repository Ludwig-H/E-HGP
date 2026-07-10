import torch
import numpy as np

class SpatialGrid3D:
    """
    Spatial Grid for fast 3D neighbor queries.
    Organizes points in flat cell index sorted order to enable fast localized grid searches.
    """
    def __init__(self, X, grid_resolution=64, device='cpu'):
        self.device = torch.device(device)
        self.grid_resolution = grid_resolution

        # Ensure X is a PyTorch tensor
        if isinstance(X, np.ndarray):
            self.X = torch.from_numpy(X).to(self.device, dtype=torch.float32)
        else:
            self.X = X.to(self.device, dtype=torch.float32)

        self.n_points = self.X.shape[0]

        # 1. Normalize points in [0, 1]^3
        self.bbox_min = self.X.min(dim=0).values
        self.bbox_max = self.X.max(dim=0).values
        self.scale = (self.bbox_max - self.bbox_min).max()
        if self.scale.item() < 1e-12:
            self.scale = torch.ones((), dtype=self.X.dtype, device=self.device)

        self.X_normalized = (self.X - self.bbox_min) / self.scale

        # 2. Compute flat cell keys on GPU
        cell_coords = (self.X_normalized * self.grid_resolution).clamp(0, self.grid_resolution - 1).to(torch.int64)
        self.cell_keys = cell_coords[:, 0] * (self.grid_resolution**2) + cell_coords[:, 1] * self.grid_resolution + cell_coords[:, 2]

        # 3. Sort keys and keep track of original indices
        self.sorted_keys, self.sorted_indices = torch.sort(self.cell_keys)

        # 4. Build start and end pointers for each cell on GPU
        diff = self.sorted_keys[1:] != self.sorted_keys[:-1]
        transitions = torch.where(diff)[0] + 1

        cell_starts = torch.cat([torch.tensor([0], device=self.device), transitions])
        cell_ends = torch.cat([transitions, torch.tensor([self.n_points], device=self.device)])

        self.unique_cell_keys = self.sorted_keys[cell_starts]
        self.cell_starts = cell_starts
        self.cell_ends = cell_ends

        # Diagnostics
        self.fallback_count_ = 0
        self.fallback_rate_ = 0.0
        self.certified_rate_ = 1.0
        self.total_queries_ = 0
        self.total_fallbacks_ = 0

    def get_points_in_cell(self, cell_key):
        key_tensor = torch.tensor([cell_key], device=self.device, dtype=torch.int64)
        idx = torch.searchsorted(self.unique_cell_keys, key_tensor)[0].item()
        if idx < len(self.unique_cell_keys) and self.unique_cell_keys[idx].item() == cell_key:
            start = self.cell_starts[idx]
            end = self.cell_ends[idx]
            return self.sorted_indices[start:end]
        return torch.empty(0, dtype=torch.int64, device=self.device)

    def _get_shell_offsets(self, radius):
        if not hasattr(self, '_shell_offsets_cache'):
            self._shell_offsets_cache = {}
        if radius not in self._shell_offsets_cache:
            r_search = radius - 1
            offsets_list = []
            for dx in range(-r_search, r_search + 1):
                for dy in range(-r_search, r_search + 1):
                    for dz in range(-r_search, r_search + 1):
                        if r_search == 0 or max(abs(dx), abs(dy), abs(dz)) == r_search:
                            offsets_list.append([dx, dy, dz])
            self._shell_offsets_cache[radius] = torch.tensor(offsets_list, dtype=torch.int64, device=self.device)
        return self._shell_offsets_cache[radius]

    def query_knn_grid(self, query_points, m_local=128):
        """
        Fast certified localized KNN search using the grid.
        Guarantees 100% exactness using a queue-certificate (stopping condition)
        and deterministic fallback to global scan if the neighborhood is uncertified.
        Fully vectorized in PyTorch without python loops over query points.
        Uses memory-efficient searchsorted lookup instead of dense hash map.
        query_points: Tensor of shape (M, 3)
        Returns:
            nbr_indices: (M, m_local)
            nbr_dists_sq: (M, m_local)
        """
        device = self.device
        M = query_points.shape[0]
        m_local = min(m_local, self.n_points)

        if M == 0:
            return (torch.empty((0, m_local), dtype=torch.int64, device=query_points.device),
                    torch.empty((0, m_local), dtype=torch.float32, device=query_points.device))

        # Adaptive chunk size based on m_local to strictly bound memory, capped at 10000 max
        grid_chunk_size = min(10000, max(1000, int(5000000 / m_local)))

        # Output tensors allocated on the same device as query_points
        nbr_indices = torch.zeros((M, m_local), dtype=torch.int64, device=query_points.device)
        nbr_dists_sq = torch.zeros((M, m_local), dtype=torch.float32, device=query_points.device)

        total_fallback_count = 0

        for start_q in range(0, M, grid_chunk_size):
            end_q = min(start_q + grid_chunk_size, M)
            q_pts_chunk = query_points[start_q:end_q]
            M_c = q_pts_chunk.shape[0]

            # Initialize outputs for this chunk on the query device
            chunk_nbr_indices = torch.zeros((M_c, m_local), dtype=torch.int64, device=q_pts_chunk.device)
            chunk_nbr_dists_sq = torch.zeros((M_c, m_local), dtype=torch.float32, device=q_pts_chunk.device)

            # Normalize queries for chunk
            q_norm = (q_pts_chunk - self.bbox_min) / self.scale

            # Compute cell coordinates
            q_cell_coords = (q_norm * self.grid_resolution).clamp(0, self.grid_resolution - 1).to(torch.int64)

            # Active mask: which queries still need to be processed
            active_mask = torch.ones(M_c, dtype=torch.bool, device=device)

            # We search cell neighborhoods progressively
            radius = 1
            max_radius = min(24, self.grid_resolution)  # Prevent searching beyond grid resolution bounds

            # Running top-m_local candidates to keep memory bounded at O(M_active * m_local)
            cand_indices = torch.full((M_c, m_local), -1, dtype=torch.int64, device=device)
            cand_dists_sq = torch.full((M_c, m_local), float('inf'), dtype=torch.float32, device=device)

            while radius <= max_radius and active_mask.any():
                r_search = radius - 1

                # Fetch precomputed/cached offsets for this radius shell
                offsets = self._get_shell_offsets(radius)
                O = offsets.shape[0]

                # For all active queries, find neighbor cell keys
                active_idx = torch.where(active_mask)[0]
                M_active = active_idx.shape[0]

                cx = q_cell_coords[active_idx, 0].unsqueeze(1) # (M_active, 1)
                cy = q_cell_coords[active_idx, 1].unsqueeze(1)
                cz = q_cell_coords[active_idx, 2].unsqueeze(1)

                # Chunk offsets loop *before* building nx/ny/nz/n_cell_idx to keep peak memory strictly O(M_active * offset_chunk_size)
                offset_chunk_size = 32
                for o_start in range(0, O, offset_chunk_size):
                    o_end = min(o_start + offset_chunk_size, O)
                    offsets_chunk = offsets[o_start:o_end]
                    O_c = offsets_chunk.shape[0]

                    nx = cx + offsets_chunk[:, 0].unsqueeze(0) # (M_active, O_c)
                    ny = cy + offsets_chunk[:, 1].unsqueeze(0)
                    nz = cz + offsets_chunk[:, 2].unsqueeze(0)

                    mask_valid = (nx >= 0) & (nx < self.grid_resolution) & \
                                 (ny >= 0) & (ny < self.grid_resolution) & \
                                 (nz >= 0) & (nz < self.grid_resolution)

                    n_cell_key = nx * (self.grid_resolution**2) + ny * self.grid_resolution + nz
                    # Clamped key to prevent out of bounds indexing
                    n_cell_key_clamped = torch.clamp(n_cell_key, max=self.grid_resolution**3 - 1)

                    # Look up cell index using binary search on unique keys
                    n_cell_idx = torch.searchsorted(self.unique_cell_keys, n_cell_key_clamped) # (M_active, O_c)
                    clamped_lookup_idx = torch.clamp(n_cell_idx, max=len(self.unique_cell_keys) - 1)
                    mask_match = (n_cell_idx < len(self.unique_cell_keys)) & (self.unique_cell_keys[clamped_lookup_idx] == n_cell_key_clamped)
                    n_cell_idx = torch.where(mask_match & mask_valid, n_cell_idx, torch.tensor(-1, device=device))

                    active_cand_idx_list = []
                    active_cand_dist_list = []

                    for o in range(O_c):
                        c_idx = n_cell_idx[:, o] # (M_active,)
                        starts = torch.where(c_idx >= 0, self.cell_starts[c_idx], torch.tensor(0, device=device))
                        ends = torch.where(c_idx >= 0, self.cell_ends[c_idx], torch.tensor(0, device=device))
                        counts = ends - starts

                        max_count = counts.max().item()
                        if max_count == 0:
                            continue

                        offsets_local = torch.arange(max_count, device=device).unsqueeze(0) # (1, max_count)
                        gather_idx = starts.unsqueeze(1) + offsets_local # (M_active, max_count)
                        gather_idx = torch.clamp(gather_idx, max=self.n_points - 1)

                        point_indices = self.sorted_indices[gather_idx] # (M_active, max_count)
                        mask = (offsets_local < counts.unsqueeze(1)) & (c_idx.unsqueeze(1) >= 0)
                        point_indices = torch.where(mask, point_indices, torch.tensor(-1, device=device))

                        # Compute distances
                        point_indices_clamped = torch.clamp(point_indices, min=0)
                        diff = self.X[point_indices_clamped] - q_pts_chunk[active_idx].unsqueeze(1) # (M_active, max_count, 3)
                        dist_sq = torch.sum(diff ** 2, dim=2) # (M_active, max_count)

                        dist_sq = torch.where(mask, dist_sq, float('inf'))

                        active_cand_idx_list.append(point_indices)
                        active_cand_dist_list.append(dist_sq)

                    if active_cand_idx_list:
                        new_cand_idx = torch.cat(active_cand_idx_list, dim=1) # (M_active, C_new)
                        new_cand_dist = torch.cat(active_cand_dist_list, dim=1) # (M_active, C_new)

                        # Merge with running candidates
                        prev_active_idx = cand_indices[active_idx]
                        prev_active_dist = cand_dists_sq[active_idx]

                        merged_idx = torch.cat([prev_active_idx, new_cand_idx], dim=1)
                        merged_dist = torch.cat([prev_active_dist, new_cand_dist], dim=1)

                        # Find top-m_local closest in the merged candidate list
                        val, idx = torch.topk(merged_dist, m_local, dim=1, largest=False)
                        cand_indices[active_idx] = torch.gather(merged_idx, 1, idx)
                        cand_dists_sq[active_idx] = val

                # Update chunk outputs for active queries
                chunk_nbr_indices[active_idx] = cand_indices[active_idx]
                chunk_nbr_dists_sq[active_idx] = cand_dists_sq[active_idx]

                # Check certified stopping condition for active queries
                valid_counts = torch.sum(cand_dists_sq[active_idx] < float('inf'), dim=1)

                # Dynamic early stopping: if we have found enough points, OR if we have already found all points in the DB
                has_enough = (valid_counts >= m_local) | (valid_counts >= self.n_points)

                d_max_sq = chunk_nbr_dists_sq[active_idx, -1] # (M_active,)

                cx_active = q_cell_coords[active_idx, 0]
                cy_active = q_cell_coords[active_idx, 1]
                cz_active = q_cell_coords[active_idx, 2]
                q_norm_active = q_norm[active_idx]

                x_min = (cx_active - r_search) / self.grid_resolution
                x_max = (cx_active + r_search + 1) / self.grid_resolution
                y_min = (cy_active - r_search) / self.grid_resolution
                y_max = (cy_active + r_search + 1) / self.grid_resolution
                z_min = (cz_active - r_search) / self.grid_resolution
                z_max = (cz_active + r_search + 1) / self.grid_resolution

                d_border = torch.full((M_active,), float('inf'), device=device)
                d_border = torch.where(cx_active - r_search > 0, torch.min(d_border, q_norm_active[:, 0] - x_min), d_border)
                d_border = torch.where(cx_active + r_search + 1 < self.grid_resolution, torch.min(d_border, x_max - q_norm_active[:, 0]), d_border)
                d_border = torch.where(cy_active - r_search > 0, torch.min(d_border, q_norm_active[:, 1] - y_min), d_border)
                d_border = torch.where(cy_active + r_search + 1 < self.grid_resolution, torch.min(d_border, y_max - q_norm_active[:, 1]), d_border)
                d_border = torch.where(cz_active - r_search > 0, torch.min(d_border, q_norm_active[:, 2] - z_min), d_border)
                d_border = torch.where(cz_active + r_search + 1 < self.grid_resolution, torch.min(d_border, z_max - q_norm_active[:, 2]), d_border)

                d_out_min_sq = (d_border * self.scale) ** 2

                finite_border = torch.isfinite(d_out_min_sq)
                roundoff = 16.0 * torch.finfo(d_max_sq.dtype).eps * torch.maximum(
                    torch.abs(d_max_sq), torch.abs(d_out_min_sq)
                )
                separated = finite_border & (d_max_sq + roundoff < d_out_min_sq - roundoff)
                all_cells_covered = ~finite_border
                certified = has_enough & (separated | all_cells_covered)

                # Deactivate certified queries
                new_active_mask = active_mask.clone()
                new_active_mask[active_idx[certified]] = False
                active_mask = new_active_mask

                radius += 1

            # Fallback to exact global scan for queries that are still active
            chunk_fallback_count = torch.sum(active_mask).item()
            total_fallback_count += chunk_fallback_count

            if active_mask.any():
                active_idx_fb = torch.where(active_mask)[0]

                # Double-chunk fallback: chunk both queries (chunk_query) and database points (chunk_db)
                chunk_query = 100
                chunk_db = 100000

                for s_q in range(0, active_idx_fb.shape[0], chunk_query):
                    e_q = min(s_q + chunk_query, active_idx_fb.shape[0])
                    chunk_active = active_idx_fb[s_q:e_q]
                    Q_c = chunk_active.shape[0]

                    chunk_nbrs_dist = torch.full((Q_c, m_local), float('inf'), device=device)
                    chunk_nbrs_idx = torch.zeros((Q_c, m_local), dtype=torch.int64, device=device)

                    for s_db in range(0, self.n_points, chunk_db):
                        e_db = min(s_db + chunk_db, self.n_points)
                        db_chunk = self.X[s_db:e_db] # (db_c, 3)

                        diff = db_chunk.unsqueeze(0) - q_pts_chunk[chunk_active].unsqueeze(1) # (Q_c, db_c, 3)
                        dist_sq = torch.sum(diff ** 2, dim=2) # (Q_c, db_c)

                        merged_dist = torch.cat([chunk_nbrs_dist, dist_sq], dim=1) # (Q_c, m_local + db_c)
                        db_indices = torch.arange(s_db, e_db, device=device).unsqueeze(0).repeat(Q_c, 1)
                        merged_idx = torch.cat([chunk_nbrs_idx, db_indices], dim=1)

                        val, idx = torch.topk(merged_dist, m_local, dim=1, largest=False)
                        chunk_nbrs_dist = val
                        chunk_nbrs_idx = torch.gather(merged_idx, 1, idx)

                    chunk_nbr_indices[chunk_active] = chunk_nbrs_idx
                    chunk_nbr_dists_sq[chunk_active] = chunk_nbrs_dist

            # Copy chunk output back to global outputs (casting to the query device if necessary)
            nbr_indices[start_q:end_q] = chunk_nbr_indices.to(query_points.device)
            nbr_dists_sq[start_q:end_q] = chunk_nbr_dists_sq.to(query_points.device)

        self.fallback_count_ = total_fallback_count
        self.fallback_rate_ = self.fallback_count_ / M
        self.certified_rate_ = 1.0 - self.fallback_rate_
        self.total_queries_ += M
        self.total_fallbacks_ += self.fallback_count_
        return nbr_indices, nbr_dists_sq

    def _global_power_knn(self, query_points, weights, m_local):
        """Exact double-chunked power KNN used as a bounded-memory fallback."""
        device = self.device
        query_points = query_points.to(device)
        weights = weights.to(device)
        m_local = min(m_local, self.n_points)
        M = query_points.shape[0]
        out_idx = torch.zeros((M, m_local), dtype=torch.int64, device=device)
        out_dist = torch.full((M, m_local), float('inf'), dtype=torch.float32, device=device)

        query_chunk = 100
        db_chunk = 100000
        for start_q in range(0, M, query_chunk):
            end_q = min(start_q + query_chunk, M)
            q_chunk = query_points[start_q:end_q]
            q_len = q_chunk.shape[0]
            best_idx = torch.zeros((q_len, m_local), dtype=torch.int64, device=device)
            best_dist = torch.full((q_len, m_local), float('inf'), dtype=torch.float32, device=device)

            for start_db in range(0, self.n_points, db_chunk):
                end_db = min(start_db + db_chunk, self.n_points)
                db = self.X[start_db:end_db]
                diff = db.unsqueeze(0) - q_chunk.unsqueeze(1)
                dist = torch.sum(diff ** 2, dim=2) + weights[start_db:end_db].unsqueeze(0)
                db_idx = torch.arange(start_db, end_db, dtype=torch.int64, device=device)
                db_idx = db_idx.unsqueeze(0).expand(q_len, -1)

                merged_dist = torch.cat([best_dist, dist], dim=1)
                merged_idx = torch.cat([best_idx, db_idx], dim=1)
                best_dist, order = torch.topk(merged_dist, m_local, dim=1, largest=False)
                best_idx = torch.gather(merged_idx, 1, order)

            out_idx[start_q:end_q] = best_idx
            out_dist[start_q:end_q] = best_dist

        return out_idx, out_dist

    def query_power_grid(self, query_points, weights, m_local=128):
        """
        Vectorized certified neighborhood search in the power complex.
        Fully vectorized in PyTorch without python loops over query points.
        Uses memory-efficient searchsorted lookup instead of dense hash map.
        query_points: (M, 3)
        weights: (N,) - weights a of the sites
        """
        device = self.device
        M = query_points.shape[0]
        m_local = min(m_local, self.n_points)

        if M == 0:
            return (torch.empty((0, m_local), dtype=torch.int64, device=query_points.device),
                    torch.empty((0, m_local), dtype=torch.float32, device=query_points.device))

        # Move queries and weights to self.device
        q_pts = query_points.to(device)
        w_sites = weights.to(device)

        # Calculate cell size in normalized coords
        delta = 1.0 / self.grid_resolution
        delta_scaled = delta * self.scale
        min_a = float(w_sites.min().item())

        # Output tensors allocated on the same device as query_points
        nbr_indices = torch.zeros((M, m_local), dtype=torch.int64, device=query_points.device)
        nbr_dists_sq = torch.zeros((M, m_local), dtype=torch.float32, device=query_points.device)

        # Sparse high-resolution grids are dominated by empty-shell traversal.
        # For small databases an exact blocked scan is both faster and safer.
        if self.n_points <= 4096:
            exact_idx, exact_dist = self._global_power_knn(q_pts, w_sites, m_local)
            self.fallback_count_ = M
            self.fallback_rate_ = 1.0
            self.certified_rate_ = 0.0
            self.total_queries_ += M
            self.total_fallbacks_ += M
            return exact_idx.to(query_points.device), exact_dist.to(query_points.device)

        # Process in chunks to strictly bound VRAM/RAM
        grid_chunk_size = min(10000, max(1000, int(5000000 / m_local)))

        total_fallback_count = 0

        for start_q in range(0, M, grid_chunk_size):
            end_q = min(start_q + grid_chunk_size, M)
            q_pts_chunk = q_pts[start_q:end_q]
            M_c = q_pts_chunk.shape[0]

            # Map query points to flat cell keys
            q_norm = (q_pts_chunk - self.bbox_min) / self.scale
            q_cell_coords = (q_norm * self.grid_resolution).clamp(0, self.grid_resolution - 1).to(torch.int64)

            # Initialize running top-m_local candidates
            cand_indices = torch.full((M_c, m_local), -1, dtype=torch.int64, device=device)
            cand_dists_sq = torch.full((M_c, m_local), float('inf'), dtype=torch.float32, device=device)

            # Active mask: which queries still need to be processed
            active_mask = torch.ones(M_c, dtype=torch.bool, device=device)

            radius = 1
            max_search_radius = self.grid_resolution

            while radius <= max_search_radius and active_mask.any():
                r_search = radius - 1

                # Fetch precomputed/cached offsets for this radius shell
                offsets = self._get_shell_offsets(radius)
                O = offsets.shape[0]

                # For all active queries, find neighbor cell keys
                active_idx = torch.where(active_mask)[0]
                M_active = active_idx.shape[0]

                cx = q_cell_coords[active_idx, 0].unsqueeze(1) # (M_active, 1)
                cy = q_cell_coords[active_idx, 1].unsqueeze(1)
                cz = q_cell_coords[active_idx, 2].unsqueeze(1)

                # Chunk offsets loop before building nx/ny/nz/n_cell_idx to keep memory strictly O(M_active * offset_chunk_size)
                offset_chunk_size = 32
                for o_start in range(0, O, offset_chunk_size):
                    o_end = min(o_start + offset_chunk_size, O)
                    offsets_chunk = offsets[o_start:o_end]
                    O_c = offsets_chunk.shape[0]

                    nx = cx + offsets_chunk[:, 0].unsqueeze(0) # (M_active, O_c)
                    ny = cy + offsets_chunk[:, 1].unsqueeze(0)
                    nz = cz + offsets_chunk[:, 2].unsqueeze(0)

                    mask_valid = (nx >= 0) & (nx < self.grid_resolution) & \
                                 (ny >= 0) & (ny < self.grid_resolution) & \
                                 (nz >= 0) & (nz < self.grid_resolution)

                    n_cell_key = nx * (self.grid_resolution**2) + ny * self.grid_resolution + nz
                    # Clamped key to prevent out of bounds indexing
                    n_cell_key_clamped = torch.clamp(n_cell_key, max=self.grid_resolution**3 - 1)

                    # Look up cell index using binary search on unique keys
                    n_cell_idx = torch.searchsorted(self.unique_cell_keys, n_cell_key_clamped) # (M_active, O_c)
                    clamped_lookup_idx = torch.clamp(n_cell_idx, max=len(self.unique_cell_keys) - 1)
                    mask_match = (n_cell_idx < len(self.unique_cell_keys)) & (self.unique_cell_keys[clamped_lookup_idx] == n_cell_key_clamped)
                    n_cell_idx = torch.where(mask_match & mask_valid, n_cell_idx, torch.tensor(-1, device=device))

                    active_cand_idx_list = []
                    active_cand_dist_list = []

                    for o in range(O_c):
                        c_idx = n_cell_idx[:, o] # (M_active,)
                        starts = torch.where(c_idx >= 0, self.cell_starts[c_idx], torch.tensor(0, device=device))
                        ends = torch.where(c_idx >= 0, self.cell_ends[c_idx], torch.tensor(0, device=device))
                        counts = ends - starts

                        max_count = counts.max().item()
                        if max_count == 0:
                            continue

                        offsets_local = torch.arange(max_count, device=device).unsqueeze(0) # (1, max_count)
                        gather_idx = starts.unsqueeze(1) + offsets_local # (M_active, max_count)
                        gather_idx = torch.clamp(gather_idx, max=self.n_points - 1)

                        point_indices = self.sorted_indices[gather_idx] # (M_active, max_count)
                        mask = (offsets_local < counts.unsqueeze(1)) & (c_idx.unsqueeze(1) >= 0)
                        point_indices = torch.where(mask, point_indices, torch.tensor(-1, device=device))

                        # Compute distances with power metric!
                        point_indices_clamped = torch.clamp(point_indices, min=0)
                        diff = self.X[point_indices_clamped] - q_pts_chunk[active_idx].unsqueeze(1) # (M_active, max_count, 3)
                        dist_sq = torch.sum(diff ** 2, dim=2) + w_sites[point_indices_clamped] # (M_active, max_count)

                        dist_sq = torch.where(mask, dist_sq, float('inf'))

                        active_cand_idx_list.append(point_indices)
                        active_cand_dist_list.append(dist_sq)

                    if active_cand_idx_list:
                        new_cand_idx = torch.cat(active_cand_idx_list, dim=1) # (M_active, C_new)
                        new_cand_dist = torch.cat(active_cand_dist_list, dim=1) # (M_active, C_new)

                        # Merge with running candidates
                        prev_active_idx = cand_indices[active_idx]
                        prev_active_dist = cand_dists_sq[active_idx]

                        merged_idx = torch.cat([prev_active_idx, new_cand_idx], dim=1)
                        merged_dist = torch.cat([prev_active_dist, new_cand_dist], dim=1)

                        # Find top-m_local closest in the merged candidate list
                        val, idx = torch.topk(merged_dist, m_local, dim=1, largest=False)
                        cand_indices[active_idx] = torch.gather(merged_idx, 1, idx)
                        cand_dists_sq[active_idx] = val

                # Update the stopping criterion conservatively. Ambiguous
                # floating-point comparisons keep searching (or fall back).
                stopping_threshold = ((radius - 1) * delta_scaled) ** 2 + min_a
                kth = cand_dists_sq[active_idx, -1]
                threshold = torch.as_tensor(stopping_threshold, dtype=kth.dtype, device=device)
                roundoff = 16.0 * torch.finfo(kth.dtype).eps * torch.maximum(
                    torch.abs(kth), torch.abs(threshold)
                )
                enough = torch.isfinite(kth)
                separated = kth + roundoff < threshold - roundoff
                all_cells_covered = radius >= self.grid_resolution
                active_mask[active_idx] = ~(enough & (separated | all_cells_covered))

                radius += 1

            # Global scan for uncertified queries
            uncertified = active_mask
            if torch.any(uncertified):
                uncert_idx = torch.where(uncertified)[0]
                total_fallback_count += len(uncert_idx)

                q_uncert = q_pts_chunk[uncert_idx]
                idx_top, val_top = self._global_power_knn(q_uncert, w_sites, m_local)
                cand_dists_sq[uncert_idx] = val_top
                cand_indices[uncert_idx] = idx_top

            nbr_indices[start_q:end_q] = cand_indices.to(query_points.device)
            nbr_dists_sq[start_q:end_q] = cand_dists_sq.to(query_points.device)

        self.fallback_count_ = total_fallback_count
        self.fallback_rate_ = self.fallback_count_ / M
        self.certified_rate_ = 1.0 - self.fallback_rate_
        self.total_queries_ += M
        self.total_fallbacks_ += self.fallback_count_

        return nbr_indices, nbr_dists_sq
