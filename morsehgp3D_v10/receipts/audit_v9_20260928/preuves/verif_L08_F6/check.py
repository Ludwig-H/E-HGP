import sys, json, time, resource, collections
HERE = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0, HERE + '/tower_clustering_20260928')
import cluster as C, measure as M
EXP = '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code/exports/'
path, conv, full = sys.argv[1], sys.argv[2], sys.argv[3] == 'full'
t = time.monotonic(); report = json.load(open(EXP + path)); tj = time.monotonic() - t
cofaces, gabriel, size = M.read_export(report)
births = M.facet_births(cofaces, gabriel, conv)
keep = None if conv == 'boundary' else gabriel
facets, plateaus = C.facet_levels(cofaces, keep)
# depth-only replica: sum over leaves of number of internal ancestors
union = C.Union(facets); top = {f: f for f in facets}; size_ = {}; parent = {}; cnt = 0; tot = 0
for beta, groups in plateaus:
    merged = collections.defaultdict(set)
    for g in groups:
        roots = {union.find(f) for f in g}
        if len(roots) < 2: continue
        a = min(roots, key=str)
        for r in roots: union.union(a, r)
        merged[union.find(a)].update(roots)
    for root, roots in merged.items():
        ch = {top[r] for r in roots}
        if len(ch) < 2: continue
        name = ('n', cnt); cnt += 1
        s = sum(size_.get(c, 1) for c in ch); size_[name] = s; tot += s
        for c in ch: parent[c] = name
        for r in roots: top[r] = name
        top[root] = name
# leaf depth
depth_sum = 0; maxd = 0
memo = {}
def d(x):
    k = 0; y = x
    while y in parent: y = parent[y]; k += 1
    return k
for f in (facets if len(facets) < 20000 else []):
    k = d(f); depth_sum += k; maxd = max(maxd, k)
out = dict(file=path, conv=conv, facets=len(facets), nodes=cnt, sum_members=tot, sum_leaf_depth=depth_sum, max_depth=maxd, mean_depth=round(depth_sum/len(facets),1), t_json=round(tj,2))
if full:
    r0 = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    t = time.monotonic(); nodes, roots = C.merge_tree(facets, plateaus, births); out['t_merge_tree'] = round(time.monotonic()-t, 2)
    out['real_sum_members'] = sum(len(v['members']) for v in nodes.values()); out['real_nodes'] = len(nodes)
    out['rss_before_mb'] = round(r0/1024); out['rss_after_mb'] = round(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss/1024)
print(json.dumps(out))
