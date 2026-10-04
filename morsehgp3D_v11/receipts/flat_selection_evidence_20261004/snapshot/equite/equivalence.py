#!/usr/bin/env python3
"""Question (b) : la tete N-aire, appliquee a l'arbre du lien simple de HDBSCAN, reproduit-elle les etiquettes de
sklearn ? Trois routes par configuration :

  (i)   sklearn tel quel : HDBSCAN(...).fit(X).labels_ ;
  (ii)  tete N-aire (nary_head.head) sur le dendrogramme N-aire tire de _single_linkage_tree_ (plateaux atomises) ;
  (iii) code de sklearn (tree_to_labels) sur la binarisation canonique (gros enfants d abord) de ce dendrogramme ;
  (iv)  transcription (sk_transcription) sur l arbre de sklearn : condensation et etiquetage de sklearn, selection
        transcrite ; sert d etiquettes sklearn quand numpy 2.5 fait echouer epsilon (TypeError) ;
  (v)   transcription sur la binarisation canonique.

Attendus (RAPPORT.md, theoremes A et B) : (ii) = (iii) partout, sauf l'exception documentee d'epsilon a la racine
N-aire ; (i) = (ii) des que l'arbre n'a aucun plateau (aucune multifusion) et que epsilon n'egale aucun niveau.
Les ecarts (i) != (ii) ne peuvent venir que des plateaux : ils sont comptes et classes.

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B equivalence.py [graine] > equivalence_out.json
"""
import json
import sys
import time
import warnings

import numpy as np

import nary_head as nh
import sk_transcription as skt

warnings.filterwarnings('ignore')


def cloud(family, n, rng):
    if family == 'blobs2d':
        c = rng.uniform(0, 20, (3, 2))
        m = int(n * 0.9)
        lab = rng.integers(0, 3, m)
        X = c[lab] + rng.normal(0, 1.0, (m, 2))
        X = np.r_[X, rng.uniform(-3, 23, (n - m, 2))]
    elif family == 'blobs3d':
        c = rng.uniform(0, 20, (4, 3))
        m = int(n * 0.9)
        lab = rng.integers(0, 4, m)
        X = c[lab] + rng.normal(0, 1.2, (m, 3))
        X = np.r_[X, rng.uniform(-3, 23, (n - m, 3))]
    elif family == 'uniform3d':
        X = rng.uniform(0, 10, (n, 3))
    elif family == 'grid3d':  # coordonnees entieres : egalites exactes de distances (regime quantifie)
        c = rng.integers(0, 30, (3, 3))
        m = int(n * 0.9)
        lab = rng.integers(0, 3, m)
        X = c[lab] + np.round(rng.normal(0, 2.0, (m, 3)))
        X = np.r_[X, rng.integers(-3, 33, (n - m, 3))]
        X = np.unique(X, axis=0)  # sites distincts
        rng.shuffle(X)
    elif family == 'lattice2d':  # cas extreme : reseau regulier perturbe par sous-ensembles
        side = int(np.ceil(np.sqrt(n)))
        g = np.array([(i, j) for i in range(side) for j in range(side)], dtype=float)[:n]
        X = g * 2.0
        X[: n // 3] += 25.0
    else:
        raise ValueError(family)
    return np.asarray(X, dtype=np.float64)


def eps_grid(tree):
    """Epsilon strictement entre deux niveaux distincts (milieux), aux quantiles 0,5 et 0,9 des niveaux."""
    vals = np.unique(np.asarray(tree['value'], dtype=np.float64))
    vals = vals[vals > 0]
    if len(vals) < 3:
        return []
    out = []
    for q in (0.5, 0.9):
        j = min(int(q * (len(vals) - 1)), len(vals) - 2)
        out.append(float((vals[j] + vals[j + 1]) / 2.0))
    return out


def main():
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 20261004
    rng = np.random.default_rng(seed)
    families = ['blobs2d', 'blobs3d', 'uniform3d', 'grid3d', 'lattice2d']
    sizes = [30, 80, 200]
    ks = [1, 2, 3, 5, 10]
    mcss = [2, 5, 10, 20]
    stats = dict(configs=0, eps_configs=0, fit_crash=0, canon_crash=0, fit_vs_tree=0, transcription_vs_fit=0,
                 transcription_vs_fit_mismatch=0, ii_eq_iii=0, ii_ne_iii=0, ii_ne_iii_root_eps=0, ii_eq_v=0,
                 ii_ne_v=0, ii_ne_v_root_eps=0, i_eq_ii=0, i_ne_ii=0, i_ne_ii_without_plateau=0, trees=0,
                 trees_with_plateau=0, plateau_nodes=0, points_differing=0, points_total=0)
    by_k = {k: dict(trees=0, with_plateau=0, plateau_nodes=0, configs=0, i_ne_ii=0) for k in ks}
    by_family = {f: dict(trees=0, with_plateau=0, configs=0, i_ne_ii=0) for f in families}
    by_method = {m: dict(configs=0, i_ne_ii=0) for m in ('eom', 'leaf')}
    by_eps = {e: dict(configs=0, i_ne_ii=0, fit_crash=0) for e in ('zero', 'positive')}
    examples = []
    t0 = time.time()
    for family in families:
        for n in sizes:
            for rep in range(2):
                X = cloud(family, n, rng)
                n_eff = len(X)
                for k in ks:
                    if k > n_eff:
                        continue
                    base = nh.hdbscan_fit(X, k, 2)
                    tree = np.asarray(base._single_linkage_tree_)
                    d = nh.from_linkage(tree, n_eff)
                    plateaus = d.plateaus()
                    stats['trees'] += 1
                    by_k[k]['trees'] += 1
                    by_k[k]['plateau_nodes'] += len(plateaus)
                    by_family[family]['trees'] += 1
                    stats['plateau_nodes'] += len(plateaus)
                    if plateaus:
                        stats['trees_with_plateau'] += 1
                        by_k[k]['with_plateau'] += 1
                        by_family[family]['with_plateau'] += 1
                    for mcs in mcss:
                        if mcs > n_eff:
                            continue
                        cl = nh.condense(d, mcs)
                        root_bigs = len(cl[0].children)
                        for method in ('eom', 'leaf'):
                            for asc in (False, True):
                                for eps in [0.0] + eps_grid(tree):
                                    stats['configs'] += 1
                                    ekey = 'zero' if eps == 0 else 'positive'
                                    by_eps[ekey]['configs'] += 1
                                    if eps:
                                        stats['eps_configs'] += 1
                                    by_k[k]['configs'] += 1
                                    by_family[family]['configs'] += 1
                                    by_method[method]['configs'] += 1
                                    lab_iv = skt.tree_to_labels(tree, mcs, method, asc, eps)
                                    try:
                                        model = nh.hdbscan_fit(X, k, mcs, method, eps, asc)
                                        lab_i = np.asarray(model.labels_)
                                        lab_t = nh.sklearn_tree_to_labels(tree, mcs, method, asc, eps)
                                        if np.array_equal(lab_i, lab_t):
                                            stats['fit_vs_tree'] += 1
                                        if np.array_equal(lab_i, lab_iv):
                                            stats['transcription_vs_fit'] += 1
                                        else:
                                            stats['transcription_vs_fit_mismatch'] += 1
                                    except TypeError:
                                        stats['fit_crash'] += 1
                                        by_eps[ekey]['fit_crash'] += 1
                                        lab_i = lab_iv  # ce que rendrait sklearn avec numpy < 2.5
                                    lab_ii = nh.head(d, mcs, 1.0, method, eps, asc)
                                    try:
                                        lab_iii = nh.sklearn_head(d, mcs, 1.0, method, eps, asc)
                                        if nh.same_partition(lab_ii, lab_iii):
                                            stats['ii_eq_iii'] += 1
                                        else:
                                            stats['ii_ne_iii'] += 1
                                            if eps > 0 and root_bigs >= 3:
                                                stats['ii_ne_iii_root_eps'] += 1
                                            elif len(examples) < 40:
                                                examples.append(dict(kind='ii_ne_iii', family=family, n=n_eff, k=k,
                                                                     mcs=mcs, method=method, asc=asc, eps=eps))
                                    except TypeError:
                                        stats['canon_crash'] += 1
                                    lab_v = skt.tree_to_labels(nh.binarize(d, mcs, 1.0), mcs, method, asc, eps)
                                    if nh.same_partition(lab_ii, lab_v):
                                        stats['ii_eq_v'] += 1
                                    else:
                                        stats['ii_ne_v'] += 1
                                        if eps > 0 and root_bigs >= 3:
                                            stats['ii_ne_v_root_eps'] += 1
                                        elif len(examples) < 40:
                                            examples.append(dict(kind='ii_ne_v', family=family, n=n_eff, k=k,
                                                                 mcs=mcs, method=method, asc=asc, eps=eps))
                                    stats['points_total'] += n_eff
                                    if nh.same_partition(lab_i, lab_ii):
                                        stats['i_eq_ii'] += 1
                                    else:
                                        stats['i_ne_ii'] += 1
                                        by_k[k]['i_ne_ii'] += 1
                                        by_family[family]['i_ne_ii'] += 1
                                        by_method[method]['i_ne_ii'] += 1
                                        by_eps[ekey]['i_ne_ii'] += 1
                                        stats['points_differing'] += nh.points_differing(lab_i, lab_ii)
                                        if not plateaus:
                                            stats['i_ne_ii_without_plateau'] += 1
                                            examples.append(dict(kind='i_ne_ii_sans_plateau', family=family,
                                                                 n=n_eff, k=k, mcs=mcs, method=method, asc=asc,
                                                                 eps=eps))
    stats['seconds'] = round(time.time() - t0, 1)
    out = dict(seed=seed, sklearn=__import__('sklearn').__version__, numpy=np.__version__, stats=stats,
               by_k={str(k): v for k, v in by_k.items()}, by_family=by_family, by_method=by_method, by_eps=by_eps,
               examples=examples)
    print(json.dumps(out, indent=1, sort_keys=True))


if __name__ == '__main__':
    main()
