#!/usr/bin/env python3
"""Independent exact oracle for one owner-positive q4 support on a shell.

This checks the projective dominance and longest-edge reduction only.  It is
not a centre enumerator, global shell census, product implementation, or
performance qualification.  No Python ``assert`` is used: -O runs the gates.
"""

from fractions import Fraction
from itertools import combinations, product
from random import Random
import json


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def det(a, b, c):
    return dot(a, cross(b, c))


def squared(a, b):
    return dot(sub(a, b), sub(a, b))


def barycentric_positive(a, b, x, y, center):
    """Independent affine 3x3 solve; None means degenerate tetrahedron."""
    matrix = [[Fraction(a[i] - y[i]), Fraction(b[i] - y[i]),
               Fraction(x[i] - y[i]), Fraction(center[i] - y[i])]
              for i in range(3)]
    for column in range(3):
        pivot = next((row for row in range(column, 3)
                      if matrix[row][column] != 0), None)
        if pivot is None:
            return False
        matrix[column], matrix[pivot] = matrix[pivot], matrix[column]
        scale = matrix[column][column]
        matrix[column] = [value / scale for value in matrix[column]]
        for row in range(3):
            if row != column:
                scale = matrix[row][column]
                matrix[row] = [value - scale * source
                               for value, source in
                               zip(matrix[row], matrix[column])]
    weights = [matrix[row][3] for row in range(3)]
    weights.append(1 - sum(weights))
    return all(weight > 0 for weight in weights)


def projective(z, a, b, center):
    a0, b0, z0 = (sub(point, center) for point in (a, b, z))
    normal = cross(a0, b0)
    depth = det(a0, b0, z0)
    if depth == 0:
        return depth, None, None
    return (depth, Fraction(det(z0, b0, normal), depth),
            Fraction(det(a0, z0, normal), depth))


def dominance(a, b, x, y, center, counts):
    """Compare the exact projective predicate to oriented determinant signs."""
    if cross(sub(a, center), sub(b, center)) == (0, 0, 0):
        return False
    dx, px, qx = projective(x, a, b, center)
    dy, py, qy = projective(y, a, b, center)
    if dx == 0 or dy == 0 or (dx > 0) == (dy > 0):
        return False
    if dx < 0:
        x, y = y, x
        dx, dy, px, py, qx, qy = dy, dx, py, px, qy, qx
    aa, bb, xx, yy = (sub(point, center) for point in (a, b, x, y))
    normal_norm2 = dot(cross(aa, bb), cross(aa, bb))
    lhs_p = det(yy, bb, xx)
    lhs_q = det(aa, yy, xx)
    require(lhs_p == dx * Fraction(dy, normal_norm2) * (py - px),
            "P determinant/projective identity")
    require(lhs_q == dx * Fraction(dy, normal_norm2) * (qy - qx),
            "Q determinant/projective identity")
    expected = px < py and qx < qy
    require(expected == (lhs_p < 0 and lhs_q < 0),
            "strict determinant sign/dominance mismatch")
    require(expected == barycentric_positive(a, b, x, y, center),
            "strict dominance/barycentric mismatch")
    counts["cross_side_pairs"] += 1
    counts["positive_pairs"] += int(expected)
    counts["coordinate_equalities"] += int(px == py or qx == qy)
    return expected


def edge_allowed(points, ids, i, j):
    owner_length2 = squared(points[0], points[1])
    length2 = squared(points[i], points[j])
    owner_key = tuple(sorted((ids[0], ids[1])))
    return (length2 < owner_length2 or
            (length2 == owner_length2 and
             tuple(sorted((ids[i], ids[j]))) >= owner_key))


def owner_all_edges(points, ids, x, y):
    return all(edge_allowed(points, ids, i, j)
               for i, j in combinations((0, 1, x, y), 2))


def reduced_exists(points, ids, center):
    """Brute range subsets but follow the proposed two support-query decisions."""
    a, b = points[:2]
    if cross(sub(a, center), sub(b, center)) == (0, 0, 0):
        return False
    m = min(ids[:2])
    owner_length2 = squared(a, b)
    projected = [projective(z, a, b, center) for z in points]
    plus = [i for i in range(2, len(points)) if projected[i][0] > 0]
    minus = [i for i in range(2, len(points)) if projected[i][0] < 0]
    for x in plus:
        if not edge_allowed(points, ids, 0, x) or not edge_allowed(points, ids, 1, x):
            continue
        px, qx = projected[x][1:]
        eligible = [y for y in minus
                    if edge_allowed(points, ids, 0, y) and
                    edge_allowed(points, ids, 1, y) and
                    projected[y][1] > px and projected[y][2] > qx]
        if not eligible:
            continue
        # On a common sphere, this minimum is exactly a 3D support maximum
        # in direction x-center.  Choose the lowest ID among ties deliberately.
        best = min(eligible, key=lambda y: (squared(points[x], points[y]), ids[y]))
        best_length2 = squared(points[x], points[best])
        if best_length2 < owner_length2:
            return True
        if best_length2 > owner_length2 or ids[x] <= m:
            continue
        restricted = [y for y in eligible if ids[y] > m]
        if restricted and min(squared(points[x], points[y]) for y in restricted) <= owner_length2:
            return True
    return False


def brute_exists(points, ids, center):
    return any(barycentric_positive(points[0], points[1], points[x], points[y], center)
               and owner_all_edges(points, ids, x, y)
               for x, y in combinations(range(2, len(points)), 2))


def check_fixture(points, ids, center, expected_positive, expected_owner, counts):
    require(len(points) == 4 and len(set(points)) == 4 and len(set(ids)) == 4,
            "fixture is not four distinct sites/IDs")
    radii = {squared(point, center) for point in points}
    require(len(radii) == 1, "fixture is not a common sphere")
    positive = barycentric_positive(*points, center)
    owned = positive and owner_all_edges(points, ids, 2, 3)
    require(positive == expected_positive and owned == expected_owner,
            "fixture positivity or ownership changed")
    require(reduced_exists(points, ids, center) == owned,
            "fixture reduction disagrees with independent affine solve")
    dominance(points[0], points[1], points[2], points[3], center, counts)
    counts["fixtures"] += 1


def check_u18_bounds(rng, counts):
    max_coord = (1 << 18) - 1
    max_delta_bits = max_numerator_bits = max_determinant_bits = 0
    for trial in range(320):
        if trial == 0:
            points = ((max_coord, max_coord, max_coord),
                      (0, max_coord, max_coord),
                      (max_coord, 0, max_coord),
                      (max_coord, max_coord, 0))
        else:
            points = tuple(tuple(rng.randrange(max_coord + 1) for _ in range(3))
                           for _ in range(4))
        base = points[0]
        rows = tuple(tuple(2 * (point[i] - base[i]) for i in range(3))
                     for point in points[1:])
        rhs = tuple(dot(point, point) - dot(base, base)
                    for point in points[1:])
        columns = tuple(tuple(row[j] for row in rows) for j in range(3))
        delta = det(*columns)
        if delta == 0:
            continue
        numerators = [det(*(rhs if j == i else columns[j] for j in range(3)))
                      for i in range(3)]
        if delta < 0:
            delta = -delta
            numerators = [-number for number in numerators]
        require(0 < delta < (1 << 60), "u18 centre denominator bound")
        require(all(abs(number) < (1 << 79) for number in numerators),
                "u18 centre numerator bound")
        center = tuple(Fraction(number, delta) for number in numerators)
        require(all(2 * dot(sub(point, base), center) ==
                    dot(point, point) - dot(base, base)
                    for point in points[1:]), "Cramer centre equation")
        homogeneous = [tuple(delta * point[i] - numerators[i]
                             for i in range(3)) for point in points]
        require(all(abs(value) < (1 << 80)
                    for vector in homogeneous for value in vector),
                "u18 homogeneous vector bound")
        raw_orientation = det(*(sub(point, base) for point in points[1:]))
        require(abs(raw_orientation) < (1 << 57), "u18 raw orientation bound")
        projective_det = det(homogeneous[3], homogeneous[1], homogeneous[2])
        exact_det = det(sub(points[3], center), sub(points[1], center),
                        sub(points[2], center))
        require(projective_det == delta ** 3 * exact_det,
                "positive homogeneous scale/orientation mismatch")
        require(abs(projective_det) < (1 << 243), "u18 i256 determinant bound")
        require(abs(dot(homogeneous[2], points[3])) < (1 << 100),
                "u18 i128 hull support dot bound")
        require(all(squared(p, q) < (1 << 38)
                    for p, q in combinations(points, 2)),
                "u18 i64 squared distance bound")
        max_delta_bits = max(max_delta_bits, delta.bit_length())
        max_numerator_bits = max(max_numerator_bits,
                                 *(abs(number).bit_length() for number in numerators))
        max_determinant_bits = max(max_determinant_bits, abs(projective_det).bit_length())
        counts["u18_tetrahedra"] += 1
    counts.update({"max_delta_bits": max_delta_bits,
                   "max_numerator_bits": max_numerator_bits,
                   "max_projective_det_bits": max_determinant_bits})


def main():
    rng = Random(20260923)
    counts = {"fixtures": 0, "random_clouds": 0, "cross_side_pairs": 0,
              "positive_pairs": 0, "non_cross_pairs": 0,
              "coordinate_equalities": 0,
              "u18_tetrahedra": 0}
    # Positive support whose xy edge outranks ab despite all four anchored
    # edges being shorter: an endpoint-only test is insufficient.
    long_xy = ((-5, 0, 0), (3, -4, 0), (0, 0, -5), (0, 3, 4))
    require(all(squared(long_xy[i], long_xy[j]) < squared(*long_xy[:2])
                for i, j in ((0, 2), (0, 3), (1, 2), (1, 3))) and
            squared(long_xy[2], long_xy[3]) > squared(*long_xy[:2]),
            "long-xy fixture must isolate the missing completion edge")
    check_fixture(long_xy, (10, 20, 30, 40), (0, 0, 0), True, False, counts)
    # Both ab and xy are equal longest edges; only original IDs break the tie.
    equal_edges = ((4, 4, 3), (-4, -4, 3), (4, -4, -3), (-4, 4, -3))
    require(all(squared(equal_edges[i], equal_edges[j]) < squared(*equal_edges[:2])
                for i, j in ((0, 2), (0, 3), (1, 2), (1, 3))) and
            squared(equal_edges[2], equal_edges[3]) == squared(*equal_edges[:2]),
            "equal-longest fixture must isolate lexicographic ownership")
    check_fixture(equal_edges, (10, 20, 30, 40), (0, 0, 0), True, True, counts)
    check_fixture(equal_edges, (10, 20, 5, 40), (0, 0, 0), True, False, counts)
    # Opposite sides alone are insufficient; equality in either projective
    # coordinate is a boundary case, never a strict positive tetrahedron.
    check_fixture(((4, 3, 0), (-4, 3, 0), (0, 0, 5), (0, 0, -5)),
                  (10, 20, 30, 40), (0, 0, 0), False, False, counts)
    # Antipodal a,b put the centre on their edge; no strict tetra is possible.
    check_fixture(((5, 0, 0), (-5, 0, 0), (0, 5, 0), (0, 0, 5)),
                  (10, 20, 30, 40), (0, 0, 0), False, False, counts)

    shell = [point for point in product(range(-5, 6), repeat=3)
             if dot(point, point) == 25]
    require(len(shell) == 30, "integer radius-five shell changed")
    for _ in range(320):
        offset = tuple(rng.randrange(-100, 101) for _ in range(3))
        picked = rng.sample(shell, rng.randrange(8, 15))
        points = tuple(tuple(point[i] + offset[i] for i in range(3))
                       for point in picked)
        ids = tuple(rng.sample(range(1, 1_000_000), len(points)))
        if cross(sub(points[0], offset), sub(points[1], offset)) == (0, 0, 0):
            continue
        for x, y in combinations(range(2, len(points)), 2):
            dx = projective(points[x], points[0], points[1], offset)[0]
            dy = projective(points[y], points[0], points[1], offset)[0]
            if dx == 0 or dy == 0 or (dx > 0) == (dy > 0):
                require(not barycentric_positive(points[0], points[1],
                                                  points[x], points[y], offset),
                        "same-side/planar pair unexpectedly positive")
                counts["non_cross_pairs"] += 1
            dominance(points[0], points[1], points[x], points[y], offset, counts)
        require(reduced_exists(points, ids, offset) == brute_exists(points, ids, offset),
                "random shell: two-support-query reduction disagrees with brute owner")
        counts["random_clouds"] += 1
    require(counts["random_clouds"] >= 300 and counts["cross_side_pairs"] >= 1000
            and counts["positive_pairs"] > 0 and counts["coordinate_equalities"] > 0,
            "random corpus did not exercise positive and equal-coordinate cases")
    check_u18_bounds(rng, counts)
    print(json.dumps({"status": "PASS", **counts}, sort_keys=True))


if __name__ == "__main__":
    main()
