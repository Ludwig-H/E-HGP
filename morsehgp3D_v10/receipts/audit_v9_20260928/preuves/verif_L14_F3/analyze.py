import csv, statistics as st, collections, random
REC = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/receipts/synthetic_bench_20260928/r1/baselines.csv'
rec = {}
for r in csv.DictReader(open(REC)):
    rec[(r['scene'], r['seed'], r['method'])] = r
rows = list(csv.DictReader(open('verify.csv')))
print('rows', len(rows))
dig_ok = sum(1 for r in rows if rec[(r['scene'], r['seed'], 'hdbscan_oracle')]['digest'] == r['digest'])
o1_ok = sum(1 for r in rows if abs(float(rec[(r['scene'], r['seed'], 'hdbscan_oracle')]['ari']) - float(r['oracle1d'])) < 1e-12)
print('digest match', dig_ok, 'oracle1d reproduces receipt', o1_ok)
def col(c): return [float(r[c]) for r in rows]
ref = [float(rec[(r['scene'], r['seed'], 'hdbscan_oracle')]['ari']) for r in rows]
dflt = [float(rec[(r['scene'], r['seed'], 'hdbscan_default')]['ari']) for r in rows]
print('receipt oracle mean %.4f  default mean %.4f' % (st.mean(ref), st.mean(dflt)))
def wtl(x, y):
    d = [a - b for a, b in zip(x, y)]
    w = sum(1 for v in d if v > 1e-9); l = sum(1 for v in d if v < -1e-9)
    rnd = random.Random(1); bs = sorted(st.mean(d[rnd.randrange(len(d))] for _ in d) for _ in range(2000))
    return '%d-%d-%d  delta=%+.4f CI95[%+.4f,%+.4f]' % (w, len(d) - w - l, l, st.mean(d), bs[50], bs[1949])
for c in ('direct_ms2_sq', 'direct_ms3_sq', 'direct_ms2_sq_fill', 'oracle2d'):
    print('%-20s mean %.4f  vs receipt oracle %s' % (c, st.mean(col(c)), wtl(col(c), ref)))
fam = collections.defaultdict(lambda: collections.defaultdict(list))
for r, o in zip(rows, ref):
    fam[r['family']]['ms2'].append(float(r['direct_ms2_sq'])); fam[r['family']]['or'].append(o)
    fam[r['family']]['or2'].append(float(r['oracle2d']))
for f, v in sorted(fam.items()):
    print('  %-16s n=%3d ms2sq=%.3f oracle1d=%.3f oracle2d=%.3f' % (f, len(v['ms2']), st.mean(v['ms2']), st.mean(v['or']), st.mean(v['or2'])))
print(collections.Counter(r['oracle2d_par'] for r in rows).most_common(8))
# compare to auditor numbers
A = '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L14_clustering_research/L14_heads.csv'
aud = {(r['scene'], r['seed'], r['method']): float(r['ari']) for r in csv.DictReader(open(A))}
for mine, theirs in (('direct_ms2_sq', 'eom_z1_ms2_mcssqrt'), ('direct_ms3_sq', 'eom_z1_ms3_mcssqrt'), ('oracle2d', 'oracle2d_eom_z1'), ('direct_ms2_sq_fill', 'eom_z1_ms2_mcssqrt_fill')):
    diffs = [abs(float(r[mine]) - aud[(r['scene'], r['seed'], theirs)]) for r in rows]
    print('%-20s vs auditor %-26s max|diff|=%.2e  n_diff>1e-9=%d' % (mine, theirs, max(diffs), sum(1 for d in diffs if d > 1e-9)))
