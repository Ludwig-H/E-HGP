"""Isole l'effet des racines legeres : meme condense, liste de racines complete vs filtree (masse >= sqrt(n))."""
import sys, os, json, math, time
import numpy as np
HERE = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0, HERE + '/tower_clustering_20260928')
sys.path.insert(0, HERE + '/synthetic_bench_20260928')
import cluster as C, measure as M, run_tower as R
import baselines, bench_datasets as data, plan as bench_plan
BIN = '/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
EXP = '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code/exports'
MINE = os.path.dirname(os.path.abspath(__file__))
fam, n, conv = sys.argv[1], int(sys.argv[2]), sys.argv[3]
spec = dict(family=fam, n=n, groups=8, level='medium', noise_fraction=0.0, seed=bench_plan.BASE_SEED)
points, truth, meta = data.generate(spec)
grid, scale = data.quantize(points)
tag = '%s_medium_%d_8_0.0_s%d_k2' % (fam, n, bench_plan.BASE_SEED)
path = os.path.join(EXP, tag + '.json')
if os.path.exists(path):
    report = json.load(open(path))
else:
    report = R.export(BIN, grid, 2, 2, MINE)
n = len(points)
mcm = math.sqrt(n)
cofaces, gabriel, size = M.read_export(report)
births = M.facet_births(cofaces, gabriel, conv)
keep = None if conv == 'boundary' else gabriel
facets, plateaus = C.facet_levels(cofaces, keep)
nodes, roots = C.merge_tree(facets, plateaus, births)
sums, totals, masses, covered = M.measure(cofaces, gabriel, 1, conv)
rm = {r: sum(float(masses[f]) for f in (nodes[r]['members'] if r in nodes else {r})) for r in roots}
heavy = [r for r in roots if rm[r] >= mcm]
out = dict(tag=tag, conv=conv, native_roots=len(report['native']['roots']), roots=len(roots), heavy_roots=len(heavy),
           light_masses=sorted(round(rm[r], 3) for r in roots if rm[r] < mcm)[-5:], light_max=round(max([rm[r] for r in roots if rm[r] < mcm] or [0]), 3))
for scale_ in ('lambda', 'log'):
    for label, rl in (('all', roots), ('heavy', heavy)):
        clusters, order = C.condense(nodes, rl, masses, births, mcm, 'radius', 1, scale_)
        for method in C.SELECTIONS:
            sel = C.select(clusters, order, method)
            lab, ties = C.vote(n, C.label_facets(clusters, sel), sums, totals, len(sel))
            sc = baselines.scores(truth, np.asarray(lab))
            light_sel = sum(1 for s in sel if clusters[s]['parent'] is None and clusters[s]['mass'] < mcm)
            out['%s_%s_%s' % (scale_, method, label)] = dict(sel=len(sel), light_sel=light_sel, groups=sc['clusters'], ari=round(sc['ari'], 4), cov=round(sc['coverage'], 4))
print(json.dumps(out), flush=True)
