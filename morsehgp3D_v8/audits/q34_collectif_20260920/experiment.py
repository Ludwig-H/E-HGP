#!/usr/bin/env python3
"""Bounded mathematical comparison; no product timings or inherited qualification."""
import hashlib
import heapq
import json
from pathlib import Path
import struct

from chord_bound_gate import bounds, forms
from collective_model import minimum_depth

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]


def distance(a, b):
    return sum((u-v)**2 for u, v in zip(a, b))


def cover_and_seeds(points, ai, bi):
    a, b = points[ai], points[bi]
    d = distance(a, b)
    cover = [i for i, z in enumerate(points)
             if sum((2*z[t]-a[t]-b[t])**2 for t in range(3)) <= 4*d]
    seeds = []
    for i, x in enumerate(points):
        if i in (ai, bi):
            continue
        e, f = distance(a, x), distance(b, x)
        owner = min([(d, tuple(sorted((ai, bi)))), (e, tuple(sorted((ai, i)))),
                     (f, tuple(sorted((bi, i))))], key=lambda p: (-p[0], p[1]))[1]
        if d+e > f and d+f > e and e+f > d and owner == tuple(sorted((ai, bi))):
            seeds.append(i)
    return cover, seeds


def evaluate(points, edge, k, name):
    ai, bi = edge
    a, b = points[ai], points[bi]
    cover, seeds = cover_and_seeds(points, ai, bi)
    # One bounded heap per edge, not per face. Midpoint proximity is only a proposal policy.
    pool = heapq.nsmallest(4*k, (i for i in cover if i not in edge),
                          key=lambda i: (sum((2*points[i][t]-a[t]-b[t])**2 for t in range(3)), i))
    methods = {name: {"q4_rejected": 0, "both_rejected": 0, "residual_scan_upper_bound": 0}
               for name in ["old_universal", "new_universal", "old_collective", "new_collective"]}
    q3_rejected = 0
    distinct_bounds = 0
    for i in seeds:
        uold, unew = bounds(a, b, points[i])
        if unew > uold:
            raise RuntimeError("new bound is not tighter")
        distinct_bounds += unew < uold
        values = [forms(a, b, points[i], points[j]) for j in pool]
        r3 = sum(p < 0 for p, _ in values) >= k-1
        q3_rejected += r3
        scores = {"old_universal": sum(p+uold*abs(b) < 0 for p, b in values),
                  "new_universal": sum(p+unew*abs(b) < 0 for p, b in values),
                  "old_collective": minimum_depth(values, uold),
                  "new_collective": minimum_depth(values, unew)}
        if scores["old_collective"] < scores["old_universal"] or scores["new_collective"] < scores["new_universal"]:
            raise RuntimeError("collective certificate lost universal witnesses")
        if scores["new_collective"] < scores["old_collective"]:
            raise RuntimeError("narrower domain decreased its minimum")
        for mode, score in scores.items():
            r4 = score >= k-2
            methods[mode]["q4_rejected"] += r4
            methods[mode]["both_rejected"] += r3 and r4
            # Worst-case one full covered scan whenever either lane remains.
            # Actual q3-only early exit and q4 work are not simulated or claimed.
            methods[mode]["residual_scan_upper_bound"] += 0 if r3 and r4 else len(cover)
    return {"case": name, "n": len(points), "edge_ids": edge, "kmax": k,
            "covered": len(cover), "seeds": len(seeds), "pool_size": len(pool),
            "cover_point_checks": len(points), "seed_point_checks": len(points)-2,
            "pool_distance_keys": max(0, len(cover)-2),
            "pool_forms": len(pool)*len(seeds), "q3_rejected": q3_rejected,
            "strictly_tighter_integer_bounds": distinct_bounds,
            "baseline_scan": len(cover)*len(seeds), "methods": methods,
            "cost_not_measured": "heap comparisons, pool sorting, family sort/positivity/callback; no timing claim"}


def adversarial(n):
    points = [(900, 1000, 1000), (1100, 1000, 1000)]
    for i in range(n-2):
        y, offset = 1120 + (i//2)//16, 40+(i//2)%16
        points.append((1000, y, 1000 + (offset if i%2 == 0 else -offset)))
    return points


def main():
    results, inputs = [], {}
    for n in (32, 64, 128, 256):
        for k in (5, 10):
            results.append(evaluate(adversarial(n), (0, 1), k, "constructor_adversarial"))
    prepared = ROOT / "morsehgp3D_v8/audits/lidar08_20260914/prepared"
    for scan in (0, 100, 200):
        folder = prepared / f"single_{scan:06d}"
        file = folder / "n8000.u16le"
        raw = file.read_bytes()
        actual = hashlib.sha256(raw).hexdigest()
        expected = next(x["sha256"] for x in json.loads((folder/"METADATA.json").read_text())["samples"] if x["n"] == 8000)
        if actual != expected or len(raw) != 48000:
            raise RuntimeError("LiDAR preparation changed")
        inputs[str(file.relative_to(ROOT))] = actual
        points = list(struct.iter_unpack("<HHH", raw))
        nearest = sorted(range(1, len(points)), key=lambda j: (distance(points[0], points[j]), j))
        # Three explicitly supplied edges; not a WSPD sample or a global generator.
        for rank in (8, 32, 128):
            results.append(evaluate(points, (0, nearest[rank-1]), 10, f"lidar{scan:06d}_neighbor{rank}"))
    print(json.dumps({"status": "completed", "scope": "bounded mathematical model, no product port",
                      "source_base": "77f659e4", "input_sha256": inputs, "results": results},
                     indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
