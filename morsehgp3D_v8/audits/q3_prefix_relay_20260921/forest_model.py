#!/usr/bin/env python3
"""Exact prefix-to-forest relay model, independent of product source code.

The abstract trees/powers test state and storage contracts, not geometric
performance.  A separate rational u16 fixture tests two actual q3 balls.
No imported audit/product code, files or network inputs are used.
"""

from collections import Counter
from fractions import Fraction as F
from hashlib import sha256
import json
from pathlib import Path
from random import Random


class Failure(RuntimeError):
    pass


def require(value, why):
    if not value:
        raise Failure(why)


class Tree:
    def __init__(self, n, shape="balanced"):
        self.nodes = []
        self.n = n
        self._build(0, n, 0, shape)
        self.end = len(self.nodes)
        self.height = max(node["depth"] for node in self.nodes)

    def _build(self, first, last, depth, shape):
        index = len(self.nodes)
        node = {"first": first, "last": last, "depth": depth, "children": ()}
        self.nodes.append(node)
        if last - first > 1:
            split = (first + last) // 2
            if shape == "left_spine":
                split = last - 1
            elif shape == "right_spine":
                split = first + 1
            left = self._build(first, split, depth + 1, shape)
            right = self._build(split, last, depth + 1, shape)
            node["children"] = left, right
        node["escape"] = len(self.nodes)
        return index

    def rank(self, cursor):
        return self.n if cursor == self.end else self.nodes[cursor]["first"]

    def leaf(self, rank):
        return next(i for i, node in enumerate(self.nodes)
                    if node["first"] == rank and node["last"] == rank + 1)


def suffix_roots(tree, cursor):
    roots = []
    while cursor != tree.end:
        require(0 <= cursor < tree.end, "cursor outside immutable index")
        roots.append(cursor)
        cursor = tree.nodes[cursor]["escape"]
    return roots


def verify_partition(tree, cursor):
    roots = suffix_roots(tree, cursor)
    ranks = [r for node in roots for r in
             range(tree.nodes[node]["first"], tree.nodes[node]["last"])]
    require(ranks == list(range(tree.rank(cursor), tree.n)), "suffix partition")
    require(len(roots) <= tree.height + 1, "too many suffix roots")
    return roots


def finish(tree, powers, threshold, ticket, mutant=None):
    """One prepared evaluator, one reusable subtree stack, then global shell."""
    initial_count, initial_cursor = ticket
    count = 0 if mutant == "reset_count" else initial_count
    cursor = 0 if mutant == "restart_with_credit" else initial_cursor
    counts = Counter(preparations=1)
    peak = 0
    visited_ranks = []
    if count >= threshold:
        return {"accepted": False, "depth": threshold, "shell": [],
                "work": dict(counts), "peak": 0, "visited": []}

    def prepare(node_id):
        node = tree.nodes[node_id]
        values = powers[node["first"]:node["last"]]
        counts["bounds"] += 1
        return node_id, min(values), max(values)

    while cursor != tree.end and count < threshold:
        root = cursor
        # Capture successor before the independently ordered subtree traversal.
        cursor = tree.nodes[root]["escape"]
        counts["root_starts"] += 1
        stack = [prepare(root)]
        while stack and count < threshold:
            peak = max(peak, len(stack))
            require(len(stack) <= tree.height + 1, "subtree stack bound")
            node_id, minimum, maximum = stack.pop()
            node = tree.nodes[node_id]
            counts["visits"] += 1
            if minimum >= 0:
                visited_ranks.extend(range(node["first"], node["last"]))
            elif maximum < 0:
                visited_ranks.extend(range(node["first"], node["last"]))
                credit = min(threshold - count, node["last"] - node["first"])
                count += credit
                counts["new_credits"] += credit
            else:
                require(len(node["children"]) == 2, "mixed singleton power")
                left, right = [prepare(child) for child in node["children"]]
                # Same local priority policy as the existing census contract.
                if left[1] <= right[1]:
                    stack.extend((right, left))
                else:
                    stack.extend((left, right))
        counts["prepared_unvisited"] += len(stack)
        if mutant == "drop_forest_roots":
            break
    require(counts["bounds"] == counts["visits"] + counts["prepared_unvisited"],
            "prepared bound ledger")
    require(counts["preparations"] == 1, "preparation repeated for suffix roots")
    require(len(visited_ranks) == len(set(visited_ranks)), "suffix population repeated")
    if mutant != "restart_with_credit":
        require(all(r >= tree.rank(initial_cursor) for r in visited_ranks),
                "classified prefix consumed again")
    accepted = count < threshold
    shell = [r for r, power in enumerate(powers) if power == 0] if accepted else []
    if accepted:
        counts["shell_passes"] += 1
    return {"accepted": accepted, "depth": count, "shell": shell,
            "work": dict(counts), "peak": peak, "visited": visited_ranks}


def check_result(result, powers, threshold):
    exact = sum(power < 0 for power in powers)
    accepted = exact < threshold
    require(result["accepted"] == accepted and result["depth"] == min(exact, threshold),
            "relay depth/admission differs from full oracle")
    expected_shell = [i for i, power in enumerate(powers) if power == 0] if accepted else []
    require(result["shell"] == expected_shell, "global shell differs from oracle")


def caught(call, name):
    try:
        call()
    except Failure as error:
        return {"name": name, "caught": str(error)}
    raise Failure("surviving mutant: " + name)


def solve(matrix, rhs):
    rows = [[F(v) for v in row] + [F(value)] for row, value in zip(matrix, rhs)]
    for i in range(len(rhs)):
        pivot = next((j for j in range(i, len(rhs)) if rows[j][i]), None)
        require(pivot is not None, "singular real fixture")
        rows[i], rows[pivot] = rows[pivot], rows[i]
        scale = rows[i][i]
        rows[i] = [v / scale for v in rows[i]]
        for j in range(len(rhs)):
            if j != i:
                scale = rows[j][i]
                rows[j] = [u - scale * v for u, v in zip(rows[j], rows[i])]
    return tuple(row[-1] for row in rows)


def sub(a, b):
    return tuple(u - v for u, v in zip(a, b))


def dot(a, b):
    return sum(u * v for u, v in zip(a, b))


def sphere(a, b, x):
    d, e = sub(b, a), sub(x, a)
    normal = (d[1] * e[2] - d[2] * e[1], d[2] * e[0] - d[0] * e[2],
              d[0] * e[1] - d[1] * e[0])
    center = solve((tuple(2 * v for v in d), tuple(2 * v for v in e), normal),
                   (dot(b, b) - dot(a, a), dot(x, x) - dot(a, a), dot(normal, a)))
    s, t = solve(((dot(d, d), dot(d, e)), (dot(d, e), dot(e, e))),
                 (dot(sub(center, a), d), dot(sub(center, a), e)))
    require(min(1 - s - t, s, t) > 0, "nonpositive q3 fixture")
    require(dot(d, d) >= max(dot(e, e), dot(sub(x, b), sub(x, b))), "edge not maximal")
    radius = dot(sub(center, a), sub(center, a))
    return center, radius


def real_fixture():
    a, b, z = (20, 20, 20), (40, 20, 20), (30, 31, 20)
    seeds = ((30, 32, 20), (30, 37, 20))
    points = (a, b, z) + seeds
    tree = Tree(len(points))
    cursor = tree.leaf(3)
    powers, rows = [], []
    for seed in seeds:
        center, radius = sphere(a, b, seed)
        values = [dot(sub(point, center), sub(point, center)) - radius for point in points]
        require(sum(v < 0 for v in values[:3]) == 1, "common real prefix")
        powers.append(values)
        rows.append({"center": [str(v) for v in center], "radius2": str(radius),
                     "powers": [str(v) for v in values]})
    require([v < 0 for v in powers[0][:3]] == [v < 0 for v in powers[1][:3]],
            "prefix booleans must agree for all seeds")
    ticket = (1, cursor)
    accepted = []
    for threshold in (2, 3):
        for values in powers:
            result = finish(tree, values, threshold, ticket)
            check_result(result, values, threshold)
            accepted.append((result["accepted"], result["depth"], result["shell"]))
    require(accepted == [(True, 1, [0, 1, 3]), (False, 2, []),
                         (True, 1, [0, 1, 3]), (True, 2, [0, 1, 4])], "real relay fixture")
    mutants = []
    for mode in ("restart_with_credit", "reset_count"):
        mutants.append(caught(lambda mode=mode: check_result(
            finish(tree, powers[0], 2, ticket, mode), powers[0], 2), mode))

    def reused_ticket():
        first = finish(tree, powers[0], 2, ticket)
        wrongly_updated = (first["depth"], tree.end)
        second = finish(tree, powers[1], 2, wrongly_updated)
        check_result(second, powers[1], 2)

    mutants.append(caught(reused_ticket, "reuse_first_seeds_mutated_ticket"))
    sparse = [-1, 0, 1, 1, -2, 1, 0, 1]
    other = Tree(len(sparse))
    other_ticket = (1, other.leaf(1))
    mutants.append(caught(lambda: check_result(
        finish(other, sparse, 3, other_ticket, "drop_forest_roots"), sparse, 3),
        "drop_forest_roots"))
    return {"sites": points, "seeds": rows, "prefix_count": 1, "prefix_ranks": 3,
            "relay_cases": 4, "results": accepted}, mutants


def randomized():
    random = Random(350921)
    work = Counter()
    max_stack, max_roots = 0, 0
    for n in (1, 2, 3, 7, 16, 31):
        for shape in ("balanced", "left_spine", "right_spine"):
            tree = Tree(n, shape)
            require(tree.height <= 48, "model exceeds u16 depth contract")
            for cursor in range(tree.end + 1):
                roots = verify_partition(tree, cursor)
                max_roots = max(max_roots, len(roots))
                rank = tree.rank(cursor)
                common = [random.choice((-3, 0, 2)) for _ in range(rank)]
                count = sum(value < 0 for value in common)
                work["partitions"] += 1
                for _ in range(3):
                    powers = common + [random.choice((-5, -1, 0, 1, 4)) for _ in range(n - rank)]
                    for threshold in sorted(set((count + 1, count + 2, n + 1))):
                        ticket = (count, cursor)
                        result = finish(tree, powers, threshold, ticket)
                        check_result(result, powers, threshold)
                        require(ticket == (count, cursor), "input ticket changed")
                        max_stack = max(max_stack, result["peak"])
                        work["relay_calls"] += 1
                        work["accepted"] += result["accepted"]
                        for key, value in result["work"].items():
                            work[key] += value
    return {"counts": dict(work), "max_stack": max_stack, "max_suffix_roots": max_roots,
            "scope": "abstract powers and bounded trees; no geometry-cost claim"}


def reordered_forest_counterexample():
    height = 8
    tree = Tree(2 ** height)
    cursor = tree.leaf(1)
    roots = verify_partition(tree, cursor)
    powers = [(-2 if r >= 128 else -1) if r % 2 == 0 else 1 for r in range(256)]
    ticket = (1, cursor)
    safe = finish(tree, powers, 257, ticket)
    check_result(safe, powers, 257)

    def minimum(node_id):
        node = tree.nodes[node_id]
        return min(powers[node["first"]:node["last"]])

    # Unsafe storage proof, not an incorrect census: prioritize all forest
    # roots together, retaining their siblings while opening the deepest root.
    stack = sorted(roots, key=minimum, reverse=True)
    peak = len(stack)
    while stack:
        peak = max(peak, len(stack))
        node = tree.nodes[stack.pop()]
        if node["children"]:
            left, right = node["children"]
            stack.extend((right, left) if minimum(left) <= minimum(right) else (left, right))
    require(len(roots) == height, "counterexample forest cardinality")
    require(safe["peak"] == height and peak == 2 * height - 1 and peak > height + 1,
            "reordered-forest storage counterexample not exercised")
    return {"kind": "abstract binary tree, not a native geometric fixture",
            "tree_height": height, "suffix_roots": len(roots),
            "sequential_roots_peak": safe["peak"], "old_one_tree_bound": height + 1,
            "reordered_combined_peak": peak,
            "general_formula": "2h-1 pending entries possible; h=48 gives95, not49"}


def main():
    real, mutants = real_fixture()
    report = {"schema": "q3-prefix-forest-relay-v1", "status": "PASS",
              "source_sha256": sha256(Path(__file__).read_bytes()).hexdigest(),
              "real_fixture": real, "abstract_matrix": randomized(),
              "forest_storage_counterexample": reordered_forest_counterexample(),
              "mutants": mutants,
              "scope": "independent model; no product qualification or performance claim"}
    print(json.dumps(report, sort_keys=True, separators=(",", ":")))


if __name__ == "__main__":
    main()
