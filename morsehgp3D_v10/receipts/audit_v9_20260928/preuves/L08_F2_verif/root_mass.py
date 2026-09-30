import sys, json, math, collections
W = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0, W + '/tower_clustering_20260928'); sys.path.insert(0, W + '/synthetic_bench_20260928')
import run_tower as R, bench_datasets as data, plan as bench_plan, measure as M, cluster as C
BIN = '/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
OUT = '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_F2_verif'
fam, k = sys.argv[1], int(sys.argv[2])
spec = dict(family=fam, n=2000, groups=8, level='medium', noise_fraction=0.0, seed=bench_plan.BASE_SEED)
points, truth, meta = data.generate(spec)
grid, scale = data.quantize(points)
report = R.export(BIN, grid, k, 2, OUT)
cofaces, gabriel, size = M.read_export(report)
for conv in ('gabriel', 'boundary'):
    keep = None if conv == 'boundary' else gabriel
    facets, plateaus = C.facet_levels(cofaces, keep)
    U = C.Union(facets)
    for beta, groups in plateaus:
        for g in groups:
            for f in g[1:]: U.union(g[0], f)
    sums, totals, masses, covered = M.measure(cofaces, gabriel, 1, conv)
    comp = collections.defaultdict(float)
    for f in facets: comp[U.find(f)] += float(masses[f])
    ms = sorted(comp.values(), reverse=True)
    print(json.dumps(dict(fam=fam, k=k, conv=conv, roots=len(ms), top_mass=round(ms[0], 2), total=round(sum(ms), 2),
                          second=round(ms[1], 3) if len(ms) > 1 else None, above_sqrt_n=sum(1 for m in ms if m >= math.sqrt(2000)))))
