"""Exact-integer model of the public fixed-frontier batching path; not native."""
import hashlib
import json
from itertools import product
from math import comb
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CHECKS = 0


def require(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(message)


source = json.loads((ROOT / "SOURCE_BEFORE.json").read_text())
require(source["source_commit"] == "77db5738eb2dd5bc84ecdc4d85ade833124c58f8", "source pin")
for rel, record in sorted(source["files"].items()):
    data = (ROOT / rel).read_bytes()
    require(hashlib.sha256(data).hexdigest() == record["sha256"], "source hash " + rel)

raw_points = list(product((0, 16), repeat=3)) + [(8, 8, 8)]


def morton(p):
    # cloud/morton.hpp: x bit i -> 3i, y -> 3i+1, z -> 3i+2.
    return sum(((p[a] >> i) & 1) << (3 * i + a) for i in range(21) for a in range(3))


points = sorted(raw_points, key=morton)
center_site = points.index((8, 8, 8))
params = dict(kmax=5, leaf_size=8, max_leaf=32, max_nodes=0,
              single_pass=True, pair_graph=True, adaptive_frontier=False,
              batch_leaves=True, cuda_leaves=False, weights=[1] * len(points))
require(len(set(points)) == 9, "distinct sites")
require(1 <= params["kmax"] <= 12, "k domain")
require(params["kmax"] + 3 <= params["leaf_size"] <= params["max_leaf"] <= 1024, "leaf domain")
require(all(0 <= v < 1 << 21 for p in points for v in p), "u21 domain")
stats = {"nodes": 0, "max_depth": 0}


def envelope(ids):
    return (tuple(min(points[i][a] for i in ids) for a in range(3)),
            tuple(max(points[i][a] for i in ids) + 1 for a in range(3)))


def prepare(ids, box, depth):
    """boxes.cpp prepare_node -> filter/reservoir -> envelope intersection."""
    stats["nodes"] += 1
    stats["max_depth"] = max(stats["max_depth"], depth)
    lo, hi = box
    # reservoir: stable nearest 3K to twice the box midpoint.
    witnesses = sorted(ids, key=lambda i: sum((2 * points[i][a] - lo[a] - hi[a]) ** 2
                                             for a in range(3)))[:3 * params["kmax"]]

    def terms(i):
        v = tuple(points[i][a] - lo[a] for a in range(3))
        return tuple(2 * (hi[a] - lo[a]) * v[a] for a in range(3)), sum(x * x for x in v)

    values = {i: terms(i) for i in ids}
    kept = []
    for i in ids:
        x, sx = values[i]
        found = 0
        for j in witnesses:
            y, sy = values[j]
            found += int(sx - sy > sum(max(0, x[a] - y[a]) for a in range(3)))
            if found == params["kmax"]:
                break
        if found < params["kmax"]:
            kept.append(i)
    if not kept:
        return None
    elo, ehi = envelope(kept)
    adjusted = (tuple(max(elo[a], lo[a]) for a in range(3)),
                tuple(min(ehi[a], hi[a]) for a in range(3)))
    if any(adjusted[0][a] >= adjusted[1][a] for a in range(3)):
        return None
    return tuple(kept), adjusted, depth


def split(ready):
    """boxes.cpp split_ready: first longest axis, integer midpoint, [lo,hi)."""
    ids, (lo, hi), depth = ready
    axis = max(range(3), key=lambda a: hi[a] - lo[a])
    width = hi[axis] - lo[axis]
    if len(ids) <= params["leaf_size"] or width <= 1:
        return None
    middle = lo[axis] + width // 2
    left_hi, right_lo = list(hi), list(lo)
    left_hi[axis], right_lo[axis] = middle, middle
    return (lo, tuple(left_hi)), (tuple(right_lo), hi)


tasks = []


def prefix(ids, box, depth):
    """frontier.cpp prefix, default cut_depth=8."""
    ready = prepare(ids, box, depth)
    if ready is None:
        return
    children = split(ready)
    if depth == 8 or children is None:
        tasks.append(ready)
        return
    for child in children:
        prefix(ready[0], child, depth + 1)


queued = []


def run_ready(ready):
    """Frontier::execute_task calls run_ready without preparing the task twice."""
    ids, box, depth = ready
    children = split(ready)
    if children is not None:
        for child in children:
            next_ready = prepare(ids, child, depth + 1)
            if next_ready is not None:
                run_ready(next_ready)
        return
    require(len(ids) <= params["max_leaf"], "wide_leaf guard")
    require(params["pair_graph"] and 1 <= len(ids) <= 32, "leaf.cpp deferred queue condition")
    queued.append({"sites": list(ids), "lo": list(box[0]), "hi": list(box[1]), "depth": depth})


prefix(tuple(range(len(points))), envelope(range(len(points))), 0)
prefix_nodes = stats["nodes"]
require(len(tasks) <= 256, "fixed frontier capacity")
for task in tasks:
    run_ready(task)
require(stats["nodes"] == 159, "node count")
require(len(queued) == 80, "queued leaf count")
require(max(len(j["sites"]) for j in queued) == 9, "max leaf size")
require(len(queued) > len(points), "guard witness")
# K-certified lists are not a partition of sites; every leaf includes the center.
require(all(center_site in j["sites"] for j in queued), "shared center witness")
for rel in ("src/catalogue/leaf_batch.cpp", "src/catalogue/leaf_batch_cuda.cu"):
    data = (ROOT / "sources" / rel).read_text()
    require("if (view.count > view.cloud_sites) return fail(Reason::catalogue_invariant);" in data,
            "new executor guard " + rel)

p = sum(comb(32, q) for q in range(1, 5))
bound = 32 * 3 * p
require(p == 41448 and bound == 3979008, "published field bound")
require(bound * ((1 << 32) - 1) < 1 << 54, "conditional 2^54 bound")
require(bound * len(queued) < (1 << 64), "counterexample needs no huge accumulator")
print(json.dumps({"schema": "ehgp.audit.gpu_batch_live.frontier_model.v1",
                  "scope": "stdlib integer source model; no native or GPU execution",
                  "checks": CHECKS, "raw_points": raw_points, "points_in_morton_order": points,
                  "center_site": center_site, "params": params,
                  "stats": {**stats, "prefix_nodes": prefix_nodes,
                            "suffix_nodes": stats["nodes"] - prefix_nodes,
                            "fixed_frontier_tasks": len(tasks), "queued_leaves": len(queued),
                            "max_leaf_sites": 9, "queued_site_entries": sum(len(j["sites"]) for j in queued)},
                  "new_guard_result_model": "catalogue_invariant",
                  "leaf_counter_bound": bound, "batch_counter_bound_for_this_case": bound * len(queued),
                  "conditional_u32_leaf_count_bound": bound * ((1 << 32) - 1),
                  "leaf_jobs_sha256": hashlib.sha256(json.dumps(queued, sort_keys=True).encode()).hexdigest(),
                  "qualification": "not_claimed"}, sort_keys=True))
