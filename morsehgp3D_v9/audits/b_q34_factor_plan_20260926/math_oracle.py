#!/usr/bin/env python3
"""Independent Fraction checks; no product imports, no GCP, no files written."""
from fractions import Fraction as F
from itertools import combinations, product
import hashlib
import json
from pathlib import Path
import random


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def witness(q, a, b, z, positive=True, strict=True):
    # Gram determinant, independent of the product's cross-product code.
    u, v = sub(z, a), sub(b, a)
    h = dot(u, v)-dot(u, u)
    xi = dot(u, u)*dot(v, v)-dot(u, v)**2
    value = (6-q)*h*h-xi
    return (not positive or h > 0) and (value > 0 if strict else value >= 0)


def box(points):
    return tuple((min(p[j] for p in points), max(p[j] for p in points)) for j in range(3))


def corners(b):
    return tuple(product(*b))


def universal(q, a, b, z):
    return all(witness(q, a, p, z) for p in corners(b))


def separated(a, b, s):
    gap = sum(max(0, a[j][0]-b[j][1], b[j][0]-a[j][1])**2 for j in range(3))
    diameter = max(sum((r-l)**2 for l, r in c) for c in (a, b))
    return gap >= s*s*diameter


def solve(matrix, rhs):
    a = [[F(v) for v in row]+[F(r)] for row, r in zip(matrix, rhs)]
    n = len(rhs)
    for i in range(n):
        pivot = next((j for j in range(i, n) if a[j][i]), None)
        need(pivot is not None, 'singular fixture')
        a[i], a[pivot] = a[pivot], a[i]
        d = a[i][i]
        a[i] = [x/d for x in a[i]]
        for j in range(n):
            if j != i:
                d = a[j][i]
                a[j] = [x-d*y for x, y in zip(a[j], a[i])]
    return [row[-1] for row in a]


def ball(support):
    vectors = [sub(p, support[0]) for p in support[1:]]
    weights = solve([[dot(u, v) for v in vectors] for u in vectors],
                    [F(dot(u, u), 2) for u in vectors])
    center = tuple(support[0][j]+sum(t*v[j] for t, v in zip(weights, vectors)) for j in range(3))
    return center, dot(sub(center, support[0]), sub(center, support[0])), [1-sum(weights)]+weights


def power(sphere, z):
    center, radius, _ = sphere
    return dot(sub(z, center), sub(z, center))-radius


def pool(q, a_points, b_points, threshold):
    own, other = box(a_points), box(b_points)
    direction = tuple(sum(other[j])-sum(own[j]) for j in range(3))
    proposals = sorted(range(len(a_points)), key=lambda i: (-dot(direction, a_points[i]), i))[:threshold+1]
    credits = [min(threshold, sum(j != i and universal(q, a, other, a_points[j]) for j in proposals))
               for i, a in enumerate(a_points)]
    exact = [min(threshold, sum(j != i and universal(q, a, other, z) for j, z in enumerate(a_points)))
             for i, a in enumerate(a_points)]
    return proposals, credits, exact


def main():
    counts = dict(ball_queries=0, strict_witnesses=0, owner_edges=0,
                  convex_boxes=0, convex_samples=0, class_products=0, pair_masks=0)
    fixtures = [((0, 0, 0), (4, 0, 0), (2, 3, 0)),
                ((1, 1, 1), (1, -1, -1), (-1, 1, -1), (-1, -1, 1)),
                ((0, 0, 0), (4, 0, 0), (2, 3, 0), (2, 1, 3))]
    for support in fixtures:
        sphere = ball(support)
        need(all(w > 0 for w in sphere[2]), 'nonpositive fixture')
        diameter = max(dot(sub(a, b), sub(a, b)) for a, b in combinations(support, 2))
        q = len(support)
        for a, b in combinations(support, 2):
            if dot(sub(a, b), sub(a, b)) != diameter:
                continue
            counts['owner_edges'] += 1
            for xyz in product(range(-4, 9), repeat=3):
                z = tuple(F(t, 2) for t in xyz)
                counts['ball_queries'] += 1
                if witness(q, a, b, z):
                    counts['strict_witnesses'] += 1
                    need(power(sphere, z) < 0, 'owner spindle escaped ball')
    rng = random.Random(260926)
    for q in (3, 4):
        for _ in range(100):
            a = (0, 0, 0)
            z = (rng.randint(1, 6), rng.randint(-3, 3), rng.randint(-3, 3))
            lo = (rng.randint(25, 40), rng.randint(-5, 0), rng.randint(-5, 0))
            hi = tuple(t+rng.randint(0, 5) for t in lo)
            b = tuple(zip(lo, hi))
            if universal(q, a, b, z):
                counts['convex_boxes'] += 1
                for weights in product((F(0), F(1, 3), F(1)), repeat=3):
                    p = tuple(l+t*(h-l) for (l, h), t in zip(b, weights))
                    counts['convex_samples'] += 1
                    need(witness(q, a, p, z), 'corner convexity failure')

    # Missed proposal is inefficiency, never justification for a rejection.
    a_points = [(0, 0, 0)]+[(x, 0, 0) for x in range(1, 5)]+[(x, 100, 0) for x in range(5, 10)]
    b_points = [(10000, 0, 0)]
    need(separated(box(a_points), box(b_points), 12), 'Pool fixture separation')
    pool_cases = {}
    for q in (3, 4):
        proposals, credits, exact = pool(q, a_points, b_points, 7-q)
        need(credits[0] == 0 and exact[0] == 7-q, 'Pool miss not exercised')
        need(all(c <= e for c, e in zip(credits, exact)), 'Pool overcount')
        pool_cases[str(q)] = dict(proposal_ids=proposals, anchor_pool=credits[0], anchor_exhaustive=exact[0])

    # Eight corners cannot be replaced by the center or actual sites.
    box_cases = {3: [(1000, -266, 0), (1020, -271, 0)],
                 4: [(1000, -170, 0), (1020, -173, 0)]}
    for q, sites in box_cases.items():
        a, z = (0, 0, 0), (1, 1, 0)
        b = box(sites)
        center = tuple(F(l+h, 2) for l, h in b)
        need(separated(box([a, z]), b, 12), 'box fixture separation')
        need(all(witness(q, a, p, z) for p in sites) and witness(q, a, center, z), 'center mutant inactive')
        need(not universal(q, a, b, z), 'eight-corner fixture does not refute center')

    # Independently enumerate all credit classes, then verify unique-pair mask union.
    nonnested = set()
    for k in range(3, 11):
        t3, t4 = k-1, k-2
        classes = list(product(range(t3+1), range(t4+1)))
        records = {}
        for i, a in enumerate(classes):
            for j, b in enumerate(classes):
                mask = (2 if a[0]+b[0] < t3 else 0) | (4 if a[1]+b[1] < t4 else 0)
                counts['class_products'] += 1
                if mask:
                    need((i, j) not in records, 'duplicated class product')
                    records[i, j] = mask
                if a[1] <= a[0] and b[1] <= b[0]:
                    nonnested.add(mask)
        q3 = {(i, j) for i, a in enumerate(classes) for j, b in enumerate(classes) if a[0]+b[0] < t3}
        q4 = {(i, j) for i, a in enumerate(classes) for j, b in enumerate(classes) if a[1]+b[1] < t4}
        need(set(records) == q3 | q4 and bool(q3 & q4), 'class partition coverage')
        for pair, mask in records.items():
            need(mask == (2 if pair in q3 else 0) | (4 if pair in q4 else 0), 'class partition mask')
            counts['pair_masks'] += 1
    need({2, 4, 6} <= nonnested, 'different thresholds not exercised')

    # Essential ownership and positivity premises, exact nonowner counterexamples.
    nonowner = [(((-1, 0, 0), (1, 0, 0), (0, 10, 0)), (0, F(-1, 2), 0)),
                (((-1, 0, 0), (1, 0, 0), (0, 10, 1), (0, 10, -1)), (0, F(-1, 4), 0))]
    for support, z in nonowner:
        sphere = ball(support)
        need(all(t > 0 for t in sphere[2]) and witness(len(support), *support[:2], z)
             and power(sphere, z) > 0, 'nonowner counterexample absent')
    for q in (3, 4):
        need(not witness(q, (0, 0, 0), (10, 0, 0), (11, 0, 0)) and
             witness(q, (0, 0, 0), (10, 0, 0), (11, 0, 0), positive=False), 'H-sign mutant absent')
        need(not witness(q, (0, 0, 0), (10, 0, 0), (0, 0, 0)) and
             witness(q, (0, 0, 0), (10, 0, 0), (0, 0, 0), positive=False, strict=False), 'self mutant absent')
    for q, b, z in [(3, (-3, -3, 0), (-2, -1, -1)), (4, (-3, -3, -2), (-2, -2, 0))]:
        need(not witness(q, (0, 0, 0), b, z) and
             witness(q, (0, 0, 0), b, z, strict=False), 'strict-boundary mutant absent')
    # Same witness illegally credited once in the core and once in A.
    tet = fixtures[1]
    interior = (1, 0, 0)
    need(witness(4, tet[0], tet[1], interior) and power(ball(tet), interior) < 0, 'overlap fixture')
    need(1 < 4-2 <= 1+1, 'overlap threshold fixture')
    need(counts['strict_witnesses'] > 0 and counts['convex_boxes'] > 0, 'vacuous run')
    print(json.dumps(dict(status='PASS', counts=counts, K=list(range(3, 11)), pool_misses=pool_cases,
                         counterexamples=['center_q3', 'center_q4', 'nonowner_q3', 'nonowner_q4',
                                          'H_sign_q3', 'H_sign_q4', 'self_q3', 'self_q4',
                                          'strict_boundary_q3', 'strict_boundary_q4',
                                          'overlapping_core', 'duplicate_lane_union', 'non_nested_masks'],
                         source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                         scope='independent_exact_math_only', product_imported=False, GCP_used=False), sort_keys=True))


if __name__ == '__main__':
    main()
