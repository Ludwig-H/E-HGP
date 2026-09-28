"""Exploration DEV (graines dev) : la meme tete (condensation N-aire EOM, lambda = r^-z) sur la tour C n X et sur
l'atteignabilite mutuelle MR_alpha (alpha = 1, 2), pour z dans {0.25, 0.5, 1, zhat}. Hors depot, hors produit.

Port Python exact de src/head/head.cpp (condense + cluster) ; valide contre les etiquettes C++ (z = 1 et zhat).
MR_alpha : poids d'arete entier exact max(d^2 / alpha^2, core2(x), core2(y)), point ne a core2 (comme
tests/head/mreach.cpp), Kruskal par plateaux (multifusions non binarisees).
"""
import argparse
import csv
import math
import os
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ProcessPoolExecutor, as_completed
from fractions import Fraction

import numpy as np
from scipy.sparse.csgraph import minimum_spanning_tree
from scipy.spatial import cKDTree

SYN = '/workspaces/E-HGP/build/v10-bench-4a3d09d8a/src/morsehgp3D_v10/bench/synthetic'
sys.path.insert(0, SYN)
import methods  # noqa: E402
import metrics  # noqa: E402
import run_campaign  # noqa: E402
import scenes  # noqa: E402


class Dendro:
    def __init__(self, level, node_rank, parent, point_node, point_rank, point_weight):
        self.level = level
        self.node_rank = node_rank
        self.parent = parent
        self.point_node = point_node
        self.point_rank = point_rank
        self.point_weight = point_weight
        n = len(node_rank)
        self.children = [[] for _ in range(n)]
        for v in range(n):
            if parent[v] >= 0:
                self.children[parent[v]].append(v)


def read_tree(path):
    with open(path) as f:
        tok = f.read().split('\n')
    i = 0
    L = int(tok[i].split()[1]); i += 1
    level = [float(tok[i + j]) for j in range(L)]; i += L
    N = int(tok[i].split()[1]); i += 1
    node_rank, parent = [], []
    for j in range(N):
        a, b = tok[i + j].split()
        node_rank.append(int(a)); parent.append(int(b))
    i += N
    P = int(tok[i].split()[1]); i += 1
    point_node = [0] * P; point_rank = [0] * P; point_weight = [1] * P
    for j in range(P):
        x, v, r, w = map(int, tok[i + j].split())
        point_node[x] = v; point_rank[x] = r; point_weight[x] = w
    return Dendro(level, node_rank, parent, point_node, point_rank, point_weight)


def lam(level, z):
    return math.inf if level <= 0 else level ** (-0.5 * z)


def cluster(d, mcs, z, selection='eom', allow_single=False):
    n = len(d.node_rank)
    att = [[] for _ in range(n)]
    for x, v in enumerate(d.point_node):
        att[v].append(x)
    mass = [0] * n
    for v in range(n):  # enfants avant parents
        mass[v] = sum(d.point_weight[x] for x in att[v]) + sum(mass[u] for u in d.children[v])
    root = [v for v in range(n) if d.parent[v] < 0][-1]
    P = len(d.point_node)
    pc = [-1] * P
    cpar, cbirth, cstab = [], [], []

    def new_cluster(parent, birth):
        cpar.append(parent); cbirth.append(birth); cstab.append(0.0)
        return len(cpar) - 1

    def drop_subtree(v, c, l):
        stack = [v]
        while stack:
            u = stack.pop()
            for x in att[u]:
                pc[x] = c
                cstab[c] += d.point_weight[x] * (l - cbirth[c])
            stack.extend(d.children[u])

    work = [(root, new_cluster(-1, 0.0))]
    while work:
        v, c = work.pop()
        for x in att[v]:
            l = lam(d.level[d.point_rank[x]], z)
            pc[x] = c
            cstab[c] += d.point_weight[x] * (l - cbirth[c])
        if not d.children[v]:
            continue
        l = lam(d.level[d.node_rank[v]], z)
        big = [u for u in d.children[v] if mass[u] >= mcs]
        if len(big) >= 2:
            for u in d.children[v]:
                if mass[u] >= mcs:
                    cstab[c] += mass[u] * (l - cbirth[c])
                    work.append((u, new_cluster(c, l)))
                else:
                    drop_subtree(u, c, l)
        else:
            for u in d.children[v]:
                if len(big) == 1 and u == big[0]:
                    work.append((u, c))
                else:
                    drop_subtree(u, c, l)
    m = len(cpar)
    kids = [[] for _ in range(m)]
    for c in range(1, m):
        kids[cpar[c]].append(c)
    chosen = [False] * m
    if selection == 'eom':
        best = [0.0] * m
        for c in range(m - 1, -1, -1):
            sub = sum(best[k] for k in kids[c])
            is_root = cpar[c] < 0
            if not kids[c]:
                best[c] = cstab[c]; chosen[c] = True
            elif is_root and not allow_single:
                best[c] = sub
            elif sub > cstab[c]:
                best[c] = sub
            else:
                best[c] = cstab[c]; chosen[c] = True
        for c in range(m):
            if not chosen[c]:
                continue
            a = cpar[c]
            while a >= 0:
                if chosen[a]:
                    chosen[c] = False
                    break
                a = cpar[a]
    else:
        chosen = [not kids[c] for c in range(m)]
    if not allow_single and m >= 1:
        chosen[0] = False
    ids = [-1] * m
    k = 0
    for c in range(m):
        if chosen[c]:
            ids[c] = k; k += 1
    out = np.full(P, -1, dtype=np.int64)
    for x in range(P):
        c = pc[x]
        while c >= 0:
            if chosen[c]:
                out[x] = ids[c]
                break
            c = cpar[c]
    return out


def mr_tree(G, K, alpha):
    """MR_alpha exact : niveaux en rayons carres ; alpha = 2 : d^2 / 4 (entiers / 4, exact en double)."""
    Gf = G.astype(np.float64)
    n = len(G)
    core2 = cKDTree(Gf).query(Gf, k=K)[0]
    core2 = (core2[:, -1] if K > 1 else np.zeros(n))
    # recalcul exact des carres (entiers)
    tree = cKDTree(Gf)
    idx = tree.query(Gf, k=K)[1]
    kth = idx[:, -1] if K > 1 else np.arange(n)
    diff = G.astype(np.int64) - G[kth].astype(np.int64)
    core2 = (diff * diff).sum(axis=1).astype(np.float64)
    D = ((Gf[:, None, :] - Gf[None, :, :]) ** 2).sum(axis=2) / float(alpha * alpha)
    M = np.maximum(D, np.maximum(core2[:, None], core2[None, :]))
    np.fill_diagonal(M, 0.0)
    T = minimum_spanning_tree(M).tocoo()
    edges = sorted(zip(T.data.tolist(), T.row.tolist(), T.col.tolist()))
    vals = sorted(set(core2.tolist()) | {e[0] for e in edges})
    rank = {v: i for i, v in enumerate(vals)}
    node_rank = [rank[c] for c in core2.tolist()]
    parent = [-1] * n
    point_node = list(range(n)); point_rank = list(node_rank); point_weight = [1] * n
    dsu = list(range(n)); node_of = list(range(n))

    def find(x):
        while dsu[x] != x:
            dsu[x] = dsu[dsu[x]]; x = dsu[x]
        return x
    i = 0
    while i < len(edges):
        j = i
        while j < len(edges) and edges[j][0] == edges[i][0]:
            j += 1
        rk = rank[edges[i][0]]
        pairs = [(find(a), find(b)) for _, a, b in edges[i:j]]
        # composantes des anciennes racines
        loc = {}
        for a, b in pairs:
            for r in (a, b):
                loc.setdefault(r, r)
        def lf(x):
            while loc[x] != x:
                loc[x] = loc[loc[x]]; x = loc[x]
            return x
        for a, b in pairs:
            ra, rb = lf(a), lf(b)
            if ra != rb:
                loc[max(ra, rb)] = min(ra, rb)
        groups = {}
        for r in loc:
            groups.setdefault(lf(r), []).append(r)
        for g in groups.values():
            if len(g) < 2:
                continue
            v = len(node_rank)
            node_rank.append(rk); parent.append(-1)
            for r in g:
                parent[node_of[r]] = v
            top = min(g)
            for r in g:
                dsu[r] = top
            node_of[top] = v
        i = j
    return Dendro(vals, node_rank, parent, point_node, point_rank, point_weight)


ZS = ('0.25', '0.5', '1', 'zhat')
COLS = ('family', 'level', 'noise', 'n', 'seed', 'source', 'k', 'z', 'selection', 'fill', 'ari_s', 'clusters')


def run_unit(spec, args):
    P, L, _ = scenes.generate(spec)
    G, T, _, _ = scenes.quantize18(P, L)
    n = len(G)
    mcs = int(round(math.sqrt(n)))
    zh = methods.zhat(G)
    out, checks = [], []

    def score(src, k, z, sel, lab):
        nc = len(set(lab.tolist()) - {-1})
        for fill in ('none', 'full'):
            l2 = lab if fill == 'none' else methods.fill_noise(G, lab)
            s = metrics.scores(T, l2)
            out.append(dict(family=spec['family'], level=spec['level'], noise=spec['noise_fraction'], n=n,
                            seed=spec['seed'], source=src, k=k, z=z, selection=sel, fill=fill,
                            ari_s=round(s['ari_s'], 6), clusters=nc))

    with tempfile.TemporaryDirectory() as tmp:
        src, dst, cfg, tr = [os.path.join(tmp, x) for x in ('in', 'out', 'cfg', 'tree')]
        np.ascontiguousarray(G, dtype='<u4').tofile(src)
        with open(cfg, 'w') as f:
            f.write('%d 1.0 eom 0\n%d %r eom 0\n' % (mcs, mcs, zh))
        r = subprocess.run([os.path.join(args.build, 'mhgp10_cluster'), src, dst, '--k-list=' + args.tower_k,
                            '--threads=1', '--configs=' + cfg, '--tree=' + tr], capture_output=True, text=True)
        if r.returncode != 0:
            raise RuntimeError('tour refusee ' + r.stderr)
        for k in map(int, args.tower_k.split(',')):
            d = read_tree(tr + '.k%d' % k)
            for zi, z in enumerate(('1', 'zhat')):
                ref = np.fromfile(dst + '.k%d.%d' % (k, zi), dtype='<i4').astype(np.int64)
                mine = cluster(d, mcs, 1.0 if z == '1' else zh, 'eom')
                checks.append(bool(np.array_equal(ref, mine)))
            for z in ZS:
                score('tower', k, z, 'eom', cluster(d, mcs, zh if z == 'zhat' else float(z), 'eom'))
            score('tower', k, '-', 'leaf', cluster(d, mcs, 1.0, 'leaf'))
    for alpha in (1, 2):
        for k in map(int, args.mr_k.split(',')):
            d = mr_tree(G, k, alpha)
            for z in ZS:
                score('mr%d' % alpha, k, z, 'eom', cluster(d, mcs, zh if z == 'zhat' else float(z), 'eom'))
            score('mr%d' % alpha, k, '-', 'leaf', cluster(d, mcs, 1.0, 'leaf'))
    return out, checks


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--sizes', default='2000')
    ap.add_argument('--tower-k', default='1,2,3,5,8,10')
    ap.add_argument('--mr-k', default='1,2,3,5,8,10,16,20')
    ap.add_argument('--jobs', type=int, default=3)
    ap.add_argument('--limit', type=int, default=0)
    ap.add_argument('--families', default='')
    args = ap.parse_args()
    specs = run_campaign.plan('dev', [int(s) for s in args.sizes.split(',')], 2)
    if args.families:
        specs = [s for s in specs if s['family'] in args.families.split(',')]
    if args.limit:
        specs = specs[:args.limit]
    print('%d unites' % len(specs), flush=True)
    bad = 0
    with open(args.out, 'w', newline='') as h:
        w = csv.DictWriter(h, fieldnames=COLS)
        w.writeheader()
        with ProcessPoolExecutor(max_workers=args.jobs) as pool:
            futs = {pool.submit(run_unit, s, args): s for s in specs}
            done = 0
            for f in as_completed(futs):
                done += 1
                try:
                    rows, checks = f.result()
                except Exception as e:
                    print('ECHEC', futs[f], repr(e), flush=True)
                    continue
                bad += checks.count(False)
                for r in rows:
                    w.writerow(r)
                h.flush()
                print('%d/%d port_ok=%d/%d' % (done, len(specs), checks.count(True), len(checks)), flush=True)
    print('ecarts du port Python contre la tete C++ :', bad, flush=True)


if __name__ == '__main__':
    main()
