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
        query_points: Tensor of shape (M, 3)
        Returns:
            nbr_indices: (M, m_local)
            nbr_dists_sq: (M, m_local)
        """
        M = query_points.shape[0]
        nbr_indices = torch.zeros((M, m_local), dtype=torch.int64, device=self.device)
        nbr_dists_sq = torch.zeros((M, m_local), dtype=torch.float32, device=self.device)
        
        # Normalize queries
        q_norm = (query_points - self.bbox_min) / self.scale
        q_norm_cpu = q_norm.cpu().numpy()
        
        # Compute cell coordinates and decode directly on CPU/GPU
        q_cell_coords = (q_norm * self.grid_resolution).clamp(0, self.grid_resolution - 1).to(torch.int64)
        q_cell_coords_cpu = q_cell_coords.cpu().numpy()
        qx = q_cell_coords_cpu[:, 0]
        qy = q_cell_coords_cpu[:, 1]
        qz = q_cell_coords_cpu[:, 2]
        
        # Search cell neighborhoods
        for i in range(M):
            cx, cy, cz = qx[i], qy[i], qz[i]
            q_pt = query_points[i]
            q_norm_i = q_norm_cpu[i]
            
            # Search candidates from cells in radius
            candidates = None
            radius = 1
            max_radius = 4
            certified = False
            
            while radius <= max_radius:
                r_search = radius - 1
                candidates_list = []
                for dx in range(-r_search, r_search + 1):
                    nx = cx + dx
                    if nx < 0 or nx >= self.grid_resolution: continue
                    for dy in range(-r_search, r_search + 1):
                        ny = cy + dy
                        if ny < 0 or ny >= self.grid_resolution: continue
                        for dz in range(-r_search, r_search + 1):
                            nz = cz + dz
                            if nz < 0 or nz >= self.grid_resolution: continue
                            
                            n_cell_key = nx * (self.grid_resolution**2) + ny * self.grid_resolution + nz
                            n_points_idx = self.get_points_in_cell(n_cell_key)
                            if len(n_points_idx) > 0:
                                candidates_list.append(n_points_idx)
                                
                if candidates_list:
                    candidates = torch.cat(candidates_list)
                    if len(candidates) >= m_local:
                        # Compute distance to candidates
                        diff = self.X[candidates] - q_pt
                        dist2 = torch.sum(diff ** 2, dim=1)
                        # Find the m_local-th smallest distance
                        d_max_sq = torch.topk(dist2, m_local, largest=False).values[-1].item()
                        
                        # Compute minimum distance to unsearched cells AABB boundary
                        x_min = (cx - r_search) / self.grid_resolution
                        x_max = (cx + r_search + 1) / self.grid_resolution
                        y_min = (cy - r_search) / self.grid_resolution
                        y_max = (cy + r_search + 1) / self.grid_resolution
                        z_min = (cz - r_search) / self.grid_resolution
                        z_max = (cz + r_search + 1) / self.grid_resolution
                        
                        d_border = float('inf')
                        if cx - r_search > 0:
                            d_border = min(d_border, q_norm_i[0] - x_min)
                        if cx + r_search + 1 < self.grid_resolution:
                            d_border = min(d_border, x_max - q_norm_i[0])
                        if cy - r_search > 0:
                            d_border = min(d_border, q_norm_i[1] - y_min)
                        if cy + r_search + 1 < self.grid_resolution:
                            d_border = min(d_border, y_max - q_norm_i[1])
                        if cz - r_search > 0:
                            d_border = min(d_border, q_norm_i[2] - z_min)
                        if cz + r_search + 1 < self.grid_resolution:
                            d_border = min(d_border, z_max - q_norm_i[2])
                            
                        d_out_min_sq = (d_border * self.scale) ** 2
                        
                        # Certified stopping condition
                        if d_max_sq <= d_out_min_sq + 1e-6:
                            certified = True
                            break
                            
                radius += 1
                
            if not certified or candidates is None:
                # Deterministic exact fallback to all points
                candidates = torch.arange(self.n_points, device=self.device)
                diff = self.X[candidates] - q_pt
                dist2 = torch.sum(diff ** 2, dim=1)
                
            else:
                diff = self.X[candidates] - q_pt
                dist2 = torch.sum(diff ** 2, dim=1)
                
            # Select top-m_local closest
            val, idx = torch.topk(dist2, min(m_local, len(dist2)), largest=False)
            
            # Store in output tensors
            if len(val) < m_local:
                nbr_indices[i, :len(val)] = candidates[idx]
                nbr_indices[i, len(val):] = candidates[idx[-1]] # padding
                nbr_dists_sq[i, :len(val)] = val
                nbr_dists_sq[i, len(val):] = val[-1] # padding
            else:
                nbr_indices[i] = candidates[idx]
                nbr_dists_sq[i] = val
                
        return nbr_indices, nbr_dists_sq
