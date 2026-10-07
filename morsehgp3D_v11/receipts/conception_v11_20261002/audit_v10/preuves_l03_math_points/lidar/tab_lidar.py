import json, os, glob
D = '/tmp/v11-audit/l03_math_points/lidar/'
demos = ['01_velos_en_rang', '02_velos_contre_facade', '03_pieton_contre_facade', '04_velos_en_rang_avec_sol', '05_temoin_voitures_en_file']
print('demo | K | plafond def. 8 (A/B/C) | cover (A/B/C) | core (A/B/C) | boules')
for d in demos:
    for K in (2, 3, 4, 5):
        p = D + ('01_K5.json' if (d.startswith('01') and K == 5) else '%s_K%d.json' % (d, K))
        if not os.path.exists(p):
            continue
        j = json.load(open(p))
        r = j['results']
        f = lambda key: ' / '.join('%.3f' % o['iou'] for o in r[key])
        print('%s | %d | %s | %s | %s | %d' % (d[:2], K, f('ceiling_def8'), f('laminar_cover'), f('laminar_core'), j['balls']))
