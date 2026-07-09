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
    Certified global Gabriel test using cell-bounding box pruning on active cells.
    Guaranteeing that no site in the entire space is an intruder.
    """
    device = Z.device
    U = cofaces.shape[0]
    passes = torch.ones(U, dtype=torch.bool, device=device)
    
    # 1. Precompute cell_min_a for all active cells
    cell_min_a = {}
    for cell_key in grid_z.cell_key_to_idx.keys():
        p_indices = grid_z.get_points_in_cell(cell_key)
        cell_min_a[cell_key] = torch.min(a[p_indices]).item()
        
    cell_size = (grid_z.scale / grid_z.grid_resolution).item()
    bbox_min_cpu = grid_z.bbox_min.cpu().numpy()
    
    # Decode coordinates of all active cell keys
    active_keys = list(grid_z.cell_key_to_idx.keys())
    bits_resolution = int(np.log2(grid_z.grid_resolution))
    
    def decode_cell_key_scalar(key, bits):
        x = 0
        y = 0
        z = 0
        for b in range(bits):
            x |= ((key >> (3 * b)) & 1) << b
            y |= ((key >> (3 * b + 1)) & 1) << b
            z |= ((key >> (3 * b + 2)) & 1) << b
        return x, y, z
        
    # Predecode all active cell bboxes
    active_cells_info = []
    for cell_key in active_keys:
        nx, ny, nz = decode_cell_key_scalar(cell_key, bits_resolution)
        xmin = bbox_min_cpu[0] + nx * cell_size
        xmax = xmin + cell_size
        ymin = bbox_min_cpu[1] + ny * cell_size
        ymax = ymin + cell_size
        zmin = bbox_min_cpu[2] + nz * cell_size
        zmax = zmin + cell_size
        
        active_cells_info.append({
            'key': cell_key,
            'xmin': xmin, 'xmax': xmax,
            'ymin': ymin, 'ymax': ymax,
            'zmin': zmin, 'zmax': zmax,
            'min_a': cell_min_a[cell_key]
        })
        
    for i in range(U):
        cof = cofaces[i]
        c = centers[i]
        r2 = radii_sq[i].item()
        c_cpu = c.cpu().numpy()
        
        intruder_found = False
        
        for cell in active_cells_info:
            # Distance from c to cell bounding box
            dx = max(0.0, cell['xmin'] - c_cpu[0], c_cpu[0] - cell['xmax'])
            dy = max(0.0, cell['ymin'] - c_cpu[1], c_cpu[1] - cell['ymax'])
            dz = max(0.0, cell['zmin'] - c_cpu[2], c_cpu[2] - cell['zmax'])
            dist_sq = dx**2 + dy**2 + dz**2
            
            # Pruning check
            if dist_sq + cell['min_a'] >= r2 - tol:
                continue
                
            # Otherwise, we must check points in the cell
            cell_pts_idx = grid_z.get_points_in_cell(cell['key'])
            if len(cell_pts_idx) == 0:
                continue
                
            # Filter out coface vertices
            mask = torch.ones(cell_pts_idx.shape[0], dtype=torch.bool, device=device)
            for val in cof:
                mask &= (cell_pts_idx != val)
            check_pts = cell_pts_idx[mask]
            
            if len(check_pts) == 0:
                continue
                
            diff = Z[check_pts] - c
            phi = torch.sum(diff ** 2, dim=1) + a[check_pts]
            
            if torch.any(phi < r2 - tol):
                intruder_found = True
                break
                
        if intruder_found:
            passes[i] = False
            
    return passes
