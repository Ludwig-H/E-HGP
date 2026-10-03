#!/usr/bin/env python3
"""Contre-garde H5 : un rival qualifie futur retarde un point coeur.

Reproduction independante du temoin du verificateur du workflow
wf_92a63749-ee7, verif_axiomes/RAPPORT_VERIFICATION.md, R1.
Gamma et MEB en Fraction viennent du modele borne de notre campagne close,
pas du produit, de son oracle ou du workflow. Aucun natif/fit.
Toutes les decisions de cette contre-garde sont rationnelles exactes.
"""
from fractions import Fraction as Q
from hashlib import sha256
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
EXPERIMENT = HERE.parent / "full_points_20261003/experiment"
sys.path.insert(0, str(EXPERIMENT))
from test_qualified import Oracle  # noqa: E402

POINTS = [(0, 0, 0), (10, 0, 0), (20, 0, 0), (30, 0, 0),
          (44, 0, 0), (54, 0, 0), (-20, 10, 0), (-20, -10, 0)]
CHECKS = 0


def require(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise ValueError(message)


def sign(a):
    return (a > 0) - (a < 0)


def diff_sign(x, y, u):
    """Signe de sqrt(x)-sqrt(y)-u, x/y>=0, par carres controles."""
    d = sign(x-y)
    if d >= 0 and u <= 0:
        return 0 if d == 0 and u == 0 else 1
    if d <= 0 and u >= 0:
        return 0 if d == 0 and u == 0 else -1
    if d < 0:
        return -diff_sign(y, x, -u)
    w = x-y-u*u
    if w <= 0:
        return 0 if w == 0 and y == 0 else -1
    return sign(w*w-4*u*u*y)


def sum_sign(a, b, c, d):
    """Signe de sqrt(a)+sqrt(b)-sqrt(c)-sqrt(d)."""
    return diff_sign(a*b, c*d, ((c+d)-(a+b))/2)


def coverage(component):
    return set(i for part in component for i in part)


def first_and_rival(cuts, m, site):
    qualified = [(beta, component) for beta, components in cuts
                 for component in components
                 if site in coverage(component) and len(coverage(component)) >= m]
    first, anchor = qualified[0]
    winner = (first, first)
    for beta, component in qualified:
        joined = anchor | component
        meet = next(level for level, components in cuts
                    if level >= max(first, beta)
                    and any(joined <= c for c in components))
        require(meet >= beta, "absolute meeting above rival birth")
        if sum_sign(meet, winner[1], winner[0], beta) > 0:
            winner = (meet, beta)
    return first, winner


def run():
    oracle = Oracle(POINTS, 2)
    cuts = [(level, oracle.gamma(2, level)) for level in oracle.levels]
    previous = next(groups for level, groups in reversed(cuts) if level < 144)
    closed = next(groups for level, groups in cuts if level == 144)
    require(any(coverage(c) == set(range(4)) for c in previous), "dense component before F")
    require(any(coverage(c) == {4, 5} for c in previous), "background pair before F")
    require(any(coverage(c) == set(range(6)) for c in closed), "true FULL fusion at F=12")
    raw_t, raw_rival = first_and_rival(cuts, 1, 0)
    qual_t, qual_rival = first_and_rival(cuts, 3, 0)
    require(raw_t == 25 and qual_t == 100, "alpha=5 and qualified t=10")
    require(raw_rival == (250, 125), "raw maximal rival interval [sqrt125,sqrt250]")
    require(qual_rival == (250, Q(625, 4)), "qualified rival [12.5,sqrt250]")
    require(oracle.core_dates[2][0] == 100, "x is a core point with d2=10")
    require(5+Q(10, 2) <= 12, "old alpha+d2/2 condition satisfied")
    require(sum_sign(raw_t, raw_rival[0], Q(144), raw_rival[1]) <= 0,
            "unqualified P1 entry before F")
    require(sum_sign(qual_t, qual_rival[0], Q(144), qual_rival[1]) > 0,
            "qualified P1 entry AFTER F, exactly sqrt250-5/2")
    require(Q(25, 2) > 12, "decisive qualified rival born AFTER F")
    require(diff_sign(qual_rival[0], qual_rival[1], Q(5)) <= 0,
            "qualified delay still bounded by d2/2")
    require(10+Q(10, 2) > 12, "corrected t-qualified+d2/2 condition does not promise entry")
    translated = [(x+20, y+10, z) for x, y, z in POINTS]
    require(all(0 <= a < 2**18 for p in translated for a in p), "unit integer fixture fits all profiles")
    for i in range(8):
        for j in range(8):
            require(sum((a-b)**2 for a, b in zip(POINTS[i], POINTS[j]))
                    == sum((a-b)**2 for a, b in zip(translated[i], translated[j])),
                    "translation preserves squared distances")
    dependencies = {"check_qualified_delay.py": sha256(Path(__file__).read_bytes()).hexdigest()}
    for name in ("test_qualified.py", "qualified.py"):
        dependencies["../full_points_20261003/experiment/"+name] = sha256((EXPERIMENT/name).read_bytes()).hexdigest()
    return dict(schema="ehgp.v11.hm_qualified_delay_check.v1", status="pass", checks=CHECKS,
                scope="bounded exact mathematical counterexample; no native qualification",
                points=POINTS, translated_u18=translated, k=2, m=3,
                alpha="5", core="10", F="12", qualified_first="10",
                rival_birth="25/2", rival_meet="sqrt(250)",
                raw_entry="5+sqrt(250)-sqrt(125)", qualified_entry="sqrt(250)-5/2",
                old_condition_satisfied=True, qualified_entry_after_F=True,
                corrected_sufficient_condition="qualified_first + d_k/2 <= F",
                dependencies=dependencies, product_imports=False, native_runs=0, fits=0)


if __name__ == "__main__":
    print(json.dumps(run(), sort_keys=True, separators=(",", ":")))
