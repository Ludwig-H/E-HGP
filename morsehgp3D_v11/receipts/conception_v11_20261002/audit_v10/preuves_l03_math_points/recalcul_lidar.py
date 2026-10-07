"""Recalcul independant (audit L03) des nombres LiDAR du rapport de batterie depuis tables/lidar_instances.csv.gz."""
import gzip, csv, collections
B = '/workspaces/E-HGP/build/v10-tour-vers-points/batterie_ab/tables/'
R = list(csv.DictReader(gzip.open(B + 'lidar_instances.csv.gz', 'rt')))
print('lignes', len(R), 'colonnes', list(R[0].keys())[:12])
print('roles', collections.Counter(r['role'] for r in R), 'k', collections.Counter(r['k'] for r in R))
srcs = ['a', 'cover', 'cover1', 'core', 'hdbscan']
for K in ('5', '10'):
    S = [r for r in R if r['k'] == K and int(r['m']) >= 50]
    # dedoublonnage : deux demonstrations sont aussi des trames du criblage -> on garde tout comme le rapport (728)
    print('K=%s instances >= 50 points : %d' % (K, len(S)))
    for s in srcs:
        v = [float(r[s + '_iou']) for r in S]
        ex = sum(int(r[s + '_exact']) for r in S)
        print('   %-8s exactes=%d  >1/2=%.3f  >4/5=%.3f  IoU=%.4f' % ({'a': 'tour'}.get(s, s), ex, sum(x > 0.5 for x in v) / len(v), sum(x > 0.8 for x in v) / len(v), sum(v) / len(v)))
    for role in ('echec', 'temoin', 'demo'):
        T = [r for r in S if r['role'] == role]
        if not T:
            continue
        per = collections.defaultdict(list)
        for r in T:
            per[r['scene']].append(float(r['cover_iou']) - float(r['hdbscan_iou']))
        d = [sum(x) / len(x) for x in per.values()]
        print('   role %-7s trames=%d instances=%d  cover - HDBSCAN par trame : %+0.4f   (+/=/- trames : %d/%d/%d)' % (
            role, len(per), len(T), sum(d) / len(d), sum(x > 0 for x in d), sum(x == 0 for x in d), sum(x < 0 for x in d)))
    # velos
    V = [r for r in S if r['cls'] == 'bicycle']
    print('   velos : %d ; > 1/2 : tour %d, cover %d, hdbscan %d, core %d' % (len(V), sum(float(r['a_iou']) > 0.5 for r in V), sum(float(r['cover_iou']) > 0.5 for r in V),
          sum(float(r['hdbscan_iou']) > 0.5 for r in V), sum(float(r['core_iou']) > 0.5 for r in V)))
# echecs enregistres de HDBSCAN (valeur Zoltan <= 1/2 a K = 5 ou 10) : par instance (scene, group)
inst = collections.defaultdict(dict)
for r in R:
    if int(r['m']) >= 50:
        inst[(r['role'], r['scene'], r['group'])][r['k']] = r
for role in ('echec', 'demo'):
    fails = {k: v for k, v in inst.items() if k[0] == role and any(x['zoltan_hdbscan'] not in ('',) and float(x['zoltan_hdbscan']) <= 0.5 for x in v.values())}
    print('role %s : instances en echec enregistre (K = 5 ou 10) : %d' % (role, len(fails)))
    for s in srcs:
        ok = sum(any(float(x[s + '_iou']) > 0.5 for x in v.values()) for v in fails.values())
        same = sum(1 for v in fails.values() for x in v.values() if x['zoltan_hdbscan'] != '' and float(x['zoltan_hdbscan']) <= 0.5 and float(x[s + '_iou']) > 0.5)
        tot = sum(1 for v in fails.values() for x in v.values() if x['zoltan_hdbscan'] != '' and float(x['zoltan_hdbscan']) <= 0.5)
        print('   %-8s > 1/2 a K = 5 ou 10 : %d ; au meme ordre que l echec : %d sur %d' % ({'a': 'tour'}.get(s, s), ok, same, tot))
