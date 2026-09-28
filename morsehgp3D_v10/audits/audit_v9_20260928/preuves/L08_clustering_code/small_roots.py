import sys, json, glob, math
sys.path.insert(0, '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code')
import probe as P
C, M = P.C, P.M
path = sys.argv[1]; conv = sys.argv[2]
report = json.load(open(path))
cofaces, gabriel, size = M.read_export(report)
births = M.facet_births(cofaces, gabriel, conv)
keep = None if conv == 'boundary' else gabriel
facets, plateaus = C.facet_levels(cofaces, keep)
nodes, roots = C.merge_tree(facets, plateaus, births)
sums, totals, masses, covered = M.measure(cofaces, gabriel, 1, conv)
big = max(roots, key=lambda r: len(nodes[r]['members']) if r in nodes else 1)
bigpts = set(p for f in nodes[big]['members'] for p in f)
print('roots', len(roots), 'big root facets', len(nodes[big]['members']), 'points', len(bigpts))
cat = report['catalogue']
for r in roots:
    if r == big: continue
    mem = nodes[r]['members'] if r in nodes else {r}
    pts = set(p for f in mem for p in f)
    cof = [(v, b) for v, b in cofaces if any(set(f) <= set(v) for f in mem)]
    print('root', r, 'facets', sorted(mem), 'mass %.3f' % sum(float(masses[f]) for f in mem), 'points', sorted(pts), 'in big root points:', pts <= bigpts)
    for v, b in cof[:6]:
        fs = [tuple(x for x in v if x != d) for d in v]
        print('   coface', v, 'beta %.6g' % float(b), 'facets kept', [f in facets for f in fs], 'gabriel', [f in gabriel for f in fs])
    # any coface linking these points' facets with big root
    # other cofaces containing >=K of these points
