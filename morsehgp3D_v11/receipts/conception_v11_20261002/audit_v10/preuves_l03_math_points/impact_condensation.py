#!/usr/bin/env python3
"""Audit L03 : ou le defaut de condensation de la tete v10 se declenche-t-il ?
On relit la hierarchie exportee par `mhgp10_cluster --tree`, puis on condense en Python de deux facons :
  mode 'v10'      : semantique du code publie (head.cpp) : un point attache sort a son niveau d'entree, mcs teste aux
                    seules fusions ;
  mode 'cohortes' : semantique de HDBSCAN sur l'ultrametrique de points : quand une cohorte de departs laisse moins
                    de mcs points, tout le reste sort a ce niveau.
Controle : le mode 'v10' doit redonner exactement les etiquettes du binaire. On compte ensuite les clusters dont la
stabilite change et les etiquettes qui changent. Graines d'audit hors plan ; diagnostic, pas une mesure de qualite."""
import os, sys, subprocess, tempfile, json
import numpy as np
sys.dont_write_bytecode = True
W = '/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10'
sys.path.insert(0, os.path.join(W, 'bench', 'synthetic'))
import scenes  # noqa: E402
BUILD = '/tmp/v11-audit/l03_math_points/build_v10'
HERE = '/tmp/v11-audit/l03_math_points/work'


def read_tree(path):
    with open(path) as f:
        L = int(f.readline().split()[1]); levels = [float(f.readline()) for _ in range(L)]
        N = int(f.readline().split()[1]); nodes = [tuple(map(int, f.readline().split())) for _ in range(N)]
        P = int(f.readline().split()[1]); pts = [tuple(map(int, f.readline().split())) for _ in range(P)]
    return levels, nodes, pts


def condense(levels, nodes, pts, mcs, z, mode):
    N = len(nodes)
    rank = [a for a, _ in nodes]; parent = [b for _, b in nodes]
    kids = [[] for _ in range(N)]
    root = None
    for v in range(N):
        if parent[v] >= 0: kids[parent[v]].append(v)
        else: root = v
    att = [[] for _ in range(N)]
    for pid, v, rk, w in pts: att[v].append((rk, pid))
    mass = [0] * N
    for v in range(N):
        mass[v] += len(att[v])
        if parent[v] >= 0: mass[parent[v]] += mass[v]
    lam = lambda lv: float('inf') if lv <= 0 else lv ** (-0.5 * z)
    cl_parent = [-1]; birth = [0.0]; stab = [0.0]
    point_cluster = {}
    declenche = 0
    def drop(u, c, l):
        st = [u]
        while st:
            x = st.pop()
            for rk, pid in att[x]:
                point_cluster[pid] = c; stab[c] += l - birth[c]
            st.extend(kids[x])
    work = [(root, 0)]
    while work:
        v, c = work.pop()
        l_node = lam(levels[rank[v]])
        big = [u for u in kids[v] if mass[u] >= mcs]
        a = sorted(att[v], reverse=True)          # departs par niveau decroissant
        if mode == 'cohortes':
            remaining = mass[v]
            i = 0; mort = False
            while i < len(a):
                j = i
                while j < len(a) and a[j][0] == a[i][0]: j += 1
                rk = a[i][0]
                if rk == rank[v]:
                    break                          # cohorte du niveau de naissance du noeud : traitee avec la fusion
                l = lam(levels[rk])
                reste = remaining - (j - i)
                if reste >= mcs:
                    for _, pid in a[i:j]:
                        point_cluster[pid] = c; stab[c] += l - birth[c]
                    remaining = reste
                    i = j
                else:
                    # le cluster meurt ici : tout ce qui reste sort a ce niveau
                    declenche += 1
                    for _, pid in a[i:]:
                        point_cluster[pid] = c; stab[c] += l - birth[c]
                    for u in kids[v]: drop(u, c, l)
                    mort = True
                    break
            if mort:
                continue
            rest = a[i:]
            for _, pid in rest:                    # cohorte du niveau de naissance : sort au lambda du noeud
                point_cluster[pid] = c; stab[c] += l_node - birth[c]
        else:
            for rk, pid in a:
                point_cluster[pid] = c; stab[c] += lam(levels[rk]) - birth[c]
        if not kids[v]:
            continue
        if len(big) >= 2:
            for u in kids[v]:
                if mass[u] >= mcs:
                    stab[c] += mass[u] * (l_node - birth[c])
                    cl_parent.append(c); birth.append(l_node); stab.append(0.0)
                    work.append((u, len(cl_parent) - 1))
                else:
                    drop(u, c, l_node)
        else:
            for u in kids[v]:
                if len(big) == 1 and u == big[0]: work.append((u, c))
                else: drop(u, c, l_node)
    m = len(cl_parent)
    ch = [[] for _ in range(m)]
    for c in range(1, m): ch[cl_parent[c]].append(c)
    chosen = [False] * m; best = [0.0] * m
    for c in range(m - 1, -1, -1):
        sub = sum(best[k] for k in ch[c])
        if not ch[c]: best[c] = stab[c]; chosen[c] = True
        elif cl_parent[c] == -1: best[c] = sub
        elif sub > stab[c]: best[c] = sub
        else: best[c] = stab[c]; chosen[c] = True
    for c in range(m):
        if chosen[c]:
            a_ = cl_parent[c]
            while a_ != -1:
                if chosen[a_]: chosen[c] = False; break
                a_ = cl_parent[a_]
    chosen[0] = False
    ids = {}
    for c in range(m):
        if chosen[c]: ids[c] = len(ids)
    lab = np.full(len(pts), -1, dtype=np.int64)
    for pid, c in point_cluster.items():
        x = c
        while x != -1:
            if chosen[x]: lab[pid] = ids[x]; break
            x = cl_parent[x]
    return lab, stab, declenche, m


def same_partition(a, b):
    import collections
    ma = {}; mb = {}
    for x, y in zip(a, b):
        if (x < 0) != (y < 0): return False
        if x < 0: continue
        if ma.setdefault(x, y) != y or mb.setdefault(y, x) != x: return False
    return True


def main():
    out = []
    tmp = tempfile.mkdtemp(prefix='ic_', dir=HERE)
    seed0 = 9020261002500
    u = 0
    for fam in ('spherical', 'unbalanced', 'filaments', 'bridge'):
        for nu in (0.0, 0.1):
            u += 1
            P, L, _ = scenes.generate(dict(family=fam, n=2000, groups=8, level='hard', noise_fraction=nu, seed=seed0 + u))
            G, Lq, dup, h = scenes.quantize18(P, L)
            src = os.path.join(tmp, 'in.u32le'); np.ascontiguousarray(G, dtype='<u4').tofile(src)
            for K in (2, 5):
                for entry in ('core', 'cover'):
                    for mcs in (10, 45):
                        for z in (1.0, 3.0):
                            lab_out = os.path.join(tmp, 'lab'); tree = os.path.join(tmp, 'tree')
                            r = subprocess.run([os.path.join(BUILD, 'mhgp10_cluster'), src, lab_out, '--k=%d' % K, '--mcs=%d' % mcs, '--z=%r' % z,
                                                '--entry=' + entry, '--tree=' + tree, '--threads=1'], capture_output=True, text=True)
                            if r.returncode != 0:
                                print('refus', fam, K, entry, r.stdout[:100]); continue
                            natif = np.fromfile(lab_out, dtype='<i4')
                            lv, nd, pt = read_tree(tree)
                            l1, s1, _, m1 = condense(lv, nd, pt, mcs, z, 'v10')
                            l2, s2, dec, m2 = condense(lv, nd, pt, mcs, z, 'cohortes')
                            row = dict(family=fam, noise=nu, K=K, entry=entry, mcs=mcs, z=z, port_v10_egal_binaire=bool(same_partition(l1, natif)),
                                       clusters_condenses=m1, morts_par_cohorte=dec,
                                       stabilites_changees=int(sum(1 for a, b in zip(s1, s2) if abs(a - b) > 1e-12 * max(1.0, abs(a)))) if m1 == m2 else -1,
                                       etiquettes_changees=int((l1 != l2).sum()) if same_partition(l1, l2) is False else 0,
                                       clusters_v10=int(l1.max() + 1), clusters_cohortes=int(l2.max() + 1))
                            out.append(row)
            print(u, fam, nu, flush=True)
    for f in os.listdir(tmp): os.remove(os.path.join(tmp, f))
    os.rmdir(tmp)
    json.dump(out, open(os.path.join(HERE, 'impact_condensation.json'), 'w'), indent=0)
    import collections
    agg = collections.defaultdict(lambda: [0, 0, 0, 0, 0, 0])
    for r in out:
        a = agg[(r['entry'], r['K'])]
        a[0] += 1; a[1] += r['port_v10_egal_binaire']; a[2] += r['morts_par_cohorte'] > 0; a[3] += r['stabilites_changees'] != 0; a[4] += r['etiquettes_changees'] > 0; a[5] += r['etiquettes_changees']
    print('entree | K | configurations | port v10 = binaire | config. ou une cohorte tue un cluster | config. a stabilites changees | config. a etiquettes changees | points changes (total)')
    for k in sorted(agg):
        print(k[0], '|', k[1], '|', ' | '.join(map(str, agg[k])))

if __name__ == '__main__':
    main()
