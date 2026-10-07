#!/usr/bin/env python3
"""Fouille v2/v3 -> v11, idee v2_v3-01 : ESTIMATION (pas une mesure du moteur) du travail du filtre G1 de l'arbre
des boites du catalogue v11, avec et sans certificats scalaires l/u (source v3 : morsehgp3D_v3/audits/
NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md § 1, « Theoreme de localite hierarchique » :
l_C(x) = min sur la fermeture de la boite de |x - c|^2, u_C(x) = max ; liste A = { x : l_C(x) <= R },
R = statistique d'ordre des u_C).

Reproduction de l'arbre v11 (lu dans morsehgp3D_v11/src/catalogue/boxes.cpp, b87285378 = eb036dbe2 pour src/catalogue) :
  * Cloud trie par cle de Morton (bit i de x en 3i, y en 3i+1, z en 3i+2 ; src/cloud/morton.hpp), sites uniques ;
  * racine = enveloppe [min, max+1) ; reservoir = les min(|L|, 3K) sites de la liste parente les plus proches du
    centre de la boite (distance (2x-lo-hi)^2), ex aequo au premier rencontre (insertion stable, l. 16-39) ;
  * test G1 de domination (l. 74-76) : |x-lo|^2 - |w-lo|^2 > sum_i max(0, 2(hi_i-lo_i)(x_i-w_i)), arret a K succes ;
    tests comptes = temoins examines (l. 72, 78) ; x garde si moins de K dominateurs ;
  * ajustement a l'enveloppe des gardes (+1 en haut), intersection avec la boite, elagage si largeur nulle (l. 131-136) ;
  * coupe au milieu du plus long cote (premier axe en cas d'egalite), feuille si |L| <= leaf_size ou largeur <= 1
    (l. 143-155) ; leaf_size = 16 (reglage mesure, AB7).
Validation : les compteurs nodes, leaves, filter_tests, max_leaf, max_depth doivent egaler ceux du recu AB7
(t_new_lidar_ng00_w1_r0.stdout : 783071, 353456, 379366675, 16, 36).

Certificats scalaires evalues (aucune decision flottante : tout est entier exact en Python) :
  * DEDANS : x dans la fermeture [lo, hi] => garde (l(x) = 0 : aucun w ne peut avoir l(w) < 0) ;
  * GARDE : #{w : l(w) < l(x) et u(w) < u(x)} < K => garde (domination => l(w) < l(x) et u(w) < u(x)) ;
  * RETRAIT : #{w : u(w) < l(x)} >= K => retire (chacun de ces w domine x strictement partout) ;
  * sinon : tests par paires restreints aux temoins candidats, dans l'ordre du reservoir, arret a K.
Chaque certificat est controle contre le test par paires (aucune divergence toleree : code 1 sinon).

Usage : python3 -B v2_v3_sim_g1.py <ng00|ng01|ng02> [K] [leaf_size] [profondeur_max]
Avec profondeur_max, seuls les noeuds de profondeur <= profondeur_max sont simules (le haut de l'arbre, celui du
preambule quasi seriel) ; les compteurs globaux ne valent alors que pour ce haut. Le tableau par profondeur publie :
noeuds, tests G1, tests sur sites DEDANS (jamais retirables), tests du noeud le plus charge et sa part DEDANS.
"""
import sys
import time

import numpy as np

DATA = '/workspaces/E-HGP/build/v11-full-data-20261002/lidar_{}.u32le'


def spread21(v):
    v = v.astype(np.uint64) & np.uint64(0x1FFFFF)
    v = (v | (v << np.uint64(32))) & np.uint64(0x001F00000000FFFF)
    v = (v | (v << np.uint64(16))) & np.uint64(0x001F0000FF0000FF)
    v = (v | (v << np.uint64(8))) & np.uint64(0x100F00F00F00F00F)
    v = (v | (v << np.uint64(4))) & np.uint64(0x10C30C30C30C30C3)
    v = (v | (v << np.uint64(2))) & np.uint64(0x1249249249249249)
    return v


def main():
    scene = sys.argv[1]
    K = int(sys.argv[2]) if len(sys.argv) > 2 else 5
    leaf_size = int(sys.argv[3]) if len(sys.argv) > 3 else 16
    depth_limit = int(sys.argv[4]) if len(sys.argv) > 4 else 10 ** 9
    per_depth = {}  # profondeur -> [noeuds, tests, tests DEDANS, max tests noeud, tests DEDANS de ce noeud, sites]
    raw =np.fromfile(DATA.format(scene), dtype='<u4').reshape(-1, 3).astype(np.int64)
    key = spread21(raw[:, 0]) | (spread21(raw[:, 1]) << np.uint64(1)) | (spread21(raw[:, 2]) << np.uint64(2))
    order = np.argsort(key, kind='stable')
    pts = raw[order]
    if len(np.unique(key)) != len(key):
        print('positions dupliquees : hors du cas simule')
        return 2
    cap = 3 * K
    t0 = time.time()
    st = dict(nodes=0, leaves=0, filter_tests=0, max_leaf=0, max_depth=0, sites_tested=0,
              inside=0, keep_cert=0, remove_cert=0, residual_sites=0, residual_tests=0,
              kept=0, removed=0, inside_tests_base=0, keepcert_tests_base=0, removecert_tests_base=0,
              residual_tests_base=0, cand_sum=0, mismatches=0)
    depth_tests = {}
    lo0 = pts.min(0)
    hi0 = pts.max(0) + 1
    stack = [(np.arange(len(pts)), lo0, hi0, 0)]
    while stack:
        lst, lo, hi, depth = stack.pop()
        st['nodes'] += 1
        st['max_depth'] = max(st['max_depth'], depth)
        P = pts[lst]
        # reservoir : distance doublee au centre, ex aequo au premier rencontre
        dc = ((2 * P - lo - hi) ** 2).sum(1)
        sel = np.argsort(dc, kind='stable')[:min(len(lst), cap)]
        W = P[sel]
        width = hi - lo
        xr = P - lo
        xs = 2 * width * xr
        xq = (xr * xr).sum(1)
        wr = W - lo
        ws = 2 * width * wr
        wq = (wr * wr).sum(1)
        right = np.maximum(0, xs[:, None, :] - ws[None, :, :]).sum(2)
        dom = (xq[:, None] - wq[None, :]) > right  # dom[i, j] : temoin j domine le site i
        cum = np.cumsum(dom, axis=1)
        found = np.minimum(cum[:, -1], K)
        reach = cum >= K
        tests = np.where(reach.any(1), reach.argmax(1) + 1, dom.shape[1])
        st['filter_tests'] += int(tests.sum())
        depth_tests[depth] = depth_tests.get(depth, 0) + int(tests.sum())
        keep = found < K
        # certificats scalaires, entiers exacts
        dlo = lo - P
        dhi = P - (hi)
        gap = np.maximum(0, np.maximum(dlo, dhi))  # distance a [lo, hi] ferme, par axe
        l = (gap * gap).sum(1)
        far = np.maximum(np.abs(P - lo), np.abs(P - hi))
        u = (far * far).sum(1)
        lw = l[sel]
        uw = u[sel]
        inside = l == 0
        cand = (lw[None, :] < l[:, None]) & (uw[None, :] < u[:, None])
        ncand = cand.sum(1)
        keep_cert = ncand < K
        nrem = (uw[None, :] < l[:, None]).sum(1)
        remove_cert = nrem >= K
        # coherence avec le test par paires
        if np.any(keep_cert & ~keep) or np.any(remove_cert & keep) or np.any(dom & ~cand):
            st['mismatches'] += 1
        resid = ~keep_cert & ~remove_cert
        # tests par paires restreints aux candidats, ordre du reservoir, arret a K
        rd = dom & cand
        rcum = np.cumsum(rd, axis=1)
        ccum = np.cumsum(cand, axis=1)
        rreach = rcum >= K
        rtests = np.where(rreach.any(1), ccum[np.arange(len(lst)), rreach.argmax(1)], ncand)
        st['sites_tested'] += len(lst)
        st['inside'] += int(inside.sum())
        st['keep_cert'] += int(keep_cert.sum())
        st['remove_cert'] += int(remove_cert.sum())
        st['residual_sites'] += int(resid.sum())
        st['residual_tests'] += int(rtests[resid].sum())
        st['cand_sum'] += int(ncand.sum())
        st['kept'] += int(keep.sum())
        st['removed'] += int((~keep).sum())
        st['inside_tests_base'] += int(tests[inside].sum())
        row = per_depth.setdefault(depth, [0, 0, 0, 0, 0, 0])
        node_tests, node_inside = int(tests.sum()), int(tests[inside].sum())
        row[0] += 1
        row[1] += node_tests
        row[2] += node_inside
        row[5] += len(lst)
        if node_tests > row[3]:
            row[3], row[4] = node_tests, node_inside
        st['keepcert_tests_base'] += int(tests[keep_cert].sum())
        st['removecert_tests_base'] += int(tests[remove_cert].sum())
        st['residual_tests_base'] += int(tests[resid].sum())
        kept = lst[keep]
        if len(kept) == 0:
            continue
        KP = pts[kept]
        alo = np.maximum(KP.min(0), lo)
        ahi = np.minimum(KP.max(0) + 1, hi)
        if np.any(alo >= ahi):
            continue
        w = ahi - alo
        axis = 0
        for i in (1, 2):
            if w[i] > w[axis]:
                axis = i
        if len(kept) <= leaf_size or w[axis] <= 1:
            st['leaves'] += 1
            st['max_leaf'] = max(st['max_leaf'], len(kept))
            continue
        if depth + 1 > depth_limit:
            continue
        mid = alo[axis] + w[axis] // 2
        left_hi = ahi.copy()
        left_hi[axis] = mid
        right_lo = alo.copy()
        right_lo[axis] = mid
        # ordre de la pile : gauche traitee d'abord (sans effet sur les comptes)
        stack.append((kept, right_lo, ahi.copy(), depth + 1))
        stack.append((kept, alo.copy(), left_hi, depth + 1))
    st['seconds'] = round(time.time() - t0, 1)
    print(f'scene={scene} K={K} leaf_size={leaf_size} sites={len(pts)}')
    for k, v in st.items():
        print(f'  {k} = {v}')
    tot = st['filter_tests']
    print(f'  part des tests de base sur sites DEDANS : {st["inside_tests_base"] / tot:.4f}')
    print(f'  part des tests de base sur sites certifies GARDE : {st["keepcert_tests_base"] / tot:.4f}')
    print(f'  part des tests de base sur sites certifies RETRAIT : {st["removecert_tests_base"] / tot:.4f}')
    print(f'  tests par paires restants / base : {st["residual_tests"] / tot:.4f}')
    print('  profondeur | noeuds | sites | tests G1 | dont DEDANS | noeud le plus charge : tests / dont DEDANS')
    cum = cum_in = crit = crit_in = 0
    for d in sorted(per_depth):
        r = per_depth[d]
        cum += r[1]
        cum_in += r[2]
        crit += r[3]
        crit_in += r[4]
        if d <= 16:
            print(f'  {d:2d} | {r[0]} | {r[5]} | {r[1]} | {r[2]} ({r[2] / max(1, r[1]):.3f}) | {r[3]} / {r[4]}'
                  f' ({r[4] / max(1, r[3]):.3f}) | cumul {cum} dont DEDANS {cum_in} ({cum_in / max(1, cum):.3f})'
                  f' ; chaine des plus charges {crit} dont DEDANS {crit_in} ({crit_in / max(1, crit):.3f})')
    top = sorted(depth_tests.items())
    acc = 0
    for d, v in top:
        acc += v
        if d in (0, 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30, 36):
            print(f'  profondeur <= {d:2d} : tests cumules {acc} ({acc / tot:.4f})')
    return 1 if st['mismatches'] else 0


if __name__ == '__main__':
    sys.exit(main())
