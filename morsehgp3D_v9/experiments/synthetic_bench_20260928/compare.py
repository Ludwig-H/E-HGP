"""Comparaison appariee : tous les couples, toutes les graines, aucun choix.

  python3 compare.py --challenger tour.csv --reference baselines.csv --out comparaison.csv

L'audit du 28 septembre a releve trois fois la meme faute sur la campagne du
27 : le resultat s'inversait sur une graine sur deux sans qu'un agregat par
graine soit publie nulle part, et le fichier de comparaison ne portait que la
paire favorable. Ce module rend le tri impossible.

Il apparie sur (scene, graine), ce qui compare deux methodes sur les memes
octets, et il publie **tous** les couples methode-reference qu'il trouve, y
compris ceux qui perdent. Chaque ligne porte l'ecart moyen, l'ecart median, le
pire ecart, le decompte victoires/defaites/nuls, et l'ecart par graine pris
separement : une inversion de graine se lit alors directement dans le tableau.
"""

import argparse
import collections
import csv
import json
import sys

COLUMNS = ('challenger', 'reference', 'slice', 'pairs', 'mean_delta', 'median_delta', 'worst_delta',
           'best_delta', 'wins', 'losses', 'ties', 'mean_challenger', 'mean_reference',
           'mean_coverage_challenger', 'mean_coverage_reference', 'per_seed_delta', 'seed_inversion')
TIE = 1e-9


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def read(path):
    """Lit un CSV de mesures et le range par (methode, scene, graine)."""
    rows = {}
    with open(path, newline='') as handle:
        for row in csv.DictReader(handle):
            for field in ('scene', 'seed', 'method', 'ari'):
                need(field in row, path + ': missing column ' + field)
            key = (row['method'], row['scene'], int(row['seed']))
            need(key not in rows, path + ': duplicate measurement for ' + str(key))
            rows[key] = row
    need(rows, path + ': no measurement')
    return rows


def _median(values):
    ordered = sorted(values)
    middle = len(ordered) // 2
    if len(ordered) % 2:
        return ordered[middle]
    return 0.5 * (ordered[middle - 1] + ordered[middle])


def _cell(challenger, reference, label, pairs):
    """Une ligne de comparaison, avec le detail par graine."""
    deltas = [c - r for c, r, _, _, _ in pairs]
    by_seed = collections.defaultdict(list)
    for delta, (_, _, seed, _, _) in zip(deltas, pairs):
        by_seed[seed].append(delta)
    seed_means = {seed: sum(values) / len(values) for seed, values in by_seed.items()}
    signs = {value > TIE for value in seed_means.values()}
    return dict(
        challenger=challenger, reference=reference, slice=label, pairs=len(pairs),
        mean_delta=sum(deltas) / len(deltas), median_delta=_median(deltas),
        worst_delta=min(deltas), best_delta=max(deltas),
        wins=sum(1 for d in deltas if d > TIE), losses=sum(1 for d in deltas if d < -TIE),
        ties=sum(1 for d in deltas if abs(d) <= TIE),
        mean_challenger=sum(c for c, _, _, _, _ in pairs) / len(pairs),
        mean_reference=sum(r for _, r, _, _, _ in pairs) / len(pairs),
        mean_coverage_challenger=sum(cc for _, _, _, cc, _ in pairs) / len(pairs),
        mean_coverage_reference=sum(rc for _, _, _, _, rc in pairs) / len(pairs),
        per_seed_delta=json.dumps({str(k): round(v, 4) for k, v in sorted(seed_means.items())}),
        seed_inversion=len(signs) > 1)


def compare(challenger_rows, reference_rows, slices=('all',)):
    """Tous les couples (methode candidate, methode de reference), sans exception.

    Rend une ligne par couple et par tranche. Une methode presente des deux
    cotes n'est jamais comparee a elle-meme.
    """
    challengers = sorted({method for method, _, _ in challenger_rows})
    references = sorted({method for method, _, _ in reference_rows})
    out = []
    for challenger in challengers:
        for reference in references:
            if challenger == reference:
                continue
            buckets = collections.defaultdict(list)
            for (method, scene, seed), row in challenger_rows.items():
                if method != challenger:
                    continue
                other = reference_rows.get((reference, scene, seed))
                if other is None:
                    continue
                item = (float(row['ari']), float(other['ari']), seed,
                        float(row.get('coverage', 'nan')), float(other.get('coverage', 'nan')))
                for label in slices:
                    if label == 'all':
                        buckets['all'].append(item)
                    elif label in row and row[label]:
                        buckets[label + '=' + row[label]].append(item)
            need(buckets, 'no paired scene between %s and %s' % (challenger, reference))
            for label in sorted(buckets):
                out.append(_cell(challenger, reference, label, buckets[label]))
    need(out, 'no comparable pair at all')
    return out


def main(argv=None):
    parser = argparse.ArgumentParser(description='paired comparison, every pair published')
    parser.add_argument('--challenger', required=True)
    parser.add_argument('--reference', required=True)
    parser.add_argument('--out', required=True)
    parser.add_argument('--slices', default='all,family,level')
    args = parser.parse_args(argv)
    slices = tuple(name for name in args.slices.split(',') if name)
    table = compare(read(args.challenger), read(args.reference), slices)
    with open(args.out, 'w', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=COLUMNS)
        writer.writeheader()
        for row in table:
            writer.writerow({key: row[key] for key in COLUMNS})
    overall = [row for row in table if row['slice'] == 'all']
    for row in sorted(overall, key=lambda r: -r['mean_delta']):
        print('%-22s contre %-22s %+.4f  (%d-%d-%d)%s'
              % (row['challenger'], row['reference'], row['mean_delta'],
                 row['wins'], row['losses'], row['ties'],
                 '  INVERSION DE GRAINE' if row['seed_inversion'] else ''))
    return 0


if __name__ == '__main__':
    sys.exit(main())
