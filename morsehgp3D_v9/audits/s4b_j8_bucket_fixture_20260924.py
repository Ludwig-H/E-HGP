#!/usr/bin/env python3
"""Independent exact-arithmetic fixture for the S4b J8 live-bucket cost.

This checks the geometry and the loop count; it does not run the product or
claim a LiDAR growth bound. No files are read or written.
"""

from fractions import Fraction
from itertools import combinations
from math import isqrt


def need(ok: bool, why: str) -> None:
    if not ok:
        raise ValueError(why)


def ceil_sqrt_half(q: int) -> int:
    m = isqrt(q // 2)
    while 2 * m * m < q:
        m += 1
    need(m == 0 or 2 * (m - 1) * (m - 1) < q, "sqrt bound")
    return m


def check(l: int) -> tuple[int, int, int]:
    need(1 <= l <= 13107, "u18 parameter")
    d = 400 * l**2
    e = f = 244 * l**2
    gram = 57600 * l**4
    q = d * (3 * gram - 2 * e * f)
    need(q == 21491200 * l**6, "parameter bound")
    mubar = ceil_sqrt_half(q)
    g5 = mubar // 4
    zs = range(11 * l, 23 * l // 2 + 1)
    last_root = None
    lens4 = 0

    for z in zs:
        power = 57600 * l**4 * (z * z - 100 * l * l)
        side = 240 * l**2 * z
        root = Fraction(power, side)
        need(power > 0 and side > 0 and 0 < root < g5,
             "strictly interior root in live bucket j=4")
        need(power - g5 * side < 0, "right-end sign")
        lens4 += power < 0 and power - g5 * side < 0
        need(last_root is None or last_root < root, "distinct ordered roots")
        last_root = root

        # Complete cover radius |ab|, longest-edge owner ab, positive q4.
        need(z * z < d and 12 * 12 * l * l < d and
             100 * l * l + z * z < d and
             144 * l * l + z * z < d and (l // 2)**2 < d,
             "cover or owner")
        wx = Fraction(11, 72)
        wy = Fraction(z * z - 100 * l * l, 2 * z * z)
        wa = (1 - wx - wy) / 2
        need(wx > 0 and wy > 0 and wa > 0, "strict barycentric weights")

    events = len(zs)
    need(events == l // 2 + 1, "event count")
    need(g5 >= 819 * l**3, "bucket upper bound")
    # No site has negative power at the left endpoint g4=0; the three
    # support sites are on the whole family, hence lens[4] is zero.
    need(lens4 == 0, "lens")
    return events, events * events, min(events, 3)


def solve(matrix: list[list[int]], rhs: list[Fraction]) -> tuple[Fraction, ...] | None:
    """Tiny independent rational solver for the nine-point global q4 check."""
    n = len(matrix)
    rows = [[Fraction(x) for x in row] + [Fraction(y)]
            for row, y in zip(matrix, rhs)]
    need(len(rows) == n and all(len(row) == n + 1 for row in rows), "matrix shape")
    for col in range(n):
        pivot = next((r for r in range(col, n) if rows[r][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        scale = rows[col][col]
        rows[col] = [x / scale for x in rows[col]]
        for r in range(n):
            if r != col:
                scale = rows[r][col]
                rows[r] = [rows[r][j] - scale * rows[col][j]
                           for j in range(n + 1)]
    return tuple(row[-1] for row in rows)


def check_global_l10() -> None:
    """Enumerate every tetrahedron, including alternatives to the ab family."""
    points = [(0, 0, 0), (200, 0, 0), (100, 120, 0)] + [
        (100, 0, z) for z in range(110, 116)
    ]
    positive = []
    for ids in combinations(range(len(points)), 4):
        support = [points[i] for i in ids]
        matrix = [[2 * (support[j][c] - support[0][c]) for c in range(3)]
                  for j in range(1, 4)]
        rhs = [sum(v * v for v in support[j]) -
               sum(v * v for v in support[0]) for j in range(1, 4)]
        center = solve(matrix, rhs)
        if center is None:
            continue
        barycentric = solve([[support[j][c] for j in range(4)]
                             for c in range(3)] + [[1] * 4], list(center) + [1])
        if barycentric is None or min(barycentric) <= 0:
            continue
        radius2 = sum((center[c] - support[0][c]) ** 2 for c in range(3))
        depth = sum(sum((center[c] - point[c]) ** 2 for c in range(3)) < radius2
                    for i, point in enumerate(points) if i not in ids)
        positive.append((ids, depth))
    expected = [((0, 1, 2, i), i - 3) for i in range(3, 9)]
    need(positive == expected, "all positive q4 supports and depths at L=10")
    need(sum(depth <= 2 for _, depth in positive) == 3, "K5 q4 count")
    need(sum(depth <= 7 for _, depth in positive) == 6, "K10 q4 count")
    print("L=10 global_q4_positive=6 shallow_K5=3 shallow_K10=6")


def main() -> None:
    for l in (8, 10, 16, 64, 256, 13107):
        events, comparisons, shallow_k5 = check(l)
        print(f"L={l} events={events} comparisons={comparisons} "
              f"shallow_K5={shallow_k5}")
    check_global_l10()


if __name__ == "__main__":
    main()
