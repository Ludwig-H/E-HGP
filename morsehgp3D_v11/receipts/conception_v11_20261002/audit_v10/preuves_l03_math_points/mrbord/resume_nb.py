import json, sys, collections, random
rows = []
for p in sys.argv[1:]:
    rows += json.load(open(p))['rows']
S = collections.defaultdict(dict)
for r in rows:
    S[(r['family'], r['level'], r['noise'], r['rep'], r['K'])][r['source']] = r
srcs = ['tour_cover', 'tour_core', 'mr1_coeur', 'mr1_bord', 'mr2_coeur', 'mr2_bord']
for K in sorted({k[4] for k in S}):
    keys = [k for k in S if k[4] == K]
    print('K = %d : %d scenes, %d groupes par source' % (K, len(keys), 8 * len(keys)))
    for s in srcs:
        v = [x for k in keys for x in S[k][s]['iou']]
        print('   %-10s IoU moyen du meilleur bloc %.4f ; > 4/5 : %.3f ; exacts %d' % (s, sum(v) / len(v), sum(x > 0.8 for x in v) / len(v), sum(S[k][s]['exact'] for k in keys)))
    rnd = random.Random(7)
    for a, b in (('tour_cover', 'mr1_coeur'), ('tour_cover', 'mr1_bord'), ('tour_cover', 'mr2_bord'), ('mr1_bord', 'mr1_coeur'), ('tour_core', 'mr1_coeur')):
        d = [sum(S[k][a]['iou']) / 8 - sum(S[k][b]['iou']) / 8 for k in keys]
        m = sum(d) / len(d)
        bs = sorted(sum(d[rnd.randrange(len(d))] for _ in d) / len(d) for _ in range(4000))
        print('   ecart apparie par scene %-10s - %-10s : %+0.4f  IC95 [%+0.4f ; %+0.4f]  scenes +/=/- %d/%d/%d' % (
            a, b, m, bs[100], bs[3899], sum(x > 1e-12 for x in d), sum(abs(x) <= 1e-12 for x in d), sum(x < -1e-12 for x in d)))
    # sans bruit / avec bruit
    for nu in sorted({k[2] for k in keys}):
        kk = [k for k in keys if k[2] == nu]
        print('   bruit %.2f :' % nu, ' ; '.join('%s %.4f' % (s, sum(x for k in kk for x in S[k][s]['iou']) / (8 * len(kk))) for s in srcs))
    fam = collections.defaultdict(list)
    for k in keys:
        fam[k[0]].append(k)
    for f, kk in fam.items():
        print('   %-16s' % f, ' ; '.join('%s %.3f' % (s.replace('tour_', 't_'), sum(x for k in kk for x in S[k][s]['iou']) / (8 * len(kk))) for s in srcs))
