#!/usr/bin/env python3
"""Comparaison exacte des regles sur les fixtures : deux triangles (T0, quatre bases + EQUILATERAL v11), cinq points,
{0, 2s, 4s} et sa perturbation, Q1 (T1_1700), Q2 (S17), Q3 (filament, 14 sites, K = 2 : 455 boules minimales),
Q4 (T6, K = 3).

Regles : core, cover (oracle du developpeur, points_reference), H_1, H_{k+1} (ecrits ici, recoupes), ER0h(1, 12),
ER0(1, 12), VOTE[p, faces, kappa] (vote de la these rendu hierarchique, ecrit ici).

Usage : python3 -B comparaison.py > recus/comparaison.json
"""
from fractions import Fraction
import json
import sys
import time

sys.dont_write_bytecode = True
import arbre  # noqa: E402
import fixtures as FX  # noqa: E402
import regles as RG  # noqa: E402
from surd import Surd  # noqa: E402
import points_reference as pr  # noqa: E402


def hang_from_pr(entries):
    return [(Surd.sqrt(e), o, {}) for e, o in entries]


def rules_for(points, k, names, with_vote=True):
    n = len(points)
    d, res, tree = arbre.v11_tree(points, k)
    ref1, _t = pr.reference_rules(res, n, 1)
    out = {}
    out['core'] = hang_from_pr(ref1['core'])
    out['cover'] = hang_from_pr(ref1['cover'])
    out['H_1'] = RG.rule_hm(tree, 1)
    if k + 1 <= n:
        out['H_k+1'] = RG.rule_hm(tree, k + 1)
    if k >= 2:
        out['ER0h(1,12)'] = RG.rule_er0h(tree, 1, 12)
        out['ER0(1,12)'] = RG.rule_er0(tree, 1, 12)
    if with_vote:
        for p in (0, 2):
            out['VOTE[p=%d,toutes]' % p] = RG.rule_vote(d, tree, k, p, 'all', None)
            out['VOTE[p=%d,toutes,k12]' % p] = RG.rule_vote(d, tree, k, p, 'all', Fraction(12))
        out['VOTE[p=2,gabriel,k12]'] = RG.rule_vote(d, tree, k, 2, 'gabriel', Fraction(12))
    table = {}
    for rule, hang in out.items():
        u = RG.ultrametric(tree, hang)
        table[rule] = dict(hierarchie=RG.hierarchy_text(u, names, 2),
                           entrees=dict((names[i], round(float(hang[i][0]), 3)) for i in range(n)),
                           racine=round(float(Surd.sqrt(tree.birth[tree.root])), 3))
    return table, tree


def main():
    t0 = time.time()
    cases = []
    eq = dict(K=2, names=['A', 'B', 'C', 'D', 'E', 'F'], points=FX.EQUILATERAL)
    cases.append(('EQUILATERAL_v11', eq))
    for nm, fx in FX.t0_bases().items():
        cases.append((nm, fx))
    cases.append(('FIVE', dict(K=2, names=['0', '1', '2', '3', '4'], points=FX.FIVE)))
    s = 1000
    cases.append(('ligne_0_2s_4s', dict(K=2, names=['g', 'x', 'd'], points=[(0, 0, 0), (2 * s, 0, 0), (4 * s, 0, 0)])))
    cases.append(('ligne_0_2s_4s+1', dict(K=2, names=['g', 'x', 'd'],
                                          points=[(0, 0, 0), (2 * s, 0, 0), (4 * s + 1, 0, 0)])))
    cases.append(('Q1_T1_1700', FX.Q1))
    cases.append(('Q2_S17', FX.Q2))
    cases.append(('Q3_filament', FX.Q3))
    cases.append(('Q4_T6', FX.Q4))
    out = {}
    for nm, fx in cases:
        tab, _tree = rules_for(fx['points'], fx['K'], fx['names'])
        out[nm] = dict(K=fx['K'], regles=tab)
        sys.stderr.write('%s %.1fs\n' % (nm, time.time() - t0))
    out['_secondes'] = round(time.time() - t0, 1)
    print(json.dumps(out, indent=1, ensure_ascii=False))


if __name__ == '__main__':
    main()
