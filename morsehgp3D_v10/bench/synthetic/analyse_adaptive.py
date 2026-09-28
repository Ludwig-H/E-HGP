"""Lecture d'une sortie d'adaptive_dev.py (graines dev) : configurations fixes contre choix DBCV par scene, sur des
sous-grilles quelconques recalculees hors ligne a partir des DBCV enregistres (meme regle : DBCV maximal, egalites
departagees par l'ordre de la grille).

  python3 analyse_adaptive.py OUT.csv [--fill full]
"""
import argparse
import collections
import csv


def grid_order(method, key):
    """Ordre de la grille d'adaptive_dev.py (tower_configs / hdb_configs), pour departager les egalites."""
    parts = key.split('_')
    if method == 'tower':
        k, sel, z = int(parts[0]), parts[1], parts[2]
        return (k, 0 if sel == 'eom' else 1, 0 if z == '1' else 1)
    ms, sel, a = int(parts[0]), parts[1], float(parts[2])
    return (ms, 0 if sel == 'eom' else 1, a)


def load(path, fill):
    units = collections.defaultdict(lambda: collections.defaultdict(dict))
    for r in csv.DictReader(open(path)):
        if r['fill'] != fill or not r['criterion'].startswith('fixed_'):
            continue
        u = (r['family'], r['level'], r['noise'], r['n'], r['seed'])
        units[u][r['method']][r['criterion'][6:]] = (float(r['dbcv']), float(r['ari_s']))
    return units


def select(cfgs, keep, method):
    grid = sorted((c for c in cfgs if keep(c)), key=lambda c: grid_order(method, c))
    best = max(grid, key=lambda c: (cfgs[c][0], -grid.index(c)))
    return best, cfgs[best][1]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('csv')
    ap.add_argument('--fill', default='full')
    a = ap.parse_args()
    units = load(a.csv, a.fill)
    print('%d unites, remplissage %s' % (len(units), a.fill))
    subgrids = {
        'tower': {
            'K<=5': lambda c: int(c.split('_')[0]) <= 5,
            'K<=10': lambda c: True,
            'K<=10 eom': lambda c: c.split('_')[1] == 'eom',
        },
        'hdb': {
            'ms<=5 a=1': lambda c: int(c.split('_')[0]) <= 5 and c.endswith('_1.0'),
            'ms<=10 a=1': lambda c: int(c.split('_')[0]) <= 10 and c.endswith('_1.0'),
            'ms<=20 a=1': lambda c: c.endswith('_1.0'),
            'ms<=20 a=1,2': lambda c: True,
            'ms<=10 a=1,2': lambda c: int(c.split('_')[0]) <= 10,
        },
    }
    fams = sorted({u[0] for u in units})
    for method in ('tower', 'hdb'):
        cfgs = sorted({c for u in units.values() for c in u[method]}, key=lambda c: grid_order(method, c))
        mean = {c: sum(u[method][c][1] for u in units.values()) / len(units) for c in cfgs}
        best = max(cfgs, key=lambda c: mean[c])
        oracle = sum(max(v[1] for v in u[method].values()) for u in units.values()) / len(units)
        print('\n== %s : meilleure configuration fixe %s = %.4f ; oracle par scene %.4f' % (method, best, mean[best],
                                                                                            oracle))
        for name, keep in subgrids[method].items():
            picks = [select(u[method], keep, method) for u in units.values()]
            fam = collections.defaultdict(list)
            for uk, (_, s) in zip(units.keys(), picks):
                fam[uk[0]].append(s)
            bestfix = max((c for c in cfgs if keep(c)), key=lambda c: mean[c])
            print('  DBCV %-14s %.4f (fixe meilleur de la sous-grille %s = %.4f) | ' % (
                name, sum(s for _, s in picks) / len(picks), bestfix, mean[bestfix]) +
                ' '.join('%s:%.3f' % (f[:5], sum(fam[f]) / len(fam[f])) for f in fams))
        top = sorted(cfgs, key=lambda c: -mean[c])[:5]
        print('  top fixes : ' + ', '.join('%s %.4f' % (c, mean[c]) for c in top))


if __name__ == '__main__':
    main()
