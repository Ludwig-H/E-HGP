"""Deterministic exact fixtures, independent of the native interval formulas."""
from fractions import Fraction as F
from math import gcd, lcm
from random import Random
import json

MAX32 = 2**32 - 1
MAX192 = 2**192 - 1


def limbs(n):
    return [(n >> 128) & (2**64-1), (n >> 64) & (2**64-1), n & (2**64-1)]


def signed(n):
    return [int(n > 0) - int(n < 0), *limbs(abs(n))]


def sphere(points):
    """Gram solve for a circumcenter, not the native polynomial construction."""
    a = points[0]
    vectors = [tuple(x-y for x, y in zip(p, a)) for p in points[1:]]
    dot = lambda u, v: sum(x*y for x, y in zip(u, v))
    rows = [[F(2*dot(u, v)) for v in vectors] + [F(dot(u, u))] for u in vectors]
    n = len(rows)
    for k in range(n):
        pivot = next(i for i in range(k, n) if rows[i][k])
        rows[k], rows[pivot] = rows[pivot], rows[k]
        divisor = rows[k][k]
        rows[k] = [v/divisor for v in rows[k]]
        for i in range(n):
            if i != k:
                factor = rows[i][k]
                rows[i] = [u-factor*v for u, v in zip(rows[i], rows[k])]
    weights = [row[-1] for row in rows]
    if not all(w > 0 for w in [1-sum(weights), *weights]):
        raise ValueError('support must be strictly positive')
    rho = [sum(w*v[j] for w, v in zip(weights, vectors)) for j in range(3)]
    d = lcm(*(v.denominator for v in rho))
    return [int(v*d) for v in rho], d


def panel():
    rng = Random(0x10_20260930)
    requests = []

    def add(op, **fields):
        requests.append(dict(id=len(requests), op=op, **fields))
        return requests[-1]['id']

    def point(n, d, a, x, label, **extra):
        return add('S', n=n, d=d, anchor=list(a), point=list(x), label=label, **extra)

    def box(n, d, a, lo, hi, label):
        return add('B', n=n, d=d, anchor=list(a), lo=list(lo), hi=list(hi), label=label)

    integers = {0, 1, 2, 3, MAX192}
    for bit in range(1, 192):
        integers.update((2**bit-1, 2**bit, 2**bit+1))
    for bits in range(54, 193):
        shift = bits-53
        head = rng.randrange(2**52, 2**53)
        integers.update((head << shift, (head << shift)+1,
                         (head << shift)+(2**shift-1)))
    integers.update(rng.randrange(1, MAX192) for _ in range(160))
    for n in sorted(integers):
        add('C', n=n, label='positive_conversion')
        if n:
            add('C', n=-n, label='negative_conversion')

    geometries = []
    for m in (1, 13, 4095, 2**18-1, 15_000_005, 15_000_010, MAX32):
        geometries.append(('triangle_'+str(m), [(0, 0, 0), (m, m, 0), (m, 0, m)]))
        geometries.append(('tetra_'+str(m), [(0, 0, 0), (m, m, 0), (m, 0, m), (0, m, m)]))
    geometries += [
        ('causal_lost', [(15_000_010, 0, 15_000_010), (15_000_010, 15_000_010, 0), (0, 0, 0)]),
        ('translated', [(4_000_000_000,)*3, (4_000_200_005, 4_000_000_000, 4_000_000_000),
                        (4_000_040_001, 4_000_200_005, 4_000_000_000)]),
    ]
    for m in (2**24-1, MAX32):
        geometries.append(('generic_tetra_'+str(m), [(0, 0, 0), (m, m-1, 1),
                                                   (m-2, 1, m), (1, m, m-2)]))
    for label, points in geometries:
        n, d = sphere(points)
        largest = max([d, *map(abs, n)])
        factors = [1, 2**max(0, 190-largest.bit_length())+1]
        for factor in factors:
            ns, ds = [factor*v for v in n], factor*d
            if max([ds, *map(abs, ns)]) > MAX192:
                raise ValueError('fixture exceeds prototype capacity')
            for p in points:
                point(ns, ds, points[0], p, label, exact_contact=True)
                for axis in range(3):
                    for delta in (-1, 1):
                        x = list(p)
                        x[axis] += delta
                        if 0 <= x[axis] <= MAX32:
                            point(ns, ds, points[0], x, label+'_jitter')
            for _ in range(4):
                lo = [rng.randrange(MAX32) for _ in range(3)]
                hi = [rng.randrange(v, MAX32+1) for v in lo]
                box(ns, ds, points[0], lo, hi, label+'_box')
            box(ns, ds, points[0], points[0], points[0], label+'_anchor_box')

    # General rational spheres exercise all signed limbs, without claiming
    # every rational sphere is a positive native Gabriel/MEB support.
    for _ in range(70):
        a = [rng.randrange(MAX32+1) for _ in range(3)]
        bits = rng.choice((1, 53, 54, 64, 128, 191, 192))
        n = [rng.randrange(-(2**bits-1), 2**bits) for _ in range(3)]
        d = rng.choice((1, 3, 2**53+1, 2**64-1, 2**128+1, MAX192))
        point(n, d, a, a, 'general_anchor', exact_contact=True)
        point(n, d, a, [rng.randrange(MAX32+1) for _ in range(3)], 'general_site')
        lo = [rng.randrange(MAX32) for _ in range(3)]
        hi = [rng.randrange(v, MAX32+1) for v in lo]
        box(n, d, a, lo, hi, 'general_box')
    for sign in (-1, 1):
        for d in (MAX192-1, 2**191-1):
            point([sign*(d+1), 0, 0], d, (0, 0, 0), (2, 0, 0), 'tiny_exact_power')
    # Translation must leave the *same* relative arithmetic bit for bit.
    for _ in range(40):
        a = [rng.randrange(5000) for _ in range(3)]
        x = [rng.randrange(5000) for _ in range(3)]
        n = [rng.randrange(-20000, 20000) for _ in range(3)]
        d = rng.choice((3, 10, 17, 2**65+1))
        previous = point(n, d, a, x, 'translation_original')
        shift = [MAX32-10000]*3
        point(n, d, [v+s for v, s in zip(a, shift)],
              [v+s for v, s in zip(x, shift)], 'translation_image', equal_to=previous)
    # Exact nearest tie from the old .02 counter-case; native nearest is NOT
    # implemented here, but its safe upper-order-statistic lemma is judged.
    m = 400_000_005
    r = 5*m
    q = [r, r, r]
    for site_id, p in enumerate(([r-3*m, r-4*m, r], q, [2*r, r, r])):
        point([0, 0, 0], 1, q, p, 'nearest_tie', nearest_group='tie', site_id=site_id)
    for _ in range(12):
        a = [rng.randrange(200000) for _ in range(3)]
        n = [rng.randrange(-200000, 200000) for _ in range(3)]
        d = rng.randrange(1, 20)
        group = 'rank_'+str(_)
        for site_id in range(12):
            p = [rng.randrange(200000) for _ in range(3)]
            point(n, d, a, p, 'nearest_random', nearest_group=group, site_id=site_id)
    for request in requests:
        coefficients = [request['n']] if request['op'] == 'C' else request['n']
        if any(abs(n) > MAX192 for n in coefficients):
            raise ValueError('coefficient outside 192-bit magnitude')
        if request['op'] != 'C' and not 1 <= request['d'] <= MAX192:
            raise ValueError('denominator outside 192-bit magnitude')
    return requests


def line(request):
    op = request['op']
    if op == 'C':
        values = signed(request['n'])
    else:
        values = request['anchor'] + (request['point'] if op == 'S' else request['lo']+request['hi'])
        values += [v for n in request['n'] for v in signed(n)] + limbs(request['d'])
    return op+' '+' '.join(map(str, values))


if __name__ == '__main__':
    print(json.dumps(panel(), sort_keys=True))
