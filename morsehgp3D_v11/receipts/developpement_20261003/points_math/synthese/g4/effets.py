"""Lecture seule des archives G4 claudepts1/2 : effet de la qualification (Pi_{k+1} contre Pi_1) et de la marge
(niveau carre), apparie par scene ; LiDAR : sauvetages/pertes par regle et par cohorte (scenes dedoublonnees)."""
import json, glob, random
SRC = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/receipts/developpement_20261003/points_g4/'
def load(d):
    return [x for x in (json.load(open(p)) for p in sorted(glob.glob(d + '/*.json'))) if x.get('status') == 'ok']
def boot(diffs, seed=20261003, draws=2000):
    rng = random.Random(seed)
    m = sorted(sum(rng.choice(diffs) for _ in diffs) / len(diffs) for _ in range(draws))
    return m[int(0.025 * draws)], m[int(0.975 * draws)]
syn = load('s1/results/cmd/001_synthetic/files/synthetic')
pairs = [('first', 'cover', 'qualification sans marge'), ('margin', 'margin1', 'qualification avec marge carree'),
         ('margin1', 'cover', 'marge carree, Pi_1'), ('margin', 'first', 'marge carree, Pi_{k+1}')]
print('== synthetique, %d scenes : difference appariee moyenne par scene [IC95 bootstrap]' % len(syn))
for k in ('2', '3', '5', '10'):
    for a, b, lab in pairs:
        diffs = []
        for s in syn:
            row = s['orders'].get(k, {})
            if a in row and b in row:
                va, vb = row[a]['best'], row[b]['best']
                diffs.append(sum(x - y for x, y in zip(va, vb)) / len(va))
        if diffs:
            lo, hi = boot(diffs)
            print('  k=%-2s %-8s - %-8s %+.4f [%+.4f ; %+.4f]  (%s, %d scenes)' % (k, a, b, sum(diffs) / len(diffs), lo, hi, lab, len(diffs)))
roles = {}
for f in ('data_manifest_pts1.json', 'data_manifest_pts2_roles.json'):
    for e in json.load(open(SRC + f))['scenes']:
        roles[e['name']] = e.get('role')
lid = load('s1/results/cmd/002_lidar/files/lidar') + load('s2/results/cmd/002_lidar/files/lidar')
prio = {'demo': 0, 'echec': 1, 'voisin': 2, 'temoin': 3}
keep = {}
for s in lid:
    sha = s['meta'].get('sites_sha256'); r = roles.get(s['name'])
    if sha not in keep or prio.get(r, 9) < prio.get(keep[sha][1], 9):
        keep[sha] = (s['name'], r)
kept = {v[0] for v in keep.values()}
rules = ('core', 'cover', 'first', 'margin1', 'margin')
print('== LiDAR : %d scenes lues, %d apres dedoublonnage' % (len(lid), len(kept)))
for role in ('demo', 'echec', 'voisin', 'temoin'):
    group = [s for s in lid if s['name'] in kept and roles.get(s['name']) == role]
    n_inst = sum(len(s['meta']['objects']) for s in group)
    print('  %s : %d scenes, %d instances' % (role, len(group), n_inst))
    for k in ('2', '3', '5', '10'):
        out = []
        for r in rules:
            sv = pe = 0; tot = 0; vals = []
            for s in group:
                row = s['orders'].get(k)
                if not row or r not in row:
                    continue
                for h, v in zip(row['hdbscan']['best'], row[r]['best']):
                    tot += 1; vals.append(v)
                    sv += (h <= 0.5 < v); pe += (v <= 0.5 < h)
            if tot:
                out.append('%s %.4f +%d/-%d' % (r, sum(vals) / tot, sv, pe))
        hd = [h for s in group if k in s['orders'] for h in s['orders'][k]['hdbscan']['best']]
        print('    k=%-2s hdbscan %.4f | ' % (k, sum(hd) / len(hd)) + ' | '.join(out))
