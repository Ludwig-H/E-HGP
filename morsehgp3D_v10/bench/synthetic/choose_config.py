"""Choix d'une configuration UNIQUE par methode sur les graines dev (jamais par scene), et resume.

  python3 choose_config.py DEV.csv [--top 5]
Score d'une configuration = moyenne des ARI_s sur les scenes-graines ou elle a tourne ; seules les
configurations presentes sur TOUTES les scenes-graines sont eligibles (aucun survivant silencieux).
Rend aussi l'oracle par scene de chaque methode (diagnostic, jamais un adversaire de decision).
"""
import argparse
import csv
import json
from collections import defaultdict

KEY = ('method', 'k', 'mcs', 'z', 'selection', 'alpha', 'fill')


def load(path):
    rows = list(csv.DictReader(open(path)))
    for r in rows:
        for m in ('ari_s', 'ari_nc', 'ami_nc', 'coverage'):
            r[m] = float(r[m])
    return rows


def configs(rows):
    by = defaultdict(dict)
    for r in rows:
        by[tuple(r[k] for k in KEY)][(r['scene'], r['seed'])] = r
    return by


def best(by, method, units, metric='ari_s'):
    cands = []
    for cfg, cells in by.items():
        if cfg[0] != method or len(cells) != len(units):
            continue
        cands.append((sum(cells[u][metric] for u in units) / len(units), cfg))
    cands.sort(reverse=True)
    return cands


def oracle(rows, method, metric='ari_s'):
    per = defaultdict(float)
    for r in rows:
        if r['method'] == method:
            u = (r['scene'], r['seed'])
            per[u] = max(per[u], r[metric])
    return sum(per.values()) / max(1, len(per))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('csv')
    ap.add_argument('--top', type=int, default=5)
    ap.add_argument('--json')
    args = ap.parse_args()
    rows = load(args.csv)
    units = sorted({(r['scene'], r['seed']) for r in rows})
    by = configs(rows)
    out = dict(units=len(units))
    for method in ('tower', 'hdbscan', 'hdbscan_default'):
        c = best(by, method, units)
        out[method] = [dict(score=round(s, 4), cfg=dict(zip(KEY, cfg))) for s, cfg in c[:args.top]]
        out[method + '_oracle'] = round(oracle(rows, method), 4)
        print('%s  (oracle par scene %.4f)' % (method, out[method + '_oracle']))
        for s, cfg in c[:args.top]:
            print('   %.4f  %s' % (s, dict(zip(KEY[1:], cfg[1:]))))
    if args.json:
        json.dump(out, open(args.json, 'w'), indent=1)


if __name__ == '__main__':
    main()
