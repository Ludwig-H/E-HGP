import unittest
import numpy as np
from sklearn.datasets import make_blobs
from e_hgp import EHGPClusterer

class TestEHGP(unittest.TestCase):
    def test_basic_clustering(self):
        # Generate two simple blobs
        X, y = make_blobs(n_samples=100, centers=2, n_features=2, random_state=42)
        
        # Instantiate E-HGP clusterer
        clusterer = EHGPClusterer(K=2, kappa=1.5, m_reg=20, min_cluster_size=5)
        
        # Fit and predict labels
        labels = clusterer.fit_predict(X)
        
        # Check output labels shape
        self.assertEqual(labels.shape[0], 100)
        
        # Check that we get some non-noise labels
        unique_labels = np.unique(labels[labels >= 0])
        self.assertGreaterEqual(len(unique_labels), 1)
        
        # Check exactness report format
        self.assertIn("hgp_hierarchy_complete", clusterer.exactness_report_)
        self.assertIn("gabriel_global_tests_done", clusterer.exactness_report_)

if __name__ == '__main__':
    unittest.main()
