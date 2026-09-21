#!/usr/bin/env python3
"""Independent exact geometry and bounded join-state model; no product imports.

Run with python3 -B, then python3 -B -O and compare the JSON reports.
The join's rejection oracle enumerates a prescribed incidence relation.  It
validates partition/cache/state contracts, not the cost of geometric bounds.
"""

from collections import Counter
from fractions import Fraction as F
from hashlib import sha256
from itertools import product
import json
from pathlib import Path
from random import Random


class CheckFailure(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise CheckFailure(message)


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def norm(a):
    return dot(a, a)


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def solve(matrix, rhs):
    n = len(rhs)
    rows = [[F(x) for x in row] + [F(value)]
            for row, value in zip(matrix, rhs)]
    for column in range(n):
        pivot = next((j for j in range(column, n) if rows[j][column]), None)
        require(pivot is not None, "singular exact fixture")
        rows[column], rows[pivot] = rows[pivot], rows[column]
        scale = rows[column][column]
        rows[column] = [x / scale for x in rows[column]]
        for j in range(n):
            if j != column:
                scale = rows[j][column]
                rows[j] = [x - scale * y for x, y in zip(rows[j], rows[column])]
    return tuple(row[-1] for row in rows)


def ball(support):
    a = support[0]
    differences = [sub(x, a) for x in support[1:]]
    rows = [tuple(2 * u for u in d) for d in differences]
    rhs = [norm(x) - norm(a) for x in support[1:]]
    if len(support) == 3:
        normal = cross(*differences)
        rows.append(normal)
        rhs.append(dot(normal, a))
    center = solve(rows, rhs)
    gram = [[dot(d, e) for e in differences] for d in differences]
    weights = solve(gram, [dot(sub(center, a), d) for d in differences])
    weights = (1 - sum(weights),) + weights
    require(all(w > 0 for w in weights), "fixture support not positive")
    radius = norm(sub(center, a))
    require(all(norm(sub(center, x)) == radius for x in support), "sphere mismatch")
    return center, radius, weights


def frame(a, b):
    d = sub(b, a)
    axis = max(range(3), key=lambda i: abs(d[i]))
    require(d[axis] != 0, "zero edge")
    p, q = [0, 0, 0], [0, 0, 0]
    i, j = (axis + 1) % 3, (axis + 2) % 3
    h, sign = abs(d[axis]), 1 if d[axis] > 0 else -1
    p[i], p[axis] = h, -sign * d[i]
    q[j], q[axis] = h, -sign * d[j]
    return tuple(p), tuple(q)


def form(a, b, x):
    p, q = frame(a, b)
    w = tuple(2 * z - u - v for z, u, v in zip(x, a, b))
    return norm(w) - norm(sub(b, a)), -2 * dot(w, p), -2 * dot(w, q)


def center_at(a, b, t):
    p, q = frame(a, b)
    center = tuple(F(u + v + pi * t[0] + qi * t[1], 2)
                   for u, v, pi, qi in zip(a, b, p, q))
    return center, norm(sub(center, a))


def power4(a, b, x, t):
    center, radius = center_at(a, b, t)
    return 4 * (norm(sub(x, center)) - radius)


def cell_corners(cell):
    return product(cell[0], cell[1])


def exact_box_bounds(a, b, box, cell):
    """Independent Cartesian sphere-distance extrema on the continuous box."""
    bounds = []
    for t in cell_corners(cell):
        center, radius = center_at(a, b, t)
        near = tuple(min(hi, max(lo, c)) for (lo, hi), c in zip(box, center))
        far = tuple(max((lo, hi), key=lambda x: abs(x - c))
                    for (lo, hi), c in zip(box, center))
        bounds.append((4 * (norm(sub(near, center)) - radius),
                       4 * (norm(sub(far, center)) - radius)))
    return min(x[0] for x in bounds), max(x[1] for x in bounds)


def touches(a, b, x, cell):
    values = [power4(a, b, x, t) for t in cell_corners(cell)]
    return min(values) <= 0 <= max(values)


def geometry_checks():
    a, b = (15, 20, 20), (24, 23, 20)
    seeds = [(20, 15, 20), (23, 16, 20)]
    for x in seeds:
        center, radius, _ = ball((a, b, x))
        require(center == (20, 20, 20) and radius == 25, "coincident seed sphere")
        require(form(a, b, x) == (80, 240, 0), "coincident seed line")
        require(norm(sub(a, b)) == 90 and
                max(norm(sub(a, x)), norm(sub(b, x))) == 80, "seed edge owner")
        require(power4(a, b, x, (F(-1, 3), F(7, 13))) == 0, "entire seed line")
    cells = [((F(-2), F(0)) if not q & 1 else (F(0), F(2)),
              (F(-2), F(0)) if not q & 2 else (F(0), F(2))) for q in range(4)]
    relation = {(r, c) for r, x in enumerate(seeds)
                for c, cell in enumerate(cells) if touches(a, b, x, cell)}
    require(relation == {(0, 0), (0, 2), (1, 0), (1, 2)}, "coincident incidences")

    a, b = (30, 30, 30), (36, 36, 30)
    x, y = (30, 36, 24), (36, 30, 24)
    shell = (a, b, x, y, (30, 36, 30), (36, 30, 30),
             (30, 30, 24), (32, 32, 32))
    center, radius, weights = ball((a, b, x, y))
    require(center == (33, 33, 27) and radius == 27, "contact sphere")
    require(weights == (F(1, 4),) * 4, "positive tetrahedron")
    powers = [norm(sub(z, center)) - radius for z in shell]
    require(powers == [0] * 8, "complete eight-site contact shell")
    contacts = [((F(0), F(1)), (F(-2), F(-1))),
                ((F(-1), F(0)), (F(-1), F(0)))]
    contact_bounds = [exact_box_bounds(a, b, tuple((v, v) for v in x), c)
                      for c in contacts]
    require(contact_bounds == [(F(-288), F(0)), (F(0), F(288))], "contact bounds")
    require(all(touches(a, b, x, c) for c in contacts), "closed corner lost")
    require(all(lo >= 0 or hi <= 0 for lo, hi in contact_bounds),
            "nonstrict-rejection mutant not causal")

    box = ((27, 39), (27, 39), (21, 33))
    cell = ((F(0), F(0)), (F(-1), F(-1)))
    minimum, maximum = exact_box_bounds(a, b, box, cell)
    corners = [power4(a, b, p, (F(0), F(-1))) for p in product(*box)]
    require((minimum, maximum) == (-108, 324), "interior minimum bounds")
    require(corners == [324] * 8 and power4(a, b, x, (F(0), F(-1))) == 0,
            "box-corners-only mutant not causal")

    random = Random(20260921)
    evaluations = 0
    for _ in range(96):
        a = tuple(random.randrange(65536) for _ in range(3))
        b = tuple(random.randrange(65536) for _ in range(3))
        box = tuple(tuple(sorted((random.randrange(65536), random.randrange(65536))))
                    for _ in range(3))
        cell = tuple(tuple(sorted((F(random.randrange(-8, 9), 4),
                                  F(random.randrange(-8, 9), 4)))) for _ in range(2))
        observed = []
        for t in cell_corners(cell):
            center, _ = center_at(a, b, t)
            choices = [(lo, hi, min(hi, max(lo, c)))
                       for (lo, hi), c in zip(box, center)]
            for point in product(*choices):
                value = power4(a, b, point, t)
                c, u, v = form(a, b, point)
                require(value == c + u * t[0] + v * t[1], "Cartesian power/form identity")
                observed.append(value)
                evaluations += 1
        require(exact_box_bounds(a, b, box, cell) == (min(observed), max(observed)),
                "separable extrema differ from extremal candidates")
    return ({"coincident_seeds": 2, "coincident_seed_leaf_pairs": 4,
             "strict_contact_cells": 2, "contact_shell_ids": 8,
             "corner_only_false_rejection": True, "random_boxes": 96,
             "exact_power_checks": evaluations}, relation)


class Node:
    def __init__(self, items, children=()):
        self.items, self.children = tuple(items), tuple(children)
        self.height = 0 if not children else 1 + max(c.height for c in children)


def spatial(first, last, skew):
    if last - first == 1:
        return Node((first,))
    middle = first + 1 if skew else (first + last) // 2
    children = (spatial(first, middle, skew), spatial(middle, last, skew))
    return Node(range(first, last), children)


def atlas(depth, first=0):
    if depth == 0:
        return Node((first,))
    size = 4 ** (depth - 1)
    children = tuple(atlas(depth - 1, first + i * size) for i in range(4))
    return Node(range(first, first + 4 * size), children)


def blocks(root, grain):
    pending, out = [root], []
    while pending:
        node = pending.pop()
        if len(node.items) <= grain:
            out.append(node)
        else:
            pending.extend(reversed(node.children))
    require(Counter(r for b in out for r in b.items) == Counter(root.items),
            "block antichain does not partition seeds")
    return out


def join(root, centers, grain, relation, valid, live, strategy, mutant=None):
    expected = {(r, c) for r, c in relation if valid[r] and c in live}
    emitted, families, validations = [], Counter(), Counter()
    visits, max_stack, bound_stack, max_cache, terminal = 0, 0, 0, 0, 0
    for block in blocks(root, grain):
        cache = [None] * len(block.items)
        max_cache = max(max_cache, len(cache))
        require(len(cache) <= grain, "cache exceeds grain")
        stack = [(block, centers)]
        cap = 1 + block.height + 3 * centers.height
        bound_stack = max(bound_stack, cap)
        while stack:
            max_stack = max(max_stack, len(stack))
            require(len(stack) <= cap, "product DFS stack bound")
            x, c = stack.pop()
            visits += 1
            live_leaves = sum(z in live for z in c.items)
            if not live_leaves or not any((r, z) in relation
                    for r in x.items for z in c.items if z in live):
                continue
            if not x.children and not c.children:
                terminal += 1
                rank, leaf = x.items[0], c.items[0]
                slot = rank - block.items[0]
                require(0 <= slot < len(cache), "rank outside block cache")
                if cache[slot] is None or mutant == "no_cache":
                    validations[rank] += 1
                    cache[slot] = valid[rank]
                    if valid[rank]:
                        families[rank] += 1
                if cache[slot]:
                    if mutant == "restart_root":
                        emitted.extend((rank, z) for z in centers.items
                                       if z in live and (rank, z) in relation)
                    else:
                        emitted.append((rank, leaf))
                continue
            split_x = bool(x.children) and (
                not c.children or strategy == "x_first" or
                (strategy == "adaptive" and len(x.items) >= live_leaves))
            children = ([(child, c) for child in x.children] if split_x else
                        [(x, child) for child in c.children])
            if mutant == "drop_child" and not split_x:
                children = children[:-1]
            stack.extend(reversed(children))
    require(Counter(emitted) == Counter(expected), "lost or duplicated seed/cell pair")
    require(all(count == 1 for count in families.values()), "family prepared repeatedly")
    require(set(families) == {r for r, _ in expected}, "family population mismatch")
    require(all(count == 1 for count in validations.values()), "seed validation repeated")
    return {"runs": 1, "products": visits, "terminal_pairs": terminal,
            "emitted_pairs": len(emitted), "families": sum(families.values()),
            "peak_stack": max_stack, "stack_bound": bound_stack,
            "peak_cache_slots": max_cache}


def rejected(call, name):
    try:
        call()
    except CheckFailure as error:
        return {"mutant": name, "caught": str(error)}
    raise CheckFailure("surviving mutant: " + name)


def structure_checks(geometric_relation):
    total = Counter()
    maxima = {"peak_stack": 0, "stack_bound": 0, "peak_cache_slots": 0}
    for n, depth, skew, strategy, grain, pattern in product(
            (1, 2, 3, 7, 11, 17), (0, 1, 2), (False, True),
            ("x_first", "c_first", "adaptive"), (1, 2, 4, 32), (0, 1, 2)):
        root, centers = spatial(0, n, skew), atlas(depth)
        valid = {r: r % 5 != 4 for r in range(n)}
        live = {c for c in centers.items if pattern == 0 or (c + pattern) % 7 != 0}
        relation = {(r, c) for r in range(n) for c in centers.items if pattern == 0 or
                    (pattern == 1 and (r + c) % 3 == 0) or
                    (pattern == 2 and ((r + 1) * (c + 3)) % 11 < 3)}
        result = join(root, centers, grain, relation, valid, live, strategy)
        for key, value in result.items():
            if key in maxima:
                maxima[key] = max(maxima[key], value)
            else:
                total[key] += value
    root, centers = spatial(0, 2, False), atlas(1)
    kwargs = dict(root=root, centers=centers, grain=2, relation=geometric_relation,
                  valid={0: True, 1: True}, live=set(centers.items), strategy="c_first")
    causal = join(**kwargs)
    require(causal["emitted_pairs"] == 4 and causal["families"] == 2,
            "geometric cache fixture not exercised")
    mutants = [rejected(lambda m=m: join(**kwargs, mutant=m), m)
               for m in ("no_cache", "restart_root")]
    full = dict(kwargs, relation={(r, c) for r in range(2) for c in centers.items})
    mutants.append(rejected(lambda: join(**full, mutant="drop_child"), "drop_child"))
    # Full product, C split first: the stated stack bound is actually attained.
    tight = join(spatial(0, 17, True), atlas(2), 32,
                 {(r, c) for r in range(17) for c in range(16)},
                 {r: True for r in range(17)}, set(range(16)), "c_first")
    require(tight["peak_stack"] == tight["stack_bound"] == 23,
            "product DFS bound not positively exercised")
    require(1 + 48 + 3 * 44 == 181, "u16/atlas bound arithmetic")
    return {"totals": dict(total), "maxima": maxima, "geometric_case": causal,
            "tight_case": tight, "u16_atlas_max_stack": 181,
            "model_rejection": "enumerated relation; no geometric performance claim"}, mutants


def main():
    geometry, relation = geometry_checks()
    structure, mutants = structure_checks(relation)
    mutants += [
        {"mutant": "reject_zero", "caught": "two genuine closed contact cells lost"},
        {"mutant": "box_corners_only", "caught": "eight corners positive, exact minimum -108, seed on shell"},
    ]
    report = {"schema": "q4-seed-cell-math-v1", "status": "PASS",
              "source_sha256": sha256(Path(__file__).read_bytes()).hexdigest(),
              "geometry": geometry, "structure": structure, "mutants": mutants,
              "scope": "independent bounded model; no product qualification or timing"}
    print(json.dumps(report, sort_keys=True, separators=(",", ":")))


if __name__ == "__main__":
    main()
