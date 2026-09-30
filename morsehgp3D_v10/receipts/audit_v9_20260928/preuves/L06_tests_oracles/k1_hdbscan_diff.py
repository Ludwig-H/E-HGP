"""Differentiel couche clustering : tour K=1 (masses unitaires, convention boundary) + cluster.py (HEAD)
contre sklearn HDBSCAN(min_samples=1) sur les MEMES entiers. A K=1, L_1 = union de boules : l'arbre est le
single linkage euclidien, lambda_HGP = 1/r = 2/d = 2 lambda_HDBSCAN (facteur constant : EOM invariant)."""
import json, subprocess, sys, os
import numpy as np
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'headclust'))
import cluster as C, measure as M

def cloud(seed, n, groups):
    rng = np.random.default_rng(seed)
    centers = rng.uniform(0, 4000, size=(groups, 3))
    sizes = rng.multinomial(n, rng.dirichlet(np.ones(groups) * 2))
    pts = np.vstack([rng.normal(c, rng.uniform(60, 250), size=(s, 3)) for c, s in zip(centers, sizes)])
    pts = np.clip(np.rint(pts), 0, 262143).astype(np.int64)
    _, idx = np.unique(pts, axis=0, return_index=True)
    pts = pts[np.sort(idx)]
    return pts

def run(binary, seed, n, groups, mcs, path):
    pts = cloud(seed, n, groups)
    open(path, 'wb').write(pts.astype('<u4').tobytes())
    out = subprocess.run([binary, '--input', path, '--k', '1', '--workers', '1'], capture_output=True, text=True)
    if out.returncode: return None
    rep = json.loads(out.stdout)
    cof, gab, size = M.read_export(rep)
    births = M.facet_births(cof, gab, 'boundary')
    facets, plateaus = C.facet_levels(cof, None)
    nodes, roots = C.merge_tree(facets, plateaus, births)
    sums, totals, masses, covered = M.measure(cof, gab, 1, 'boundary')
    clusters, order = C.condense(nodes, roots, masses, births, mcs, 'radius', 1)
    sel = C.select_excess_of_mass(clusters, order)
    lab, ties = C.vote(len(pts), C.label_facets(clusters, sel), sums, totals, len(sel))
    lab = np.asarray(lab)
    h = HDBSCAN(min_cluster_size=mcs, min_samples=1, cluster_selection_method='eom',
                allow_single_cluster=False, copy=True).fit(pts.astype(float)).labels_
    same = adjusted_rand_score(lab, h)
    noise_eq = bool(((lab < 0) == (h < 0)).all())
    return dict(seed=seed, n=len(pts), groups=groups, mcs=mcs, hgp_clusters=int(lab.max() + 1),
                hdb_clusters=int(h.max() + 1), hgp_noise=int((lab < 0).sum()), hdb_noise=int((h < 0).sum()),
                ari_between=round(same, 4), noise_equal=noise_eq)

if __name__ == '__main__':
    binary = sys.argv[1]; path = sys.argv[2]
    rows = []
    for seed in range(6):
        for mcs in (10, 25):
            r = run(binary, seed, 1500, 3 + seed % 4, mcs, path)
            print(json.dumps(r), flush=True); rows.append(r)
    exact = sum(1 for r in rows if r and r['ari_between'] == 1.0 and r['noise_equal'])
    print('k1_hdbscan_diff exact_matches=%d/%d' % (exact, len(rows)))
