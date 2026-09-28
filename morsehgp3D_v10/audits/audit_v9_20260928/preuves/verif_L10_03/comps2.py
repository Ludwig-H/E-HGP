import sys, json, collections
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import measure as M, cluster as C
src = '/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/'
for name in sys.argv[1:]:
    rep = json.load(open(src + name))
    nat = rep['native']
    print(name, 'native k', nat['k'], 'computed_orders', nat.get('computed_orders'), 'native roots', len(nat['roots']), str(nat['roots'])[:200])
    cofaces, gabriel, K = M.read_export(rep)
    for conv in ('gabriel', 'boundary'):
        births = M.facet_births(cofaces, gabriel, conv)
        keep = None if conv == 'boundary' else gabriel
        facets, plateaus = C.facet_levels(cofaces, keep)
        nodes, roots = C.merge_tree(facets, plateaus, births)
        # component coverage
        uf = C.Union(facets)
        for beta, groups in plateaus:
            for g in groups:
                for f in g[1:]:
                    uf.union(g[0], f)
        comp = collections.defaultdict(list)
        for f in facets: comp[uf.find(f)].append(f)
        sizes = sorted((len(v) for v in comp.values()), reverse=True)
        big = max(comp.values(), key=len)
        bigpts = set(p for f in big for p in f)
        others_inside = sum(1 for v in comp.values() if v is not big and set(p for f in v for p in f) <= bigpts)
        allpts = set(p for f in facets for p in f)
        print('  conv=%s K=%d facets=%d merge_tree_roots=%d uf_components=%d largest=%d pts_big=%d pts_all=%d small_inside_big_cover=%d sizes_head=%s' % (conv, K, len(facets), len(roots), len(comp), sizes[0], len(bigpts), len(allpts), others_inside, sizes[:6]), flush=True)
