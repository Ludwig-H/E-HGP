#!/usr/bin/env python3
"""Fixtures exactes de l'etage condense (oracle de la definition, H^r_{k+1}, k = 2, m = 3).

Pour chaque nuage : dates par site (entree e, premiere couverture qualifiee t, marge D, resolution rho, coeur d_k,
coeur du noeud sB) ; puis, pour chaque critere d'existence et chaque mcs, la hierarchie condensee sur les fenetres
des cellules de l'utilisateur, et les sorties plates (EOM z = 1, 3, log ; feuilles), a cote de HDBSCAN
(scikit-learn, min_samples = k) et de HDBSCAN condense N-aire (meme etage que la tete).

Criteres (dates de comptage s_i ; un site est membre de son bloc des son entree, il ne compte pour l'existence
qu'a partir de s_i) :
  A  entres            s = e
  B  coeur du noeud    s = premier rayon >= e ou x_i est dans la composante (C n X) du noeud de son bloc
  C  resolus           s = max(e, rencontres des rivaux qualifies) (rho)
  E_th maturite        s = (1 - th) e + th d_k  (forme interpolee de la v10), th dans {1/4, 1/2, 3/4, 1}
  D  couverts          (etage FULL) bloc non vide et amas discret du noeud >= mcs : rapporte a part.

    python3 fixtures_existence.py > ../sorties/fixtures_existence.txt
"""
from fractions import Fraction
import json
import sys

import modele_lib as ml
from modele_lib import Rad, rcmp

EQUILATERAL = [tuple(c + 2 for c in p) for p in ((-1, -1, 0), (-1, 0, -1), (0, 0, 0), (1, 1, 0), (2, 2, 0),
                                                  (2, 1, 1))]
ABCDEF = ['A', 'B', 'C', 'D', 'E', 'F']
P1 = [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (4000, 2000, 0), (5732, 3000, 0), (5732, 1000, 0)]
P2 = [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (3998, 2000, 0), (5730, 3000, 0), (5730, 1000, 0)]
S_PLAN = [(400, 3200, 0), (400, 800, 0), (2000, 2000, 0), (4000, 2000, 0), (5600, 3200, 0), (5600, 800, 0)]
S_3D = [(3000, 1000, 2000), (3000, 2000, 1000), (2000, 2000, 2000), (2000, 3000, 3000), (1000, 4000, 3000),
        (1000, 3000, 4000)]


def T1(L):
    return [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (2000 + L, 2000, 0), (3732 + L, 3000, 0),
            (3732 + L, 1000, 0)]


FIVE = [(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]
EIGHT = [(20, 20, 0), (30, 20, 0), (40, 20, 0), (50, 20, 0), (64, 20, 0), (74, 20, 0), (0, 30, 0), (0, 10, 0)]
EIGHT_NAMES = ['x', 'y', 's2', 's3', 'b1', 'b2', 'w1', 'w2']
O = (10000, 10000, 10000)
FIL = {'x': (0, 0, 0), 'f1': (700, 3, 0), 'f2': (1401, -2, 0), 'f3': (2100, 4, 0), 'f4': (2802, 0, 0),
       'f5': (3500, -3, 0), 'c0': (-900, 0, 0), 'c1': (-880, 200, 0), 'c2': (-880, -200, 0), 'c3': (-880, 0, 200),
       'c4': (-880, 0, -200), 'c5': (-1100, 0, 0), 'c6': (-1080, 150, 100), 'c7': (-1080, -150, -100)}
Q3_NAMES = list(FIL)


def q3(dep=None):
    out = []
    for nm in Q3_NAMES:
        p = [a + b for a, b in zip(O, FIL[nm])]
        if dep and nm in dep:
            p = [a + b for a, b in zip(p, dep[nm])]
        out.append(tuple(p))
    return out


R = Rad.rat


def sq(x):
    return Rad.sqrt(Fraction(x))


def criteria(ph):
    out = [('A', list(ph.e)), ('B', list(ph.sB)), ('C', list(ph.rho))]
    for th in (Fraction(1, 4), Fraction(1, 2), Fraction(3, 4), Fraction(1)):
        out.append(('E%s' % th, ph.maturity(th)))
    return out


def window_states(tg, mcs, a, b):
    """Etats condenses (gros blocs, par masse comptee) aux rangs dont le rayon est dans [a, b] (coupe fermee), plus
    l'etat en a (dernier rang <= a). Rend la liste des ensembles de gros blocs distincts rencontres."""
    radii = tg.radii
    ranks = [r for r in range(len(radii)) if rcmp(radii[r], a) <= 0]
    start = ranks[-1] if ranks else None
    inside = [r for r in range(len(radii)) if rcmp(radii[r], a) > 0 and rcmp(radii[r], b) <= 0]
    seen = []
    for r in ([start] if start is not None else []) + inside:
        big = []
        for blk in tg.blocks_at(r):
            mass = sum(1 for i in blk if tg.srank[i] <= r)
            if mass >= mcs:
                big.append(tuple(sorted(tg.names[i] for i in blk)))
        big = tuple(sorted(big))
        if big not in seen:
            seen.append(big)
    return seen


def window_states_D(ph, mcs, a, b):
    """Critere D (etage FULL) sur une fenetre : bloc non vide dont le noeud couvre au moins mcs sites."""
    tg = ph.treegram()
    radii, _ = ml.sort_unique(list(tg.radii) + list(ph.birth) + [a, b])
    pts = [r for r in radii if rcmp(r, a) >= 0 and rcmp(r, b) <= 0]
    seen = []
    for rad in pts:
        ranks = [r for r in range(len(tg.radii)) if rcmp(tg.radii[r], rad) <= 0]
        big = []
        if ranks:
            for blk in tg.blocks_at(ranks[-1]):
                i = next(iter(blk))
                node = ph.alive_at(ph.owner[i], rad)
                if len(ph.covered_at(node, rad)) >= mcs:
                    big.append(tuple(sorted(tg.names[j] for j in blk)))
        big = tuple(sorted(big))
        if big not in seen:
            seen.append(big)
    return seen


def flat(tg, mcs, zs=(1, 3, 'log', 'leaves')):
    clusters, viol = ml.condensed_tree(tg, mcs)
    res = {}
    for z in zs:
        sel, forced = ml.eom_select(tg, clusters, z)
        lab = ml.labels_from_selection(tg, clusters, sel)
        res[str(z)] = dict(partition=ml.partition_of(lab, tg.names), non_tranche=forced)
    return res, len(viol)


def hdbscan_flat(points, names, k, mcs):
    lab = ml.hdbscan_labels(points, k, mcs)
    U = ml.hdbscan_ultrametric(points, k)
    tg = ml.Treegram(len(points), U, [U[i][i] for i in range(len(points))], names=names)
    nary, _ = flat(tg, mcs, zs=(1, 3))
    return dict(sklearn=ml.partition_of(lab, names), nary_z1=nary['1']['partition'], nary_z3=nary['3']['partition'])


def describe_sites(ph, out):
    for i in range(ph.n):
        out.append('  %-3s e=%9.4f t=%9.4f D=%8.4f rho=%9.4f d_k=%9.4f sB=%9.4f  e=%s' % (
            ph.names[i], ph.e[i].approx(), ph.t[i].approx(), ph.D[i].approx(), ph.rho[i].approx(),
            ph.d[i].approx(), ph.sB[i].approx(), ph.e[i]))


def run_fixture(name, points, names, windows, mcs_list, out, js, k=2):
    ph = ml.PointHierarchy(points, k, names=names)
    out.append('=' * 100)
    out.append('%s  (k = %d, m = %d, n = %d)' % (name, ph.k, ph.m, ph.n))
    out.append('  naissances FULL (rayon) : %s' % sorted(set(round(b.approx(), 4) for b in ph.birth)))
    describe_sites(ph, out)
    crit = criteria(ph)
    entry = dict(name=name, sites={ph.names[i]: dict(e=str(ph.e[i]), e_approx=ph.e[i].approx(),
                                                      rho=ph.rho[i].approx(), d_k=ph.d[i].approx(),
                                                      sB=ph.sB[i].approx(), D=ph.D[i].approx())
                                   for i in range(ph.n)}, windows={}, flat={}, hdbscan={})
    for (label, a, b, mcs_set, target) in windows:
        for mcs in mcs_set:
            line = '  fenetre %s [%.4f ; %.4f] mcs=%d cible=%s :' % (label, a.approx(), b.approx(), mcs, target)
            out.append(line)
            for cname, dates in crit:
                tg = ml.Treegram(ph.n, [[ph.u(i, j) for j in range(ph.n)] for i in range(ph.n)], dates,
                                 names=ph.names)
                states = window_states(tg, mcs, a, b)
                ok = all(list(s) == target for s in states) if target is not None else None
                out.append('     %-6s %s %s' % (cname, 'CONFORME' if ok else 'non conforme', states))
                entry['windows'].setdefault('%s_mcs%d' % (label, mcs), {})[cname] = dict(
                    states=[list(s) for s in states], ok=ok)
            states = window_states_D(ph, mcs, a, b)
            ok = all(list(s) == target for s in states) if target is not None else None
            out.append('     %-6s %s %s' % ('D', 'CONFORME' if ok else 'non conforme', states))
            entry['windows'].setdefault('%s_mcs%d' % (label, mcs), {})['D'] = dict(
                states=[list(s) for s in states], ok=ok)
    for mcs in mcs_list:
        for cname, dates in crit[:3]:
            tg = ml.Treegram(ph.n, [[ph.u(i, j) for j in range(ph.n)] for i in range(ph.n)], dates, names=ph.names)
            res, nviol = flat(tg, mcs)
            out.append('  plat mcs=%d %s : %s%s' % (mcs, cname, {z: v['partition'] for z, v in res.items()},
                                                    ' VIOLATIONS=%d' % nviol if nviol else ''))
            entry['flat']['%s_mcs%d' % (cname, mcs)] = res
        hd = hdbscan_flat(points, names, k, mcs)
        out.append('  plat mcs=%d HDBSCAN sklearn : %s ; N-aire z1 : %s ; N-aire z3 : %s' % (
            mcs, hd['sklearn'], hd['nary_z1'], hd['nary_z3']))
        entry['hdbscan']['mcs%d' % mcs] = hd
    js.append(entry)
    return ph


def covered_criterion_demo(ph, mcs, out):
    """Critere D (etage FULL) : bloc non vide dont le noeud couvre >= mcs sites ; rapporte les amas de moins de mcs
    membres qu'il fabrique."""
    tg = ph.treegram()
    bad = []
    radii, _ = ml.sort_unique(list(tg.radii) + list(ph.birth))
    for rad in radii:
        # etat des blocs au rayon rad : dernier rang du treegramme <= rad
        ranks = [r for r in range(len(tg.radii)) if rcmp(tg.radii[r], rad) <= 0]
        if not ranks:
            continue
        for blk in tg.blocks_at(ranks[-1]):
            i = next(iter(blk))
            node = ph.alive_at(ph.owner[i], rad)
            cov = ph.covered_at(node, rad)
            if len(cov) >= mcs and len(blk) < mcs:
                bad.append((round(rad.approx(), 4), sorted(tg.names[j] for j in blk), len(cov)))
    out.append('  critere D (couverts >= %d) : blocs de moins de %d membres declares amas : %d %s' % (
        mcs, mcs, len(bad), bad[:4]))
    return bad


def main():
    out, js = [], []
    # T0 exact (oracle) : triangles a s / sqrt 3, racine a 1,2247 ; fenetre [1,3 r0 ; 1,7 r0], r0 = sqrt(2)/2
    r0 = sq(Fraction(1, 2))
    w = [('T0', r0.scale(Fraction(13, 10)), r0.scale(Fraction(17, 10)), (2, 3),
          [('A', 'B', 'C'), ('D', 'E', 'F')]),
         ('T0_forcee', r0.scale(Fraction(13, 10)), r0.scale(Fraction(17, 10)), (4,), [])]
    run_fixture('T0 equilateral exact (plan x - y - z = 0)', EQUILATERAL, ABCDEF, w, (2, 3, 4), out, js)
    for nm, pts in (('T0_P1', P1), ('T0_P2', P2), ('T0_S_plan', S_PLAN)):
        w = [('T0', R(1300), R(1700), (2, 3), [('A', 'B', 'C'), ('D', 'E', 'F')])]
        run_fixture(nm, pts, ABCDEF, w, (3,), out, js)
    w = [('T0', sq(845000), sq(1445000), (2, 3), [('A', 'B', 'C'), ('D', 'E', 'F')])]
    run_fixture('T0_S_3D', S_3D, ABCDEF, w, (3,), out, js)
    # Q1 : pont de 1700 ; mcs 3 ancree sur [19/20 f ; f[ (f = 1787,360) ; mcs 2 (Q1bis) triangles
    ph = ml.PointHierarchy(T1(1700), 2, names=ABCDEF)
    f = max((b for b in ph.birth), key=lambda v: v.approx())
    lo = f.scale(Fraction(19, 20))
    eps = Fraction(1, 10 ** 6)
    w = [('Q1', lo, f - R(eps), (2, 3), [('A', 'B', 'C'), ('D', 'E', 'F')])]
    run_fixture('Q1_T1_1700', T1(1700), ABCDEF, w, (2, 3), out, js)
    # cinq points de l'auditeur
    w = [('cinq', sq(35), sq(35), (2,), [('1', '2'), ('3', '4')])]
    run_fixture('cinq_points', FIVE, ['0', '1', '2', '3', '4'], w, (2, 3), out, js)
    # Q3 (filament contre amas) et ses variantes : Q3 (a) mcs 2-6 ; Pi2 derivee mcs 7-8 ; Q-Pi2 mcs 9
    amas = tuple(['c%d' % j for j in range(8)])
    target_a = sorted([tuple(sorted(['x', 'f1', 'f2', 'f3', 'f4', 'f5'])), tuple(sorted(amas))])
    target_78 = [tuple(sorted(list(amas) + ['x']))]
    for vname, dep in (('base', None), ('x_vers_amas', {'x': (-1, 0, 0)}), ('x_vers_filament', {'x': (1, 0, 0)}),
                       ('c0_vers_x', {'c0': (1, 0, 0)}), ('f1_vers_x', {'f1': (-1, 0, 0)})):
        w = [('Q3a', R(705), R(790), (2, 6), target_a), ('Q3_Pi2', R(705), R(790), (7, 8), target_78),
             ('Q-Pi2', R(705), R(790), (9,), [])]
        ph = run_fixture('Q3_%s' % vname, q3(dep), Q3_NAMES, w, (3, 6, 9) if vname == 'base' else (9,), out, js)
        if vname == 'base':
            covered_criterion_demo(ph, 9, out)
    # R1 (huit sites de l'auditeur) : x coeur, fusion parasite F = 12
    run_fixture('R1_huit_sites', EIGHT, EIGHT_NAMES, [], (2, 3, 4), out, js)
    print('\n'.join(out))
    with open('../sorties/fixtures_existence.json', 'w') as fh:
        json.dump(js, fh, indent=1, default=str)


if __name__ == '__main__':
    sys.setrecursionlimit(10000)
    main()
