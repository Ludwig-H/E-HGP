import torch
import numpy as np

def local_gabriel_filter(cofaces, Z, a, centers, radii_sq, nbr_indices, tol=1e-6):
    """
    Fast local Gabriel filter.
    Prunes cofaces that are clearly not Gabriel by checking only the union of neighbor supports.
    """
    device = Z.device
    U = cofaces.shape[0]
    passes = torch.ones(U, dtype=torch.bool, device=device)
    
    for i in range(U):
        cof = cofaces[i]
        c = centers[i]
        r2 = radii_sq[i]
        
        # Local support union
        union_support = torch.unique(nbr_indices[cof])
        
        # Exclude coface vertices
        mask = torch.ones(union_support.shape[0], dtype=torch.bool, device=device)
        for val in cof:
            mask &= (union_support != val)
        check_set = union_support[mask]
        
        if check_set.shape[0] == 0:
            continue
            
        # Compute power distance to check_set
        diff = Z[check_set] - c
        phi = torch.sum(diff ** 2, dim=1) + a[check_set]
        
        if torch.any(phi < r2 - tol):
            passes[i] = False
            
    return passes


def global_gabriel_grid_test(cofaces, Z, a, centers, radii_sq, grid_z, tol=1e-6):
    """
    Exact global Gabriel test using the spatial grid Z.
    Query grid cells intersecting the ball to certify that no site is an intruder.
    """
    device = Z.device
    U = cofaces.shape[0]
    passes = torch.ones(U, dtype=torch.bool, device=device)
    
    # Grid coordinates decoding/encoding
    bits_resolution = int(np.log2(grid_z.grid_resolution))
    
    def decode_cell_key(key):
        x = torch.zeros_like(key)
        y = torch.zeros_like(key)
        z = torch.zeros_like(key)
        for b in range(bits_resolution):
            x |= ((key >> (3 * b)) & 1) << b
            y |= ((key >> (3 * b + 1)) & 1) << b
            z |= ((key >> (3 * b + 2)) & 1) << b
        return x, y, z
    
    def encode_cell_coords(x, y, z):
        key = torch.zeros_like(x)
        for b in range(bits_resolution):
            key |= ((x >> b) & 1) << (3 * b)
            key |= ((y >> b) & 1) << (3 * b + 1)
            key |= ((z >> b) & 1) << (3 * b + 2)
        return key

    for i in range(U):
        cof = cofaces[i]
        c = centers[i]
        r2 = radii_sq[i]
        r = torch.sqrt(torch.clamp(r2, min=0.0))
        
        # 1. Normalize center
        c_norm = (c - grid_z.bbox_min) / grid_z.scale
        
        # 2. Get cell coordinate of the center
        from .grid import morton3D_encode_pytorch
        c_morton = morton3D_encode_pytorch(c_norm.unsqueeze(0), bits_per_axis=21)
        shift_bits = (21 - bits_resolution) * 3
        c_cell_key = c_morton >> shift_bits
        
        cx, cy, cz = decode_cell_key(c_cell_key)
        cx, cy, cz = cx.item(), cy.item(), cz.item()
        
        # 3. Determine grid search radius (r in grid units)
        cell_size = grid_z.scale / grid_z.grid_resolution
        search_radius_cells = int(np.ceil(r.item() / cell_size)) + 1
        
        # 4. Search all cell keys in bounding box
        intruder_found = False
        
        for dx in range(-search_radius_cells, search_radius_cells + 1):
            nx = cx + dx
            if nx < 0 or nx >= grid_z.grid_resolution: continue
            for dy in range(-search_radius_cells, search_radius_cells + 1):
                ny = cy + dy
                if ny < 0 or ny >= grid_z.grid_resolution: continue
                for dz in range(-search_radius_cells, search_radius_cells + 1):
                    nz = cz + dz
                    if nz < 0 or nz >= grid_z.grid_resolution: continue
                    
                    cell_key = encode_cell_coords(torch.tensor(nx, device=device), 
                                                  torch.tensor(ny, device=device), 
                                                  torch.tensor(nz, device=device)).item()
                                                  
                    cell_pts_idx = grid_z.get_points_in_cell(cell_key)
                    if len(cell_pts_idx) == 0:
                        continue
                        
                    # Filter out coface vertices
                    mask = torch.ones(cell_pts_idx.shape[0], dtype=torch.bool, device=device)
                    for val in cof:
                        mask &= (cell_pts_idx != val)
                    check_pts = cell_pts_idx[mask]
                    
                    if len(check_pts) == 0:
                        continue
                        
                    # Compute power distances
                    diff = Z[check_pts] - c
                    phi = torch.sum(diff ** 2, dim=1) + a[check_pts]
                    
                    if torch.any(phi < r2 - tol):
                        intruder_found = True
                        break
            if intruder_found:
                break
                
        if intruder_found:
            passes[i] = False
            
    return passes
