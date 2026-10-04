#!/usr/bin/env python3
"""Ex aequo : ce que la binarisation de sklearn change, sur des fixtures exactes et sur de petits nuages.

Mecanisme (sklearn 1.9.1, identique en 1.7.2) : Prim (mst_from_data_matrix) ajoute un point a la fois, avec
departage par indice d'entree ; _process_mst trie les aretes par np.argsort (tri par defaut, non stable pour des
tableaux melanges sur cette machine) ; make_single_linkage fusionne deux a deux dans cet ordre. Un plateau
d'egalites exactes devient donc une chaine binaire dont la forme depend de l'ordre d'entree des points et de
l'implantation du tri (jeu d'instructions). On emule d'autres ordres de tri des ex aequo avec les fonctions de
sklearn elles-memes (seul l'ordre des aretes egales change, jamais leurs valeurs).

Sorties : ties_out.txt (lisible) et ties_out.json.
Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B -u ties.py > ties_out.txt
"""
import collections
import itertools
import json
import warnings

import numpy as np

import nary_head as nh
import sk_transcription as skt

warnings.filterwarnings('ignore')
RESULT = {}


def sk_mst(X, k, alpha=1.0):
    """MST de Prim de sklearn (meme appel que _hdbscan_prims), aretes dans l'ordre d'ajout."""
    from sklearn.cluster._hdbscan._linkage import mst_from_data_matrix
    from sklearn.metrics._dist_metrics import DistanceMetric
    from sklearn.neighbors import NearestNeighbors
    X = np.ascontiguousarray(X, dtype=np.float64)
    nbrs = NearestNeighbors(n_neighbors=k, algorithm='kd_tree', leaf_size=40, metric='euclidean', p=None).fit(X)
    core = np.ascontiguousarray(nbrs.kneighbors(X, k, return_distance=True)[0][:, -1])
    return mst_from_data_matrix(X, core, DistanceMetric.get_metric('euclidean'), alpha)


def sk_tree(mst, order):
    from sklearn.cluster._hdbscan._linkage import make_single_linkage
    return make_single_linkage(mst[np.asarray(order)])


def tie_orders(mst, rng, shuffles=5):
    w = mst['distance']
    m = len(w)
    out = {'numpy_defaut': np.argsort(w), 'stable': np.argsort(w, kind='stable'),
           'ex_aequo_inverses': np.lexsort((-np.arange(m), w))}
    for s in range(shuffles):
        out['ex_aequo_melanges_%d' % s] = np.lexsort((rng.permutation(m), w))
    return out


def name_partition(labels, names):
    part, noise = nh.canonical_partition(labels)
    key = ' | '.join(sorted(''.join(names[i] for i in sorted(b)) for b in part))
    if noise:
        key += '  ; bruit : ' + ''.join(names[i] for i in sorted(noise))
    return key or 'aucun cluster'


def section(title):
    print()
    print('=' * 100)
    print(title)
    print('=' * 100)


# ------------------------------------------------------------------------------------------------------------
def fixture_triangles():
    section('F0. Deux triangles equilateraux exacts de la these (par. 6.1), k = 2 : toutes les aretes MST valent sqrt 2')
    tri = np.array([(-1, -1, 0), (-1, 0, -1), (0, 0, 0), (1, 1, 0), (2, 2, 0), (2, 1, 1)], dtype=float) + 2
    names = 'ABCDEF'
    out = {}
    for mcs in (2, 3):
        for method in ('eom', 'leaf'):
            perms = collections.Counter()
            for perm in itertools.permutations(range(6)):
                lab = nh.hdbscan_fit(tri[list(perm)], 2, mcs, method).labels_
                back = np.empty(6, dtype=np.int64)
                back[list(perm)] = lab
                perms[name_partition(back, names)] += 1
            mst = sk_mst(tri, 2)
            orders = collections.Counter()
            for order in itertools.permutations(range(5)):
                tree = sk_tree(mst, list(order))
                lab = nh.sklearn_tree_to_labels(tree, mcs, method)
                orders[name_partition(lab, names)] += 1
            d = nh.from_linkage(nh.hdbscan_fit(tri, 2, mcs, method)._single_linkage_tree_, 6)
            nary = name_partition(nh.head(d, mcs, 1.0, method), names)
            print('mcs=%d %-4s | N-aire : [%s] | sklearn, 720 ordres d entree (cette machine) : %s' %
                  (mcs, method, nary, dict(perms)))
            print('               | sklearn, 120 ordres des 5 aretes ex aequo (emulation d autres tris) : %s' %
                  dict(orders))
            out['mcs%d_%s' % (mcs, method)] = dict(nary=nary, permutations=dict(perms), tie_orders=dict(orders))
    print('Lecture : l arbre de HDBSCAN n a qu une fusion a six enfants ; aucune hierarchie ne separe ABC de DEF.')
    print('Selon l ordre des ex aequo, sklearn peut pourtant rendre ABC | DEF (clusters de stabilite nulle).')
    RESULT['F0_deux_triangles'] = out


# ------------------------------------------------------------------------------------------------------------
def fixture_bridge():
    section('F1. Pont a egalite exacte (1D) : L = {0,1,2,3}, b = 6, R = {9,10,11,12} ; min_samples = 1, mcs = 3')
    X = np.array([0, 1, 2, 3, 6, 9, 10, 11, 12], dtype=float)[:, None]
    names = ['0', '1', '2', '3', 'b', '9', 'A', 'B', 'C']  # A, B, C = 10, 11, 12
    out = {}
    for method in ('eom', 'leaf'):
        perms = collections.Counter()
        prng = np.random.default_rng(91)
        for _ in range(3000):
            perm = prng.permutation(9).tolist()
            lab = nh.hdbscan_fit(X[list(perm)], 1, 3, method).labels_
            back = np.empty(9, dtype=np.int64)
            back[list(perm)] = lab
            perms[name_partition(back, names)] += 1
        d = nh.from_linkage(nh.hdbscan_fit(X, 1, 3, method)._single_linkage_tree_, 9)
        nary = name_partition(nh.head(d, 3, 1.0, method), names)
        print('%-4s | N-aire : [%s]' % (method, nary))
        print('     | sklearn, %d ordres d entree tires au hasard : %s' % (sum(perms.values()), dict(perms)))
        out[method] = dict(nary=nary, permutations=dict(perms))
    print('Lecture : b est a distance 3 de L et de R ; a 3, L, b, R fusionnent d un coup. La tete N-aire laisse b')
    print('en bruit (il sort du parent) ; sklearn le donne a L ou a R selon l ordre de Prim, donc selon l ordre d entree.')
    RESULT['F1_pont'] = out


# ------------------------------------------------------------------------------------------------------------
def fixture_phantom():
    section('F2. Cluster fantome et fausse scission (1D) : P = {0,1,2,3,6,9}, C = {100,...,103} ; min_samples = 1, mcs = 2')
    X = np.array([0, 1, 2, 3, 6, 9, 100, 101, 102, 103], dtype=float)[:, None]
    names = ['0', '1', '2', '3', '6', '9', 'a', 'b', 'c', 'd']
    out = {}
    for method in ('eom', 'leaf'):
        perms = collections.Counter()
        prng = np.random.default_rng(92)
        for _ in range(3000):
            perm = prng.permutation(10).tolist()
            lab = nh.hdbscan_fit(X[list(perm)], 1, 2, method).labels_
            back = np.empty(10, dtype=np.int64)
            back[list(perm)] = lab
            perms[name_partition(back, names)] += 1
        firsts = {}
        for first in range(10):
            perm = [first] + [i for i in range(10) if i != first]
            lab = nh.hdbscan_fit(X[perm], 1, 2, method).labels_
            back = np.empty(10, dtype=np.int64)
            back[perm] = lab
            firsts[names[first]] = name_partition(back, names)
        d = nh.from_linkage(nh.hdbscan_fit(X, 1, 2, method)._single_linkage_tree_, 10)
        nary = name_partition(nh.head(d, 2, 1.0, method), names)
        emul = {}
        for first in (0, 5, 6):
            perm = [first] + [i for i in range(10) if i != first]
            mst = sk_mst(X[perm], 1)
            for tname, o in tie_orders(mst, np.random.default_rng(7), shuffles=0).items():
                lab = nh.sklearn_tree_to_labels(sk_tree(mst, o), 2, method)
                back = np.empty(10, dtype=np.int64)
                back[perm] = lab
                emul['premier=%s, tri %s' % (names[first], tname)] = name_partition(back, names)
        print('%-4s | N-aire : [%s]' % (method, nary))
        print('     | sklearn selon le premier point d entree (le reste dans l ordre) :')
        for f, v in firsts.items():
            print('     |   premier = %s : %s' % (f, v))
        print('     | sklearn, %d ordres d entree tires au hasard : %s' % (sum(perms.values()), dict(perms)))
        print('     | emulation (fonctions de sklearn, seul l ordre des aretes ex aequo change) :')
        for key, v in emul.items():
            print('     |   %s : %s' % (key, v))
        out[method] = dict(nary=nary, by_first=firsts, permutations=dict(perms), emulation=emul)
    print('Lecture : a 3, B = {0,1,2,3}, {6} et {9} fusionnent d un coup (plateau). Si Prim part de 9, l arete 9-6')
    print('precede 6-3 : sklearn forme {6,9} (taille 2 = mcs) puis le scinde de B au meme niveau : {6,9} est un')
    print('cluster qui n est une composante a AUCUN niveau (fantome, stabilite nulle) et P perd sa masse au profit')
    print('de B (fausse scission). La tete N-aire garde P = {0,1,2,3,6,9} entier.')
    RESULT['F2_fantome'] = out


# ------------------------------------------------------------------------------------------------------------
def fixture_eps_quirk():
    section('F3. Epsilon egal a un niveau de naissance (transcription : numpy 2.5 fait echouer le code compile)')
    # Arbre binaire construit a la main : racine (10) -> E, F ; E (4) -> L1, Cc ; Cc (2) -> L2, L3 ; points
    # tombant de E entre 10 et 4 : x1, x2. Niveaux distincts : aucun plateau.
    # feuilles : L1 = {0,1,2} (fusion a 1), L2 = {3,4,5} (1), L3 = {6,7,8} (1), F = {9,10,11} (1), x1 = 12, x2 = 13
    rows = []
    n = 14
    nid = [n]

    def node(a, b, v, s):
        rows.append((a, b, v, s))
        nid[0] += 1
        return nid[0] - 1

    def trio(p, q, r):
        u = node(p, q, 1.0, 2)
        return node(u, r, 1.0, 3)

    L1, L2, L3, F = trio(0, 1, 2), trio(3, 4, 5), trio(6, 7, 8), trio(9, 10, 11)
    Cc = node(L2, L3, 2.0, 6)
    E0 = node(L1, Cc, 4.0, 9)
    E1 = node(E0, 12, 6.0, 10)
    E2 = node(E1, 13, 8.0, 11)
    node(E2, F, 10.0, 14)
    tree = np.array(rows, dtype=nh.HIERARCHY_dtype)
    d = nh.from_linkage(tree, n)
    out = {}
    for eps in (3.0, 4.0, 5.0):
        sk = skt.tree_to_labels(tree, 3, 'eom', False, eps)
        try:
            nh.sklearn_tree_to_labels(tree, 3, 'eom', False, eps)
            compiled = 'execute'
        except TypeError as e:
            compiled = 'TypeError (%s)' % e
        cons = nh.head(d, 3, 1.0, 'eom', eps, False, eps_mode='consistent')
        skm = nh.head(d, 3, 1.0, 'eom', eps, False, eps_mode='sklearn')
        names = [str(i) for i in range(12)] + ['x1', 'x2']
        print('eps=%.1f | sklearn (transcription) : %s' % (eps, name_partition(sk, names)))
        print('        | N-aire, regle coherente (>=) : %s' % name_partition(cons, names))
        print('        | N-aire, asymetrie de sklearn (< puis >) : %s' % name_partition(skm, names))
        print('        | code compile de sklearn sur cette machine : %s' % compiled)
        out['eps_%.1f' % eps] = dict(sklearn=name_partition(sk, names), consistent=name_partition(cons, names),
                                     sklearn_mode=name_partition(skm, names), compiled=compiled)
    print('Lecture : a eps = 4 = naissance de L1 et de Cc, sklearn garde L1 (4 < 4 faux) mais saute Cc (4 > 4 faux)')
    print('et remonte a E : il retient {L1, E}, emboites ; l etiquetage donne a E les points tombes de E (x1, x2).')
    print('La regle coherente retient {L1, Cc}. Hors egalite eps = niveau, les deux coincident.')
    RESULT['F3_epsilon'] = out


# ------------------------------------------------------------------------------------------------------------
def clouds_machine_dependence():
    section('C1. Petits nuages : etiquettes de sklearn selon l ordre de tri des aretes ex aequo (fonctions de sklearn)')
    rng = np.random.default_rng(20261004)
    from equivalence import cloud
    rows = []
    agg = collections.defaultdict(lambda: dict(cases=0, variable=0, points=0, n=0, m05_span=0.0, best_span=0.0,
                                               nary_vs_default=0))
    for family in ('blobs3d', 'grid3d', 'uniform3d'):
        for n in (100, 300):
            for rep in range(3):
                # verite terrain pour les familles a groupes
                X, gt = cloud_with_truth(family, n, rng)
                for k in (1, 2, 3, 5, 10):
                    mst = sk_mst(X, k)
                    orders = tie_orders(mst, rng)
                    ref = nh.hdbscan_fit(X, k, 2)._single_linkage_tree_
                    same_default = np.array_equal(np.asarray(ref), np.asarray(sk_tree(mst, orders['numpy_defaut'])))
                    if not same_default:
                        raise RuntimeError('reconstruction de _process_mst non conforme')
                    d = nh.from_linkage(ref, len(X))
                    for mcs in (5, 10, 20):
                        for method in ('eom', 'leaf'):
                            labs = {name: nh.sklearn_tree_to_labels(sk_tree(mst, o), mcs, method)
                                    for name, o in orders.items()}
                            nary = nh.head(d, mcs, 1.0, method)
                            parts = set(nh.canonical_partition(l) for l in labs.values())
                            key = (family, k, method)
                            a = agg[key]
                            a['cases'] += 1
                            a['n'] += len(X)
                            if len(parts) > 1:
                                a['variable'] += 1
                                base = labs['numpy_defaut']
                                a['points'] += max(nh.points_differing(base, l) for l in labs.values())
                            if not nh.same_partition(nary, labs['numpy_defaut']):
                                a['nary_vs_default'] += 1
                            if gt is not None:
                                sc = [nh.score_flat(l, gt) for l in labs.values()]
                                a['m05_span'] = max(a['m05_span'], max(s['m05'] for s in sc) - min(s['m05'] for s in sc))
                                a['best_span'] = max(a['best_span'],
                                                     max(s['best'] for s in sc) - min(s['best'] for s in sc))
    print('%-10s %3s %-5s %6s %10s %14s %16s %12s %12s' % ('famille', 'k', 'tete', 'cas', 'variables',
                                                            'points max/cas', 'N-aire != defaut', 'ecart m05', 'ecart best'))
    out = []
    for (family, k, method), a in sorted(agg.items()):
        pts = a['points'] / a['variable'] if a['variable'] else 0.0
        if family == 'uniform3d':
            spans = '%12s %12s' % ('n/a', 'n/a')
        else:
            spans = '%12.3f %12.3f' % (a['m05_span'], a['best_span'])
        print('%-10s %3d %-5s %6d %10d %14.1f %16d %s' % (family, k, method, a['cases'], a['variable'], pts,
                                                         a['nary_vs_default'], spans))
        out.append(dict(family=family, k=k, method=method, **a))
    print('Lecture : « variables » = cas ou au moins deux ordres de tri des ex aequo donnent deux partitions differentes ;')
    print('« points max/cas » = moyenne, sur ces cas, du plus grand nombre de points dont le bloc change ; ecarts = plus')
    print('grande difference de score (m05, best) entre ordres de tri, sur la verite terrain des groupes (uniform3d : sans')
    print('verite, n/a). Chaque cas : 8 ordres (defaut de numpy, stable, ex aequo inverses, 5 melanges des ex aequo).')
    RESULT['C1_ordres_de_tri'] = out


def cloud_with_truth(family, n, rng):
    if family == 'blobs3d':
        c = rng.uniform(0, 20, (4, 3))
        m = int(n * 0.9)
        lab = rng.integers(0, 4, m)
        X = c[lab] + rng.normal(0, 1.2, (m, 3))
        X = np.r_[X, rng.uniform(-3, 23, (n - m, 3))]
        gt = np.r_[lab, -np.ones(n - m, dtype=np.int64)]
        return X, gt
    if family == 'grid3d':
        c = rng.integers(0, 30, (3, 3))
        m = int(n * 0.9)
        lab = rng.integers(0, 3, m)
        X = c[lab] + np.round(rng.normal(0, 2.0, (m, 3)))
        X = np.r_[X, rng.integers(-3, 33, (n - m, 3))]
        gt = np.r_[lab, -np.ones(n - m, dtype=np.int64)]
        X, idx = np.unique(X, axis=0, return_index=True)
        gt = gt[idx]
        p = rng.permutation(len(X))
        return X[p].astype(np.float64), gt[p]
    X = rng.uniform(0, 10, (n, 3))
    return X, None


def main():
    print('sklearn', __import__('sklearn').__version__, 'numpy', np.__version__)
    fixture_triangles()
    fixture_bridge()
    fixture_phantom()
    fixture_eps_quirk()
    clouds_machine_dependence()
    with open('ties_out.json', 'w') as f:
        json.dump(RESULT, f, indent=1, sort_keys=True)


if __name__ == '__main__':
    main()
