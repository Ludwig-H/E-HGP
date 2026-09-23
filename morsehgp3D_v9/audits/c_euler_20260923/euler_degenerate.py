#!/usr/bin/env python3
"""Auditeur C (v9) -- contribution exacte d'une boule degeneree a chi(sous-niveau de d_K).

Pour un centre c, p interieurs stricts et une coquille U (u points), la contribution a
l'ordre K (m = K - p, 1 <= m <= u) vaut 1 - chi(L_m), ou
L_m = { v dans S^2 : #{x dans U : <v, x - c> > 0} >= m }  (ouvert de S^2).
chi(L_m) = somme des (-1)^dim des cellules ouvertes de l'arrangement des grands cercles
(x - c)^perp contenues dans L_m (chi = chi_c pour un ouvert d'une surface).
Directions entieres exactes : D = 2a x + b (c = -b/(2a), a > 0).
Usage : euler_degenerate.py degenerate.jsonl Kmax generic_sum_json
"""
import json
import sys
from math import gcd


def dot(u, v):
    return u[0] * v[0] + u[1] * v[1] + u[2] * v[2]


def cross(u, v):
    return (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])


def prim(v):
    g = 0
    for t in v:
        g = gcd(g, abs(t))
    if g == 0:
        raise ValueError("null vector")
    return (v[0] // g, v[1] // g, v[2] // g)


def neg(v):
    return (-v[0], -v[1], -v[2])


def add(u, v):
    return (u[0] + v[0], u[1] + v[1], u[2] + v[2])


def f_count(v, dirs):
    return sum(1 for d in dirs if dot(v, d) > 0)


def sign_vector(v, dirs):
    return tuple((dot(v, d) > 0) - (dot(v, d) < 0) for d in dirs)


def chi_cells(dirs, m):
    """Return chi(L_m) by exact cell enumeration of the great-circle arrangement."""
    # Distinct circles: normals up to sign.
    normals = []
    for d in dirs:
        n = prim(d)
        if n not in normals and neg(n) not in normals:
            normals.append(n)
    v_in = e_in = 0
    faces = set()
    # Vertices: +-(n_i x n_j), deduplicated as primitive directions.
    verts = set()
    for i in range(len(normals)):
        for j in range(i + 1, len(normals)):
            w = cross(normals[i], normals[j])
            if w == (0, 0, 0):
                continue
            w = prim(w)
            verts.add(w)
            verts.add(neg(w))
    for w in verts:
        if f_count(w, dirs) >= m:
            v_in += 1
    for n in normals:
        on = [w for w in verts if dot(w, n) == 0]
        # directions of dirs lying on this circle's normal line (parallel / antiparallel)
        plus = sum(1 for d in dirs if cross(d, n) == (0, 0, 0) and dot(d, n) > 0)
        minus = sum(1 for d in dirs if cross(d, n) == (0, 0, 0) and dot(d, n) < 0)
        if not on:
            # whole circle is one cell (S^1, chi_c = 0); faces on both sides.
            e1 = prim(cross(n, (1, 0, 0))) if cross(n, (1, 0, 0)) != (0, 0, 0) else prim(cross(n, (0, 1, 0)))
            base = sign_vector(e1, dirs)
            for side, extra in ((1, plus), (-1, minus)):
                sv = tuple((side * (1 if dot(d, n) > 0 else -1)) if cross(d, n) == (0, 0, 0) else base[k]
                           for k, d in enumerate(dirs))
                faces.add(sv)
            continue
        # order vertices cyclically around n
        e1 = on[0]
        e2 = cross(n, e1)

        def key(w):
            x, y = dot(w, e1), dot(w, e2)
            # half: 0 for angle in [0, pi), 1 for [pi, 2pi)
            half = 0 if (y > 0 or (y == 0 and x > 0)) else 1
            return half, w

        import functools

        def cmp(a, b):
            ha, hb = key(a)[0], key(b)[0]
            if ha != hb:
                return ha - hb
            s = dot(n, cross(a, b))
            return -1 if s > 0 else (1 if s < 0 else 0)

        on.sort(key=functools.cmp_to_key(cmp))
        k = len(on)
        for idx in range(k):
            a, b = on[idx], on[(idx + 1) % k]
            if cross(a, b) == (0, 0, 0):
                # antipodal consecutive vertices (only two on circle): midpoint = n x a oriented
                mid = cross(n, a)
            else:
                # a and b separated by < pi along the positive orientation (dot(n, a x b) > 0)
                if dot(n, cross(a, b)) > 0:
                    mid = add(a, b)
                else:
                    mid = neg(add(a, b))
            if f_count(mid, dirs) >= m:
                e_in += 1
            base = sign_vector(mid, dirs)
            for side in (1, -1):
                sv = tuple((side * (1 if dot(d, n) > 0 else -1)) if cross(d, n) == (0, 0, 0) else base[t]
                           for t, d in enumerate(dirs))
                faces.add(sv)
    f_in = sum(1 for sv in faces if sum(1 for s in sv if s > 0) >= m)
    return v_in - e_in + f_in


def contribution(ball, k):
    p, u = ball["p"], ball["u"]
    m = k - p
    if m < 1 or m > u:
        return 0
    a = int(ball["a"])
    b = [int(t) for t in ball["b"]]
    dirs = [tuple(2 * a * x[i] + b[i] for i in range(3)) for x in ball["shell"]]
    return 1 - chi_cells(dirs, m)


def generic(p, q, k):
    # support de q points en position generale, centre dans l'interieur relatif :
    # contribution (-1)^(q-m) * C(q-1, m-1), m = k - p (multifusion : C(q-1, m-1) branches)
    from math import comb
    m = k - p
    if m < 1 or m > q:
        return 0
    return (-1) ** (q - m) * comb(q - 1, m - 1)


def main():
    path, kmax = sys.argv[1], int(sys.argv[2])
    generic_sum = json.loads(sys.argv[3])
    n_sites = int(sys.argv[4]) if len(sys.argv) > 4 else None
    totals = [0] * (kmax + 1)
    n = 0
    with open(path) as fh:
        for line in fh:
            ball = json.loads(line)
            n += 1
            if ball["u"] != len(ball["shell"]):
                raise SystemExit("shell recount mismatch")
            for k in range(1, kmax + 1):
                totals[k] += contribution(ball, k)
    out = {"degenerate_balls": n, "euler_by_k": {}}
    for k in range(1, kmax + 1):
        out["euler_by_k"][k] = generic_sum[k - 1] + totals[k] + (n_sites if (k == 1 and n_sites) else 0)
    out["checkable_k"] = [k for k in range(1, max(0, kmax - 2) + 1) if k > 1 or n_sites]
    out["pass"] = all(out["euler_by_k"][k] == 1 for k in out["checkable_k"])
    print(json.dumps(out))
    return 0 if out["pass"] else 1


if __name__ == "__main__":
    # self-test: generic formula agrees with cell computation on nondegenerate supports
    tests = [
        ([(1, 0, 0), (-1, 0, 0)], 0),
        ([(2, 0, 0), (-1, 2, 0), (-1, -2, 0)], 0),
        ([(1, 1, 1), (1, -1, -1), (-1, 1, -1), (-1, -1, 1)], 0),
        ([(5, 1, 0), (-3, 4, 1), (-2, -5, 1), (0, 0, -7)], 0),
    ]
    for dirs, p in tests:
        q = len(dirs)
        for k in range(p + 1, p + q + 1):
            got = 1 - chi_cells(dirs, k - p)
            want = generic(p, q, k)
            if got != want:
                raise SystemExit(f"selftest failed dirs={dirs} k={k} got={got} want={want}")
    # degenerate hand-derived case: diametral pair plus a third shell point
    dirs = [(1, 0, 0), (-1, 0, 0), (0, 1, 0)]
    got = [1 - chi_cells(dirs, m) for m in (1, 2, 3)]
    if got != [0, -1, 1]:
        raise SystemExit(f"selftest degenerate failed got={got}")
    sys.exit(main())
