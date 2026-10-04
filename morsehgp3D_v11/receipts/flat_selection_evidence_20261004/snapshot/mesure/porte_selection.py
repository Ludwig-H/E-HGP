#!/usr/bin/env python3
"""Porte de la selection commune : identite avec sklearn sur ses propres arbres, sensibilite aux fusions ex aequo,
fixture T0 (deux triangles de la these) sur H^r_{k+1} par l'oracle exact, mutants causaux.

    python3 -B porte_selection.py > sorties/porte_selection.txt ; code 0 si conforme.

Petits nuages seulement (n <= 2 000) : c'est un oracle de correction, pas une mesure.
"""
from fractions import Fraction
import math
import os
import sys
import warnings

import numpy as np
from sklearn.cluster import HDBSCAN
import sklearn.cluster._hdbscan._tree as skt
import sklearn.cluster._hdbscan.hdbscan as skh

import selection_commune as sc
import vendor_scenes_v10_pin as vs

warnings.filterwarnings('ignore')
HERE = os.path.dirname(os.path.abspath(__file__))
FAIL = []


def same_partition(a, b):
    """Egalite de partitions (bruit = -1 compare tel quel), a renommage des clusters pres."""
    a, b = np.asarray(a), np.asarray(b)
    if not np.array_equal(a < 0, b < 0):
        return False
    m = {}
    for x, y in zip(a.tolist(), b.tolist()):
        if x < 0:
            continue
        if m.setdefault(x, y) != y:
            return False
    return len(set(m.values())) == len(m)


def canon(ct):
    return sorted((int(e['parent']), int(e['child']), float(e['value']), int(e['cluster_size'])) for e in ct)


def identity_block():
    print('== identite avec sklearn (arbres binaires de sklearn, z = 1)')
    total = same_ct = same_lab = plateau_diff = plateau_total = 0
    eps_total = eps_same = eps_raise = 0
    for family in vs.FAMILIES:
        for n, seed in ((300, 7101), (1000, 7102), (2000, 7103)):
            pts, lab, _ = vs.generate(dict(family=family, level='hard', groups=8 if n >= 1000 else 3, n=n,
                                           noise_fraction=0.05, seed=seed))
            grid, lab, _, _ = vs.quantize18(pts, lab)
            X = grid.astype(np.float64)
            for k in (2, 3, 5, 10):
                tree = HDBSCAN(min_samples=k, min_cluster_size=2, algorithm='kd_tree', n_jobs=1,
                               copy=True).fit(X)._single_linkage_tree_
                h = sc.from_sklearn(tree, len(X))
                hp = sc.plateau(h)
                ties = int(np.sum(np.diff(np.sort(tree['value'])) == 0))
                for mcs in sorted({k, 5, 10, 20}):
                    ct_sk = skt._condense_tree(tree, mcs)
                    ct_me = sc.condense(h, mcs)
                    ok_ct = canon(ct_sk) == canon(ct_me)
                    for method in ('eom', 'leaf'):
                        total += 1
                        same_ct += ok_ct
                        ref = skh.tree_to_labels(tree, mcs, method, False, 0.0, None)[0]
                        mine = sc.flat(h, mcs, method)
                        if np.array_equal(ref, mine):
                            same_lab += 1
                        else:
                            FAIL.append('labels %s n%d k%d mcs%d %s' % (family, n, k, mcs, method))
                        plateau_total += 1
                        plateau_diff += not same_partition(mine, sc.flat(hp, mcs, method))
                    # epsilon > 0 : sklearn peut lever (numpy >= 2.5) ; on compare la ou il tourne
                    if mcs == 20:
                        from sklearn.neighbors import NearestNeighbors
                        dk = NearestNeighbors(n_neighbors=k).fit(X).kneighbors(X)[0][:, -1]
                        med = float(np.median(dk))
                        for c in (1, 2, 5):
                            for method in ('eom', 'leaf'):
                                eps_total += 1
                                mine = sc.flat(h, mcs, method, eps=c * med)
                                try:
                                    ref = skh.tree_to_labels(tree, mcs, method, False, c * med, None)[0]
                                except TypeError:
                                    eps_raise += 1
                                    continue
                                if np.array_equal(ref, mine):
                                    eps_same += 1
                                else:
                                    FAIL.append('eps %s n%d k%d c%d %s' % (family, n, k, c, method))
    print('  configurations %d : arbre condense identique %d, etiquettes identiques %d' % (total, same_ct, same_lab))
    print('  epsilon > 0 : %d configurations, %d identiques, %d ou sklearn leve (numpy %s)' % (
        eps_total, eps_same, eps_raise, np.__version__))
    print('  fusions ex aequo lues comme multifusions (plateaux) : partitions differentes dans %d / %d '
          'configurations' % (plateau_diff, plateau_total))
    if same_lab != total or same_ct != total:
        FAIL.append('identite')
    return plateau_diff, plateau_total


def t0_block():
    """T0 : deux triangles equilateraux exacts (these § 6.1), k = 2 ; H^r_3 par l'oracle exact."""
    print('\n== fixture T0 (deux triangles), k = 2')
    sys.path.insert(0, os.path.join(HERE, '..', 'contexte'))
    import oracle_hierarchy as oh
    tri = [tuple(c + 2 for c in p) for p in ((-1, -1, 0), (-1, 0, -1), (0, 0, 0), (1, 1, 0), (2, 2, 0), (2, 1, 1))]
    o = oh.hierarchy(tri, 2)
    h = sc.Hier(6, o.parent, o.birth, o.owner, o.entry)
    for mcs in (2, 3):
        for method in ('eom', 'leaf'):
            lab = sc.flat(h, mcs, method)
            ok = same_partition(lab, [0, 0, 0, 1, 1, 1])
            print('  tour H^r_3, mcs %d, %s : %s %s' % (mcs, method, lab.tolist(), 'ABC | DEF' if ok else 'ECHEC'))
            if not ok:
                FAIL.append('T0 tour mcs%d %s' % (mcs, method))
    X = np.array(tri, dtype=np.float64)
    tree = HDBSCAN(min_samples=2, min_cluster_size=2, algorithm='kd_tree', n_jobs=1, copy=True).fit(X)._single_linkage_tree_
    print('  arbre HDBSCAN (min_samples=2) : hauteurs %s' % [round(float(v), 4) for v in tree['value']])
    for mcs in (2, 3):
        ref = skh.tree_to_labels(tree, mcs, 'eom', False, 0.0, None)[0]
        mine = sc.flat(sc.from_sklearn(tree, 6), mcs, 'eom')
        mine_p = sc.flat(sc.plateau(sc.from_sklearn(tree, 6)), mcs, 'eom')
        print('  HDBSCAN mcs %d eom : sklearn %s ; selection commune %s ; plateaux %s' % (
            mcs, ref.tolist(), mine.tolist(), mine_p.tolist()))


def mutants_block():
    """Mutants causaux de la selection : chacun doit changer au moins une partition sur le banc d'identite."""
    print('\n== mutants (doivent etre tues : au moins une partition differente de sklearn)')
    pts, lab, _ = vs.generate(dict(family='bridge', level='hard', groups=8, n=2000, noise_fraction=0.05, seed=7103))
    grid, lab, _, _ = vs.quantize18(pts, lab)
    X = grid.astype(np.float64)
    tree = HDBSCAN(min_samples=5, min_cluster_size=2, algorithm='kd_tree', n_jobs=1, copy=True).fit(X)._single_linkage_tree_
    h = sc.from_sklearn(tree, len(X))
    refs = {(mcs, m): skh.tree_to_labels(tree, mcs, m, False, 0.0, None)[0] for mcs in (5, 10, 20) for m in ('eom', 'leaf')}

    def killed(fn):
        return any(not np.array_equal(refs[key], fn(*key)) for key in refs)

    orig_lam = sc.lam
    sc.lam = lambda r, z: r  # lambda = r au lieu de 1/r
    k1 = killed(lambda mcs, m: sc.flat(h, mcs, m))
    sc.lam = orig_lam
    k2 = killed(lambda mcs, m: sc.flat(h, mcs + 1, m))  # seuil mcs decale (> au lieu de >=)
    k3 = killed(lambda mcs, m: sc.flat(h, mcs, m, z=2.0))  # exposant z = 2 au lieu de 1
    orig_stab = sc.stability
    sc.stability = lambda ct: {k: -v for k, v in orig_stab(ct).items()}  # stabilite inversee
    k4 = killed(lambda mcs, m: sc.flat(h, mcs, m))
    sc.stability = orig_stab
    for name, k in (('lambda = r', k1), ('mcs + 1', k2), ('z = 2', k3), ('stabilite inversee', k4)):
        print('  mutant %-20s %s' % (name, 'tue' if k else 'SURVIT'))
        if not k:
            FAIL.append('mutant ' + name)


def main():
    identity_block()
    t0_block()
    mutants_block()
    print('\nporte_selection', 'conforme' if not FAIL else 'ECHEC %s' % FAIL[:10])
    return 0 if not FAIL else 1


if __name__ == '__main__':
    sys.exit(main())
