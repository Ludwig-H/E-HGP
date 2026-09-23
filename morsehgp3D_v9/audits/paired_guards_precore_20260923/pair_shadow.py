#!/usr/bin/env python3
"""Audit-only exact paired-guard shadow on pinned S2 survivors.

This intentionally scans the WHOLE cloud once per sampled edge to pick an
oracle palette. It measures geometric potential, not a viable implementation.
"""

import argparse
from collections import Counter
from hashlib import sha256
import heapq
import json
from pathlib import Path
import random
import struct

HERE = Path(__file__).resolve().parent
PROVENANCE = json.loads((HERE / "PROVENANCE.json").read_text())
EXPECTED = PROVENANCE["input_sha256"]
BANDS = ((1000, 3000), (3000, 6000), (6000, None))
BUDGETS = (4, 8, 16)
QUARTERS = ("quarter_x_neg_y_neg", "quarter_x_neg_y_nonneg",
            "quarter_x_nonneg_y_neg", "quarter_x_nonneg_y_nonneg")


def require(condition, reason):
    if not condition:
        raise RuntimeError(reason)


def digest(path):
    return sha256(path.read_bytes()).hexdigest()


def load_cloud(inputs):
    p = inputs / "s00_full_full.u32le"
    q = inputs / "s00_full_full.raw_return_ids.u32le"
    require(digest(p) == EXPECTED["points"], "points SHA mismatch")
    require(digest(q) == EXPECTED["raw_ids"], "raw ID SHA mismatch")
    points = list(struct.iter_unpack("<III", p.read_bytes()))
    ids = [x[0] for x in struct.iter_unpack("<I", q.read_bytes())]
    require(len(points) == len(ids) == 123389 and len(set(ids)) == len(ids), "cloud identity")
    return points, ids, {rid: i for i, rid in enumerate(ids)}


def load_sectors(inputs, ids):
    which = {}
    for sector, name in enumerate(QUARTERS):
        path = inputs / f"s00_full_{name}.raw_return_ids.u32le"
        require(digest(path) == PROVENANCE["sector_raw_ids_sha256"][name],
                "physical quarter SHA mismatch")
        for (rid,) in struct.iter_unpack("<I", path.read_bytes()):
            require(rid not in which, "overlapping physical quarters")
            which[rid] = sector
    require(set(which) == set(ids), "physical quarters do not partition input")
    return which


def select_edges(trace, seed):
    rng = random.Random(seed)
    seen = [0, 0, 0]
    sample = [[], [], []]
    files = sorted(trace.glob("part_*.bin"))
    require(len(files) == 8, "expected eight S2 trace parts")
    for part, path in enumerate(files):
        require(digest(path) == EXPECTED["trace_parts"][part], "trace SHA mismatch")
        data = path.read_bytes()
        require(len(data) % 16 == 0, "truncated S2 record")
        for a, b, F, mask in struct.iter_unpack("<IIII", data):
            require(a < b and F >= 2 and mask in (2, 4, 6), "invalid S2 record")
            if F < 1000:
                continue
            band = 0 if F < 3000 else 1 if F < 6000 else 2
            seen[band] += 1
            row = {"a": a, "b": b, "F": F, "mask": mask, "band": band}
            if len(sample[band]) < 20:
                sample[band].append(row)
            else:
                j = rng.randrange(seen[band])
                if j < 20:
                    sample[band][j] = row
    require(all(len(x) == 20 for x in sample), "underfilled reservoir")
    return seen, [row for group in sample for row in group]


def basis(d):
    main = max(range(3), key=lambda i: abs(d[i]))
    axis_a, axis_b = (main + 1) % 3, (main + 2) % 3
    h = abs(d[main])
    sign = 1 if d[main] > 0 else -1
    A = [0, 0, 0]
    B = [0, 0, 0]
    A[axis_a], A[main] = h, -sign * d[axis_a]
    B[axis_b], B[main] = h, -sign * d[axis_b]
    return A, B


def palette(points, raw_ids, a_id, b_id):
    a, b = points[a_id], points[b_id]
    d = tuple(b[i] - a[i] for i in range(3))
    D = sum(x * x for x in d)
    require(D > 0, "zero-length edge")
    midpoint2 = tuple(a[i] + b[i] for i in range(3))
    A, B = basis(d)
    bins = [[], [], [], []
    ]  # best 16 per sign quadrant of the edge's perpendicular plane
    core = 2
    for site, p in enumerate(points):
        if site in (a_id, b_id):
            continue
        w = tuple(2 * p[i] - midpoint2[i] for i in range(3))
        norm = sum(x * x for x in w)
        if norm > D:
            continue
        core += 1
        u = sum(w[i] * A[i] for i in range(3))
        v = sum(w[i] * B[i] for i in range(3))
        quadrant = (int(u >= 0) << 1) | int(v >= 0)
        # A min-heap with the worst candidate first. Ties prefer the
        # smaller raw site ID, independent of index rank or worker order.
        item = (-norm, -raw_ids[site], site)
        bucket = bins[quadrant]
        if len(bucket) < 16:
            heapq.heappush(bucket, item)
        elif item > bucket[0]:
            heapq.heapreplace(bucket, item)
    selected = []
    for bucket in bins:
        selected.append([site for _, _, site in sorted(bucket, reverse=True)])
    return D, d, core, selected


def valid_pair(a, b, g, h, lane):
    d = tuple(b[i] - a[i] for i in range(3))
    D = sum(x * x for x in d)
    s = tuple(a[i] + b[i] for i in range(3))
    wg = tuple(2 * g[i] - s[i] for i in range(3))
    wh = tuple(2 * h[i] - s[i] for i in range(3))
    H = 2 * D - sum(x * x for x in wg) - sum(x * x for x in wh)
    t = tuple(wg[i] + wh[i] for i in range(3))
    X = sum((d[(i + 1) % 3] * t[(i + 2) % 3] -
             d[(i + 2) % 3] * t[(i + 1) % 3]) ** 2 for i in range(3))
    margin = 3 * H * H - 4 * X if lane == 3 else H * H - 2 * X
    return H > 0 and margin > 0, H, X, margin


def greedy_pairs(points, raw_ids, a, b, candidates, lane):
    edges = []
    for i in range(len(candidates)):
        for j in range(i + 1, len(candidates)):
            gi, hj = candidates[i], candidates[j]
            okay, H, X, margin = valid_pair(points[a], points[b], points[gi], points[hj], lane)
            if okay:
                lo, hi = sorted((raw_ids[gi], raw_ids[hj]))
                edges.append((-margin, lo, hi, gi, hj, H, X))
    edges.sort()
    used = set()
    chosen = []
    for _, lo, hi, gi, hj, H, X in edges:
        if gi in used or hj in used:
            continue
        used.update((gi, hj))
        chosen.append([lo, hi, H, X])
    return chosen, len(edges)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--inputs", type=Path, required=True)
    parser.add_argument("--trace", type=Path)
    parser.add_argument("--sample", type=Path,
                        help="saved RESULT.json sample, when trace parts are unavailable")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--seed", type=int, default=230923,
                        help="reservoir seed; used only with --trace")
    args = parser.parse_args()
    require((args.trace is None) != (args.sample is None), "provide trace OR saved sample")
    points, raw_ids, where = load_cloud(args.inputs)
    sectors = load_sectors(args.inputs, raw_ids)
    if args.trace is not None:
        seen, rows = select_edges(args.trace, args.seed)
    else:
        saved = json.loads(args.sample.read_text())
        seen = saved["band_populations"]
        rows = [{k: row[k] for k in ("a", "b", "F", "mask", "band")}
                for row in saved["rows"]]
    require(len(rows) == 60, "expected 60 sampled edges")
    summary = {str(B): [{"edges": 0, "F": 0, "q3": 0, "q4": 0} for _ in BANDS]
               for B in BUDGETS}
    proof_points = {}
    for row in rows:
        a, b = where[row["a"]], where[row["b"]]
        D, d, core, bins = palette(points, raw_ids, a, b)
        require(core == row["F"], "independent diametral core mismatch")
        row["sector_a"], row["sector_b"] = sectors[row["a"]], sectors[row["b"]]
        row["quadrant_populations"] = [len(bucket) for bucket in bins]
        row["results"] = {}
        for B in BUDGETS:
            candidates = [site for bucket in bins for site in bucket[:B]]
            q3, n3 = greedy_pairs(points, raw_ids, a, b, candidates, 3) if row["mask"] & 2 else ([], 0)
            q4, n4 = greedy_pairs(points, raw_ids, a, b, candidates, 4) if row["mask"] & 4 else ([], 0)
            closed3 = not (row["mask"] & 2) or len(q3) >= 4
            closed4 = not (row["mask"] & 4) or len(q4) >= 3
            closed = closed3 and closed4
            acc = summary[str(B)][row["band"]]
            acc["q3"] += closed3
            acc["q4"] += closed4
            acc["edges"] += closed
            acc["F"] += row["F"] if closed else 0
            row["results"][str(B)] = {"q3_pairs": len(q3), "q4_pairs": len(q4),
                                      "q3_graph_edges": n3, "q4_graph_edges": n4,
                                      "closed": closed}
            if closed:
                proof = {"q3": q3[:4] if row["mask"] & 2 else [],
                         "q4": q4[:3] if row["mask"] & 4 else []}
                row["results"][str(B)]["proof"] = proof
                for pair in proof["q3"] + proof["q4"]:
                    for rid in pair[:2]:
                        proof_points[str(rid)] = points[where[rid]]
        for rid in (row["a"], row["b"]):
            proof_points[str(rid)] = points[where[rid]]
    payload = {
        "schema": "mhgp9_audit_paired_guard_shadow_v1",
        "context": "CPU audit only, SemanticKITTI 08/000000 raw/full 1mm/u18 K5/s8 S2 survivors; oracle full-cloud scan per edge",
        "band_populations": seen,
        "sample_seed": args.seed if args.trace is not None else saved["sample_seed"],
        "sample_rule": f"20-item reservoir per F band, sorted trace parts/records, Python Random({args.seed if args.trace is not None else saved['sample_seed']})",
        "cloud_sites": len(points),
        "oracle_point_examinations": len(points) * len(rows),
        "sample_F": sum(row["F"] for row in rows),
        "sector_cross_edges": sum(row["sector_a"] != row["sector_b"] for row in rows),
        "x_cross_edges": sum((row["sector_a"] >> 1) != (row["sector_b"] >> 1) for row in rows),
        "summary": summary,
        "rows": rows,
        "proof_points": proof_points,
    }
    require(not args.output.exists(), "refusing to overwrite audit output")
    args.output.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n")
    print(json.dumps({k: payload[k] for k in ("band_populations", "sample_F",
                                                 "sector_cross_edges", "x_cross_edges",
                                                 "oracle_point_examinations", "summary")}, sort_keys=True))


if __name__ == "__main__":
    main()
