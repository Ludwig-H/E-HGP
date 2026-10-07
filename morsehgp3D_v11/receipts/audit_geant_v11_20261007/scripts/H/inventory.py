import json, sys, glob, os
root = sys.argv[1]
paths = sorted(glob.glob(root + '/**/gpu_ab_report*.json', recursive=True))
for p in paths:
    try:
        r = json.load(open(p))
    except Exception as e:
        print('ERR', p, e); continue
    rel = os.path.relpath(p, root)
    modes = r.get('modes')
    cold = r.get('cold', [])
    warm = r.get('warm', [])
    ncold = len(cold)
    codes = sorted(set(c.get('code') for c in cold))
    print('%-95s reps=%s kmax=%s leaf=%s modes=%s ncold=%d nwarm=%d verdict=%s codes=%s variants=%s' % (
        rel, r.get('reps'), r.get('kmax'), r.get('leaf'), json.dumps(modes, separators=(',',':')), ncold, len(warm), r.get('verdict'), codes, r.get('variants')))
