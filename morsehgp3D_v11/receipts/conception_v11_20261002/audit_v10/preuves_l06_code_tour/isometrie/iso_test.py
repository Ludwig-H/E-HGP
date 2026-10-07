"""Audit L06 : l'attache des points est-elle invariante par isometrie de la grille ?

Variante B = echange des axes x et y, miroir de z. L'objet (tour, niveaux) est invariant ; l'indice de site (rang de
Morton) ne l'est pas. On compare, par ordre, la partition des sites induite par point_node (deux sites sont ensemble
ssi ils sont attaches au meme noeud) et les niveaux d'entree, entre A et B, en coordonnees d'origine.
Usage : python3 iso_test.py BUILD IN.u32le core|cover K
"""
import os
import subprocess
import sys

import numpy as np
from fractions import Fraction

build, src, entry, K = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4])
a = np.fromfile(src, dtype='<u4').reshape(-1, 3).astype(np.int64)
zmax = int(a[:, 2].max())
b = np.column_stack([a[:, 1], a[:, 0], zmax - a[:, 2]])
tmp = '/tmp/v11-audit/l06_code_tour/iso'


def run(tag, pts):
    f = os.path.join(tmp, tag + '.u32le')
    np.ascontiguousarray(pts, dtype='<u4').tofile(f)
    d = os.path.join(tmp, tag + '.dump')
    r = subprocess.run([os.path.join(build, 'mhgp10_tower'), f, '--k=%d' % K, '--threads=3', '--entry=' + entry,
                        '--dump=' + d], capture_output=True, text=True)
    assert r.returncode == 0, r.stdout
    orders = {}
    cur = None
    with open(d) as fh:
        for line in fh:
            if line.startswith('order'):
                cur = {}
                orders[int(line.split()[1])] = cur
            elif line.startswith('point'):
                t = line.split()
                cur[(int(t[1]), int(t[2]), int(t[3]))] = (int(t[4]), ' '.join(t[5:]))
    os.remove(d)
    nodes = sum(1 for _ in [0])
    return orders


A = run('A_' + entry, a)
B = run('B_' + entry, b)
inv = lambda p: (p[1], p[0], zmax - p[2])  # noqa: E731  B -> coordonnees d'origine
for k in sorted(A):
    ga, gb = {}, {}
    lev_diff = 0
    repr_diff = [0]
    for p, (v, lv) in A[k].items():
        ga.setdefault(v, []).append(p)
    for p, (v, lv) in B[k].items():
        q = inv(p)
        gb.setdefault(v, []).append(q)
        la = A[k][q][1]
        # en entree cover le dump donne « r<rang> num den » : comparer num/den seulement
        ta, tb = la.split(), lv.split()
        fa = Fraction(int(ta[-2]), int(ta[-1])) if len(ta) == 3 else Fraction(int(ta[0]))
        fb = Fraction(int(tb[-2]), int(tb[-1])) if len(tb) == 3 else Fraction(int(tb[0]))
        if fa != fb:
            lev_diff += 1
        elif la.split()[-2:] != lv.split()[-2:] and la != lv:
            repr_diff[0] += 1
    ca = {p: frozenset(g) for g in ga.values() for p in g}
    cb = {p: frozenset(g) for g in gb.values() for p in g}
    diff = sum(1 for p in ca if ca[p] != cb[p])
    print('entree %s ordre %d : sites %d, classes A %d, classes B %d, sites dont la classe d\'attache differe %d, '
          'niveaux d\'entree differents %d, meme niveau ecrit autrement (num/den non reduits) %d'
          % (entry, k, len(ca), len(ga), len(gb), diff, lev_diff, repr_diff[0]), flush=True)
