#!/usr/bin/env python3
"""E1 : cibles de l'utilisateur (A8) et fixtures de reference, pour core, cover et la famille H_kappa^(m).

Pour chaque fixture : dates d'entree (rayon) par site et par regle, blocs a des rayons de controle, verdicts.
Regles : core ; cover ; H1sq (regle du developpeur, kappa 1, m 1) ; H1rad (meme regle en rayon) ;
Hk1sq / Hk1rad (m = k + 1, regle retenue par le developpeur) ; Hmcs (m = max(k + 1, mcs), choix H5) ;
H2rad (kappa 2, m 1) ; Hk1rad2 (kappa 2, m = k + 1).

    python3 -B e1_cibles.py > recus_e1_cibles.json
"""
from decimal import Decimal
from fractions import Fraction
import json
import sys
import time

import hk

FIXTURES = {
    'T0_equilateral_exact': dict(
        k=2, names='ABCDEF',
        points=[(1, 1, 2), (1, 2, 1), (2, 2, 2), (3, 3, 2), (4, 4, 2), (4, 3, 3)]),
    'Q1_T1_1700': dict(
        k=2, names='ABCDEF',
        points=[(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (3700, 2000, 0), (5432, 3000, 0), (5432, 1000, 0)]),
    'Q2_S17': dict(
        k=2, names=['x', 'a', 'b1', 'b2'],
        points=[(1000, 1000, 1000), (1100, 1000, 1000), (1010, 1120, 1000), (1010, 1119, 1016)]),
    'Q3_filament': dict(
        k=2, names=['x', 'f1', 'f2', 'f3', 'f4', 'f5', 'c0', 'c1', 'c2', 'c3', 'c4', 'c5', 'c6', 'c7'],
        points=[(10000 + dx, 10000 + dy, 10000 + dz) for dx, dy, dz in (
            (0, 0, 0), (700, 3, 0), (1401, -2, 0), (2100, 4, 0), (2802, 0, 0), (3500, -3, 0),
            (-900, 0, 0), (-880, 200, 0), (-880, -200, 0), (-880, 0, 200), (-880, 0, -200), (-1100, 0, 0),
            (-1080, 150, 100), (-1080, -150, -100))]),
    'Q4_T6_K3': dict(
        k=3, names=['C', 'P', 'Q', 'R', 'm', 'D', 'P2', 'Q2', 'R2'],
        points=[(3000, 3000, 3000), (4000, 4000, 3000), (4000, 3000, 4000), (3000, 4000, 4000),
                (2600, 2600, 2600), (2200, 2200, 2200), (1200, 1200, 2200), (1200, 2200, 1200),
                (2200, 1200, 1200)]),
    'FIVE_auditeur': dict(
        k=2, names=['0', '1', '2', '3', '4'],
        points=[(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]),
    'LINE_024': dict(k=2, names=['l', 'x', 'r'], points=[(0, 0, 0), (2, 0, 0), (4, 0, 0)]),
    'LINE_025': dict(k=2, names=['l', 'x', 'r'], points=[(0, 0, 0), (2, 0, 0), (5, 0, 0)]),
}

# rayons de controle (Fraction) et mcs a juger, par fixture
CHECKS = {
    'T0_equilateral_exact': [Fraction(2, 1) * Fraction(1), Fraction(115, 100), Fraction(16, 10), Fraction(19, 10)],
    'Q1_T1_1700': [Fraction(1155), Fraction(1400), Fraction(1600), Fraction(1787)],
    'Q2_S17': [Fraction(61), Fraction(65), Fraction(70), Fraction(75)],
    'Q3_filament': [Fraction(705), Fraction(750), Fraction(790)],
    'Q4_T6_K3': [Fraction(86603, 100), Fraction(900), Fraction(1000), Fraction(1078), Fraction(1200),
                 Fraction(1334)],
    'FIVE_auditeur': [Fraction(4), Fraction(5), Fraction(59, 10)],
    'LINE_024': [Fraction(1), Fraction(19, 10), Fraction(2)],
    'LINE_025': [Fraction(1), Fraction(19, 10), Fraction(2), Fraction(22, 10)],
}
MCS = {'T0_equilateral_exact': [2, 3], 'Q1_T1_1700': [2, 3], 'Q2_S17': [2], 'Q3_filament': [2, 3, 6, 9],
       'Q4_T6_K3': [2, 3], 'FIVE_auditeur': [2, 3], 'LINE_024': [2], 'LINE_025': [2]}


def rules_for(res, n, k, mcs_list):
    ref, tree_ref_sq = hk.rule_reference(res, n, 1, 'sq')
    out = {}
    out['core'] = ('sq', ref['core'], tree_ref_sq)
    out['cover'] = ('sq', ref['cover'], tree_ref_sq)
    for name, kappa, m, scale in (('H1sq', 1, 1, 'sq'), ('H1rad', 1, 1, 'rad'), ('Hk1sq', 1, k + 1, 'sq'),
                                  ('Hk1rad', 1, k + 1, 'rad'), ('H2rad', 2, 1, 'rad'), ('Hk1rad2', 2, k + 1, 'rad')):
        if m > n:
            continue
        ent, tree = hk.rule_H(res, n, kappa, m, scale)
        out[name] = (scale, ent, tree)
    for mcs in mcs_list:
        m = max(k + 1, mcs)
        if m > n or m == k + 1:
            continue
        ent, tree = hk.rule_H(res, n, 1, m, 'sq')
        out['Hmcs%d_sq' % mcs] = ('sq', ent, tree)
    return out


def radius_of(scale, x):
    if x is None:
        return None
    return float(hk.dsqrt(x)) if scale == 'sq' else float(x)


def blocks_r(scale, u, r):
    level = r * r if scale == 'sq' else Decimal(r.numerator) / Decimal(r.denominator)
    sc = hk.Scale(scale)
    return hk.blocks_at(u, level, sc.le)


def named(blocks, names):
    return [''.join(names[i]) if isinstance(names, str) else '{' + ','.join(names[i] for i in b) + '}'
            for b in blocks]


def label(b, names):
    if isinstance(names, str):
        return ''.join(names[i] for i in b)
    return '{' + ','.join(names[i] for i in b) + '}'


def timeline(scale, u, names):
    """Suite des blocs (taille >= 2) a chaque rayon d'evenement de u."""
    n = len(u)
    levels = sorted(set(u[i][j] for i in range(n) for j in range(n) if u[i][j] is not None))
    sc = hk.Scale(scale)
    out = []
    for lv in levels:
        bl = [b for b in hk.blocks_at(u, lv, sc.le) if len(b) >= 2]
        out.append((radius_of(scale, lv), [label(b, names) for b in bl]))
    return out


def verdicts(fx, names, scale, u, root_r):
    """Verdicts par cible (lecture structurelle et lecture stricte), selon la fixture."""
    idx = {nm: i for i, nm in enumerate(names)}
    v = {}

    def bl(r, mcs):
        return [set(b) for b in blocks_r(scale, u, r) if len(b) >= mcs]

    if fx in ('T0_equilateral_exact', 'Q1_T1_1700'):
        ABC = set(idx[c] for c in 'ABC')
        DEF = set(idx[c] for c in 'DEF')
        # juste avant la fusion : dernier rayon d'evenement strictement sous la racine
        lv = sorted(set(u[i][j] for i in range(len(u)) for j in range(len(u)) if u[i][j] is not None))
        below = [x for x in lv if radius_of(scale, x) < root_r - 1e-9]
        last = below[-1] if below else None
        sc = hk.Scale(scale)
        for mcs in (2, 3):
            got = [set(b) for b in hk.blocks_at(u, last, sc.le) if len(b) >= mcs] if last is not None else []
            v['ABC|DEF_avant_fusion_mcs%d' % mcs] = sorted(map(sorted, got)) == sorted(map(sorted, [ABC, DEF]))
    if fx == 'Q2_S17':
        XA = {idx['x'], idx['a']}
        BB = {idx['b1'], idx['b2']}
        for r in (Fraction(61), Fraction(75)):
            got = bl(r, 2)
            v['x_avec_a_et_b1b2_a_%s' % r] = sorted(map(sorted, got)) == sorted(map(sorted, [XA, BB]))
        got = bl(Fraction(70), 2)
        v['x_avec_b1b2_a_70'] = any({idx['x'], idx['b1'], idx['b2']} <= b for b in got)
    if fx == 'Q3_filament':
        x, f1, c0 = idx['x'], idx['f1'], idx['c0']
        for r in (Fraction(705), Fraction(790)):
            got = bl(r, 2)
            v['x_avec_filament_a_%s' % r] = any({x, f1} <= b for b in got)
            v['x_avec_amas_a_%s' % r] = any({x, c0} <= b for b in got)
            v['aucun_cluster_mcs9_a_%s' % r] = len(bl(r, 9)) == 0
    if fx == 'Q4_T6_K3':
        C, m, D, P, P2 = idx['C'], idx['m'], idx['D'], idx['P'], idx['P2']
        got = bl(Fraction(86603, 100), 2)
        v['C_hors_tetra_a_866'] = not any({C, P} <= b for b in got) and not any({D, P2} <= b for b in got)
        v['PQR_et_P2Q2R2_a_866'] = any({idx['P'], idx['Q'], idx['R']} <= b and C not in b for b in got) and \
            any({idx['P2'], idx['Q2'], idx['R2']} <= b and D not in b for b in got)
        chain = False
        for r in (Fraction(1000), Fraction(1078), Fraction(1200), Fraction(1334)):
            got = bl(r, 2)
            if any({C, m, D} <= b and P not in b and P2 not in b for b in got):
                chain = True
        v['CmD_avant_racine'] = chain
        got = bl(Fraction(1334), 2)
        v['C_avec_tetra_a_1334'] = any({C, P} <= b for b in got)
        got = bl(Fraction(86603, 100), 2)
        v['CmD_pas_cluster_a_866'] = not any({C, m, D} <= b for b in got)
    return v


def main():
    t0 = time.time()
    report = {}
    for fx, spec in FIXTURES.items():
        pts, k, names = spec['points'], spec['k'], spec['names']
        n = len(pts)
        t1 = time.time()
        res = hk.oracle(pts, k)
        root = max(node.level for node in res.nodes)
        root_r = float(hk.dsqrt(root))
        rep = dict(k=k, n=n, root_level=str(root), root_radius=root_r, rules={})
        rules = rules_for(res, n, k, MCS[fx])
        for mm in sorted(set([2, k + 1])):
            if mm <= n:
                rules['closure_m%d' % mm] = ('sq', None, hk.rule_closure(res, n, mm))
        for rname, (scale, ent, tree) in rules.items():
            if ent is None:
                u = tree
                ent = [(u[i][i], None) if u[i][i] is not None else None for i in range(n)]
            else:
                u = hk.ultrametric(ent, tree)
            entries = {}
            for i, nm in enumerate(names):
                if ent[i] is None:
                    entries[nm] = None
                else:
                    entries[nm] = round(radius_of(scale, ent[i][0]), 6)
            checks = {}
            for r in CHECKS[fx]:
                checks[str(float(r))] = [label(b, names) for b in blocks_r(scale, u, r) if len(b) >= 2]
            rep['rules'][rname] = dict(scale=scale, entry_radius=entries, blocks_at=checks,
                                       timeline=timeline(scale, u, names),
                                       verdicts=verdicts(fx, names, scale, u, root_r))
        # D_k (coeur) en rayon pour comparaison
        rep['core_radius'] = {nm: round(float(hk.dsqrt(res.core[i].level)), 6) for i, nm in enumerate(names)}
        rep['first_cover_radius'] = {nm: round(float(hk.dsqrt(res.cover[i].level)), 6) for i, nm in enumerate(names)}
        rep['seconds'] = round(time.time() - t1, 2)
        report[fx] = rep
    report['_near_ties_rad'] = hk.NEAR_TIES[0]
    report['_seconds_total'] = round(time.time() - t0, 1)
    print(json.dumps(report, indent=1, default=str))


if __name__ == '__main__':
    main()
