#!/usr/bin/env python3
"""Q1--Q3: bounded independent Gram/Gamma guards, no product imports.

The copied oracle is our already closed auditor Gram/Fraction kernel. Its optional
product-reference main is never called. Radicals below are compared by exact
rational intervals, with exact equality first (square classes over Q).
These guards qualify no native execution and establish no statistical optimum.
"""
from fractions import Fraction as Q
from hashlib import sha256
from math import isqrt
from pathlib import Path
import json
import sys

HERE = Path(__file__).resolve().parent
PINS = json.loads((HERE / "SOURCE_PINS.json").read_text())
for name, digest in PINS["oracle"].items():
    if sha256((HERE / "oracle" / name).read_bytes()).hexdigest() != digest:
        raise RuntimeError("oracle source changed")
sys.dont_write_bytecode = True
sys.path.insert(0, str(HERE / "oracle"))
from test_qualified import Oracle, NONE  # noqa: E402

CHECKS = 0


def need(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise RuntimeError(message)


def rational_root(a):
    n, d = isqrt(a.numerator), isqrt(a.denominator)
    return Q(n, d) if n*n == a.numerator and d*d == a.denominator else None


def reduced(terms):
    groups = {}
    for factor, radicand in sorted(terms, key=lambda t: t[1]):
        if not radicand or not factor:
            continue
        for representative in groups:
            ratio = rational_root(radicand / representative)
            if ratio is not None:
                groups[representative] += factor*ratio
                break
        else:
            groups[radicand] = factor
    return [(c, a) for a, c in groups.items() if c]


def sign(terms):
    terms = reduced(terms)
    if not terms:
        return 0
    for digits in (12, 24, 48, 96):
        scale = 10**digits
        lo = hi = Q(0)
        for coefficient, a in terms:
            lower_integer = isqrt(a.numerator*scale*scale // a.denominator)
            lower = Q(lower_integer, scale)
            upper = lower if lower*lower == a else lower + Q(1, scale)
            left, right = coefficient*lower, coefficient*upper
            lo += min(left, right)
            hi += max(left, right)
        if lo > 0:
            return 1
        if hi < 0:
            return -1
    raise RuntimeError("explicit unresolved radical comparison")


def sub(a, b):
    return list(a) + [(-c, r) for c, r in b]


def root(a):
    return [(Q(1), Q(a))]


def encode(terms):
    return [[str(c), str(a)] for c, a in reduced(terms)]


def projection(points, k, m):
    oracle = Oracle(points, k)
    tree = oracle.trees[k]
    nodes = tree["nodes"]

    def meet(v, w):
        ancestors = set()
        while v != NONE:
            ancestors.add(v)
            v = nodes[v]["parent"]
        while w not in ancestors:
            w = nodes[w]["parent"]
        return oracle.levels[nodes[w]["rank"]]

    profiles = [[] for _ in points]
    seen = [set() for _ in points]
    for beta in oracle.levels:
        groups = {}
        for part, node in tree["snapshots"][beta].items():
            groups.setdefault(node, set()).update(part)
        for node, sites in groups.items():
            if len(sites) < m:
                continue
            for i in sites:
                if node not in seen[i]:
                    profiles[i].append((beta, node, tuple(sorted(sites))))
                    seen[i].add(node)
    dates = []
    rows = []
    for profile in profiles:
        need(bool(profile), "nonempty qualification")
        alpha, first, _ = profile[0]
        candidates = [root(alpha)]
        described = []
        for beta, node, sites in profile:
            mu = meet(first, node)
            candidates.append(root(alpha) + root(mu) + [(-Q(1), beta)])
            described.append(dict(beta=str(beta), meeting_beta=str(mu), sites=sites))
        winner = candidates[0]
        for candidate in candidates[1:]:
            if sign(sub(candidate, winner)) > 0:
                winner = candidate
        dates.append(reduced(winner))
        rows.append(described)
    return oracle, profiles, dates, rows


def main():
    # No insertion/deletion stability at fixed k, even when Hausdorff -> 0.
    old = [(0, 0, 0), (2, 0, 0), (4, 0, 0)]
    _, _, old_dates, _ = projection(old, 2, 3)
    for date in old_dates:
        need(sign(sub(date, root(4))) == 0, "old entry radius 2")
    insertions = []
    for eta in (Q(1, 2), Q(1, 1000)):
        new = old + [(eta, 0, 0)]
        oracle, _, dates, rows = projection(new, 2, 3)
        need(sign(sub(dates[0], root(1))) == 0, "inserted near 0 gives entry 1")
        need(rows[0][0]["beta"] == "1", "first qualified triple exactly at beta 1")
        need(eta < 1, "Hausdorff bound smaller than entry difference")
        insertions.append(dict(eta=str(eta), hausdorff=str(eta), old_entry="2",
                               new_entry="1", difference="1"))

    # Geometrically restricted image n=k+1: one qualified branch, constant 1.
    small = [([(0, 0, 0), (2, 0, 0), (4, 0, 0)], 2),
             ([(1, 1, 2), (1, 2, 1), (2, 2, 2)], 2),
             ([(0, 0, 0), (2, 0, 0), (0, 2, 0), (0, 0, 2)], 3)]
    domain = []
    for points, k in small:
        oracle, profiles, dates, rows = projection(points, k, k+1)
        beta = oracle.meb(tuple(range(len(points))))[0]
        for profile, date in zip(profiles, dates):
            need(profile[0][0] == beta, "n=k+1 first qualified at global MEB")
            need(sign(sub(date, root(beta))) == 0, "n=k+1 no delay")
            need(len(profile) == 1, "n=k+1 single qualified branch")
        domain.append(dict(k=k, n=len(points), common_entry_beta=str(beta)))

    # H5 counterexample: a genuinely large component, not a tiny isolated group.
    points = [(0, 0, 0), (10, 0, 0), (20, 0, 0), (30, 0, 0),
              (44, 0, 0), (54, 0, 0), (-20, 10, 0), (-20, -10, 0)]
    oracle, profiles, dates, rows = projection(points, 2, 3)
    expected = root(250) + [(-Q(5, 2), Q(1))]
    need(profiles[0][0][0] == 100, "qualified first radius 10")
    need(profiles[0][0][2] == (0, 1, 2, 3), "first qualified component has four sites")
    need(sign(sub(dates[0], expected)) == 0, "exact qualified entry sqrt250-5/2")
    need(sign(sub(dates[0], root(144))) > 0, "entry after parasite radius 12")
    need(oracle.core_dates[2][0] == 100, "core radius 10")
    need(sign(sub(dates[0], root(225))) < 0, "entry <= qualified_first + core/2")
    closed = [sorted({i for part in component for i in part})
              for component in oracle.gamma(2, Q(144))]
    need([0, 1, 2, 3, 4, 5] in closed, "x belongs to six-site core component at parasite")
    need(Q(250) > Q(29, 2)**2, "exact radical proof e>12")

    # Intrinsic sharpness only: no claim these profile changes are realized by clouds.
    t, s, mu, delta = Q(1), Q(3), Q(6), Q(1, 4)
    e = max(t, mu-(s-t))
    shifted = max(t+delta, mu+delta-((s-delta)-(t+delta)))
    need(shifted-e == 3*delta, "abstract intrinsic sharp entry constant 3")
    result = dict(scope="pure bounded mathematical guards; no native qualification",
                  checks=CHECKS, insertions=insertions, image_domain=domain,
                  qualified_counterexample=dict(points=points, first_beta="100",
                      core_beta="100", parasite_beta="144", entry=encode(dates[0]),
                      x_profile=rows[0], closed_at_parasite=closed),
                  abstract_only=dict(t=str(t), s=str(s), meet=str(mu), delta=str(delta),
                      old_entry=str(e), new_entry=str(shifted), ratio="3",
                      geometric_realization="not established"))
    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == "__main__":
    main()
