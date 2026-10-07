#!/usr/bin/env python3
"""Lecture de la hierarchie de points du binaire v10 (construit depuis l'arbre de lecture afb081774, hors source)."""
import os, subprocess, tempfile, struct, json
import numpy as np

BUILD = '/tmp/v11-audit/l03_math_points/build_v10'


def run_cluster(P, K, entry, mcs=2, z=1.0, sel='eom', extra=(), threads=1):
    tmp = tempfile.mkdtemp(prefix='l03_', dir='/tmp/v11-audit/l03_math_points/work')
    src = os.path.join(tmp, 'in.u32le')
    np.ascontiguousarray(np.array(P, dtype='<u4')).tofile(src)
    out = os.path.join(tmp, 'out.i32le')
    tree = os.path.join(tmp, 'tree.txt')
    cmd = [os.path.join(BUILD, 'mhgp10_cluster'), src, out, '--k=%d' % K, '--mcs=%d' % mcs, '--z=%r' % z,
           '--selection=%s' % sel, '--entry=%s' % entry, '--tree=%s' % tree, '--threads=%d' % threads] + list(extra)
    r = subprocess.run(cmd, capture_output=True, text=True)
    res = {'code': r.returncode, 'stdout': r.stdout.strip(), 'stderr': r.stderr.strip()}
    if r.returncode == 0:
        res['labels'] = list(np.fromfile(out, dtype='<i4'))
        res['tree'] = read_tree(tree)
    for f in os.listdir(tmp):
        os.remove(os.path.join(tmp, f))
    os.rmdir(tmp)
    return res


def read_tree(path):
    lines = open(path).read().split('\n')
    L = int(lines[0].split()[1])
    levels = [float(x) for x in lines[1:1 + L]]
    N = int(lines[1 + L].split()[1])
    nodes = [tuple(int(v) for v in ln.split()) for ln in lines[2 + L:2 + L + N]]
    off = 2 + L + N
    Pn = int(lines[off].split()[1])
    pts = {}
    for ln in lines[off + 1:off + 1 + Pn]:
        x, v, rk, w = map(int, ln.split())
        pts[x] = (v, rk)
    return {'levels': levels, 'nodes': nodes, 'points': pts}


def suite(tree, mcs=1):
    """Suite des partitions (blocs >= mcs) aux rangs ou elle change : [(niveau r^2 double, blocs)]."""
    levels, nodes, pts = tree['levels'], tree['nodes'], tree['points']
    N = len(nodes)
    out = []
    last = None
    for t in range(len(levels)):
        # noeud vivant au rang t pour chaque point entre
        blocks = {}
        for x, (v, rk) in pts.items():
            if rk > t:
                continue
            u = v
            while nodes[u][1] != -1 and nodes[nodes[u][1]][0] <= t:
                u = nodes[u][1]
            blocks.setdefault(u, []).append(x)
        p = tuple(sorted(tuple(sorted(b)) for b in blocks.values() if len(b) >= mcs))
        if p != last:
            out.append((levels[t], p))
            last = p
    return out
