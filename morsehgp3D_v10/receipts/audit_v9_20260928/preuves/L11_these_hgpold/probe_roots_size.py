import json, subprocess, sys, os
import numpy as np
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import measure as M
import cluster as C
B = '/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
D = os.path.dirname(os.path.abspath(__file__))
for n, k, seed in ((300, 3, 1), (300, 3, 2), (1000, 2, 1), (1000, 3, 1)):
    rng = np.random.default_rng(seed)
    centers = rng.uniform(2000, 8000, size=(4, 3))
    p = centers[rng.integers(0, 4, n)] + rng.normal(0, 400, size=(n, 3))
    g = np.unique(np.clip(np.rint(p), 0, 2**17).astype('<u4'), axis=0)
    path = os.path.join(D, 'c2.u32le'); open(path, 'wb').write(g.tobytes())
    r = subprocess.run(['nice', '-n', '19', B, '--input', path, '--k', str(k), '--workers', '2'], capture_output=True, text=True)
    cof, gab, K = M.read_export(json.loads(r.stdout))
    for conv in M.CONVENTIONS:
        births = M.facet_births(cof, gab, conv)
        keep = None if conv == 'boundary' else gab
        facets, plateaus = C.facet_levels(cof, keep)
        nodes, roots = C.merge_tree(facets, plateaus, births)
        sums, totals, masses, covered = M.measure(cof, gab, 1, conv)
        rm = []
        for rt in roots:
            mem = nodes[rt]['members'] if rt in nodes else {rt}
            rm.append(round(float(sum(masses.get(f, 0) for f in mem)), 1))
        rm.sort(reverse=True)
        print(json.dumps(dict(n=len(g), k=K, seed=seed, conv=conv, roots=len(roots), root_masses_top=rm[:6], sqrt_n=round(len(g) ** 0.5, 1))), flush=True)
