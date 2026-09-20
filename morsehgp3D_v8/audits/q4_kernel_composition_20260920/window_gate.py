#!/usr/bin/env python3
"""Independent weighted event-window model; no product imports."""
from fractions import Fraction as F
import json
import random


def require(ok, why):
    if not ok:
        raise RuntimeError(why)


def groups(raw):
    out, start = [], 0
    for root, kind, weight in raw:
        require(kind in ('E', 'I') and weight > 0, 'invalid event group')
        out.append((F(root), kind, tuple(range(start, start+weight))))
        start += weight
    return out


def census(events, t, c, shell):
    depth = c+sum(len(ids) for root, kind, ids in events if (t < root if kind == 'E' else root < t))
    boundary = tuple(sorted(list(shell)+[i for root, _, ids in events if root == t for i in ids]))
    return depth, boundary


def window(events, T, c, weighted=True):
    if c >= T:
        return None
    H = T-c
    exits, entries = [], []
    for root, kind, ids in events:
        (exits if kind == 'E' else entries).extend([root]*(len(ids) if weighted else 1))
    exits.sort(reverse=True); entries.sort()
    L = exits[H-1] if len(exits) >= H else None
    U = entries[H-1] if len(entries) >= H else None
    return None if L is not None and U is not None and L > U else (L, U, H)


def inside(t, interval, opened=False):
    if interval is None:
        return False
    L, U, _ = interval
    return (L is None or (L < t if opened else L <= t)) and (U is None or (t < U if opened else t <= U))


def check(events, T, c, shell, counts):
    interval = window(events, T, c)
    roots = sorted({r for r, _, _ in events})
    full = {r: census(events, r, c, shell) for r in roots if census(events, r, c, shell)[0] < T}
    retained = [e for e in events if inside(e[0], interval)]
    if interval is None:
        require(not full, 'empty window lost a shallow root')
    else:
        L, U, H = interval
        interior = sum(len(ids) for r, _, ids in retained if (L is None or L < r) and (U is None or r < U))
        require(interior <= 2*H-2, 'strict interior ID bound failed')
        t0 = L if L is not None else (U if U is not None else F(0))
        dropped = [e for e in events if not inside(e[0], interval)]
        base = census(dropped, t0, c, ())[0]
        clipped = {r: census(retained, r, base, shell) for r in roots
                   if inside(r, interval) and census(retained, r, base, shell)[0] < T}
        require(clipped == full, 'window changed admitted roots, strict depth or complete shell')
        for r in roots:
            if inside(r, interval):
                require(census(retained, r, base, shell) == census(events, r, c, shell), 'in-window census changed')
                counts['root_checks'] += 1
    probes = roots+[(a+b)/2 for a, b in zip(roots, roots[1:])]
    probes += [roots[0]-1, roots[-1]+1] if roots else [F(0)]
    for t in probes:
        if not inside(t, interval):
            require(census(events, t, c, shell)[0] >= T, 'outside-window depth certificate failed')
            counts['outside_checks'] += 1
    counts['cases'] += 1
    counts['admitted_roots'] += len(full)


def main():
    counts = dict(cases=0, root_checks=0, outside_checks=0, admitted_roots=0, dual_formula_checks=0)
    fixtures = [([(0, 'E', 3), (0, 'I', 4)], 1, 0),
                ([(0, 'E', 3), (0, 'I', 4)], 2, 1),
                ([(0, 'E', 3), (-1, 'E', 3)], 2, 0),
                ([(0, 'I', 3), (1, 'I', 3)], 2, 0),
                ([(2, 'E', 1), (0, 'I', 1)], 1, 0),
                ([(F(1, 3), 'E', 1), (F(2, 3), 'I', 1)], 3, 0),
                ([], 2, 0), ([(0, 'E', 1)], 2, 2)]
    events = groups(fixtures[0][0])
    require(window(events, 1, 0) == (F(0), F(0), 1), 'isolated window changed')
    require(census(events, F(0), 0, (-2, -1)) == (0, tuple(range(-2, 7))), 'massive opposite shell lost IDs')
    require(not inside(F(0), window(events, 1, 0), opened=True), 'open-boundary mutant survived')
    events = groups(fixtures[2][0])
    wrong = window(events, 2, 0, weighted=False)
    wrong_inner = sum(len(ids) for r, _, ids in events if inside(r, wrong, opened=True))
    require(wrong_inner > 2, 'unweighted-quantile mutant did not break the interior ID bound')
    rng = random.Random(20260924)
    for _ in range(500):
        raw = [(F(rng.randrange(-9, 10), rng.randrange(1, 6)), rng.choice(('E', 'I')), rng.randrange(1, 5))
               for _ in range(rng.randrange(11))]
        fixtures.append((raw, rng.randrange(1, 7), rng.randrange(4)))
    for raw, T, c in fixtures:
        check(groups(raw), T, c, (-2, -1), counts)
    # Root coordinate and orientation in the finite dual chart, v0 != 0.
    for _ in range(300):
        u0, v0, u, v = [F(rng.randrange(-9, 10), rng.randrange(1, 6)) for _ in range(4)]
        if v0 == 0:
            continue
        cx, cz = rng.randrange(1, 8), rng.choice((-1, 1))*rng.randrange(1, 8)
        a, b, az, bz = cx*u0, cx*v0, cz*u, cz*v
        D = u*v0-v*u0
        if D:
            root = (v-v0)/D
            require(root == (b*cz-bz*cx)/(a*bz-az*b), 'fractional dual root formula failed')
            eta = -(cx+a*root)/b
            require(cz+az*root+bz*eta == 0, 'dual root not on witness line')
            require((az*b-bz*a)/b == cz*D/v0, 'entry/exit orientation formula failed')
        else:
            require(az*b-bz*a == 0, 'pole is not a constant restriction')
            require((cz*b-bz*cx == 0) == ((u, v) == (u0, v0)), 'pole/coincident-point distinction failed')
        counts['dual_formula_checks'] += 1
    print(json.dumps({'schema': 'mhgp8_audit_window_index_math_v1', 'status': 'PASS',
                      'scope': 'independent rational events and dual formulas; no index implementation or product qualification',
                      **counts, 'mutants': ['open_window_loses_isolated_root', 'unweighted_quantile_breaks_inner_id_bound']}, sort_keys=True))


if __name__ == '__main__':
    main()
