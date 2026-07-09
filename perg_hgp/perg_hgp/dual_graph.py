import os
import tempfile
import torch
import numpy as np

def compute_facet_ids(cofaces, K, max_ram_facets=2000000):
    """
    Computes unique IDs for each K-facet of the cofaces.
    If the total number of facets exceeds max_ram_facets, runs in out-of-core
    bucketed external sort mode to limit RAM consumption to < 1 GB.
    Otherwise, runs fast in-memory CPU NumPy unique.
    """
    device = cofaces.device
    U = cofaces.shape[0]
    total_facets = U * (K + 1)
    
    if total_facets <= max_ram_facets:
        # --- IN-MEMORY PATH ---
        facets_list = []
        for r in range(K + 1):
            mask = torch.ones(K + 1, dtype=torch.bool, device=device)
            mask[r] = False
            facet = cofaces[:, mask]
            facets_list.append(facet)
            
        facets_all = torch.cat(facets_list, dim=0) # ((K+1)*U, K)
        facets_all_sorted, _ = torch.sort(facets_all, dim=1)
        
        facets_all_sorted_cpu = facets_all_sorted.cpu().numpy()
        unique_facets_cpu, unique_idx_cpu, inverse_indices_cpu = np.unique(
            facets_all_sorted_cpu, axis=0, return_index=True, return_inverse=True
        )
        
        unique_idx = torch.from_numpy(unique_idx_cpu).to(device)
        inverse_indices = torch.from_numpy(inverse_indices_cpu).to(device)
        
        unique_facets = facets_all_sorted[unique_idx]
        facet_ids = inverse_indices.view(K + 1, U).t()
        return facet_ids, unique_facets

    else:
        # --- OUT-OF-CORE PATH ---
        print(f"[PERG-HGP] Running out-of-core facet deduplication for {total_facets} facets...")
        # Create temp files
        temp_dir = tempfile.mkdtemp()
        facets_path = os.path.join(temp_dir, "facets_raw.bin")
        bucketed_path = os.path.join(temp_dir, "facets_bucketed.bin")
        orig_idx_path = os.path.join(temp_dir, "orig_idx.bin")
        unique_path = os.path.join(temp_dir, "facets_unique.bin")
        
        # 1. Generate and sort facets in chunks and write to raw memmap
        facets_raw = np.memmap(facets_path, dtype=np.int32, mode='w+', shape=(total_facets, K))
        
        chunk_size = 100000
        for start_u in range(0, U, chunk_size):
            end_u = min(start_u + chunk_size, U)
            
            cofaces_chunk = cofaces[start_u:end_u]
            facets_list = []
            for r in range(K + 1):
                mask = torch.ones(K + 1, dtype=torch.bool, device=device)
                mask[r] = False
                facet = cofaces_chunk[:, mask]
                facets_list.append(facet)
                
            facets_chunk = torch.cat(facets_list, dim=0) # ((K+1)*C, K)
            facets_chunk_sorted, _ = torch.sort(facets_chunk, dim=1)
            
            # Write to raw memmap
            start_f = start_u * (K + 1)
            end_f = end_u * (K + 1)
            facets_raw[start_f:end_f] = facets_chunk_sorted.cpu().numpy()
            
        facets_raw.flush()
        
        # Find maximum vertex index in cofaces to determine bucket ranges
        N = int(cofaces.max().item()) + 1
        
        # 2. Partition facets into B buckets based on first vertex
        B = 64
        bucket_size_v = (N + B - 1) // B
        
        # Pass 1: Count facets per bucket
        bucket_counts = np.zeros(B, dtype=np.int64)
        for start_f in range(0, total_facets, chunk_size * (K + 1)):
            end_f = min(start_f + chunk_size * (K + 1), total_facets)
            first_v = facets_raw[start_f:end_f, 0]
            b_idx = np.minimum(first_v // bucket_size_v, B - 1)
            counts = np.bincount(b_idx, minlength=B)
            bucket_counts += counts
            
        bucket_offsets = np.cumsum(bucket_counts) - bucket_counts
        
        # Allocate bucketed arrays
        facets_bucketed = np.memmap(bucketed_path, dtype=np.int32, mode='w+', shape=(total_facets, K))
        orig_indices = np.memmap(orig_idx_path, dtype=np.int64, mode='w+', shape=(total_facets,))
        
        write_pointers = bucket_offsets.copy()
        
        # Pass 2: Write facets to bucket sections
        for start_f in range(0, total_facets, chunk_size * (K + 1)):
            end_f = min(start_f + chunk_size * (K + 1), total_facets)
            chunk_facets = facets_raw[start_f:end_f]
            first_v = chunk_facets[:, 0]
            b_idx = np.minimum(first_v // bucket_size_v, B - 1)
            
            for i in range(end_f - start_f):
                b = b_idx[i]
                ptr = write_pointers[b]
                facets_bucketed[ptr] = chunk_facets[i]
                orig_indices[ptr] = start_f + i
                write_pointers[b] += 1
                
        facets_bucketed.flush()
        orig_indices.flush()
        
        # 3. Process each bucket in RAM
        unique_ids_raw = np.zeros(total_facets, dtype=np.int64)
        unique_facets_raw = np.memmap(unique_path, dtype=np.int32, mode='w+', shape=(total_facets, K))
        
        start_unique_offset = 0
        
        for b in range(B):
            count = bucket_counts[b]
            if count == 0:
                continue
                
            start_b = bucket_offsets[b]
            end_b = start_b + count
            
            # Load bucket into RAM
            loaded_facets = facets_bucketed[start_b:end_b]
            loaded_orig = orig_indices[start_b:end_b]
            
            # Deduplicate
            unique_b, unique_idx_b, inv_idx_b = np.unique(
                loaded_facets, axis=0, return_index=True, return_inverse=True
            )
            
            U_b = unique_b.shape[0]
            
            # Write unique facets to unique memmap
            unique_facets_raw[start_unique_offset:start_unique_offset + U_b] = unique_b
            
            # Map original indices to global unique IDs
            unique_ids_raw[loaded_orig] = start_unique_offset + inv_idx_b
            
            start_unique_offset += U_b
            
        unique_facets_raw.flush()
        
        # 4. Load the final unique facets and facet IDs back
        unique_facets_cpu = unique_facets_raw[:start_unique_offset]
        unique_facets = torch.from_numpy(unique_facets_cpu).to(device)
        
        facet_ids_cpu = unique_ids_raw.reshape(K + 1, U).T
        facet_ids = torch.from_numpy(facet_ids_cpu).to(device)
        
        # Clean up temp files
        del facets_raw, facets_bucketed, orig_indices, unique_facets_raw
        
        os.remove(facets_path)
        os.remove(bucketed_path)
        os.remove(orig_idx_path)
        os.remove(unique_path)
        os.rmdir(temp_dir)
        
        return facet_ids, unique_facets


def build_dual_edges(cofaces, facet_ids, weights):
    """
    Builds the dual graph edges using star expansion.
    """
    U = cofaces.shape[0]
    K = cofaces.shape[1] - 1
    device = cofaces.device
    
    edge_u = torch.zeros((U, K), dtype=torch.int64, device=device)
    edge_v = torch.zeros((U, K), dtype=torch.int64, device=device)
    edge_w = torch.zeros((U, K), dtype=torch.float32, device=device)
    edge_coface = torch.zeros((U, K), dtype=torch.int64, device=device)
    
    for j in range(1, K + 1):
        edge_u[:, j - 1] = facet_ids[:, 0]
        edge_v[:, j - 1] = facet_ids[:, j]
        edge_w[:, j - 1] = weights
        edge_coface[:, j - 1] = torch.arange(U, device=device)
        
    return edge_u.flatten(), edge_v.flatten(), edge_w.flatten(), edge_coface.flatten()
