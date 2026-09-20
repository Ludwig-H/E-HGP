#!/usr/bin/env python3
"""Exact closed-segment clipping model; independent of product and audit C++."""
from fractions import Fraction as F
import json
import random


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def direct(forms, base, t):
    values = [p + s*t for p, s in forms]
    return base + sum(v < 0 for v in values), tuple(i for i, v in enumerate(values) if v == 0)


def sweep(forms, base, segment, t0, clip=False, mutant=False):
    lo, hi = segment
    require(lo <= t0 <= hi, "sample outside the closed segment")
    count, constants, groups = base, [], {}
    for i, (p, s) in enumerate(forms):
        if s == 0:
            count += p < 0
            if p == 0:
                constants.append(i)
            continue
        root = -F(p)/s
        if clip and not lo <= root <= hi:
            require(p+s*t0 != 0, "discarded root touched the sample")
            if not mutant:
                count += p+s*t0 < 0
            continue
        groups.setdefault(root, []).append(i)
        count += s > 0
    out = []
    for root, ids in sorted(groups.items()):
        count -= sum(forms[i][1] > 0 for i in ids)
        if lo <= root <= hi:
            out.append((root, count, tuple(sorted(constants+ids))))
        count += sum(forms[i][1] < 0 for i in ids)
    return out, len(groups), sum(map(len, groups.values()))


def check(forms, base, segment, t0, counts):
    full, ng, ne = sweep(forms, base, segment, t0)
    clipped, cg, ce = sweep(forms, base, segment, t0, clip=True)
    require(full == clipped, "clipping changed a strict root depth or shell")
    counts["cases"] += 1
    counts["full_events"] += ne
    counts["clipped_events"] += ce
    counts["outside_groups_removed"] += ng-cg
    for root, depth, shell in clipped:
        require((depth, shell) == direct(forms, base, root), "root census mismatch")
        counts["root_checks"] += 1
    lo, hi = segment
    roots = sorted({-F(p)/s for p, s in forms if s and lo <= -F(p)/s <= hi})
    require([root for root, _, _ in clipped] == roots, "missing retained event")
    boundaries = sorted(set([lo, hi, t0]+roots))
    probes = boundaries+[(a+b)/2 for a, b in zip(boundaries, boundaries[1:])]
    for t in probes:
        # Replace only nonconstant forms with roots strictly outside the segment.
        values = [p+s*(t0 if s and not lo <= -F(p)/s <= hi else t) for p, s in forms]
        actual = base+sum(v < 0 for v in values), tuple(i for i, v in enumerate(values) if v == 0)
        require(actual == direct(forms, base, t), "clipped algebraic identity mismatch")
        counts["point_checks"] += 1


def side_sample(form, cell, q):
    """Homogeneous point on line intersect closed cell, or None if disjoint.

    Cell bounds are integer multiples of 1/q. No general rational products.
    """
    c, a, b = form
    require(a or b, "zero form is not a seed line")
    al, ah, bl, bh = cell
    candidates = []
    if b:
        candidates += [(alpha*b, -q*c-a*alpha, q*b) for alpha in (al, ah)]
    if a:
        candidates += [(-q*c-b*beta, beta*a, q*a) for beta in (bl, bh)]
    for nx, ny, den in candidates:
        if den < 0:
            nx, ny, den = -nx, -ny, -den
        if al*den <= q*nx <= ah*den and bl*den <= q*ny <= bh*den:
            require(c*den+a*nx+b*ny == 0, "side sample misses the seed line")
            return nx, ny, den
    return None


def main():
    counts = dict(cases=0, root_checks=0, point_checks=0, full_events=0,
                  clipped_events=0, outside_groups_removed=0, side_samples=0,
                  max_sample_evaluation_bits=0)
    fixtures = [
        ([(0, 0), (-1, 0), (1, 0), (0, 1), (0, -1), (-2, 1), (2, -1)], (F(0), F(2))),
        ([(0, 0), (-1, 1), (1, -1), (2, 1), (-3, -1)], (F(1), F(1))),
        ([(0, 0), (-1, -1), (-1, 1)], (F(0), F(2))),
        ([(0, 0), (-3, 0), (5, 0)], (F(-2), F(3)))
    ]
    for forms, segment in fixtures:
        for t0 in (segment[0], segment[1], sum(segment)/2):
            check(forms, 2, segment, t0, counts)
    forms, segment = fixtures[2]
    right = sweep(forms, 0, segment, F(0), clip=True)[0]
    wrong = sweep(forms, 0, segment, F(0), clip=True, mutant=True)[0]
    require(right != wrong and right[0][1] == 1 and wrong[0][1] == 0,
            "discard-outside-without-constant mutant survived")
    rng = random.Random(20260922)
    for _ in range(600):
        forms = [(F(rng.randrange(-30, 31), rng.randrange(1, 10)),
                  F(rng.randrange(-8, 9), rng.randrange(1, 10))) for _ in range(14)]
        forms += [(F(0), F(0))]  # Seed, or a coplanar constant shell presentation.
        segment = tuple(sorted(F(rng.randrange(-20, 21), rng.randrange(1, 9)) for _ in range(2)))
        t0 = segment[0]+F(rng.randrange(9), 8)*(segment[1]-segment[0])
        check(forms, rng.randrange(6), segment, t0, counts)
    M, q = 65535, 1024
    samples = [((16, -16, 16), (0, 1, -4, -3), 4),
               ((1, -2, 0), (0, 1, 0, 1), 1),
               ((1, 0, -2), (0, 1, 0, 1), 1),
               ((-2, 1, 1), (0, 1, 0, 1), 1)]
    samples += [(tuple(rng.randrange(-bound, bound+1)*M*M for bound in (15, 8, 8)),
                 (-2*q, 2*q, -2*q, 2*q), q) for _ in range(600)]
    for form, cell, q in samples:
        if not (form[1] or form[2]):
            continue
        answer = side_sample(form, cell, q)
        c, a, b = form
        corners = [q*c+a*x+b*y for x in cell[:2] for y in cell[2:]]
        require((answer is not None) == (min(corners) <= 0 <= max(corners)), "side intersection completeness")
        if answer is None:
            continue
        nx, ny, den = answer
        counts["side_samples"] += 1
        for witness in ((15*M*M, 8*M*M, -8*M*M), (-15*M*M, -8*M*M, 8*M*M)):
            terms = witness[0]*den, witness[1]*nx, witness[2]*ny
            require(sum(abs(t) for t in terms) <= 616*M**4*q < 1 << 84 < 1 << 127, "sample evaluation i128 bound")
            require(F(sum(terms), den) == witness[0]+witness[1]*F(nx, den)+witness[2]*F(ny, den), "sample evaluation mismatch")
            counts["max_sample_evaluation_bits"] = max(counts["max_sample_evaluation_bits"], abs(sum(terms)).bit_length())
    print(json.dumps({"schema": "mhgp8_audit_local_clip_math_v1", "status": "PASS",
                      "scope": "independent rational segment model; no C++ or product qualification",
                      "mutant": "discard_outside_without_constant", **counts}, sort_keys=True))


if __name__ == "__main__":
    main()
