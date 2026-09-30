#!/usr/bin/env python3
"""Exact, private group-energy certificate oracle; see PROTOCOL.txt."""
from fractions import Fraction as F
from itertools import product
import argparse
import json
import sys


CHECKS = 0


def require(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(message)


def point(*xyz):
    return tuple(F(x) for x in xyz)


def norm2(v):
    return sum(x * x for x in v)


def dot(v, w):
    return sum(x * y for x, y in zip(v, w))


def sub(v, w):
    return tuple(x - y for x, y in zip(v, w))


def power(z, a, centre):
    # Direct geometric oracle, not the affine/moment implementation.
    return norm2(sub(z, centre)) - norm2(sub(a, centre))


def corners(box):
    return tuple(product(*(tuple(axis) for axis in box)))


def census(points, weights, a, centre):
    values = tuple(power(z, a, centre) for z in points)
    interior = tuple(i for i, value in enumerate(values) if value < 0)
    shell = tuple(i for i, value in enumerate(values) if value == 0)
    p = sum(weights[i] for i in interior)
    return p, interior, shell


def moments(points, weights, ids):
    require(len(ids) == len(set(ids)), "group repeats an actual site ID")
    require(all(0 <= i < len(points) for i in ids), "invalid group ID")
    require(all(isinstance(weights[i], int) and weights[i] > 0 for i in ids),
            "weights must be actual positive integer multiplicities")
    W = sum(weights[i] for i in ids)
    M = tuple(sum(weights[i] * points[i][k] for i in ids) for k in range(3))
    V = sum(weights[i] * norm2(points[i]) for i in ids)
    return W, M, V


def affine_sum(mom, anchor, centre):
    W, M, V = mom
    return V - W * norm2(anchor) - 2 * dot(sub(M, tuple(W * x for x in anchor)), centre)


def ceil_fraction(x):
    return -((-x.numerator) // x.denominator)


def certificate(points, weights, ids, anchor, box):
    mom = moments(points, weights, ids)
    cs = corners(box)
    sigma = max(affine_sum(mom, anchor, c) for c in cs)
    radius2 = max(norm2(sub(anchor, c)) for c in cs)
    credit = ceil_fraction(-sigma / radius2) if sigma < 0 and radius2 > 0 else 0
    return {"sigma": sigma, "R2": radius2, "credit": credit, "moments": mom}


def rejects(cert, theta, equality_mutant=False):
    if cert["R2"] <= 0:
        return False
    value = cert["sigma"] + theta * cert["R2"]
    return value <= 0 if equality_mutant else value < 0


def barycentre(points, indices, bary):
    require(all(x > 0 for x in bary), "support barycentric weight not positive")
    require(sum(bary) == 1, "support barycentric weights do not sum to1")
    return tuple(sum(t * points[i][k] for i, t in zip(indices, bary)) for k in range(3))


def determinant3(u, v, w):
    return (u[0] * (v[1] * w[2] - v[2] * w[1])
            - u[1] * (v[0] * w[2] - v[2] * w[0])
            + u[2] * (v[0] * w[1] - v[1] * w[0]))


def check_support(points, weights, ids, centre, bary, expected_p):
    require(barycentre(points, ids, bary) == centre, "wrong positive-support centre")
    if len(ids) == 4:
        a, b, d, e = (points[i] for i in ids)
        require(determinant3(sub(b, a), sub(d, a), sub(e, a)) != 0,
                "tetrahedron is degenerate")
    if len(ids) == 3:
        a, b, d = (points[i] for i in ids)
        u, v = sub(b, a), sub(d, a)
        require(norm2(u) * norm2(v) - dot(u, v) ** 2 > 0,
                "triangle is degenerate")
    anchor = points[ids[0]]
    require(all(power(points[i], anchor, centre) == 0 for i in ids),
            "claimed support is not on the sphere")
    p, interior, shell = census(points, weights, anchor, centre)
    require(p == expected_p, "wrong exact interior census")
    require(shell == tuple(ids), "unexpected contact / incomplete expected shell")
    return {"arity": len(ids), "centre": centre,
            "radius2": norm2(sub(anchor, centre)), "bary": bary,
            "interior_mass": p, "interior_ids": interior, "shell_ids": shell}


def line_oracle(points, weights, ids, anchor, box):
    cx, cy = F(12), F(6)
    lo, hi = box[2]
    roots = {lo, hi}
    for i in ids:
        v0 = power(points[i], anchor, point(cx, cy, 0))
        v1 = power(points[i], anchor, point(cx, cy, 1))
        slope = v1 - v0
        if slope != 0:
            root = -v0 / slope
            if lo <= root <= hi:
                roots.add(root)
    ordered = sorted(roots)
    samples = ordered + [(x + y) / 2 for x, y in zip(ordered, ordered[1:])]
    masses = []
    for zc in samples:
        c = point(cx, cy, zc)
        masses.append(sum(weights[i] for i in ids if power(points[i], anchor, c) < 0))
    return min(masses), len(samples), ordered


def check_main_fixture(name, e_z, z_bounds):
    points = [point(0, 1, 9), point(24, 1, 9), point(12, 19, 9), point(12, 6, e_z)]
    points += [point(12, 6 + y, 9 + 9 * s)
               for y in (-6, -3, 0, 3, 6) for s in (-1, 1)]
    weights = [1] * len(points)
    group = tuple(range(4, 14))
    require(len(points) == 14 and len(set(points)) == 14, "not fourteen distinct sites")
    require(all(0 <= x <= 262143 and x.denominator == 1 for p in points for x in p),
            "fixture is not raw u18 integer geometry")
    delta = F(1, 64)
    box = ((F(12) - delta, F(12) + delta),
           (F(6) - delta, F(6) + delta), tuple(F(z) for z in z_bounds))
    cs = corners(box)
    support_indices = (0, 1, 2, 3)
    dominance = []
    for i in support_indices:
        dom = tuple(j for j, z in enumerate(points)
                    if max(power(z, points[i], c) for c in cs) < 0)
        dominance.append(dom)
    require(dominance[:3] == [(), (), ()], "old dominance already rejects the q3 supports")
    if name == "q4_wide":
        require(dominance[3] == (), "old dominance already rejects the q4 fourth support")
    else:
        require(dominance[3] == (5, 7, 9, 11, 13),
                "lost the initial fixture's known q4 old-filter limitation")
    axis_samples = [tuple(lo + (hi - lo) * F(i, 4) for i in range(5))
                    for lo, hi in box]
    samples = set(product(*axis_samples))
    samples.add(tuple(lo + (hi - lo) * t
                      for (lo, hi), t in zip(box, (F(2, 3), F(2, 5), F(1, 7)))))
    samples.add(point(12, 6, 9))
    certs = []
    for i in (0, 1, 2):
        anchor = points[i]
        cert = certificate(points, weights, group, anchor, box)
        require(cert["credit"] == (4 if name == "q3_narrow" else 3), "unexpected group credit")
        require(cert["sigma"] == (-F(11115, 16) if i < 2 else -F(11135, 16)),
                "wrong affine maximum")
        expected_R = F((420929 if i < 2 else 420673)
                       if name == "q3_narrow" else (642113 if i < 2 else 641857), 2048)
        require(cert["R2"] == expected_R, "wrong radius bound")
        require(rejects(cert, 2), "K5 q4 threshold not exceeded")
        require(rejects(cert, 3) == (name == "q3_narrow"), "wrong K5 q3 threshold decision")
        require(not rejects(cert, 4), "K5 q2 / conservative weighted threshold was confused with q3/q4")
        require(all(max(power(points[j], anchor, c) for c in cs) >= 0 for j in group),
                "one group member is already an individual universal witness")
        for c in cs + tuple(samples):
            direct = sum(weights[j] * power(points[j], anchor, c) for j in group)
            require(direct == affine_sum(cert["moments"], anchor, c), "moment identity mismatch")
            require(direct <= cert["sigma"], "affine corner maximum lost an interior centre")
            r2 = norm2(sub(anchor, c))
            require(r2 <= cert["R2"], "convex radius corner maximum failed")
            mass = sum(weights[j] for j in group if power(points[j], anchor, c) < 0)
            require(all(power(points[j], anchor, c) >= -cert["R2"] for j in group),
                    "per-site lower power bound failed")
            require(direct >= -cert["R2"] * mass, "energy-to-mass inequality failed")
            require(mass >= cert["credit"], "unsafe group credit")
            global_p, _, _ = census(points, weights, anchor, c)
            require(global_p >= mass, "group is not a subset of the actual cloud")
        certs.append({"anchor_id": i, "sigma": cert["sigma"], "R2": cert["R2"],
                      "credit": cert["credit"], "reject_K5_q3": rejects(cert, 3),
                      "reject_K5_q4": rejects(cert, 2)})
    c3 = point(12, 6, 9)
    q3 = check_support(points, weights, (0, 1, 2), c3,
                       (F(13, 36), F(13, 36), F(5, 18)), 10)
    if name == "q3_narrow":
        c4 = point(12, 6, F(591, 40))
        bary4 = (F(7397, 28800), F(7397, 28800), F(569, 2880), F(231, 800))
        p4 = 5
    else:
        c4 = point(12, 6, F(279, 28))
        bary4 = (F(4745, 14112), F(4745, 14112), F(1825, 7056), F(27, 392))
        p4 = 10
    require(all(lo <= x <= hi for x, (lo, hi) in zip(c4, box)), "q4 centre not in Q")
    q4 = check_support(points, weights, (0, 1, 2, 3), c4, bary4, p4)
    minimum, events, roots = line_oracle(points, weights, group, points[0], box)
    require(minimum == 5, "wrong exact line census minimum")
    return {"name": name, "points": points, "group_ids": group, "box": box,
            "certificates": certs, "individual_dom_ids_for_supports": dominance,
            "q3": q3, "q4": q4, "line_exact_minimum": minimum,
            "line_probes": events, "line_event_roots": roots,
            "fractional_centres_tested_per_anchor": len(samples) + len(cs)}


def check_equality(mutant):
    # A real admitted K3 q2 sphere; actual multiplicity2 is interior, shells weight1.
    points = [point(2, 0, 0), point(0, 0, 0), point(1, 0, 0)]
    weights = [1, 1, 2]
    centre = point(1, 0, 0)
    box = tuple((x, x) for x in centre)
    cert = certificate(points, weights, (2,), points[0], box)
    p, interior, shell = census(points, weights, points[0], centre)
    require(p == 2 and interior == (2,) and shell == (0, 1), "wrong equality census")
    require(p + 2 <= 3 + 1, "equality fixture is not admitted K3 q2")
    require(cert["sigma"] + 2 * cert["R2"] == 0, "not an exact rejection equality")
    require(not rejects(cert, 2, mutant), "equality mutant rejects an admitted K3 q2")
    contact_cert = certificate(points, weights, (0, 1), points[0], box)
    require(contact_cert["sigma"] == 0 and contact_cert["credit"] == 0,
            "contact got strict-interior credit")
    require(not rejects(contact_cert, 0), "exact contact equality was discarded")
    return {"theta": 2, "sigma": cert["sigma"], "R2": cert["R2"],
            "actual_interior_mass": p, "admitted_K": 3, "arity": 2,
            "retained": True, "contact_sigma": contact_cert["sigma"]}


def check_overlap(mutant):
    points = [point(0, 1, 9), point(24, 1, 9), point(12, 19, 9),
              point(12, 6, 9), point(12, 7, 9), point(13, 6, 9)]
    weights = [1] * len(points)
    centre = point(12, 6, 9)
    box = tuple((x, x) for x in centre)
    q3 = check_support(points, weights, (0, 1, 2), centre,
                       (F(13, 36), F(13, 36), F(5, 18)), 3)
    require(q3["interior_mass"] + 3 <= 5 + 1, "overlap fixture is not admitted K5 q3")
    g1, g2 = (3, 4), (3, 5)
    c1 = certificate(points, weights, g1, points[0], box)
    c2 = certificate(points, weights, g2, points[0], box)
    union = certificate(points, weights, (3, 4, 5), points[0], box)
    require(c1["credit"] == 2 and c2["credit"] == 2 and union["credit"] == 3,
            "wrong overlap credits")
    valid_credit = c1["credit"] + c2["credit"] if mutant else max(c1["credit"], c2["credit"])
    require(valid_credit <= q3["interior_mass"], "overlap mutant double-counts a real interior")
    require(valid_credit <= 3, "overlap mutant rejects an admitted K5 q3")
    require(not rejects(union, 3), "safe ID-union moments reject the admitted sphere")
    disjoint = ((3, 4), (5,))
    require(set(disjoint[0]).isdisjoint(disjoint[1]), "groups declared disjoint overlap")
    disjoint_credit = sum(certificate(points, weights, ids, points[0], box)["credit"]
                          for ids in disjoint)
    require(disjoint_credit == 3 and disjoint_credit <= q3["interior_mass"],
            "disjoint-credit addition failed")
    try:
        moments(points, weights, (3, 3))
    except ValueError as exc:
        require(str(exc) == "group repeats an actual site ID", "wrong duplicate-ID refusal")
    else:
        require(False, "repeated actual site ID was accepted")
    return {"overlap_groups": (g1, g2), "credits": (c1["credit"], c2["credit"]),
            "unsafe_sum": 4, "actual_mass": 3, "safe_max": 2,
            "union_credit": 3, "disjoint_groups": disjoint,
            "disjoint_credit_sum": disjoint_credit, "admitted_K": 5, "arity": 3}


def enc(value):
    if isinstance(value, F):
        return str(value)
    if isinstance(value, dict):
        return {k: enc(v) for k, v in value.items()}
    if isinstance(value, (list, tuple)):
        return [enc(v) for v in value]
    return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mutant-equality", action="store_true")
    parser.add_argument("--mutant-overlap", action="store_true")
    args = parser.parse_args()
    require(not (args.mutant_equality and args.mutant_overlap), "choose one logical mutant")
    receipts = [check_main_fixture("q3_narrow", 29, (3, 15)),
                check_main_fixture("q4_wide", 23, (3, 21))]
    equality = check_equality(args.mutant_equality)
    overlap = check_overlap(args.mutant_overlap)
    print(json.dumps(enc({"status": "PASS", "checks": CHECKS,
                          "fixtures": receipts, "equality": equality, "overlap": overlap,
                          "scope": "Fraction certificate only; no native, timings, FULL or GPU"}),
                     sort_keys=True, indent=2))


if __name__ == "__main__":
    try:
        main()
    except (ValueError, TypeError, ZeroDivisionError) as exc:
        print(json.dumps({"status": "FAIL", "reason": str(exc), "checks_before_failure": CHECKS},
                         sort_keys=True), file=sys.stderr)
        sys.exit(1)
