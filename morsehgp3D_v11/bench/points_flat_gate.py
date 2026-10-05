#!/usr/bin/env python3
"""Porte de la sortie plate v11 : tete certifiee (bench/points_flat.py) contre l'oracle des petits nuages
(bench/points_flat_oracle.py), fixtures gravees, mutants causaux, planchers (spec E1 du juge, § 4.3).

    python3 bench/points_flat_gate.py --export BUILD/mhgp11_points_export --work DIR --out FICHIER.json [--seconds 300]
    python3 bench/points_flat_gate.py --reference --work DIR --out FICHIER.json   # essai local, PAS une qualification

Cote banc : export natif (ou, avec --reference, l'ordre du banc construit sur les coupes de la definition), pendaison
H^r_{k+1} (points_radius, m = qualification(k) : 1 a k = 1), arbre de points, condensation, selection, labels ;
cote HDBSCAN : arbre du lien simple de sklearn (min_samples = k) relu exactement, meme tete. L'oracle recalcule tout
depuis la definition, avec sa propre arithmetique (parties sans facteur carre par factorisation), et enumere les
antichaines. Comparaison exacte des labels canoniques (plus petit PointId du cluster, -1 = bruit), pour k <= 4,
mcs 2, 3, 4, EOM z = 1, 2 et 3, feuilles, des deux cotes. Codes : 0 conforme ; 1 desaccord ; 2 refus d'entree ;
3 plancher non atteint, fixture fausse ou mutant non tue.
"""
import argparse
from fractions import Fraction
import itertools
import json
import os
from pathlib import Path
import random
import sys
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'reference'))
import points_flat as pf  # noqa: E402
import points_flat_oracle as po  # noqa: E402
import points_gate as pg  # noqa: E402
import points_radius as prad  # noqa: E402
import points_reference as pr  # noqa: E402
from hgp11_ref import Definition  # noqa: E402

LINES = (('eom', 1), ('eom', 2), ('eom', 3), ('leaf', 1))  # z = 2 : regle primaire du synthetique (dev)
MCS = (2, 3, 4)
MUTANTS = ('binarise', 'masse_finale', 'seuil_moins_un', 'flottant_seul', 'egalite_enfants', 'racine_admise',
           'sorties_brutes', 'niveau_carre', 'coupe_ouverte')
FLOORS = dict(clouds=150, comparisons=4000, equalities=5, fixtures=15, mutants=len(MUTANTS))
MUTANT_CLOUDS = 24

TRIANGLES = [(1, 1, 2), (1, 2, 1), (2, 2, 2), (3, 3, 2), (4, 4, 2), (4, 3, 3)]
FIVE = [(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]
NINE = [(x, 0, 0) for x in (0, 2, 4, 7, 9, 11, 17, 19, 21)]
# Meme schema, troisieme groupe plus proche : EOM z = 1 fusionne les deux premiers groupes, z = 2 et z = 3 les separent
# (oracle independant, k = 2, mcs 3) ; seule fixture ou z = 2 se distingue de z = 1.
NINE_B = [(x, 0, 0) for x in (0, 2, 4, 7, 9, 11, 16, 18, 20)]
F4_Z2 = [[0, 1, 2, 3, 4, 5], [6, 7, 8]]
TEN = [(1, 6, 0), (2, 3, 3), (2, 7, 5), (3, 5, 7), (4, 2, 5), (5, 6, 8), (7, 0, 7), (8, 3, 2), (8, 5, 6), (8, 5, 8)]
LINE1D = [(x, 0, 0) for x in (0, 3, 7, 16, 22, 27, 99, 107, 114)]
SIX = [(0, 0, 3), (0, 3, 0), (0, 5, 4), (0, 6, 5), (1, 0, 3), (2, 4, 2)]
MIXED = [(0, 0, 0), (10, 0, 0), (20, 2, 0), (5, 9, 0), (15, 9, 1), (10, 18, 0), (100, 0, 0), (103, 0, 0)]
EIGHT_B3 = pg.EIGHT + [(74, 30, 0)]
# T0 et ses variantes (build/v11-points-select/modele/scripts/fixtures_existence.py, recopie des coordonnees).
T0_VARIANTS = {
    'P1': [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (4000, 2000, 0), (5732, 3000, 0), (5732, 1000, 0)],
    'P2': [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (3998, 2000, 0), (5730, 3000, 0), (5730, 1000, 0)],
    'S_plan': [(400, 3200, 0), (400, 800, 0), (2000, 2000, 0), (4000, 2000, 0), (5600, 3200, 0), (5600, 800, 0)],
    'S_3D': [(3000, 1000, 2000), (3000, 2000, 1000), (2000, 2000, 2000), (2000, 3000, 3000), (1000, 4000, 3000),
             (1000, 3000, 4000)]}
# Q3 : FIL decale de (10 000, 10 000, 10 000) ; ordre x, f1..f5, c0..c7 (meme source).
FIL = [(0, 0, 0), (700, 3, 0), (1401, -2, 0), (2100, 4, 0), (2802, 0, 0), (3500, -3, 0), (-900, 0, 0),
       (-880, 200, 0), (-880, -200, 0), (-880, 0, 200), (-880, 0, -200), (-1100, 0, 0), (-1080, 150, 100),
       (-1080, -150, -100)]
Q3 = [tuple(10000 + c for c in p) for p in FIL]


def f5_points(t, embed=False):
    xs = [0, 6, 12] + [x + t for x in (22, 28, 34, 52, 58, 64)]
    return [(x, x, 0) if embed else (x, 0, 0) for x in xs]


def groups(labels):
    out = {}
    for i, c in enumerate(np.asarray(labels).tolist()):
        if c >= 0:
            out.setdefault(c, []).append(i)
    return sorted(out.values())


def sklearn_tree(points, k):
    from sklearn.cluster import HDBSCAN
    model = HDBSCAN(min_samples=k, min_cluster_size=2, metric='euclidean', algorithm='kd_tree', n_jobs=1, copy=True)
    model.fit(np.asarray(points, dtype=np.float64))
    return np.asarray(model._single_linkage_tree_), np.asarray(model.labels_)


class Bench(object):
    """Arbres de points du banc pour un nuage, avec mutants de construction eventuels."""

    def __init__(self, binary, work, reference):
        self.binary, self.work, self.reference = binary, work, reference
        self.cache = {}

    def orders(self, points, name, kmax):
        n = len(points)
        if self.reference:
            definition = Definition(points)
            return {k: pr.order_from_definition(definition.order(k), n, k) for k in range(1, kmax + 1)}, \
                np.arange(n, dtype=np.int64)
        data = pg.run_export(self.binary, points, self.work, name, kmax)
        return data['orders'], data['ids'].astype(np.int64)

    def trees(self, points, name, kmax, mutant=None):
        key = (tuple(map(tuple, points)), kmax, mutant)
        got = self.cache.get(key)
        if got is not None:
            return got
        if len(self.cache) > 64:
            self.cache.clear()
        orders, ids = self.orders(points, name, kmax)
        out = self.cache[key] = {}
        for k in range(1, kmax + 1):
            hang = prad.hang_margin_radius(orders[k], prad.qualification(k))
            pt = pf.tower_point_tree(hang, split_entries=mutant == 'entrees_separees', binarize=mutant == 'binarise')
            pt.ids = ids
            tree, _ = sklearn_tree(points, k)
            apt = pf.linkage_point_tree(tree, len(points), binarize=mutant == 'binarise')
            out[k] = (pt, apt)
        return out


def bench_labels(tree, mcs, z, method, mutant, stats):
    inject = mutant if mutant in ('masse_finale', 'seuil_moins_un', 'flottant_seul', 'egalite_enfants',
                                  'racine_admise', 'sorties_brutes', 'niveau_carre') else None
    try:
        lab, st = pf.flat(tree, mcs, z, method, inject)
    except pf.Refusal as error:
        stats['refusals'].append(str(error))
        return None
    stats['equalities'] += st['equalities']
    stats['exact_paths'] += st['exact']
    return lab


def oracle_labels(side, points, k, mcs, z, method):
    fn = po.tower_flat if side == 'T' else po.hdbscan_flat
    return np.asarray(fn(points, k, mcs, z, method)['labels'], dtype=np.int64)


def compare_cloud(bench, points, name, stats, mutant=None):
    """Toutes les lignes du banc contre l'oracle ; rend faux au premier desaccord (mutants : arret rapide)."""
    n = len(points)
    kmax = min(4, n - 1)
    trees = bench.trees(points, name, kmax, mutant)
    for k in range(1, kmax + 1):
        for side, tree in zip(('T', 'A'), trees[k]):
            for mcs in MCS:
                for method, z in LINES:
                    got = bench_labels(tree, mcs, z, method, mutant, stats)
                    want = oracle_labels(side, points, k, mcs, z, method)
                    stats['comparisons'] += 1
                    if got is None or not np.array_equal(got, want):
                        stats['disagreements'].append(dict(cloud=name, points=points, side=side, k=k, mcs=mcs, z=z,
                                                           method=method, bench=None if got is None else got.tolist(),
                                                           oracle=want.tolist()))
                        if mutant:
                            return False
    stats['clouds'] += 1
    return True


def tree_from_spec(n, levels, blocks, entries):
    """Arbre de points abstrait (fixtures de la tete seule) : levels = liste de Level croissants, blocks = liste de
    (plateau, enfants), entries = liste de (site, bloc, plateau)."""
    pt = pf.PointTree(n)
    for level in levels:
        pt.add_plateau(level)
    for plateau, children in blocks:
        pt.add_block(plateau, children)
    for site, block, plateau in entries:
        pt.enter(site, block, plateau)
    return pt.finish()


def sq(x):
    """Niveau de rayon x rationnel (t = x^2)."""
    x = Fraction(x)
    return pf.Level(x * x)


def head_fixtures(mutant, stats):
    """F14 : arbres abstraits, tete seule ; sorties calculees a la main."""
    facts = []
    # (a) 2^-70 : enfants D1 = {0, 1}, D2 = {2, 3} rejoints au rayon a, fusion en P au rayon 2, haut de P au rayon 4
    # (sous une racine) ; S(D1) + S(D2) - S(P) = 4 (1/a - 2/2 + 1/4) = 2^-70 > 0 : les enfants l'emportent.
    inv_a = Fraction(3, 4) + Fraction(1, 2 ** 72)
    a = 1 / inv_a
    pt = tree_from_spec(6, [sq(a), sq(2), sq(3), sq(4)],
                        [(0, []), (0, []), (1, [0, 1]), (2, []), (3, [2, 3])],
                        [(0, 0, 0), (1, 0, 0), (2, 1, 0), (3, 1, 0), (4, 3, 2), (5, 3, 2)])
    lab = bench_labels(pt, 2, 1, 'eom', mutant, stats)
    facts.append(dict(fixture='F14a_2_moins_70', got=None if lab is None else groups(lab),
                      ok=lab is not None and groups(lab) == [[0, 1], [2, 3], [4, 5]]))
    # (b) plateau a gros et petits enfants : D1 = {0,1,2}, D2 = {3,4,5} (gros a mcs 3), petit {6} ; fusion N-aire au
    # rayon 3 ; puis un groupe lointain {7,8,9} et la racine au rayon 10. Feuilles : D1 | D2 | {7,8,9} ; le site 6
    # rejoint le parent (bruit si le parent n'est pas retenu).
    pt = tree_from_spec(10, [sq(1), sq(3), sq(10)],
                        [(0, []), (0, []), (0, []), (1, [0, 1, 2]), (0, []), (2, [3, 4])],
                        [(0, 0, 0), (1, 0, 0), (2, 0, 0), (3, 1, 0), (4, 1, 0), (5, 1, 0), (6, 2, 0),
                         (7, 4, 0), (8, 4, 0), (9, 4, 0)])
    lab = bench_labels(pt, 3, 1, 'leaf', mutant, stats)
    facts.append(dict(fixture='F14b_plateau_gros_petits', got=None if lab is None else groups(lab),
                      ok=lab is not None and groups(lab) == [[0, 1, 2], [3, 4, 5], [7, 8, 9]]))
    # (c) entree entre deux niveaux qui franchit mcs : bloc {0,1} (rayon 1), le site 2 entre a la date
    # sqrt(4) + sqrt(9) - sqrt(1) = 4, strictement entre 3 et 5 ; groupe {3,4,5} ; racine au rayon 9. mcs 3 :
    # deux clusters {0,1,2} et {3,4,5}.
    pt = tree_from_spec(6, [sq(1), sq(3), pf.Level(4, 9, 1), sq(9)],
                        [(0, []), (1, []), (3, [0, 1])],
                        [(0, 0, 0), (1, 0, 0), (2, 0, 2), (3, 1, 1), (4, 1, 1), (5, 1, 1)])
    lab = bench_labels(pt, 3, 1, 'eom', mutant, stats)
    facts.append(dict(fixture='F14c_entree_franchit_mcs', got=None if lab is None else groups(lab),
                      ok=lab is not None and groups(lab) == [[0, 1, 2], [3, 4, 5]]))
    # (d) n < mcs : tout bruit.
    pt = tree_from_spec(3, [sq(1), sq(2)], [(0, []), (0, []), (1, [0, 1])], [(0, 0, 0), (1, 0, 0), (2, 1, 0)])
    lab = bench_labels(pt, 4, 1, 'eom', mutant, stats)
    facts.append(dict(fixture='F14d_n_sous_mcs', got=None if lab is None else lab.tolist(),
                      ok=lab is not None and bool(np.all(lab < 0))))
    # (e) foret a racine virtuelle : deux arbres jamais reunis, {0,1,2} et {3,4,5}, mcs 3 : chacun est un enfant de la
    # racine virtuelle (haut infini, phi = 0), donc admissible ; EOM et feuilles rendent les deux.
    pt = tree_from_spec(6, [sq(1), sq(2)], [(0, []), (1, [])],
                        [(0, 0, 0), (1, 0, 0), (2, 0, 0), (3, 1, 1), (4, 1, 1), (5, 1, 1)])
    ok = True
    got = []
    for method in ('eom', 'leaf'):
        lab = bench_labels(pt, 3, 1, method, mutant, stats)
        got.append(None if lab is None else groups(lab))
        ok = ok and lab is not None and groups(lab) == [[0, 1, 2], [3, 4, 5]]
    facts.append(dict(fixture='F14e_foret_racine_virtuelle', got=got, ok=ok))
    return facts


def fixtures(bench, mutant, stats):
    """F1-F15 de la spec (F2 et F10 : voir le recu ; remplaces par des faits exacts equivalents ou comptes a part)."""
    facts = []

    def flat(points, name, k, mcs, z, method, side='T'):
        trees = bench.trees(points, name, k, mutant)[k]
        return bench_labels(trees[0] if side == 'T' else trees[1], mcs, z, method, mutant, stats)

    def fact(name, lab, expected, extra=None):
        got = None if lab is None else groups(lab)
        facts.append(dict(fixture=name, got=got, expected=expected, ok=got == expected, **(extra or {})))

    # F1 deux triangles equilateraux, k = 2
    for mcs in (2, 3):
        fact('F1_T_mcs%d' % mcs, flat(TRIANGLES, 'f1', 2, mcs, 1, 'eom'), [[0, 1, 2], [3, 4, 5]])
        fact('F1_A_mcs%d' % mcs, flat(TRIANGLES, 'f1', 2, mcs, 1, 'eom', 'A'), [])
    fact('F1_T_mcs4', flat(TRIANGLES, 'f1', 2, 4, 1, 'eom'), [])
    # F2 variantes de T0, k = 2, mcs 3 : ABC | DEF
    for name, pts in T0_VARIANTS.items():
        fact('F2_%s' % name, flat(pts, 'f2', 2, 3, 1, 'eom'), [[0, 1, 2], [3, 4, 5]])
    # F3 cinq points sous 120 permutations (T et A) : {1,2} | {3,4}, le site 0 bruit
    bad = []
    for perm in itertools.permutations(range(5)):
        pts = [FIVE[i] for i in perm]
        for side in ('T', 'A'):
            lab = flat(pts, 'f3', 2, 2, 1, 'eom', side)
            if lab is None:
                bad.append((perm, side))
                continue
            back = np.full(5, -1)
            back[list(perm)] = lab
            if groups(back) != [[1, 2], [3, 4]]:
                bad.append((perm, side))
    facts.append(dict(fixture='F3_cinq_points_120_permutations', bad=len(bad), ok=not bad))
    # F4 neuf sites, k = 2, mcs 3
    fact('F4_z1', flat(NINE, 'f4', 2, 3, 1, 'eom'), [[0, 1, 2, 3, 4, 5], [6, 7, 8]])
    fact('F4_z2', flat(NINE, 'f4', 2, 3, 2, 'eom'), F4_Z2)
    fact('F4b_z1', flat(NINE_B, 'f4b', 2, 3, 1, 'eom'), [[0, 1, 2, 3, 4, 5], [6, 7, 8]])
    fact('F4b_z2', flat(NINE_B, 'f4b', 2, 3, 2, 'eom'), [[0, 1, 2], [3, 4, 5], [6, 7, 8]])
    fact('F4_z3', flat(NINE, 'f4', 2, 3, 3, 'eom'), [[0, 1, 2], [3, 4, 5], [6, 7, 8]])
    fact('F4_feuilles', flat(NINE, 'f4', 2, 3, 1, 'leaf'), [[0, 1, 2], [3, 4, 5], [6, 7, 8]])
    fact('F4_mcs4', flat(NINE, 'f4', 2, 4, 1, 'eom'), [])
    # F5 et F6 : egalites certifiees 1/4 et sqrt(2)/8
    for t, expected in ((-1, [[0, 1, 2, 3, 4, 5], [6, 7, 8]]), (0, [[0, 1, 2, 3, 4, 5], [6, 7, 8]]),
                        (1, [[0, 1, 2], [3, 4, 5], [6, 7, 8]])):
        before = stats['equalities']
        lab = flat(f5_points(t), 'f5', 2, 3, 1, 'eom')
        fact('F5_t%+d' % t, lab, expected, dict(equalities=stats['equalities'] - before))
        if t == 0:
            facts[-1]['ok'] = facts[-1]['ok'] and stats['equalities'] - before >= 1
    before = stats['equalities']
    fact('F6_racine_2_sur_8', flat(f5_points(0, True), 'f6', 2, 3, 1, 'eom'), [[0, 1, 2, 3, 4, 5], [6, 7, 8]],
         dict(equalities=stats['equalities'] - before))
    facts[-1]['ok'] = facts[-1]['ok'] and stats['equalities'] - before >= 1
    # F7 dix points, k = 2, mcs 2 : mort par masse engagee
    fact('F7_dix_points', flat(TEN, 'f7', 2, 2, 1, 'eom'), [[1, 2, 3, 4], [5, 8, 9]])
    # F8 1D, k = 1 : T = A, egalite exacte S(A u B) = S(A) + S(B) ; sklearn consigne
    before = stats['equalities']
    lt, la = flat(LINE1D, 'f8', 1, 3, 1, 'eom'), flat(LINE1D, 'f8', 1, 3, 1, 'eom', 'A')
    _, sk = sklearn_tree_labels(LINE1D, 1, 3)
    facts.append(dict(fixture='F8_1d_egalite', tower=groups(lt) if lt is not None else None,
                      hdbscan_head=groups(la) if la is not None else None, sklearn=sk,
                      ok=lt is not None and la is not None and groups(lt) == groups(la) ==
                      [[0, 1, 2, 3, 4, 5], [6, 7, 8]] and stats['equalities'] - before >= 2))
    # F9 six points, min_samples = 3, mcs 2 : cote A contre l'oracle ; sklearn consigne
    la = flat(SIX, 'f9', 3, 2, 1, 'eom', 'A')
    want = oracle_labels('A', SIX, 3, 2, 1, 'eom')
    _, sk = sklearn_tree_labels(SIX, 3, 2)
    facts.append(dict(fixture='F9_six_points_A', got=None if la is None else groups(la), oracle=groups(want),
                      sklearn=sk, ok=la is not None and np.array_equal(la, want)))
    # F10 Q3, k = 2 : mcs 3 -> {x, c0..c7} | {f1..f5} ; mcs 6 -> tout bruit (prix declare de la racine exclue)
    fact('F10_q3_mcs3', flat(Q3, 'f10', 2, 3, 1, 'eom'), [[0] + list(range(6, 14)), [1, 2, 3, 4, 5]])
    fact('F10_q3_mcs6', flat(Q3, 'f10', 2, 6, 1, 'eom'), [])
    # F11 paire a boule mixte, k = 2, mcs 2 : banc contre oracle, feuilles et EOM
    ok = True
    for method, z in LINES:
        lab = flat(MIXED, 'f11', 2, 2, z, method)
        ok = ok and lab is not None and np.array_equal(lab, oracle_labels('T', MIXED, 2, 2, z, method))
    facts.append(dict(fixture='F11_boule_mixte', ok=ok))
    # F12 huit sites de l'auditeur + b3, k = 2, mcs 3, z = 3 : banc contre oracle
    lab = flat(EIGHT_B3, 'f12', 2, 3, 3, 'eom')
    facts.append(dict(fixture='F12_huit_sites_b3', got=None if lab is None else groups(lab),
                      ok=lab is not None and np.array_equal(lab, oracle_labels('T', EIGHT_B3, 2, 3, 3, 'eom'))))
    # F13 identite a k = 1 (D1) : T = A pour tout mcs et toute selection, sur 24 nuages
    rng = random.Random(1313)
    bad = 0
    for index in range(24):
        pts = random_cloud(rng)
        trees = bench.trees(pts, 'f13_%d' % index, 1, mutant)[1]
        for mcs in MCS:
            for method, z in LINES:
                a = bench_labels(trees[0], mcs, z, method, mutant, stats)
                b = bench_labels(trees[1], mcs, z, method, mutant, stats)
                bad += a is None or b is None or not np.array_equal(a, b)
    facts.append(dict(fixture='F13_identite_k1', clouds=24, bad=bad, ok=bad == 0))
    # F14 arbres abstraits (tete seule)
    facts.extend(head_fixtures(mutant, stats))
    # F15 metriques
    try:
        import points_flat_metrics as pm
        failures = pm.self_test()
        facts.append(dict(fixture='F15_metriques', failures=failures[:5], ok=not failures))
    except ImportError as error:
        facts.append(dict(fixture='F15_metriques', error=str(error), ok=False))
    return facts


def sklearn_tree_labels(points, k, mcs):
    from sklearn.cluster import HDBSCAN
    model = HDBSCAN(min_samples=k, min_cluster_size=mcs, metric='euclidean', algorithm='kd_tree', n_jobs=1, copy=True)
    model.fit(np.asarray(points, dtype=np.float64))
    return None, np.asarray(model.labels_).tolist()


def random_cloud(rng):
    """Nuages de 6 a 10 sites a ex aequo frequents (grilles, plans, echelles)."""
    n = rng.randint(6, 10)
    side = rng.choice([3, 4, 5, 8, 12, 30])
    scale = rng.choice([1, 1, 3, 7])
    pts = set()
    while len(pts) < n:
        pts.add((rng.randrange(side) * scale, rng.randrange(side) * scale,
                 rng.choice([0, 0, rng.randrange(side) * scale])))
    pts = sorted(pts)
    rng.shuffle(pts)
    return pts


def run_mutants(bench, work, stats, seed):
    out = {}
    for name in MUTANTS:
        local = dict(clouds=0, comparisons=0, disagreements=[], refusals=[], equalities=0, exact_paths=0)
        how = []
        guard = pg.Mutant('coupe_ouverte') if name == 'coupe_ouverte' else None
        try:
            if guard is not None:  # egalite date = naissance du parent tranchee comme stricte (points_gate)
                guard.__enter__()
            facts = fixtures(Bench(bench.binary, work / name, bench.reference), name, local)
            failed = [f['fixture'] for f in facts if not f['ok']]
            if failed:
                how.append('fixtures:' + ','.join(failed[:4]))
            rng = random.Random(seed)
            for index in range(MUTANT_CLOUDS):
                if not compare_cloud(Bench(bench.binary, work / name, bench.reference), random_cloud(rng),
                                     'm%03d' % index, local, name):
                    how.append('desaccord:m%03d' % index)
                    break
        except (pf.Refusal, ValueError, ZeroDivisionError, KeyError, IndexError) as error:
            how.append('invariant:%s:%s' % (type(error).__name__, error))
        finally:
            if guard is not None:
                guard.__exit__(None, None, None)
        out[name] = dict(killed=bool(how), how=how[:3])
    stats['mutants'] = out
    return sum(v['killed'] for v in out.values())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--export', type=Path)
    parser.add_argument('--reference', action='store_true', help='ordre de reference Python (essai local)')
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--seconds', type=float, default=300.0)
    parser.add_argument('--seed', type=int, default=20261004)
    parser.add_argument('--mirror', type=Path)
    args = parser.parse_args()
    if not args.reference and (args.export is None or not args.export.is_file()):
        print('refus : exportateur absent', file=sys.stderr)
        return 2
    args.work.mkdir(parents=True, exist_ok=True)
    bench = Bench(args.export, args.work, args.reference)
    stats = dict(clouds=0, comparisons=0, disagreements=[], refusals=[], equalities=0, exact_paths=0,
                 mode='reference' if args.reference else 'native')
    started = time.monotonic()
    stats['fixtures'] = fixtures(bench, None, stats)
    for name, cloud in (('fixe_triangles', TRIANGLES), ('fixe_cinq', FIVE), ('fixe_neuf', NINE),
                        ('fixe_dix', TEN), ('fixe_six', SIX), ('fixe_mixte', MIXED), ('fixe_huit_b3', EIGHT_B3)):
        compare_cloud(bench, cloud, name, stats)
    rng = random.Random(args.seed)
    index = 0
    while time.monotonic() - started < args.seconds:
        compare_cloud(bench, random_cloud(rng), 'c%05d' % index, stats)
        index += 1
    killed = run_mutants(bench, args.work / 'mutants', stats, args.seed + 1)
    fixtures_ok = all(f['ok'] for f in stats['fixtures'])
    floors_ok = stats['clouds'] >= FLOORS['clouds'] and stats['comparisons'] >= FLOORS['comparisons'] and \
        stats['equalities'] >= FLOORS['equalities'] and len(stats['fixtures']) >= FLOORS['fixtures'] and \
        killed >= FLOORS['mutants'] and not stats['refusals']
    verdict = 'desaccord' if stats['disagreements'] else ('conforme' if fixtures_ok and floors_ok else 'plancher')
    stats.update(verdict=verdict, floors=FLOORS, seconds=round(time.monotonic() - started, 1), seed=args.seed,
                 disagreements=stats['disagreements'][:20], refusals=stats['refusals'][:20])
    text = json.dumps(stats, indent=1, sort_keys=True, default=str) + '\n'
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(text)
    if args.mirror:
        args.mirror.parent.mkdir(parents=True, exist_ok=True)
        args.mirror.write_text(text)
    print('points_flat_gate_verdict %s nuages%d comparaisons%d egalites%d fixtures%d/%d mutants%d/%d' % (
        verdict, stats['clouds'], stats['comparisons'], stats['equalities'],
        sum(f['ok'] for f in stats['fixtures']), len(stats['fixtures']), killed, len(MUTANTS)))
    return {'conforme': 0, 'desaccord': 1}.get(verdict, 3)


if __name__ == '__main__':
    raise SystemExit(main())
