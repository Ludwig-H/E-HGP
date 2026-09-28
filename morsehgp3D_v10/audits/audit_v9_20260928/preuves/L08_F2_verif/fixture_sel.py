import sys, json, math
W = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928'
sys.path.insert(0, W)
import measure as M, cluster as C
rep = json.load(open(W + '/fixture_tiny64_k2.json'))
cofaces, gabriel, size = M.read_export(rep)
for conv in ('gabriel', 'boundary'):
    births = M.facet_births(cofaces, gabriel, conv)
    keep = None if conv == 'boundary' else gabriel
    facets, plateaus = C.facet_levels(cofaces, keep)
    nodes, roots = C.merge_tree(facets, plateaus, births)
    sums, totals, masses, covered = M.measure(cofaces, gabriel, 1, conv)
    for r in roots:
        mem = nodes[r]['members'] if r in nodes else {r}
        print(conv, 'root', r if r not in nodes else r, 'facets', len(mem), 'mass', round(sum(float(masses[f]) for f in mem), 3))
    clusters, order = C.condense(nodes, roots, masses, births, math.sqrt(64), 'radius', 1)
    for meth in C.SELECTIONS:
        sel = C.select(clusters, order, meth)
        print(conv, meth, [(s, clusters[s]['node'] if clusters[s]['node'] not in nodes else 'node', round(clusters[s]['mass'], 3)) for s in sel])
