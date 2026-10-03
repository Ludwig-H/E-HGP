#!/usr/bin/env python3
"""Lecture des campagnes points (bench/points_campaign.py) : synthetiques et LiDAR, sans nouveau calcul.

    python3 bench/points_summary.py --synthetic DIR --lidar DIR [--lidar DIR ...] [--manifest MANIFEST.json ...]

Synthetique : par ordre et par regle, IoU moyen du meilleur bloc, parts > 1/2 et > 4/5, difference appariee avec
HDBSCAN et intervalle bootstrap a 95 % par scene (graine fixe). LiDAR : instances d'au moins 50 points ; un
« sauvetage » est un objet dont HDBSCAN reste a 1/2 ou moins au meme ordre et que la regle depasse strictement ;
un sauvetage « fort » est un objet ou HDBSCAN reste a 1/2 ou moins a TOUS les ordres testes.
"""
import argparse
import glob
import json
import os
import random

RULES = ('core', 'cover', 'first', 'margin1', 'margin', 'margin_r')


def load(directory):
    out = []
    for path in sorted(glob.glob(os.path.join(directory, '*.json'))):
        data = json.load(open(path))
        if data.get('status') == 'ok':
            out.append(data)
    return out


def bootstrap(diffs, seed=20261003, draws=2000):
    if not diffs:
        return (0.0, 0.0)
    rng = random.Random(seed)
    means = sorted(sum(rng.choice(diffs) for _ in diffs) / len(diffs) for _ in range(draws))
    return means[int(0.025 * draws)], means[int(0.975 * draws)]


def synthetic(scenes):
    rows = []
    orders = sorted({k for s in scenes for k in s['orders']}, key=int)
    for k in orders:
        base = [s['orders'][k]['hdbscan']['best'] for s in scenes if k in s['orders']]
        flat = [x for b in base for x in b]
        rows.append((k, 'hdbscan', sum(flat) / len(flat), sum(x > 0.5 for x in flat) / len(flat),
                     sum(x > 0.8 for x in flat) / len(flat), None, None))
        for rule in RULES:
            vals, diffs = [], []
            for s in scenes:
                row = s['orders'].get(k, {})
                if rule not in row:
                    continue
                v, h = row[rule]['best'], row['hdbscan']['best']
                vals.extend(v)
                diffs.append(sum(a - b for a, b in zip(v, h)) / len(v))
            if vals:
                lo, hi = bootstrap(diffs)
                rows.append((k, rule, sum(vals) / len(vals), sum(x > 0.5 for x in vals) / len(vals),
                             sum(x > 0.8 for x in vals) / len(vals), sum(diffs) / len(diffs), (lo, hi)))
    return rows


def families(scenes, k, rule):
    out = {}
    for s in scenes:
        fam = s['meta']['spec']['family']
        row = s['orders'].get(k, {})
        if rule in row:
            out.setdefault(fam, [[], []])
            out[fam][0].extend(row[rule]['best'])
            out[fam][1].extend(row['hdbscan']['best'])
    return {f: (sum(a) / len(a), sum(b) / len(b)) for f, (a, b) in sorted(out.items())}


def lidar(scenes, roles):
    objects = []
    for s in scenes:
        keys = s['meta'].get('objects', [])
        for o, key in enumerate(keys):
            entry = dict(scene=s['name'], role=roles.get(s['name']), object=o, label=key, sem=key & 0xFFFF,
                         inst=key >> 16, orders={})
            for k, row in s['orders'].items():
                entry['orders'][k] = {m: row[m]['best'][o] for m in ('hdbscan',) + RULES if m in row}
            objects.append(entry)
    return objects


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--synthetic', action='append', default=[])
    parser.add_argument('--lidar', action='append', default=[])
    parser.add_argument('--manifest', action='append', default=[])
    parser.add_argument('--json')
    args = parser.parse_args()
    report = {}
    if args.synthetic:
        scenes = [s for d in args.synthetic for s in load(d)]
        rows = synthetic(scenes)
        print('== synthetique : %d scenes' % len(scenes))
        print('%-3s %-8s %7s %6s %6s %9s %s' % ('k', 'regle', 'IoU', '>1/2', '>4/5', 'vs HDB', 'IC95'))
        for k, rule, mean, half, high, diff, ci in rows:
            print('%-3s %-8s %7.4f %6.3f %6.3f %9s %s' % (
                k, rule, mean, half, high, '' if diff is None else '%+.4f' % diff,
                '' if ci is None else '[%+.4f ; %+.4f]' % ci))
        for k in sorted({r[0] for r in rows}, key=int):
            print('  familles k=%s margin contre HDBSCAN :' % k,
                  {f: '%.3f/%.3f' % v for f, v in families(scenes, k, 'margin').items()})
        report['synthetic'] = [dict(k=r[0], rule=r[1], mean=r[2], over_half=r[3], over_08=r[4], diff=r[5],
                                    ci=r[6]) for r in rows]
    if args.lidar:
        roles = {}
        for path in args.manifest:
            for entry in json.load(open(path))['scenes']:
                roles[entry['name']] = entry.get('role')
        scenes = [s for d in args.lidar for s in load(d)]
        objects = lidar(scenes, roles)
        print('== LiDAR : %d scenes, %d instances >= 50 points' % (len(scenes), len(objects)))
        orders = sorted({k for o in objects for k in o['orders']}, key=int)
        for k in orders:
            line = []
            for m in ('hdbscan',) + RULES:
                vals = [o['orders'][k][m] for o in objects if k in o['orders'] and m in o['orders'][k]]
                if vals:
                    line.append('%s %.4f (>1/2 %d)' % (m, sum(vals) / len(vals), sum(v > 0.5 for v in vals)))
            print('  k=%s : %s' % (k, ' ; '.join(line)))
        rescues, strong, losses = [], [], []
        for o in objects:
            fails = [k for k in o['orders'] if o['orders'][k]['hdbscan'] <= 0.5]
            for k in fails:
                for m in ('margin', 'margin1', 'cover', 'first', 'core'):
                    v = o['orders'][k].get(m)
                    if v is not None and v > 0.5:
                        rescues.append(dict(o, k=k, rule=m, value=v, hdbscan=o['orders'][k]['hdbscan']))
            if fails and len(fails) == len(o['orders']):
                best = max(((o['orders'][k].get('margin', 0), k) for k in o['orders']))
                if best[0] > 0.5:
                    strong.append(dict(o, k=best[1], value=best[0]))
            for k in o['orders']:
                v = o['orders'][k].get('margin')
                if v is not None and o['orders'][k]['hdbscan'] > 0.5 >= v:
                    losses.append(dict(o, k=k, value=v, hdbscan=o['orders'][k]['hdbscan']))
        def show(title, items, key):
            print('  %s : %d' % (title, len(items)))
            for r in sorted(items, key=key)[:60]:
                print('    %-34s %-7s obj %2d sem %3d inst %4d k=%-2s %s %.3f HDB %s' % (
                    r['scene'], r['role'] or '', r['object'], r['sem'], r['inst'], r['k'], r.get('rule', 'margin'),
                    r['value'], '%.3f' % r['hdbscan'] if 'hdbscan' in r else ''))
        show('sauvetages a ordre egal (toute regle)', rescues, lambda r: (r['scene'], r['object'], int(r['k'])))
        show('sauvetages de margin a ordre egal', [r for r in rescues if r['rule'] == 'margin'],
             lambda r: (r['scene'], r['object'], int(r['k'])))
        show('sauvetages forts (HDBSCAN <= 1/2 a tous les ordres, margin > 1/2)', strong,
             lambda r: (r['scene'], r['object']))
        show('pertes de margin (HDBSCAN > 1/2 >= margin)', losses, lambda r: (r['scene'], r['object'], int(r['k'])))
        report['lidar'] = dict(objects=objects, rescues=rescues, strong=strong, losses=losses)
    if args.json:
        json.dump(report, open(args.json, 'w'), indent=1, sort_keys=True)


if __name__ == '__main__':
    raise SystemExit(main())
