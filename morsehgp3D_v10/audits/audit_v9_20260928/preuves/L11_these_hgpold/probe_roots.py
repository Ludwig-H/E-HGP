"""Compte les racines (composantes a r=inf) sous les deux conventions v9, sur nuages aleatoires."""
import json, subprocess, sys, os
import numpy as np
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import measure as M
import cluster as C
B = '/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
D = os.path.dirname(os.path.abspath(__file__))
def cloud(kind, n, seed):
    rng = np.random.default_rng(seed)
    if kind == 'uniform':
        p = rng.uniform(0, 10000, size=(n, 3))
    else:
        centers = rng.uniform(2000, 8000, size=(4, 3))
        p = centers[rng.integers(0, 4, n)] + rng.normal(0, 400, size=(n, 3))
    g = np.unique(np.clip(np.rint(p), 0, 2**17).astype('<u4'), axis=0)
    return g
out = []
for kind in ('uniform', 'blobs'):
    for k in (2, 3):
        for seed in (1, 2):
            g = cloud(kind, 300, seed)
            path = os.path.join(D, 'c.u32le'); open(path, 'wb').write(g.tobytes())
            r = subprocess.run(['nice', '-n', '19', B, '--input', path, '--k', str(k), '--workers', '2'], capture_output=True, text=True)
            if r.returncode != 0:
                print(kind, k, seed, 'REFUS', r.stderr[:120]); continue
            rep = json.loads(r.stdout)
            cof, gab, K = M.read_export(rep)
            row = dict(kind=kind, n=len(g), k=K, seed=seed, cofaces=len(cof))
            ngab = [sum(1 for f in M.facets(v) if f in gab) for v, _ in cof]
            row['cofaces_<=1_gabriel_facet'] = sum(1 for c in ngab if c <= 1)
            for conv in M.CONVENTIONS:
                births = M.facet_births(cof, gab, conv)
                keep = None if conv == 'boundary' else gab
                facets, plateaus = C.facet_levels(cof, keep)
                nodes, roots = C.merge_tree(facets, plateaus, births)
                row['roots_' + conv] = len(roots)
                row['facets_' + conv] = len(facets)
            print(json.dumps(row), flush=True)
