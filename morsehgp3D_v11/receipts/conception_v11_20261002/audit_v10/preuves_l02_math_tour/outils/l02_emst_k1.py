#!/usr/bin/env python3
"""Invariant global K = 1 (hors depot) : la foret d'ordre 1 de la v10 contre l'arbre couvrant minimal euclidien.

Fait invoque : a K = 1, Gamma_1(a) est le graphe geometrique (aretes |uv|^2 / 4 <= a) ; le multiensemble des poids
d'un arbre couvrant minimal est unique ; une multifusion a c enfants au niveau a vaut c - 1 aretes de poids 4a.
Juge independant : sklearn.cluster.HDBSCAN(min_samples=1) (Prim exact sur les distances euclidiennes ; la racine
carree en double est strictement croissante sur les entiers < 2^38, donc l'ordre des aretes est l'ordre exact).

    python3 l02_emst_k1.py BUILD_DIR IN.u32le [threads]
Sortie : une ligne JSON. Code 0 si les deux multiensembles sont egaux, 1 sinon.
"""
import json
import os
import subprocess
import sys
import tempfile
import time
from collections import Counter
from fractions import Fraction

import numpy as np
from sklearn.cluster import HDBSCAN


def main():
    build, src = sys.argv[1], sys.argv[2]
    threads = sys.argv[3] if len(sys.argv) > 3 else '2'
    raw = np.fromfile(src, dtype='<u4').reshape(-1, 3)
    X = np.unique(raw, axis=0)
    n = len(X)
    with tempfile.TemporaryDirectory(dir=os.environ.get('L02_TMP')) as tmp:
        dump = os.path.join(tmp, 'k1.txt')
        t0 = time.time()
        r = subprocess.run([os.path.join(build, 'mhgp10_tower'), src, '--k=1', '--threads=' + threads,
                            '--dump=' + dump], capture_output=True, text=True)
        t_tower = time.time() - t0
        if r.returncode != 0:
            print(json.dumps(dict(status='refus', code=r.returncode, out=r.stdout.strip()[:300])))
            return 2
        parent, level = [], []
        leaf_of = {}  # coordonnees -> noeud feuille (attache core a K = 1 : le site lui-meme, niveau 0)
        for line in open(dump):
            t = line.split()
            if t[0] == 'node':
                parent.append(int(t[2]))
                level.append((int(t[3]), int(t[4])))
            elif t[0] == 'point':
                leaf_of[(int(t[1]), int(t[2]), int(t[3]))] = int(t[4])
    nn = len(parent)
    kids = [0] * nn
    roots = 0
    for v, p in enumerate(parent):
        if p < 0:
            roots += 1
        else:
            kids[p] += 1
    tower_w = Counter()
    births = merges = ar3 = armax = 0
    for v in range(nn):
        if kids[v] == 0:
            births += 1
            if level[v][0] != 0:
                print(json.dumps(dict(status='ecart', why='feuille K=1 de niveau non nul', node=v)))
                return 1
            continue
        merges += 1
        ar3 += kids[v] >= 3
        armax = max(armax, kids[v])
        w = Fraction(level[v][0], level[v][1]) * 4
        if w.denominator != 1:
            print(json.dumps(dict(status='ecart', why='4 * niveau non entier', node=v, level=str(w / 4))))
            return 1
        tower_w[int(w)] += kids[v] - 1
    t0 = time.time()
    h = HDBSCAN(min_cluster_size=2, min_samples=1, algorithm='kd_tree', copy=True).fit(X.astype(np.float64))
    t_sk = time.time() - t0
    d = np.asarray(h._single_linkage_tree_['value'], dtype=np.float64)
    s = np.rint(d * d).astype(np.int64)
    if not np.all(np.abs(d * d - s) < 1e-3):
        print(json.dumps(dict(status='ecart', why='poids sklearn non entier au carre')))
        return 1
    sk_w = Counter(int(v) for v in s)
    equal = tower_w == sk_w
    # Structure complete : famille laminaire des noeuds N-aires (niveau, empreinte de l'ensemble des feuilles, taille).
    # Empreinte = somme modulo 2^64 d'un alea 64 bits par site : deux ensembles differents coincident avec
    # probabilite ~ 2^-64 par paire.
    rng = np.random.default_rng(20261002)
    hx = rng.integers(0, 2**63, size=n, dtype=np.int64).astype(np.uint64)
    M = (1 << 64) - 1
    th = [0] * nn
    ts = [0] * nn
    structural = None
    if len(leaf_of) == n:
        for i in range(n):
            v = leaf_of[(int(X[i, 0]), int(X[i, 1]), int(X[i, 2]))]
            th[v] = int(hx[i])
            ts[v] = 1
        for v in range(nn):  # enfants avant parents
            p = parent[v]
            if p >= 0:
                th[p] = (th[p] + th[v]) & M
                ts[p] += ts[v]
        tower_nodes = Counter()
        for v in range(nn):
            if kids[v]:
                tower_nodes[(int(Fraction(level[v][0], level[v][1]) * 4), th[v], ts[v])] += 1
        slt = h._single_linkage_tree_
        L, R = slt['left_node'], slt['right_node']
        m = len(L)
        sh = [int(v) for v in hx] + [0] * m
        ss = [1] * n + [0] * m
        par = [-1] * (n + m)
        for i in range(m):
            a, b = int(L[i]), int(R[i])
            sh[n + i] = (sh[a] + sh[b]) & M
            ss[n + i] = ss[a] + ss[b]
            par[a] = par[b] = n + i
        sk_nodes = Counter()
        for i in range(m):
            q = par[n + i]
            if q >= 0 and s[q - n] == s[i]:
                continue  # absorbe par son parent de meme poids (plateau)
            sk_nodes[(int(s[i]), sh[n + i], ss[n + i])] += 1
        structural = tower_nodes == sk_nodes
    out = dict(status='ok' if equal else 'ecart', input=os.path.basename(src), sites=n, nodes=nn, births=births, roots=roots,
               merges=merges, merges_arity_ge3=int(ar3), arity_max=int(armax), tower_edges=sum(tower_w.values()),
               emst_edges=sum(sk_w.values()), distinct_weights=len(sk_w), equal_multisets=equal,
               equal_nary_trees=structural, nary_nodes_tower=(sum(tower_nodes.values()) if structural is not None else None),
               nary_nodes_slt=(sum(sk_nodes.values()) if structural is not None else None),
               sum_w_tower=sum(k * v for k, v in tower_w.items()), sum_w_emst=sum(k * v for k, v in sk_w.items()),
               tower_s=round(t_tower, 2), sklearn_s=round(t_sk, 2))
    if not equal:
        diff = (tower_w - sk_w) + (sk_w - tower_w)
        out['diff_examples'] = sorted(diff.items())[:6]
    print(json.dumps(out))
    return 0 if equal and structural and births == n and roots == 1 else 1


if __name__ == '__main__':
    sys.exit(main())
