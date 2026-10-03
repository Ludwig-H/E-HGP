#!/usr/bin/env python3
"""Exact integer equilateral plateau and bounded invariance review."""
from fractions import Fraction
from itertools import permutations
import hashlib
import json

from projection_helpers import Definition, Reference, judge, project

CHECKS = 0
ORIGINAL = [(-1, -1, 0), (-1, 0, -1), (0, 0, 0),
            (1, 1, 0), (2, 2, 0), (2, 1, 1)]
POINTS = [tuple(x + 2 for x in p) for p in ORIGINAL]


def check(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise RuntimeError(message)


def rename(group, ordering):
    return ''.join(sorted(chr(65 + ordering[ord(c) - 65]) for c in group))


def mask_label(mask, ordering):
    return ''.join(sorted(chr(65 + ordering[i]) for i in range(6) if mask & (1 << i)))


def fingerprint(result, ordering, square):
    def level(value):
        return None if value is None else str(Fraction(value) / square)
    cuts = []
    for cut in result.cuts:
        cuts.append(dict(level=level(cut.level),
                         opened=sorted((mask_label(cov, ordering), mask_label(cor, ordering))
                                       for _v, cov, cor in cut.opened),
                         closed=sorted((mask_label(cov, ordering), mask_label(cor, ordering))
                                       for _v, cov, cor in cut.closed)))
    entries = {}
    for i, entry in enumerate(result.cover):
        _opened, closed = judge.cut_at(result, entry.level)
        pop = {v: mask_label(cov, ordering) for v, cov, _cor in closed}
        entries[chr(65 + ordering[i])] = dict(date=level(entry.level),
                                              owners=sorted(pop[v] for v in entry.nodes))
    families = {}
    for mode in ('core', 'first_cover_lca'):
        families[mode] = sorted((rename(g['group'], ordering), level(g['start']), level(g['end']))
                                for g in project(result, mode))
    return dict(k=result.k, node_levels_arities=sorted((level(n.level), len(n.children)) for n in result.nodes),
                cuts=cuts, first_cover=entries, families=families)


def main():
    check(all(x - y - z == 0 for x, y, z in ORIGINAL), 'coplanarity')
    opposite = (4, 5, 3, 2, 0, 1)
    check(all((1 - p[0], 1 - p[1], -p[2]) == ORIGINAL[opposite[i]] for i, p in enumerate(ORIGINAL)),
          'central symmetry exchanges A/E, B/F, C/D')
    distances = {}
    for i in range(6):
        for j in range(i + 1, 6):
            distances[chr(65 + i) + chr(65 + j)] = sum((a - b) ** 2 for a, b in zip(ORIGINAL[i], ORIGINAL[j]))
    seven = ['AB', 'AC', 'BC', 'CD', 'DE', 'DF', 'EF']
    check(sorted(key for key, value in distances.items() if value == 2) == seven, 'exact seven short pairs')
    check(all(value >= 2 for value in distances.values()), 'unexpected shorter pair')
    base = Definition(POINTS)
    check(base.beta((1, 2, 3)) == Fraction(3, 2) and
          base.beta((2, 3, 5)) == Fraction(3, 2), 'MEB BCD/CDF proves joining level')
    wanted = [fingerprint(base.order(k), tuple(range(6)), 1) for k in (1, 2, 3)]
    r2, r3 = base.order(2), base.order(3)
    check(len(r2.nodes) == 10, 'FULL2 node count')
    check([(n.level, len(n.children)) for n in r2.nodes] ==
          [(Fraction(1, 2), 0)] * 7 + [(Fraction(2, 3), 3)] * 2 + [(Fraction(3, 2), 3)],
          'atomic seven births / two triples / root')
    for value, expected in ((Fraction(1, 2), seven), (Fraction(2, 3), ['ABC', 'CD', 'DEF']),
                            (Fraction(3, 2), ['ABCDEF'])):
        _opened, closed = judge.cut_at(r2, value)
        check(sorted(mask_label(cov, tuple(range(6))) for _v, cov, _cor in closed) == expected,
              'FULL2 closed coverage')
    check([len(e.nodes) for e in r2.cover] == [2, 2, 3, 3, 2, 2], 'complete first-cover incidences')
    check(wanted[1]['first_cover']['C']['owners'] == ['AC', 'BC', 'CD'] and
          wanted[1]['first_cover']['D']['owners'] == ['CD', 'DE', 'DF'],
          'contact bridge retained in complete first-cover')
    check({g['group'] for g in project(r2, 'first_cover_lca')} == {'AB', 'EF', 'ABCDEF'},
          'LCA2 loses both triangles')
    check({g['group'] for g in project(r2, 'core')} == {'ABCDEF'}, 'core2 only root')
    check(all(e.level == 2 for e in r2.core), 'core2 date')
    check(all(e.level == Fraction(2, 3) and len(e.nodes) == 1 for e in r3.cover), 'unambiguous cover3')
    check({g['group'] for g in project(r3, 'first_cover_lca')} == {'ABC', 'DEF', 'ABCDEF'},
          'LCA3 saves triangles')
    check(all(e.level == 2 for e in r3.core), 'core3 date')
    transforms = [
        ('translation', 1, lambda p: p),
        ('axes', 1, lambda p: (p[1], p[2], p[0])),
        ('reflection', 1, lambda p: (6 - p[0], p[1], p[2])),
        ('central_symmetry', 1, lambda p: (5 - p[0], 5 - p[1], 4 - p[2])),
        ('similarity3', 9, lambda p: tuple(7 + 3 * x for x in p)),
    ]
    all_permutations = list(permutations(range(6)))
    selected = [all_permutations[i] for i in (0, 1, 7, 23, 119, 120, 359, 360, 480, 600, 718, 719)]
    cases = []
    for name, square, transform in transforms:
        for ordering in selected:
            points = [transform(POINTS[i]) for i in ordering]
            check(all(0 <= x < 2 ** 18 for p in points for x in p), 'admissible coordinates')
            a, b = Definition(points), Reference(points, 3)
            previous = None
            for k in (1, 2, 3):
                ra, rb = a.order(k), b.order(k)
                check(not judge.compare_orders(ra, rb), 'A/B difference')
                check(judge.coherence(ra, 6, previous) is None, 'coherence')
                previous = ra
                check(fingerprint(ra, ordering, square) == wanted[k - 1], 'similarity/permutation semantic difference')
            cases.append(dict(transform=name, ordering=ordering))
    output = dict(scope='Exact integer plateau, not the exact median-axis drawing of the thesis; no native/G4',
                  original=ORIGINAL, admissible_points=POINTS, distances_squared=distances,
                  baseline=wanted, cases=cases, checks=CHECKS)
    print(json.dumps(output, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
