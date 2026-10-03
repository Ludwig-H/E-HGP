import json, glob, os, sys
rules = ('hdbscan','core','cover','first','margin1','margin')
files = sorted(glob.glob('s1/results/cmd/002_lidar/files/lidar/zoltan_*.json'))
for f in files:
    d = json.load(open(f))
    name = d['name']; objs = d['meta']['objects']
    print('==', name, 'sites', d.get('sites'), 'objets', len(objs))
    for o, key in enumerate(objs):
        sem, inst = key & 0xFFFF, key >> 16
        hd = {k: d['orders'][k]['hdbscan']['best'][o] for k in d['orders']}
        if min(hd.values()) > 0.5:
            continue
        print('  obj %2d sem %3d inst %3d' % (o, sem, inst))
        for k in sorted(d['orders'], key=int):
            row = d['orders'][k]
            print('     k=%-2s ' % k + ' '.join('%s=%.3f' % (r, row[r]['best'][o]) for r in rules if r in row))
