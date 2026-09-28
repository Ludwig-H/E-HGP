"""Sonde en lecture seule : la convention 'gabriel' de v9 casse-t-elle la connexite ?"""
import json, sys
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import measure as M
import cluster as C
report = json.load(open(sys.argv[1]))
cofaces, gabriel, K = M.read_export(report)
print('K =', K, 'cofaces =', [(v, str(b)) for v, b in cofaces])
print('gabriel facets =', sorted(gabriel))
for conv in M.CONVENTIONS:
    births = M.facet_births(cofaces, gabriel, conv)
    keep = None if conv == 'boundary' else gabriel
    facets, plateaus = C.facet_levels(cofaces, keep)
    nodes, roots = C.merge_tree(facets, plateaus, births)
    sums, totals, masses, covered = M.measure(cofaces, gabriel, 2, conv)
    print('\n[%s] facets=%s' % (conv, sorted(facets)))
    print('  roots (composantes a r=inf) =', len(roots), roots)
    for name, node in sorted(nodes.items()):
        print('  node', name, 'level', node['level'], 'children', node['children'])
    print('  masses z=2 =', {f: str(m) for f, m in sorted(masses.items())})
