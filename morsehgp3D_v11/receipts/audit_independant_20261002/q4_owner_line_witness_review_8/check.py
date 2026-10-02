"""Exact proposal checks; no import, build or execution of the native product."""
from fractions import Fraction as Q
from itertools import permutations, product
import json
import random

checks = 0


def require(ok, note):
    global checks
    checks += 1
    if not ok:
        raise RuntimeError(note)


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def solve(rows, rhs):
    matrix = [[Q(v) for v in row]+[Q(r)] for row, r in zip(rows, rhs)]
    for j in range(3):
        pivot = next((i for i in range(j, 3) if matrix[i][j]), None)
        if pivot is None:
            return None
        matrix[j], matrix[pivot] = matrix[pivot], matrix[j]
        divisor = matrix[j][j]
        matrix[j] = [v/divisor for v in matrix[j]]
        for i in range(3):
            if i != j:
                factor = matrix[i][j]
                matrix[i] = [x-factor*y for x, y in zip(matrix[i], matrix[j])]
    return tuple(row[-1] for row in matrix)


def triple(t):
    a, b, c = t
    v, w = sub(b, a), sub(c, a)
    n = cross(v, w)
    if not any(n):
        return None
    d = 2*dot(n, n)
    wxn, nxv = cross(w, n), cross(n, v)
    N = tuple(dot(v, v)*x+dot(w, w)*y for x, y in zip(wxn, nxv))
    center = tuple(Q(x)+Q(y, d) for x, y in zip(a, N))
    gram = solve((v, w, n), (Q(dot(v, v), 2), Q(dot(w, w), 2), 0))
    require(gram is not None and tuple(Q(x, d) for x in N) == gram, 'circle versus independent Gram solve')
    require(dot(n, N) == 0, 'relative circle center is in the plane')
    return a, n, N, d, center


def interval(a, n, N, d, box):
    lo, hi = box
    lower, upper = None, None
    for i in range(3):
        left, right = d*(lo[i]-a[i])-N[i], d*(hi[i]-a[i])-N[i]
        if n[i] == 0:
            if not left <= 0 <= right:
                return None
            continue
        ends = sorted((Q(left, n[i]), Q(right, n[i])))
        lower = ends[0] if lower is None else max(lower, ends[0])
        upper = ends[1] if upper is None else min(upper, ends[1])
    require(lower is not None and upper is not None, 'nonzero normal implies bounded clipping')
    return None if lower > upper else (lower, upper)


def witness(data, box, z):
    a, n, N, d, _ = data
    bounds = interval(a, n, N, d, box)
    if bounds is None:
        return None
    delta = sub(z, a)
    F = d*dot(delta, delta)-2*dot(N, delta)
    S = dot(n, delta)
    values = tuple(F-2*u*S for u in bounds)
    return max(values) < 0, values


def sphere4(t, s):
    a = t[0]
    diffs = [sub(p, a) for p in (*t[1:], s)]
    relative = solve(diffs, [Q(dot(v, v), 2) for v in diffs])
    if relative is None:
        return None
    center = tuple(Q(x)+y for x, y in zip(a, relative))
    return center, dot(relative, relative)


def inside_box(c, box):
    lo, hi = box
    return all(l <= x <= h for l, x, h in zip(lo, c, hi))


def power(c, r2, z):
    delta = sub(z, c)
    return dot(delta, delta)-r2


# Noncoplanar witness admitted by owner clipping, excluded by unrestricted family.
T = ((10, 5, 50), (2, 9, 50), (2, 1, 50))
box = ((4, 4, 50), (6, 6, 52))
z = (5, 5, 51)
owned_s, outside_s = (5, 5, 56), (5, 5, 9)
data = triple(T)
require(dot(data[1], sub(z, T[0])) != 0, 'witness is not coplanar')
require(witness(data, box, z)[0], 'strict common witness on owner line')
c, r2 = sphere4(T, owned_s)
require(inside_box(c, box) and power(c, r2, z) < 0, 'owned extension contains witness')
c2, r22 = sphere4(T, outside_s)
require(not inside_box(c2, box) and power(c2, r22, z) == Q(672, 41), 'unowned extension invalidates unrestricted assertion')
for contact in (*T, (5, 5, 55)):
    require(not witness(data, box, contact)[0], 'contact never receives strict credit')
require(interval(*data[:4], ((0, 0, 0), (1, 1, 1))) is None, 'empty owner line')

# Three nonzero normal components: all constraints, reversals and permutations.
T3 = ((12, 10, 8), (8, 12, 10), (10, 8, 12))
box3 = ((9, 10, 11), (12, 13, 14))
for t in permutations(T3):
    td = triple(t)
    bounds = interval(*td[:4], box3)
    require(bounds is not None, 'nonempty three-axis clipping')
    centers = [tuple(Q(a)+Q(N+u*n, td[3]) for a, N, n in zip(td[0], td[2], td[1])) for u in bounds]
    require(set(centers) == {(Q(11), Q(11), Q(11)), (Q(12), Q(12), Q(12))}, 'all three constraints select intersection')

# Exhaust corners for the common-anchor projected cross bound.
corners = tuple(product((0, 1), repeat=2))
for a, b, c in product(corners, repeat=3):
    v, w = sub(b, a), sub(c, a)
    require(abs(v[0]*w[1]-v[1]*w[0]) <= 1, 'unit square area bound')

rng = random.Random(20261002)
owned_checks, credits = 0, 0
for unused in range(64):
    points = [tuple(rng.randrange(17) for _ in range(3)) for _ in range(16)]
    t = tuple(points[:3])
    td = triple(t)
    if td is None:
        continue
    box_r = ((3, 3, 3), (13, 13, 13))
    for z_r in points[3:9]:
        result = witness(td, box_r, z_r)
        if result is None:
            continue
        flag, vals = result
        credits += int(flag)
        for s in points[9:]:
            sph = sphere4(t, s)
            if sph is None or not inside_box(sph[0], box_r):
                continue
            center, radius = sph
            u = next(Q((center[i]-td[0][i])*td[3]-td[2][i], td[1][i]) for i in range(3) if td[1][i])
            delta = sub(z_r, td[0])
            F = td[3]*dot(delta, delta)-2*dot(td[2], delta)
            require(Q(F-2*u*dot(td[1], delta), td[3]) == power(center, radius, z_r), 'affine power agrees with independently solved sphere')
            require(min(vals) <= td[3]*power(center, radius, z_r) <= max(vals), 'power is between clipped endpoint values')
            require(not flag or power(center, radius, z_r) < 0, 'only strict interiors credited')
            owned_checks += 1
require(owned_checks > 0 and credits > 0, 'positive random branches exercised')

# Exact profile/domain intermediates, including maximum-sized grid boxes.
profiles = []
for B in (18, 21, 24):
    M = 1 << B
    for t in (((0, 0, 0), (M-1, M-1, 0), (M-1, 0, M-1)),
              ((M-1, 0, 0), (0, M-1, 0), (0, 0, M-1)),
              ((0, 0, 0), (M-1, 1, 0), (M-1, 0, 1))):
        td = triple(t)
        a, n, N, d, circle = td
        require(all(abs(v) < M*M for v in n), 'normal common-anchor bound')
        require(d < 24*M**4 and all(abs(v) < 24*M**5 for v in N), 'certified coarse q3 coefficients')
        for z_b in product((0, M-1), repeat=3):
            delta = sub(z_b, a)
            F, S = d*dot(delta, delta)-2*dot(N, delta), dot(n, delta)
            require(abs(F) < 216*M**6 and abs(S) < 3*M**3, 'power and normal projection bounds')
            for i in range(3):
                for edge in (0, M):
                    A = d*(edge-a[i])-N[i]
                    require(abs(A) < 48*M**5 and abs(A) < 1 << 127, 'clipping numerator native i128')
                    if n[i]:
                        h = Q(A, n[i])
                        partials = (F*h.denominator, -2*h.numerator*S)
                        require(sum(abs(v) for v in partials) < 504*M**8, 'every degree-eight partial bounded')
                        require(abs(sum(partials)) < 1 << (8*B+9), 'signed comparison budget')
    profiles.append({'bits': B, 'sign_budget_bits': 8*B+9, 'proposed_wide_words': (8*B+9+63)//64})

print(json.dumps({'status': 'PASS', 'checks': checks, 'owned_sphere_comparisons': owned_checks,
                  'strict_witnesses_in_random_cases': credits, 'profiles': profiles,
                  'outside_owner_power': str(power(c2, r22, z)),
                  'scope': 'future owner-line family rejection; no native code or performance qualification'}, sort_keys=True))
