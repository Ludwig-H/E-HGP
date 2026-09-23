#!/usr/bin/env python3
"""Auditeur C -- oracle exhaustif independant de l'invariant d'Euler par ordre K.

Pour n <= 9 points entiers distincts, enumere toutes les boules minimales (MEB d'un support
S, |S| <= 4, centre dans l'interieur relatif de conv S), en rationnels exacts, puis verifie
pour chaque K = 1..n : somme des contributions de Morse = chi(R^3) = 1, avec la contribution
generique (-1)^(q-m) C(q-1, m-1) si la coquille egale le support, et 1 - chi(L_m) sinon.
Aucune dependance au code v9.
"""
import itertools
import random
import sys
from fractions import Fraction as F

import importlib.util

spec = importlib.util.spec_from_file_location("e", __file__.replace("euler_oracle.py", "euler_degenerate.py"))
E = importlib.util.module_from_spec(spec)
spec.loader.exec_module(E)


def solve(mat, rhs):
    n = len(mat)
    a = [list(map(F, row)) + [F(r)] for row, r in zip(mat, rhs)]
    for col in range(n):
        piv = next((r for r in range(col, n) if a[r][col] != 0), None)
        if piv is None:
            return None
        a[col], a[piv] = a[piv], a[col]
        for r in range(n):
            if r != col and a[r][col] != 0:
                f = a[r][col] / a[col][col]
                a[r] = [x - f * y for x, y in zip(a[r], a[col])]
    return [a[i][n] / a[i][i] for i in range(n)]


def meb_center(S):
    """Circumcenter of S within its affine hull, barycentric coords; None if degenerate."""
    p0 = S[0]
    vs = [tuple(s[i] - p0[i] for i in range(3)) for s in S[1:]]
    k = len(vs)
    # center = p0 + sum lam_j v_j with (c - p0).v_i = |v_i|^2 / 2
    gram = [[sum(vs[i][t] * vs[j][t] for t in range(3)) for j in range(k)] for i in range(k)]
    rhs = [F(sum(v[t] * v[t] for t in range(3)), 2) for v in vs]
    lam = solve(gram, rhs)
    if lam is None:
        return None, None
    c = tuple(F(p0[t]) + sum(lam[j] * vs[j][t] for j in range(k)) for t in range(3))
    bary = [1 - sum(lam)] + lam
    return c, bary


def balls(points):
    out = {}
    for q in (2, 3, 4):
        for S in itertools.combinations(points, q):
            c, bary = meb_center(list(S))
            if c is None or any(b <= 0 for b in bary):
                continue  # centre hors de l'interieur relatif : pas un support minimal
            r2 = sum((S[0][t] - c[t]) ** 2 for t in range(3))
            key = (c, r2)
            if key in out:
                continue
            p = 0
            shell = []
            for z in points:
                d2 = sum((z[t] - c[t]) ** 2 for t in range(3))
                if d2 < r2:
                    p += 1
                elif d2 == r2:
                    shell.append(z)
            out[key] = (q, p, shell, c)
    # q_min : plus petite arite qui presente la boule
    return out


def contribution(q, p, shell, c, k):
    u = len(shell)
    m = k - p
    if m < 1 or m > u:
        return 0
    if u == q:
        from math import comb
        return (-1) ** (q - m) * comb(q - 1, m - 1)
    den = 1
    for t in range(3):
        den = den * c[t].denominator // __import__("math").gcd(den, c[t].denominator)
    dirs = [tuple(int((z[t] - c[t]) * den) for t in range(3)) for z in shell]
    return 1 - E.chi_cells(dirs, m)


def check(points):
    bs = balls(points)
    n = len(points)
    res = []
    for k in range(1, n + 1):
        s = sum(contribution(q, p, sh, c, k) for (c, r2), (q, p, sh, _) in bs.items())
        if k == 1:
            s += n  # minima de d_1 : les sites eux-memes (boules q=1 de rayon nul)
        res.append(s)
    return res, bs


def main():
    rng = random.Random(int(sys.argv[1]) if len(sys.argv) > 1 else 1)
    trials = int(sys.argv[2]) if len(sys.argv) > 2 else 200
    bad = 0
    degenerate_seen = 0
    for t in range(trials):
        n = rng.randint(4, 10)
        span = rng.choice([2, 3, 4, 50, 1000])  # petites grilles : cosphericites frequentes
        pts = set()
        while len(pts) < n:
            pts.add(tuple(rng.randint(0, span) for _ in range(3)))
        pts = sorted(pts)
        res, bs = check(pts)
        degenerate_seen += sum(1 for (q, p, sh, c) in bs.values() if len(sh) != q)
        # le sous-niveau de d_K est vide ou R^3 : l'invariant ne vaut que pour K <= n
        # points coplanaires : d_K reste propre, chi(R^3) = 1 aussi
        if any(v != 1 for v in res):
            bad += 1
            print("FAIL", pts, res)
            if bad > 5:
                break
    print({"trials": trials, "failures": bad, "degenerate_balls_seen": degenerate_seen})
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
