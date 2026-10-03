"""Strategie differee contre le minimum exhaustif Gram/Fraction existant.

Les deux routes partagent les spheres Fraction ; le minimum englobant, calcule
sans filtre positif, reste l'autorite geometrique. Ici on juge le changement
d'ordre des calculs et les sept compteurs logiques, pas un nouveau moteur.
"""
import copy
import itertools as it
import sys
from collections import Counter

import fraction_oracle as reference


BASES = (((3, 4, 5),), ((0, 0, 0), (4, 0, 0), (2, 3, 0)),
         ((1, 2, 0), (0, 5, 0), (8, 1, 0), (8, 9, 0)),
         ((10, 5, 5), (9, 8, 5), (5, 2, 1), (1, 5, 8)),
         ((0, 0, 0), (4, 4, 0), (4, 0, 4), (0, 4, 4)),
         ((5, 5, 0), (2, 1, 5), (10, 5, 5), (2, 9, 5), (5, 9, 8)),
         ((0, 0, 0), (1, 0, 0), (2, 0, 0), (4, 4, 0), (4, 0, 4), (0, 4, 4)),
         ((0, 0, 0), (4, 4, 0), (4, 0, 4), (0, 4, 4), (7, 1, 2), (3, 7, 1), (1, 2, 7)),
         ((0, 0, 5), (2, 7, 4), (0, 7, 6), (8, 4, 0), (8, 1, 6), (1, 2, 8)))
FIELDS = ('presentations', 'nondegenerate', 'positive', 'containing', 'comparisons', 'point_tests',
          'diameter_pairs')
require = reference.require


def fixtures(bits):
    return [tuple(sorted((tuple(scale*v for v in p) for p in base), key=reference.morton))
            for base in BASES for scale in (1, ((1 << bits)-1)//16)]


def deferred(points):
    n = len(points)
    ledger = dict.fromkeys(FIELDS, 0)
    stats = Counter()
    first = (0,)
    if n > 1:
        pairs = list(it.combinations(range(n), 2))
        lengths = {t: reference.dot(reference.sub(points[t[0]], points[t[1]]),
                                    reference.sub(points[t[0]], points[t[1]])) for t in pairs}
        first = min(t for t in pairs if lengths[t] == max(lengths.values()))
        ledger['diameter_pairs'] = len(pairs)
    candidates = it.chain((first,), it.combinations(range(n), 3), it.combinations(range(n), 4))
    for support in candidates:
        q = len(support)
        ledger['presentations'] += 1
        ball = reference.circumsphere(tuple(points[i] for i in support))
        if ball is None:
            stats[f'q{q}_degenerate'] += 1
            continue
        center, radius, weights = ball
        ledger['nondegenerate'] += 1
        if min(weights) <= 0:
            stats[f'q{q}_non_strict'] += 1
            continue
        ledger['positive'] += 1
        # q3 Sphere devient necessaire ici ; q4 ne paie toujours pas sa materialisation.
        stats['materialized_before_scan'] += q != 4
        included = True
        for point in points:
            ledger['point_tests'] += 1
            v = reference.sub(point, center)
            if reference.dot(v, v) > radius:
                stats[f'q{q}_outside'] += 1
                included = False
                break
        if not included:
            continue
        ledger['containing'] += 1
        stats['materialized_after_scan'] += q == 4
        return dict(support=support, center=center, radius=radius, ledger=ledger), stats
    raise ValueError('aucun certificat englobant')


def judge(points, actual):
    truth = reference.local_meb(points)
    require(actual.keys() == {'support', 'center', 'radius', 'ledger'}, 'champs resultat')
    for key in ('support', 'center', 'radius'):
        require(actual[key] == truth[key], key)
    require(actual['ledger'].keys() == set(FIELDS), 'champs ledger')
    for key in FIELDS:
        require(type(actual['ledger'][key]) is int and actual['ledger'][key] == truth['ledger'][key], key)
    return 12


def selftest():
    checks = count = corruptions = 0
    totals = Counter()
    for bits in (18, 21, 24):
        for points in fixtures(bits):
            actual, stats = deferred(points)
            checks += judge(points, actual)
            count += 1
            totals.update(stats)
            for key in FIELDS:
                bad = copy.deepcopy(actual)
                bad['ledger'][key] += 1
                try:
                    judge(points, bad)
                except ValueError:
                    corruptions += 1
                else:
                    raise ValueError('ledger corrompu accepte')
            for key, value in (('support', ()), ('center', (0, 0, -1)), ('radius', actual['radius']+1)):
                bad = dict(actual, **{key: value})
                try:
                    judge(points, bad)
                except ValueError:
                    corruptions += 1
                else:
                    raise ValueError('geometrie corrompue acceptee')
    for key in ('q3_degenerate', 'q3_non_strict', 'q4_degenerate', 'q4_non_strict', 'q4_outside',
                'materialized_before_scan', 'materialized_after_scan'):
        require(totals[key] > 0, 'branche non exercee '+key)
        checks += 1
    print(f'meb_deferred_model_verdict conforme requests{count} checks{checks} corruptions{corruptions} native0')
    print('meb_deferred_branches '+str(dict(sorted(totals.items()))))


if __name__ == '__main__':
    try:
        selftest()
    except ValueError as error:
        print('REFUS meb_deferred_model:', error, file=sys.stderr)
        sys.exit(1)
