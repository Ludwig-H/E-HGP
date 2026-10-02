#!/usr/bin/env python3
"""Delta publié : gardes scalaires, sans import ni exécution produit."""
from pathlib import Path
import hashlib
import json
import random


def need(condition, message):
    if not condition:
        raise ValueError(message)


def trace(points, lo, hi):
    a, b, c = points
    u = tuple(x-y for x, y in zip(a, b))
    v = tuple(x-y for x, y in zip(a, c))
    if all(u[i]*v[j]-u[j]*v[i] == 0 for i in range(3) for j in range(i+1, 3)):
        return [], 'degenerate'
    p0 = sum(x*x-y*y for x, y in zip(a, b))-sum((l+h)*x for l, h, x in zip(lo, hi, u))
    p1 = sum(x*x-y*y for x, y in zip(a, c))-sum((l+h)*x for l, h, x in zip(lo, hi, v))
    values = []
    for k in range(3):
        if u[k] == 0 and v[k] == 0:
            continue
        terms = (v[k]*p0, u[k]*p1)
        signed = terms[0]-terms[1]
        right = sum((hi[j]-lo[j])*abs(v[k]*u[j]-u[k]*v[j]) for j in range(3))
        values.append(dict(k=k, terms=terms, signed_left=signed, left=abs(signed), right=right))
        if abs(signed) > right:
            return values, 'disjoint'
    return values, 'intersects'


def magnitude(row):
    return max(abs(row['signed_left']), abs(row['terms'][0]), abs(row['terms'][1]), row['right'])


def announced_random_inputs(bits):
    # Reconstruction locale de la SEULE génération d'entrée publiée, pas de son juge.
    # Les tirages q2 doivent être consommés pour préserver l'état du générateur avant q3.
    rng = random.Random(110031)
    m = (1 << bits)-1
    for q in (2, 3):
        for scale in (7, m):
            for i in range(40):
                points = tuple(tuple(rng.randrange(scale+1) for _ in range(3)) for _ in range(3))
                lo = tuple(rng.randrange(scale+1) for _ in range(3))
                hi = tuple(rng.randrange(x+1, scale+2) for x in lo)
                if q == 3:
                    yield 'random_q%d_s%d_%d' % (q, scale, i), points, lo, hi


def main():
    root = Path(__file__).resolve().parent
    bindings = json.loads((root/'FILE_BINDINGS.json').read_text())
    blobs = {}
    for row in bindings['files']:
        data = (root/row['binding']).read_bytes()
        need(hashlib.sha256(data).hexdigest() == row['sha256'], 'source binding '+row['path'])
        blobs[row['path']] = data.decode()

    # Nouveau témoin natif : seules les deux premières normales passent.
    points = ((4, 7, 2), (2, 2, 7), (6, 2, 2))
    rows, relation = trace(points, (0, 0, 0), (2, 2, 2))
    need([(r['left'], r['right']) for r in rows] == [(54, 60), (55, 90), (95, 70)], 'nouvelle normale k2')
    need(relation == 'disjoint' and all(r['left'] <= r['right'] for r in rows[:2]), 'omission k2 change verdict')
    # Voie géométrique directe indépendante : -2x+5y=25/2 -> y=5/2+2x/5>2 dans Q.
    need(all(10*y-4*x < 25 for x in (0, 2) for y in (0, 2)), 'plan exclu de la boîte')

    cpp = blobs['morsehgp3D_v11/src/num/center_region.cpp']
    numeric = json.loads(blobs['morsehgp3D_v11/tests/mutants/num.json'])
    region_mutants = [m for m in numeric['mutants'] if m['id'].startswith('region_')]
    need(len(region_mutants) == 6, 'six mutants numériques déclarés')
    for mutation in region_mutants:
        need(cpp.count(mutation['cherche']) == 1, 'motif mutant unique '+mutation['id'])

    profiles = []
    for bits in (18, 21, 24):
        m = (1 << bits)-1
        simple, _ = trace(((0, 0, 0), (m, 0, 0), (0, m, 0)), (0, 0, 0), (1, 1, 1))
        explicit, _ = trace(((0, 0, 0), (m, m, 0), (m, 0, m)), (0, 0, 0), (1, 1, 1))
        simple_left = max(r['left'] for r in simple)
        explicit_left = max(r['left'] for r in explicit)
        need(simple_left == m**3-m**2 and explicit_left == 2*m**3-2*m**2, 'valeurs extrêmes exactes')
        largest, over, first = 0, 0, None
        for name, points, lo, hi in announced_random_inputs(bits):
            values, _ = trace(points, lo, hi)
            for row in values:
                largest = max(largest, magnitude(row))
                if magnitude(row) > (1 << 63)-1:
                    over += 1
                    if first is None:
                        first = dict(name=name, k=row['k'])
        if bits == 21:
            need(simple_left <= (1 << 63)-1 < explicit_left and over == 0, 'portée de couverture B21')
        if bits == 24:
            need(over > 0, 'intermédiaires B24 au-delà i64')
        profiles.append(dict(bits=bits, simple_axes_left=str(simple_left), proposed_left=str(explicit_left),
                             proposed_exceeds_i64=explicit_left > (1 << 63)-1,
                             local_announced_random_largest=str(largest),
                             local_announced_random_normals_over_i64=over, first=first))
    print(json.dumps(dict(status='PASS',pin=bindings['pin'],bound_sources=len(bindings['files']),
                          new_k2_left_right=[[r['left'],r['right']] for r in rows],
                          numeric_region_mutants_unique=len(region_mutants),profiles=profiles,
                          native_runs=0,product_imports=0,mutants_executed=0),sort_keys=True))


if __name__ == '__main__':
    main()
