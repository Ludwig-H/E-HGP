"""Cout de la chaine Python aux tailles d'interet (8000, 32000) : memoire des ensembles `members`."""
import sys, os, json, math, time, resource, collections
sys.path.insert(0, '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code')
import probe as P
C, M = P.C, P.M
def rss():
    return resource.getrusage(resource.RUSAGE_SELF).ru_maxrss / 1024.0
def count_tree(facets, plateaus):
    """Replique de merge_tree ne gardant que les tailles : somme des |members|."""
    union = C.Union(facets)
    top = {f: f for f in facets}
    size = {}
    total, count = 0, 0
    for beta, groups in plateaus:
        merged = collections.defaultdict(set)
        for group in groups:
            roots = {union.find(f) for f in group}
            if len(roots) < 2: continue
            anchor = min(roots, key=lambda r: str(r))
            for r in roots: union.union(anchor, r)
            merged[union.find(anchor)].update(roots)
        for root, roots in merged.items():
            children = {top[r] for r in roots}
            if len(children) < 2: continue
            name = ('n', count); count += 1
            s = sum(size.get(ch, 1) for ch in children)
            size[name] = s; total += s
            for r in roots: top[r] = name
            top[root] = name
    return count, total
fam, n, conv = sys.argv[1], int(sys.argv[2]), sys.argv[3]
full = len(sys.argv) > 4 and sys.argv[4] == 'full'
spec = dict(family=fam, n=n, groups=8, level='medium', noise_fraction=0.0, seed=P.bench_plan.BASE_SEED)
points, truth, meta = P.data.generate(spec)
grid, scale = P.data.quantize(points)
tag = '%s_medium_%d_8_0.0_s%d_k2' % (fam, n, P.bench_plan.BASE_SEED)
path = os.path.join(P.OUT, tag + '.json')
t = time.monotonic()
if os.path.exists(path):
    report = json.load(open(path)); t_export = None
else:
    report = P.R.export(P.BIN, grid, 2, 2, P.OUT); t_export = time.monotonic() - t
    json.dump(report, open(path, 'w'))
out = dict(tag=tag, conv=conv, t_export=t_export, native_roots=len(report['native']['roots']), native_nodes=len(report['native']['nodes']))
t = time.monotonic(); cofaces, gabriel, size = M.read_export(report); out['t_read'] = round(time.monotonic() - t, 2)
births = M.facet_births(cofaces, gabriel, conv)
keep = None if conv == 'boundary' else gabriel
facets, plateaus = C.facet_levels(cofaces, keep)
out.update(cofaces=len(cofaces), facets=len(facets), plateaus=len(plateaus), multi=sum(1 for b, g in plateaus if len(g) > 1))
t = time.monotonic(); cnt, total = count_tree(facets, plateaus); out.update(nodes=cnt, members_total=total, t_count=round(time.monotonic() - t, 2))
t = time.monotonic(); sums, totals, masses, covered = M.measure(cofaces, gabriel, 1, conv); out['t_measure'] = round(time.monotonic() - t, 2)
out['rss_mb_before_tree'] = round(rss())
if full:
    t = time.monotonic(); nodes, roots = C.merge_tree(facets, plateaus, births); out['t_merge_tree'] = round(time.monotonic() - t, 2)
    out['roots'] = len(roots); out['rss_mb_after_tree'] = round(rss())
    t = time.monotonic(); clusters, order = C.condense(nodes, roots, masses, births, math.sqrt(n), 'radius', 1); out['t_condense'] = round(time.monotonic() - t, 2)
    t = time.monotonic(); sel = C.select(clusters, order, 'eom'); lab, ties = C.vote(n, C.label_facets(clusters, sel), sums, totals, len(sel)); out['t_select_vote'] = round(time.monotonic() - t, 2)
    sc = P.baselines.scores(truth, P.np.asarray(lab)); out['ari_eom'] = round(sc['ari'], 4); out['clusters'] = sc['clusters']
    out['rss_mb_end'] = round(rss())
print(json.dumps(out))
