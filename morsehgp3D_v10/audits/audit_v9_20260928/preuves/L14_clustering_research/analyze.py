import csv, collections, statistics as st, sys
F = sys.argv[1]
TOWER = '/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/bench/'
REC = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/receipts/synthetic_bench_20260928/r1/baselines.csv'
DIM = {'spherical': 3, 'anisotropic': 3, 'heteroscedastic': 3, 'unbalanced': 3, 'bridge': 3, 'hierarchical': 3, 'shells': 2, 'filaments': 1}
rows = list(csv.DictReader(open(F)))
res = collections.defaultdict(dict)
meta = {}
for r in rows:
    k = (r['scene'], r['seed'])
    res[k][r['method']] = (float(r['ari']), float(r['coverage']), int(r['clusters']), r['extra'])
    meta[k] = r
for r in csv.DictReader(open(REC)):
    k = (r['scene'], r['seed'])
    if k in res:
        res[k]['receipt_' + r['method']] = (float(r['ari']), float(r['coverage']), int(r['clusters']), r['parameter'])
for name, f in (('tower_prefix', 'tour_z1.csv'), ('tower_postfix', 'tour_z1b.csv')):
    for r in csv.DictReader(open(TOWER + f)):
        k = (r['scene'], r['seed'])
        if k in res:
            res[k][name] = (float(r['ari']), float(r['coverage']), int(r['clusters']), '')
# declared-z variant: z = intrinsic dimension of the family (side information, declared)
for k, v in res.items():
    d = DIM[meta[k]['family']]
    for tag in ('mcssqrt', 'mcs20'):
        for ms in (2, 3, 5, 8, 12, 20, 32):
            for fill in ('', '_fill'):
                src = 'eom_z%d_ms%d_%s%s' % (d, ms, tag, fill)
                if src in v:
                    v['eom_zdim_ms%d_%s%s' % (ms, tag, fill)] = v[src]
keys = sorted(res)
methods = sorted({m for v in res.values() for m in v})
want = [m for m in methods if any(s in m for s in sys.argv[2].split(','))] if len(sys.argv) > 2 else methods


def agg(m, sel=lambda k: True):
    L = [res[k][m] for k in keys if m in res[k] and sel(k)]
    return L


def line(m, ref='receipt_hdbscan_oracle'):
    L = [(res[k][m][0], res[k][ref][0], res[k][m][1], res[k][m][2], int(meta[k]['groups'])) for k in keys if m in res[k] and ref in res[k]]
    if not L:
        return ''
    a = st.mean(x[0] for x in L); b = st.mean(x[1] for x in L)
    w = sum(1 for x in L if x[0] > x[1] + 1e-9); l = sum(1 for x in L if x[0] < x[1] - 1e-9)
    ratio = sorted(x[3] / x[4] for x in L)[len(L) // 2]
    fams = collections.defaultdict(list)
    for k in keys:
        if m in res[k]:
            fams[meta[k]['family']].append(res[k][m][0])
    fam = ' '.join('%s=%.2f' % (f[:4], st.mean(v)) for f, v in sorted(fams.items()))
    return '%-34s n=%3d ARI=%.3f cov=%.2f k/g=%.2f | vs oracle(%.3f) %3d-%d-%-3d | %s' % (
        m, len(L), a, st.mean(x[2] for x in L), ratio, b, w, len(L) - w - l, l, fam)


for m in want:
    s = line(m)
    if s:
        print(s)
