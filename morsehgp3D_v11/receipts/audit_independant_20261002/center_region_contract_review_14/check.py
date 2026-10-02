#!/usr/bin/env python3
"""CentreRegion : modèles autonomes exacts, sans import ni exécution du produit."""
from fractions import Fraction as F
from itertools import permutations, product
from pathlib import Path
import hashlib
import json


def need(condition, message):
    if not condition:
        raise ValueError(message)


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def affine(a, b):
    return sub(a, b), dot(a, a)-dot(b, b)


def pair_corners(a, b, lo, hi):
    values = [dot(sub(a, z), sub(a, z))-dot(sub(b, z), sub(b, z)) for z in product(*zip(lo, hi))]
    return min(values) <= 0 <= max(values)


def line_clip(a, b, c, lo, hi):
    # Paramétrisation directe de 2u.z=A,2v.z=B, puis intersection des six faces.
    # Aucun zonogone ni test de normales dans cet oracle.
    u, constant_u = affine(a, b)
    v, constant_v = affine(a, c)
    direction = cross(u, v)
    if direction == (0, 0, 0):
        return 'degenerate', None
    free = next(i for i in range(3) if direction[i])
    j, k = [i for i in range(3) if i != free]
    determinant = u[j]*v[k]-u[k]*v[j]
    origin = [F(0)]*3
    origin[j] = F(constant_u*v[k]-u[k]*constant_v, 2*determinant)
    origin[k] = F(u[j]*constant_v-constant_u*v[j], 2*determinant)
    need(2*dot(u, origin) == constant_u and 2*dot(v, origin) == constant_v, 'origine droite')
    low_t, high_t = None, None
    for i in range(3):
        if not direction[i]:
            if not lo[i] <= origin[i] <= hi[i]:
                return 'disjoint', None
            continue
        ends = sorted((F(lo[i]-origin[i], direction[i]), F(hi[i]-origin[i], direction[i])))
        low_t = ends[0] if low_t is None else max(low_t, ends[0])
        high_t = ends[1] if high_t is None else min(high_t, ends[1])
    if low_t > high_t:
        return 'disjoint', None
    witness = tuple(origin[i]+low_t*direction[i] for i in range(3))
    need(all(lo[i] <= witness[i] <= hi[i] for i in range(3)), 'témoin fermeture')
    return 'intersects', witness


def zonogon(a, b, c, lo, hi, bits):
    # Expression SAT dérivée à la main, comparée à la voie paramétrique indépendante.
    maximum = 1 << bits
    u, const_u = affine(a, b)
    v, const_v = affine(a, c)
    widths = tuple(h-l for l, h in zip(lo, hi))
    center_twice = tuple(h+l for l, h in zip(lo, hi))
    p0, p1 = const_u-dot(u, center_twice), const_v-dot(v, center_twice)
    need(abs(const_u) < 3*maximum**2 and abs(const_v) < 3*maximum**2, 'budget constantes')
    need(abs(p0) < 9*maximum**2 and abs(p1) < 9*maximum**2, 'budget centrage')
    direction = cross(u, v)
    if direction == (0, 0, 0):
        return 'degenerate', 0
    max_left = 0
    for k in range(3):
        if u[k] == 0 and v[k] == 0:
            continue
        left = abs(v[k]*p0-u[k]*p1)
        right = sum(widths[j]*abs(v[k]*u[j]-u[k]*v[j]) for j in range(3))
        need(left < 18*maximum**3 and right < 4*maximum**3, 'budget SAT')
        max_left = max(max_left, left)
        if left > right:
            return 'disjoint', max_left
    return 'intersects', max_left


def main():
    root = Path(__file__).resolve().parent
    before = json.loads((root/'SOURCE_BEFORE.json').read_text())
    for row in before['files']:
        need(hashlib.sha256((root/'source_wip'/row['path']).read_bytes()).hexdigest() == row['wip_sha256'],
             'source WIP figée '+row['path'])
    base_triangles = (
        ('droit', ((0, 0, 0), (4, 0, 0), (0, 4, 0))),
        ('obtus', ((0, 0, 0), (4, 0, 0), (1, 1, 0))),
        ('oblique', ((1, 1, 0), (4, 0, 3), (0, 4, 2))),
        ('contact_sommet', ((2, 2, 1), (1, 2, 2), (0, 0, 1))),
        ('paires_non_suffisantes', ((7, 4, 2), (7, 0, 1), (7, 3, 4))),
        ('aligne', ((0, 0, 0), (1, 1, 1), (4, 4, 4))),
        ('doublon', ((0, 0, 0), (0, 0, 0), (0, 4, 0))),
    )
    regions = (((0, 0, 0), (1, 1, 1)), ((0, 0, 0), (2, 2, 2)),
               ((2, 2, 0), (3, 3, 1)), ((4, 4, 4), (5, 5, 5)))
    lines, pairs = 0, 0
    counts = {'degenerate': 0, 'disjoint': 0, 'intersects': 0}
    extreme_left = {}
    for bits in (18, 21, 24):
        m = (1 << bits)-1
        triangles = base_triangles + (('extreme_elargissement', ((0, 0, 0), (m, m, 0), (m, 0, m))),)
        all_regions = regions + (((0, 0, 0), (m+1, m+1, m+1)), ((m, m, m), (m+1, m+1, m+1)))
        for name, triangle in triangles:
            for lo, hi in all_regions:
                for perm in permutations(triangle):
                    wanted, witness = line_clip(*perm, lo, hi)
                    got, left = zonogon(*perm, lo, hi, bits)
                    need(got == wanted, 'SAT / paramétrique '+name)
                    lines += 1
                    counts[got] += 1
                for i, j in ((0, 1), (0, 2), (1, 2)):
                    a, b = triangle[i], triangle[j]
                    u, constant = affine(a, b)
                    endpoints = [constant-2*dot(u, z) for z in product(*zip(lo, hi))]
                    need(pair_corners(a, b, lo, hi) == (min(endpoints) <= 0 <= max(endpoints)), 'bissectrice')
                    pairs += 1
        a, b, c = triangles[-1][1]
        result, left = zonogon(a, b, c, (0, 0, 0), (1, 1, 1), bits)
        extreme_left[str(bits)] = dict(left=str(left), bit_length=left.bit_length(), exceeds_i64=left > (1 << 63)-1)
        if bits >= 21:
            need(extreme_left[str(bits)]['exceeds_i64'], 'fixture multiplication avant élargissement')
        maximum = 1 << bits
        accepted = lambda lo, hi: all(0 <= l < h <= maximum for l, h in zip(lo, hi))
        need(accepted((m, m, m), (maximum, maximum, maximum)), 'extrémité centre M autorisée')
        for lo, hi in (((-1, 0, 0), (1, 1, 1)), ((0, 0, 0), (0, 1, 1)),
                       ((2, 0, 0), (1, 1, 1)), ((0, 0, 0), (maximum+1, 1, 1))):
            need(not accepted(lo, hi), 'refus domaine')

    missed = base_triangles[4][1]
    lo, hi = (0, 0, 0), (2, 2, 2)
    need(all(pair_corners(missed[i], missed[j], lo, hi) for i, j in ((0, 1), (0, 2), (1, 2))), 'toutes paires rencontrent')
    need(line_clip(*missed, lo, hi)[0] == 'disjoint', 'droite commune manque')
    relation, contact = line_clip(*base_triangles[3][1], (0, 0, 0), (1, 1, 1))
    need(relation == 'intersects' and contact == (1, 1, 1), 'contact fermé au sommet')
    print(json.dumps(dict(status='PASS',profiles=[18,21,24],line_comparisons=lines,pair_comparisons=pairs,
                          relations=counts,extreme_left=extreme_left,closed_contact=[str(x) for x in contact],
                          source_wip_files=len(before['files']),native_runs=0,product_imports=0),sort_keys=True))


if __name__ == '__main__':
    main()
