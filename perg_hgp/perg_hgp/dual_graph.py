import os
import tempfile
import torch
import numpy as np

def compute_facet_ids(cofaces, K, max_ram_facets=2000000, chunk_size=100000):
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

        # 1. Generate and sort facets in chunks and write to raw memmap in global layout
        facets_raw = np.memmap(facets_path, dtype=np.int32, mode='w+', shape=(total_facets, K))

        for start_u in range(0, U, chunk_size):
            end_u = min(start_u + chunk_size, U)

            cofaces_chunk = cofaces[start_u:end_u]
            for r in range(K + 1):
                mask = torch.ones(K + 1, dtype=torch.bool, device=device)
                mask[r] = False
                facet = cofaces_chunk[:, mask]
                facet_sorted, _ = torch.sort(facet, dim=1)

                # Write to the correct global layout position (r * U + u)
                facets_raw[r * U + start_u : r * U + end_u] = facet_sorted.cpu().numpy()

        facets_raw.flush()

        # Find maximum vertex index in cofaces to determine bucket ranges
        N = int(cofaces.max().item()) + 1

        # 2. Partition facets into B buckets based on first vertex
        # Scale B dynamically to keep average bucket size around 500k elements
        B = max(64, int(np.ceil(total_facets / 500000)))
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

        # Pass 2: Write facets to bucket sections (vectorized)
        for start_f in range(0, total_facets, chunk_size * (K + 1)):
            end_f = min(start_f + chunk_size * (K + 1), total_facets)
            chunk_len = end_f - start_f
            chunk_facets = facets_raw[start_f:end_f]
            first_v = chunk_facets[:, 0]
            b_idx = np.minimum(first_v // bucket_size_v, B - 1)

            # Sort the chunk elements by bucket index to write contiguously
            sort_order = np.argsort(b_idx)
            sorted_facets = chunk_facets[sort_order]
            sorted_b_idx = b_idx[sort_order]

            # Find transitions in sorted bucket indices
            diff = sorted_b_idx[1:] != sorted_b_idx[:-1]
            transitions = np.where(diff)[0] + 1
            starts = np.concatenate([[0], transitions])
            ends = np.concatenate([transitions, [chunk_len]])
            active_buckets = sorted_b_idx[starts]

            for idx_b in range(len(active_buckets)):
                b = active_buckets[idx_b]
                st = starts[idx_b]
                en = ends[idx_b]
                count = en - st

                ptr = write_pointers[b]
                facets_bucketed[ptr : ptr + count] = sorted_facets[st:en]
                orig_indices[ptr : ptr + count] = start_f + sort_order[st:en]
                write_pointers[b] += count

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

            # Load bucket into RAM and deduplicate, with skew protection
            loaded_orig = orig_indices[start_b:end_b]

            if count > max_ram_facets:
                import warnings
                warnings.warn(
                    f"Bucket {b} has size {count} which exceeds max_ram_facets {max_ram_facets}. "
                    "Applying second-level partitioning to prevent memory spike.",
                    UserWarning
                )
                 # Check if first vertex varies or is unique in this bucket
                first_vertices = facets_bucketed[start_b:end_b, 0]
                min_v, max_v = int(first_vertices.min()), int(first_vertices.max())

                B2 = 16
                if min_v < max_v:
                    # Partition by first vertex to preserve lexicographical order
                    v_range = max_v - min_v + 1
                    sub_bucket_size = (v_range + B2 - 1) // B2
                    sub_assigned = (first_vertices - min_v) // sub_bucket_size
                else:
                    # All have the same first vertex, partition by second vertex to preserve order
                    second_vertices = facets_bucketed[start_b:end_b, 1]
                    min_v2, max_v2 = int(second_vertices.min()), int(second_vertices.max())
                    v_range = max(1, max_v2 - min_v2 + 1)
                    sub_bucket_size = (v_range + B2 - 1) // B2
                    sub_assigned = (second_vertices - min_v2) // sub_bucket_size

                sub_assigned = np.clip(sub_assigned, 0, B2 - 1)

                sub_unique_list = []
                inv_idx_b = np.zeros(count, dtype=np.int64)
                sub_offset = 0
                for sb in range(B2):
                    sb_mask = sub_assigned == sb
                    sb_indices = np.where(sb_mask)[0]
                    if sb_indices.shape[0] == 0:
                        continue
                    # Load only this sub-bucket into RAM
                    sb_facets = facets_bucketed[start_b + sb_indices]
                    sb_unique, sb_idx, sb_inv = np.unique(
                        sb_facets, axis=0, return_index=True, return_inverse=True
                    )
                    sub_unique_list.append(sb_unique)
                    inv_idx_b[sb_indices] = sub_offset + sb_inv
                    sub_offset += sb_unique.shape[0]

                unique_b = np.concatenate(sub_unique_list, axis=0)
            else:
                loaded_facets = facets_bucketed[start_b:end_b]
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


class DualEdgesIterable:
    def __init__(self, cofaces, facet_ids, weights):
        self.cofaces = cofaces
        self.facet_ids = facet_ids
        self.weights = weights

    def chunks(self, chunk_size=1000000):
        return get_edge_chunks(self.cofaces, self.facet_ids, self.weights, chunk_size)

    def __iter__(self):
        import warnings
        U = self.cofaces.shape[0]
        K = self.cofaces.shape[1] - 1
        if U * K > 5000000:
            warnings.warn(
                f"Unpacking {U * K} edges in memory using legacy mode. "
                "For better memory scaling, use build_dual_edges(...).chunks() instead.",
                UserWarning
            )
        edge_u = torch.zeros((U, K), dtype=torch.int64, device='cpu')
        edge_v = torch.zeros((U, K), dtype=torch.int64, device='cpu')
        edge_w = torch.zeros((U, K), dtype=torch.float32, device='cpu')
        edge_coface = torch.zeros((U, K), dtype=torch.int64, device='cpu')
        facet_ids_cpu = self.facet_ids.cpu()
        weights_cpu = self.weights.cpu()
        for j in range(1, K + 1):
            edge_u[:, j - 1] = facet_ids_cpu[:, 0]
            edge_v[:, j - 1] = facet_ids_cpu[:, j]
            edge_w[:, j - 1] = weights_cpu
            edge_coface[:, j - 1] = torch.arange(U, device='cpu')
        yield edge_u.flatten()
        yield edge_v.flatten()
        yield edge_w.flatten()
        yield edge_coface.flatten()


def build_dual_edges(cofaces, facet_ids, weights):
    """
    Builds the dual graph edges using star expansion.
    Returns a DualEdgesIterable that supports legacy unpacking:
      edge_u, edge_v, edge_w, edge_coface = build_dual_edges(...)
    as well as memory-efficient chunked streaming:
      build_dual_edges(...).chunks(chunk_size)
    """
    return DualEdgesIterable(cofaces, facet_ids, weights)


def get_edge_chunks(cofaces, facet_ids, weights, chunk_size=1000000):
    """
    Generator that yields dual graph edges in chunks to prevent memory spikes.
    Yields:
        edge_u, edge_v, edge_w, edge_coface (flat CPU tensors)
    """
    U = cofaces.shape[0]
    K = cofaces.shape[1] - 1

    for start in range(0, U, chunk_size):
        end = min(start + chunk_size, U)
        c_len = end - start

        edge_u = torch.zeros((c_len, K), dtype=torch.int64, device='cpu')
        edge_v = torch.zeros((c_len, K), dtype=torch.int64, device='cpu')
        edge_w = torch.zeros((c_len, K), dtype=torch.float32, device='cpu')
        edge_coface = torch.zeros((c_len, K), dtype=torch.int64, device='cpu')

        facet_ids_cpu = facet_ids[start:end].cpu()
        weights_cpu = weights[start:end].cpu()

        for j in range(1, K + 1):
            edge_u[:, j - 1] = facet_ids_cpu[:, 0]
            edge_v[:, j - 1] = facet_ids_cpu[:, j]
            edge_w[:, j - 1] = weights_cpu
            edge_coface[:, j - 1] = torch.arange(start, end, device='cpu')

        yield edge_u.flatten(), edge_v.flatten(), edge_w.flatten(), edge_coface.flatten()
