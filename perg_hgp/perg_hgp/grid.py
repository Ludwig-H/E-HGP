import torch
import numpy as np

def morton3D_encode_pytorch(coords, bits_per_axis=21):
    """
    Vectorized 3D Morton encoding in PyTorch.
    coords: PyTorch tensor of shape (N, 3) in [0, 1].
    Returns: PyTorch tensor of shape (N,) of dtype=torch.int64.
    """
    device = coords.device
    # Scale coordinates to integer grid of size 2^bits_per_axis
    max_val = (1 << bits_per_axis) - 1
    xyz = (coords * max_val).clamp(0, max_val).to(torch.int64)
    
    # Interleave bits of x, y, z
    # Morton key = ... z2 y2 x2 z1 y1 x1 z0 y0 x0
    x = xyz[:, 0]
    y = xyz[:, 1]
    z = xyz[:, 2]
    
    # Interleave bits using standard bit manipulation masks
    # We do it by spreading bits
    def spread_bits(val):
        # val has 21 bits. Spread them by inserting 2 zeros between each bit.
        # e.g. bit 20 is shifted to position 60, bit 19 to 57, etc.
        val = (val | (val << 32)) & 0x7fff00000000ffff
        val = (val | (val << 16)) & 0x00ff0000ff0000ff
        val = (val | (val << 8))  & 0x700f00f00f00f00f
        val = (val | (val << 4))  & 0x30c30c30c30c30c3
        val = (val | (val << 2))  & 0x1249249249249249
        return val
    
    morton = spread_bits(x) | (spread_bits(y) << 1) | (spread_bits(z) << 2)
    return morton

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
        
        # 5. Fast GPU flat lookup map from cell key to start/end index
        self.cell_starts_map = torch.full((self.grid_resolution**3,), -1, dtype=torch.int64, device=self.device)
        self.cell_starts_map[self.unique_cell_keys] = torch.arange(len(self.unique_cell_keys), device=self.device)

    def get_points_in_cell(self, cell_key):
        idx = self.cell_starts_map[cell_key].item()
        if idx < 0:
            return torch.empty(0, dtype=torch.int64, device=self.device)
        start = self.cell_starts[idx]
        end = self.cell_ends[idx]
        return self.sorted_indices[start:end]

    def query_knn_grid(self, query_points, m_local=128):
        """
        Fast certified localized KNN search using the grid.
        Guarantees 100% exactness using a queue-certificate (stopping condition)
        and deterministic fallback to global scan if the neighborhood is uncertified.
        Fully vectorized in PyTorch without python loops over query points.
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
        max_radius = 4
        
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
            
            # Look up cell index
            n_cell_idx = self.cell_starts_map[n_cell_key_clamped] # (M_active, O)
            n_cell_idx = torch.where(mask_valid, n_cell_idx, torch.tensor(-1, device=device))
            
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
        if active_mask.any():
            active_idx = torch.where(active_mask)[0]
            
            # Chunk fallback to prevent GPU memory spikes if active_idx is large
            chunk_size = 10000
            for s_idx in range(0, active_idx.shape[0], chunk_size):
                e_idx = min(s_idx + chunk_size, active_idx.shape[0])
                chunk_active = active_idx[s_idx:e_idx]
                
                diff = self.X.unsqueeze(0) - query_points[chunk_active].unsqueeze(1) # (M_chunk, N, 3)
                dist_sq = torch.sum(diff ** 2, dim=2) # (M_chunk, N)
                
                val, idx = torch.topk(dist_sq, m_local, dim=1, largest=False)
                nbr_indices[chunk_active] = idx
                nbr_dists_sq[chunk_active] = val
                
        return nbr_indices, nbr_dists_sq
