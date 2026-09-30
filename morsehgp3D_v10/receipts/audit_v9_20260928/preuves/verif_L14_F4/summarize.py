import csv, sys, collections, statistics as st, random
rows = list(csv.DictReader(open(sys.argv[1])))
T = '/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/bench/tour_z1.csv'
ref = {(r['scene'], r['seed']): float(r['ari']) for r in csv.DictReader(open(T))}
D = collections.defaultdict(dict)
for r in rows:
    D[(r['scene'], r['seed'])][r['method']] = {k: float(r[k]) for k in ('ari', 'ami', 'cov', 'ari_cov', 'ari_fill')}
mx = max(abs(D[k]['tower_k2_z1']['ari'] - ref[k]) for k in D)
print('runs', len(D), 'max |tower - tour_z1.csv| =', mx)
def boot(d):
    r = random.Random(1); bs = sorted(sum(d[r.randrange(len(d))] for _ in d) / len(d) for _ in range(2000)); return bs[50], bs[1949]
for grp, sel in (('noise0', lambda k: k[0].endswith('noise0')), ('noise0p3', lambda k: k[0].endswith('noise0p3')), ('noise0p1', lambda k: k[0].endswith('noise0p1')), ('all', lambda k: True)):
    ks = [k for k in sorted(D) if sel(k)]
    print('==', grp, len(ks))
    for m in ('tower_k2_z1', 'hdbscan_default', 'eom_z1_ms2_sqrt', 'eom_z1_ms3_sqrt', 'eom_z3_ms2_sqrt', 'eom_z3_ms3_sqrt'):
        v = [D[k][m] for k in ks]
        print('  %-16s ARI=%.3f AMI=%.3f cov=%.3f ARI_cov=%.3f ARI_fill=%.3f' % (m, *(st.mean(x[f] for x in v) for f in ('ari', 'ami', 'cov', 'ari_cov', 'ari_fill'))))
    for m in ('hdbscan_default', 'eom_z1_ms2_sqrt', 'eom_z1_ms3_sqrt', 'eom_z3_ms3_sqrt'):
        for f in ('ari', 'ari_cov', 'ari_fill'):
            d = [D[k]['tower_k2_z1'][f] - D[k][m][f] for k in ks]
            lo, hi = boot(d); w = sum(x > 1e-9 for x in d); l = sum(x < -1e-9 for x in d)
            print('    tower-%-16s %-8s d=%+.3f [%+.3f,%+.3f] %d-%d-%d' % (m, f, st.mean(d), lo, hi, w, len(d) - w - l, l))
    d = [D[k]['tower_k2_z1']['ari'] - D[k]['eom_z3_ms3_sqrt']['ari_fill'] for k in ks]
    lo, hi = boot(d); print('    tower(ari) - eom_z3_ms3 fill: d=%+.3f [%+.3f,%+.3f]' % (st.mean(d), lo, hi))
    d = [D[k]['tower_k2_z1']['ari'] - D[k]['eom_z1_ms3_sqrt']['ari_fill'] for k in ks]
    lo, hi = boot(d); print('    tower(ari) - eom_z1_ms3 fill: d=%+.3f [%+.3f,%+.3f]' % (st.mean(d), lo, hi))
    d = [D[k]['tower_k2_z1']['ari'] - D[k]['eom_z1_ms2_sqrt']['ari_fill'] for k in ks]
    lo, hi = boot(d); print('    tower(ari) - eom_z1_ms2 fill: d=%+.3f [%+.3f,%+.3f]' % (st.mean(d), lo, hi))
