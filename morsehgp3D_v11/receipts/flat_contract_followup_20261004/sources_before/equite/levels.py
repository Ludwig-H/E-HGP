#!/usr/bin/env python3
"""Convention de niveau : rayon de la tour contre distance d'atteignabilite mutuelle de HDBSCAN.

L1  facteur global : sklearn sur X et sur X/2 (mise a l'echelle exacte en flottant) rend les memes etiquettes ;
L2  la tete N-aire est invariante par multiplication de tous les niveaux par c > 0, pour tout z (proposition H) ;
L3  mais elle depend de la forme de phi : z = 1 contre z = 3 sur le meme arbre ;
L4  alpha de sklearn : a min_samples = 1, alpha = 2 n'est qu'un facteur global (memes etiquettes) ; a min_samples
    >= 2, alpha change la filtration (le terme de coeur n'est pas divise) ; L4b : sauf dans la voie 'brute', ou
    sklearn divise toute la matrice avant les distances de coeur (alpha n'y est qu'un facteur global) ;
L5  temps de coeur : d_k(x) de la tour (oracle) = distance de coeur de sklearn a min_samples = k (point compris) ;
L6  etalonnage k = 1 : H^r_1 = liaison simple en rayon = arbre de HDBSCAN(min_samples = 1) / 2 ; les tetes N-aires
    des deux arbres rendent les memes etiquettes pour tout mcs, toute selection et tout z ;
L7  Fait 10 de la these (u_RSL = 1/2 min max d_mreach) contre sa Remarque (coeur r_K <= r, paire d <= 2r).

Usage : PYTHONDONTWRITEBYTECODE=1 python3 -B -u levels.py > levels_out.txt
"""
import json
import warnings

import numpy as np

import nary_head as nh
import tower

warnings.filterwarnings('ignore')
OUT = {}


def section(title):
    print()
    print('=' * 100)
    print(title)
    print('=' * 100)


def blobs(rng, n, dim=3):
    c = rng.uniform(0, 20, (4, dim))
    m = int(n * 0.9)
    lab = rng.integers(0, 4, m)
    X = c[lab] + rng.normal(0, 1.2, (m, dim))
    return np.r_[X, rng.uniform(-3, 23, (n - m, dim))]


def scaled_dendrogram(d, c):
    e = nh.Dendrogram(d.n)
    for v in range(d.n, d.nodes):
        e.add(d.children[v], d.value[v] * c, d.rank[v])
    return e


def l1_l2_l3(rng):
    section('L1-L3. Facteur global et forme de phi')
    s = dict(l1_cases=0, l1_same=0, l2_cases=0, l2_same=0, l3_cases=0, l3_diff=0, l1_x3_cases=0, l1_x3_same=0)
    for rep in range(6):
        X = blobs(rng, 200)
        for k in (1, 2, 3, 5, 10):
            for mcs in (5, 10, 20):
                for method in ('eom', 'leaf'):
                    a = nh.hdbscan_fit(X, k, mcs, method).labels_
                    b = nh.hdbscan_fit(X * 0.5, k, mcs, method).labels_
                    c3 = nh.hdbscan_fit(X * 3.0, k, mcs, method).labels_
                    s['l1_cases'] += 1
                    s['l1_same'] += int(np.array_equal(a, b))
                    s['l1_x3_cases'] += 1
                    s['l1_x3_same'] += int(nh.same_partition(a, c3))
                    d = nh.from_linkage(nh.hdbscan_fit(X, k, mcs, method)._single_linkage_tree_, len(X))
                    for z in (1.0, 2.0, 3.0):
                        base = nh.head(d, mcs, z, method)
                        for c in (0.5, 2.0, 3.7):
                            s['l2_cases'] += 1
                            s['l2_same'] += int(nh.same_partition(base, nh.head(scaled_dendrogram(d, c), mcs, z,
                                                                                    method)))
                    s['l3_cases'] += 1
                    s['l3_diff'] += int(not nh.same_partition(nh.head(d, mcs, 1.0, method),
                                                              nh.head(d, mcs, 3.0, method)))
    print('L1 sklearn, X contre X/2 (exact) : %d/%d etiquettes identiques' % (s['l1_same'], s['l1_cases']))
    print('   sklearn, X contre 3X (arrondis) : %d/%d partitions identiques' % (s['l1_x3_same'], s['l1_x3_cases']))
    print('L2 tete N-aire, niveaux multiplies par c in {0.5, 2, 3.7}, z in {1, 2, 3} : %d/%d partitions identiques' %
          (s['l2_same'], s['l2_cases']))
    print('L3 tete N-aire, z = 1 contre z = 3 sur le meme arbre : %d/%d partitions differentes' %
          (s['l3_diff'], s['l3_cases']))
    OUT['L1_L3'] = s


def l4(rng):
    section('L4. alpha de sklearn (public) : facteur global a min_samples = 1, autre filtration au-dela')
    s = {}
    for k in (1, 2, 3, 5, 10):
        same_lab = cases = same_tree = 0
        for rep in range(6):
            X = blobs(rng, 200)
            for mcs in (5, 10, 20):
                for method in ('eom', 'leaf'):
                    m1 = nh.hdbscan_fit(X, k, mcs, method, alpha=1.0)
                    m2 = nh.hdbscan_fit(X, k, mcs, method, alpha=2.0)
                    cases += 1
                    same_lab += int(nh.same_partition(m1.labels_, m2.labels_))
                    d1 = nh.from_linkage(m1._single_linkage_tree_, len(X))
                    d2 = nh.from_linkage(m2._single_linkage_tree_, len(X))
                    b1 = sorted(tuple(d1.leaves(v)) for v in range(d1.n, d1.nodes))
                    b2 = sorted(tuple(d2.leaves(v)) for v in range(d2.n, d2.nodes))
                    same_tree += int(b1 == b2)
        s[k] = dict(cases=cases, same_labels=same_lab, same_blocks=same_tree)
        print('min_samples = %2d : etiquettes identiques %d/%d ; memes blocs (arbre N-aire) %d/%d' %
              (k, same_lab, cases, same_tree, cases))
    OUT['L4_alpha'] = s
    # L4b : la voie 'brute' divise la matrice AVANT les distances de coeur (_hdbscan_brute, l. 254) : alpha n'y est
    # qu'un facteur global ; la voie 'kd_tree' (defaut 'auto' en euclidien dense) ne divise que les paires.
    tb = dict(cases=0, brute_a2_eq_brute_a1=0, kd_a2_eq_brute_a2=0, kd_a1_eq_brute_a1=0)
    for rep in range(4):
        X = blobs(rng, 150)
        for k in (2, 3, 5):
            for mcs in (5, 10):
                b1 = nh.hdbscan_fit(X, k, mcs, 'eom', alpha=1.0, algorithm='brute').labels_
                b2 = nh.hdbscan_fit(X, k, mcs, 'eom', alpha=2.0, algorithm='brute').labels_
                k1 = nh.hdbscan_fit(X, k, mcs, 'eom', alpha=1.0, algorithm='kd_tree').labels_
                k2 = nh.hdbscan_fit(X, k, mcs, 'eom', alpha=2.0, algorithm='kd_tree').labels_
                tb['cases'] += 1
                tb['brute_a2_eq_brute_a1'] += int(nh.same_partition(b1, b2))
                tb['kd_a2_eq_brute_a2'] += int(nh.same_partition(k2, b2))
                tb['kd_a1_eq_brute_a1'] += int(nh.same_partition(k1, b1))
    print('L4b (min_samples 2-5) : brute alpha=2 = brute alpha=1 : %d/%d ; kd_tree alpha=2 = brute alpha=2 : %d/%d ;'
          ' kd_tree alpha=1 = brute alpha=1 : %d/%d' % (tb['brute_a2_eq_brute_a1'], tb['cases'],
                                                       tb['kd_a2_eq_brute_a2'], tb['cases'],
                                                       tb['kd_a1_eq_brute_a1'], tb['cases']))
    OUT['L4b_alpha_algorithm'] = tb


def l5(rng):
    section('L5. Temps de coeur : oracle de la tour (d_k, point compris) contre sklearn (min_samples = k)')
    from sklearn.neighbors import NearestNeighbors
    tot = eq = 0
    worst = 0.0
    for rep in range(30):
        n = int(rng.integers(6, 11))
        pts = [tuple(int(x) for x in rng.integers(0, 12, 3)) for _ in range(n)]
        if len(set(pts)) < n:
            continue
        X = np.array(pts, dtype=np.float64)
        for k in range(1, min(n, 5) + 1):
            h = tower.hierarchy(pts, k, stage='A')
            core_sk = NearestNeighbors(n_neighbors=k).fit(X).kneighbors(X, k)[0][:, -1]
            for i in range(n):
                tot += 1
                eq += int(h.core[i] == core_sk[i])
                worst = max(worst, abs(h.core[i] - core_sk[i]))
    print('%d/%d temps de coeur egaux au bit pres (ecart maximal %.3g)' % (eq, tot, worst))
    OUT['L5_coeur'] = dict(total=tot, equal=eq, worst=worst)


def l6(rng):
    section('L6. Etalonnage k = 1 : H^r_1 (oracle de la tour) contre HDBSCAN(min_samples = 1)')
    s = dict(clouds=0, same_blocks=0, ratio_two=0, head_cases=0, head_same=0, blocks_checked=0)
    for rep in range(24):
        n = int(rng.integers(7, 13)) if rep < 16 else int(rng.integers(16, 25))
        pts = list(set(tuple(int(x) for x in rng.integers(0, 40, 3)) for _ in range(n)))
        n = len(pts)
        X = np.array(pts, dtype=np.float64)
        h = tower.hierarchy(pts, 1, stage='A' if n <= 12 else 'B')
        dt = tower.dendrogram(h)
        s['blocks_checked'] += tower.check_blocks(h, dt)
        dh = nh.from_linkage(nh.hdbscan_fit(X, 1, 2)._single_linkage_tree_, n)
        bt = sorted(tuple(dt.leaves(v)) for v in range(dt.n, dt.nodes))
        bh = sorted(tuple(dh.leaves(v)) for v in range(dh.n, dh.nodes))
        s['clouds'] += 1
        s['same_blocks'] += int(bt == bh)
        vt = {tuple(dt.leaves(v)): dt.value[v] for v in range(dt.n, dt.nodes)}
        vh = {tuple(dh.leaves(v)): dh.value[v] for v in range(dh.n, dh.nodes)}
        s['ratio_two'] += int(all(abs(vh[b] - 2.0 * vt[b]) <= 1e-12 * vh[b] for b in vt if b in vh))
        for mcs in (2, 3, 4):
            for method in ('eom', 'leaf'):
                for z in (1.0, 3.0):
                    s['head_cases'] += 1
                    s['head_same'] += int(nh.same_partition(nh.head(dt, mcs, z, method), nh.head(dh, mcs, z, method)))
    print('%d nuages (n de 7 a 24, coordonnees entieres) : memes blocs %d/%d ; niveaux HDBSCAN = 2 x rayon %d/%d' %
          (s['clouds'], s['same_blocks'], s['clouds'], s['ratio_two'], s['clouds']))
    print('blocs de l oracle recoupes (trois routes) a %d niveaux FULL' % s['blocks_checked'])
    print('tetes N-aires (mcs 2-4, eom/leaf, z = 1 et 3) : %d/%d partitions identiques' % (s['head_same'],
                                                                                         s['head_cases']))
    OUT['L6_k1'] = s


def l7():
    section('L7. These, Fait 10 contre Remarque (p. 36) : ou va le facteur 1/2 ?')
    # deux points a distance 2, chacun le plus proche voisin de l'autre ; K = 2 (point compris) : r_K = 2
    rk, d = 2.0, 2.0
    fait10 = 0.5 * max(rk, rk, d)
    remarque = max(rk, rk, d / 2.0)
    print('x, y a distance 2, r_2(x) = r_2(y) = 2 : Fait 10 -> u = 1/2 max(2, 2, 2) = %.1f ; Remarque (r_K <= r et' % fait10)
    print('d <= 2r) -> premier r = max(2, 2, 1) = %.1f. Le Fait 10 divise aussi le coeur par 2 (facteur global sur' % remarque)
    print('alpha = 1) ; la Remarque est alpha = 2 de sklearn. Ce sont deux filtrations differentes des que K >= 2.')
    OUT['L7'] = dict(fait10=fait10, remarque=remarque)


def main():
    print('sklearn', __import__('sklearn').__version__, 'numpy', np.__version__)
    rng = np.random.default_rng(20261004)
    l1_l2_l3(rng)
    l4(rng)
    l5(rng)
    l6(rng)
    l7()
    with open('levels_out.json', 'w') as f:
        json.dump(OUT, f, indent=1, sort_keys=True, default=str)


if __name__ == '__main__':
    main()
