"""Lecture des sorties de bigk_dev.py (graines dev) : effet de l'ordre K, apparie par scene.

  python3 analyse_bigk.py OUT1.csv [OUT2.csv ...] [--fill asc20_b2] [--boot 4000]
Imprime, pour le temoin MR2-bord (z = 3 et 6) et pour sklearn (feuilles, alpha = 2) : l'ARI_s moyen a chaque K, par
taille et par famille, l'ecart apparie a K = 10 avec un IC bootstrap 95 % sur les scenes, puis la tour a K = 10.
Les refus (ARI_s = 0, EVAL_v2 D8) restent dans les moyennes et sont comptes.
"""
import argparse
import collections
import csv

import numpy as np


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('csv', nargs='+')
    ap.add_argument('--fill', default='asc20_b2')
    ap.add_argument('--boot', type=int, default=4000)
    a = ap.parse_args()
    data = collections.defaultdict(dict)
    refused = 0
    for path in a.csv:
        for r in csv.DictReader(open(path)):
            if r['fill'] != a.fill:
                continue
            u = (r['family'], r['level'], r['noise'], r['n'], r['seed'])
            data[(r['method'], int(r['z']), int(r['k']))][u] = float(r['ari_s'])
            refused += int(r['refused'])
    units = sorted(set.intersection(*(set(v) for v in data.values())))
    sizes = sorted({u[3] for u in units}, key=int)
    fams = sorted({u[0] for u in units})
    rng = np.random.default_rng(20260929)
    print('%d scenes, tailles %s, remplissage %s, refus %d' % (len(units), ','.join(sizes), a.fill, refused))
    for meth, z in (('mr2b', 3), ('mr2b', 6), ('sklearn', 0)):
        ks = sorted(k for (m, zz, k) in data if m == meth and zz == z)
        ref = data[(meth, z, 10)]
        print('\n== %s z=%d' % (meth, z))
        print('%4s %7s %s %8s %18s %9s  %s' % ('K', 'ARI_s', ' '.join('%8s' % ('n=' + s) for s in sizes), 'delta',
                                               'IC95', 'gain/per', ' '.join('%9s' % f[:9] for f in fams)))
        for k in ks:
            cur = data[(meth, z, k)]
            d = np.array([cur[u] - ref[u] for u in units])
            m = d[rng.integers(0, len(d), size=(a.boot, len(d)))].mean(axis=1)
            by_n = [np.mean([cur[u] for u in units if u[3] == s]) for s in sizes]
            by_f = [np.mean([cur[u] - ref[u] for u in units if u[0] == f]) for f in fams]
            print('%4d %7.4f %s %+8.4f [%+7.4f ; %+7.4f] %4d/%-4d %s' % (
                k, np.mean([cur[u] for u in units]), ' '.join('%8.4f' % x for x in by_n), d.mean(),
                np.quantile(m, 0.025), np.quantile(m, 0.975), (d > 0.005).sum(), (d < -0.005).sum(),
                ' '.join('%+9.4f' % x for x in by_f)))
    tw = data[('tour', 6, 10)]
    print('\n== tour K=10 z=6 : ARI_s %.4f ; par taille %s ; par famille %s' % (
        np.mean([tw[u] for u in units]), ' '.join('%.4f' % np.mean([tw[u] for u in units if u[3] == s]) for s in sizes),
        ' '.join('%s=%.4f' % (f[:9], np.mean([tw[u] for u in units if u[0] == f])) for f in fams)))


if __name__ == '__main__':
    main()
