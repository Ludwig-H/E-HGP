import sys, json, glob
from pathlib import Path
sys.path.insert(0, '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench')
import numpy as np
import points_flat as pf, points_hierarchy as ph, points_flat_study as fs
R = Path('/workspaces/E-HGP/build/v11-persist/e1_s0/results/results/cmd/003_dump2/files/lot2')
S = Path('/workspaces/E-HGP/build/v11-persist/bouts_lot2/data')

def members_counts(cond, first, ids, obj, void, nobj):
    nc = len(cond)
    size = np.zeros(nc, dtype=np.int64); cnt = np.zeros((nc, nobj), dtype=np.int64)
    for s, c in enumerate(first.tolist()):
        if c >= 0:
            p = ids[s]
            if void[p]: continue
            size[c] += 1
            if obj[p] >= 0: cnt[c, obj[p]] += 1
    for c in range(nc):
        for d in cond.children[c]:
            size[c] += size[d]; cnt[c] += cnt[d]
    return size, cnt

def oracle_count(cond, size, cnt, gsize):
    nc = len(cond); best = [0] * nc
    for c in range(nc):
        own = 0
        if cnt.shape[1]:
            g = int(np.argmax(cnt[c])); inter = int(cnt[c, g])
            own = 1 if 3 * inter > size[c] + gsize[g] else 0
        below = sum(best[d] for d in cond.children[c])
        best[c] = max(own, below) if cond.parent[c] >= 0 else below
    return sum(best[c] for c in range(nc) if cond.parent[c] < 0)

out = []
for meta in sorted(R.glob('*.json')):
    info = json.loads(meta.read_text()); name = info['name']
    raw = np.fromfile(S / (name + '_labels.u32le'), dtype='<u4')
    obj, void, keys = ph.lidar_objects(raw, 50)
    nobj = len(keys)
    if not nobj: continue
    gsize = np.bincount(obj[(obj >= 0) & ~void], minlength=nobj)
    for k in (2, 3, 5, 10):
        pt = pf.PointTree.load(R / ('%s_k%d_tower.npz' % (name, k)))
        lb = fs.best_blocks(pt, obj[pt.ids], void[pt.ids], nobj)
        for mcs in (10, 20):
            cond, first = pf.condense(pt, mcs)
            size, cnt = members_counts(cond, first, pt.ids, obj, void, nobj)
            orc = oracle_count(cond, size, cnt, gsize)
            row = dict(name=name, k=k, mcs=mcs, objects=nobj, hier=int(sum(x > 0.5 for x in lb)), oracle=orc)
            for rule in ('eom1', 'eom3', 'leaf'):
                method, z = fs.RULES[rule]
                sel = pf.select(pt, cond, z, method)
                lab = pf.labels(pt, cond, first, sel)
                rows, _ = fs.object_rows(lab, obj, void, nobj)
                row[rule] = sum(r['found'] for r in rows)
            out.append(row)
json.dump(out, open('/workspaces/E-HGP/build/v11-persist/e1_s0/oracle_lot2.json', 'w'))
for subset, f in (('bouts', lambda r: not r['name'].startswith('zoltan')), ('demos', lambda r: r['name'].startswith('zoltan'))):
    for mcs in (10, 20):
        rs = [r for r in out if f(r) and r['mcs'] == mcs]
        tot = sum(r['objects'] for r in rs)
        print('%s mcs%d objets %d | hierarchie %d | oracle antichaine %d | eom1 %d | eom3 %d | feuilles %d' % (
            subset, mcs, tot, sum(r['hier'] for r in rs), sum(r['oracle'] for r in rs), sum(r['eom1'] for r in rs),
            sum(r['eom3'] for r in rs), sum(r['leaf'] for r in rs)))
