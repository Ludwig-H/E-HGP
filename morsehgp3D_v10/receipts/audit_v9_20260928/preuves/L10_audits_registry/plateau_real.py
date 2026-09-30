import sys, json, collections
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import measure as M, cluster as C
src = '/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/'
for name in sys.argv[1:]:
    rep = json.load(open(src + name))
    cofaces, gabriel, K = M.read_export(rep)
    for conv in ('gabriel', 'boundary'):
        keep = None if conv == 'boundary' else gabriel
        births = M.facet_births(cofaces, gabriel, conv)
        facets, plateaus = C.facet_levels(cofaces, keep)
        nodes, roots = C.merge_tree(facets, plateaus, births)
        multi = sum(1 for beta, groups in plateaus if len(groups) > 1)
        # facets reachable from roots
        reach = set()
        for r in roots:
            reach |= (nodes[r]['members'] if r in nodes else {r})
        # same-level parent/child cascades
        casc = sum(1 for n in nodes.values() for c in n['children'] if c in nodes and nodes[c]['level'] == n['level'])
        # true components by union-find over all groups
        uf = C.Union(facets)
        for beta, groups in plateaus:
            for g in groups:
                for f in g[1:]:
                    uf.union(g[0], f)
        comps = len({uf.find(f) for f in facets})
        print('%s K=%d conv=%s cofaces=%d facets=%d plateaus_multi=%d nodes=%d roots=%d true_components=%d reachable=%d lost=%d same_level_cascades=%d'
              % (name, K, conv, len(cofaces), len(facets), multi, len(nodes), len(roots), comps, len(reach), len(facets) - len(reach), casc), flush=True)
