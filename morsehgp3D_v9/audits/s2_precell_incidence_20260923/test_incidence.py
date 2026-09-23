#!/usr/bin/env python3
"""Independent exact-integer checks for the conservative q4 cell incidence.

V, L, H and M are in units of one quarter of an input lattice unit.
The tests use exact integer lattice points and deliberately include closed
face, plane, radial and axial-projection tangencies.
"""

from itertools import product
from random import Random

if not __debug__:
    raise RuntimeError("this fixture requires assertions enabled")


def may_meet(a, b, low, high):
    d = [y - x for x, y in zip(a, b)]
    D = sum(x * x for x in d)
    assert D > 0
    M = [2 * (x + y) for x, y in zip(a, b)]
    gap = [max(l - m, 0, m - h) for l, m, h in zip(low, M, high)]
    if any(x * x > 2 * (D - di * di) for x, di in zip(gap, d)):
        return False
    if sum(x * x for x in gap) > 2 * D:
        return False
    pmin = sum(di * ((l if di >= 0 else h) - m)
               for di, l, h, m in zip(d, low, high, M))
    pmax = sum(di * ((h if di >= 0 else l) - m)
               for di, l, h, m in zip(d, low, high, M))
    return pmin <= 0 <= pmax


def in_disk(a, b, v):
    d = [y - x for x, y in zip(a, b)]
    D = sum(x * x for x in d)
    M = [2 * (x + y) for x, y in zip(a, b)]
    delta = [x - m for x, m in zip(v, M)]
    return sum(x * y for x, y in zip(delta, d)) == 0 and sum(x * x for x in delta) <= 2 * D


def check(a, b, low, high, witness, expected):
    assert all(l <= x <= h for l, x, h in zip(low, witness, high))
    assert may_meet(a, b, low, high) == expected
    if expected:
        assert in_disk(a, b, witness)
    else:
        assert not in_disk(a, b, witness)


# An exact q4 boundary point shared by two closed cells; the bisector plane
# is simultaneously a cell face. Replacing a strict rejection with >= fails.
a, b = (0, 0, 0), (4, 0, 0)
for low, high in [((4, 4, 4), (8, 8, 8)), ((8, 4, 4), (12, 8, 8))]:
    check(a, b, low, high, (8, 4, 4), True)
check(a, b, (8, 4, 5), (8, 4, 5), (8, 4, 5), False)  # radial strict outside
check(a, b, (8, 6, 0), (8, 6, 0), (8, 6, 0), False)  # axial projection
check(a, b, (9, 0, 0), (9, 0, 0), (9, 0, 0), False)  # bisector plane
a, b = (0, 0, 0), (2, 2, 0)
check(a, b, (4, 4, 4), (4, 4, 4), (4, 4, 4), True)  # radial and axial equality

# Search every integer quarter-coordinate inside small random cells. This is
# a one-way oracle: any actual disk point must survive the conservative gate.
rng = Random(0xEC311)
tested = found = 0
for _ in range(1200):
    a = tuple(rng.randrange(4) for _ in range(3))
    b = tuple(rng.randrange(4) for _ in range(3))
    if a == b:
        continue
    low = tuple(rng.randrange(-4, 17) for _ in range(3))
    high = tuple(x + rng.randrange(5) for x in low)
    actual = any(in_disk(a, b, v) for v in product(*(range(l, h + 1) for l, h in zip(low, high))))
    gate = may_meet(a, b, low, high)
    assert not actual or gate, (a, b, low, high)
    found += actual
    tested += 1
print(f"PASS: {tested} cells, {found} with an exact quarter-lattice disk point; tangencies retained")
