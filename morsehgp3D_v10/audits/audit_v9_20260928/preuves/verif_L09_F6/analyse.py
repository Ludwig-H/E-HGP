import csv, collections, sys
rows = list(csv.DictReader(open(sys.argv[1])))
by = {}
for r in rows:
    by[(r['method'], r['scene'], r['seed'])] = r
methods = sorted({r['method'] for r in rows})
keys = sorted({(r['scene'], r['seed']) for r in rows})
print('scenes x seeds:', len(keys))
# gap ari - sing27 per method
for m in methods:
    d = [float(by[(m,)+k]['ari']) - float(by[(m,)+k]['ari_sing27']) for k in keys if (m,)+k in by]
    print('%-24s gap ari-sing27 mean %+.4f min %+.4f max %+.4f  |gap|>0.05: %d/%d' % (m, sum(d)/len(d), min(d), max(d), sum(abs(x)>0.05 for x in d), len(d)))
refs = ['hdbscan_default', 'hdbscan_oracle_ari', 'hdbscan_oracle_sing27']
tow = [m for m in methods if m.startswith('tower_')]
def cmp(ch, ref, metric, refmetric=None, subset=None):
    refmetric = refmetric or metric
    ds = []
    for k in keys:
        if subset and not subset(k): continue
        a = by.get((ch,)+k); b = by.get((ref,)+k)
        if a is None or b is None: continue
        ds.append(float(a[metric]) - float(b[refmetric]))
    w = sum(x > 1e-9 for x in ds); l = sum(x < -1e-9 for x in ds)
    return sum(ds)/len(ds), w, l, len(ds)
print()
print('%-22s %-22s %-28s %-28s' % ('challenger', 'reference', 'ARI tous points (banc)', 'ARI singletons (27)'))
for ch in tow:
    for ref in refs:
        a = cmp(ch, ref, 'ari'); b = cmp(ch, ref, 'ari_sing27')
        print('%-22s %-22s %+.4f %3d-%3d/%d        %+.4f %3d-%3d/%d' % (ch, ref, a[0], a[1], a[2], a[3], b[0], b[1], b[2], b[3]))
# per-scene sign flips tower vs oracle between metrics
print()
for ch in tow:
    for ref, refs2 in (('hdbscan_oracle_ari', 'hdbscan_oracle_sing27'), ('hdbscan_default', 'hdbscan_default')):
        flips = []
        for k in keys:
            a = by.get((ch,)+k); b = by.get((ref,)+k); b2 = by.get((refs2,)+k)
            if not (a and b and b2): continue
            s1 = float(a['ari']) - float(b['ari']); s2 = float(a['ari_sing27']) - float(b2['ari_sing27'])
            if (s1 > 1e-9 and s2 < -1e-9) or (s1 < -1e-9 and s2 > 1e-9):
                flips.append((k[0], k[1], round(s1,3), round(s2,3)))
        print(ch, 'vs', ref, '(bank metric) /', refs2, '(sing27): sign flips', len(flips), flips[:6])
# per family, tower_lambda_leaf and lambda_eom vs oracle
print()
fam = lambda f: (lambda k: k[0].startswith(f + '_'))
for ch in tow:
    for f in sorted({k[0].split('_')[0] for k in keys}):
        a = cmp(ch, 'hdbscan_oracle_ari', 'ari', subset=fam(f)); b = cmp(ch, 'hdbscan_oracle_sing27', 'ari_sing27', subset=fam(f))
        print('%-22s %-16s bank %+.4f (%d-%d/%d)  sing27 %+.4f (%d-%d/%d)' % (ch, f, a[0], a[1], a[2], a[3], b[0], b[1], b[2], b[3]))
