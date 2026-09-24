#!/usr/bin/env python3
"""Exact u18 fixture for the equal-root group in the S4b refinement scratch.

Enumerates every lattice point used by the construction. No product code is
imported and no file is read or written. Run normally and with ``python3 -O``.
"""

from fractions import Fraction
from math import isqrt


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def dot(a: tuple[int, ...], b: tuple[int, ...]) -> int:
    return sum(p * q for p, q in zip(a, b))


def sub(a: tuple[int, ...], b: tuple[int, ...]) -> tuple[int, ...]:
    return tuple(p - q for p, q in zip(a, b))


def cross(a: tuple[int, ...], b: tuple[int, ...]) -> tuple[int, int, int]:
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def dist2(a: tuple[int, ...], b: tuple[int, ...]) -> int:
    return dot(sub(a, b), sub(a, b))


def lattice_sphere(radius: int):
    """Yield each distinct integer point on the sphere centred at zero."""
    r2 = radius * radius
    for xx in range(-radius, radius + 1):
        yz2 = r2 - xx * xx
        ymax = isqrt(yz2)
        for yy in range(-ymax, ymax + 1):
            zz2 = yz2 - yy * yy
            zz = isqrt(zz2)
            if zz * zz != zz2:
                continue
            yield (xx, yy, zz)
            if zz:
                yield (xx, yy, -zz)


def check(scale: int, expected: int) -> None:
    a = (-24 * scale, 0, 7 * scale)
    b = (24 * scale, 0, 7 * scale)
    x = (0, 15 * scale, -20 * scale)
    witness = (0, -15 * scale, -20 * scale)
    d, u = sub(b, a), sub(x, a)
    normal = cross(d, u)
    D, E, F = dist2(a, b), dist2(a, x), dist2(b, x)
    du = dot(d, u)
    G = D * E - du * du
    W = tuple(E * (D - du) * d[i] + D * (E - du) * u[i]
              for i in range(3))
    Q = D * (3 * G - 2 * E * F)
    need((D, E, F, G, Q) ==
         (2304 * scale**2, 1530 * scale**2, 1530 * scale**2,
          2198016 * scale**4, 4405819392 * scale**6), "seed constants")
    mubar = isqrt(Q // 2)
    if 2 * mubar * mubar < Q:
        mubar += 1
    need(2 * (mubar - 1) ** 2 < Q <= 2 * mubar**2, "J8 bound")
    grid = [0] * 9
    for i in range(5):
        grid[4 + i] = mubar * i // 4
        grid[4 - i] = -grid[4 + i]
    root = -10080 * scale**3
    need(grid[3] < root < grid[4] == 0, "root inside live bucket j3")

    ys: list[tuple[int, int, int]] = []
    centre_twice = tuple(a[i] + b[i] for i in range(3))
    for y in lattice_sphere(25 * scale):
        # These inequalities guarantee both complete-cover membership and
        # longest-edge ownership for each tetrahedron (a,b,x,y).
        if max(dist2(y, a), dist2(y, b), dist2(y, x)) > D:
            continue
        need(sum((2 * y[i] - centre_twice[i]) ** 2 for i in range(3))
             <= 4 * D, "complete closed edge cover")
        v = sub(y, a)
        B = dot(normal, v)
        P = G * dot(v, v) - dot(W, v)
        if B == 0 or P <= 0:
            continue
        need(P == root * B and B < 0, "common exact event root")
        ys.append(y)

    need(len(ys) == expected and witness in ys, "lattice event count")
    need(len(set(ys)) == expected, "distinct sites")
    for y in ys:
        # IDs are a=0,b=1,x=2,y>=3. This checks owner ties exactly as
        # the scratch's lexicographic longest-edge rule does.
        vertices = (a, b, x, y)
        edges = [(dist2(vertices[i], vertices[j]), (i, j))
                 for i in range(4) for j in range(i + 1, 4)]
        longest = max(length for length, _ in edges)
        owner = min(pair for length, pair in edges
                    if length == longest)
        need(owner == (0, 1), "support owner including longest-edge ties")
    shift = (25 * scale,) * 3
    for point in (a, b, x, *ys):
        need(all(0 <= point[i] + shift[i] < 1 << 18 for i in range(3)),
             "translated u18 domain")
    need(D > E and D > F and D + E > F and D + F > E and E + F > D,
         "strictly acute seed and longest edge among a,b,x")

    lens = [0] * 8
    for y in ys:
        v = sub(y, a)
        B = dot(normal, v)
        P = G * dot(v, v) - dot(W, v)
        for j in range(8):
            lens[j] += P - grid[j] * B < 0 and P - grid[j + 1] * B < 0
    need(lens == [expected, expected, expected, 0, 0, 0, 0, 0],
         "live-bucket lenses")
    need(all(lens[3] < k - 2 for k in (5, 10)), "j3 live at K5/K10")

    # The origin is strictly inside abxw; every y is on that same sphere.
    weights = (Fraction(10, 27), Fraction(10, 27),
               Fraction(7, 54), Fraction(7, 54))
    need(sum(weights) == 1 and all(weight > 0 for weight in weights),
         "positive barycentric weights")
    need(all(sum(weight * p[i] for weight, p in zip(weights, (a, b, x, witness))) == 0
             for i in range(3)), "positive q4 centre")
    need(all(dot(p, p) == 625 * scale**2 for p in (a, b, x, *ys)),
         "common shell")

    # Scratch `std::find(grp.begin(),grp.end(),c2)` is run once for each
    # pending candidate, with grp containing all candidate event positions.
    # Their order does not affect the sum of the search positions 1+...+m.
    m = len(ys)
    probes = m * (m + 1) // 2
    declared_group_steps = (m + 31) // 32
    need(probes > declared_group_steps, "hidden membership work")
    print(f"L={scale} events={m} bucket=j3 membership_probes={probes} "
          f"declared_compare_steps={declared_group_steps} shell={m + 3}")


def main() -> None:
    for scale, expected in ((1, 60), (10, 282), (100, 1439)):
        check(scale, expected)


if __name__ == "__main__":
    main()
