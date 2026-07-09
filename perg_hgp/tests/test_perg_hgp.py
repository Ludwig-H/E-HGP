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
    
    def test_morton_encoding_and_grid(self):
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
        
if __name__ == '__main__':
    unittest.main()
