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


def main():
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
    print(dumps({"status": "PASS", "cases": count,
                 "exact_shallow_centers": total_centers,
                 "perturbed_shallow_vertices": total_perturbed,
                 "max_oriented_groups": max_groups}, sort_keys=True))


if __name__ == "__main__":
    main()
