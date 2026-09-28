import sys, json, time, os
W = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0, W + '/tower_clustering_20260928')
sys.path.insert(0, W + '/synthetic_bench_20260928')
import run_tower as R, bench_datasets as data, plan as bench_plan
sys.path.insert(0, '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_F2_verif')
import roots as RT
BIN = '/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
OUT = '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_F2_verif'
fam, n, k = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
spec = dict(family=fam, n=n, groups=8, level='medium', noise_fraction=0.0, seed=bench_plan.BASE_SEED)
points, truth, meta = data.generate(spec)
grid, scale = data.quantize(points)
t = time.monotonic(); report = R.export(BIN, grid, k, 2, OUT); te = time.monotonic() - t
r = RT.run(report)
r.update(fam=fam, n_spec=n, k=k, t_export=round(te, 1), digest=meta['digest'])
print(json.dumps(r), flush=True)
