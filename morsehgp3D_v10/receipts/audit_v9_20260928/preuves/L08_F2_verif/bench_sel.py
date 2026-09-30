import sys, json, math
import numpy as np
W = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0, W + '/tower_clustering_20260928'); sys.path.insert(0, W + '/synthetic_bench_20260928')
import run_tower as R, bench_datasets as data, plan as bench_plan, measure as M, cluster as C
BIN = '/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
OUT = '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_F2_verif'
fam = sys.argv[1]
spec = dict(family=fam, n=2000, groups=8, level='medium', noise_fraction=0.0, seed=bench_plan.BASE_SEED)
points, truth, meta = data.generate(spec)
grid, scale = data.quantize(points)
report = R.export(BIN, grid, 2, 2, OUT)
cofaces, gabriel, size = M.read_export(report)
thr = math.sqrt(len(points))
births = M.facet_births(cofaces, gabriel, 'gabriel')
facets, plateaus = C.facet_levels(cofaces, gabriel)
nodes, roots = C.merge_tree(facets, plateaus, births)
sums, totals, masses, covered = M.measure(cofaces, gabriel, 1, 'gabriel')
clusters, order = C.condense(nodes, roots, masses, births, thr, 'radius', 1)
for meth in C.SELECTIONS:
    sel = C.select(clusters, order, meth)
    small = [(s, round(clusters[s]['mass'], 3)) for s in sel if clusters[s]['parent'] is None and clusters[s]['mass'] < thr]
    labels, ties = C.vote(len(points), C.label_facets(clusters, sel), sums, totals, len(sel))
    lab = np.asarray(labels)
    idx = {s: i for i, s in enumerate(sel)}
    pts_in_small = sum(int((lab == idx[s]).sum()) for s, _ in small)
    print(fam, meth, 'selected', len(sel), 'sub-threshold root clusters', small, 'points labelled by them', pts_in_small)
