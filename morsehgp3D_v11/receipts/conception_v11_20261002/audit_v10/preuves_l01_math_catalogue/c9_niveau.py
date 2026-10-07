#!/usr/bin/env python3
"""Constat C9 (audit L01) : la representation (num, den) publiee d'un niveau n'est pas toujours celle de S*.
Coquilles etendues de 5 sites sur la sphere de rayon carre 9 centree en (3, 3, 3), q_min = 3 (un triangle aigu dont le
plan contient le centre, aucune paire antipodale), interieur vide. On compare le niveau du noeud de naissance d'ordre 5
imprime par mhgp10_tower --dump a la formule level3(S*) = |u|^2 |v|^2 |u-v|^2 / (4 |u x v|^2) de geometry.cpp.
Usage : c9_niveau.py BUILD_DIR"""
import itertools
import os
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, '/tmp/v11-audit/l01_math_catalogue/oracle')
import oracle_indep as O  # noqa: E402

build = sys.argv[1]
V = [(3, 0, 0), (-3, 0, 0), (0, 3, 0), (0, -3, 0), (0, 0, 3), (0, 0, -3)]
V += [(sx * a, sy * b, sz * c) for (a, b, c) in ((1, 2, 2), (2, 1, 2), (2, 2, 1)) for sx in (1, -1) for sy in (1, -1) for sz in (1, -1)]
tri = [(3, 0, 0), (-1, 2, 2), (-1, -2, -2)]
found = same = tested = 0
with tempfile.TemporaryDirectory(dir='/tmp/v11-audit/l01_math_catalogue/c9') as tmp:
    for e1, e2 in itertools.combinations([v for v in V if v not in tri], 2):
        pts = tri + [e1, e2]
        if any(tuple(-x for x in p) in pts for p in pts):
            continue  # paire antipodale : q_min = 2
        P = [tuple(3 + x for x in p) for p in pts]
        pos, w = O.sites_of(P)
        balls = O.critical_balls(pos, w)
        big = [(k, v) for k, v in balls.items() if len(v[3]) == 5]
        if len(big) != 1 or big[0][1][0] != 3:
            continue
        (c, r2), (q, S, I, U) = big[0]
        tested += 1
        a, b, d = (pos[s] for s in S)
        u = tuple(x - y for x, y in zip(b, a)); v = tuple(x - y for x, y in zip(d, a)); e = tuple(x - y for x, y in zip(d, b))
        wv = (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])
        num3 = sum(x * x for x in u) * sum(x * x for x in v) * sum(x * x for x in e)
        den3 = 4 * sum(x * x for x in wv)
        src = os.path.join(tmp, 'in.u32le')
        with open(src, 'wb') as f:
            for p in P:
                f.write(struct.pack('<3I', *p))
        dump = os.path.join(tmp, 'tower.txt')
        r = subprocess.run([os.path.join(build, 'mhgp10_tower'), src, '--k=5', '--threads=1', '--dump=' + dump], capture_output=True, text=True)
        if r.returncode != 0:
            continue
        cur = None
        got = None
        for line in open(dump):
            t = line.split()
            if t[0] == 'order':
                cur = int(t[1])
            elif t[0] == 'node' and cur == 5:
                got = (int(t[3]), int(t[4]))
        if got is None:
            continue
        assert got[0] * 1 == 9 * got[1], got
        if got == (num3, den3):
            same += 1
        else:
            found += 1
            if found <= 3:
                print('points %s ; S* = %s ; level3(S*) = %d/%d ; publie = %d/%d' % (P, [pos[s] for s in S], num3, den3, got[0], got[1]))
print('c9 configurations %d : representation publiee = level3(S*) dans %d cas, differente dans %d cas (valeur egale a 9 partout)' % (tested, same, found))
