import sys, json
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import measure as M, cluster as C
src = '/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/'
for name in sys.argv[1:]:
    rep = json.load(open(src + name))
    cofaces, gabriel, K = M.read_export(rep)
    pts = set(v for vs, _ in cofaces for v in vs)
    for conv in ('gabriel', 'boundary'):
        keep = None if conv == 'boundary' else gabriel
        facets, plateaus = C.facet_levels(cofaces, keep)
        uf = C.Union(facets)
        for beta, groups in plateaus:
            for g in groups:
                for f in g[1:]:
                    uf.union(g[0], f)
        comps = len({uf.find(f) for f in facets})
        covered = set(v for f in facets for v in f)
        print('%s K=%d conv=%s cofaces=%d facets=%d components=%d points_in_cofaces=%d points_covered_by_F=%d' % (name, K, conv, len(cofaces), len(facets), comps, len(pts), len(covered)), flush=True)
