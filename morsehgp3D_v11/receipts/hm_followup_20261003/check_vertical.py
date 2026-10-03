#!/usr/bin/env python3
"""Deux ordres qualifies de P1 peuvent choisir des proprietaires incompatibles.

Six sites collineaires, Gamma/MEB en Fraction de notre modele independant.
Ce temoin refute V pour P1 qualifie, pas toute synthese multi-k stable.
Le theoreme D v10 exige aussi l'entree immediate non ambigue.
"""
from fractions import Fraction as Q
from hashlib import sha256
import json
from math import isqrt
from pathlib import Path

import check_qualified_delay as model

POINTS = [(x, 0, 0) for x in (0, 3, 6, 18, 19, 20)]
CHECKS = 0


def require(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise ValueError(message)


def root(value):
    v = Q(isqrt(value.numerator), isqrt(value.denominator))
    require(v*v == value, "collinear exact rational radius")
    return v


def blocks(oracle, k, m, radius):
    cuts = [(level, oracle.gamma(k, level)) for level in oracle.levels]
    entries, anchors = [], []
    for i in range(len(POINTS)):
        first, rival = model.first_and_rival(cuts, m, i)
        entries.append(root(first)+root(rival[0])-root(rival[1]))
        anchor = next(component for beta, components in cuts if beta == first
                      for component in components
                      if i in model.coverage(component) and len(model.coverage(component)) >= m)
        anchors.append(anchor)
    components = oracle.gamma(k, radius*radius)
    groups = {}
    for i, (entry, anchor) in enumerate(zip(entries, anchors)):
        if entry <= radius:
            owners = [j for j, c in enumerate(components) if anchor <= c]
            require(len(owners) == 1, "unique alive ancestor of selected first branch")
            groups.setdefault(owners[0], set()).add(i)
    return entries, sorted([sorted(g) for g in groups.values()])


def run():
    oracle = model.Oracle(POINTS, 3)
    e2, b2 = blocks(oracle, 2, 3, Q(7))
    e3, b3 = blocks(oracle, 3, 4, Q(7))
    require(e2 == list(map(Q, (3, 3, 4, 1, 1, 1))), "exact P1 Pi3 dates")
    require(e3 == list(map(Q, (9, 8, 7, 7, 7, 7))), "exact P1 Pi4 dates")
    require(b2 == [[0, 1, 2], [3, 4, 5]], "order2 blocks at7")
    require(b3 == [[2, 3, 4, 5]], "order3 block at7")
    require(any(not any(set(b) <= set(a) for a in b2) for b in b3), "higher-order block does not refine order2")
    covered2 = sorted([sorted(model.coverage(c)) for c in oracle.gamma(2, Q(49))])
    covered3 = sorted([sorted(model.coverage(c)) for c in oracle.gamma(3, Q(49))])
    require(covered2 == [[0, 1, 2], [2, 3, 4, 5]], "two raw FULL2 coverages at7")
    require(covered3 == [[0, 1, 2], [2, 3, 4, 5]], "two raw FULL3 coverages; only right is qualified")
    early = [model.coverage(c) for c in oracle.gamma(2, Q(9))]
    require(any(c == {0, 1, 2} for c in early), "left qualified cover at3")
    require(not any({0, 5} <= model.coverage(c) for c in oracle.gamma(2, Q(49))),
            "no faithful lower-order common block can contain both endpoints")
    here = Path(__file__).resolve().parent
    dependencies = {"check_vertical.py": sha256(Path(__file__).read_bytes()).hexdigest(),
                    "check_qualified_delay.py": sha256((here/"check_qualified_delay.py").read_bytes()).hexdigest()}
    return dict(schema="ehgp.v11.hm_vertical_check.v1", status="pass", checks=CHECKS,
                scope="bounded exact P1 vertical counterexample; no general impossibility without extra axiom",
                points=POINTS, radius="7", k2=2, m2=3, k3=3, m3=4,
                entries2=[str(e) for e in e2], entries3=[str(e) for e in e3],
                blocks2=b2, blocks3=b3, coverages2=covered2, coverages3=covered3,
                violated="V: partitions of active points refine lower orders at identical radius",
                extension_requires="entry immediately when only ONE QUALIFIED component covers a site",
                constructive_concession="transport highest-order anchors to lower orders; lose their own early entries",
                dependencies=dependencies, native_runs=0, fits=0)


if __name__ == "__main__":
    print(json.dumps(run(), sort_keys=True, separators=(",", ":")))
