import sys, json, time, resource
W = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0, W + '/tower_clustering_20260928'); sys.path.insert(0, W + '/synthetic_bench_20260928')
import run_tower as R, bench_datasets as data, plan as bench_plan, measure as M, cluster as C
BIN = '/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
OUT = '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_F2_verif'
fam, n, k = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
spec = dict(family=fam, n=n, groups=8, level='medium', noise_fraction=0.0, seed=bench_plan.BASE_SEED)
points, truth, meta = data.generate(spec)
grid, scale = data.quantize(points)
t = time.monotonic(); report = R.export(BIN, grid, k, 2, OUT); te = time.monotonic() - t
native_roots = len(report['native']['roots'])
cofaces, gabriel, size = M.read_export(report)
del report
out = dict(fam=fam, n=n, k=k, native_roots=native_roots, cofaces=len(cofaces), t_export=round(te,1))
for conv in ('gabriel', 'boundary'):
    keep = None if conv == 'boundary' else gabriel
    facets, plateaus = C.facet_levels(cofaces, keep)
    U = C.Union(facets)
    for beta, groups in plateaus:
        for g in groups:
            for f in g[1:]: U.union(g[0], f)
    out[conv] = len({U.find(f) for f in facets})
out['maxrss_mb'] = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss // 1024
print(json.dumps(out), flush=True)
