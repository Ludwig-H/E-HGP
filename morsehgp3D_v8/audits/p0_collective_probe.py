#!/usr/bin/env python3
"""Exact bounded model of collective witnesses; no producer or timing claim."""
from __future__ import annotations

import argparse
from fractions import Fraction as F
import hashlib
import itertools
import json
from pathlib import Path


def require(condition: bool, cause: str) -> None:
    if not condition:
        raise RuntimeError(cause)


def dot(a: tuple, b: tuple) -> F:
    return sum((F(x) * F(y) for x, y in zip(a, b)), F(0))


def subtract(a: tuple, b: tuple) -> tuple:
    return tuple(F(x) - F(y) for x, y in zip(a, b))


def power(point: tuple, center: tuple, radius2: F) -> F:
    delta = subtract(point, center)
    return dot(delta, delta) - radius2


def certificate(a: tuple, b: tuple, sites: list, weights: list,
                lam: F, *, allow_equal: bool = False,
                ignore_mean: bool = False) -> tuple[bool, F]:
    require(len(sites) == len(weights) and bool(sites), "empty/mismatched group")
    require(all(weight >= 0 for weight in weights) and sum(weights) == 1,
            "invalid convex weights")
    require(0 < lam < 1, "interior chord parameter required")
    mean = tuple(sum((weight * F(z[axis]) for weight, z in zip(weights, sites)), F(0))
                 for axis in range(3))
    chord = tuple((1 - lam) * F(a[axis]) + lam * F(b[axis]) for axis in range(3))
    gap = sum((weight * dot(z, z) for weight, z in zip(weights, sites)), F(0))
    gap -= (1 - lam) * dot(a, a) + lam * dot(b, b)
    return ((ignore_mean or mean == chord) and
            (gap <= 0 if allow_equal else gap < 0)), gap


def row_ball(distance: int, i: int, j: int, cy: F, cz: F) -> tuple:
    cx = F(distance * distance + j * j - i * i - 2 * cy * (j - i), 2 * distance)
    center = (cx, cy, cz)
    radius2 = dot(subtract((0, i, 0), center), subtract((0, i, 0), center))
    require(power((distance, j, 0), center, radius2) == 0, "endpoint not on sphere")
    return center, radius2


def cross(a: tuple, b: tuple) -> tuple:
    return (F(a[1]) * b[2] - F(a[2]) * b[1],
            F(a[2]) * b[0] - F(a[0]) * b[2],
            F(a[0]) * b[1] - F(a[1]) * b[0])


def group_moments(sites: list, integer_weights: list | None = None) -> tuple:
    """Proof weights, not weighted HGP input; preprocessing visits each site once.

    Return W, X=sum(k*z), S=sum(k*|z|^2). Python integers are unbounded;
    this model makes no claim about a fixed-width C++ moment implementation.
    Uniform moments use k=1. A zero weight is allowed but contributes no credit.
    """
    weights = [1] * len(sites) if integer_weights is None else integer_weights
    require(bool(sites) and len(sites) == len(weights), "moment.group_shape")
    require(all(isinstance(k, int) and k >= 0 for k in weights) and sum(weights) > 0,
            "moment.integer_weights")
    require(all(len(z) == 3 and all(isinstance(v, int) and 0 <= v <= 65535 for v in z)
                for z in sites), "moment.u16_sites")
    mass = sum(weights)
    first = tuple(sum(k * z[j] for k, z in zip(weights, sites)) for j in range(3))
    second = sum(k * sum(v * v for v in z) for k, z in zip(weights, sites))
    return mass, first, second


def moment_metrics(a: tuple, b: tuple, moments: tuple) -> tuple:
    """Aggregate before squaring; endpoints may be rational box samples."""
    mass, first, second = moments
    h = dot(first, tuple(F(a[j]) + F(b[j]) for j in range(3)))
    h -= mass * dot(a, b) + second
    xb, xa, ab = cross(first, b), cross(first, a), cross(a, b)
    vector = tuple(xb[j] - xa[j] - mass * ab[j] for j in range(3))
    return h, vector, dot(vector, vector)


def fixed_moment_soc(q: int, a: tuple, b: tuple, moments: tuple) -> bool:
    require(q in (2, 3, 4), "moment.lane")
    h, _, xi = moment_metrics(a, b, moments)
    return h > 0 and (q == 2 or (3 if q == 3 else 2) * h * h > xi)


def box_corners(box: tuple) -> list:
    low, high = box
    require(len(low) == len(high) == 3 and all(low[j] <= high[j] for j in range(3)),
            "moment.box")
    return list(itertools.product(*(sorted({low[j], high[j]}) for j in range(3))))


def fixed_box_soc(q: int, a_box: tuple, b_box: tuple, moments: tuple) -> tuple:
    # The exact same moments are tested at every corner. No per-corner optimizer.
    answers = [fixed_moment_soc(q, a, b, moments)
               for a in box_corners(a_box) for b in box_corners(b_box)]
    return all(answers), len(answers)


def check_positive_q4(support: list, center: tuple, radius2: F,
                      bary: list, cause: str) -> None:
    a, b, c, d = support
    require(all(w > 0 for w in bary) and sum(bary) == 1, cause + ".weights")
    require(all(power(z, center, radius2) == 0 for z in support), cause + ".sphere")
    require(all(sum(w * z[j] for w, z in zip(bary, support)) == center[j]
                for j in range(3)), cause + ".center")
    require(dot(subtract(b, a), cross(subtract(c, a), subtract(d, a))) != 0,
            cause + ".rank")
    longest = dot(subtract(a, b), subtract(a, b))
    require(all(dot(subtract(support[i], support[j]), subtract(support[i], support[j]))
                < longest for i in range(4) for j in range(i + 1, 4)
                if (i, j) != (0, 1)), cause + ".unique_longest_edge")


def moment_checks() -> dict:
    metrics_checked = corner_checks = interior_checks = power_checks = 0
    # This genuine q4 support is the existing fixture translated upward in z.
    a, b = (0, 4, 10), (8, 4, 10)
    support = [a, b, (4, 7, 15), (4, 1, 15)]
    center, radius2 = (F(4), F(4), F(59, 5)), F(481, 25)
    check_positive_q4(support, center, radius2,
                     [F(8, 25), F(8, 25), F(9, 50), F(9, 50)], "moment.q4")

    # Nonuniform proof weights, off-chord mean: aggregate C cancels before
    # squaring, even though neither individual W3/W4 is true.
    sites, weights = [(4, 7, 10), (4, 1, 10)], [2, 1]
    moments = group_moments(sites, weights)
    mass, first, _ = moments
    h, vector, xi = moment_metrics(a, b, moments)
    require(h / mass == 7 and xi / (mass * mass) == 64, "moment.weighted_metrics")
    require(tuple(F(x, mass) for x in first) == (4, 5, 10), "moment.off_chord_mean")
    for q in (3, 4):
        require(fixed_moment_soc(q, a, b, moments), "moment.weighted_positive")
        require(fixed_moment_soc(q, a, b, group_moments(sites, [14, 7])),
                "moment.weight_scale_invariance")
        require(not any(fixed_moment_soc(q, a, b, group_moments([z])) for z in sites),
                "moment.individual_failure")
        metrics_checked += 1
    require(sum(F(k, mass) * power(z, center, radius2) for k, z in zip(weights, sites)) < 0,
            "moment.weighted_sphere")
    power_checks += len(sites)

    # A uniform group strictly extends the all-spheres chord certificate:
    # its convex hull stays at z=11, while the chord is at z=10.
    sites = [(4, 7, 11), (4, 1, 11)]
    moments = group_moments(sites)
    mass, first, _ = moments
    h, vector, xi = moment_metrics(a, b, moments)
    require(h / mass == 6 and xi / (mass * mass) == 64, "moment.uniform_metrics")
    require(fixed_moment_soc(4, a, b, moments), "moment.uniform_positive")
    individual_xi = sum(moment_metrics(a, b, group_moments([z]))[2] for z in sites) / 2
    require(individual_xi == 640 and 2 * (h / mass) ** 2 <= individual_xi,
            "moment.cancellation_gain")
    require(all(power(z, center, radius2) == F(-48, 5) for z in sites),
            "moment.uniform_q4_power")
    # This larger sphere passes through a,b but violates the q4 radius bound.
    # Both members are outside; no all-spheres claim is made by the SOC rule.
    large_center, large_radius2 = (F(4), F(4), F(6)), F(32)
    require(power(a, large_center, large_radius2) == power(b, large_center, large_radius2) == 0,
            "moment.large_sphere_endpoints")
    require(all(power(z, large_center, large_radius2) == 2 for z in sites),
            "moment.not_all_spheres")
    power_checks += 2 * len(sites)

    # A nontrivial product of endpoint boxes passes with fixed moments.
    # Its separate min(H) / max(|C|) shortcut is strictly weaker than testing
    # the coupled condition at every corner.
    a_box = ((4, 10, 10), (5, 10, 10))
    b_box = ((13, 10, 10), (14, 10, 10))
    boxed_sites = [(9, 13, 11), (9, 7, 11)]
    boxed_moments = group_moments(boxed_sites)
    answers = [moment_metrics(aa, bb, boxed_moments)
               for aa in box_corners(a_box) for bb in box_corners(b_box)]
    require(2 * min(answer[0] for answer in answers) ** 2 <=
            max(answer[2] for answer in answers), "moment.split_extrema_loss")
    for q in (2, 3, 4):
        ok, count = fixed_box_soc(q, a_box, b_box, boxed_moments)
        require(ok, "moment.fixed_corner_positive")
        corner_checks += count
        for ax in (F(4), F(17, 4), F(9, 2), F(19, 4), F(5)):
            for bx in (F(13), F(53, 4), F(27, 2), F(55, 4), F(14)):
                aa, bb = (ax, 10, 10), (bx, 10, 10)
                require(fixed_moment_soc(q, aa, bb, boxed_moments),
                        "moment.rational_box_sample")
                # Direct weighted per-site H/C verifies the compressed moments.
                direct = [moment_metrics(aa, bb, group_moments([z])) for z in boxed_sites]
                hh, cc, xx = moment_metrics(aa, bb, boxed_moments)
                require(hh == sum(value[0] for value in direct) and
                        all(cc[j] == sum(value[1][j] for value in direct) for j in range(3)) and
                        xx == dot(cc, cc), "moment.direct_aggregation")
                interior_checks += 1

    # Bad rule 1: replacing E[|z|^2] by |E[z]|^2 drops the variance.
    # Translate the symmetric y=+-5 configuration to keep every coordinate u16.
    aa, bb = (0, 10, 10), (8, 10, 10)
    sites = [(4, 15, 10), (4, 5, 10)]
    variance_center = (F(4), F(10), F(59, 5))
    moments = group_moments(sites)
    mass, first, second = moments
    mean = tuple(F(x, mass) for x in first)
    h, _, _ = moment_metrics(aa, bb, moments)
    require(h / mass == -9 and not fixed_moment_soc(4, aa, bb, moments),
            "moment.variance_nominal")
    wrong_moments = (mass, first, mass * dot(mean, mean))
    require(moment_metrics(aa, bb, wrong_moments)[0] / mass == 16 and
            fixed_moment_soc(4, aa, bb, wrong_moments), "moment.variance_mutant")
    require(all(power(z, variance_center, radius2) == 9 for z in sites),
            "moment.variance_geometry")
    power_checks += len(sites)

    # Bad rule 2: E[H^2] in place of E[H]^2. The same true positive q4 support
    # excludes BOTH group sites, although that enlarged left-hand side passes.
    sites, weights = [(4, 4, 7), (4, 4, 0)], [13, 1]
    moments = group_moments(sites, weights)
    mass, _, _ = moments
    h, _, xi = moment_metrics(a, b, moments)
    average_square = sum(F(k, mass) * moment_metrics(a, b, group_moments([z]))[0] ** 2
                         for k, z in zip(weights, sites))
    require(h / mass == F(1, 2) and xi / (mass * mass) == 784 and
            not fixed_moment_soc(4, a, b, moments), "moment.square_nominal")
    require(2 * average_square == 1099 and 2 * average_square > xi / (mass * mass),
            "moment.square_mutant")
    require([power(z, center, radius2) for z in sites] == [F(19, 5), F(120)],
            "moment.square_geometry")
    power_checks += len(sites)

    # Bad rule 3: change the group's proof weights independently at each corner.
    # Every individual corner then passes, while all weights fail at the middle.
    low_a, high_a, bb = (1000, 468, 0), (1000, 532, 0), (0, 500, 0)
    sites = [(999, 468, 0), (999, 532, 0)]
    require(1000 * 1000 >= 12 * 12 * 64 * 64, "moment.variable_separation")
    one_hot = ([1, 0], [0, 1])
    for endpoint, weights in zip((low_a, high_a), one_hot):
        moments = group_moments(sites, weights)
        h, _, xi = moment_metrics(endpoint, bb, moments)
        require(h == 999 and xi == 1024 and fixed_moment_soc(4, endpoint, bb, moments),
                "moment.variable_corner_positive")
        corner_checks += 1
    uniform = group_moments(sites)
    require(not fixed_box_soc(4, (low_a, high_a), (bb, bb), uniform)[0],
            "moment.fixed_weights_reject")
    middle = (1000, 500, 0)
    for z in sites:
        require(moment_metrics(middle, bb, group_moments([z]))[0] == -25,
                "moment.variable_middle_h")
        require(moment_metrics(low_a, bb, group_moments([z]))[0] +
                moment_metrics(high_a, bb, group_moments([z]))[0] == -50,
                "moment.no_fixed_weights_on_corners")
    second_center, second_radius2 = (F(500), F(500), F(801, 802)), F(250000) + F(801, 802) ** 2
    beta = F(801, 2 * 802 * 401)
    check_positive_q4([middle, bb, (500, 800, 401), (500, 200, 401)],
                     second_center, second_radius2,
                     [(1 - 2 * beta) / 2, (1 - 2 * beta) / 2, beta, beta],
                     "moment.variable_q4")
    require(all(power(z, second_center, second_radius2) == 25 for z in sites),
            "moment.variable_actual_q4_counterexample")
    power_checks += len(sites)

    rejected_weights = 0
    for bad in ([0, 0], [-1, 2], [F(1, 2), F(1, 2)]):
        try:
            group_moments(sites, bad)
        except RuntimeError as error:
            require(str(error) == "moment.integer_weights", "moment.wrong_rejection")
            rejected_weights += 1
        else:
            raise RuntimeError("moment.bad_weights_survived")
    require(corner_checks == 14 and interior_checks == 75 and rejected_weights == 3,
            "moment.nonvacuity")
    return {"moment_weighted_lane_checks": metrics_checked,
            "moment_corner_checks": corner_checks,
            "moment_rational_box_samples": interior_checks,
            "moment_power_checks": power_checks,
            "moment_positive_q4_supports": 2,
            "moment_weight_rejections": rejected_weights,
            "moment_model_mutants": ["missing_variance", "mean_square_h", "per_corner_reweighting"],
            "moment_scope": "fixed integer proof weights; aggregate H and C before squaring; no product port"}


def run() -> dict:
    groups = spheres = inside = boundary_members = 0
    distance = 59000
    # u16 fixture, separation s12, exhaustive triples of ranks on small rows.
    for m in (4, 7, 11):
        require(distance >= 12 * (m - 1), "separation")
        for i in range(m):
            for j in range(i + 2, m):
                for k in range(i + 1, j):
                    s, t = k - i, j - k
                    a, b = (0, i, 0), (distance, j, 0)
                    sites = [(0, k, 0), (distance, k, 0)]
                    weights = [F(t, s + t), F(s, s + t)]
                    ok, gap = certificate(a, b, sites, weights, F(s, s + t))
                    require(ok and gap == -s * t, "collective row identity")
                    groups += 1
                    seen_outside = [False, False]
                    for cy in (F(-m), F(i + k, 2), F(i + j, 2), F(j + k, 2), F(2 * m)):
                        for cz in (F(0), F(1, 3)):
                            center, radius2 = row_ball(distance, i, j, cy, cz)
                            values = [power(z, center, radius2) for z in sites]
                            require(sum(w * p for w, p in zip(weights, values)) == gap,
                                    "independent sphere power disagrees")
                            require(min(values) < 0, "group misses sphere")
                            spheres += 1
                            inside += sum(value < 0 for value in values)
                            boundary_members += sum(value == 0 for value in values)
                            seen_outside = [old or value > 0 for old, value in zip(seen_outside, values)]
                    require(all(seen_outside), "individual nonuniversality not exercised")

    # Noncoplanar positive tetrahedral support; one group member is defining.
    a, b, c, d = (0, 10, 10), (10, 10, 10), (5, 16, 10), (5, 10, 16)
    center = (F(5), F(131, 12), F(131, 12))
    radius2 = dot(subtract(a, center), subtract(a, center))
    support = [a, b, c, d]
    bary = [F(25, 72), F(25, 72), F(11, 72), F(11, 72)]
    require(all(power(z, center, radius2) == 0 for z in support), "q4 support boundary")
    require(all(w > 0 for w in bary) and sum(bary) == 1, "q4 support positivity")
    require(all(sum(w * z[axis] for w, z in zip(bary, support)) == center[axis]
                for axis in range(3)), "q4 center not convex barycenter")
    # det(b-a,c-a,d-a)=10*6*6, hence affine rank three.
    require((b[0] - a[0]) * (c[1] - a[1]) * (d[2] - a[2]) == 360, "q4 rank")
    sites = [c, (5, 9, 11), (5, 9, 9)]
    weights = [F(1, 7), F(3, 7), F(3, 7)]
    ok, gap = certificate(a, b, sites, weights, F(1, 2))
    require(ok and gap == F(-127, 7), "q4 group relation")
    values = [power(z, center, radius2) for z in sites]
    require(values[0] == 0 and values[1] < 0 and values[2] < 0, "q4 defining member exclusion")
    require(sum(w * p for w, p in zip(weights, values)) == gap, "q4 identity")

    # A second positive tetrahedron has a unique longest edge a-b and a
    # collective pair whose two sites BOTH fail the individual W3/W4 tests.
    a, b, c, d = (0, 4, 4), (8, 4, 4), (4, 7, 9), (4, 1, 9)
    center, radius2 = (F(4), F(4), F(29, 5)), F(481, 25)
    support, bary = [a, b, c, d], [F(8, 25), F(8, 25), F(9, 50), F(9, 50)]
    require(all(w > 0 for w in bary) and sum(bary) == 1, "second q4 support positivity")
    require(all(power(z, center, radius2) == 0 for z in support), "second q4 sphere")
    require(all(sum(w * z[axis] for w, z in zip(bary, support)) == center[axis]
                for axis in range(3)), "second q4 positive barycenter")
    require((c[1] - a[1]) * (d[2] - a[2]) - (c[2] - a[2]) * (d[1] - a[1]) == 30,
            "second q4 rank")
    require(dot(subtract(a, b), subtract(a, b)) == 64 and
            all(dot(subtract(support[i], support[j]), subtract(support[i], support[j])) < 64
                for i in range(4) for j in range(i + 1, 4) if (i, j) != (0, 1)),
            "second q4 owner longest edge")
    sites, weights = [(4, 7, 4), (4, 1, 4)], [F(1, 2), F(1, 2)]
    ok, gap = certificate(a, b, sites, weights, F(1, 2))
    require(ok and gap == -7, "second q4 collective certificate")
    require(all(power(z, center, radius2) < 0 for z in sites), "second q4 depth")
    for z in sites:
        u, v = subtract(z, a), subtract(b, z)
        h = dot(u, v)
        xi = dot(u, u) * dot(v, v) - h * h
        require(h == 7 and xi == 576 and 3 * h * h < xi and 2 * h * h < xi,
                "individual W3/W4 obstruction not exercised")

    # Three explicit erroneous rules, each accepted by the mutant and refuted
    # by sphere geometry; these are model mutants, not product source mutants.
    a, b = (0, 1, 0), (2, 1, 0)
    sites, weights = [(1, 2, 0), (1, 0, 0)], [F(1, 2), F(1, 2)]
    require(not certificate(a, b, sites, weights, F(1, 2))[0], "equality strict gate")
    require(certificate(a, b, sites, weights, F(1, 2), allow_equal=True)[0], "weak mutant inactive")
    require(all(power(z, (1, 1, 0), F(1)) == 0 for z in sites), "weak mutant counterexample")

    a, b, sites, weights = (0, 0, 0), (10, 0, 0), [(5, 1, 0)], [F(1)]
    require(not certificate(a, b, sites, weights, F(1, 2))[0], "mean identity gate")
    require(certificate(a, b, sites, weights, F(1, 2), ignore_mean=True)[0], "mean mutant inactive")
    require(power(sites[0], (5, -100, 0), F(10025)) > 0, "mean mutant counterexample")

    a, b = (0, 10, 10), (10, 10, 10)
    u, v, w = (5, 11, 10), (5, 9, 10), (5, 12, 10)
    require(certificate(a, b, [u, v], [F(1, 2), F(1, 2)], F(1, 2))[0],
            "first overlapping group")
    require(certificate(a, b, [v, w], [F(2, 3), F(1, 3)], F(1, 2))[0],
            "second distinct overlapping group")
    sites = [u, v, w]  # Even the deduplicated union has only one interior.
    center, radius2 = (5, -90, 10), F(10025)
    exact_interior = sum(power(z, center, radius2) < 0 for z in sites)
    require(exact_interior == 1 and 2 > exact_interior, "double group credit counterexample")

    require(groups >= 200 and spheres >= 2000 and boundary_members >= 800,
            "nonvacuity floor")
    return {"status": "passed_bounded_collective_model", "rank_groups": groups,
            "sphere_checks": spheres, "strict_inside_members": inside,
            "boundary_members": boundary_members, "positive_noncoplanar_q4": 2,
            "individual_w3_w4_failures_in_positive_q4": 2,
            "refuted_model_mutants": ["closed_margin", "missing_barycenter", "overlapping_groups"],
            "full_qualification": False, "performance_claim": False, "gcp_used": False,
            "source_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            **moment_checks()}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", action="store_true", required=True)
    parser.parse_args()
    print(json.dumps(run(), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
