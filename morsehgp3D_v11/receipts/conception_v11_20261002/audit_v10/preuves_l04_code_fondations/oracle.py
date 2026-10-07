#!/usr/bin/env python3
"""L04 audit : juge independant (entiers Python, fractions exactes) des predicats de arith/geometry.hpp.

Usage : oracle.py BITS [N_RANDOM] [HARNESS]
Les resultats du harnais C++ sont juges par leurs PROPRIETES de definition (equidistance, coplanarite, signe d'un
determinant rationnel), pas par une recopie des formules du code. Le script mesure aussi la longueur binaire
maximale de chaque quantite intermediaire, telle que le code l'evalue.
Sortie : compte des desaccords par predicat ; code 1 si desaccord, 0 sinon (jamais d'assert).
"""
import random
import subprocess
import sys
from fractions import Fraction

BITS = int(sys.argv[1])
NRAND = int(sys.argv[2]) if len(sys.argv) > 2 else 6000
HARNESS = sys.argv[3] if len(sys.argv) > 3 else './harness'
M = (1 << BITS) - 1
rng = random.Random(20261002 + BITS)


def sub(p, q):
    return (p[0] - q[0], p[1] - q[1], p[2] - q[2])


def dot(p, q):
    return p[0] * q[0] + p[1] * q[1] + p[2] * q[2]


def cross(p, q):
    return (p[1] * q[2] - p[2] * q[1], p[2] * q[0] - p[0] * q[2], p[0] * q[1] - p[1] * q[0])


def det3(u, v, w):
    return dot(u, cross(v, w))


def sign(x):
    return (x > 0) - (x < 0)


def corner():
    return tuple(rng.choice((0, M)) for _ in range(3))


def near_corner():
    return tuple(min(M, max(0, rng.choice((0, M)) + rng.randint(-3, 3))) for _ in range(3))


def uniform():
    return tuple(rng.randint(0, M) for _ in range(3))


def configs():
    corners = [(x, y, z) for x in (0, M) for y in (0, M) for z in (0, M)]
    # 1. tous les triplets ordonnes de coins distincts, d et z tires parmi les coins
    for a in corners:
        for b in corners:
            for c in corners:
                if len({a, b, c}) < 3:
                    continue
                for _ in range(6):
                    yield (a, b, c, rng.choice(corners), rng.choice(corners))
    # 2. coins perturbes, uniformes, quasi degeneres
    for _ in range(NRAND):
        yield tuple(near_corner() for _ in range(5))
    for _ in range(NRAND):
        yield tuple(uniform() for _ in range(5))
    for _ in range(NRAND):  # c proche de la droite (a, b), d proche du plan : grands rayons
        a, b = near_corner(), near_corner()
        t = Fraction(rng.randint(0, 1000), 1000)
        c = tuple(min(M, max(0, int(a[i] + t * (b[i] - a[i])) + rng.randint(-2, 2))) for i in range(3))
        d = tuple(min(M, max(0, c[i] + rng.randint(-2, 2))) for i in range(3)) if rng.random() < 0.5 else near_corner()
        yield (a, b, c, d, near_corner())
    for _ in range(NRAND):  # triangles aigus et tetraedres a centre interieur probables : points sur de grandes "spheres"
        yield tuple(uniform() if rng.random() < 0.5 else near_corner() for _ in range(5))


cfgs = list(configs())
inp = '\n'.join(' '.join(str(v) for p in cfg for v in p) for cfg in cfgs) + '\n'
res = subprocess.run([HARNESS], input=inp, capture_output=True, text=True)
blocks = res.stdout.split('END\n')
ubsan = [l for l in res.stderr.splitlines() if 'runtime error' in l]
bad = {}
seen = {}
maxbits = {}


def note(name, ok):
    seen[name] = seen.get(name, 0) + 1
    if not ok:
        bad[name] = bad.get(name, 0) + 1


def mb(name, *vals):
    m = max(abs(int(v)).bit_length() for v in vals)
    if m > maxbits.get(name, 0):
        maxbits[name] = m


def center_from(a, N, D):
    return tuple(Fraction(a[i]) + Fraction(N[i], D) for i in range(3))


def d2(p, q):
    return sum((Fraction(p[i]) - Fraction(q[i])) ** 2 for i in range(3))


if len(blocks) - 1 != len(cfgs):
    print('SORTIE TRONQUEE : %d blocs pour %d configurations (rc=%s)' % (len(blocks) - 1, len(cfgs), res.returncode))
    bad['harnais'] = 1

prev_r4 = None
for cfg, blk in zip(cfgs, blocks):
    a, b, c, d, z = cfg
    rows = {}
    for line in blk.strip().splitlines():
        t = line.split()
        rows[t[0]] = t[1:]
    u, v, s = sub(b, a), sub(c, a), sub(d, a)
    w = cross(u, v)
    collinear = w == (0, 0, 0)
    det = det3(u, v, s)
    # ---- q2
    N2 = tuple(int(x) for x in rows['C2'][:3])
    note('center2', N2 == u and int(rows['C2'][3]) == 2)
    c2 = center_from(a, N2, 2)
    r2_2 = d2(a, c2)
    # ---- q3
    ok3 = rows['C3'][0] == '1'
    note('center3.refus', ok3 == (not collinear))
    c3 = None
    if ok3 and not collinear:
        N3 = tuple(int(x) for x in rows['C3'][1:4])
        D3 = int(rows['C3'][4])
        c3 = center_from(a, N3, D3) if D3 != 0 else None
        good = D3 > 0 and c3 is not None and d2(a, c3) == d2(b, c3) == d2(c, c3) and dot(N3, w) == 0
        note('center3', good)
        # valeurs VRAIES (formule fermee) pour le budget de bits, independantes du resultat du code
        uu, vv = dot(u, u), dot(v, v)
        t3 = tuple(uu * v[i] - vv * u[i] for i in range(3))
        N3t = cross(t3, w)
        D3t = 2 * dot(w, w)
        mb('q3.t=uu*v-vv*u', *t3)
        mb('q3.N', *N3t)
        mb('q3.N.produits', t3[1] * w[2], t3[2] * w[1], t3[2] * w[0], t3[0] * w[2], t3[0] * w[1], t3[1] * w[0])
        mb('q3.D', D3t)
        c3t = center_from(a, N3t, D3t)
        for (nm, zz) in (('z', z), ('d', d)):
            dz = sub(zz, a)
            lhs = D3t * dot(dz, dz)
            p0, p1, p2 = N3t[0] * dz[0], N3t[1] * dz[1], N3t[2] * dz[2]
            mb('q3.side.lhs', lhs)
            mb('q3.side.rhs+partiels', p0, p1, p2, p0 + p1, p0 + p1 + p2, 2 * (p0 + p1 + p2))
            mb('q3.side_key', lhs - 2 * (p0 + p1 + p2))
        key = D3t * (d2(z, c3t) - d2(a, c3t))
        got = rows['S3']
        note('side_key.q3', Fraction(int(got[0])) == key and int(got[1]) == sign(key))
        keyd = D3t * (d2(d, c3t) - d2(a, c3t))
        note('side_key.q3', Fraction(int(got[2])) == keyd and int(got[3]) == sign(keyd))
        # orientation du centre q3 par rapport au plan (b, d, z)
        pa, pb = sub(d, b), sub(z, b)
        ww = cross(pa, pb)
        cc = tuple(N3t[i] + D3t * (a[i] - b[i]) for i in range(3))
        mb('q3.orient_center.cc', *cc)
        mb('q3.orient_center.produits+somme', ww[0] * cc[0], ww[1] * cc[1], ww[2] * cc[2], dot(ww, cc))
        note('orient_center_wide.q3', int(rows['OC3W'][0]) == sign(dot(ww, cc)))
        if 'OC3N' in rows:
            note('orient_center(i128).q3[hors contrat]', int(rows['OC3N'][0]) == sign(dot(ww, cc)))
        # is_midpoint(a, b, anchor=a, c3) : le centre est-il le milieu de [a, b] ?
        mb('q3.is_midpoint', *(2 * (a[i] * D3t + N3t[i]) for i in range(3)), *((a[i] + b[i]) * D3t for i in range(3)))
        note('is_midpoint.q3', (rows['MID'][1] == '1') == (c3t == tuple(Fraction(a[i] + b[i], 2) for i in range(3))))
        # niveau q3
        L3 = rows['L3']
        r2_3 = d2(a, c3t)
        dd = dot(sub(c, b), sub(c, b))
        mb('q3.level.uu*vv', uu * vv)
        mb('q3.level.num', uu * vv * dd)
        mb('q3.level.den', 4 * dot(w, w))
        note('level3', int(L3[1]) > 0 and Fraction(int(L3[0]), int(L3[1])) == r2_3)
    # ---- q4
    ok4 = rows['C4'][0] == '1'
    note('center4.refus', ok4 == (det != 0))
    if ok4 and det != 0:
        N4 = tuple(int(x) for x in rows['C4'][1:4])
        D4 = int(rows['C4'][4])
        c4 = center_from(a, N4, D4) if D4 != 0 else None
        good = D4 > 0 and c4 is not None and d2(a, c4) == d2(b, c4) == d2(c, c4) == d2(d, c4)
        note('center4', good)
        uu, vv, ss = dot(u, u), dot(v, v), dot(s, s)
        vs, su, uv = cross(v, s), cross(s, u), cross(u, v)
        N4t = tuple(uu * vs[i] + vv * su[i] + ss * uv[i] for i in range(3))
        D4t = 2 * det
        if D4t < 0:
            D4t, N4t = -D4t, tuple(-x for x in N4t)
        mb('q4.det', det)
        mb('q4.N', *N4t)
        mb('q4.N.produits', *(uu * vs[i] for i in range(3)), *(vv * su[i] for i in range(3)), *(ss * uv[i] for i in range(3)))
        mb('q4.D', D4t)
        c4t = center_from(a, N4t, D4t)
        dz = sub(z, a)
        lhs = D4t * dot(dz, dz)
        p0, p1, p2 = N4t[0] * dz[0], N4t[1] * dz[1], N4t[2] * dz[2]
        mb('q4.side.lhs', lhs)
        mb('q4.side.rhs+partiels', p0, p1, p2, p0 + p1, p0 + p1 + p2, 2 * (p0 + p1 + p2))
        mb('q4.side_key', lhs - 2 * (p0 + p1 + p2))
        key = D4t * (d2(z, c4t) - d2(a, c4t))
        got = rows['S4']
        note('side_key.q4', Fraction(int(got[0])) == key and int(got[1]) == sign(key))
        pa, pb = sub(c, b), sub(z, b)
        ww = cross(pa, pb)
        cc = tuple(N4t[i] + D4t * (a[i] - b[i]) for i in range(3))
        mb('q4.orient_center.cc', *cc)
        mb('q4.orient_center.produits+somme', ww[0] * cc[0], ww[1] * cc[1], ww[2] * cc[2], ww[0] * cc[0] + ww[1] * cc[1], dot(ww, cc))
        want = sign(dot(ww, cc))
        note('orient_center(i128).q4', int(rows['OC4'][0]) == want)
        note('orient_center_wide.q4', int(rows['OC4'][1]) == want)
        # centre strictement interieur au tetraedre : coordonnees barycentriques > 0
        T = (a, b, c, d)
        inside = True
        for f in range(4):
            p0_, p1_, p2_ = T[(f + 1) % 4], T[(f + 2) % 4], T[(f + 3) % 4]
            so = sign(det3(sub(p1_, p0_), sub(p2_, p0_), sub(T[f], p0_)))
            n_ = cross(sub(p1_, p0_), sub(p2_, p0_))
            sc = sign(sum(Fraction(n_[i]) * (c4t[i] - p0_[i]) for i in range(3)))
            if sc == 0 or sc != so:
                inside = False
        note('strictly_inside_tetra', (rows['IN4'][0] == '1') == inside)
        L4 = rows['L4']
        mb('q4.level.N_i^2', *(x * x for x in N4t))
        mb('q4.level.num', dot(N4t, N4t))
        mb('q4.level.den', D4t * D4t)
        note('level4', int(L4[1]) > 0 and Fraction(int(L4[0]), int(L4[1])) == d2(a, c4t))
        r4 = d2(a, c4t)
        if 'CMP44' in rows and prev_r4 is not None:
            note('compare(L4,L4)', int(rows['CMP44'][0]) == sign(prev_r4[0] - r4))
            mb('compare.produit_croise', prev_r4[1] * D4t * D4t, dot(N4t, N4t) * prev_r4[2])
        prev_r4 = (r4, dot(N4t, N4t), D4t * D4t)
    # ---- communs
    key = 2 * (d2(z, c2) - r2_2)
    note('side_key.q2', Fraction(int(rows['S2'][0])) == key and int(rows['S2'][1]) == sign(key))
    note('orient', int(rows['OR'][0]) == sign(det))
    ac = dot(sub(b, a), sub(c, a)) > 0 and dot(sub(a, b), sub(c, b)) > 0 and dot(sub(a, c), sub(b, c)) > 0
    note('acute', (rows['AC'][0] == '1') == ac)
    note('is_midpoint.q2', rows['MID'][0] == '1')
    mb('dot', dot(u, u), dot(v, v), dot(s, s))
    mb('cross', *w)
    L2 = rows['L2']
    note('level2', Fraction(int(L2[0]), int(L2[1])) == r2_2)
    cm = rows['CMP']
    if cm[0] != '9' and not collinear:
        note('compare(L2,L3)', int(cm[0]) == sign(r2_2 - d2(a, c3t)))
        l3n, l3d = dot(u, u) * dot(v, v) * dot(sub(c, b), sub(c, b)), 4 * dot(w, w)
        mb('compare.produit_croise', dot(u, u) * l3d, l3n * 4)
    if cm[1] != '9' and det != 0:
        note('compare(L2,L4)', int(cm[1]) == sign(r2_2 - d2(a, c4t)))
    if cm[2] != '9' and det != 0 and not collinear:
        note('compare(L3,L4)', int(cm[2]) == sign(d2(a, c3t) - d2(a, c4t)))
        mb('compare.produit_croise', l3n * D4t * D4t, dot(N4t, N4t) * l3d)

print('bits=%d configurations=%d ubsan_reports=%d' % (BITS, len(cfgs), len(ubsan)))
for name in sorted(seen):
    print('  %-40s controles=%7d desaccords=%7d' % (name, seen[name], bad.get(name, 0)))
print('  longueurs binaires maximales observees (valeurs vraies, telles que le code les evalue) :')
for name in sorted(maxbits):
    print('    %-36s %4d bits' % (name, maxbits[name]))
if ubsan:
    sites = {}
    for l in ubsan:
        k = l.split(' runtime error')[0].split('morsehgp3D_v10/')[-1].rsplit(':', 1)[0]
        sites[k] = sites.get(k, 0) + 1
    print('  sites UBSan (fichier:ligne -> occurrences) :')
    for k in sorted(sites):
        print('    %s -> %d' % (k, sites[k]))
sys.exit(1 if bad else 0)
