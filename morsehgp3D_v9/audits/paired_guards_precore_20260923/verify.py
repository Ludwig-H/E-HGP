#!/usr/bin/env python3
"""Static exact reader for successful paired-guard proofs in RESULT.json."""

import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
NAME = sys.argv[1] if len(sys.argv) == 2 else "RESULT.json"
EXPECTED = {
    "RESULT.json": (230923, 260032, 37, 37, [20, 22, 27], [82118, 94938, 111883], [8, 10, 12]),
    "RESULT_SEED2.json": (230924, 260599, 45, 44, [32, 34, 40], [124221, 137364, 168845], [20, 22, 27]),
}
if NAME not in EXPECTED:
    raise RuntimeError("unknown pinned result name")
SEED, SAMPLE_F, CROSS, X_CROSS, CLOSURES, CLOSED_F, CROSS_CLOSED = EXPECTED[NAME]
data = json.loads((HERE / NAME).read_text())
provenance = json.loads((HERE / "PROVENANCE.json").read_text())


def require(condition, reason):
    if not condition:
        raise RuntimeError(reason)


def pair_metrics(a, b, g, h):
    d = tuple(b[i] - a[i] for i in range(3))
    D = sum(x * x for x in d)
    s = tuple(a[i] + b[i] for i in range(3))
    wg = tuple(2 * g[i] - s[i] for i in range(3))
    wh = tuple(2 * h[i] - s[i] for i in range(3))
    H = 2 * D - sum(x * x for x in wg) - sum(x * x for x in wh)
    t = tuple(wg[i] + wh[i] for i in range(3))
    X = sum((d[(i + 1) % 3] * t[(i + 2) % 3] -
             d[(i + 2) % 3] * t[(i + 1) % 3]) ** 2 for i in range(3))
    return H, X


require(data["schema"] == "mhgp9_audit_paired_guard_shadow_v1", "schema")
require(provenance["schema"] == "mhgp9_audit_paired_guard_provenance_v1", "provenance")
require(data["band_populations"] == [20252, 41146, 29869], "band sizes")
require(data["sample_seed"] == SEED and len(data["rows"]) == 60, "sample")
require(data["cloud_sites"] == 123389, "cloud count")
require(data["oracle_point_examinations"] == 60 * data["cloud_sites"], "scan count")
require(data["sample_F"] == sum(row["F"] for row in data["rows"]) == SAMPLE_F, "sample F")
require(data["sector_cross_edges"] == sum(row["sector_a"] != row["sector_b"]
                                          for row in data["rows"]) == CROSS, "sector cross")
require(data["x_cross_edges"] == sum((row["sector_a"] >> 1) != (row["sector_b"] >> 1)
                                     for row in data["rows"]) == X_CROSS, "x cross")
require((HERE / "fixture.stdout").read_text() ==
        "PASS surviving=6 q3_credits=0 q4_credits=0 disjoint_pairs=4 q3_closed=1 q4_closed=1\n",
        "product S2 fixture output")

points = {int(k): tuple(v) for k, v in data["proof_points"].items()}
for rid, p in points.items():
    require(0 <= rid < 1 << 32 and len(p) == 3 and
            all(0 <= x <= 262143 for x in p), "proof point out of u18 domain")

seen = set()
summary = {str(B): [{"edges": 0, "F": 0, "q3": 0, "q4": 0} for _ in range(3)]
           for B in (4, 8, 16)}
proofs_checked = 0
cross_closed = {str(B): 0 for B in (4, 8, 16)}
for ordinal, row in enumerate(data["rows"]):
    aid, bid, F, mask, band = (row[k] for k in ("a", "b", "F", "mask", "band"))
    require(aid < bid and (aid, bid) not in seen, "duplicate or unoriented edge")
    seen.add((aid, bid))
    require(mask in (2, 4, 6) and band in (0, 1, 2), "mask/band")
    require((1000 <= F < 3000) if band == 0 else
            (3000 <= F < 6000) if band == 1 else F >= 6000, "F band")
    require(band == ordinal // 20, "reservoir ordering")
    require(aid in points and bid in points and points[aid] != points[bid], "edge points")
    require(0 <= row["sector_a"] < 4 and 0 <= row["sector_b"] < 4, "sector")
    require(len(row["quadrant_populations"]) == 4 and
            all(0 <= x <= 16 for x in row["quadrant_populations"]), "palette bounds")
    a, b = points[aid], points[bid]
    for budget in (4, 8, 16):
        result = row["results"][str(budget)]
        q3_count, q4_count = result["q3_pairs"], result["q4_pairs"]
        require(q3_count <= 2 * budget and q4_count <= 2 * budget,
                "too many disjoint pairs")
        require((mask & 2) != 0 or q3_count == 0, "inactive q3 count")
        require((mask & 4) != 0 or q4_count == 0, "inactive q4 count")
        closed3 = not (mask & 2) or q3_count >= 4
        closed4 = not (mask & 4) or q4_count >= 3
        closed = bool(closed3 and closed4)
        require(result["closed"] == closed, "invalid closure predicate")
        acc = summary[str(budget)][band]
        acc["q3"] += bool(closed3)
        acc["q4"] += bool(closed4)
        acc["edges"] += closed
        acc["F"] += F if closed else 0
        cross_closed[str(budget)] += bool(closed and row["sector_a"] != row["sector_b"])
        if not closed:
            require("proof" not in result, "proof emitted for unclosed edge")
            continue
        proof = result["proof"]
        for lane, bit, target in ((3, 2, 4), (4, 4, 3)):
            pairs = proof[f"q{lane}"]
            require(len(pairs) == (target if mask & bit else 0), "pair threshold")
            guards = set()
            for gid, hid, observed_H, observed_X in pairs:
                require(gid != hid and gid not in guards and hid not in guards,
                        "guard reused within lane")
                guards.update((gid, hid))
                require(gid not in (aid, bid) and hid not in (aid, bid), "endpoint guard")
                require(gid in points and hid in points, "missing guard coordinate")
                H, X = pair_metrics(a, b, points[gid], points[hid])
                require(H == observed_H and X == observed_X, "pair metric mismatch")
                require(H > 0 and (3 * H * H > 4 * X if lane == 3 else H * H > 2 * X),
                        "pair does not strictly certify lane")
                proofs_checked += 1

require(summary == data["summary"], "aggregate mismatch")
require([sum(s["edges"] for s in summary[str(B)]) for B in (4, 8, 16)] ==
        CLOSURES, "published closures")
require([sum(s["F"] for s in summary[str(B)]) for B in (4, 8, 16)] ==
        CLOSED_F, "published sampled F")
require([cross_closed[str(B)] for B in (4, 8, 16)] == CROSS_CLOSED,
        "published closed cross-sector edges")
print(f"PASS {NAME}: 60 pinned sampled edges, {proofs_checked} exact strict "
      f"disjoint-pair checks, B4/B8/B16 closures {CLOSURES}; "
      "failure counts require LIVE replay")
