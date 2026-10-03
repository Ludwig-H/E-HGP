#!/usr/bin/env python3
"""C2 : contre-familles du rapport, recalculees par vfull.

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B c2_families.py
"""
import json
import sys
from fractions import Fraction

sys.dont_write_bytecode = True
import mpmath  # noqa: E402
import vfull as V  # noqa: E402


def f(x):
    return float(x)


def main():
    rec = {}
    # (a) rival lointain {-2L, 0, 2}, K = 2, site 0 ; puis a deplace en 3
    fam = []
    for L in (10, 100, 1000, 10000):
        full = V.Full(V.Cloud([(-2 * L, 0, 0), (0, 0, 0), (2, 0, 0)]), 2)
        full2 = V.Full(V.Cloud([(-2 * L, 0, 0), (0, 0, 0), (3, 0, 0)]), 2)
        q, q2 = V.rule_Q(full, 1, 1), V.rule_Q(full2, 1, 1)
        p, p2 = V.rule_P(full, 1, 1), V.rule_P(full2, 1, 1)
        core = V.rule_core(full)
        # pire ecart des hauteurs u (rayon) sur toutes les paires, Q1 et P1
        dq = max(abs(q.u(i, j) - q2.u(i, j)) for i in range(3) for j in range(i, 3))
        dp = max(abs(p.u(i, j) - p2.u(i, j)) for i in range(3) for j in range(i, 3))
        fam.append({'L': L, 'Q1_date_site0': f(q.e[1]), 'Q1_level_site0': str(q.e2[1]),
                    'Q1_date_moved': f(q2.e[1]), 'Q1_level_moved': str(q2.e2[1]),
                    'Q1_delta_date': f(q2.e[1] - q.e[1]), 'Q1_max_delta_u': f(dq),
                    'P1_date_site0': f(p.e[1]), 'P1_date_moved': f(p2.e[1]), 'P1_max_delta_u': f(dp),
                    'core_level_site0': str(core.e2[1]), 'ratio_level_Q1_over_core': f(q.e2[1] / core.e2[1])})
    rec['rival_lointain'] = fam
    # (b) {0,10,20,30}, K = 3
    full = V.Full(V.Cloud([(0, 0, 0), (10, 0, 0), (20, 0, 0), (30, 0, 0)]), 3)
    rec['0_10_20_30_K3'] = {}
    for kap in (1, 2, 4, 1000):
        p = V.rule_P(full, kap, 1)
        rec['0_10_20_30_K3']['P%d' % kap] = [f(v) for v in p.e]
    rec['0_10_20_30_K3']['dk_radius'] = [f(V.R(full.cloud.dk2(x, 3))) for x in range(4)]
    rec['0_10_20_30_K3']['alpha'] = [f(V.R(min(a for a, _ in full.entries(x, 1)))) for x in range(4)]
    # (c) FX-A9 {0,2,7,10,13}, K = 3, blocs a r = 5
    full = V.Full(V.Cloud([(0, 0, 0), (2, 0, 0), (7, 0, 0), (10, 0, 0), (13, 0, 0)]), 3)
    lab = ['0', '2', '7', '10', '13']
    out = {}
    for name, rule in (('core', V.rule_core(full)), ('P1', V.rule_P(full, 1, 1)), ('P2', V.rule_P(full, 2, 1)),
                       ('cover', V.rule_lca(full, 1, 'cover'))):
        out[name] = {'dates': [f(v) for v in rule.e],
                     'blocks_r5': [[lab[i] for i in sorted(b)] for b in rule.blocks(mpmath.mpf(5))]}
    rec['FX_A9_K3'] = out
    # (d) point median {0,2,5}, K = 2
    full = V.Full(V.Cloud([(0, 0, 0), (2, 0, 0), (5, 0, 0)]), 2)
    q, p, core = V.rule_Q(full, 1, 1), V.rule_P(full, 1, 1), V.rule_core(full)
    rec['median_0_2_5'] = {'Q1_site2': f(q.e[1]), 'Q1_level': str(q.e2[1]), 'P1_site2': f(p.e[1]),
                           'core_level': str(core.e2[1])}
    # (e) paire d'anticipation (theoreme B)
    ant = {}
    for tag, b in (('b=(-20,0,0)', (-20, 0, 0)), ('b=(-16,12,0)', (-16, 12, 0)), ('b=(20,0,0)', (20, 0, 0)),
                   ('b=(-2.2,..) delta=0.1 x10', None)):
        if b is None:
            pts = [(0, 0, 0), (20, 0, 0), (-22, 0, 0)]  # X_delta a l'echelle 10, delta = 0.1
        else:
            pts = [(0, 0, 0), (2, 0, 0), b]
        full = V.Full(V.Cloud(pts), 2)
        events = sorted(set(str(L) for L in full.level))
        cov = {}
        for x in range(3):
            cov[x] = sorted((str(a), F) for a, F in full.entries(x, 1))
        ant[tag] = {'P1_x': f(V.rule_P(full, 1, 1).e[0]), 'P2_x': f(V.rule_P(full, 2, 1).e[0]),
                    'Q1_x': f(V.rule_Q(full, 1, 1).e[0]), 'cover_x': f(V.rule_lca(full, 1, 'c').e[0]),
                    'levels': events, 'entries': {str(x): [[a, list(F)] for a, F in cov[x]] for x in cov}}
    rec['anticipation'] = ant
    with open('recus_c2_families.json', 'w') as fh:
        json.dump(rec, fh, indent=1)
    print(json.dumps(rec, indent=1))


if __name__ == '__main__':
    main()
