import json, glob, os, sys
root = sys.argv[1]
for rec in ['g4_tower_r5_20260923','g4_tower_r6_20260923','g4_tower_r7b_20260923']:
    print('==', rec)
    files = sorted(glob.glob(os.path.join(root, rec, 'vm', 'probe_*.stdout')), key=lambda p:int(p.split('_')[-1].split('.')[0]))
    for f in files:
        txt = open(f).read().strip()
        if not txt:
            print(os.path.basename(f), 'EMPTY'); continue
        try:
            d = json.loads(txt.splitlines()[-1])
        except Exception as e:
            print(os.path.basename(f), 'PARSE', e); continue
        t = d.get('times_ms', {})
        g = d.get('generator', {})
        o = d.get('options', {})
        print(os.path.basename(f), d['input']['sites'], 'K', o.get('K'), 'W', o.get('workers'),
              'q2_ms', t.get('q2'), 'q34_ms', t.get('q34'), 'chain', t.get('chain_total'),
              'rect', g.get('q2_front_rectangles'), 'cand', g.get('q2_candidate_pairs'), 'acc', g.get('q2_accepted_pairs'),
              'levers_core', o.get('levers',{}).get('q34_dead_core'), 'meb', o.get('levers',{}).get('tower_meb_proposal'))
