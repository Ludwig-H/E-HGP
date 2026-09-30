import sys, json, math, collections
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments/tower_clustering_20260928')
import measure as M, cluster as C
src = '/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/'
name, conv = sys.argv[1], sys.argv[2]
rep = json.load(open(src + name))
n = len(rep['native']['points'])
cofaces, gabriel, K = M.read_export(rep)
del rep
births = M.facet_births(cofaces, gabriel, conv)
keep = None if conv == 'boundary' else gabriel
facets, plateaus = C.facet_levels(cofaces, keep)
nodes, roots = C.merge_tree(facets, plateaus, births)
sums, totals, masses, covered = M.measure(cofaces, gabriel, 1, conv)
def members(r): return nodes[r]['members'] if r in nodes else {r}
rootmass = {r: sum(float(masses.get(f,0)) for f in members(r)) for r in roots}
main = max(roots, key=lambda r: len(members(r)))
mass = math.sqrt(n)
print('%s conv=%s K=%d roots=%d root_masses_sorted=%s' % (name, conv, K, len(roots), [round(v,1) for v in sorted(rootmass.values(), reverse=True)[:8]]))
for scale in C.STABILITY_SCALES:
    clusters, order = C.condense(nodes, roots, masses, births, mass, 'radius', 1, scale)
    def top(c):
        while clusters[c]['parent'] is not None: c = clusters[c]['parent']
        return clusters[c]['node']
    for method in C.SELECTIONS:
        sel = C.select(clusters, order, method)
        labels, ties = C.vote(n, C.label_facets(clusters, sel), sums, totals, len(sel))
        spurious = [i for i, c in enumerate(sel) if top(c) != main]
        pts_sp = sum(1 for l in labels if l in set(spurious))
        sizes = collections.Counter(l for l in labels if l >= 0)
        print('   scale=%s sel=%s selected=%d from_non_main_roots=%d points_labelled_by_them=%d noise=%d sizes_head=%s' % (scale, method, len(sel), len(spurious), pts_sp, labels.count(-1), sorted(sizes.values(), reverse=True)[:10]), flush=True)
