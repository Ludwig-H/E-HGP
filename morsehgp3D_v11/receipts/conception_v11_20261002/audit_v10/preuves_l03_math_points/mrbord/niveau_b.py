#!/usr/bin/env python3
"""Audit L03 : niveau B (meilleur bloc par groupe vrai) pour la tour (entrees core, cover) et pour le temoin
d'atteignabilite mutuelle du depot (MR_alpha, entrees coeur et bord), MEME evaluateur, memes sites.
Graines d'audit hors de tout plan (aucune graine dev ni test du depot). Aucune selection, aucun z."""
import os, sys, subprocess, tempfile, json, time
import numpy as np
sys.dont_write_bytecode = True
W = '/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10'
sys.path.insert(0, os.path.join(W, 'bench', 'synthetic'))
import scenes  # noqa: E402
BUILD = '/tmp/v11-audit/l03_math_points/build_v10'
HERE = '/tmp/v11-audit/l03_math_points/mrbord'


def read_tree(path):
    with open(path) as f:
        L = int(f.readline().split()[1])
        levels = np.array([float(f.readline()) for _ in range(L)])
        N = int(f.readline().split()[1])
        nodes = np.loadtxt([f.readline() for _ in range(N)], dtype=np.int64).reshape(N, 2)
        P = int(f.readline().split()[1])
        pts = np.loadtxt([f.readline() for _ in range(P)], dtype=np.int64).reshape(P, 4)
    return levels, nodes, pts


def best_blocks(path, labels, G):
    """Meilleur IoU par groupe sur tous les blocs (noeud, rang) de la hierarchie de points ; coupes fermees."""
    levels, nodes, pts = read_tree(path)
    N = len(nodes)
    rank, parent = nodes[:, 0], nodes[:, 1]
    m = np.array([(labels == g).sum() for g in range(G)], dtype=np.float64)
    order = np.lexsort((pts[:, 2], pts[:, 1]))          # par noeud puis rang d'entree
    pn, pr, pid = pts[order, 1], pts[order, 2], pts[order, 0]
    start = np.searchsorted(pn, np.arange(N), side='left')
    end = np.searchsorted(pn, np.arange(N), side='right')
    cnt = np.zeros((N, G), dtype=np.int64)
    size = np.zeros(N, dtype=np.int64)
    best = np.zeros(G)
    exact = np.zeros(G, dtype=bool)

    def ev(c, s):
        nz = c > 0
        if not nz.any():
            return
        iou = c[nz] / (s + m[nz] - c[nz])
        idx = np.nonzero(nz)[0]
        up = iou > best[idx]
        best[idx[up]] = iou[up]
        ex = (c[nz] == m[nz]) & (s == c[nz])
        exact[idx[ex]] = True

    for v in range(N):
        c = cnt[v]
        s = size[v]
        a, b = start[v], end[v]
        if a == b:
            if s > 0:
                ev(c, s)
        else:
            if s > 0 and pr[a] > rank[v]:
                ev(c, s)
            i = a
            while i < b:
                j = i
                while j < b and pr[j] == pr[i]:
                    g = labels[pid[j]]
                    if g >= 0:
                        c[g] += 1
                    s += 1
                    j += 1
                ev(c, s)
                i = j
            size[v] = s
        p = parent[v]
        if p >= 0:
            cnt[p] += c
            size[p] += s
    return best, exact, N


def run(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError('%s -> %d %s %s' % (' '.join(cmd), r.returncode, r.stdout[-300:], r.stderr[-300:]))
    return r.stdout


def main():
    out_path = sys.argv[1]
    n = int(sys.argv[2])
    ks = [int(k) for k in sys.argv[3].split(',')]
    levels = sys.argv[4].split(',')
    noises = [float(x) for x in sys.argv[5].split(',')]
    reps = int(sys.argv[6])
    G = 8
    rows = []
    tmp = tempfile.mkdtemp(prefix='nb_', dir=HERE)
    seed0 = 9020261002000        # graines d'audit L03, hors de tout plan
    t0 = time.time()
    unit = 0
    for fam in scenes.FAMILIES:
        for lev in levels:
            for nu in noises:
                for rep in range(reps):
                    unit += 1
                    spec = dict(family=fam, n=n, groups=G, level=lev, noise_fraction=nu, seed=seed0 + unit)
                    P, L, meta = scenes.generate(spec)
                    Gd, Lq, dup, h = scenes.quantize18(P, L)
                    src = os.path.join(tmp, 'in.u32le')
                    np.ascontiguousarray(Gd, dtype='<u4').tofile(src)
                    for K in ks:
                        trees = {}
                        run([os.path.join(BUILD, 'mhgp10_cluster'), src, os.path.join(tmp, 'o'), '--k=%d' % K, '--mcs=%d' % max(K, 2),
                             '--entry=core,cover', '--tree=' + os.path.join(tmp, 'tw'), '--threads=1'])
                        trees['tour_core'] = os.path.join(tmp, 'tw.core.k%d' % K)
                        trees['tour_cover'] = os.path.join(tmp, 'tw.cover.k%d' % K)
                        for alpha in (1, 2):
                            for ent in ('core', 'border'):
                                p = os.path.join(tmp, 'mr%d_%s' % (alpha, ent))
                                run([os.path.join(HERE, 'export_mreach_tree'), src, p, '--k=%d' % K, '--alpha=%d' % alpha, '--entry=' + ent, '--threads=1'])
                                trees['mr%d_%s' % (alpha, 'coeur' if ent == 'core' else 'bord')] = p
                        for name, p in trees.items():
                            best, exact, N = best_blocks(p, Lq, G)
                            rows.append(dict(family=fam, level=lev, noise=nu, rep=rep, n=len(Gd), K=K, source=name, nodes=int(N),
                                             iou=[round(float(x), 6) for x in best], exact=int(exact.sum())))
                    print('%3d %-16s %-7s nu=%.2f n=%d  %.0f s' % (unit, fam, lev, nu, len(Gd), time.time() - t0), flush=True)
    for f in os.listdir(tmp):
        os.remove(os.path.join(tmp, f))
    os.rmdir(tmp)
    json.dump(dict(cadre='audit L03 v11 ; graines d audit %d+ ; n=%d ; 8 groupes ; binaire v10 afb081774' % (seed0, n), rows=rows), open(out_path, 'w'))
    # resume
    import collections
    S = collections.defaultdict(list)
    E = collections.Counter()
    for r in rows:
        S[(r['K'], r['source'])].extend(r['iou'])
        E[(r['K'], r['source'])] += r['exact']
    print('\nK | source | groupes | IoU moyen du meilleur bloc | exacts')
    for (K, s), v in sorted(S.items()):
        print('%2d | %-10s | %d | %.4f | %d' % (K, s, len(v), sum(v) / len(v), E[(K, s)]))


if __name__ == '__main__':
    main()
