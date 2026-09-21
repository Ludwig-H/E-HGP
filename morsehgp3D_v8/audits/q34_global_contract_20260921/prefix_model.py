#!/usr/bin/env python3
"""Exact executable model of two inherited q3/q4 population-prefix states.

This is NOT a product implementation or a fast bounding algorithm. A block's
universal Boolean certificate is obtained by exhaustive enumeration. Factor
lists, invariant checks and JSON pause copies are model scaffolding. Only the
per-lane (count, fixed-tree cursor, status) state is the proposed compact object.
No witness IDs, ambiguity frontier, census credits or geometric speed claim.
"""
import hashlib
import json
from pathlib import Path
import random


def require(test, message):
    if not test:
        raise RuntimeError(message)


def bump(work, key, amount=1):
    work[key] = work.get(key, 0) + amount


def witness(a, b, z, q):
    d = [b[i] - a[i] for i in range(3)]
    u = [z[i] - a[i] for i in range(3)]
    h = sum(u[i] * (d[i] - u[i]) for i in range(3))
    cross = [d[1]*u[2]-d[2]*u[1], d[2]*u[0]-d[0]*u[2],
             d[0]*u[1]-d[1]*u[0]]
    return h > 0 and (6-q)*h*h > sum(x*x for x in cross)


def oracle(points, k):
    # Independent distance identities: 2H=D-U-V; Xi=D*U-(D+U-V)^2/4.
    def dist(a, b):
        return sum((x-y)**2 for x, y in zip(a, b))
    result = []
    for i, a in enumerate(points):
        for j in range(i+1, len(points)):
            b, mask = points[j], 0
            for q in (3, 4):
                count, d = 0, dist(a, b)
                for z in points:
                    u, v = dist(a, z), dist(b, z)
                    h2, projection2 = d-u-v, d+u-v
                    count += h2 > 0 and (6-q)*h2*h2 > 4*d*u-projection2**2
                if count < k+2-q:
                    mask |= 1 << (q-2)
            if mask:
                result.append([i, j, mask])
    return result


def tree(n):
    nodes = []
    def visit(first, last):
        at = len(nodes)
        nodes.append(None)
        left = right = -1
        if last-first > 1:
            middle = (first+last)//2
            left, right = visit(first, middle), visit(middle, last)
        nodes[at] = [first, last, left, right, len(nodes)]
        return at
    visit(0, n)
    return nodes


def initial(n, k):
    pending = []
    def partition(ids):
        if len(ids) < 2:
            return
        middle = len(ids)//2
        a, b = ids[:middle], ids[middle:]
        # status 0=searching, 1=EOF survivor, 2=rejected/inactive.
        pending.append([a, b, [[0, 0, 2 if k+2-q <= 0 else 0]
                               for q in (3, 4)], 0])
        partition(a)
        partition(b)
    partition(list(range(n)))
    return {"pending": pending, "records": [], "work": {}}


def invariant(points, nodes, task, k, work):
    a, b, lanes, _ = task
    for slot, (count, cursor, status) in enumerate(lanes):
        if status == 2:
            continue
        q = slot+3
        rank = len(points) if cursor == len(nodes) else nodes[cursor][0]
        require(0 <= count < k+2-q, "unsaturated prefix count")
        require(status != 1 or rank == len(points), "survivor before EOF")
        for i in a:
            for j in b:
                actual = sum(witness(points[i], points[j], z, q) for z in points[:rank])
                require(actual == count, "prefix is not an exact common count")
                bump(work, "prefix_pair_checks")


def step(points, nodes, k, state, mutant, checked):
    task = state["pending"].pop()
    a, b, lanes, turn = task
    work = state["work"]
    bump(work, "steps")
    if checked:
        invariant(points, nodes, task, k, work)
    if lanes[0][1] != lanes[1][1]:
        bump(work, "different_cursors")
    if any(x[2] == 1 for x in lanes) and any(x[2] == 0 for x in lanes):
        bump(work, "eof_with_other_active")
    active = [slot for slot in (turn, 1-turn) if lanes[slot][2] == 0]
    if not active:
        mask = sum(1 << (slot+1) for slot in (0, 1) if lanes[slot][2] == 1)
        state["records"].extend([i, j, mask] for i in a for j in b if mask)
        return
    slot = active[0]
    task[3] = 1-slot
    count, cursor, _ = lanes[slot]
    q, threshold = slot+3, k-1-slot
    if cursor == len(nodes):
        lanes[slot][2] = 2 if mutant == "eof_reject" else 1
        bump(work, "eof")
    else:
        first, last, left, _, escape = nodes[cursor]
        values = [witness(points[i], points[j], points[z], q)
                  for i in a for j in b for z in range(first, last)]
        bump(work, "enumerated_predicates", len(values))
        if all(values):
            lanes[slot][0] += last-first
            lanes[slot][1] = escape
            bump(work, "positive_blocks")
            bump(work, "positive_multiple_blocks", last-first > 1)
            if lanes[slot][0] >= threshold:
                lanes[slot][2] = 2
                bump(work, "saturations_q"+str(q))
        elif not any(values) or mutant == "failed_positive_excludes":
            lanes[slot][1] = escape
            bump(work, "negative_blocks")
        elif left >= 0:
            lanes[slot][1] = left
            bump(work, "z_descents")
        else:
            require(len(a)*len(b) > 1, "singleton pair/leaf must decide")
            bump(work, "product_splits")
            bump(work, "splits_with_credit", any(x[0] for x in lanes))
            bump(work, "splits_with_different_cursors", lanes[0][1] != lanes[1][1])
            bump(work, "splits_with_eof", any(x[2] == 1 for x in lanes))
            if mutant == "consume_ambiguous_leaf":
                lanes[slot][1] = escape
            factor = 0 if len(a) >= len(b) else 1
            ids = task[factor]
            for part in (ids[:len(ids)//2], ids[len(ids)//2:]):
                child = [list(a), list(b), [list(x) for x in lanes], task[3]]
                child[factor] = part
                if mutant == "double_inherited_credit":
                    for lane, data in enumerate(child[2]):
                        if data[2] == 0:
                            data[0] *= 2
                            if data[0] >= k-1-lane:
                                data[2] = 2
                if checked:
                    invariant(points, nodes, child, k, work)
                state["pending"].append(child)
            return
    if checked:
        invariant(points, nodes, task, k, work)
    state["pending"].append(task)


def run(points, k, quantum, mutant="", checked=True):
    nodes, state = tree(len(points)), initial(len(points), k)
    pauses = 0
    while state["pending"]:
        for _ in range(quantum):
            if not state["pending"]:
                break
            step(points, nodes, k, state, mutant, checked)
        if state["pending"]:
            state = json.loads(json.dumps(state))  # Ownership-transfer model.
            pauses += 1
    records = sorted(state["records"])
    require(len(records) == len({(r[0], r[1]) for r in records}), "duplicate pair")
    return records, state["work"], pauses


def fixtures():
    result = {
        "line": [(i*7+100, 103, 105) for i in range(7)],
        "triangle_contact": [(30,30,30),(36,36,30),(36,30,36),(32,34,28)],
        "tetrahedron_contact": [(30,30,30),(36,36,30),(30,36,24),(36,30,24),(32,32,32)],
        "credit_then_ambiguity": [(5,2,0),(5,6,0),(0,0,0),(0,4,0),(10,0,0),(10,4,0)],
        "u16": [(0,0,0),(65535,65535,65535),(0,65535,65535),
                (65535,0,65535),(65535,65535,0),(32767,32768,32767)],
        "cube": [(x,y,z) for x in (11,17) for y in (19,25) for z in (29,35)],
    }
    for seed in range(12):
        rng = random.Random(53000+seed)
        points = []
        while len(points) < 6+seed%5:
            p = tuple(rng.randrange(1, 32) for _ in range(3))
            if p not in points:
                points.append(p)
        result["random_"+str(seed)] = points
    return result


def main():
    totals, campaigns, mutants = {}, [], {}
    cases = fixtures()
    # These are strict contacts, not witnesses; equality must survive both lanes.
    require(not witness(*cases["triangle_contact"][:2], cases["triangle_contact"][-1], 3),
            "q3 strict contact")
    require(not witness(*cases["tetrahedron_contact"][:2], cases["tetrahedron_contact"][-1], 4),
            "q4 strict contact")
    for name, points in cases.items():
        require(len(points) == len(set(points)), "duplicate input site")
        require(all(len(p) == 3 and all(type(x) is int and 0 <= x <= 65535 for x in p)
                    for p in points), "expected u16 input")
        for k in (1, 2, 3, 5, 10):
            expected, baseline = oracle(points, k), None
            for quantum in (1, 2, 7, 1000000):
                actual, work, pauses = run(points, k, quantum)
                require(actual == expected, "oracle mismatch: "+str((name, k, quantum)))
                if baseline is None:
                    baseline = work
                    for key, value in work.items():
                        bump(totals, key, value)
                require(work == baseline, "pause changed discrete work")
                campaigns.append([name, k, quantum, len(actual), pauses])
    for key in ("product_splits", "splits_with_credit", "splits_with_different_cursors",
                "splits_with_eof", "different_cursors", "eof_with_other_active", "positive_multiple_blocks",
                "saturations_q3", "saturations_q4"):
        require(totals.get(key, 0) > 0, "vacuous branch: "+key)
    for mutant in ("eof_reject", "failed_positive_excludes", "double_inherited_credit",
                   "consume_ambiguous_leaf"):
        for name, points in cases.items():
            for k in (1, 2, 3, 5, 10):
                expected = oracle(points, k)
                actual, work, _ = run(points, k, 1000000, mutant, False)
                if actual != expected:
                    changed = sorted(set(map(tuple, actual)) ^ set(map(tuple, expected)))
                    mutants[mutant] = {"fixture": name, "K": k, "difference": changed,
                                       "steps": work["steps"]}
                    break
            if mutant in mutants:
                break
        require(mutant in mutants, "surviving mutant: "+mutant)
    result = {"status": "PASS", "kind": "exhaustive_boolean_prefix_model",
              "source_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              "fixtures": len(cases), "configurations": len(campaigns)//4,
              "executions": len(campaigns), "work_once_per_configuration": totals,
              "campaign": campaigns, "mutants": mutants,
              "limits": "No product port, fast geometric bounds, LiDAR timing or complexity gain."}
    print(json.dumps(result, sort_keys=True, separators=(",", ":")))


if __name__ == "__main__":
    main()
