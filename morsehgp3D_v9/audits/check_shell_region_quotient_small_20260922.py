#!/usr/bin/env python3
"""Exact small-shell comparison: strict-mask DSU versus oriented regions.

No product implementation is imported. The primal test enumerates affinely
independent barycentric supports; the dual test uses integer Fourier-Motzkin
elimination for strict sign feasibility. Deliberately bounded to u <= 8.
"""

import json
from fractions import Fraction
from itertools import combinations, product
from math import gcd


def need(ok, cause):
    if not ok:
        raise RuntimeError(cause)


def zero_in_conv(points):
    """Exact Caratheodory test, including boundary supports and degeneracy."""
    for q in range(1, min(4, len(points)) + 1):
        for subset in combinations(points, q):
            rows = [
                [Fraction(p[axis]) if axis < 3 else Fraction(1) for p in subset]
                + [Fraction(axis == 3)]
                for axis in range(4)
            ]
            rank = 0
            for column in range(q):
                pivot = next((r for r in range(rank, 4) if rows[r][column]), None)
                if pivot is None:
                    break
                rows[rank], rows[pivot] = rows[pivot], rows[rank]
                scale = rows[rank][column]
                rows[rank] = [value / scale for value in rows[rank]]
                for r in range(4):
                    if r != rank:
                        scale = rows[r][column]
                        rows[r] = [a - scale * b for a, b in zip(rows[r], rows[rank])]
                rank += 1
            if rank != q:
                continue  # A smaller independent support is enumerated.
            if any(not any(row[:q]) and row[-1] for row in rows):
                continue
            if all(rows[r][-1] >= 0 for r in range(q)):
                return True
    return False


def strict_sign_feasible(signed_points):
    """Find n with n.dot(point)>0, using exact integer elimination.

    Finite strict inequalities are homogeneous, so after rescaling n they
    are equivalent to n.dot(point)>=1 for every signed point.
    """
    rows = {(*point, 1) for point in signed_points}
    for _ in range(3):
        positive = [row for row in rows if row[0] > 0]
        negative = [row for row in rows if row[0] < 0]
        next_rows = {row[1:] for row in rows if row[0] == 0}
        for p in positive:
            for n in negative:
                next_rows.add(
                    tuple((-n[0]) * p[j] + p[0] * n[j] for j in range(1, len(p)))
                )
        rows = set()
        for row in next_rows:
            scale = gcd(*row)
            reduced = tuple(value // scale for value in row) if scale else row
            if not any(reduced[:-1]):
                if reduced[-1] > 0:
                    return False
            else:
                rows.add(reduced)
    return True


def primitive_oriented(point):
    scale = gcd(*point)
    vector = tuple(value // scale for value in point)
    first = next(value for value in vector if value)
    if first < 0:
        return tuple(-value for value in vector), -1
    return vector, 1


def dsu(items):
    parent = {item: item for item in items}

    def root(item):
        while parent[item] != item:
            parent[item] = parent[parent[item]]
            item = parent[item]
        return item

    def union(a, b):
        a, b = root(a), root(b)
        if a != b:
            parent[max(a, b)] = min(a, b)

    return root, union


def partition(items, root):
    groups = {}
    for item in items:
        groups.setdefault(root(item), []).append(item)
    return sorted(tuple(sorted(group)) for group in groups.values())


def verify(name, points):
    u = len(points)
    need(2 <= u <= 8 and len(set(points)) == u, name + ".domain")
    norms = {sum(value * value for value in point) for point in points}
    need(len(norms) == 1 and 0 not in norms, name + ".same_sphere")
    closed = {
        mask
        for mask in range(1, 1 << u)
        if zero_in_conv([point for i, point in enumerate(points) if mask & (1 << i)])
    }
    need((1 << u) - 1 in closed, name + ".positive_BallKey")
    qmin = min(mask.bit_count() for mask in closed)
    h = max(mask.bit_count() for mask in range(1 << u) if mask not in closed)

    circles = {}
    for i, point in enumerate(points):
        key, orientation = primitive_oriented(point)
        circles.setdefault(key, []).append((i, orientation))
    circle_groups = list(circles.values())
    need(all(len(group) <= 2 for group in circle_groups), name + ".distinct_positions")
    regions = {}
    for choices in product((-1, 1), repeat=len(circle_groups)):
        signs = [0] * u
        for group, choice in zip(circle_groups, choices):
            for i, orientation in group:
                signs[i] = choice * orientation
        signed = [tuple(sign * value for value in point) for sign, point in zip(signs, points)]
        if strict_sign_feasible(signed):
            regions[choices] = sum(1 << i for i, sign in enumerate(signs) if sign > 0)
    need(regions and h == max(mask.bit_count() for mask in regions.values()), name + ".h")

    ranks = {}
    for t in range(1, u + 1):
        strict = [mask for mask in range(1 << u) if mask.bit_count() == t and mask not in closed]
        mask_root, mask_union = dsu(strict)
        for coface in range(1 << u):
            if coface.bit_count() != t + 1 or coface in closed:
                continue
            faces = [coface ^ (1 << i) for i in range(u) if coface & (1 << i)]
            for face in faces[1:]:
                mask_union(faces[0], face)

        active = {sign for sign, mask in regions.items() if mask.bit_count() >= t}
        region_root, region_union = dsu(active)
        for sign in active:
            for j in range(len(circle_groups)):
                neighbor = tuple(-value if k == j else value for k, value in enumerate(sign))
                if neighbor in active and (regions[sign] & regions[neighbor]).bit_count() >= t:
                    region_union(sign, neighbor)

        assignment = {}
        for mask in strict:
            containing = [sign for sign in active if regions[sign] & mask == mask]
            need(containing, name + ".strict_mask_without_region")
            roots = {region_root(sign) for sign in containing}
            need(len(roots) == 1, name + ".strict_mask_split")
            assignment[mask] = next(iter(roots))
        primal = partition(strict, mask_root)
        dual = partition(strict, lambda mask: assignment[mask])
        need(primal == dual, name + ".partition")
        cover_primal = {i for mask in strict for i in range(u) if mask & (1 << i)}
        cover_dual = {i for sign in active for i in range(u) if regions[sign] & (1 << i)}
        need(cover_primal == cover_dual, name + ".coverage")
        if u >= 2 * t - 1:
            need(cover_dual == set(range(u)), name + ".large_shell_cover")
        region_representatives = {}
        for sign in active:
            positive_ids = [i for i in range(u) if regions[sign] & (1 << i)]
            candidate = sum(1 << i for i in positive_ids[:t])
            root = region_root(sign)
            region_representatives[root] = min(
                candidate, region_representatives.get(root, candidate)
            )
        need(sorted(region_representatives.values()) == sorted(min(group) for group in primal),
             name + ".representatives")
        ranks[str(t)] = {"strict": len(strict), "components": len(primal)}
    return {"u": u, "regions": len(regions), "q_min": qmin, "h": h, "ranks": ranks}


def main():
    need(strict_sign_feasible([(1, 0, 0), (0, 1, 0)]), "dual.feasible")
    need(not strict_sign_feasible([(1, 0, 0), (-1, 0, 0)]), "dual.infeasible")
    fixtures = {
        "antipodal_pair": [(5, 0, 0), (-5, 0, 0)],
        "octahedron": [
            tuple(sign * 5 if axis == coordinate else 0 for coordinate in range(3))
            for axis in range(3) for sign in (-1, 1)
        ],
        "tetrahedron": [(1, 1, 1), (1, -1, -1), (-1, 1, -1), (-1, -1, 1)],
        "square_arcs": [(1313, 0, 0), (-1313, 0, 0), (0, 1313, 0),
                        (0, -1313, 0), (1287, 260, 0), (1212, 505, 0)],
        "cube": list(product((-1, 1), repeat=3)),
        "qmin3_5": [(5, 0, 0), (-3, 4, 0), (-3, -4, 0), (0, 0, 5), (0, 4, 3)],
        "many_positive_8": [(5, 0, 0), (4, 3, 0), (4, -3, 0), (0, 3, 4),
                            (0, -3, 4), (0, 0, 5), (-3, 4, 0), (-3, -4, 0)],
    }
    result = {name: verify(name, points) for name, points in fixtures.items()}
    need(result["antipodal_pair"]["ranks"]["1"]["components"] == 2,
         "nonvacuity.antipodal_split")
    need(result["square_arcs"]["ranks"]["2"]["components"] == 2,
         "nonvacuity.isolated_pair")
    need(result["qmin3_5"]["q_min"] == 3 and result["tetrahedron"]["q_min"] == 4,
         "nonvacuity.qmin_3_4")
    need(result["many_positive_8"]["h"] >= 5 and result["many_positive_8"]["q_min"] == 3,
         "nonvacuity.large_strict_subset")
    print(json.dumps({"status": "passed", "scope": "small_exact_region_quotient",
                      "fixtures": result}, sort_keys=True))


if __name__ == "__main__":
    main()
