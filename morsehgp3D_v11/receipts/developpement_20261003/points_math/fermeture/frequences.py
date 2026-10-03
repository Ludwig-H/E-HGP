#!/usr/bin/env python3
"""Frequence et ampleur des reunions de la fermeture AVANT la fusion FULL, sur nuages aleatoires bornes.

    python3 frequences.py --clouds 900 --seed 11 --out resultats/frequences.json

Generateurs : gate (ex aequo frequents, comme points_gate), generic (position generale), two_blobs (deux amas de
3-4 sites + 1-2 sites de vallee). Pour chaque (nuage, k in {2, 3}, m in {1, k+1, k+2}) : evenements de
lib_fermeture.parasitic_events, classes en absorption / site_partage / deux_amas_entiers (seuil max(m, k+1) sites
exclusifs dans chaque lignee a un meme niveau avant la fusion FULL). Pour two_blobs, en plus : niveau de reunion
des germes de A et B (fermeture contre FULL) et fraction de A recuperee avant (fermeture, FULL, H_m).
Ce sont des frequences d'oracle sur petits nuages : elles illustrent un mecanisme, elles n'etablissent aucune
pente ni aucune frequence a l'echelle n = 8000.
"""
import argparse
from fractions import Fraction
import json
import math
import random
import statistics
import sys
import time

sys.dont_write_bytecode = True
import lib_fermeture as lf  # noqa: E402

KINDS = ('absorption', 'site_partage', 'deux_amas_entiers')


def seed_of(defn, k, group):
    return min(group, key=lambda i: (defn.nearest(i)[k - 1][0], i))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--clouds', type=int, default=900)
    parser.add_argument('--seed', type=int, default=11)
    parser.add_argument('--seconds', type=float, default=300.0)
    parser.add_argument('--out', required=True)
    args = parser.parse_args()
    rng = random.Random(args.seed)
    table = {}
    blobs = {}
    started = time.monotonic()
    counts = dict(gate=0, generic=0, two_blobs=0)
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
            pts, A, B, V = lf.cloud_two_blobs(rng)
            groups = (A, B)
        counts[gen] += 1
        n = len(pts)
        defn = lf.Definition(pts)
        for k in (2, 3):
            if k >= n:
                continue
            res = defn.order(k)
            merge_pairs = sum(len(nd.children) * (len(nd.children) - 1) // 2 for nd in res.nodes if nd.children)
            for m in sorted(set([1, k + 1, k + 2])):
                if m > n:
                    continue
                key = '%s k=%d m=%d' % (gen, k, m)
                row = table.setdefault(key, dict(clouds=0, merge_pairs=0, events=0, clouds_any=0,
                                                 **{'clouds_' + x: 0 for x in KINDS},
                                                 **{'events_' + x: 0 for x in KINDS},
                                                 ratios_two_whole=[], ratios_site=[]))
                ev = lf.parasitic_events(res, n, m, k)
                row['clouds'] += 1
                row['merge_pairs'] += merge_pairs
                row['events'] += len(ev)
                row['clouds_any'] += bool(ev)
                for x in KINDS:
                    sel = [e for e in ev if e['kind'] == x]
                    row['events_' + x] += len(sel)
                    row['clouds_' + x] += bool(sel)
                row['ratios_two_whole'] += [e['ratio_r'] for e in ev if e['kind'] == 'deux_amas_entiers']
                row['ratios_site'] += [e['ratio_r'] for e in ev if e['kind'] == 'site_partage']
                if groups is not None:
                    A, B = groups
                    sa, sb = seed_of(defn, k, A), seed_of(defn, k, B)
                    mf, ff = lf.full_fraction_before_merge(res, n, sa, sb, A, k)
                    cl = lf.closure(res, n, m)
                    mc, fc = lf.fraction_before_merge_laminar(cl['u'], sa, sb, A)
                    ref, tree = lf.faithful_rules(res, n, m)
                    uh = lf.ultrametric_of(ref['margin'], tree)
                    mh, fh = lf.fraction_before_merge_laminar(uh, sa, sb, A)
                    b = blobs.setdefault('k=%d m=%d' % (k, m), dict(clouds=0, closure_before_full=0,
                                                                    closure_equal_full=0, closure_after_full=0,
                                                                    ratio_r=[], frac_cl_gt_full=0,
                                                                    frac_cl_lt_full=0, frac_cl_eq_full=0,
                                                                    frac_cl_gt_hm=0, frac_cl_lt_hm=0,
                                                                    frac_cl_eq_hm=0, mean_frac=[0, 0, 0]))
                    b['clouds'] += 1
                    if mc < mf:
                        b['closure_before_full'] += 1
                        b['ratio_r'].append(math.sqrt(mc / mf))
                    elif mc == mf:
                        b['closure_equal_full'] += 1
                    else:
                        b['closure_after_full'] += 1
                    b['frac_cl_gt_full'] += fc > ff
                    b['frac_cl_lt_full'] += fc < ff
                    b['frac_cl_eq_full'] += fc == ff
                    b['frac_cl_gt_hm'] += fc > fh
                    b['frac_cl_lt_hm'] += fc < fh
                    b['frac_cl_eq_hm'] += fc == fh
                    b['mean_frac'][0] += float(fc)
                    b['mean_frac'][1] += float(ff)
                    b['mean_frac'][2] += float(fh)
    elapsed = time.monotonic() - started

    def summ(xs):
        if not xs:
            return None
        return dict(n=len(xs), min=round(min(xs), 4), median=round(statistics.median(xs), 4),
                    mean=round(statistics.mean(xs), 4))
    out_table = {}
    for key, row in table.items():
        r = dict(row)
        r['ratios_two_whole'] = summ(row['ratios_two_whole'])
        r['ratios_site'] = summ(row['ratios_site'])
        out_table[key] = r
    out_blobs = {}
    for key, b in blobs.items():
        r = dict(b)
        r['ratio_r'] = summ(b['ratio_r'])
        r['mean_frac'] = [round(x / b['clouds'], 4) for x in b['mean_frac']]
        out_blobs[key] = r
    with open(args.out, 'w') as fh:
        json.dump(dict(seed=args.seed, clouds=counts, seconds=round(elapsed, 1), table=out_table,
                       two_blobs=out_blobs), fh, indent=1, sort_keys=True)
    print('nuages %s en %.1f s' % (counts, elapsed))
    print('%-22s %6s %7s %7s %8s %8s %8s   %s' % ('generateur k m', 'nuages', 'fusions', 'avance',
                                                 'absorb.', 'site', '2 amas', 'rapport rayon 2 amas (min/med)'))
    for key in sorted(out_table):
        r = out_table[key]
        rt = r['ratios_two_whole']
        print('%-22s %6d %7d %7d %8d %8d %8d   %s' % (
            key, r['clouds'], r['merge_pairs'], r['clouds_any'], r['clouds_absorption'], r['clouds_site_partage'],
            r['clouds_deux_amas_entiers'], '%s/%s (%d evts)' % (rt['min'], rt['median'], rt['n']) if rt else '-'))
    print('\ntwo_blobs : reunion des germes de A et B, fermeture contre FULL ; fraction de A recuperee avant')
    for key in sorted(out_blobs):
        b = out_blobs[key]
        print('%-8s nuages %3d | fermeture avant FULL %3d, egale %3d, apres %d ; rapport rayon %s | frac fermeture'
              ' >,=,< FULL : %d,%d,%d ; >,=,< H_m : %d,%d,%d ; moyennes fermeture/FULL/H_m %s' % (
                  key, b['clouds'], b['closure_before_full'], b['closure_equal_full'], b['closure_after_full'],
                  b['ratio_r'], b['frac_cl_gt_full'], b['frac_cl_eq_full'], b['frac_cl_lt_full'],
                  b['frac_cl_gt_hm'], b['frac_cl_eq_hm'], b['frac_cl_lt_hm'], b['mean_frac']))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
