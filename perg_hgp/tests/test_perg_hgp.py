import unittest
import torch
import numpy as np
from sklearn.datasets import make_blobs

from perg_hgp import (
    PERGHGPClusterer,
    SpatialGrid3D,
    solve_local_gibbs_entropy,
    compute_regularized_sites,
    compute_rank_soft_dp,
    compute_rank_soft_dp_batched,
    solve_weighted_miniball_active_set_3d,
    compute_facet_ids,
    build_dual_edges
)
from perg_hgp.cofaces import solve_weighted_miniball_brute_force_3d, extract_top_cofaces
from perg_hgp.gabriel import global_gabriel_grid_test

class TestPERGHGP(unittest.TestCase):

    def test_flat_grid_indexing_and_search(self):
        device = 'cpu'
        # Generate 100 points
        X = np.random.uniform(0, 1, (100, 3)).astype(np.float32)
        grid = SpatialGrid3D(X, grid_resolution=16, device=device)

        # Test KNN query
        query_pts = torch.tensor(X[:5], device=device)
        nbr_indices, nbr_dists_sq = grid.query_knn_grid(query_pts, m_local=10)

        self.assertEqual(nbr_indices.shape, (5, 10))
        self.assertEqual(nbr_dists_sq.shape, (5, 10))

        # Closest neighbor should be the point itself (dist close to 0)
        self.assertLess(nbr_dists_sq[0, 0].item(), 1e-5)

    def test_knn_exactness_vs_brute_force(self):
        device = 'cpu'
        np.random.seed(42)
        torch.manual_seed(42)

        # 1. Generate random point cloud
        X = np.random.uniform(10.0, 20.0, (200, 3)).astype(np.float32)
        grid = SpatialGrid3D(X, grid_resolution=8, device=device)

        # 2. Query points: some internal, some external (outside the bbox)
        query_pts = np.random.uniform(5.0, 25.0, (20, 3)).astype(np.float32)
        query_pts_tensor = torch.from_numpy(query_pts).to(device)

        # 3. Query grid KNN
        nbr_indices, nbr_dists_sq = grid.query_knn_grid(query_pts_tensor, m_local=10)

        # Verify fallback and certified rate are tracked
        self.assertTrue(hasattr(grid, 'fallback_count_'))
        self.assertTrue(hasattr(grid, 'fallback_rate_'))
        self.assertAlmostEqual(grid.fallback_rate_ + grid.certified_rate_, 1.0, places=5)

        # 4. Compare with brute-force scan
        X_tensor = torch.from_numpy(X).to(device)
        for i in range(20):
            q_pt = query_pts_tensor[i]
            diff = X_tensor - q_pt
            dist2 = torch.sum(diff ** 2, dim=1)

            top_val, top_idx = torch.topk(dist2, 10, largest=False)

            for k in range(10):
                self.assertAlmostEqual(nbr_dists_sq[i, k].item(), top_val[k].item(), places=4)

    def test_local_gibbs(self):
        device = 'cpu'
        nbr_dists_sq = torch.rand((10, 32), device=device, dtype=torch.float32) + 0.1
        entropy_target = 10.0

        Q, eta = solve_local_gibbs_entropy(nbr_dists_sq, entropy_target)

        self.assertEqual(Q.shape, (10, 32))
        self.assertEqual(eta.shape, (10,))

        # Q rows must sum to 1
        row_sums = torch.sum(Q, dim=1)
        for val in row_sums:
            self.assertAlmostEqual(val.item(), 1.0, places=4)

    def test_rank_dp(self):
        device = 'cpu'
        # 32 energies
        E = torch.sort(torch.rand(32, device=device, dtype=torch.float32))[0]
        Kmax = 10
        eps = 0.5

        Q, b = compute_rank_soft_dp(E, Kmax, eps)

        self.assertEqual(Q.shape, (Kmax + 1,))
        self.assertEqual(b.shape, (Kmax + 1, 32))

        # Shell weights must sum to 1
        for k in range(Kmax + 1):
            self.assertAlmostEqual(torch.sum(b[k]).item(), 1.0, places=4)

    def test_rank_dp_batched(self):
        device = 'cpu'
        # 5 batches of 32 energies
        E = torch.rand((5, 32), device=device, dtype=torch.float32)
        Kmax = 10
        eps = 0.5

        Q_batch, b_batch = compute_rank_soft_dp_batched(E, Kmax, eps)

        self.assertEqual(Q_batch.shape, (5, Kmax + 1))
        self.assertEqual(b_batch.shape, (5, Kmax + 1, 32))

        # Verify that each batch element matches the scalar computation exactly
        for i in range(5):
            Q_scalar, b_scalar = compute_rank_soft_dp(E[i], Kmax, eps)
            self.assertLess(torch.norm(Q_batch[i] - Q_scalar).item(), 1e-4)
            self.assertLess(torch.norm(b_batch[i] - b_scalar).item(), 1e-4)

    def test_miniball_vs_brute_force(self):
        device = 'cpu'
        np.random.seed(42)
        torch.manual_seed(42)

        # Generate 20 random trials
        for trial in range(20):
            # Size of coface between 4 and 8
            n_pts = np.random.randint(4, 9)
            Z_cof = torch.randn((n_pts, 3), device=device)
            a_cof = torch.rand(n_pts, device=device) * 0.1

            c_as, r2_as = solve_weighted_miniball_active_set_3d(Z_cof, a_cof, tol=1e-6)
            c_bf, r2_bf = solve_weighted_miniball_brute_force_3d(Z_cof, a_cof, tol=1e-6)

            # Compare center and radius squared
            self.assertLess(torch.norm(c_as - c_bf).item(), 1e-4)
            self.assertAlmostEqual(r2_as.item(), r2_bf.item(), places=4)

    def test_global_gabriel_vs_scan(self):
        device = 'cpu'
        np.random.seed(42)
        torch.manual_seed(42)

        # Generate 50 points
        X = torch.rand((50, 3), device=device)
        a = torch.rand(50, device=device) * 0.05
        grid_z = SpatialGrid3D(X, grid_resolution=8, device=device)

        # Generate a candidate coface (first 4 points)
        cof = torch.tensor([0, 1, 2, 3], device=device)
        cofaces = cof.unsqueeze(0)

        # Compute exact miniball of this coface
        c, r2 = solve_weighted_miniball_active_set_3d(X[cof], a[cof], tol=1e-6)
        centers = c.unsqueeze(0)
        radii_sq = r2.unsqueeze(0)

        # Grid-based global Gabriel test
        grid_passes = global_gabriel_grid_test(cofaces, X, a, centers, radii_sq, grid_z, tol=1e-6)

        # Brute-force scan of all points in the dataset
        brute_pass = True
        for j in range(50):
            if j not in cof:
                dist_sq = torch.sum((X[j] - c) ** 2) + a[j]
                if dist_sq < r2 - 1e-6:
                    brute_pass = False
                    break

        self.assertEqual(grid_passes[0].item(), brute_pass)

    def test_estimator_basic(self):
        # Test the end-to-end fit_predict on simple blobs
        X, y = make_blobs(n_samples=200, centers=2, n_features=3, random_state=42)
        X = X.astype(np.float32)

        clusterer = PERGHGPClusterer(
            K=3,
            m_local=20,
            m_active=20,
            grid_resolution=8,
            W1_budget=100,
            budget_per_rank=100,
            min_cluster_size=3,
            device='cpu'
        )

        labels = clusterer.fit_predict(X)

        self.assertEqual(labels.shape[0], 200)
        unique_labels = np.unique(labels[labels >= 0])
        self.assertGreaterEqual(len(unique_labels), 1)
        self.assertIn("candidate_cofaces", clusterer.exactness_report_)

    def test_estimator_small_n(self):
        # Regression test for small N (N < 128) where m_local / m_active might exceed N.
        # Must not produce any NaN values in regularized sites or labels.
        X, y = make_blobs(n_samples=50, centers=2, n_features=3, random_state=42)
        X = X.astype(np.float32)

        clusterer = PERGHGPClusterer(
            K=2,
            m_local=128,  # Exceeds N=50
            m_active=128, # Exceeds N=50
            min_cluster_size=5,
            device='cpu'
        )

        labels = clusterer.fit_predict(X)

        # Check for NaN in Z_
        self.assertFalse(np.isnan(clusterer.Z_.cpu().numpy()).any(), "Z_ contains NaN values!")
        # Check for NaN in a_
        self.assertFalse(np.isnan(clusterer.a_.cpu().numpy()).any(), "a_ contains NaN values!")
        # Check for NaN in labels
        self.assertFalse(np.isnan(labels).any(), "Labels contain NaN values!")

    def test_dual_graph(self):
        device = 'cpu'
        # 3 cofaces of size K+1 = 4 (K=3)
        # We define them with overlapping elements to check facet sharing
        cofaces = torch.tensor([
            [0, 1, 2, 3],
            [1, 2, 3, 4],
            [2, 3, 4, 5]
        ], dtype=torch.int64, device=device)

        facet_ids, unique_facets = compute_facet_ids(cofaces, K=3)

        # Verify shape
        # Each cofaces has 4 facets of size 3
        # Unique facets should contain all unique combinations of size 3 from each coface
        # Facets for [0, 1, 2, 3]: (1,2,3), (0,2,3), (0,1,3), (0,1,2)
        # Facets for [1, 2, 3, 4]: (2,3,4), (1,3,4), (1,2,4), (1,2,3)
        # Facets for [2, 3, 4, 5]: (3,4,5), (2,4,5), (2,3,5), (2,3,4)
        # Total unique facets:
        # (0,1,2), (0,1,3), (0,2,3), (1,2,3), (1,2,4), (1,3,4), (2,3,4), (2,3,5), (2,4,5), (3,4,5)
        # That's 10 unique facets
        self.assertEqual(unique_facets.shape[1], 3)
        self.assertEqual(unique_facets.shape[0], 10)

        # Build dual edges
        weights = torch.tensor([0.5, 0.4, 0.3], dtype=torch.float32, device=device)
        edge_u, edge_v, edge_w, edge_coface = build_dual_edges(cofaces, facet_ids, weights)

        # Each coface has K=3 edges (star expansion, pivot is facet 0 to facets 1, 2, 3)
        # Total edges = 3 * 3 = 9
        self.assertEqual(edge_u.shape[0], 9)
        self.assertEqual(edge_v.shape[0], 9)
        self.assertEqual(edge_w.shape[0], 9)
        self.assertEqual(edge_coface.shape[0], 9)

    def test_soft_only_mode(self):
        # Test soft_only mode on blobs
        X, y = make_blobs(n_samples=100, centers=2, n_features=3, random_state=42)
        X = X.astype(np.float32)

        clusterer = PERGHGPClusterer(
            exactness_mode='soft_only',
            grid_resolution=8,
            m_local=20,
            min_cluster_size=3,
            device='cpu'
        )

        labels = clusterer.fit_predict(X)
        self.assertEqual(labels.shape[0], 100)
        self.assertEqual(clusterer.exactness_report_['exactness_mode'], 'soft_only')
        self.assertEqual(clusterer.exactness_report_['candidate_cofaces'], 0)
        self.assertEqual(clusterer.exactness_report_['num_facets'], 100)

    def test_checkpoint_save_and_resume(self):
        import tempfile
        import os

        X, _ = make_blobs(n_samples=50, centers=2, n_features=3, random_state=42)
        X = X.astype(np.float32)

        with tempfile.TemporaryDirectory() as tmpdir:
            # 1. Run first time to save checkpoints
            clusterer1 = PERGHGPClusterer(
                K=2,
                grid_resolution=4,
                m_local=10,
                m_active=10,
                W1_budget=50,
                budget_per_rank=50,
                min_cluster_size=2,
                device='cpu',
                checkpoint_dir=tmpdir
            )
            labels1 = clusterer1.fit_predict(X)

            # Assert checkpoint files exist
            expected_files = ["sites.pt", "witnesses_rank_1.pt", "certified_cofaces.pt", "dual_mst.pt", "Z_tree.pt"]
            for f in expected_files:
                self.assertTrue(os.path.exists(os.path.join(tmpdir, f)), f"Checkpoint file {f} was not saved")

            # 2. Run second time to load checkpoints
            clusterer2 = PERGHGPClusterer(
                K=2,
                grid_resolution=4,
                m_local=10,
                m_active=10,
                W1_budget=50,
                budget_per_rank=50,
                min_cluster_size=2,
                device='cpu',
                checkpoint_dir=tmpdir
            )
            labels2 = clusterer2.fit_predict(X)

            # Assert both runs produced identical results
            np.testing.assert_array_equal(labels1, labels2)
            self.assertEqual(clusterer2.exactness_report_['candidate_cofaces'], clusterer1.exactness_report_['candidate_cofaces'])

    def test_no_cofaces_certified(self):
        # Set gabriel_tol to a huge negative value to reject all cofaces
        X, _ = make_blobs(n_samples=50, centers=2, n_features=3, random_state=42)
        X = X.astype(np.float32)

        clusterer = PERGHGPClusterer(
            K=2,
            grid_resolution=4,
            m_local=10,
            m_active=10,
            gabriel_tol=-1e9, # force rejection
            device='cpu'
        )
        labels = clusterer.fit_predict(X)
        self.assertEqual(labels.shape[0], 50)
        self.assertTrue(np.all(labels == -1))
        self.assertEqual(clusterer.exactness_report_['certified_cofaces'], 0)
        self.assertEqual(clusterer.exactness_report_['num_facets'], 0)

    def test_out_of_core_facet_deduplication(self):
        from perg_hgp.dual_graph import compute_facet_ids
        device = 'cpu'

        # Generate dummy cofaces
        np.random.seed(42)
        cofaces_np = np.random.randint(0, 100, size=(100, 4))
        # ensure unique rows
        cofaces_np = np.unique(cofaces_np, axis=0)
        cofaces = torch.from_numpy(cofaces_np).to(device)

        # 1. Run in-memory unique
        facet_ids_in_mem, unique_facets_in_mem = compute_facet_ids(cofaces, K=3, max_ram_facets=100000)

        # 2. Run out-of-core unique (set threshold very low and chunk_size small to force multiple chunks)
        facet_ids_ooc, unique_facets_ooc = compute_facet_ids(cofaces, K=3, max_ram_facets=10, chunk_size=10)

        # Verify identity
        np.testing.assert_array_equal(facet_ids_in_mem.cpu().numpy(), facet_ids_ooc.cpu().numpy())
        np.testing.assert_array_equal(unique_facets_in_mem.cpu().numpy(), unique_facets_ooc.cpu().numpy())

    def test_boruvka_vs_kruskal(self):
        from perg_hgp.hierarchy import dual_graph_mst
        device = 'cpu'
        np.random.seed(42)
        torch.manual_seed(42)

        # Generate random connected-like graph edges
        num_facets = 50
        num_edges = 150
        edge_u = torch.randint(0, num_facets, (num_edges,), device=device)
        edge_v = torch.randint(0, num_facets, (num_edges,), device=device)
        # Avoid self-loops for basic edge pool
        mask = edge_u != edge_v
        edge_u = edge_u[mask]
        edge_v = edge_v[mask]

        # Ensure graph is connected by adding a cycle/line 0-1-2...-N-0
        line_u = torch.arange(num_facets, device=device)
        line_v = (line_u + 1) % num_facets
        edge_u = torch.cat([edge_u, line_u])
        edge_v = torch.cat([edge_v, line_v])

        actual_num_edges = edge_u.shape[0]
        # Assign unique weights to make MST unique (multiplied by 10.0 to stress-test large weights)
        edge_w = torch.rand(actual_num_edges, dtype=torch.float32, device=device) * 10.0
        edge_coface = torch.arange(actual_num_edges, device=device)

        # 1. Run Boruvka
        u_b, v_b, w_b, cof_b = dual_graph_mst(edge_u, edge_v, edge_w, edge_coface, num_facets)

        # 2. Run reference Kruskal
        parent = np.arange(num_facets, dtype=np.int32)
        def find(i):
            path = []
            while parent[i] != i:
                path.append(i)
                parent[i] = parent[parent[i]]
                i = parent[i]
            for node in path:
                parent[node] = i
            return i

        u_np = edge_u.cpu().numpy()
        v_np = edge_v.cpu().numpy()
        w_np = edge_w.cpu().numpy()
        cof_np = edge_coface.cpu().numpy()

        order = np.argsort(w_np)
        mst_u_ref = []
        mst_v_ref = []
        mst_w_ref = []
        mst_cof_ref = []

        for idx in order:
            ru = find(u_np[idx])
            rv = find(v_np[idx])
            if ru != rv:
                parent[rv] = ru
                mst_u_ref.append(u_np[idx])
                mst_v_ref.append(v_np[idx])
                mst_w_ref.append(w_np[idx])
                mst_cof_ref.append(cof_np[idx])

        # Compare MST weights sum (must be identical)
        self.assertAlmostEqual(w_b.sum(), np.array(mst_w_ref).sum(), places=4)

        # Compare sorted edge indices (since weights are unique, the MST is unique)
        self.assertEqual(len(cof_b), len(mst_cof_ref))
        np.testing.assert_array_equal(np.sort(cof_b), np.sort(mst_cof_ref))

    def test_batched_miniball_vs_active_set(self):
        from perg_hgp.cofaces import solve_weighted_miniball_batched, solve_weighted_miniball_active_set_3d
        device = 'cpu'
        np.random.seed(42)
        torch.manual_seed(42)

        U = 20
        Nc = 4 # size <= 4
        Z_cofaces = torch.rand((U, Nc, 3), dtype=torch.float32, device=device)
        a_cofaces = torch.rand((U, Nc), dtype=torch.float32, device=device)

        # 1. Run batched
        centers_b, r2_b = solve_weighted_miniball_batched(Z_cofaces, a_cofaces, tol=1e-6)

        # 2. Run sequential
        for i in range(U):
            c_seq, r2_seq = solve_weighted_miniball_active_set_3d(Z_cofaces[i], a_cofaces[i], tol=1e-6)
            np.testing.assert_allclose(centers_b[i].cpu().numpy(), c_seq.cpu().numpy(), rtol=1e-4, atol=1e-4)
            self.assertAlmostEqual(r2_b[i].item(), r2_seq.item(), places=4)

    def test_bucket_skew_protection(self):
        # We test that compute_facet_ids triggers second-level partitioning
        # without errors when a bucket has more facets than max_ram_facets.
        from perg_hgp.dual_graph import compute_facet_ids
        import warnings

        # 10 cofaces of size K+1 = 3 (K=2)
        cofaces = torch.tensor([
            [0, 1, 2],
            [0, 1, 3],
            [0, 1, 4],
            [0, 2, 3],
            [0, 2, 4],
            [0, 3, 4],
            [0, 1, 5],
            [0, 2, 5],
            [0, 3, 5],
            [0, 4, 5]
        ], dtype=torch.int64)

        # Set max_ram_facets = 3 so that the bucket for first vertex 0
        # exceeds max_ram_facets.
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            facet_ids, unique_facets = compute_facet_ids(cofaces, K=2, max_ram_facets=3)

            # Check that warning was triggered
            warning_triggered = any("second-level partitioning" in str(warn.message) for warn in w)
            self.assertTrue(warning_triggered, "Second-level partitioning warning was not triggered!")

        # Verify shapes and uniqueness
        self.assertEqual(facet_ids.shape, (10, 3))
        self.assertEqual(unique_facets.shape[0], 15)

    def test_oracle_six_points(self):
        # Exact CPU reference oracle test for 6 points (K=2)
        from perg_hgp.reference.oracle import exact_regularized_sites, exact_cech_gabriel_complex, exact_dual_mst, exact_condense_tree, exact_extract_labels

        # 6 points in 3D: 2 clusters of size 3 (triangles)
        X = np.array([
            [0.0, 0.0, 0.0],
            [1.0, 0.0, 0.0],
            [0.5, 0.8, 0.0],
            [10.0, 0.0, 0.0],
            [11.0, 0.0, 0.0],
            [10.5, 0.8, 0.0]
        ], dtype=np.float32)

        # 1. Exact sites and weights
        Z, a = exact_regularized_sites(X, entropy_target=3)
        self.assertEqual(Z.shape, (6, 3))
        self.assertEqual(a.shape, (6,))

        # 2. Exact Čech/Gabriel complex (K=2)
        cofaces, weights = exact_cech_gabriel_complex(Z, a, K=2)
        # Should have at least the two triangles certified
        self.assertGreaterEqual(cofaces.shape[0], 2)

        # 3. Exact dual MST
        mst_u, mst_v, mst_w, mst_cof, num_facets, unique_facets = exact_dual_mst(cofaces, weights, K=2)
        self.assertGreaterEqual(num_facets, 2)

        # 4. Exact condense tree
        # Compute facet_birth_weights first
        from itertools import combinations
        facet_birth_weights = np.full(num_facets, np.inf, dtype=np.float32)
        facet_to_idx = {tuple(f): idx for idx, f in enumerate(unique_facets)}
        for i, cof in enumerate(cofaces):
            w_val = weights[i]
            for f in combinations(cof, 2):
                f_idx = facet_to_idx[tuple(sorted(f))]
                if w_val < facet_birth_weights[f_idx]:
                    facet_birth_weights[f_idx] = w_val
        facet_birth_weights[facet_birth_weights == np.inf] = 0.0

        S_faces = 1.0 / ((facet_birth_weights + 1e-12) ** 2)
        flat_facets = unique_facets.flatten()
        S_faces_expanded = np.repeat(S_faces, 2)
        T_points = np.bincount(flat_facets, weights=S_faces_expanded, minlength=6).astype(np.float32)
        inv_T_points = np.divide(1.0, T_points, out=np.zeros_like(T_points), where=(T_points > 0.0))
        sum_inv_Tp_face = np.sum(inv_T_points[unique_facets], axis=1)
        W_nodes = S_faces * sum_inv_Tp_face

        Z_tree = exact_condense_tree(W_nodes, mst_u, mst_v, mst_w, min_cluster_size=2)

        # 5. Extract labels
        labels = exact_extract_labels(Z_tree, unique_facets, 6, S_faces, K=2)
        self.assertEqual(labels.shape[0], 6)

if __name__ == '__main__':
    unittest.main()
