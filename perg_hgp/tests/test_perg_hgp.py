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
    solve_weighted_miniball_active_set_3d
)

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
            
    def test_miniball_active_set_3d(self):
        device = 'cpu'
        # 5 points in 3D
        Z_cof = torch.randn((5, 3), device=device)
        a_cof = torch.rand(5, device=device) * 0.1
        
        center, r2 = solve_weighted_miniball_active_set_3d(Z_cof, a_cof)
        
        self.assertEqual(center.shape, (3,))
        self.assertTrue(r2.item() >= 0.0)
        
    def test_estimator_basic(self):
        # Test the end-to-end fit_predict on simple blobs
        X, y = make_blobs(n_samples=200, centers=2, n_features=3, random_state=42)
        X = X.astype(np.float32)
        
        clusterer = PERGHGPClusterer(
            K=3,
            m_local=20,
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
        
if __name__ == '__main__':
    unittest.main()
