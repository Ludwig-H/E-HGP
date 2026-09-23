#!/usr/bin/env python3
"""Static reader and independent exact check for this audit-only capture."""

from collections import defaultdict
from hashlib import sha256
from itertools import product
import json
from math import isqrt
from pathlib import Path

if not __debug__:
    raise RuntimeError("this verifier requires assertions enabled")

HERE = Path(__file__).resolve().parent
receipt = json.loads((HERE / "RECEIPT.json").read_text())


def digest(path):
    return sha256(path.read_bytes()).hexdigest()


assert digest(HERE / "measure.cpp") == receipt["source_sha256"]
assert digest(HERE / "test_incidence.py") == receipt["fixture_sha256"]
for name, expected in receipt["outputs_sha256"].items():
    assert digest(HERE / name) == expected, name


def read(name):
    lines = (HERE / name).read_text().splitlines()
    summary = {}
    for line in lines:
        parts = line.split()
        if parts[0] in ("config", "head", "shadow"):
            assert parts[0] not in summary
            summary[parts[0]] = list(map(int, parts[1:]))
    assert len(summary) == 3
    return lines, summary


full_lines, full = read("full_near64.stdout")
_, sparse = read("sparse_near64.stdout")
assert full["config"] == [64, 1, 1] and sparse["config"] == [64, 1, 0]
assert full["head"] == [123389, 6175011, 238364135, 2548453, 22034426,
                        503488729, 3986433, 559661741, 3502180, 3665711,
                        351867, 2196586, 271207, 382641]
assert sparse["head"] == [7718, 290309, 1365422, 121258, 287095,
                          13770152, 169900, 2229643, 157378, 149400,
                          11016, 110242, 1775, 2289]
assert full["shadow"][:28] == [11174, 396481, 478635662, 38, 1393, 600315,
                                29537, 59855, 0, 21904104, 4560447, 3359017,
                                9602945, 1181588, 19842, 68877, 422826,
                                2738013, 117, 0, 3128, 430707, 396481, 0,
                                44, 1375, 18, 0]
assert sparse["shadow"][:28] == [101, 2343, 160510, 0, 0, 0,
                                  278, 530, 0, 129344, 39719, 31348,
                                  75221, 7906, 465, 693, 1909,
                                  16168, 3, 0, 12, 2564, 2343, 0,
                                  0, 0, 0, 0]
assert full["shadow"][3] == 38 and full["shadow"][24] == 44
assert full["shadow"][25] + full["shadow"][26] == full["shadow"][4]
assert full["shadow"][17] + full["shadow"][20] + full["shadow"][21] == 8 * full["shadow"][1]
assert sparse["shadow"][17] + sparse["shadow"][20] + sparse["shadow"][21] == 8 * sparse["shadow"][1]

points = {int(k): tuple(v) for k, v in receipt["proof_points"].items()}
cloud_low = receipt["cloud_box"]["low"]
cloud_high = receipt["cloud_box"]["high"]


def may_meet(a, b, low, high):
    d = [y - x for x, y in zip(a, b)]
    D = sum(x * x for x in d)
    M = [2 * (x + y) for x, y in zip(a, b)]
    gap = [max(l - m, 0, m - h) for l, m, h in zip(low, M, high)]
    if any(g * g > 2 * (D - di * di) for g, di in zip(gap, d)):
        return False
    if sum(g * g for g in gap) > 2 * D:
        return False
    pmin = sum(di * ((l if di >= 0 else h) - m)
               for di, l, h, m in zip(d, low, high, M))
    pmax = sum(di * ((h if di >= 0 else l) - m)
               for di, l, h, m in zip(d, low, high, M))
    return pmin <= 0 <= pmax


def dist2(p, v):
    return sum((4 * x - y) ** 2 for x, y in zip(p, v))


anomalies = {}
proofs = defaultdict(dict)
for line in full_lines:
    p = line.split()
    if p[0] == "anomaly":
        assert len(p) == 7
        key = int(p[1]), int(p[2])
        assert key not in anomalies
        anomalies[key] = tuple(map(int, p[3:]))
    elif p[0] == "proof":
        assert len(p) == 14
        key = int(p[1]), int(p[2])
        cell = int(p[3])
        assert 0 <= cell < 8 and cell not in proofs[key]
        proofs[key][cell] = (tuple(map(int, p[4:7])),
                             tuple(map(int, p[7:10])),
                             tuple(map(int, p[10:14])))
assert len(anomalies) == 18 and set(proofs) == set(anomalies)

for (aid, bid), (before, after, _F, flags) in anomalies.items():
    assert before in (2, 4, 6) and after in (2, 4, 6) and after & ~before == 0
    a, b = points[aid], points[bid]
    assert a != b
    axes = []
    for axis in range(3):
        boundaries = {side[axis] for low, high, _ in proofs[aid, bid].values()
                      for side in (low, high)}
        assert len(boundaries) == 3
        axes.append(sorted(boundaries))
    D = sum((x - y) ** 2 for x, y in zip(a, b))
    r = isqrt((D + 7) // 8)
    while 8 * r * r < D:
        r += 1
    for axis in range(3):
        M = 2 * (a[axis] + b[axis])
        assert axes[axis][0] <= max(M - 4 * r, 4 * cloud_low[axis])
        assert axes[axis][2] >= min(M + 4 * r, 4 * cloud_high[axis])
    actual_flags = 0
    for cell in range(8):
        ix, iy, iz = (cell >> 2) & 1, (cell >> 1) & 1, cell & 1
        idx = (ix, iy, iz)
        low = tuple(axes[axis][idx[axis]] for axis in range(3))
        high = tuple(axes[axis][idx[axis] + 1] for axis in range(3))
        if not may_meet(a, b, low, high):
            assert cell not in proofs[aid, bid]
            continue
        actual_flags |= 1 << cell
        proof_low, proof_high, guards = proofs[aid, bid][cell]
        assert (low, high) == (proof_low, proof_high)
        assert len(set(guards)) == 4 and aid not in guards and bid not in guards
        for v in product(*((l, h) for l, h in zip(low, high))):
            q = dist2(a, v) + dist2(b, v)
            for gid in guards:
                assert 2 * dist2(points[gid], v) < q, (aid, bid, cell, gid)
    assert flags == actual_flags != 0

print("PASS: two pinned S2 joins/counters; 18 post-core-open edges have exact incidence and four strict guards on every relevant cell")
