import json, sys
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import measure as M, cluster as C
r = json.load(open(sys.argv[1]))
cofaces, gabriel, size = M.read_export(r)
print('cofaces', cofaces); print('gabriel facets', gabriel)
for conv in M.CONVENTIONS:
    births = M.facet_births(cofaces, gabriel, conv)
    keep = None if conv == 'boundary' else gabriel
    facets, plateaus = C.facet_levels(cofaces, keep)
    nodes, roots = C.merge_tree(facets, plateaus, births)
    print(conv, 'facets', sorted(facets), 'roots', roots)
    for name, node in nodes.items():
        print('   ', name, node['level'], node['children'])
