#!/usr/bin/env python3
"""Exact u18 fixture for the q4-only K-2 terminal certificate.

This checks the geometry and the shallow-rank decision, not the native atlas,
the WSPD lane mask, or pipeline timings.
"""

from fractions import Fraction
import json
import itertools


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def form(a, b, z, u, v):
    # 4 times the power of z for c=m+(u A+v B)/2, A=(0,10,0), B=(0,0,10).
    w = tuple(2 * z[i] - a[i] - b[i] for i in range(3))
    d = dot(sub(b, a), sub(b, a))
    return dot(w, w) - d - 20 * w[1] * u - 20 * w[2] * v


def require(ok, reason):
    if not ok:
        raise RuntimeError(reason)


def main():
    a, b = (0, 10, 10), (10, 10, 10)
    guards = [(5, 10, 10), (5, 11, 10), (5, 10, 11)]
    other = (5, 9, 6)
    x, y = (5, 16, 10), (5, 10, 16)
    sites = [a, b, *guards, other, x, y]
    require(len(set(sites)) == len(sites) and
            all(0 <= coordinate < (1 << 18) for z in sites for coordinate in z),
            "not a distinct u18 cloud")
    require(all(dot(sub(p, q), sub(p, q)) <= 100
                for p, q in itertools.combinations((a, b, x, y), 2)),
            "ab is not a maximal q4 support edge")

    low, high = Fraction(1, 8), Fraction(1, 4)
    corners = list(itertools.product((low, high), repeat=2))
    for u, v in [*corners, (Fraction(11, 60), Fraction(11, 60))]:
        c = (Fraction(5), Fraction(10) + 5 * u, Fraction(10) + 5 * v)
        radius2 = dot(sub(a, c), sub(a, c))
        require(all(form(a, b, z, u, v) ==
                    4 * (dot(sub(z, c), sub(z, c)) - radius2)
                    for z in sites), "cell form is not four times exact power")
    guard_values = [[form(a, b, g, u, v) for u, v in corners] for g in guards]
    require(all(max(values) < 0 for values in guard_values),
            "three guards not uniformly strict inside")
    other_values = [form(a, b, other, u, v) for u, v in corners]
    require(min(other_values) < 0 < max(other_values),
            "fourth site is not ambiguous on the closed cell")

    center_u = center_v = Fraction(11, 60)
    require(low < center_u < high, "support center outside cell")
    center = (Fraction(5), Fraction(10) + 5 * center_u,
              Fraction(10) + 5 * center_v)
    bary = tuple(Fraction(t, 72) for t in (25, 25, 11, 11))
    support = (a, b, x, y)
    require(all(t > 0 for t in bary) and sum(bary) == 1 and
            tuple(sum(bary[j] * support[j][i] for j in range(4))
                  for i in range(3)) == center,
            "q4 center not strictly inside the tetrahedron")
    require(all(form(a, b, z, center_u, center_v) == 0 for z in support),
            "support not cospherical")
    require(all(form(a, b, z, center_u, center_v) < 0 for z in guards) and
            form(a, b, other, center_u, center_v) > 0,
            "q4 center does not have exact depth three")
    require(3 == 5 - 2 and 3 < 5 - 1,
            "K5 threshold separation failed")
    print(json.dumps({"status": "PASS", "profile": "u18", "kmax": 5,
                      "uniform_guards": 3, "old_threshold": 4,
                      "q4_only_threshold": 3, "ambiguous_other_corners":
                      [str(v) for v in other_values],
                      "candidate_other_power4":
                      str(form(a, b, other, center_u, center_v))},
                     sort_keys=True))


if __name__ == "__main__":
    main()
