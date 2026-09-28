import sys, os, json, time
sys.path.insert(0, '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code')
import probe as P, lost_probe as L
fam = sys.argv[1]; k = int(sys.argv[2])
spec = dict(family=fam, n=2000, groups=8, level='medium', noise_fraction=0.0, seed=P.bench_plan.BASE_SEED)
points, truth, meta = P.data.generate(spec)
grid, scale = P.data.quantize(points)
t = time.monotonic(); report = P.R.export(P.BIN, grid, k, 2, P.OUT); te = time.monotonic() - t
cofaces, gabriel, size = P.M.read_export(report)
for conv in ('gabriel', 'boundary'):
    keep = None if conv == 'boundary' else gabriel
    facets, plateaus = P.C.facet_levels(cofaces, keep)
    out = L.audit_tree(facets, plateaus)
    out.update(fam=fam, k=k, conv=conv, native_roots=len(report['native']['roots']), t_export=round(te, 1), cofaces=len(cofaces))
    print(json.dumps(out), flush=True)
