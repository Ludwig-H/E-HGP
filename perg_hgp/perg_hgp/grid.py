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
    Organizes points in Morton-sorted order to enable fast localized grid searches.
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
        
        # 2. Compute Morton keys
        # For grid_resolution=64, 6 bits per axis are enough (2^6 = 64)
        # But we can use bits_per_axis=21 for general high-resolution and shift
        self.morton_keys = morton3D_encode_pytorch(self.X_normalized, bits_per_axis=21)
        
        # 3. Sort keys and keep track of original indices
        self.sorted_keys, self.sorted_indices = torch.sort(self.morton_keys)
        self.X_sorted = self.X[self.sorted_indices]
        self.X_norm_sorted = self.X_normalized[self.sorted_indices]
        
        # 4. Define cells by taking the top bits of Morton keys corresponding to grid_resolution
        # e.g., grid_resolution=64 (6 bits per axis, 18 bits total)
        # We shift right by (21 - 6) * 3 = 45 bits
        bits_resolution = int(np.log2(grid_resolution))
        shift_bits = (21 - bits_resolution) * 3
        self.cell_keys = self.sorted_keys >> shift_bits
        
        # 5. Build start and count pointers for each cell
        # cell_keys is sorted, so we can find cell transitions
        diff = self.cell_keys[1:] != self.cell_keys[:-1]
        transitions = torch.where(diff)[0] + 1
        
        # Start and end positions of each cell in sorted arrays
        cell_starts = torch.cat([torch.tensor([0], device=self.device), transitions])
        cell_ends = torch.cat([transitions, torch.tensor([self.n_points], device=self.device)])
        
        self.unique_cell_keys = self.cell_keys[cell_starts]
        self.cell_counts = cell_ends - cell_starts
        
        # Fast lookup map from cell key to start position
        # We use a hash map or sorted search (binary search via torch.bucketize)
        self.cell_key_to_idx = {key.item(): idx for idx, key in enumerate(self.unique_cell_keys)}
        
        self.cell_starts = cell_starts
        self.cell_ends = cell_ends

    def get_points_in_cell(self, cell_key):
        idx = self.cell_key_to_idx.get(cell_key, None)
        if idx is None:
            return torch.empty(0, dtype=torch.int64, device=self.device)
        start = self.cell_starts[idx]
        end = self.cell_ends[idx]
        return self.sorted_indices[start:end]

    def query_knn_grid(self, query_points, m_local=128):
        """
        Fast localized KNN search using the grid.
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
        
        # Compute cell keys of query points
        q_morton = morton3D_encode_pytorch(q_norm, bits_per_axis=21)
        bits_resolution = int(np.log2(self.grid_resolution))
        shift_bits = (21 - bits_resolution) * 3
        q_cell_keys = q_morton >> shift_bits
        
        # Decode cell keys on CPU for fast python loops
        q_cell_keys_cpu = q_cell_keys.cpu().numpy()
        
        def decode_cell_key_cpu(key, bits):
            x = np.zeros_like(key)
            y = np.zeros_like(key)
            z = np.zeros_like(key)
            for b in range(bits):
                x |= ((key >> (3 * b)) & 1) << b
                y |= ((key >> (3 * b + 1)) & 1) << b
                z |= ((key >> (3 * b + 2)) & 1) << b
            return x, y, z
            
        def encode_cell_coords_cpu(x, y, z, bits):
            key = 0
            for b in range(bits):
                key |= ((x >> b) & 1) << (3 * b)
                key |= ((y >> b) & 1) << (3 * b + 1)
                key |= ((z >> b) & 1) << (3 * b + 2)
            return key
            
        qx, qy, qz = decode_cell_key_cpu(q_cell_keys_cpu, bits_resolution)
        
        # Search cell neighborhoods
        for i in range(M):
            cx, cy, cz = qx[i], qy[i], qz[i]
            q_pt = query_points[i]
            
            # Search candidates from cells in radius
            candidates = []
            radius = 1
            max_radius = 4
            
            while len(candidates) < m_local and radius <= max_radius:
                candidates_list = []
                for dx in range(-radius, radius + 1):
                    nx = cx + dx
                    if nx < 0 or nx >= self.grid_resolution: continue
                    for dy in range(-radius, radius + 1):
                        ny = cy + dy
                        if ny < 0 or ny >= self.grid_resolution: continue
                        for dz in range(-radius, radius + 1):
                            nz = cz + dz
                            if nz < 0 or nz >= self.grid_resolution: continue
                            
                            n_cell_key = encode_cell_coords_cpu(nx, ny, nz, bits_resolution)
                            n_points_idx = self.get_points_in_cell(n_cell_key)
                            if len(n_points_idx) > 0:
                                candidates_list.append(n_points_idx)
                if candidates_list:
                    candidates = torch.cat(candidates_list)
                radius += 1
                
            if len(candidates) == 0:
                # Fallback to all points sorted by distance (on a subset if too large)
                if self.n_points <= 5000:
                    candidates = torch.arange(self.n_points, device=self.device)
                else:
                    candidates = torch.randperm(self.n_points, device=self.device)[:5000]
                    
            # Compute distance to query
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
