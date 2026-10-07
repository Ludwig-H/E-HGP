"""Recalcul en memoire (aucune ecriture) des tableaux de HIERARCHIE_POINTS.md par 7 depuis le recu points_g4.

Reprend la logique de bench/points_summary.py (cohortes, sauvetages/pertes) et de check.py (synthetique par taille),
sans importer le depot. Lecture seule.
"""
import json
import random
import sys
import tarfile
from pathlib import Path

REC = Path('/workspaces/E-HGP/morsehgp3D_v11/receipts/developpement_20261003/points_g4')
PRIORITY = {'demo': 0, 'echec': 1, 'voisin': 2, 'temoin': 3}


def scenes_of(session):
    out = []
    with tarfile.open(REC / 'sessions' / session / 'results.tar.gz') as t:
        for m in t.getmembers():
            parts = m.name.split('/')
            if m.isfile() and len(parts) >= 6 and parts[-2] in ('synthetic', 'lidar') and m.name.endswith('.json'):
                d = json.loads(t.extractfile(m).read())
                if d.get('status') == 'ok':
                    out.append((parts[-2], d))
    return out


def boot(diffs, seed=20261003, draws=2000):
    rng = random.Random(seed)
    means = sorted(sum(rng.choice(diffs) for _ in diffs) / len(diffs) for _ in range(draws))
    return means[int(0.025 * draws)], means[int(0.975 * draws)]


def synth_by_size(scenes, rules=('margin_r', 'first', 'cover', 'core', 'margin')):
    rows = {}
    for d in scenes:
        n = d['meta']['spec']['n']
        for k, o in d['orders'].items():
            if 'hdbscan' not in o:
                continue
            h = sum(o['hdbscan']['best']) / len(o['hdbscan']['best'])
            for rule in rules:
                if rule in o:
                    r = sum(o[rule]['best']) / len(o[rule]['best'])
                    rows.setdefault((n, int(k), rule), []).append(r - h)
    for key in sorted(rows):
        diffs = rows[key]
        lo, hi = boot(diffs)
        print('  n=%-5d k=%-2d %-9s scenes %3d ecart %+.4f [%+.4f ; %+.4f] >=HDB %d/%d' % (
            key[0], key[1], key[2], len(diffs), sum(diffs) / len(diffs), lo, hi, sum(x >= 0 for x in diffs), len(diffs)))


def cohorts(scenes, roles, rule='margin_r'):
    objects = []
    for d in scenes:
        for o, key in enumerate(d['meta'].get('objects', [])):
            e = dict(scene=d['name'], role=roles.get(d['name']), sha=d['meta'].get('sites_sha256'), orders={})
            for k, row in d['orders'].items():
                e['orders'][k] = {m: row[m]['best'][o] for m in ('hdbscan', rule) if m in row}
            objects.append(e)
    keep = {}
    for o in objects:
        b = keep.get(o['sha'])
        if b is None or PRIORITY.get(o['role'], 9) < PRIORITY.get(b[1], 9):
            keep[o['sha']] = (o['scene'], o['role'])
    kept = set(v[0] for v in keep.values())
    for role in ('demo', 'echec', 'voisin', 'temoin'):
        g = [o for o in objects if o['role'] == role and o['scene'] in kept]
        if not g:
            continue
        line = []
        for k in sorted({k for o in g for k in o['orders']}, key=int):
            pairs = [(o['orders'][k]['hdbscan'], o['orders'][k][rule]) for o in g
                     if k in o['orders'] and rule in o['orders'][k] and 'hdbscan' in o['orders'][k]]
            if not pairs:
                continue
            mh = sum(p[0] for p in pairs) / len(pairs)
            mr = sum(p[1] for p in pairs) / len(pairs)
            sv = sum(1 for h, v in pairs if h <= 0.5 < v)
            ps = sum(1 for h, v in pairs if v <= 0.5 < h)
            line.append('k=%s %.3f/%.3f +%d/-%d' % (k, mr, mh, sv, ps))
        print('  %-7s scenes %3d instances %4d  %s' % (role, len(set(o['scene'] for o in g)), len(g), ' | '.join(line)))


def main():
    roles = {}
    for p in sorted(REC.glob('data_manifest_*.json')):
        for e in json.load(open(p))['scenes']:
            roles[e['name']] = e.get('role')
    for session in sys.argv[1:]:
        sc = scenes_of(session)
        syn = [d for kind, d in sc if kind == 'synthetic']
        lid = [d for kind, d in sc if kind == 'lidar']
        print('==', session, 'synthetiques', len(syn), 'lidar', len(lid))
        if syn:
            synth_by_size(syn)
        if lid:
            cohorts(lid, roles)


if __name__ == '__main__':
    main()
