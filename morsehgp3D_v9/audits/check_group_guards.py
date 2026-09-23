#!/usr/bin/env python3
"""Exact integer fixtures for the q3/q4 guard-group certificate; no input files."""
from itertools import combinations, product


def score(a, b, guards):
    d = tuple(b[i] - a[i] for i in range(3))
    D = sum(x * x for x in d)
    ws = [tuple(2 * g[i] - a[i] - b[i] for i in range(3)) for g in guards]
    H = len(guards) * D - sum(sum(x * x for x in w) for w in ws)
    W = tuple(sum(w[i] for w in ws) for i in range(3))
    C = (d[1] * W[2] - d[2] * W[1],
         d[2] * W[0] - d[0] * W[2],
         d[0] * W[1] - d[1] * W[0])
    X = sum(x * x for x in C)
    return H, X


def q3(h, x):
    return h > 0 and 3 * h * h > 4 * x


def q4(h, x):
    return h > 0 and h * h > 2 * x


def corners(low, high):
    return [tuple(high[i] if bits[i] else low[i] for i in range(3))
            for bits in product((0, 1), repeat=3)]


a, b = (3, 10, 10), (17, 10, 10)
yz = ((16, 10), (7, 15), (7, 5))
offsets = ((0, 0, 0), (1, 2, -3), (2, 3, -5), (3, 1, -4))
groups = [tuple((10 + o, *yz[i]) for i, o in enumerate(row))
          for row in offsets]
guards = [g for group in groups for g in group]
assert len(set((a, b, *guards))) == 14
assert [score(a, b, group) for group in groups] == [
    (172, 0), (116, 0), (20, 0), (68, 0)]
assert all(q3(*score(a, b, group)) and q4(*score(a, b, group))
           for group in groups)
pairs = list(combinations(guards, 2))
assert len(pairs) == 66
assert all(not q3(*score(a, b, pair)) for pair in pairs)
assert all(not q4(*score(a, b, pair)) for pair in pairs)
assert max(score(a, b, pair)[0] for pair in pairs) == 120
assert min(score(a, b, pair)[1] for pair in pairs) == 26656

A = ((900, 1000, 1000), (901, 1001, 1001))
B = ((1100, 1000, 1000), (1101, 1001, 1001))
aa = corners(A[0], A[1])
bb = corners(B[0], B[1])
assert len(set(aa)) == len(set(bb)) == 8
delta = (-2, -1, 1, 2)
large_groups = [
    ((1000 + 10 * q, 1080, 1000),
     (1000 + 10 * q, 960, 1070),
     (1000 + 10 * q, 960, 930))
    for q in delta
]
large_guards = [g for group in large_groups for g in group]
assert len(set((*A, *B, *large_guards))) == 16
tested = [score(x, y, group)
          for group in large_groups for x in aa for y in bb]
assert min(h for h, _ in tested) == 36136
assert min(3 * h * h - 4 * x for h, x in tested) == 3906026400
assert min(h * h - 2 * x for h, x in tested) == 1300107952
assert all(q3(h, x) and q4(h, x) for h, x in tested)
all_candidates = (*A, *B, *large_guards)
assert sum(all(q3(*score(x, y, pair)) for x in aa for y in bb)
           for pair in combinations(all_candidates, 2)) == 0
print("PASS 4 disjoint triples, 66 failed point pairs, 64-corner rectangle fixture, 120 failed box pairs")
