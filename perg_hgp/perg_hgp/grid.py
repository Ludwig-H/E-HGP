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
        if self.scale < 1e-12:
            self.scale = 1.0
            
        self.X_normalized = (self.X - self.bbox_min) / self.scale
        
        # 2. Compute flat cell keys on GPU
        cell_coords = (self.X_normalized * self.grid_resolution).clamp(0, self.grid_resolution - 1).to(torch.int64)
        self.cell_keys = cell_coords[:, 0] * (self.grid_resolution**2) + cell_coords[:, 1] * self.grid_resolution + cell_coords[:, 2]
        
        # 3. Sort keys and keep track of original indices
        self.sorted_keys, self.sorted_indices = torch.sort(self.cell_keys)
        self.X_sorted = self.X[self.sorted_indices]
        self.X_norm_sorted = self.X_normalized[self.sorted_indices]
        
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
        
        # Initialize output tensors
        nbr_indices = torch.zeros((M, m_local), dtype=torch.int64, device=device)
        nbr_dists_sq = torch.zeros((M, m_local), dtype=torch.float32, device=device)
        
        # Normalize queries
        q_norm = (query_points - self.bbox_min) / self.scale
        
        # Compute cell coordinates
        q_cell_coords = (q_norm * self.grid_resolution).clamp(0, self.grid_resolution - 1).to(torch.int64)
        
        # Active mask: which queries still need to be processed
        active_mask = torch.ones(M, dtype=torch.bool, device=device)
        
        # We search cell neighborhoods progressively
        radius = 1
        max_radius = 8
        
        # We accumulate candidates for each query
        cand_indices = torch.empty((M, 0), dtype=torch.int64, device=device)
        cand_dists_sq = torch.empty((M, 0), dtype=torch.float32, device=device)
        
        while radius <= max_radius and active_mask.any():
            r_search = radius - 1
            
            # Generate offsets for this radius shell
            offsets_list = []
            for dx in range(-r_search, r_search + 1):
                for dy in range(-r_search, r_search + 1):
                    for dz in range(-r_search, r_search + 1):
                        if r_search == 0 or max(abs(dx), abs(dy), abs(dz)) == r_search:
                            offsets_list.append([dx, dy, dz])
            offsets = torch.tensor(offsets_list, dtype=torch.int64, device=device) # (O, 3)
            O = offsets.shape[0]
            
            # For all active queries, find neighbor cell keys
            active_idx = torch.where(active_mask)[0]
            M_active = active_idx.shape[0]
            
            cx = q_cell_coords[active_idx, 0].unsqueeze(1) # (M_active, 1)
            cy = q_cell_coords[active_idx, 1].unsqueeze(1)
            cz = q_cell_coords[active_idx, 2].unsqueeze(1)
            
            nx = cx + offsets[:, 0].unsqueeze(0) # (M_active, O)
            ny = cy + offsets[:, 1].unsqueeze(0)
            nz = cz + offsets[:, 2].unsqueeze(0)
            
            mask_valid = (nx >= 0) & (nx < self.grid_resolution) & \
                         (ny >= 0) & (ny < self.grid_resolution) & \
                         (nz >= 0) & (nz < self.grid_resolution)
                         
            n_cell_key = nx * (self.grid_resolution**2) + ny * self.grid_resolution + nz
            # Clamped key to prevent out of bounds indexing
            n_cell_key_clamped = torch.clamp(n_cell_key, max=self.grid_resolution**3 - 1)
            
            # Look up cell index using binary search on unique keys
            n_cell_idx = torch.searchsorted(self.unique_cell_keys, n_cell_key_clamped) # (M_active, O)
            clamped_lookup_idx = torch.clamp(n_cell_idx, max=len(self.unique_cell_keys) - 1)
            mask_match = (n_cell_idx < len(self.unique_cell_keys)) & (self.unique_cell_keys[clamped_lookup_idx] == n_cell_key_clamped)
            n_cell_idx = torch.where(mask_match & mask_valid, n_cell_idx, torch.tensor(-1, device=device))
            
            # Gather candidates for each offset
            active_cand_idx_list = []
            active_cand_dist_list = []
            
            for o in range(O):
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
                diff = self.X[point_indices_clamped] - query_points[active_idx].unsqueeze(1) # (M_active, max_count, 3)
                dist_sq = torch.sum(diff ** 2, dim=2) # (M_active, max_count)
                
                dist_sq = torch.where(mask, dist_sq, torch.tensor(float('inf'), device=device))
                
                active_cand_idx_list.append(point_indices)
                active_cand_dist_list.append(dist_sq)
                
            if active_cand_idx_list:
                new_cand_idx = torch.cat(active_cand_idx_list, dim=1) # (M_active, C_new)
                new_cand_dist = torch.cat(active_cand_dist_list, dim=1) # (M_active, C_new)
                
                # Merge with previous candidates
                prev_active_idx = cand_indices[active_idx]
                prev_active_dist = cand_dists_sq[active_idx]
                
                merged_idx = torch.cat([prev_active_idx, new_cand_idx], dim=1)
                merged_dist = torch.cat([prev_active_dist, new_cand_dist], dim=1)
                
                # Find top-m_local closest in the merged candidate list
                val, idx = torch.topk(merged_dist, min(m_local, merged_dist.shape[1]), dim=1, largest=False)
                nbr_indices_active = torch.gather(merged_idx, 1, idx)
                nbr_dists_sq_active = val
                
                # Pad if we have fewer than m_local neighbors
                if nbr_indices_active.shape[1] < m_local:
                    pad_len = m_local - nbr_indices_active.shape[1]
                    pad_idx = nbr_indices_active[:, -1:].repeat(1, pad_len)
                    pad_dist = nbr_dists_sq_active[:, -1:].repeat(1, pad_len)
                    nbr_indices_active = torch.cat([nbr_indices_active, pad_idx], dim=1)
                    nbr_dists_sq_active = torch.cat([nbr_dists_sq_active, pad_dist], dim=1)
                    
                # Update global output for active queries
                nbr_indices[active_idx] = nbr_indices_active
                nbr_dists_sq[active_idx] = nbr_dists_sq_active
                
                # Save the merged candidates back for the next iteration
                new_size = merged_idx.shape[1]
                if cand_indices.shape[1] < new_size:
                    pad_len = new_size - cand_indices.shape[1]
                    cand_indices = torch.cat([cand_indices, torch.full((M, pad_len), -1, dtype=torch.int64, device=device)], dim=1)
                    cand_dists_sq = torch.cat([cand_dists_sq, torch.full((M, pad_len), float('inf'), dtype=torch.float32, device=device)], dim=1)
                cand_indices[active_idx, :new_size] = merged_idx
                cand_dists_sq[active_idx, :new_size] = merged_dist
                
            # Check certified stopping condition for active queries
            valid_counts = torch.sum(cand_dists_sq[active_idx] < float('inf'), dim=1)
            has_enough = valid_counts >= m_local
            
            d_max_sq = nbr_dists_sq[active_idx, -1] # (M_active,)
            
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
            
            certified = has_enough & (d_max_sq <= d_out_min_sq + 1e-6)
            
            # Deactivate certified queries
            new_active_mask = active_mask.clone()
            new_active_mask[active_idx[certified]] = False
            active_mask = new_active_mask
            
            radius += 1
            
        # Fallback to exact global scan for queries that are still active
        self.fallback_count_ = torch.sum(active_mask).item()
        self.fallback_rate_ = self.fallback_count_ / M
        self.certified_rate_ = 1.0 - self.fallback_rate_
        
        if active_mask.any():
            active_idx = torch.where(active_mask)[0]
            
            # Double-chunk fallback: chunk both queries (chunk_query) and database points (chunk_db)
            # to strictly bound memory consumption for O(M_active * N) scans.
            chunk_query = 100
            chunk_db = 100000
            
            for s_q in range(0, active_idx.shape[0], chunk_query):
                e_q = min(s_q + chunk_query, active_idx.shape[0])
                chunk_active = active_idx[s_q:e_q]
                Q_c = chunk_active.shape[0]
                
                chunk_nbrs_dist = torch.full((Q_c, m_local), float('inf'), device=device)
                chunk_nbrs_idx = torch.zeros((Q_c, m_local), dtype=torch.int64, device=device)
                
                for s_db in range(0, self.n_points, chunk_db):
                    e_db = min(s_db + chunk_db, self.n_points)
                    db_chunk = self.X[s_db:e_db] # (db_c, 3)
                    
                    diff = db_chunk.unsqueeze(0) - query_points[chunk_active].unsqueeze(1) # (Q_c, db_c, 3)
                    dist_sq = torch.sum(diff ** 2, dim=2) # (Q_c, db_c)
                    
                    merged_dist = torch.cat([chunk_nbrs_dist, dist_sq], dim=1) # (Q_c, m_local + db_c)
                    db_indices = torch.arange(s_db, e_db, device=device).unsqueeze(0).repeat(Q_c, 1)
                    merged_idx = torch.cat([chunk_nbrs_idx, db_indices], dim=1)
                    
                    val, idx = torch.topk(merged_dist, m_local, dim=1, largest=False)
                    chunk_nbrs_dist = val
                    chunk_nbrs_idx = torch.gather(merged_idx, 1, idx)
                    
                nbr_indices[chunk_active] = chunk_nbrs_idx
                nbr_dists_sq[chunk_active] = chunk_nbrs_dist
                
        self.total_queries_ += M
        self.total_fallbacks_ += self.fallback_count_
        return nbr_indices, nbr_dists_sq
