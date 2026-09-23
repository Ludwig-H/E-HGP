#!/usr/bin/env python3
"""Juge D1 : oracle exhaustif exact (rationnels) du catalogue v9 sur de petits nuages adverses.

Catalogue attendu : toutes les boules minimales (support S, 2 <= |S| <= 4, centre dans
l'interieur relatif de conv S), p = |interieur strict|, q_min = plus petite arite,
p + q_min <= Kmax + 1. Coquille > 12 dans le catalogue : refus attendu.
"""
import itertools, random, subprocess, sys
from fractions import Fraction as F

EXE = "./cat_dump"
M = 262143


def solve(mat, rhs):
    n = len(mat)
    a = [[F(x) for x in row] + [F(r)] for row, r in zip(mat, rhs)]
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


def expected(points, kmax):
    out = {}
    for q in (2, 3, 4):
        for S in itertools.combinations(points, q):
            p0 = S[0]
            vs = [tuple(s[t] - p0[t] for t in range(3)) for s in S[1:]]
            k = len(vs)
            gram = [[sum(vs[i][t] * vs[j][t] for t in range(3)) for j in range(k)] for i in range(k)]
            rhs = [F(sum(v[t] * v[t] for t in range(3)), 2) for v in vs]
            lam = solve(gram, rhs)
            if lam is None:
                continue
            bary = [1 - sum(lam)] + lam
            if any(b <= 0 for b in bary):
                continue
            c = tuple(F(p0[t]) + sum(lam[j] * vs[j][t] for j in range(k)) for t in range(3))
            r2 = sum((F(p0[t]) - c[t]) ** 2 for t in range(3))
            key = (c, r2)
            if key in out:
                out[key][0] = min(out[key][0], q)
                continue
            p = u = 0
            for z in points:
                d2 = sum((z[t] - c[t]) ** 2 for t in range(3))
                if d2 < r2:
                    p += 1
                elif d2 == r2:
                    u += 1
            out[key] = [q, p, u]
    return {key: v for key, v in out.items() if v[1] + v[0] <= kmax + 1}


def run_chain(points, kmax, workers, tower):
    txt = "".join("%d %d %d\n" % p for p in points)
    r = subprocess.run([EXE, str(kmax), str(workers), str(tower)], input=txt, capture_output=True, text=True)
    if r.returncode != 0:
        return ("crash:%d" % r.returncode, r.stderr[-300:]), {}
    lines = r.stdout.splitlines()
    status = lines[0].split()[1:]
    got = {}
    for ln in lines[1:]:
        if not ln.startswith("B "):
            continue
        a, b0, b1, b2, cc, p, q, u = map(int, ln.split()[1:])
        c = (F(-b0, 2 * a), F(-b1, 2 * a), F(-b2, 2 * a))
        r2 = sum(x * x for x in c) - F(cc, a)
        got[(c, r2)] = [q, p, u]
    return status, got


def judge(points, kmax, workers=1, tower=1, tag=""):
    exp = expected(points, kmax)
    status, got = run_chain(points, kmax, workers, tower)
    maxshell = max((v[2] for v in exp.values()), default=0)
    if status[0] != "complete_relative":
        if status[0] == "unsupported_degeneracy" and maxshell > 12:
            return "refused_ok", len(exp)
        return "BAD_STATUS %s maxshell=%d" % (status, maxshell), len(exp)
    if maxshell > 12:
        return "BAD accepted shell %d" % maxshell, len(exp)
    missing = [k for k in exp if k not in got]
    extra = [k for k in got if k not in exp]
    wrong = [k for k in exp if k in got and exp[k] != got[k]]
    if missing or extra or wrong:
        return "MISMATCH missing=%d extra=%d wrong=%d ex=%s" % (
            len(missing), len(extra), len(wrong),
            [(k, exp.get(k), got.get(k)) for k in (missing + extra + wrong)[:3]]), len(exp)
    return "ok", len(exp)
