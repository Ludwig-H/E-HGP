#!/usr/bin/env python3
"""Bounded exact guard for a robust Palm-obstruction pattern, not a Poisson simulation.

The general event and probability statements are proved in README.md. This
program checks the finite template with our frozen independent Gram/Gamma
kernel and its strict quantitative margins. It neither constructs H_infinity
nor tests percolation, convergence, native arithmetic or statistical quality.
"""
from fractions import Fraction as Q
from hashlib import sha256
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
from test_qualified import Oracle  # noqa: E402

CHECKS = 0


def need(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise RuntimeError(message)


def covers(oracle, beta):
    return sorted(tuple(sorted({i for part in component for i in part}))
                  for component in oracle.gamma(2, beta))


def d2(a, b):
    return sum((x-y)**2 for x, y in zip(a, b))


def main():
    X = [(0, 0, 0), (10, 0, 0), (20, 0, 0), (30, 0, 0),
         (43, 0, 0), (53, 0, 0), (-20, 10, 0), (-20, -10, 0)]
    oracle = Oracle(X, 2)
    R, F, delta, guard = Q(15), Q(12), Q(1, 10), Q(31)
    S = {0, 6, 7}
    need(oracle.meb((0, 6, 7))[0] == Q(625, 4), "rival birth radius 25/2")
    need(oracle.core_dates[2][0] == 100, "origin core radius 10")
    for z in range(2, len(X)):
        need(oracle.meb((0, 1, z))[0] >= 100, "every first coface through x,y starts at >=10")
    need(oracle.meb((0, 1, 2))[0] == 100, "first qualified primary at 10")
    early = next(beta for beta in oracle.levels
                 if any(0 in C and len(C) >= 3 for C in covers(oracle, beta)))
    need(early == 100, "exhaustive first qualified radius 10")

    chain = [(0, 1, 2), (1, 2, 3), (2, 3, 4), (3, 4, 5)]
    chain_levels = [oracle.meb(part)[0] for part in chain]
    need(chain_levels == [100, 100, Q(529, 4), Q(529, 4)], "strict primary coface chain")
    need(Q(23, 2)+delta < F, "primary chain stays active at 12 in every open box")
    at_F, at_R = covers(oracle, F*F), covers(oracle, R*R)
    need((0, 1, 2, 3, 4, 5) in at_F, "primary contains origin and port at F")
    need((0, 6, 7) in at_R, "rival component remains separate at R")
    need(all(not {0, 1, 6}.issubset(C) for C in at_R), "primary and rival not merged at R")
    minimum_w_primary = min(d2(X[w], X[i]) for w in (6, 7) for i in range(1, 6))
    need(minimum_w_primary == 1000, "minimum local rival-primary distance squared")
    need(minimum_w_primary > (2*R+2*delta)**2, "no local cross-pair even after perturbation")
    need(guard-delta > 2*R, "foreign sites outside guard cannot reach perturbed S")
    need(Q(25, 2)+delta < R, "rival is qualified before isolation cut")
    entry_lower = (Q(10)-delta) + (R-(Q(25, 2)+delta))
    need(entry_lower == Q(123, 10) and entry_lower > F, "uniform delayed-entry certificate")
    need(delta < Q(1, 6), "chosen open-box radius satisfies stronger stated bound")
    need(min(d2(X[5], X[s]) for s in S) > (guard+delta)**2,
         "port is strictly outside protected region")

    # Representative admissible outside additions, never a substitute for the proof.
    foreign = [(60, 0, 0), (70, 0, 0), (0, 0, 40)]
    for z in foreign:
        need(all(d2(z, X[s]) > guard*guard for s in S), "admissible foreign point")
    extended = Oracle(X+foreign, 2)
    early_extended = next(beta for beta in extended.levels
                          if any(0 in C and len(C) >= 3 for C in covers(extended, beta)))
    need(early_extended == 100, "foreign additions did not advance qualification")
    need((0, 6, 7) in covers(extended, R*R), "rival still isolated with outside additions")
    need(any({0, 5, 8, 9}.issubset(C) for C in covers(extended, F*F)),
         "primary port can accept an outside corridor")

    print(json.dumps(dict(scope="finite exact template and deterministic open margins only",
                          checks=CHECKS, points=X, R=str(R), F=str(F),
                          delta=str(delta), protected_radius=str(guard),
                          first_qualified_beta=str(early), rival_beta="625/4",
                          core_beta="100", primary_chain_beta=list(map(str, chain_levels)),
                          covers_at_F=at_F, covers_at_R=at_R,
                          uniform_entry_lower=str(entry_lower),
                          foreign_points=foreign,
                          conditional_claims=["infinite FULL component must exist",
                                              "H_infinity must be defined and measurable",
                                              "no finite-window limit is proved"]),
                     sort_keys=True, indent=2))


if __name__ == "__main__":
    main()
