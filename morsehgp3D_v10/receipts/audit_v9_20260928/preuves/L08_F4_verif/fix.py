import sys, json
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import cluster as C, measure as M
rep = json.load(open('/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928/fixture_tiny64_k2.json'))
print('native roots', rep.get('native', {}).get('roots') if isinstance(rep.get('native'), dict) else None)
for conv in ('gabriel', 'boundary'):
    cofaces, gabriel, size = M.read_export(rep)
    births = M.facet_births(cofaces, gabriel, conv)
    keep = None if conv == 'boundary' else gabriel
    facets, plateaus = C.facet_levels(cofaces, keep)
    nodes, roots = C.merge_tree(facets, plateaus, births)
    sums, totals, masses, covered = M.measure(cofaces, gabriel, 1, conv)
    print(conv, 'facets', len(facets), 'roots', len(roots))
    for r in roots:
        mem = nodes[r]['members'] if r in nodes else {r}
        print('  root', r, 'nfacets', len(mem), 'mass %.3f' % sum(float(masses.get(f,0)) for f in mem))
    for mcm in (8.0,):
        for scale in C.STABILITY_SCALES:
            clusters, order = C.condense(nodes, roots, masses, births, mcm, 'radius', 1, scale)
            for method in C.SELECTIONS:
                sel = C.select(clusters, order, method)
                print('  ', scale, method, [(s, clusters[s]['node'], round(clusters[s]['mass'],3)) for s in sel])
print('--- vote on fixture, gabriel')
import collections
cofaces, gabriel, size = M.read_export(rep)
births = M.facet_births(cofaces, gabriel, 'gabriel')
facets, plateaus = C.facet_levels(cofaces, gabriel)
nodes, roots = C.merge_tree(facets, plateaus, births)
sums, totals, masses, covered = M.measure(cofaces, gabriel, 1, 'gabriel')
clusters, order = C.condense(nodes, roots, masses, births, 8.0, 'radius', 1, 'lambda')
sel = C.select(clusters, order, 'eom')
labels, ties = C.vote(64, C.label_facets(clusters, sel), sums, totals, len(sel))
print('labels count', collections.Counter(labels), 'ties', ties, 'sel', sel)
for p in (16, 30):
    print(p, 'label', labels[p], 'facets containing', [(f, float(sums[f])/float(totals[p])) for f in sums if p in f])
