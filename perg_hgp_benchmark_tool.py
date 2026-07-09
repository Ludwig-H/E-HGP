import argparse
import time
import sys
import os
import json
import numpy as np
import torch

try:
    import resource
    def get_peak_ram_mb():
        # ru_maxrss is in kilobytes on Linux
        return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / 1024.0
except ImportError:
    def get_peak_ram_mb():
        return 0.0

try:
    from hdbscan import HDBSCAN
    hdbscan_available = True
except ImportError:
    hdbscan_available = False

# Import perg_hgp relatively
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "perg_hgp")))
try:
    from perg_hgp import PERGHGPClusterer
except ImportError:
    from perg_hgp.perg_hgp.estimator import PERGHGPClusterer

from sklearn.metrics import adjusted_rand_score
from sklearn.datasets import make_blobs

def make_concentric_spheres(n_samples=10000, noise=0.08):
    n_inner = n_samples // 2
    r_inner = 1.0
    phi = np.random.uniform(0, 2 * np.pi, n_inner)
    theta = np.random.uniform(0, np.pi, n_inner)
    x = r_inner * np.sin(theta) * np.cos(phi) + np.random.normal(0, noise, n_inner)
    y = r_inner * np.sin(theta) * np.sin(phi) + np.random.normal(0, noise, n_inner)
    z = r_inner * np.cos(theta) + np.random.normal(0, noise, n_inner)
    X_inner = np.column_stack([x, y, z])
    y_inner = np.zeros(n_inner)
    
    n_outer = n_samples - n_inner
    r_outer = 4.0
    phi = np.random.uniform(0, 2 * np.pi, n_outer)
    theta = np.random.uniform(0, np.pi, n_outer)
    x = r_outer * np.sin(theta) * np.cos(phi) + np.random.normal(0, noise * 2, n_outer)
    y = r_outer * np.sin(theta) * np.sin(phi) + np.random.normal(0, noise * 2, n_outer)
    z = r_outer * np.cos(theta) + np.random.normal(0, noise * 2, n_outer)
    X_outer = np.column_stack([x, y, z])
    y_outer = np.ones(n_outer)
    
    return np.vstack([X_inner, X_outer]).astype(np.float32), np.concatenate([y_inner, y_outer])

def main():
    parser = argparse.ArgumentParser(description="PERG-HGP Performance and Scaling Benchmark Tool")
    parser.add_argument("--n_samples", type=int, default=10000, help="Number of points in dataset")
    parser.add_argument("--centers", type=int, default=5, help="Number of centers for blobs")
    parser.add_argument("--k", type=int, default=10, help="Order K of Čech complex")
    parser.add_argument("--expz", type=float, default=2.0, help="Weight scaling exponent expZ")
    parser.add_argument("--k_rho", type=int, default=None, help="K_rho neighborhood parameter")
    parser.add_argument("--min_cluster_size", type=int, default=100, help="Min cluster size for tree condensation")
    parser.add_argument("--device", type=str, default="cpu", help="Calculation device (cpu or cuda)")
    parser.add_argument("--seed", type=int, default=42, help="Global random seed")
    parser.add_argument("--dataset", type=str, choices=["blobs", "spheres"], default="blobs", help="Dataset geometry")
    parser.add_argument("--exactness_mode", type=str, default="atlas_exact", help="Exactness configuration mode")
    parser.add_argument("--output_format", type=str, choices=["markdown", "json"], default="markdown", help="Output display format")
    parser.add_argument("--output_file", type=str, default=None, help="File to write benchmark results")
    
    args = parser.parse_args()
    
    # Configure global seeds
    np.random.seed(args.seed)
    torch.manual_seed(args.seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(args.seed)
        torch.cuda.reset_peak_memory_stats()
        
    # Generate data
    if args.dataset == "blobs":
        X, y = make_blobs(n_samples=args.n_samples, centers=args.centers, n_features=3, cluster_std=1.0, random_state=args.seed)
    else:
        X, y = make_concentric_spheres(n_samples=args.n_samples, noise=0.08)
    X = X.astype(np.float32)
    
    results = {}
    
    # Cap parameters for small N to prevent invalid KNN setups
    k_rho_val = args.k_rho if args.k_rho is not None else 64
    k_rho_val = min(k_rho_val, args.n_samples)
    m_local_val = min(128, args.n_samples)
    m_active_val = min(128, args.n_samples)
    min_cluster_size_val = min(args.min_cluster_size, args.n_samples // 2)
    min_cluster_size_val = max(2, min_cluster_size_val)
    
    # 1. Run PERG-HGP
    t0 = time.time()
    
    clusterer = PERGHGPClusterer(
        K=args.k,
        K_rho=k_rho_val,
        expZ=args.expz,
        m_local=m_local_val,
        m_active=m_active_val,
        min_cluster_size=min_cluster_size_val,
        device=args.device,
        exactness_mode=args.exactness_mode
    )
    
    labels_perg = clusterer.fit_predict(X)
    t_perg = time.time() - t0
    
    ram_peak_perg = get_peak_ram_mb()
    vram_peak_perg = 0.0
    if args.device == "cuda" and torch.cuda.is_available():
        vram_peak_perg = torch.cuda.max_memory_allocated() / (1024 * 1024)
        
    ari_perg = adjusted_rand_score(y, labels_perg)
    n_cl_perg = len(np.unique(labels_perg[labels_perg >= 0]))
    pct_perg = np.sum(labels_perg >= 0) / len(labels_perg) * 100
    
    results["PERG-HGP"] = {
        "time_s": t_perg,
        "ari": ari_perg,
        "clusters": n_cl_perg,
        "pct_clustered": pct_perg,
        "peak_ram_mb": ram_peak_perg,
        "peak_vram_mb": vram_peak_perg,
        "exactness_report": clusterer.exactness_report_
    }
    
    # 2. Run HDBSCAN (if available)
    if hdbscan_available:
        t0 = time.time()
        
        hdb = HDBSCAN(min_cluster_size=min_cluster_size_val)
        labels_hdb = hdb.fit_predict(X)
        t_hdb = time.time() - t0
        
        ram_peak_hdb = get_peak_ram_mb()
        ari_hdb = adjusted_rand_score(y, labels_hdb)
        n_cl_hdb = len(np.unique(labels_hdb[labels_hdb >= 0]))
        pct_hdb = np.sum(labels_hdb >= 0) / len(labels_hdb) * 100
        
        results["HDBSCAN"] = {
            "time_s": t_hdb,
            "ari": ari_hdb,
            "clusters": n_cl_hdb,
            "pct_clustered": pct_hdb,
            "peak_ram_mb": ram_peak_hdb,
            "peak_vram_mb": 0.0
        }
        
    # Format and output results
    if args.output_format == "json":
        output_str = json.dumps(results, indent=2)
        print(output_str)
    else:
        # Markdown table
        lines = []
        lines.append("## PERG-HGP vs HDBSCAN Benchmark Report")
        lines.append(f"- **Dataset**: {args.dataset} ({args.n_samples} points)")
        lines.append(f"- **K**: {args.k} | **expZ**: {args.expz} | **K_rho**: {k_rho_val}")
        lines.append(f"- **Device**: {args.device} | **Exactness Mode**: {args.exactness_mode}")
        lines.append("")
        lines.append("| Algorithm | Execution Time | ARI | Clusters Found | Clustered % | Process Peak RAM | Peak VRAM |")
        lines.append("| --- | --- | --- | --- | --- | --- | --- |")
        
        perg_res = results["PERG-HGP"]
        lines.append(f"| **PERG-HGP** | {perg_res['time_s']:.4f}s | {perg_res['ari']:.4f} | {perg_res['clusters']} | {perg_res['pct_clustered']:.2f}% | {perg_res['peak_ram_mb']:.2f} MB | {perg_res['peak_vram_mb']:.2f} MB |")
        
        if hdbscan_available:
            hdb_res = results["HDBSCAN"]
            lines.append(f"| **HDBSCAN** | {hdb_res['time_s']:.4f}s | {hdb_res['ari']:.4f} | {hdb_res['clusters']} | {hdb_res['pct_clustered']:.2f}% | {hdb_res['peak_ram_mb']:.2f} MB | - |")
            
        lines.append("")
        lines.append("### PERG-HGP Exactness Report:")
        for k, v in perg_res["exactness_report"].items():
            lines.append(f"- **{k}**: {v}")
            
        output_str = "\n".join(lines)
        print(output_str)
        
    if args.output_file:
        with open(args.output_file, "w") as f:
            f.write(output_str)
            
if __name__ == "__main__":
    main()
