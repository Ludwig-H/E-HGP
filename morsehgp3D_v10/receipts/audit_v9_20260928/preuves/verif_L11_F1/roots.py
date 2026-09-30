import json, subprocess, sys, os
import numpy as np
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import measure as M, cluster as C
B = '/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
D = os.path.dirname(os.path.abspath(__file__))
n, k, seed = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3])
rng = np.random.default_rng(seed)
centers = rng.uniform(2000, 8000, size=(4, 3))
p = centers[rng.integers(0, 4, n)] + rng.normal(0, 400, size=(n, 3))
g = np.unique(np.clip(np.rint(p), 0, 2**17).astype('<u4'), axis=0)
path = os.path.join(D, 'blob_%d_%d_%d.u32le' % (n, k, seed)); open(path, 'wb').write(g.tobytes())
r = subprocess.run(['nice', '-n', '19', B, '--input', path, '--k', str(k), '--workers', '2'], capture_output=True, text=True)
rep = json.loads(r.stdout)
json.dump(rep, open(path + '.json', 'w'))
cof, gab, K = M.read_export(rep)
print('status', rep.get('status'), 'cofaces', len(cof), 'gabriel facets', len(gab))
for conv in M.CONVENTIONS:
    births = M.facet_births(cof, gab, conv)
    keep = None if conv == 'boundary' else gab
    facets, plateaus = C.facet_levels(cof, keep)
    nodes, roots = C.merge_tree(facets, plateaus, births)
    info = []
    for rt in roots:
        mem = nodes[rt]['members'] if rt in nodes else {rt}
        pts = set(x for f in mem for x in f)
        info.append((len(mem), len(pts), str(nodes[rt]['level']) if rt in nodes else 'leaf'))
    info.sort(reverse=True)
    print(conv, 'facets', len(facets), 'roots', len(roots), 'top (facets, points, level):', info[:5])
    if conv == 'boundary' and len(roots) > 1:
        small = [rt for rt in roots if (len(nodes[rt]['members']) if rt in nodes else 1) < 50]
        for rt in small:
            mem = nodes[rt]['members'] if rt in nodes else {rt}
            print('  small root', rt, sorted(mem))
            pts = sorted(set(x for f in mem for x in f))
            cs = [(v, str(b)) for v, b in cof if set(v) <= set(pts) or any(tuple(sorted(set(v)-{d})) in mem for d in v)]
            print('  cofaces touching', cs[:20])
