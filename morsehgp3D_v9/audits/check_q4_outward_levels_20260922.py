#!/usr/bin/env python3
"""Oracle rationnel des sommets peu profonds sous perturbation sortante.

Les coefficients de f_i*(u,v) = f_i(u,v) + e + e**(i+2) sont des entiers.
Le signe en e > 0 infinitesimal est celui du premier coefficient non nul.
Cet oracle verifie la couverture combinatoire locale, pas le moteur HGP.
"""

from collections import Counter
from fractions import Fraction
from itertools import combinations
from json import dumps
from math import gcd
from random import Random


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def primitive(form):
    c, x, y = form
    divisor = gcd(gcd(abs(c), abs(x)), abs(y))
    return (c // divisor, x // divisor, y // divisor)


def crossing(left, right):
    c, x, y = left
    other_c, other_x, other_y = right
    determinant = x * other_y - other_x * y
    if determinant == 0:
        return None
    return (
        Fraction(y * other_c - other_y * c, determinant),
        Fraction(other_x * c - x * other_c, determinant),
    )


def constant_polynomial(c, index):
    return {0: c, 1: 1, index + 2: 1}


def add_scaled(target, polynomial, scale):
    for degree, coefficient in polynomial.items():
        target[degree] = target.get(degree, 0) + scale * coefficient


def infinitesimal_sign(polynomial):
    for degree in sorted(polynomial):
        coefficient = polynomial[degree]
        if coefficient:
            return (coefficient > 0) - (coefficient < 0)
    return 0


def perturbed_depth(groups, weights, first, second):
    _, x_i, y_i = groups[first]
    _, x_j, y_j = groups[second]
    determinant = x_i * y_j - x_j * y_i
    require(determinant != 0, "parallel pair passed to perturbed_depth")
    constants = [constant_polynomial(line[0], index)
                 for index, line in enumerate(groups)]
    depth = 0
    for index, (_, x, y) in enumerate(groups):
        if index in (first, second):
            continue
        polynomial = {}
        add_scaled(polynomial, constants[index], determinant)
        add_scaled(polynomial, constants[first], -x * y_j + y * x_j)
        add_scaled(polynomial, constants[second], x * y_i - y * x_i)
        sign = infinitesimal_sign(polynomial) * (1 if determinant > 0 else -1)
        require(sign != 0, "triple concurrence apres perturbation sortante")
        if sign < 0:
            depth += weights[index]
    return depth


def exact_depth(groups, weights, point):
    u, v = point
    return sum(weight for (c, x, y), weight in zip(groups, weights)
               if c + x * u + y * v < 0)


def check_case(raw, limit, label):
    # Le signe d'une forme est conserve lors de la normalisation ; les
    # groupes de sens oppose restent distincts et gardent leurs poids.
    constant_negative = sum(c < 0 for c, x, y in raw if not (x or y))
    weights_by_form = Counter(primitive(form) for form in raw
                              if form[1] or form[2])
    groups = sorted(weights_by_form)
    weights = [weights_by_form[group] for group in groups]
    original = set()
    snapped = set()
    perturbed_shallow = 0
    for first, second in combinations(range(len(groups)), 2):
        point = crossing(groups[first], groups[second])
        if point is None:
            continue
        if exact_depth(groups, weights, point) + constant_negative <= limit:
            original.add(point)
        if perturbed_depth(groups, weights, first, second) + constant_negative <= limit:
            snapped.add(point)
            perturbed_shallow += 1
    require(original == snapped,
            f"{label}: sommets manquants={original - snapped}, "
            f"sommets en trop={snapped - original}; formes={raw}, K={limit}")
    return len(original), perturbed_shallow, len(groups)


def cases():
    fixed = [
        ("opposees_zero", [(0, 1, 0), (0, -1, 0),
                            (0, 0, 1), (0, 0, -1)], 0),
        ("concurrence_ponderee", [(0, 1, 0), (0, 2, 0),
                                   (0, -1, 0), (0, 0, 1),
                                   (0, 0, -1), (0, 1, 1),
                                   (0, -1, -1)], 0),
        ("paralleles_verticales", [(1, 1, 0), (0, 1, 0),
                                    (0, -1, 0), (0, 0, 1),
                                    (0, 0, -1)], 0),
        ("constantes_et_coincidences", [(-1, 0, 0), (0, 0, 0),
                                         (0, 1, 0), (0, -2, 0),
                                         (0, 0, 1), (0, 0, -2)], 1),
    ]
    yield from fixed
    random = Random(4621)
    for case in range(2000):
        raw = []
        for _ in range(random.randrange(3, 11)):
            x, y = random.randrange(-3, 4), random.randrange(-3, 4)
            if x or y:
                raw.append((random.randrange(-3, 4), x, y))
        if case % 5 == 0:
            raw.extend(((0, 1, 0), (0, -1, 0),
                        (0, 0, 1), (0, 0, -1)))
        if raw and case % 7 == 0:
            raw.append(tuple(2 * value for value in raw[0]))
        yield f"aleatoire_{case}", raw, random.randrange(0, 3)


def check_cell_owner_after_snap():
    # A q_min=4 sphere on a dyadic corner. Filtering epsilon vertices by
    # the owning cell before snapping them loses its genuine q4 centre.
    points = ((0, 6, 3), (16, 6, 3), (2, 0, 7), (7, 12, 12), (2, 5, 0))
    a, b, x, y, z = points
    center = (8, 6, 6)

    def subtract(left, right):
        return tuple(p - q for p, q in zip(left, right))

    def dot(left, right):
        return sum(p * q for p, q in zip(left, right))

    def distance_squared(left, right):
        delta = subtract(left, right)
        return dot(delta, delta)

    def determinant(left, middle, right):
        cross = (middle[1] * right[2] - middle[2] * right[1],
                 middle[2] * right[0] - middle[0] * right[2],
                 middle[0] * right[1] - middle[1] * right[0])
        return dot(left, cross)

    require(all(distance_squared(site, center) == 73 for site in points),
            "cell_owner: all five sites must contact the sphere")
    weights = (Fraction(35, 208), Fraction(77, 208),
               Fraction(3, 13), Fraction(3, 13))
    require(all(weight > 0 for weight in weights) and sum(weights) == 1 and
            all(sum(weights[j] * points[j][i] for j in range(4)) == center[i]
                for i in range(3)), "cell_owner: positive q4 support")
    require(all(distance_squared(left, right) < 256
                for left, right in combinations((a, b, x, y), 2)
                if (left, right) != (a, b)), "cell_owner: ab is longest")
    vectors = [subtract(site, center) for site in points]
    require(all(determinant(vectors[i], vectors[j], vectors[k]) != 0
                for i, j, k in combinations(range(5), 3)),
            "cell_owner: no support of arity at most three")

    midpoint_twice = tuple(a[i] + b[i] for i in range(3))
    basis_a, basis_b = (0, 16, 0), (0, 0, 16)

    def site_form(site):
        w = tuple(2 * site[i] - midpoint_twice[i] for i in range(3))
        raw = (dot(w, w) - 256, -2 * dot(w, basis_a),
               -2 * dot(w, basis_b))
        divisor = gcd(*raw)
        return tuple(coefficient // divisor for coefficient in raw)

    forms = tuple(site_form(site) for site in (x, y, z))
    require(forms == ((3, 12, -8), (9, -16, -24), (-9, 8, 24)),
            "cell_owner: primitive oriented forms")
    original = (Fraction(0), Fraction(3, 8))
    require(all(constant + slope_u * original[0] + slope_v * original[1] == 0
                for constant, slope_u, slope_v in forms),
            "cell_owner: original intersections coincide")

    normals = [form[1:] for form in forms]
    drifts = []
    for first, second in combinations(range(3), 2):
        slope_u, slope_v = normals[first]
        other_u, other_v = normals[second]
        determinant2 = slope_u * other_v - slope_v * other_u
        require(determinant2 != 0, "cell_owner: independent lines")
        drift = (Fraction(slope_v - other_v, determinant2),
                 Fraction(other_u - slope_u, determinant2))
        drifts.append(drift)
        require(all(1 + u * drift[0] + v * drift[1] > 0
                    for index, (u, v) in enumerate(normals)
                    if index not in (first, second)),
                "cell_owner: outward vertex must be shallow")
    require(drifts == [(Fraction(-1, 26), Fraction(7, 104)),
                       (Fraction(-1, 11), Fraction(-1, 88)),
                       (Fraction(1, 4), Fraction(-1, 8))],
            "cell_owner: first-order symbolic drifts")
    require(all(u < 0 or v < 0 for u, v in drifts),
            "cell_owner: pre-snap clipping must lose all three vertices")
    require(Fraction(0) <= original[0] <= Fraction(1, 32) and
            Fraction(3, 8) <= original[1] <= Fraction(13, 32),
            "cell_owner: snapped centre belongs to right/up dyadic cell")


def main():
    check_cell_owner_after_snap()
    total_centers = total_perturbed = max_groups = 0
    count = 0
    for label, raw, limit in cases():
        centers, perturbed, groups = check_case(raw, limit, label)
        if not label.startswith("aleatoire_"):
            require(centers > 0 and perturbed >= centers,
                    f"fixture non discriminante : {label}")
        total_centers += centers
        total_perturbed += perturbed
        max_groups = max(max_groups, groups)
        count += 1
    print(dumps({"status": "PASS", "cases": count, "cell_owner_fixture": 1,
                 "exact_shallow_centers": total_centers,
                 "perturbed_shallow_vertices": total_perturbed,
                 "max_oriented_groups": max_groups}, sort_keys=True))


if __name__ == "__main__":
    main()
