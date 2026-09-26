#!/usr/bin/env python3
"""Audit-only exact affine/window oracle. No product or GPU qualification."""
from fractions import Fraction
from hashlib import sha256
from math import isqrt
from pathlib import Path
import json
import random


def require(value, reason):
    if not value:
        raise RuntimeError(reason)


def power(point):
    # a=(-8,0,0), b=(8,0,0), x=(0,12,0), a strictly acute owned seed.
    # Gram=36864, normal=(0,0,192), physical q3 centre=(0,10/3,0).
    x, y, z = point
    return 36864 * (x*x + y*y + z*z - 64) - 245760*y, 192*z


def windows(forms, lo, hi, counts):
    lens = [i for i, (p, s) in enumerate(forms)
            if p-lo*s < 0 and p-hi*s < 0]
    constant_shell = [i for i, (p, s) in enumerate(forms) if p == s == 0]
    events = [(Fraction(p, s), s, i) for i, (p, s) in enumerate(forms)
              if s and lo <= Fraction(p, s) <= hi]
    entries = sorted((r, i) for r, s, i in events if s > 0)
    exits = sorted((r, i) for r, s, i in events if s < 0)
    roots = sorted({r for r, _, _ in events})
    previously_deep = {k: False for k in range(3, 11)}
    for r in roots:
        # This oracle computes boundaries directly. A product must obtain all
        # boundaries with grouped scans, not these Python comprehensions.
        before = [i for t, i in entries if t < r]
        after = [i for t, i in exits if t > r]
        packet = lens + before + after
        actual = [i for i, (p, s) in enumerate(forms) if p-r*s < 0]
        shell = constant_shell + [i for t, _, i in events if t == r]
        actual_shell = [i for i, (p, s) in enumerate(forms) if p-r*s == 0]
        require(len(packet) == len(set(packet)), "window duplicate")
        require(sorted(packet) == actual, "window omitted or added interior")
        require(sorted(shell) == actual_shell, "shell identity")
        # The T1 option: already-paid comparisons collect event IDs; lens
        # IDs are copied only if the final monotonically counted depth is low.
        t1 = lens + [i for t, s, i in events
                     if (s > 0 and t < r) or (s < 0 and t > r)]
        require(sorted(t1) == actual, "T1 payload differs")
        counts["root_groups"] += 1
        counts["tied_groups"] += int(sum(t == r for t, _, _ in events) > 1)
        counts["boundary_groups"] += int(r in (lo, hi))
        counts["interior_ids"] += len(actual)
        for k in range(3, 11):
            if len(actual) < k-2:
                require(len(lens) < k-2, "accepted bucket cannot be saturated")
                require(len(packet) <= k-3, "packet exceeds accepted capacity")
                counts["accepted_groups_k%d" % k] += 1
                counts["deep_to_shallow_transitions"] += int(previously_deep[k])
                previously_deep[k] = False
            else:
                previously_deep[k] = True


def main():
    counts = dict(fixtures=0, root_groups=0, tied_groups=0, boundary_groups=0,
                  interior_ids=0, deep_to_shallow_transitions=0, causal_mutants=0)
    counts.update({"accepted_groups_k%d" % k: 0 for k in range(3, 11)})
    q = 256 * (3*36864 - 2*208*208)
    m = isqrt(q // 2)
    while 2*m*m < q:
        m += 1
    grid = [-m, -(3*m//4), -(m//2), -(m//4), 0,
            m//4, m//2, 3*m//4, m]
    rng = random.Random(20260926)
    fixed = [(-8, 0, 0), (8, 0, 0), (0, 12, 0), (0, 0, 8), (0, 0, -8),
             (1, 0, 1), (-1, 0, 1), (1, 0, -1), (-1, 0, -1)]
    for size in (9, 16, 32, 65, 128):
        for _ in range(24):
            points = list(fixed)
            seen = set(points)
            while len(points) < size:
                p = tuple(rng.randint(-18, 18) for _ in range(3))
                if p not in seen:
                    points.append(p)
                    seen.add(p)
            forms = list(map(power, points))
            for lo, hi in zip(grid, grid[1:]):
                windows(forms, lo, hi, counts)
            counts["fixtures"] += 1

    # An owned positive tetrahedron with 0, 1 and 2 strict interior sites:
    # its centre is (0, 10/3, 19/15), strictly inside the tetrahedron.
    tetra = [(-8, 0, 0), (8, 0, 0), (0, 12, 0), (0, 4, 10)]
    interior = [(0, 3, 1), (1, 3, 1)]
    centre = (Fraction(0), Fraction(10, 3), Fraction(19, 15))
    weights = (Fraction(287, 900), Fraction(287, 900), Fraction(53, 225), Fraction(19, 150))
    require(sum(weights) == 1 and all(w > 0 for w in weights), "positive tetra weights")
    require(all(sum(w*p[c] for w, p in zip(weights, tetra)) == centre[c]
                for c in range(3)), "positive tetra centre")
    dist2 = lambda a, b: sum((x-y)**2 for x, y in zip(a, b))
    radius2 = dist2(centre, tetra[0])
    require(all(dist2(centre, p) == radius2 for p in tetra), "tetra shell")
    require(all(dist2(centre, p) < radius2 for p in interior), "tetra interiors")
    require(all(dist2(tetra[i], tetra[j]) < 256 for i in range(4)
                for j in range(i+1, 4) if (i, j) != (0, 1)), "tetra owner")
    for depth in range(3):
        points = tetra + interior[:depth]
        for lo, hi in zip(grid, grid[1:]):
            windows(list(map(power, points)), lo, hi, counts)
        counts["fixtures"] += 1

    # Adversarial affine cases: mixed simultaneous entry/exit, constants,
    # endpoint contacts, repeated roots and deep -> shallow transitions.
    forms = [(1, 1), (-1, -1), (2, 2), (-2, -2), (-1, 0), (0, 0),
             (0, 1), (0, -1), (2, 1), (-2, -1), (-4, -1)]
    for lo, hi in ((0, 1), (1, 2), (2, 4), (0, 4)):
        windows(forms, lo, hi, counts)
    counts["fixtures"] += 1
    windows([(1, 1), (-1, -1), (0, 0)], 0, 2, counts)
    counts["fixtures"] += 1
    windows([(-i, -1) for i in range(1, 11)] + [(0, 1), (11, 1)], 0, 11, counts)
    counts["fixtures"] += 1

    # Mutant A: capped active reservoir loses an unrecorded site on exits.
    active, reservoir = {1, 2, 3}, {1, 2}
    for i in (1, 2):
        active.remove(i)
        reservoir.discard(i)
    require(active != reservoir and len(active) == 1, "reservoir mutant alive")
    counts["causal_mutants"] += 1
    # Mutant B: entry before emission wrongly includes an exact contact.
    r = Fraction(1)
    actual = {i for i, (p, s) in enumerate(forms) if p-r*s < 0}
    mutant = actual | {i for i, (p, s) in enumerate(forms) if s > 0 and p-r*s == 0}
    require(mutant != actual, "entry-before-emission mutant alive")
    counts["causal_mutants"] += 1
    # Mutant C: emitting before exits has the symmetric contact error.
    mutant = actual | {i for i, (p, s) in enumerate(forms) if s < 0 and p-r*s == 0}
    require(mutant != actual, "exit-after-emission mutant alive")
    counts["causal_mutants"] += 1
    # Raw count/sum/xor cannot identify IDs even when there are only two.
    require({0, 3} != {1, 2} and 0+3 == 1+2 and 0 ^ 3 == 1 ^ 2,
            "count/sum/xor counterexample")
    counts["causal_mutants"] += 1
    # Mutant D: copying one point fewer and forging its count passes all
    # remaining signs. Complete cardinality cannot come from the payload.
    require(len(actual) > 1, "missing interior fixture")
    forged = actual - {min(actual)}
    require(forged < actual and all(forms[i][0]-r*forms[i][1] < 0 for i in forged),
            "forged count example")
    counts["causal_mutants"] += 1
    # Mutant F: forgetting the sign of a negative denominator reverses an
    # event root and hence whether an exiting site is still interior.
    require(Fraction(-2, -1) > 1 and Fraction(-2, abs(-1)) < 1,
            "negative-denominator mutant alive")
    counts["causal_mutants"] += 1
    # Mutant G: retaining a single representative of a tied root loses shell.
    same = [i for i, (p, s) in enumerate(forms) if s and Fraction(p, s) == r]
    require(len(same) > 1 and {same[0]} != set(same), "tie-collapse mutant alive")
    counts["causal_mutants"] += 1
    require(all(counts[k] > 0 for k in ("tied_groups", "boundary_groups",
            "accepted_groups_k3", "accepted_groups_k5", "accepted_groups_k10",
            "deep_to_shallow_transitions")),
            "vacuous fixture")
    print(json.dumps({"schema": "mhgp9_audit_q4_payload_math_v1", "status": "pass",
                      "scope": "exact_affine_identities_not_engine_or_gpu",
                      "script_sha256": sha256(Path(__file__).read_bytes()).hexdigest(),
                      "counters": counts}, sort_keys=True))


if __name__ == "__main__":
    main()
