"""Entrees degenerees de J2c (u32le, x y z par point) dans /tmp/j2work/degen.

grid10, grid16 : grilles entieres n^3 (pas 1) ; grid10s7 : grille 10^3 de pas 7, decalee de 100 ;
sphere101, sphere314 : points entiers de x^2 + y^2 + z^2 = r2 (168 et 312 points cospheriques), centres en 200.
"""
import math
import os
import struct

OUT = '/tmp/j2work/degen'
os.makedirs(OUT, exist_ok=True)


def write(name, pts):
    with open(os.path.join(OUT, name + '.u32le'), 'wb') as f:
        for p in pts:
            f.write(struct.pack('<3I', *p))
    print(name, len(pts))


def shell(r2):
    r = math.isqrt(r2)
    rng = range(-r, r + 1)
    return [(x, y, z) for x in rng for y in rng for z in rng if x * x + y * y + z * z == r2]


for n in (10, 16):
    write('grid%d' % n, [(x, y, z) for x in range(n) for y in range(n) for z in range(n)])
write('grid10s7', [(7 * x + 100, 7 * y + 100, 7 * z + 100) for x in range(10) for y in range(10) for z in range(10)])
for r2 in (101, 314):
    write('sphere%d' % r2, [(x + 200, y + 200, z + 200) for (x, y, z) in shell(r2)])
