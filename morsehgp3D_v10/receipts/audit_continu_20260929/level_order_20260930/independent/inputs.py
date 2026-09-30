"""Integer/rational comparison fixtures; no prototype arithmetic is imported."""
from fractions import Fraction as F
from math import lcm
from random import Random

MAX_N = 2**266-1
MAX_D = 2**200-1


def words(n, count):
    if type(n) is not int or not 0 <= n < 2**(64*count):
        raise ValueError('unsigned word domain')
    return [(n >> (64*i)) & (2**64-1) for i in range(count-1, -1, -1)]


def radius(points):
    """Independent exact Gram solve; certify strictly positive support."""
    a = points[0]
    vectors = [tuple(x-y for x, y in zip(p, a)) for p in points[1:]]
    dot = lambda u, v: sum(x*y for x, y in zip(u, v))
    rows = [[F(2*dot(u, v)) for v in vectors] + [F(dot(u, u))] for u in vectors]
    for k in range(len(rows)):
        pivot = next(i for i in range(k, len(rows)) if rows[i][k])
        rows[k], rows[pivot] = rows[pivot], rows[k]
        scale = rows[k][k]
        rows[k] = [v/scale for v in rows[k]]
        for i in range(len(rows)):
            if i != k:
                factor = rows[i][k]
                rows[i] = [u-factor*v for u, v in zip(rows[i], rows[k])]
    weights = [row[-1] for row in rows]
    if not all(w > 0 for w in [1-sum(weights), *weights]):
        raise ValueError('support not strictly positive')
    rho = [sum(w*v[j] for w, v in zip(weights, vectors)) for j in range(3)]
    return sum(v*v for v in rho)


def level(points):
    """Unreduced exact level, cross-checked against an independent Gram solve."""
    a = points[0]
    v = [tuple(x-y for x, y in zip(p, a)) for p in points[1:]]
    dot = lambda u, w: sum(x*y for x, y in zip(u, w))
    cross = lambda u, w: (u[1]*w[2]-u[2]*w[1], u[2]*w[0]-u[0]*w[2], u[0]*w[1]-u[1]*w[0])
    if len(points) == 2:
        n, d = dot(v[0], v[0]), 4
    elif len(points) == 3:
        u, w = v
        delta = tuple(x-y for x, y in zip(w, u))
        c = cross(u, w)
        n, d = dot(u, u)*dot(w, w)*dot(delta, delta), 4*dot(c, c)
    else:
        u, w, t = v
        det = dot(u, cross(w, t))
        coeff = [sum(norm*c[j] for norm, c in
                     zip((dot(u,u), dot(w,w), dot(t,t)),
                         (cross(w,t), cross(t,u), cross(u,w)))) for j in range(3)]
        n, d = sum(c*c for c in coeff), 4*det*det
    expected = radius(points)
    if not d or F(n,d) != expected or n > MAX_N or d > MAX_D:
        raise ValueError('independent geometric level/domain disagreement')
    return dict(n=n, d=d, support=[list(p) for p in points], radius=str(expected))


def panel():
    rng = Random(0x20260930_512)
    rows = []

    def compare(n1,d1,n2,d2,label,**metadata):
        rows.append(dict(id=len(rows),op='C',n1=n1,d1=d1,n2=n2,d2=d2,label=label,**metadata))

    def multiply(n,d,label):
        rows.append(dict(id=len(rows),op='M',n=n,d=d,label=label))

    for n1,d1,n2,d2 in [(0,1,0,MAX_D),(0,MAX_D,1,MAX_D),
                       (MAX_N,MAX_D,MAX_N,MAX_D),(MAX_N,1,MAX_N-1,1),
                       (2**256+1,1,2**256,1),(2**256,1,2**256+1,1),
                       (MAX_N,MAX_D,MAX_N,MAX_D-1),(1,MAX_D,1,MAX_D-1)]:
        compare(n1,d1,n2,d2,'extreme_or_close')
    for bit in range(1,266):
        d = 2**min(bit,199)+1
        compare(2**bit-1,d,2**bit,d,'numerator_bit_boundary')
        compare(2**bit,d,2**bit-1,d,'antisymmetric_image')
    for bit in range(1,200):
        n = 2**min(bit+50,265)+1
        compare(n,2**bit-1,n,2**bit,'denominator_bit_boundary')
    for _ in range(250):
        n = rng.randrange(0,2**128)
        d = rng.randrange(1,2**96)
        factor = rng.randrange(1,2**100)
        compare(n,d,n*factor,d*factor,'unreduced_equality')
    for _ in range(450):
        n1,n2 = (rng.randrange(2**rng.randrange(0,267)) for _ in range(2))
        d1,d2 = (rng.randrange(1,2**rng.randrange(1,201)+1) for _ in range(2))
        compare(n1,d1,n2,d2,'random_domain')
    geometries = []
    for m in (1,13,4095,2**18-1,2**21-1,2**24-1,2**32-1):
        t = [(0,0,0),(m,m,0),(m,0,m),(0,m,m)]
        for q in (2,3,4):
            geometries.append(level(t[:q]))
    for m in (2**24-1,2**32-1):
        t = [(0,0,0),(m,m-1,1),(m-2,1,m),(1,m,m-2)]
        for q in (2,3,4):
            geometries.append(level(t[:q]))
    for a in geometries:
        for b in geometries:
            compare(a['n'],a['d'],b['n'],b['d'],'positive_support_levels',
                    geometry_a=a,geometry_b=b)
    for bits_n,bits_d in [(0,0),(64,64),(128,128),(266,200),(320,256),(320,193),(257,256)]:
        for delta in (-1,0,1):
            n = max(0,min(2**320-1,2**bits_n+delta))
            d = max(0,min(2**256-1,2**bits_d+delta))
            multiply(n,d,'raw_product_boundary')
    for _ in range(250):
        n = rng.randrange(2**rng.randrange(0,321))
        d = rng.randrange(2**rng.randrange(0,257))
        multiply(n,d,'raw_product_random')
    multiply(2**320-1,2**256-1,'raw_product_definite_overflow')
    for n1,d1,n2,d2 in [(2**266,1,0,1),(0,2**200,0,1),(0,0,0,1),
                       (0,1,2**266,1),(0,1,0,2**200),(0,1,0,0)]:
        compare(n1,d1,n2,d2,'domain_refusal')
    return rows


def line(row):
    if row['op'] == 'C':
        values = words(row['n1'],5)+words(row['d1'],4)+words(row['n2'],5)+words(row['d2'],4)
    else:
        values = words(row['n'],5)+words(row['d'],4)
    return row['op']+' '+' '.join(map(str,values))
