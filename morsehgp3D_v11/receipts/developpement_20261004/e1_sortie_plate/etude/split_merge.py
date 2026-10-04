import sys, json
from pathlib import Path
import concurrent.futures as cf
sys.path.insert(0, '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench')
import numpy as np
import points_flat as pf, points_hierarchy as ph, points_flat_study as fs
BASE = Path('/workspaces/E-HGP/build/v11-persist')
LOTS = [(BASE / 'e1_s0/results/results/cmd/003_dump2/files/lot2', BASE / 'bouts_lot2/data', 'lot2'),
        (BASE / 'e1_s0/results/results/cmd/004_dump1/files/lot1', BASE / 'bouts_lot1/data', 'lot1')]
RULES = [('T', 'eom1'), ('T', 'eom2'), ('T', 'eom3'), ('T', 'leaf'), ('R0', 'eom')]

def classify(lab, obj, void, nobj):
    keep = ~void
    lab, ob = lab[keep], obj[keep]
    out = []
    clusters = np.unique(lab[lab >= 0])
    # objet majoritaire de chaque cluster
    major = {}
    for c in clusters.tolist():
        m = ob[lab == c]
        m = m[m >= 0]
        major[c] = int(np.bincount(m).argmax()) if len(m) else -1
    csize = {c: int(np.sum(lab == c)) for c in clusters.tolist()}
    for o in range(nobj):
        pts = lab[ob == o]
        g = len(pts)
        cs, counts = np.unique(pts[pts >= 0], return_counts=True)
        best_iou = max((counts[i] / (g + csize[c] - counts[i]) for i, c in enumerate(cs.tolist())), default=0.0)
        intact = any(3 * counts[i] > g + csize[c] for i, c in enumerate(cs.tolist()))
        merged = any(major[c] != o and counts[i] * 2 > g for i, c in enumerate(cs.tolist())) or \
            any(major[c] == o and any(major.get(c) == o and (lab == c).sum() and False for _ in [0]) for c in cs.tolist())
        # fusion : un cluster contient plus de la moitie de cet objet ET plus de la moitie d'un autre objet
        merged = False
        for i, c in enumerate(cs.tolist()):
            inside = ob[lab == c]
            others = np.bincount(inside[(inside >= 0) & (inside != o)], minlength=nobj) if np.any((inside >= 0) & (inside != o)) else np.zeros(nobj, int)
            gs = np.bincount(ob[ob >= 0], minlength=nobj)
            if 2 * counts[i] > g and np.any(2 * others > gs):
                merged = True
        pure = int(sum(counts[i] for i, c in enumerate(cs.tolist()) if major[c] == o))
        pieces = int(sum(1 for i, c in enumerate(cs.tolist()) if major[c] == o and counts[i] * 10 >= g))
        state = 'intact' if intact and not merged else ('fusionne' if merged else
                 ('decoupe' if 2 * pure >= g else ('perdu_bruit' if 2 * int(np.sum(pts < 0)) > g else 'absorbe')))
        out.append(dict(state=state, pieces=pieces, iou=round(float(best_iou), 3)))
    return out

def job(args):
    dumps, scenes, lot, name, k = args
    raw = np.fromfile(scenes / (name + '_labels.u32le'), dtype='<u4')
    obj, void, keys = ph.lidar_objects(raw, 50)
    nobj = len(keys)
    if not nobj: return None
    pt = pf.PointTree.load(dumps / ('%s_k%d_tower.npz' % (name, k)))
    lb = fs.best_blocks(pt, obj[pt.ids], void[pt.ids], nobj)
    with np.load(dumps / ('%s_k%d_sklearn.npz' % (name, k))) as z:
        r0 = z['labels_mcs20'].astype(np.int64)
    res = dict(lot=lot, name=name, k=k, hier=[x > 0.5 for x in lb], rules={})
    cond, first = pf.condense(pt, 20)
    for side, rule in RULES:
        if side == 'R0':
            lab = r0
        else:
            method, z = fs.RULES[rule]
            try:
                sel = pf.select(pt, cond, z, method)
            except pf.Refusal:
                continue
            lab = pf.labels(pt, cond, first, sel)
        res['rules'][side + '_' + rule] = classify(lab, obj, void, nobj)
    return res

jobs = []
for dumps, scenes, lot in LOTS:
    for meta in sorted(dumps.glob('*.json')):
        info = json.loads(meta.read_text())
        if lot == 'lot1' and (BASE / 'bouts_lot2/data' / (info['name'] + '_sites.u32le')).exists():
            continue  # deja dans le lot 2 (niveaux exacts)
        for k in (2, 3, 5, 10):
            jobs.append((dumps, scenes, lot, info['name'], k))
with cf.ProcessPoolExecutor(7) as pool:
    out = [r for r in pool.map(job, jobs, chunksize=2) if r]
json.dump(out, open(BASE / 'e1_s0/split_merge.json', 'w'))
for subset, f in (('exemples organises (bouts)', lambda r: r['lot'] == 'lot2' and not r['name'].startswith('zoltan')),
                  ('demos (scenes entieres)', lambda r: r['name'].startswith('zoltan')),
                  ('criblage (lot 1)', lambda r: r['lot'] == 'lot1')):
    rs = [r for r in out if f(r)]
    print('==', subset, '; objets trouves par la hierarchie (niveau B > 1/2), mcs 20')
    for key in ('T_eom1', 'T_eom2', 'T_eom3', 'T_leaf', 'R0_eom'):
        objs = [r['rules'][key][o] for r in rs if key in r['rules'] for o in range(len(r['hier'])) if r['hier'][o]]
        n = len(objs)
        cnt = {s: sum(1 for x in objs if x['state'] == s) for s in ('intact', 'fusionne', 'decoupe', 'absorbe', 'perdu_bruit')}
        pieces = [x['pieces'] for x in objs if x['state'] == 'decoupe']
        print('  %-7s n=%4d intact %.3f | fusionne %.3f | decoupe %.3f (morceaux med %s, >=4 : %.3f) | absorbe %.3f | bruit %.3f' % (
            key, n, cnt['intact'] / n, cnt['fusionne'] / n, cnt['decoupe'] / n,
            int(np.median(pieces)) if pieces else '-', (sum(1 for p in pieces if p >= 4) / n), cnt['absorbe'] / n, cnt['perdu_bruit'] / n))
