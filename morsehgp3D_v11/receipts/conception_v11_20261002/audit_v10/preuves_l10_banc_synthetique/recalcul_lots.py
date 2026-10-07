"""Audit L10 : recalcul independant des lots A et C a partir des results.csv des recus (aucune reprise de decide.py).

Usage : python3 recalcul_lots.py <results.csv du lot A> <results.csv du lot C>
Sortie : texte sur stdout. Lecture seule ; aucun fichier ecrit.
"""
import csv
import statistics
import sys
from collections import defaultdict


def load(path):
    rows = list(csv.DictReader(open(path)))
    units = defaultdict(dict)
    meta = {}
    for r in rows:
        assert r['refused'] == '0', r
        units[r['unit']][r['method']] = float(r['ari_s'])
        meta[r['unit']] = (r['family'], r['level'], r['noise'], r['n'])
    return units, meta


def wmean(units, meta, values):
    """Moyenne ponderee par cellule (famille, niveau, bruit, taille) : chaque cellule pese autant."""
    cells = defaultdict(list)
    for u, v in values.items():
        cells[meta[u]].append(v)
    return sum(sum(v) / len(v) for v in cells.values()) / len(cells), len(cells)


def pair(units, meta, a, b, keep=lambda m: True):
    d = {u: units[u][a] - units[u][b] for u in units if keep(meta[u])}
    mean, ncell = wmean(units, meta, d)
    vals = list(d.values())
    wins = sum(1 for x in vals if x > 1e-9)
    losses = sum(1 for x in vals if x < -1e-9)
    return dict(mean=mean, median=statistics.median(vals), wins=wins, losses=losses, ties=len(vals) - wins - losses,
                n=len(vals), cells=ncell)


def show(title, units, meta, pairs):
    fams = sorted({m[0] for m in meta.values()})
    print('\n=== ' + title)
    print('scenes', len(units), 'methodes', len(next(iter(units.values()))))
    print('%-22s %8s %8s %6s %6s %6s | %s' % ('paire', 'moyenne', 'mediane', 'gains', 'pertes', 'egal',
                                              'sans shells / par famille'))
    for name, a, b in pairs:
        s = pair(units, meta, a, b)
        ns = pair(units, meta, a, b, keep=lambda m: m[0] != 'shells')
        byf = ' '.join('%s%+.3f' % (f[:4], pair(units, meta, a, b, keep=lambda m, f=f: m[0] == f)['mean'])
                       for f in fams)
        print('%-22s %+8.4f %+8.4f %6d %6d %6d | sans shells %+.4f (gains %d, pertes %d) | %s' % (
            name, s['mean'], s['median'], s['wins'], s['losses'], s['ties'], ns['mean'], ns['wins'], ns['losses'],
            byf))


def means(units, meta, methods):
    print('\nARI_s moyen pondere par cellule :')
    for m in methods:
        v, _ = wmean(units, meta, {u: units[u][m] for u in units})
        print('  %-14s %.4f' % (m, v))


def main():
    a_units, a_meta = load(sys.argv[1])
    c_units, c_meta = load(sys.argv[2])
    means(a_units, a_meta, ['tw_K1', 'hdb_ms1', 'tw_K2', 'hdb_ms2', 'tw_K3', 'hdb_ms3', 'hdb_lib', 'hdb_these_K1'])
    show('LOT A : tour (tete C inter X, zhat) contre sklearn regle sur dev, K = min_samples', a_units, a_meta,
         [('K=%d' % k, 'tw_K%d' % k, 'hdb_ms%d' % k) for k in (1, 2, 3)])
    ks = (1, 2, 3, 5, 8, 10)
    means(c_units, c_meta, sorted(next(iter(c_units.values()))))
    show('LOT C principal : tour v10-b contre sklearn regle sur dev (avec remplissage)', c_units, c_meta,
         [('K=%d' % k, 'tw_K%d' % k, 'hdb_ms%d' % k) for k in ks])
    show('LOT C descriptif : sans remplissage (tour EOM z dev ; sklearn EOM alpha=1)', c_units, c_meta,
         [('K=%d nf' % k, 'tw_K%d_nf' % k, 'hdb_ms%d_nf' % k) for k in ks])
    show('LOT C objet : tour contre MR2-bord (meme entree, meme tete)', c_units, c_meta,
         [('K=%d obj' % k, 'tw_K%d' % k, 'mrb_K%d' % k) for k in (2, 3, 5, 8, 10)])
    show('LOT C lot B : tete C inter X du 28 septembre contre sklearn', c_units, c_meta,
         [('K=%d capB' % k, 'cap_K%d' % k, 'hdb_ms%d' % k) for k in (5, 8, 10)])
    # sklearn libre de choisir min_samples parmi les six K testes (meilleure moyenne globale : lue ci-dessus)
    best = max(('hdb_ms%d' % k for k in ks), key=lambda m: wmean(c_units, c_meta, {u: c_units[u][m] for u in c_units})[0])
    show('LOT C : tour a K contre le meilleur sklearn des six K testes (%s)' % best, c_units, c_meta,
         [('K=%d vs %s' % (k, best), 'tw_K%d' % k, best) for k in ks])
    # tour contre le MEILLEUR des deux sklearn mesures au meme K, scene par scene (majorant de l adversaire, diagnostic)
    print('\n=== LOT C : par famille, ARI_s moyen de tw_K, hdb_msK (regle dev, rempli), hdb_msK_nf (EOM alpha=1, non rempli)')
    fams = sorted({m[0] for m in c_meta.values()})
    for k in ks:
        line = []
        for f in fams:
            us = [u for u in c_units if c_meta[u][0] == f]
            t = sum(c_units[u]['tw_K%d' % k] for u in us) / len(us)
            h = sum(c_units[u]['hdb_ms%d' % k] for u in us) / len(us)
            hn = sum(c_units[u]['hdb_ms%d_nf' % k] for u in us) / len(us)
            line.append('%s %.3f/%.3f/%.3f' % (f[:5], t, h, hn))
        print('K=%-2d ' % k + ' | '.join(line))
    # par taille et par niveau
    for key, idx in (('n', 3), ('niveau', 1), ('bruit', 2)):
        print('\n=== LOT C principal par %s (ecart moyen tour - sklearn)' % key)
        vals = sorted({m[idx] for m in c_meta.values()}, key=lambda x: (len(x), x))
        for k in ks:
            print('K=%-2d ' % k + '  '.join('%s:%+.3f' % (v, pair(c_units, c_meta, 'tw_K%d' % k, 'hdb_ms%d' % k,
                                                                    keep=lambda m, v=v: m[idx] == v)['mean'])
                                             for v in vals))


if __name__ == '__main__':
    main()
