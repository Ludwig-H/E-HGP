#!/usr/bin/env python3
"""Independent rational/integer centre-domain gate; no product import.

The tests support a conservative certificate with UNKNOWN/fallback. They do
not qualify a product implementation, global candidate generator, or FULL.
"""
from fractions import Fraction as F
from itertools import product
import json
import random


def require(ok, why):
    if not ok:
        raise RuntimeError(why)


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def distance(a, b):
    d = sub(a, b)
    return dot(d, d)


def solve(matrix, rhs):
    rows = [[F(x) for x in row]+[F(rhs[i])] for i, row in enumerate(matrix)]
    for col in range(len(rhs)):
        pivot = next((i for i in range(col, len(rhs)) if rows[i][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        value = rows[col][col]
        rows[col] = [x/value for x in rows[col]]
        for i in range(len(rhs)):
            if i != col:
                value = rows[i][col]
                rows[i] = [x-value*y for x, y in zip(rows[i], rows[col])]
    return tuple(row[-1] for row in rows)


def ball(points):
    edges = [sub(p, points[0]) for p in points[1:]]
    coef = solve([[dot(u, v) for v in edges] for u in edges], [F(dot(u, u), 2) for u in edges])
    if coef is None:
        return None
    shift = tuple(sum(coef[i]*edges[i][d] for i in range(len(edges))) for d in range(3))
    return tuple(points[0][d]+shift[d] for d in range(3)), dot(shift, shift), (1-sum(coef),)+coef


def frame(a, b):
    v = sub(b, a)
    D = dot(v, v)
    if not D:
        raise ValueError("repeated endpoints")
    k = max(range(3), key=lambda d: abs(v[d]))
    i, j = [d for d in range(3) if d != k]
    h = abs(v[k])
    sign = 1 if v[k] > 0 else -1
    A, B = [0]*3, [0]*3
    A[i], A[k], B[j], B[k] = h, -sign*v[i], h, -sign*v[j]
    require(dot(A, v) == dot(B, v) == 0, "basis leaves the bisector plane")
    require(dot(A, A)*dot(B, B)-dot(A, B)**2 == h*h*D, "Gram determinant")
    require(D <= 3*h*h, "largest-coordinate condition number")
    return v, D, h, i, j, tuple(A), tuple(B)


def centre(fr, xi, eta):
    return tuple(fr[5][d]*xi+fr[6][d]*eta for d in range(3))


def wpoint(a, b, z):
    return tuple(2*z[d]-a[d]-b[d] for d in range(3))


def projection(v, D, w):
    return tuple(F(D*w[d]-dot(w, v)*v[d], D) for d in range(3))


def form(a, b, z, t):
    w = wpoint(a, b, z)
    return dot(w, w)-distance(a, b)-2*dot(w, t)


def hull(points):
    """Strict convex hull of distinct integer 2D keys, preserving raw w."""
    by_key = {}
    for key, raw in points:
        by_key.setdefault(key, raw)
    keys = sorted(by_key)
    if len(keys) <= 1:
        return [(key, by_key[key]) for key in keys]
    def turn(a, b, c):
        return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    lower, upper = [], []
    for target, order in ((lower, keys), (upper, keys[::-1])):
        for key in order:
            while len(target) >= 2 and turn(target[-2], target[-1], key) <= 0:
                target.pop()
            target.append(key)
    return [(key, by_key[key]) for key in lower[:-1]+upper[:-1]]


def projected_domain(a, b, eligible):
    fr = frame(a, b)
    v, D, _, i, j, _, _ = fr
    if not eligible:
        return [((0, 0), (0, 0, 0))], []
    intervals = [(min(z[d] for z in eligible), max(z[d] for z in eligible)) for d in range(3)]
    raw = [(0, 0, 0)]+[wpoint(a, b, z) for z in product(*intervals)]
    vertices = hull([((D*w[i]-dot(w, v)*v[i], D*w[j]-dot(w, v)*v[j]), w) for w in raw])
    facets = []
    if len(vertices) >= 3:
        for pos, (_, p) in enumerate(vertices):
            q = vertices[(pos+1) % len(vertices)][1]
            normal = cross(v, sub(q, p))
            level = dot(normal, p)
            values = [dot(normal, r)-level for _, r in vertices]
            if max(values) > 0:
                normal = tuple(-x for x in normal)
                level = -level
            require(all(dot(normal, r) <= level for _, r in vertices), "facet orientation")
            facets.append((normal, level))
    return vertices, facets


def in_hull(fr, vertices, t):
    _, D, _, i, j, _, _ = fr
    p = (D*t[i], D*t[j])
    keys = [key for key, _ in vertices]
    def orient(a, b, c):
        return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    if len(keys) == 1:
        return p == keys[0]
    if len(keys) == 2:
        return orient(keys[0], keys[1], p) == 0 and all(min(keys[0][d], keys[1][d]) <= p[d] <= max(keys[0][d], keys[1][d]) for d in range(2))
    return all(orient(keys[pos], keys[(pos+1) % len(keys)], p) >= 0 for pos in range(len(keys)))


def check_tetra(points, extra, counts):
    answer = ball(points)
    if answer is None or min(answer[2]) <= 0:
        return
    c, radius, weights = answer
    ai, bi = min(((i, j) for i in range(4) for j in range(i+1, 4)),
                 key=lambda ij: (-distance(points[ij[0]], points[ij[1]]), ij))
    a, b = points[ai], points[bi]
    fr = frame(a, b)
    v, D, h, i, j, A, B = fr
    cloud = list(points)+list(extra)
    eligible = [z for z in cloud if z != a and z != b and distance(a, z) <= D and distance(b, z) <= D]
    t = tuple(2*c[d]-a[d]-b[d] for d in range(3))
    require(dot(t, v) == 0 and centre(fr, t[i]/h, t[j]/h) == t, "centre coordinates")
    require(2*dot(t, t) <= D, "Jung disk missed a positive owned tetra")
    require(abs(t[i]/h) <= 2 and abs(t[j]/h) <= 2, "root square missed a centre")
    projected = [projection(v, D, wpoint(a, b, z)) for z in points]
    require(t == tuple(sum(weights[r]*projected[r][d] for r in range(4)) for d in range(3)),
            "positive-centre projected convex combination")
    vertices, facets = projected_domain(a, b, eligible)
    require(len(vertices) <= 9 and in_hull(fr, vertices, t), "completion AABB hull missed a positive centre")
    require(all(dot(normal, t) <= level for normal, level in facets), "stored 3D facet missed a centre")
    # Test rational power, without using the family P/B implementation.
    for z in cloud:
        require(form(a, b, z, t) == 4*(dot(sub(z, c), sub(z, c))-radius), "affine power identity")
        counts["power_checks"] += 1
    # Small dyadic cell enclosing the exact centre: a conservative OUT cannot kill it.
    q = 1 << 10
    alpha, beta = t[i]*q/h, t[j]*q/h
    alo, blo = alpha.numerator//alpha.denominator, beta.numerator//beta.denominator
    corners = [centre(fr, F(x, q), F(y, q)) for x, y in product((alo, alo+1), (blo, blo+1))]
    intervals = [(min(p[d] for p in corners), max(p[d] for p in corners)) for d in range(3)]
    norm_min = sum(0 if lo <= 0 <= hi else min(lo*lo, hi*hi) for lo, hi in intervals)
    require(2*norm_min <= D, "3D AABB disk OUT killed a containing cell")
    require(not any(min(dot(normal, p)-level for p in corners) > 0 for normal, level in facets),
            "polygon OUT killed a containing cell")
    counts["positive_tetrahedra"] += 1
    counts["cell_domain_checks"] += 2


def rotation(quaternion):
    w, x, y, z = quaternion
    matrix = ((w*w+x*x-y*y-z*z, 2*(x*y-w*z), 2*(x*z+w*y)),
              (2*(x*y+w*z), w*w-x*x+y*y-z*z, 2*(y*z-w*x)),
              (2*(x*z-w*y), 2*(y*z+w*x), w*w-x*x-y*y+z*z))
    norm = sum(q*q for q in quaternion)
    require(all(dot(matrix[i], matrix[j]) == (norm*norm if i == j else 0) for i in range(3) for j in range(3)),
            "integer rotation/similarity matrix")
    return matrix


def main():
    counts = dict(positive_tetrahedra=0, power_checks=0, cell_domain_checks=0, rotation_cases=0)
    obtuse = ((10,20,20), (30,20,20), (20,29,27), (20,11,22))
    regular = ((0,0,0), (2,2,0), (2,0,2), (0,2,2))
    answer = ball(obtuse)
    require(answer[0] == (F(20), F(1135,54), F(125,6)) and min(answer[2]) > 0,
            "obtuse-completion fixture changed")
    require(min(ball(obtuse[:3])[2]) > 0 and min(ball((obtuse[0], obtuse[1], obtuse[3]))[2]) < 0,
            "fixture does not separate acute seed and obtuse completion")
    a, b, x, y = obtuse
    fr = frame(a, b)
    t = tuple(2*answer[0][d]-a[d]-b[d] for d in range(3))
    true_domain, _ = projected_domain(a, b, (x,y))
    wrong_domain, _ = projected_domain(a, b, (x,))
    require(in_hull(fr, true_domain, t) and not in_hull(fr, wrong_domain, t), "seeds-only AABB mutant survived")

    rng = random.Random(20260920)
    for points in (obtuse, regular):
        check_tetra(points, (), counts)
        for quaternion in ((1,1,1,0), (1,2,3,1), (1,1,2,3), (0,1,1,1)):
            matrix = rotation(quaternion)
            transformed = tuple(tuple(2000+dot(row, p) for row in matrix) for p in points)
            require(all(0 <= v <= 65535 for p in transformed for v in p), "rotated fixture left u16")
            check_tetra(transformed, (), counts)
            counts["rotation_cases"] += 1
    for _ in range(1400):
        points = tuple(tuple(rng.randrange(25) for _ in range(3)) for _ in range(4))
        extra = tuple(tuple(rng.randrange(25) for _ in range(3)) for _ in range(7))
        check_tetra(points, extra, counts)
    extreme = tuple(tuple(v*32767 for v in p) for p in regular)
    check_tetra(extreme, (), counts)

    # A true regular q4 centre lies at the corner of a cell and on the Jung disk.
    a, b, x, y = regular
    fr = frame(a, b)
    corners = [centre(fr, xi, eta) for xi, eta in product((F(0), F(1,4)), (F(1), F(5,4)))]
    maximum = max(form(a, b, y, t) for t in corners)
    exact_t = (F(0), F(0), F(2))
    require(maximum == 0 and form(a, b, y, exact_t) == 0,
            "nonstrict universal-credit mutant fixture changed")
    require(maximum <= 0 and not maximum < 0, "nonstrict universal-credit mutant survived")
    intervals = [(min(t[d] for t in corners), max(t[d] for t in corners)) for d in range(3)]
    lower = sum(0 if lo <= 0 <= hi else min(lo*lo, hi*hi) for lo, hi in intervals)
    require(2*lower == fr[1] and 2*dot(exact_t, exact_t) == fr[1], "nonstrict disk OUT mutant survived")

    # Corner depths do not bound the strict population inside a cell.
    a, b = (9,10,10), (11,10,10)
    fr = frame(a, b)
    witnesses = ((10,11,10), (10,9,10))
    corners = [centre(fr, xi, eta) for xi, eta in product((F(-1,4), F(1,4)), repeat=2)]
    depth = lambda t: sum(form(a,b,z,t) < 0 for z in witnesses)
    require(min(map(depth, corners)) == 1 and depth((0,0,0)) == 0,
            "corner-depth mutant / strict coincident plateau fixture changed")
    require(counts["positive_tetrahedra"] > 50, "vacant positive-tetra gate")
    print(json.dumps({"schema":"mhgp8_audit_q4_center_domain_v1", "status":"PASS",
                      "scope":"independent rational mathematical model; no product qualification",
                      **counts, "design_mutants_killed":4,
                      "mutants":["seeds_only_AABB", "universal_max_nonstrict", "disk_OUT_nonstrict", "depth_at_corners_only"],
                      "dyadic_depth_checked":10, "integer_similarity_rotations":8}, sort_keys=True))


if __name__ == "__main__":
    main()
