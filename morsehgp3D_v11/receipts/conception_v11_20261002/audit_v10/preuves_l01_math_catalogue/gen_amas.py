#!/usr/bin/env python3
"""Famille adverse L01 : c amas denses (m points tires dans un cube de cote a) aux coins d'un cube d'arete D.
Usage : gen_amas.py SORTIE c m a D graine"""
import random, struct, sys
out, c, m, a, D, seed = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5]), int(sys.argv[6])
rnd = random.Random(seed)
corners = [(x, y, z) for x in (0, 1) for y in (0, 1) for z in (0, 1)]
order = [0, 7, 3, 5, 6, 1, 2, 4][:c]
pts = set()
for ci in order:
    o = tuple(v * (D - a) for v in corners[ci])
    got = set()
    while len(got) < m:
        got.add(tuple(o[j] + rnd.randrange(a) for j in range(3)))
    pts |= got
with open(out, 'wb') as f:
    for p in sorted(pts):
        f.write(struct.pack('<3I', *p))
print(len(pts))
