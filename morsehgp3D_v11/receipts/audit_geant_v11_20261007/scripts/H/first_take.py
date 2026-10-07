import json, glob, os, statistics
R = '/workspaces/E-HGP/morsehgp3D_v11/receipts/'
rows = []
for p in sorted(glob.glob(R + '**/gpu_ab_report*.json', recursive=True)):
    r = json.load(open(p))
    cold = r.get('cold', [])
    if not cold:
        continue
    first = cold[0]
    mode_mask = r['modes'].get(first['mode'])
    is_gpu = any(int(str(m).split(':')[0].split('@')[0]) & 65536 for m in r['modes'].values())
    # autres prises de la meme cellule (meme trame, meme mode)
    same = [c for c in cold[1:] if c['frame'] == first['frame'] and c['mode'] == first['mode'] and c['code'] == 0]
    other_modes = [c for c in cold[1:] if c['frame'] == first['frame'] and c['code'] == 0]
    s = first['summary']
    di = (s.get('batch') or {}).get('device_init_ns')
    med_same = statistics.median([c['summary']['wall_ms'] for c in same]) if same else None
    walls_same = sorted(round(c['summary']['wall_ms'], 1) for c in same)
    rows.append((os.path.relpath(p, R), first['frame'], first['mode'], mode_mask, round(s['wall_ms'], 1), round(s['domain_ms'], 1),
                 None if di is None else round(di / 1e6, 1), med_same, walls_same[:3] + ['...'] + walls_same[-2:] if walls_same else []))
for row in rows:
    print('%-88s %s %-12s %-12s first_wall=%s dom=%s devinit=%s | med_autres=%s %s' % row)
