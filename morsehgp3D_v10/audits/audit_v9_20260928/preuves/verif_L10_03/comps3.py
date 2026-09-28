import sys, json, collections
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import measure as M, cluster as C
src = '/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/'
name = sys.argv[1]
rep = json.load(open(src + name))
cofaces, gabriel, K = M.read_export(rep)
del rep
for conv in sys.argv[2:]:
    keep = None if conv == 'boundary' else gabriel
    facets, plateaus = C.facet_levels(cofaces, keep)
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
    inside = sum(1 for v in comp.values() if v is not big and set(p for f in v for p in f) <= bigpts)
    print('%s conv=%s K=%d facets=%d components=%d largest=%d pts_big=%d small_inside_big_cover=%d sizes_head=%s' % (name, conv, K, len(facets), len(comp), sizes[0], len(bigpts), inside, sizes[:6]), flush=True)
