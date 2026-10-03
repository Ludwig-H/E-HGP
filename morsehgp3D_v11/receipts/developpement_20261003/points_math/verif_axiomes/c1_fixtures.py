#!/usr/bin/env python3
"""C1 : fixtures et cellules du rapport `axiomes`, recalculees par vfull (independant).

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B c1_fixtures.py
Sortie : recus_c1_fixtures.json (lignes de temps des blocs d'au moins deux sites, dates par site).
"""
import json
import sys
from fractions import Fraction

sys.dont_write_bytecode = True
import mpmath  # noqa: E402
import vfull as V  # noqa: E402

O = (10000, 10000, 10000)


def off(d):
    return tuple(a + b for a, b in zip(O, d))


FIX = {
    'T0': (2, [(1, 1, 2), (1, 2, 1), (2, 2, 2), (3, 3, 2), (4, 4, 2), (4, 3, 3)], 'A B C D E F'),
    'Q1': (2, [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (3700, 2000, 0), (5432, 3000, 0), (5432, 1000, 0)],
           'A B C D E F'),
    'Q2': (2, [(1000, 1000, 1000), (1100, 1000, 1000), (1010, 1120, 1000), (1010, 1119, 1016)], 'x a b1 b2'),
    'Q3': (2, [O, off((700, 3, 0)), off((1401, -2, 0)), off((2100, 4, 0)), off((2802, 0, 0)), off((3500, -3, 0)),
               off((-900, 0, 0)), off((-880, 200, 0)), off((-880, -200, 0)), off((-880, 0, 200)), off((-880, 0, -200)),
               off((-1100, 0, 0)), off((-1080, 150, 100)), off((-1080, -150, -100))],
           'x f1 f2 f3 f4 f5 c0 c1 c2 c3 c4 c5 c6 c7'),
    'Q4': (3, [(3000, 3000, 3000), (4000, 4000, 3000), (4000, 3000, 4000), (3000, 4000, 4000), (2600, 2600, 2600),
               (2200, 2200, 2200), (1200, 1200, 2200), (1200, 2200, 1200), (2200, 1200, 1200)],
           'C P Q R m D P2 Q2 R2'),
}


def names_of(blocks, names):
    return [' '.join(names[i] for i in sorted(b)) for b in blocks if len(b) >= 2]


def timeline(rule, names, n):
    vals = []
    for i in range(n):
        for j in range(i, n):
            v = rule.u(i, j)
            if all(abs(v - w) > V.TOL for w in vals):
                vals.append(v)
    out = []
    prev = None
    for r in sorted(vals):
        b = names_of(rule.blocks(r), names)
        if b != prev:
            out.append((float(r), b))
            prev = b
    return out


def rules_for(full, k):
    out = {}
    for m in sorted(set([1, k + 1])):
        for kap in (1, 2):
            out['P%d_m%d' % (kap, m)] = V.rule_P(full, kap, m)
        out['Q1_m%d' % m] = V.rule_Q(full, 1, m)
    out['core'] = V.rule_core(full)
    out['cover'] = V.rule_lca(full, 1, 'cover')
    out['closure_m%d' % (k + 1)] = V.Closure(full, k + 1)
    return out


def main():
    rec = {}
    for name, (k, pts, labels) in FIX.items():
        names = labels.split()
        n = len(pts)
        full = V.Full(V.Cloud(pts), k)
        rr = rules_for(full, k)
        if name == 'Q3':
            rr['P1_m9'] = V.rule_P(full, 1, 9)
            rr['Q1_m9'] = V.rule_Q(full, 1, 9)
        if name == 'Q4':
            rr['P1_m3'] = V.rule_P(full, 1, 3)
        rec[name] = {}
        for rn, rule in rr.items():
            rec[name][rn] = {'dates': {names[i]: float(rule.e[i]) for i in range(n)},
                             'timeline': timeline(rule, names, n)}
        # niveaux internes de l'arbre de Kruskal (rayons) pour lecture
        rec[name]['_levels_radius'] = sorted(set(round(float(V.R(L)), 4) for L in full.level))
    with open('recus_c1_fixtures.json', 'w') as f:
        json.dump(rec, f, indent=1)
    for name in rec:
        print('=====', name)
        for rn, d in rec[name].items():
            if rn.startswith('_'):
                print('  levels', d[:40])
                continue
            print('  --', rn, {a: round(b, 4) for a, b in d['dates'].items()})
            for r, b in d['timeline']:
                print('      %.4f  %s' % (r, ' | '.join(b)))


if __name__ == '__main__':
    main()
