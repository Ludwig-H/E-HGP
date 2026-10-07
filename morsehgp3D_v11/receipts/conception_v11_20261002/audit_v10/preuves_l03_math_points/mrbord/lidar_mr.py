#!/usr/bin/env python3
"""Audit L03 : meilleur IoU (sens PQ des demos Zoltan : points void exclus) des objets suivis A, B, C dans la
hierarchie d'atteignabilite mutuelle du depot (temoin tests/head/mreach.cpp), entrees coeur et bord, alpha 1 et 2.
Entrees : scenes preparees du harnais v10 (cache hors depot, lecture seule). Sorties : IoU et comptes seulement
(aucune coordonnee)."""
import os, sys, subprocess, json, time
import numpy as np
sys.dont_write_bytecode = True
HERE = '/tmp/v11-audit/l03_math_points/mrbord'
S = '/workspaces/E-HGP/build/v10-lidar-demos/_cache/scenes'
sys.path.insert(0, HERE)
from niveau_b import read_tree  # noqa: E402


def best_objects(path, w_eval, C):
    levels, nodes, pts = read_tree(path)
    N = len(nodes)
    rank, parent = nodes[:, 0], nodes[:, 1]
    M = C.shape[1]
    m = C.sum(axis=0).astype(np.int64)
    order = np.lexsort((pts[:, 2], pts[:, 1]))
    pn, pr, pid = pts[order, 1], pts[order, 2], pts[order, 0]
    start = np.searchsorted(pn, np.arange(N), side='left')
    end = np.searchsorted(pn, np.arange(N), side='right')
    cnt = np.zeros((N, M), dtype=np.int64)
    size = np.zeros(N, dtype=np.int64)
    best = [(0, 1, 0, 0)] * M   # (num, den, inter, evald)

    def ev(c, s):
        for o in range(M):
            if c[o] > 0:
                num, den = int(c[o]), int(s + m[o] - c[o])
                if num * best[o][1] > best[o][0] * den:
                    best[o] = (num, den, int(c[o]), int(s))

    for v in range(N):
        c = cnt[v]
        s = size[v]
        a, b = start[v], end[v]
        if a == b:
            if c.any():
                ev(c, s)
        else:
            if c.any() and pr[a] > rank[v]:
                ev(c, s)
            i = a
            while i < b:
                j = i
                touched = False
                while j < b and pr[j] == pr[i]:
                    p = pid[j]
                    row = C[p]
                    if row.any():
                        c += row
                        touched = True
                    s += w_eval[p]
                    j += 1
                if c.any():
                    ev(c, s)
                i = j
            size[v] = s
        p = parent[v]
        if p >= 0:
            cnt[p] += c
            size[p] += s
    return best


def main():
    demo, ks = sys.argv[1], [int(k) for k in sys.argv[2].split(',')]
    variants = sys.argv[3].split(',')
    threads = sys.argv[4] if len(sys.argv) > 4 else '2'
    sites = os.path.join(S, demo, 'sites.u32le')
    n = os.path.getsize(sites) // 12
    obj = np.fromfile(os.path.join(S, demo, 'objets.u32le'), dtype='<u4').reshape(n, -1)
    w_eval, C = obj[:, 0].astype(np.int64), obj[:, 1:].astype(np.int64)
    out = []
    for K in ks:
        for var in variants:
            alpha, ent = var[2], ('border' if var.endswith('bord') else 'core')
            tree = os.path.join(HERE, 'tmp_%s_%s_K%d.tree' % (demo[:2], var, K))
            t0 = time.time()
            r = subprocess.run([os.path.join(HERE, 'export_mreach_tree'), sites, tree, '--k=%d' % K, '--alpha=' + alpha, '--entry=' + ent, '--threads=' + threads], capture_output=True, text=True)
            if r.returncode != 0:
                print('echec', demo, K, var, r.returncode, r.stderr[-200:])
                continue
            t1 = time.time()
            best = best_objects(tree, w_eval, C)
            os.remove(tree)
            row = dict(demo=demo, K=K, source=var, sites=n, iou=[round(b[0] / b[1], 4) for b in best], fractions=['%d/%d' % (b[0], b[1]) for b in best],
                       inter=[b[2] for b in best], taille_evaluee=[b[3] for b in best], export_s=round(t1 - t0, 1), eval_s=round(time.time() - t1, 1))
            out.append(row)
            print(json.dumps(row), flush=True)
    json.dump(out, open(os.path.join(HERE, 'lidar_mr_%s_K%s.json' % (demo[:2], '-'.join(map(str, ks)))), 'w'))


if __name__ == '__main__':
    main()
