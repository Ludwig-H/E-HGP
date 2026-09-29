"""Lecture d'une sortie d'alloc_dev.py (graines dev) : remplissages compares par tete et par K, apparies par scene.

  python3 analyse_alloc.py OUT.csv [--boot 4000]
Imprime :
  - pour chaque tete (tour, sklearn) et chaque K, l'ARI_s moyen de chaque remplissage (toutes tailles, puis par n),
    l'ecart apparie au remplissage du lot C (IC bootstrap 95 % sur les scenes), gains et pertes a 0,005 pres, la
    couverture et l'AMI ;
  - l'ecart tour - sklearn, chaque tete avec le remplissage du lot C, puis les deux avec le meme remplissage (le
    remplissage est externe a l'objet : il ne doit pas etre compte a la tour) ;
  - l'ecart par famille du meilleur remplissage de chaque tete au remplissage du lot C.
"""
import argparse
import collections
import csv

import numpy as np

LOTC = {'tour': {1: 'b2', 2: 'b2', 3: 'b2', 5: 'b2', 8: 'b1.5', 10: 'b1.5'},
        'sklearn': {1: 'b1.5', 2: 'b1.5', 3: 'b2', 5: 'b2.5', 8: 'b2.5', 10: 'b2.5'}}


def load(path):
    data = collections.defaultdict(dict)  # (tete, K, remplissage) -> {scene: (ari_s, ami_nc, couverture)}
    refused = collections.Counter()  # refus (EVAL_v2 D8) : lignes a ARI_s = 0, gardees dans les moyennes
    for r in csv.DictReader(open(path)):
        u = (r['family'], r['level'], r['noise'], r['n'], r['seed'])
        data[(r['method'], int(r['k']), r['fill'])][u] = (float(r['ari_s']), float(r['ami_nc']), float(r['coverage']))
        if int(r.get('refused') or 0) and r['fill'] == 'none':
            refused[(r['method'], int(r['k']))] += 1
    return data, refused


def paired(a, b, units):
    return np.array([a[u][0] - b[u][0] for u in units])


def ci(d, boot, rng):
    m = d[rng.integers(0, len(d), size=(boot, len(d)))].mean(axis=1)
    return float(np.quantile(m, 0.025)), float(np.quantile(m, 0.975))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('csv')
    ap.add_argument('--boot', type=int, default=4000)
    a = ap.parse_args()
    data, refused = load(a.csv)
    rng = np.random.default_rng(20260929)
    heads = sorted({m for m, _, _ in data})
    ks = sorted({k for _, k, _ in data})
    fills = sorted({f for _, _, f in data}, key=lambda f: (f.startswith('asc'), f))
    units = sorted(set.intersection(*(set(v) for v in data.values())))
    sizes = sorted({u[3] for u in units}, key=int)
    fams = sorted({u[0] for u in units})
    print('%d scenes completes (toutes tetes, K et remplissages), tailles %s' % (len(units), ','.join(sizes)))
    print('refus (ARI_s = 0, gardes) : %s' % (dict(refused) if refused else 'aucun'))
    best = {}
    for m in heads:
        for k in ks:
            ref = data[(m, k, LOTC[m][k])]
            print('\n== %s K=%d (remplissage du lot C : %s)' % (m, k, LOTC[m][k]))
            print('%-11s %7s %s %8s %18s %9s %6s %6s' % ('remplissage', 'ARI_s', ' '.join('%7s' % ('n=' + s) for s in sizes),
                                                      'delta', 'IC95', 'gain/per', 'couv', 'AMI'))
            rows = []
            for f in fills:
                cur = data[(m, k, f)]
                d = paired(cur, ref, units)
                lo, hi = ci(d, a.boot, rng)
                by_n = [np.mean([cur[u][0] for u in units if u[3] == s]) for s in sizes]
                rows.append((float(np.mean([cur[u][0] for u in units])), f))
                print('%-11s %7.4f %s %+8.4f [%+7.4f ; %+7.4f] %4d/%-4d %6.3f %6.4f' % (
                    f, rows[-1][0], ' '.join('%7.4f' % x for x in by_n), d.mean(), lo, hi, (d > 0.005).sum(),
                    (d < -0.005).sum(), np.mean([cur[u][2] for u in units]), np.mean([cur[u][1] for u in units])))
            best[(m, k)] = max(rows)[1]
    print('\n== tour - sklearn')
    print('%4s %22s %s' % ('K', 'remplissages du lot C', ' '.join('%18s' % f for f in fills)))
    for k in ks:
        d = paired(data[('tour', k, LOTC['tour'][k])], data[('sklearn', k, LOTC['sklearn'][k])], units)
        same = [paired(data[('tour', k, f)], data[('sklearn', k, f)], units).mean() for f in fills]
        print('%4d %+22.4f %s' % (k, d.mean(), ' '.join('%+18.4f' % x for x in same)))
    print('\n== meilleur remplissage de chaque tete moins celui du lot C, par famille')
    print('%-8s %4s %-11s %s' % ('tete', 'K', 'meilleur', ' '.join('%12s' % f[:12] for f in fams)))
    for m in heads:
        for k in ks:
            f = best[(m, k)]
            d = [paired(data[(m, k, f)], data[(m, k, LOTC[m][k])], [u for u in units if u[0] == fam]).mean()
                 for fam in fams]
            print('%-8s %4d %-11s %s' % (m, k, f, ' '.join('%+12.4f' % x for x in d)))


if __name__ == '__main__':
    main()
