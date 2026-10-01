#!/usr/bin/env python3
"""Independent Fraction sphere-cell Euler counting; no native/geometry imports."""
from fractions import Fraction as Q
from functools import cmp_to_key
from itertools import combinations, product
from math import comb, gcd, lcm
import json


def require(ok, what):
    if not ok:
        raise RuntimeError(what)


def add(a, b):
    return tuple(x + y for x, y in zip(a, b))


def scale(a, t):
    return tuple(x * t for x in a)


def sub(a, b):
    return add(a, scale(b, -1))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def ray(a):
    a = tuple(Q(x) for x in a)
    den = lcm(*(x.denominator for x in a))
    integers = tuple(int(x * den) for x in a)
    g = gcd(*integers)
    require(g != 0, "zero ray")
    return tuple(x // g for x in integers)


def line(a):
    r = ray(a)
    return scale(r, -1) if next(x for x in r if x) < 0 else r


def choose(n, k):
    return comb(n, k) if 0 <= k <= n else 0


def solve_unique(matrix, rhs):
    """Exact overdetermined solve, None if inconsistent or non-unique."""
    a = [[Q(x) for x in row] + [Q(r)] for row, r in zip(matrix, rhs)]
    columns = len(matrix[0])
    pivots = []
    for j in range(columns):
        pivot = next((i for i in range(len(pivots), len(a)) if a[i][j]), None)
        if pivot is None:
            continue
        i = len(pivots)
        a[i], a[pivot] = a[pivot], a[i]
        t = a[i][j]
        a[i] = [x / t for x in a[i]]
        for r in range(len(a)):
            if r != i and a[r][j]:
                t = a[r][j]
                a[r] = [x - t * y for x, y in zip(a[r], a[i])]
        pivots.append(j)
    if any(all(x == 0 for x in row[:-1]) and row[-1] != 0 for row in a):
        return None
    if len(pivots) != columns:
        return None
    out = [Q(0)] * columns
    for i, j in enumerate(pivots):
        out[j] = a[i][-1]
    return out


def encloses_zero(points):
    """Independent exact Caratheodory oracle, including lower-dimensional supports."""
    for size in range(1, min(4, len(points)) + 1):
        for support in combinations(points, size):
            matrix = [[p[axis] for p in support] for axis in range(3)] + [[1] * size]
            weights = solve_unique(matrix, [0, 0, 0, 1])
            if weights is not None and min(weights) >= 0:
                return True
    return False


def exact_meb(points):
    """Independent full MEB: enumerate at most four affinely independent supports."""
    best = None
    for size in range(1, min(4, len(points)) + 1):
        for support in combinations(points, size):
            anchor = support[0]
            if size == 1:
                center = anchor
            else:
                basis = [sub(p, anchor) for p in support[1:]]
                coefficients = solve_unique([[2 * dot(a, b) for b in basis] for a in basis],
                                            [dot(a, a) for a in basis])
                if coefficients is None:
                    continue
                center = anchor
                for t, b in zip(coefficients, basis):
                    center = add(center, scale(b, t))
            beta = dot(sub(center, anchor), sub(center, anchor))
            if all(dot(sub(p, center), sub(p, center)) <= beta for p in points):
                if best is None or beta < best[1]:
                    best = (center, beta)
    require(best is not None, "MEB missing")
    return best


def sphere_cells(shell, metadata=False):
    """Explicit exact arrangement. Deliberately cubic sign annotation, not optimized DCEL."""
    require(shell and all(dot(p, p) == dot(shell[0], shell[0]) > 0 for p in shell), "shell norm")
    require(len(set(shell)) == len(shell), "distinct shell sites")
    normals = sorted(set(line(p) for p in shell))
    vertices = set()
    for a, b in combinations(normals, 2):
        v = ray(cross(a, b))
        vertices.update((v, scale(v, -1)))
    # A lone great circle is S^1, not one open edge: split with two auxiliary vertices.
    if len(normals) == 1:
        n = normals[0]
        axis = next(a for a in ((1, 0, 0), (0, 1, 0), (0, 0, 1)) if any(cross(n, a)))
        v = ray(cross(n, axis))
        vertices.update((v, scale(v, -1)))
    edges = []
    edge_data = []
    faces = {}
    for n in normals:
        axis = next(a for a in ((1, 0, 0), (0, 1, 0), (0, 0, 1)) if any(cross(n, a)))
        e = cross(n, axis)
        f = cross(n, e)
        ring = [v for v in vertices if dot(n, v) == 0]

        def compare(a, b):
            ax, ay = dot(a, e), dot(a, f)
            bx, by = dot(b, e), dot(b, f)
            ah = 0 if ay > 0 or (ay == 0 and ax > 0) else 1
            bh = 0 if by > 0 or (by == 0 and bx > 0) else 1
            if ah != bh:
                return -1 if ah < bh else 1
            det = ax * by - ay * bx
            return -1 if det > 0 else (1 if det < 0 else 0)

        ring.sort(key=cmp_to_key(compare))
        for i, a in enumerate(ring):
            b = ring[(i + 1) % len(ring)]
            q = add(a, b)
            if not any(q):
                q = cross(n, a)  # Positive semicircle in this ring orientation.
            require(dot(n, q) == 0 and any(q), "edge sample")
            zeros = [m for m in normals if dot(m, q) == 0]
            require(zeros == [n], "edge has unexpected zero circle")
            edges.append(q)
            # Perturb a QUERY vector, never the shell sites: exact samples in adjacent faces.
            bounds = [abs(Q(dot(m, q), 2 * dot(m, n))) for m in normals
                      if dot(m, n) != 0 and dot(m, q) != 0]
            delta = min(bounds) if bounds else Q(1)
            adjacent = []
            for direction in (-1, 1):
                v = add(q, scale(n, direction * delta))
                signs = tuple(1 if dot(p, v) > 0 else -1 for p in shell)
                require(all(dot(p, v) != 0 for p in shell), "face on boundary")
                faces.setdefault(signs, v)
                adjacent.append(signs)
            edge_data.append({"normal": n, "a": a, "b": b, "q": q, "faces": adjacent})
    cells = [(0, v) for v in sorted(vertices)] + [(1, v) for v in edges] + [(2, v) for v in faces.values()]
    require(sum((-1) ** dim for dim, _ in cells) == 2, "sphere Euler characteristic")
    if metadata:
        # Dense sign tuples and their hashing belong ONLY to this cubic toy
        # construction. The annotation stage receives compact integer face IDs.
        face_ids = {signs: i for i, signs in enumerate(faces)}
        for edge in edge_data:
            edge["faces"] = tuple(face_ids[signs] for signs in edge["faces"])
        return cells, {"vertices": sorted(vertices), "edges": edge_data,
                       "faces": list(faces.values()), "normals": normals}
    return cells


def annotation_program(shell, data):
    """Sparse linear DAG from exact face adjacency and circle/vertex incidences.

    Construction of the toy arrangement above is NOT claimed quadratic. This
    stage never scans all shell sites per cell. Input nodes are formal weights.
    """
    u = len(shell)
    groups = {n: [] for n in data["normals"]}
    for i, p in enumerate(shell):
        groups[line(p)].append(i)
    require(max(map(len, groups.values())) <= 2, "distinct same-radius direction groups <=2")
    expressions = [[] for _ in shell]
    outputs = []
    incidence_visits = 0

    def node(terms):
        expressions.append(terms)
        return len(expressions) - 1

    face_nodes = [None] * len(data["faces"])
    neighbors = [[] for _ in data["faces"]]
    for edge in data["edges"]:
        a, b = edge["faces"]
        neighbors[a].append((b, edge["normal"]))
        neighbors[b].append((a, edge["normal"]))
    root = 0
    face_nodes[root] = node([(i, 1) for i, p in enumerate(shell) if dot(p, data["faces"][root]) > 0])
    todo = [root]
    while todo:
        current = todo.pop()
        for other, normal in neighbors[current]:
            if face_nodes[other] is not None:
                continue
            terms = [(face_nodes[current], 1)]
            for i in groups[normal]:
                incidence_visits += 1
                before = int(dot(shell[i], data["faces"][current]) > 0)
                after = int(dot(shell[i], data["faces"][other]) > 0)
                terms.append((i, after - before))
                require(before != after, "crossing group toggles sign")
            face_nodes[other] = node(terms)
            todo.append(other)
    require(all(node_id is not None for node_id in face_nodes), "face adjacency connected")
    for key, v in enumerate(data["faces"]):
        outputs.append((2, face_nodes[key], v))
    vertex_normals = {v: set() for v in data["vertices"]}
    vertex_adjacent = {}
    for edge in data["edges"]:
        adjacent = edge["faces"][0]
        normal = edge["normal"]
        terms = [(face_nodes[adjacent], 1)]
        for i in groups[normal]:
            incidence_visits += 1
            if dot(shell[i], data["faces"][adjacent]) > 0:
                terms.append((i, -1))
        outputs.append((1, node(terms), edge["q"]))
        for v in (edge["a"], edge["b"]):
            vertex_normals[v].add(normal)
            vertex_adjacent.setdefault(v, adjacent)
    for v, normals in vertex_normals.items():
        adjacent = vertex_adjacent[v]
        terms = [(face_nodes[adjacent], 1)]
        for normal in normals:
            for i in groups[normal]:
                incidence_visits += 1
                require(dot(shell[i], v) == 0, "vertex zero incidence")
                if dot(shell[i], data["faces"][adjacent]) > 0:
                    terms.append((i, -1))
        outputs.append((0, node(terms), v))
    return expressions, outputs, {"nodes": len(expressions),
                                 "terms": sum(len(e) for e in expressions),
                                 "incidence_visits": incidence_visits,
                                 "cells": len(outputs), "input_nodes": u}


def evaluate_program(expressions, weights):
    values = list(weights)
    for terms in expressions[len(weights):]:
        values.append(sum(coefficient * values[parent] for parent, coefficient in terms))
    return values


def transpose_program(expressions, outputs, cell_weights, u, mutant=False):
    """Transpose of a fixed linear map. NOT derivative of binomial coefficients."""
    adjoints = [0] * len(expressions)
    for (_, index, _), value in zip(outputs, cell_weights):
        adjoints[index] += value
    for index in range(len(expressions) - 1, u - 1, -1):
        for parent, coefficient in expressions[index]:
            adjoints[parent] += (abs(coefficient) if mutant else coefficient) * adjoints[index]
    return adjoints[:u]


def compact_counts(shell, kmax, mutant=False):
    cells, data = sphere_cells(shell, metadata=True)
    expressions, outputs, work = annotation_program(shell, data)
    u = len(shell)
    values = evaluate_program(expressions, [1] * u)
    # Causal correctness of the LINEAR operator for arbitrary signed formal weights.
    for weights in ([1] * u, [(-1) ** i * (i + 2) for i in range(u)]):
        evaluations = evaluate_program(expressions, weights)
        for _, index, v in outputs:
            direct = sum(weights[i] for i, p in enumerate(shell) if dot(p, v) > 0)
            require(evaluations[index] == direct, "annotation operator identity")
    h = [0] * (kmax + 1)
    hx = [[0] * (kmax + 1) for _ in shell]
    for j in range(1, kmax + 1):
        h[j] = choose(u, j) - sum((-1) ** dim * choose(values[index], j) for dim, index, _ in outputs)
        lambdas = [(-1) ** dim * choose(values[index] - 1, j - 1) if values[index] > 0 else 0
                   for dim, index, _ in outputs]
        bad_x = transpose_program(expressions, outputs, lambdas, u, mutant=mutant)
        for i in range(u):
            hx[i][j] = choose(u - 1, j - 1) - bad_x[i]
    work["reverse_term_visits_all_j"] = kmax * work["terms"]
    require(len(outputs) == len(cells), "program cell census")
    return h, hx, work


def fixed_k_counts(shell, p, K, expressions, outputs, values):
    """One transpose at fixed K; shared scalar for all p strict interior sites."""
    u = len(shell)
    lambdas = [(-1) ** dim * choose(p + values[index] - 1, K - 1) if values[index] > 0 else 0
               for dim, index, _ in outputs]
    bad_x = transpose_program(expressions, outputs, lambdas, u)
    hx = [choose(p + u - 1, K - 1) - bad for bad in bad_x]
    interior_scalar = None
    if p >= 1:
        interior_scalar = (choose(p + u - 1, K - 1) + choose(p - 1, K - 1)
                           - sum((-1) ** dim * choose(p - 1 + values[index], K - 1)
                                 for dim, index, _ in outputs))
    all_parts = (choose(p + u, K) + choose(p, K)
                 - sum((-1) ** dim * choose(p + values[index], K) for dim, index, _ in outputs))
    return hx, interior_scalar, all_parts


def all_subset_mebs(points, kmax):
    """Independent exact MEB oracle, memoized without using shell or Euler logic.

    If MEB(F minus z) covers z, it is exactly MEB(F). Otherwise F has at most
    four essential points and is solved directly by affine Gram supports.
    Exact containment bit masks are cached per rational center/radius.
    """
    balls = []
    intern = {}
    subsets = {}
    direct_calls = 0
    for mask in range(1, 1 << len(points)):
        if mask.bit_count() > kmax:
            continue
        chosen = None
        remaining = mask
        while remaining:
            bit = remaining & -remaining
            child = mask ^ bit
            if child and balls[subsets[child]][2] & bit:
                chosen = subsets[child]
                break
            remaining ^= bit
        if chosen is None:
            require(mask.bit_count() <= 4, "MEB essential support exceeds dimension+1")
            support = [points[i] for i in range(len(points)) if mask & (1 << i)]
            center, beta = exact_meb(support)
            direct_calls += 1
            key = (center, beta)
            chosen = intern.get(key)
            if chosen is None:
                cover = 0
                for i, point in enumerate(points):
                    if dot(sub(point, center), sub(point, center)) <= beta:
                        cover |= 1 << i
                chosen = len(balls)
                intern[key] = chosen
                balls.append((center, beta, cover))
        require(balls[chosen][2] & mask == mask, "MEB oracle fails containment")
        subsets[mask] = chosen
    return subsets, balls, direct_calls


def fixed_k_checks(shell, name):
    """All p=0..5/K=1..8; independent full MEBs versus both exact formulas."""
    shell = [tuple(Q(v) for v in point) for point in shell]
    interior = [(Q(0), Q(0), Q(0))] + [scale(shell[0], Q(t, 8)) for t in (1, 2, -1, -2)]
    require(len(set(shell + interior)) == len(shell) + 5, "interior distinct")
    require(all(dot(point, point) < dot(shell[0], shell[0]) for point in interior), "strict interiors")
    points = shell + interior
    u = len(shell)
    subsets, balls, direct_calls = all_subset_mebs(points, 8)
    observations = {}
    for p in range(6):
        n = u + p
        for K in range(1, 9):
            observations[p, K] = ([0] * n, 0, 0)
    for mask, ball_id in subsets.items():
        K = mask.bit_count()
        center, beta, _ = balls[ball_id]
        for p in range(6):
            n = u + p
            if mask >> n:
                continue
            counts, good, tried = observations[p, K]
            tried += 1
            if center == (0, 0, 0) and beta == dot(shell[0], shell[0]):
                good += 1
                for i in range(n):
                    if mask & (1 << i):
                        counts[i] += 1
            observations[p, K] = counts, good, tried
    h, hx, _ = shell_counts(shell, 8)
    _, data = sphere_cells(shell, metadata=True)
    expressions, outputs, _ = annotation_program(shell, data)
    values = evaluate_program(expressions, [1] * u)
    # Explicit zero-row check: n_C=0 rows are zero linear maps even at arbitrary lambda.
    zero_lambdas = [1 if values[index] == 0 else 0 for _, index, _ in outputs]
    require(transpose_program(expressions, outputs, zero_lambdas, u) == [0] * u, "zero-cell rows")
    records = []
    for p in range(6):
        for K in range(1, 9):
            observed, all_parts, tried = observations[p, K]
            require(tried == choose(u + p, K), "MEB exhaustive subset census")
            shell_predicted, interior_scalar, total = fixed_k_counts(shell, p, K, expressions, outputs, values)
            predicted = shell_predicted + ([interior_scalar] * p if p else [])
            old = [sum(choose(p, K - j) * row[j] for j in range(1, K + 1)) for row in hx]
            if p:
                old += [sum(choose(p - 1, K - 1 - j) * h[j] for j in range(1, K))] * p
            else:
                require(interior_scalar is None, "no fictitious interior scalar for p=0")
            require(predicted == old == observed, "fixed-K direct/convolution/MEB " + name + str((p, K)))
            require(total == all_parts and sum(predicted) == K * total, "fixed-K total/incidence")
            if p == 1 and K == 1:
                wrong = interior_scalar - choose(p - 1, K - 1)
                require(wrong != observed[-1], "missing empty-set correction mutant killed")
            records.append({"p": p, "K": K, "parts": total, "by_site": predicted, "oracle_subsets": tried})
    return {"name": name, "cases": records, "distinct_subsets_memoized": len(subsets),
            "direct_gram_meb_calls": direct_calls, "exact_balls_interned": len(balls)}


def shell_counts(shell, kmax, mutant=None):
    cells = sphere_cells(shell)
    u = len(shell)
    h = [0] * (kmax + 1)
    hx = [[0] * (kmax + 1) for _ in shell]
    for j in range(1, kmax + 1):
        bad = 0
        bx = [0] * u
        for dim, v in cells:
            positive = [i for i, p in enumerate(shell) if dot(p, v) > 0]
            if mutant == "zeros_positive":
                positive = [i for i, p in enumerate(shell) if dot(p, v) >= 0]
            sign = 1 if mutant == "no_alternation" else (-1) ** dim
            bad += sign * choose(len(positive), j)
            for i in positive:
                bx[i] += sign * choose(len(positive) - 1, j - 1)
        h[j] = choose(u, j) - bad
        for i in range(u):
            hx[i][j] = choose(u - 1, j - 1) - bx[i]
    return h, hx, tuple(sum(dim == d for dim, _ in cells) for d in range(3))


def brute_counts(shell, kmax):
    h = [0] * (kmax + 1)
    hx = [[0] * (kmax + 1) for _ in shell]
    subsets = 0
    for j in range(1, min(kmax, len(shell)) + 1):
        for ids in combinations(range(len(shell)), j):
            subsets += 1
            if encloses_zero([shell[i] for i in ids]):
                h[j] += 1
                for i in ids:
                    hx[i][j] += 1
    return h, hx, subsets


def check():
    axes = [(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)]
    tetra = [(1, 1, 1), (1, -1, -1), (-1, 1, -1), (-1, -1, 1)]
    def circle(t):
        t = Q(t)
        return ((1 - t * t) / (1 + t * t), 2 * t / (1 + t * t), Q(0))
    hexagon = [circle(t) for t in (0, Q(1, 2), Q(-1, 2))]
    hexagon += [scale(p, -1) for p in hexagon]
    fixtures = [
        ("one_direction", axes[:1]), ("antipodal_pair", axes[:2]),
        ("two_directions", [axes[0], axes[2]]), ("square", axes[:4]),
        ("octahedron", axes), ("tetrahedron", tetra),
        ("cube", list(product((-1, 1), repeat=3))),
        ("hexagon_coplanar", hexagon), ("hexagon_with_poles", hexagon + axes[4:]),
        ("tetrahedron_extra", tetra + [(Q(-1, 5), Q(7, 5), Q(1))]),
    ]
    cases = []
    total_subsets = 0
    fixed_checks = []
    for name, shell in fixtures:
        shell = [tuple(Q(v) for v in p) for p in shell]
        h, hx, cells = shell_counts(shell, 5)
        compact_h, compact_hx, program_work = compact_counts(shell, 5)
        expected, expected_x, subsets = brute_counts(shell, 5)
        require(h == expected and hx == expected_x, "Euler vs Caratheodory: " + name)
        require(compact_h == h and compact_hx == hx, "compact transpose vs scanned Euler: " + name)
        require(all(sum(row[j] for row in hx) == j * h[j] for j in range(1, 6)), "incidence sum: " + name)
        cases.append({"name": name, "u": len(shell), "cells_dim012": cells, "h_0_to_5": h,
                      "h5_by_site": [row[5] for row in hx], "oracle_subsets": subsets})
        cases[-1]["annotation_program"] = program_work
        total_subsets += subsets
        fixed_checks.append(fixed_k_checks(shell, name))
    h, hx, _ = shell_counts(axes, 5)
    require(h == [0, 0, 3, 12, 15, 6] and all(row[5] == 5 for row in hx), "octahedron engraved")
    # K5 support-canonical mutant: x=+e1, canonical diameter +/-e1 gives four, but true count is five.
    canonical_only = choose(4, 3)
    require(canonical_only == 4 and hx[0][5] == 5, "canonical support undercount killed")
    for mutant in ("zeros_positive", "no_alternation"):
        got, got_x, _ = shell_counts(axes[:4], 5, mutant)
        want, want_x, _ = brute_counts(axes[:4], 5)
        require(got != want or got_x != want_x, "mutant not killed: " + mutant)
    _, wrong_x, _ = compact_counts(axes[:4], 5, mutant=True)
    _, expected_x, _ = brute_counts(axes[:4], 5)
    require(wrong_x != expected_x, "transpose sign mutant killed")
    # Actual MEB independent check of the interior/shell convolutions at K3 and K5.
    meb_cases = []
    for name, shell, interior in [("octahedron_center", axes, [(0, 0, 0)]),
                                   ("tetrahedron_two_interior", tetra, [(0, 0, 0), (Q(1, 4), 0, 0)])]:
        points = [tuple(Q(v) for v in p) for p in shell + interior]
        u, p = len(shell), len(interior)
        h, hx, _ = shell_counts(shell, 5)
        for K in (3, 5):
            predicted = [sum(choose(p, K - j) * row[j] for j in range(1, K + 1)) for row in hx]
            predicted += [sum(choose(p - 1, K - 1 - j) * h[j] for j in range(1, K))] * p
            observed = [0] * len(points)
            count = 0
            for ids in combinations(range(len(points)), K):
                center, beta = exact_meb([points[i] for i in ids])
                if center == (0, 0, 0) and beta == dot(shell[0], shell[0]):
                    count += 1
                    for i in ids:
                        observed[i] += 1
            require(predicted == observed, "MEB convolution: " + name + str(K))
            meb_cases.append({"name": name, "K": K, "by_site": observed, "all_parts": count,
                              "enumerated_oracle_only": choose(len(points), K)})
    return {"status": "PASS", "algorithm": "exact_open_sphere_cells_compact_Euler",
            "fixtures": cases, "caratheodory_subsets": total_subsets, "meb_cases": meb_cases,
            "fixed_k_checks": fixed_checks, "fixed_k_case_count": sum(len(c["cases"]) for c in fixed_checks),
            "fixed_k_unique_memoized_subsets": sum(c["distinct_subsets_memoized"] for c in fixed_checks),
            "fixed_k_exhaustive_subsets": sum(r["oracle_subsets"] for c in fixed_checks for r in c["cases"]),
            "mutants_killed": ["zeros_positive", "no_alternation", "canonical_support_only", "transpose_abs_sign",
                               "fixed_k_missing_empty_correction"],
            "scope": "private Fraction cells; cubic toy arrangement; sparse linear annotation/transpose; no engine, FULL, GCP or timing claim"}


if __name__ == "__main__":
    print(json.dumps(check(), sort_keys=True, separators=(",", ":")))
