"""Scalar index regression and exact block certificate; no engine/GCP call."""
from fractions import Fraction as F
from itertools import product
import hashlib
import json
from pathlib import Path


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def last_block(length, old):
    # All sampled levels satisfy the query: lo = ceil(L/64).
    lo = (length + 63) // 64
    a = (lo - 1) * 64 + 1
    b = min((lo * 64) & 0xffffffff, length) if old else min(lo * 64, length)
    steps = 0
    while a < b and steps < 200:
        mid = ((a + b) & 0xffffffff) // 2 if old else a + (b - a) // 2
        a = mid + 1  # Query includes every level in this final block.
        steps += 1
    return {"result": a, "steps": steps, "terminated": a >= b,
            "initial_upper": min((lo * 64) & 0xffffffff, length) if old else min(lo * 64, length)}


lengths = [1, 63, 64, 65, 127, 128, 2**31 - 64, 2**31,
           2**31 + 64, 2**32 - 65, 2**32 - 64, 2**32 - 2]
ranks = []
for length in lengths:
    old, fixed = last_block(length, True), last_block(length, False)
    require(fixed["terminated"] and fixed["result"] == length, "safe scalar rank wrong")
    ranks.append({"length": length, "old": old, "fixed": fixed})
require(not ranks[8]["old"]["terminated"], "midpoint overflow fixture disappeared")
require(ranks[-1]["old"]["initial_upper"] == 0 and
        ranks[-1]["old"]["result"] == (2**32 - 2) - 61,
        "product overflow fixture disappeared")


def distance(a, b):
    return sum((x - y)**2 for x, y in zip(a, b))


def gap_squared(q, z):
    return sum(max(F(0), a - d, c - b)**2
               for (a, b), (c, d) in zip(q, z))


def hull(points):
    return tuple((min(p[a] for p in points), max(p[a] for p in points)) for a in range(3))


def samples(q):
    points = list(product(*[(a, b) if a != b else (a,) for a, b in q]))
    points.extend(tuple(a + t * (b - a) for a, b in q) for t in (F(1, 3), F(1, 2), F(2, 3)))
    return sorted(set(points))


clouds = [
    ("grid", list(product((0, 2, 5), repeat=3))),
    ("shell", [(10 + x, 10 + y, 10 + z) for x, y, z in
               ((-1, 0, 0), (1, 0, 0), (0, -1, 0), (0, 1, 0), (0, 0, -1), (0, 0, 1))]),
    ("two_groups", [(i % 4, i // 4, (i * 3) % 5) for i in range(8)] +
                   [(300 + i % 4, 300 + i // 4, 300 + (i * 3) % 5) for i in range(8)]),
    ("line", [(i * i, 0, 0) for i in range(12)]),
    ("sensor_like", [(i * i + 3, (i * 17) % 31, (i * 7) % 13) for i in range(20)]),
]
counters = {"configs": 0, "block_tests": 0, "rejected_blocks": 0,
            "center_checks": 0, "retained_tie_neighbors": 0}
for name, cloud in clouds:
    require(len(cloud) == len(set(cloud)), "fixture has duplicate sites")
    boxes = [tuple((F(a), F(b)) for a, b in hull(cloud)),
             ((F(0), F(1)),) * 3, ((F(10), F(11)),) * 3]
    for k in (1, 2, 3, 5):
        if k > len(cloud):
            continue
        for q in boxes:
            corners = list(product(*[(a, b) for a, b in q]))
            for mode in ("first_k", "near_midpoint"):
                middle = tuple((a + b) / 2 for a, b in q)
                witnesses = cloud[:k] if mode == "first_k" else sorted(cloud, key=lambda p: (distance(p, middle), p))[:k]
                radius = max(distance(p, v) for p in witnesses for v in corners)
                retained = set()
                for start in range(0, len(cloud), 4):
                    ids = list(range(start, min(start + 4, len(cloud))))
                    z = hull([cloud[i] for i in ids])
                    reject = gap_squared(q, z) > radius
                    counters["block_tests"] += 1
                    counters["rejected_blocks"] += int(reject)
                    if not reject:
                        retained.update(ids)
                for center in samples(q):
                    distances = [distance(p, center) for p in cloud]
                    kth = sorted(distances)[k - 1]
                    nearest_closed = {i for i, d in enumerate(distances) if d <= kth}
                    require(kth <= radius, "witness radius not upper bound")
                    require(nearest_closed <= retained, "block pruning lost closed neighbor")
                    counters["center_checks"] += 1
                    counters["retained_tie_neighbors"] += sum(distances[i] == kth for i in nearest_closed)
                counters["configs"] += 1

# A tied site must not be removed. A >= rejection is unsafe even for K1.
q = ((F(1), F(1)), (F(0), F(0)), (F(0), F(0)))
z = ((F(2), F(2)), (F(0), F(0)), (F(0), F(0)))
radius = distance((0, 0, 0), (1, 0, 0))
gap = gap_squared(q, z)
require(gap == radius and not gap > radius and gap >= radius,
        "closed equality control failed")
require(counters["rejected_blocks"] > 0, "no positive block rejection exercised")
print(json.dumps({"status": "PASS", "scope": "exact scalar proof checks only, not producer scaling",
                  "engine_calls": 0, "GCP_used": False, "rank_cases": ranks,
                  "block_checks": counters,
                  "closed_equality": {"radius_squared": str(radius), "gap_squared": str(gap),
                                      "strict_reject": gap > radius, "unsafe_ge_reject": gap >= radius},
                  "source_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}, sort_keys=True, indent=2))
