#!/usr/bin/env python3
"""Exact local counterexample to stopping a q4 seed at its first deep block.

Read-only rational calculation; imports only the existing Gram oracle helpers.
Does not run, model admission by, or qualify the C++ pipeline.
"""

from __future__ import annotations

import hashlib
import json
from fractions import Fraction as Q
from itertools import combinations
from pathlib import Path

from meb_rational_oracle_20260905 import (
    check, circumball, dot, power, subtract,
)


POINTS = [(57, 50, 51), (45, 55, 50), (45, 45, 50),
          (57, 50, 49), (57, 50, 48)]


def main() -> None:
    a, b, x, y, z = POINTS
    check(len(set(POINTS)) == 5, "five distinct points")
    check(all(0 <= v <= 65535 for p in POINTS for v in p), "u16")
    ab, ax = subtract(b, a), subtract(x, a)
    normal = (ab[1] * ax[2] - ab[2] * ax[1],
              ab[2] * ax[0] - ab[0] * ax[2],
              ab[0] * ax[1] - ab[1] * ax[0])
    gram, d2 = dot(normal, normal), dot(ab, ab)
    face = circumball([a, b, x])
    check(face is not None and face["positive"], "acute seed")
    l_ax, l_bx = dot(ax, ax), dot(subtract(x, b), subtract(x, b))
    jung = d2 * (3 * gram - 2 * l_ax * l_bx)
    check((gram, d2, jung, normal) ==
          (14500, 170, 1615000, (-10, 0, 120)), "seed constants")
    # Physical powers computed by Gram elimination, not by the sweep recurrence.
    roots = []
    for pid in (3, 4):
        site = POINTS[pid]
        p = gram * power(face, site)
        bz = dot(normal, subtract(site, a))
        check(bz < 0, "two exits")
        mu = p / bz
        check(2 * p * p <= jung * bz * bz, "root in closed chord")
        twice_offset = tuple(2 * site[i] - a[i] - b[i] for i in range(3))
        check(dot(twice_offset, twice_offset) <= 4 * d2, "site in cover")
        tetra = circumball([a, b, x, site])
        check(tetra is not None, "independent completion")
        expected_center = tuple(face["center"][i] + mu * normal[i] / (2 * gram)
                                for i in range(3))
        check(tetra["center"] == expected_center, "affine center identity")
        affine = [gram * power(face, v) - mu * dot(normal, subtract(v, a))
                  for v in POINTS]
        physical = [power(tetra, v) for v in POINTS]
        check(affine == [gram * v for v in physical], "all power identities")
        depth = sum(v < 0 for v in physical)
        roots.append({"point_id": pid, "P": p, "B": bz, "mu": mu,
                      "strict_depth": depth, "scaled_powers": affine})
    roots.sort(key=lambda row: row["mu"])
    check([r["point_id"] for r in roots] == [4, 3], "Z precedes Y")
    check([r["mu"] for r in roots] == [Q(-1325, 6), Q(-100)], "root values")
    check([r["strict_depth"] for r in roots] == [1, 0], "depth decreases")
    check(roots[0]["scaled_powers"][3] == -29000, "Y inside at Z")
    check(roots[1]["scaled_powers"][4] == 43500, "Z outside at Y")
    # The second root really is a positive, owned, canonical q4 candidate.
    tetra_y = circumball([a, b, x, y])
    check(tetra_y is not None and tetra_y["positive"], "positive tetrahedron")
    check(tetra_y["center"] == (Q(50), Q(50), Q(50)), "center")
    check(tetra_y["radius"] == 50, "radius squared")
    weights = [Q(5, 24), Q(7, 24), Q(7, 24), Q(5, 24)]
    check(sum(weights) == 1 and min(weights) > 0, "positive barycentrics")
    check(tuple(sum(w * p[i] for w, p in zip(weights, [a, b, x, y]))
                for i in range(3)) == tetra_y["center"], "barycentric center")
    edges = [(dot(subtract(POINTS[i], POINTS[j]),
                  subtract(POINTS[i], POINTS[j])), (i, j))
             for i, j in combinations(range(4), 2)]
    owner = min(edges, key=lambda edge: (-edge[0], edge[1]))
    check(owner == (d2, (0, 1)), "maximum edge with PointId tie break")
    eligible = []
    for pid in (2, 3):
        seed = circumball([a, b, POINTS[pid]])
        if seed is not None and seed["positive"]:
            eligible.append(pid)
    check(eligible and min(eligible) == 2, "X is the canonical acute seed")
    l_ay = dot(subtract(y, a), subtract(y, a))
    l_by = dot(subtract(y, b), subtract(y, b))
    l_xy = dot(subtract(y, x), subtract(y, x))
    check(2 * max(l_ay, l_by, l_xy) > d2 and
          max(l_ax + l_ay, l_bx + l_by) > d2, "i64 necessary conditions")
    check(power(face, y) > 0, "positive face power")
    check(dot(subtract(z, b), subtract(z, b)) == 173 > d2,
          "Z was rejected by lens before the old depth counter")
    # This is a mathematical counterexample, not execution of a product mutant.
    h4 = 1
    check(roots[0]["strict_depth"] >= h4 > roots[1]["strict_depth"],
          "early seed stop loses a shallow completion")
    audit = Path(__file__).resolve().parent
    pins = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
            for p in [Path(__file__), audit / "meb_rational_oracle_20260905.py"]}
    result = {
        "schema": "mhgp7-q4-block-rational-counterfixture-v1",
        "status": "passed", "scope": "local rational geometry only",
        "public_status": "not_claimed", "cpp_executed": False,
        "global_regularity_qualified": False,
        "points_in_point_id_order": POINTS, "seed": [0, 1, 2],
        "smax": 4, "h4": h4, "D2": d2, "G": gram, "J": jung,
        "normal": normal, "roots": roots,
        "retained_positive_support": [0, 1, 2, 3],
        "center": tetra_y["center"], "radius_squared": tetra_y["radius"],
        "barycentrics": weights, "counter_population_difference": {
            "deep_roots": 1, "old_depth_killed": 0, "old_lens_rejected": 1},
        "counterexample": "breaking at deep Z loses admissible Y",
        "helper_pins": pins,
    }
    print(json.dumps(result, indent=2, default=str) + "\n", end="")


if __name__ == "__main__":
    main()
