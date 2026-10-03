#!/usr/bin/env python3
"""Variante exterieure fidele (fermeture exclusive EC) contre H_m, sur nuages aleatoires bornes.

    python3 ec_vs_hm.py --clouds 600 --seed 23 --out resultats/ec_vs_hm.json

Pour chaque (nuage, k in {2, 3}, m in {1, k+1}) et chaque site : t = premiere couverture qualifiee (= entree de la
fermeture), entrees de EC, de H_m (margin, ou margin1 si m = 1) et de first (LCA des ex aequo). On compte les sites
retardes (e > t), la comparaison EC contre H_m, et le retard moyen en rayon sqrt(e / t) (sites a t > 0).
Pour two_blobs : fraction de A recuperee avant la reunion avec B, EC contre H_m contre fermeture.
"""
import argparse
import json
import math
import random
import statistics
import sys
import time

sys.dont_write_bytecode = True
import lib_fermeture as lf  # noqa: E402


def seed_of(defn, k, group):
    return min(group, key=lambda i: (defn.nearest(i)[k - 1][0], i))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--clouds', type=int, default=600)
    parser.add_argument('--seed', type=int, default=23)
    parser.add_argument('--seconds', type=float, default=200.0)
    parser.add_argument('--out', required=True)
    args = parser.parse_args()
    rng = random.Random(args.seed)
    rows = {}
    started = time.monotonic()
    count = 0
    for c in range(args.clouds):
        if time.monotonic() - started > args.seconds:
            break
        gen = ('gate', 'generic', 'two_blobs')[c % 3]
        groups = None
        if gen == 'gate':
            pts = lf.cloud_gate(rng)
        elif gen == 'generic':
            pts = lf.cloud_generic(rng)
        else:
            pts, A, B, _V = lf.cloud_two_blobs(rng)
            groups = (A, B)
        n = len(pts)
        defn = lf.Definition(pts)
        count += 1
        for k in (2, 3):
            if k >= n:
                continue
            res = defn.order(k)
            for m in (1, k + 1):
                if m > n:
                    continue
                ref, tree = lf.faithful_rules(res, n, m)
                hm = ref['margin1'] if m == 1 else ref['margin']
                first = ref['cover'] if m == 1 else ref['first']
                ec = lf.ec_hanging(res, n, m)
                cl = lf.closure(res, n, m)
                row = rows.setdefault('k=%d m=%d' % (k, m), dict(sites=0, delayed_hm=0, delayed_ec=0,
                                                                 delayed_first=0, ec_lt_hm=0, ec_eq_hm=0,
                                                                 ec_gt_hm=0, ratio_hm=[], ratio_ec=[],
                                                                 blobs=0, frac=[0.0, 0.0, 0.0, 0.0],
                                                                 frac_ec_gt_hm=0, frac_ec_lt_hm=0))
                for i in range(n):
                    t = cl['entries'][i]
                    eh, ee, ef = hm[i][0], ec[i][0], first[i][0]
                    row['sites'] += 1
                    row['delayed_hm'] += eh > t
                    row['delayed_ec'] += ee > t
                    row['delayed_first'] += ef > t
                    row['ec_lt_hm'] += ee < eh
                    row['ec_eq_hm'] += ee == eh
                    row['ec_gt_hm'] += ee > eh
                    if t > 0:
                        row['ratio_hm'].append(math.sqrt(eh / t))
                        row['ratio_ec'].append(math.sqrt(ee / t))
                if groups is not None:
                    A, B = groups
                    sa, sb = seed_of(defn, k, A), seed_of(defn, k, B)
                    fr = []
                    for u in (cl['u'], lf.ultrametric_of(hm, tree), lf.ultrametric_of(ec, tree)):
                        fr.append(lf.fraction_before_merge_laminar(u, sa, sb, A)[1])
                    fr.append(lf.full_fraction_before_merge(res, n, sa, sb, A, k)[1])
                    row['blobs'] += 1
                    for x in range(4):
                        row['frac'][x] += float(fr[x])
                    row['frac_ec_gt_hm'] += fr[2] > fr[1]
                    row['frac_ec_lt_hm'] += fr[2] < fr[1]
    elapsed = time.monotonic() - started
    out = {}
    print('nuages %d en %.1f s' % (count, elapsed))
    print('%-8s %6s %10s %10s %10s %18s %22s %22s' % ('k m', 'sites', 'ret. H_m', 'ret. EC', 'ret. first',
                                                    'EC <,=,> H_m', 'sqrt(e/t) H_m moy/max', 'sqrt(e/t) EC moy/max'))
    for key in sorted(rows):
        r = rows[key]
        rh, re_ = r['ratio_hm'], r['ratio_ec']
        summary = dict(r)
        summary['ratio_hm'] = dict(mean=statistics.mean(rh), max=max(rh)) if rh else None
        summary['ratio_ec'] = dict(mean=statistics.mean(re_), max=max(re_)) if re_ else None
        if r['blobs']:
            summary['frac'] = dict(zip(('fermeture', 'H_m', 'EC', 'FULL'),
                                       [round(x / r['blobs'], 4) for x in r['frac']]))
        out[key] = summary
        print('%-8s %6d %10d %10d %10d %18s %22s %22s' % (
            key, r['sites'], r['delayed_hm'], r['delayed_ec'], r['delayed_first'],
            '%d,%d,%d' % (r['ec_lt_hm'], r['ec_eq_hm'], r['ec_gt_hm']),
            '%.3f/%.2f' % (summary['ratio_hm']['mean'], summary['ratio_hm']['max']),
            '%.3f/%.2f' % (summary['ratio_ec']['mean'], summary['ratio_ec']['max'])))
    print('\ntwo_blobs : fraction moyenne de A recuperee avant la reunion avec B')
    for key in sorted(out):
        if out[key].get('blobs'):
            print('%-8s nuages %d : %s ; EC >,< H_m : %d,%d' % (key, out[key]['blobs'], out[key]['frac'],
                                                                out[key]['frac_ec_gt_hm'],
                                                                out[key]['frac_ec_lt_hm']))
    with open(args.out, 'w') as fh:
        json.dump(dict(seed=args.seed, clouds=count, seconds=round(elapsed, 1), rows=out), fh, indent=1,
                  sort_keys=True, default=str)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
