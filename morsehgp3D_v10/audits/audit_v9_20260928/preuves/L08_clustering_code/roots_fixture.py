import sys, json
sys.path.insert(0, '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code')
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import fixture_check as FC, cluster as C, measure as M
rep = json.load(open('/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928/fixture_tiny64_k2.json'))
cofaces, gabriel, facets, nodes, roots = FC.analyse(rep, 'gabriel')
for r in roots:
    mem = nodes[r]['members'] if r in nodes else {r}
    print('root', r, 'members', len(mem), sorted(mem)[:10], 'level', nodes[r]['level'] if r in nodes else None)
small = [r for r in roots if (len(nodes[r]['members']) if r in nodes else 1) < 5]
for r in small:
    mem = nodes[r]['members'] if r in nodes else {r}
    for f in mem:
        cof = [(v, b) for v, b in cofaces if set(f) <= set(v)]
        print('facet', f, 'beta', gabriel[f], 'cofaces', [(v, float(b), [tuple(x for x in v if x != d) in gabriel for d in v]) for v, b in cof])
# masses and condensation with sqrt(n)=8
sums, totals, masses, covered = M.measure(cofaces, gabriel, 1, 'gabriel')
births = M.facet_births(cofaces, gabriel, 'gabriel')
clusters, order = C.condense(nodes, roots, masses, births, 8.0, 'radius', 1)
for name in order:
    c = clusters[name]
    print(name, 'parent', c['parent'], 'children', c['children'], 'mass %.3f' % c['mass'], 'stab %.4g' % c['stability'])
print('eom', C.select(clusters, order, 'eom'), 'leaf', C.select(clusters, order, 'leaf'))
