#!/usr/bin/env python3
"""Independent rational geometry and closed-region enumeration audit model."""
from fractions import Fraction as F
from itertools import combinations, product
from math import gcd
import json
import random


def require(ok, why):
    if not ok:
        raise RuntimeError(why)


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def difference(a, b):
    return tuple(x-y for x, y in zip(a, b))


def solve(rows, rhs):
    a = [[F(v) for v in row]+[F(y)] for row, y in zip(rows, rhs)]
    n = len(a)
    for col in range(n):
        pivot = next((i for i in range(col, n) if a[i][col]), None)
        if pivot is None:
            return None
        a[col], a[pivot] = a[pivot], a[col]
        divisor = a[col][col]
        a[col] = [v/divisor for v in a[col]]
        for i in range(n):
            if i != col:
                multiplier = a[i][col]
                a[i] = [u-multiplier*v for u, v in zip(a[i], a[col])]
    return tuple(row[-1] for row in a)


def ball(points):
    a = points[0]
    rows = [tuple(2*v for v in difference(z, a)) for z in points[1:]]
    center = solve(rows, [dot(z, z)-dot(a, a) for z in points[1:]])
    if center is None:
        return None
    radius2 = dot(difference(center, a), difference(center, a))
    columns = [difference(z, a) for z in points[1:]]
    weights = solve(list(zip(*columns)), difference(center, a))
    return center, radius2, (1-sum(weights),)+weights


def edge_forms(points):
    a, b = points[:2]
    v = difference(b, a)
    D = dot(v, v)
    k = max(range(3), key=lambda j: abs(v[j]))
    require(D > 0, "zero owner edge")
    i, j = [axis for axis in range(3) if axis != k]
    h, sign = abs(v[k]), 1 if v[k] > 0 else -1
    A, B = [0]*3, [0]*3
    A[i], A[k], B[j], B[k] = h, -sign*v[i], h, -sign*v[j]
    forms = []
    for z in points:
        w = tuple(2*z[d]-a[d]-b[d] for d in range(3))
        forms.append((dot(w, w)-D, -2*dot(w, A), -2*dot(w, B)))
    return D, tuple(A), tuple(B), forms


def value(form, point):
    c, a, b = form
    return c+a*point[0]+b*point[1]


def census(forms, point):
    values = [value(form, point) for form in forms]
    return sum(v < 0 for v in values), tuple(i for i, v in enumerate(values) if v == 0)


def intersection(x, z):
    c, a, b = x
    cz, az, bz = z
    den = a*bz-az*b
    return None if not den else (F(b*cz-bz*c, den), F(az*c-a*cz, den))


def hull(points):
    points = sorted(set(points))
    if len(points) <= 1:
        return points
    def turn(a, b, c):
        return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    chains = []
    for sequence in (points, reversed(points)):
        chain = []
        for p in sequence:
            while len(chain) >= 2 and turn(chain[-2], chain[-1], p) <= 0:
                chain.pop()
            chain.append(p)
        chains.append(chain[:-1])
    return chains[0]+chains[1]


def clip(poly, form):
    if not poly:
        return []
    out = []
    for previous, current in zip(poly[-1:]+poly[:-1], poly):
        vp, vc = value(form, previous), value(form, current)
        if (vp >= 0) != (vc >= 0):
            fraction = vp/(vp-vc)
            out.append(tuple(u+fraction*(v-u) for u, v in zip(previous, current)))
        if vc >= 0:
            out.append(current)
    return hull(out)


def rectangle(cell):
    al, ah, bl, bh = cell
    return [(F(al), F(bl)), (F(ah), F(bl)), (F(ah), F(bh)), (F(al), F(bh))]


def brute_vertices(forms, cell, k):
    al, ah, bl, bh = cell
    out = {}
    for x, z in combinations(forms, 2):
        point = intersection(x, z)
        if point is not None and al <= point[0] <= ah and bl <= point[1] <= bh:
            result = census(forms, point)
            if result[0] <= k:
                out[point] = result
    return out


def closed_regions(forms, cell, k):
    """Reference enumerator, potentially exponential in k; no fast-port claim.

    Empty polygons branch over ALL remaining groups, a valid but coarse
    infeasibility certificate. Only oriented proportional forms are grouped.
    """
    grouped = {}
    base = 0
    for form in forms:
        if form[1:] == (0, 0):
            base += form[0] < 0
            continue
        divisor = gcd(gcd(abs(form[0]), abs(form[1])), abs(form[2]))
        key = tuple(v//divisor for v in form)
        grouped[key] = grouped.get(key, 0)+1
    groups = list(grouped)
    weights = list(grouped.values())
    remaining_budget = k-base
    out, seen, stack = {}, set(), [frozenset()] if remaining_budget >= 0 else []
    while stack:
        removed = stack.pop()
        if removed in seen:
            continue
        seen.add(removed)
        cost = sum(weights[i] for i in removed)
        poly = rectangle(cell)
        remaining = [i for i in range(len(groups)) if i not in removed]
        for i in remaining:
            poly = clip(poly, groups[i])
        for point in poly:
            depth, shell = census(forms, point)
            require(depth <= k, "feasible closed region has excessive true depth")
            if any(forms[i][1]*forms[j][2] != forms[i][2]*forms[j][1]
                   for i, j in combinations(shell, 2)):
                out[point] = depth, shell
        active = remaining if not poly else [i for i in remaining if any(value(groups[i], p) == 0 for p in poly)]
        for i in active:
            if cost+weights[i] <= remaining_budget:
                stack.append(removed | {i})
    return out, len(seen)


def main():
    points = [(30, 30, 30), (36, 36, 30), (30, 36, 24), (36, 30, 24),
              (30, 36, 30), (36, 30, 30), (30, 30, 24), (32, 32, 32)]
    D, A, B, forms = edge_forms(points)
    require((D, A, B) == (72, (-6, 6, 0), (0, 0, 6)), "physical basis fixture changed")
    require(forms[4:] == [(0, -144, 0), (0, 144, 0), (144, 0, 144), (-48, 0, -48)], "opposite forms fixture changed")
    center, radius2, weights = ball(points[:4])
    require((center, radius2, weights) == ((F(33), F(33), F(27)), F(27), (F(1, 4),)*4), "positive rational tetra oracle")
    require(all(dot(difference(points[i], points[j]), difference(points[i], points[j])) == D
                for i, j in combinations(range(4), 2)), "regular tetra owner tie")
    physical_balls, presentations = set(), 0
    for i, j in combinations(range(2, len(points)), 2):
        ids = (0, 1, i, j)
        answer = ball([points[t] for t in ids])
        if answer is None or min(answer[2]) <= 0:
            continue
        owner = min(combinations(ids, 2), key=lambda pair: (-dot(difference(points[pair[0]], points[pair[1]]), difference(points[pair[0]], points[pair[1]])), pair))
        if owner != (0, 1):
            continue
        c, r2, _ = answer
        powers = [dot(difference(z, c), difference(z, c))-r2 for z in points]
        if sum(v < 0 for v in powers) == 0:  # K3, T=1.
            physical_balls.add((c, r2))
            presentations += 1
            require(all(v == 0 for v in powers), "physical shell lost an ID")
    require(physical_balls == {(center, radius2)} and presentations > 0, "positive owned q4 root missing")
    root, cell = (F(0), F(-1)), (-2, 2, -2, 2)
    poly = rectangle(cell)
    for form in forms:
        poly = clip(poly, form)
    require(poly == [root], "zero-depth feasible set is not the isolated center")
    require(census(forms, root) == (0, tuple(range(8))), "true isolated strict shell")
    require(not (len(poly) >= 3), "open-face-only mutant did not drop the valid point")
    depth_probes = 0
    for xi, eta in product((F(-1), F(-1, 3), F(0), F(2, 5), F(1)), repeat=2):
        p = xi, eta-1
        c3 = tuple(F(points[0][d]+points[1][d], 2)+(A[d]*p[0]+B[d]*p[1])/2 for d in range(3))
        r2 = dot(difference(points[0], c3), difference(points[0], c3))
        for form, z in zip(forms, points):
            require(value(form, p) == 4*(dot(difference(z, c3), difference(z, c3))-r2), "affine power identity")
        require(p == root or census(forms, p)[0] >= 1, "noncentral zero-depth point")
        depth_probes += 1
    weighted = [(0, 1, 0)]*3+[(0, -1, 0)]*3+[(-1, 0, 1)]*3+[(1, 0, -1)]*3
    actual, _ = closed_regions(weighted, cell, 2)
    require(actual == {(F(0), F(1)): (0, tuple(range(12)))}, "weighted opposite-group isolation")
    require(census(weighted, (F(1), F(1)))[0] == 3 and census(list(dict.fromkeys(weighted)), (F(1), F(1)))[0] == 1,
            "unit-weight dedup mutant did not undercount")
    require(census(weighted, (F(1), F(1)))[0] != 0, "opposite-sign cancellation mutant survived")
    rng = random.Random(20260923)
    cases, nodes, roots = 0, 0, 0
    examples = [(forms, cell, 0), (weighted, cell, 2),
                ([(0, 1, 0), (0, -1, 0), (0, 0, 1), (1, 0, -1)], cell, 0),
                ([(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, 0, 1)], cell, 1)]
    tangent_forms = [(i*i, -2*i, 1) for i in range(1, 9)]
    tangent_cell = (0, 9, -1, 81)
    _, tangent_states = closed_regions(tangent_forms, tangent_cell, 2)
    require(tangent_states == 1+8+28, "all-subsets branching fixture changed")
    examples.append((tangent_forms, tangent_cell, 2))
    for _ in range(72):
        fixture = [tuple(rng.randrange(-5, 6) for _ in range(3)) for _ in range(7)]
        if rng.randrange(3) == 0:
            fixture += [fixture[0], tuple(-v for v in fixture[0])]
        examples.append((fixture, (-3, 3, -3, 3), rng.randrange(3)))
    for fixture, domain, k in examples:
        got, n = closed_regions(fixture, domain, k)
        expected = brute_vertices(fixture, domain, k)
        require(got == expected, "closed-region enumeration differs from all-pairs rational oracle")
        cases += 1; nodes += n; roots += len(expected)
    print(json.dumps({"schema": "mhgp8_audit_q4_degeneracies_v1", "status": "PASS",
                      "scope": "independent rational models; one real u16 positive isolated-root fixture; no product qualification",
                      "physical_sites": len(points), "physical_owned_presentations": presentations,
                      "physical_balls": len(physical_balls), "physical_shell_ids": 8,
                      "power_probe_centers": depth_probes, "closed_region_cases": cases,
                      "closed_region_states": nodes, "shallow_vertices_checked": roots,
                      "all_subsets_fixture_states": tangent_states,
                      "mutants": ["discard_dimensions_0_and_1", "unit_weight_dedup", "cancel_opposite_signs"]}, sort_keys=True))


if __name__ == "__main__":
    main()
