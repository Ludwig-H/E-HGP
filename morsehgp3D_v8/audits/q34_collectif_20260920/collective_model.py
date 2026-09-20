#!/usr/bin/env python3
"""Independent integer model of a collective q4 witness-pool certificate.

For forms (P, B), a site is strictly inside at parameter mu iff P-mu*B<0.
The public function returns the minimum population on the CLOSED chord
[-U,U]. It imports no product code and does not qualify a product engine.
Run directly (also with python -O) for the deterministic, JSON-reporting judge.
"""

from fractions import Fraction
from itertools import product
from math import isqrt
from random import Random
import json


def _require(condition, message):
    if not condition:
        raise RuntimeError(message)


def _inputs(forms, U):
    if type(U) is not int or U < 0:
        raise ValueError("U must be a nonnegative integer")
    result = []
    for form in forms:
        pair = tuple(form)
        if len(pair) != 2 or any(type(value) is not int for value in pair):
            raise ValueError("each form must contain two integers P and B")
        result.append(pair)
    return tuple(result)


def minimum_depth(forms, U):
    """Return min_{-U <= mu <= U} #{(P,B): P-mu*B < 0} exactly.

    O(m log(1+m)) rational comparisons and O(m) temporary storage for m
    forms. Integer bit complexity is additional; no bounded-word arithmetic
    claim is made. A pool minimum is a lower bound for the whole cloud, not
    its exact census. Do not clip a population that later loses exits.
    """
    forms = _inputs(forms, U)
    left, right = Fraction(-U), Fraction(U)
    groups = {}
    population = 0  # Value immediately before the left endpoint.
    for P, B in forms:
        if B == 0:
            population += P < 0
            continue
        root = Fraction(P, B)
        population += (B > 0 and root < left) or (B < 0 and root >= left)
        if left <= root <= right:
            group = groups.setdefault(root, [0, 0])
            group[0 if B > 0 else 1] += 1

    # If there are exits at the left endpoint, its depth is lower than this
    # initial value and will be considered by the first group below. Otherwise
    # this initial value IS the left endpoint's strict depth.
    result = population
    for root in sorted(groups):
        entries, exits = groups[root]
        population -= exits
        _require(population >= 0, "exit count exceeded the current population")
        result = min(result, population)  # All equal-root sites are on shell.
        population += entries
        _require(population <= len(forms), "population exceeded the pool")

    # Between consecutive roots the depth is constant and is at least the
    # depth at either adjacent group after removal of that group's exits.
    # At U the group, if present, was read BEFORE addition of its entries.
    # Thus the left endpoint and root groups also cover every interval and
    # the right endpoint; this includes the degenerate chord U=0.
    _require(0 <= result <= len(forms), "minimum is outside the population bounds")
    return int(result)


def _samples(forms, U):
    points = {Fraction(-U), Fraction(U)}
    points.update(Fraction(P, B) for P, B in forms
                  if B != 0 and -U <= Fraction(P, B) <= U)
    endpoints = sorted(points)
    return endpoints + [(a + b) / 2 for a, b in zip(endpoints, endpoints[1:])]


def _oracle(forms, U, inclusive=False):
    # No sweep, event transfer or prefix state: independent direct evaluation
    # at every possible discontinuity and in every intervening open interval.
    values = []
    for mu in _samples(forms, U):
        values.append(sum(P - mu * B <= 0 if inclusive else P - mu * B < 0
                          for P, B in forms))
    return min(values)


def _dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def _difference(a, b):
    return tuple(x - y for x, y in zip(a, b))


def _forms_from_seed(points, ids, pool):
    a, b, x = (points[i] for i in ids)
    d, u = _difference(b, a), _difference(x, a)
    D, E, F = _dot(d, d), _dot(u, u), _dot(d, u)
    G = D * E - F * F
    X = _dot(_difference(x, b), _difference(x, b))
    _require(G > 0 and min(F, D - F, E - F) > 0, "geometric seed is not acute")
    _require(D >= max(E, X), "AB is not a maximal edge of the geometric seed")
    normal = _cross(d, u)
    W = tuple(E * (D - F) * di + D * (E - F) * ui for di, ui in zip(d, u))
    J = D * (3 * G - 2 * E * X)
    _require(J > 0, "geometric seed has no positive containing chord")
    half_ceil = J // 2 + J % 2
    floor = isqrt(half_ceil)
    U = floor + (floor * floor < half_ceil)
    forms = []
    for i in pool:
        v = _difference(points[i], a)
        forms.append((G * _dot(v, v) - _dot(W, v), _dot(normal, v)))
    return forms, U, {"D": D, "E": E, "X": X, "F": F, "G": G, "J": J}


def _solve(matrix, right):
    """Small rational Gaussian solver, unrelated to product closed forms."""
    n = len(right)
    rows = [[Fraction(value) for value in row] + [Fraction(value)]
            for row, value in zip(matrix, right)]
    for col in range(n):
        pivot = next((r for r in range(col, n) if rows[r][col]), None)
        _require(pivot is not None, "singular geometric reference system")
        rows[col], rows[pivot] = rows[pivot], rows[col]
        scale = rows[col][col]
        rows[col] = [value / scale for value in rows[col]]
        for r in range(n):
            if r == col:
                continue
            multiple = rows[r][col]
            rows[r] = [a - multiple * b for a, b in zip(rows[r], rows[col])]
    return tuple(row[-1] for row in rows)


def _geometric_fixture():
    # Six actual u16 sites, no point alignment assumed by the certificate.
    # AB is the owner edge of the positive tetrahedron (0,1,2,5).
    points = [(8, 10, 10), (12, 10, 10), (10, 13, 10),
              (10, 11, 12), (10, 11, 8), (10, 11, 7)]
    seed, pool, support = (0, 1, 2), (3, 4), (0, 1, 2, 5)
    forms, U, coefficients = _forms_from_seed(points, seed, pool)
    _require(forms == [(-96, 24), (-96, -24)] and U == 28,
             "engraved geometric forms changed")
    _require(minimum_depth(forms, U) == _oracle(forms, U) == 1,
             "collective geometric certificate failed")
    individual = [minimum_depth([form], U) for form in forms]
    _require(individual == [0, 0], "an individual witness unexpectedly covers the chord")
    _require(all(P + U * abs(B) >= 0 for P, B in forms),
             "fixture does not refute the individual-universal-only approach")

    origin = points[support[0]]
    directions = [_difference(points[i], origin) for i in support[1:]]
    centre_relative = _solve([[2 * x for x in row] for row in directions],
                             [_dot(row, row) for row in directions])
    bary = _solve(list(zip(*directions)), centre_relative)
    bary = (1 - sum(bary),) + bary
    _require(all(weight > 0 for weight in bary), "tetrahedron is not strictly positive")
    owner = min(((-_dot(_difference(points[i], points[j]),
                        _difference(points[i], points[j])), (i, j))
                 for i, j in product(support, repeat=2) if i < j))[1]
    _require(owner == (0, 1), "AB is not the tetrahedron's canonical maximal edge")
    centre = tuple(a + b for a, b in zip(origin, centre_relative))
    radius_squared = _dot(centre_relative, centre_relative)
    powers = [_dot(_difference(point, centre), _difference(point, centre)) - radius_squared
              for point in points]
    depth = sum(power < 0 for power in powers)
    shell = [i for i, power in enumerate(powers) if power == 0]
    _require(depth == 1 and shell == list(support), "geometric whole-cloud reference changed")
    candidate_form, _, _ = _forms_from_seed(points, seed, [support[-1]])
    root = Fraction(*candidate_form[0])
    _require(-U <= root <= U, "positive completion escaped the containing chord")
    return {"points": points, "seed_ids": seed, "pool_ids": pool, "forms": forms,
            "U": U, "coefficients": coefficients, "minimum": 1,
            "individual_minima": individual, "positive_support": support,
            "barycentric_weights": [str(weight) for weight in bary],
            "completion_root": str(root), "global_depth": depth,
            "global_shell": shell, "Kmax": 3,
            "meaning": "q4 rejected collectively at Kmax-2=1 with zero individual universal witnesses"}


def main():
    cases = 0
    subset_checks = 0
    palette = list(product((-2, 0, 2), (-1, 0, 1)))
    for count in range(4):
        for forms in product(palette, repeat=count):
            for U in (0, 1, 3):
                _require(minimum_depth(forms, U) == _oracle(forms, U),
                         "sweep differs from exhaustive direct evaluation")
                cases += 1
    rng = Random(20260920)
    for _ in range(512):
        U = rng.randrange(9)
        forms = [(rng.randrange(-31, 32), rng.randrange(-7, 8))
                 for _ in range(rng.randrange(9))]
        answer = minimum_depth(forms, U)
        _require(answer == _oracle(forms, U), "sweep differs on the deterministic random corpus")
        extra = [(rng.randrange(-31, 32), rng.randrange(-7, 8)) for _ in range(2)]
        _require(_oracle(forms + extra, U) >= answer,
                 "a subset certificate exceeded the full population minimum")
        subset_checks += 1
        cases += 1

    large = 1 << 400
    for forms, U in [([(-large, large // 2), (-large, -large // 2)], 3),
                     ([(large + 1, large), (large - 1, large)], 2),
                     ([(-1, 0), (0, 0), (large, 0)], large)]:
        _require(minimum_depth(forms, U) == _oracle(forms, U), "big-integer case differs")
        cases += 1

    # Every named mutant must disagree with the exact model on its fixture.
    # These are mathematical-model mutants, NOT compiled product mutations.
    mutants = []
    mixed = [(0, 1), (0, -1)]
    _require(minimum_depth(mixed, 1) == 0 and _oracle(mixed, 1, inclusive=True) == 1,
             "nonstrict shell mutant survived")
    mutants.append("shell_counted_inside")
    endpoints_only = min(sum(P - mu * B < 0 for P, B in mixed) for mu in (-1, 1))
    _require(endpoints_only != minimum_depth(mixed, 1), "endpoints-only mutant survived")
    mutants.append("interior_root_group_omitted")
    for form, endpoint in [((-1, 1), -1), ((-1, -1), 1)]:
        _require(minimum_depth([form], 1) == 0 and form[0] - Fraction(endpoint, 2) * form[1] < 0,
                 "closed-endpoint mutant survived")
    mutants.append("chord_endpoints_treated_open")
    outside = [(-2, 1)]  # Entry at -2, already interior everywhere on [-1,1].
    omitted = [(P, B) for P, B in outside if B == 0 or -1 <= Fraction(P, B) <= 1]
    _require(minimum_depth(outside, 1) == 1 and minimum_depth(omitted, 1) == 0,
             "outside-root baseline transfer mutant survived")
    mutants.append("outside_root_contribution_not_transferred")
    dropping = [(0, -1), (0, -1)]
    clipped_initial, exits = 1, 2
    _require(minimum_depth(dropping, 1) == 0 and clipped_initial - exits < 0,
             "clipped decrementing population mutant survived")
    mutants.append("population_saturated_before_exits")
    _require(sum(P + B < 0 for P, B in dropping) >= 1 and minimum_depth(dropping, 1) < 1,
             "early-rejection-at-first-depth mutant survived")
    mutants.append("initial_depth_transferred_to_entire_chord")

    invalid = [([], -1), ([], True), ([(1, False)], 1), ([(1, 2, 3)], 1)]
    for forms, U in invalid:
        try:
            minimum_depth(forms, U)
        except ValueError:
            continue
        raise RuntimeError("invalid model input was accepted")

    print(json.dumps({"schema": "mhgp8_audit_collective_model_v1", "status": "passed",
                      "phase": "independent_mathematical_model", "product_qualification": False,
                      "public_status": "not_claimed", "direct_oracle_cases": cases,
                      "subset_lower_bound_checks": subset_checks,
                      "killed_model_mutants": mutants, "invalid_inputs": len(invalid),
                      "geometric_fixture": _geometric_fixture()}, sort_keys=True))


if __name__ == "__main__":
    main()
