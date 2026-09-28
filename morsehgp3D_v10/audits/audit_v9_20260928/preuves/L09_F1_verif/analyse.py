import csv, collections, statistics as st, sys
W = sys.argv[1]
B = '/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/bench'
R = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/receipts/synthetic_bench_20260928/r1/baselines.csv'
dec = list(csv.DictReader(open(W + '/decomp_all.csv')))
z1 = {(r['scene'], r['seed']): float(r['ari']) for r in csv.DictReader(open(B + '/tour_z1.csv'))}
z1b = {(r['scene'], r['seed']): float(r['ari']) for r in csv.DictReader(open(B + '/tour_z1b.csv'))}
ref = collections.defaultdict(dict)
for r in csv.DictReader(open(R)):
    ref[r['method']][(r['scene'], r['seed'])] = float(r['ari'])
var = collections.defaultdict(dict); meta = {}
for r in dec:
    k = (r['scene'], r['seed']); var[r['variant']][k] = (float(r['ari']), int(r['clusters'])); meta[k] = (r['family'], r['level'])
def exact(v, tgt):
    same = sum(1 for k in var[v] if abs(var[v][k][0] - tgt[k]) < 1e-12)
    return '%d/%d' % (same, len(var[v]))
print('cda_eom == tour_z1:', exact('cda_eom', z1), ' head_eom == tour_z1b:', exact('head_eom', z1b))
for v in var:
    print('\n==', v, 'single-cluster rows', sum(1 for k in var[v] if var[v][k][1] == 1), 'mean ARI %.3f' % st.mean(a for a, _ in var[v].values()))
    for refname in ('hdbscan_oracle', 'hdbscan_default'):
        groups = collections.defaultdict(list)
        for k, (a, _) in var[v].items():
            d = a - ref[refname][k]
            groups['all'].append(d); groups['family=' + meta[k][0]].append(d); groups['level=' + meta[k][1]].append(d)
        out = []
        for g in sorted(groups):
            ds = groups[g]
            out.append('%s %+.3f %d-%d' % (g, st.mean(ds), sum(d > 1e-9 for d in ds), sum(d < -1e-9 for d in ds)))
        print(' vs', refname + ':', ' | '.join(out))
