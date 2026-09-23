#!/usr/bin/env python3
"""Oracle exact (fractions) du catalogue attendu de la chaine v9.

Pour chaque sous-ensemble S de 2 a 4 sites dont la boule circonscrite de
centre dans aff(S) a des coordonnees barycentriques STRICTEMENT positives
(meme critere que oracle/tower/local_plateau_oracle.hpp:77-99), on forme la
boule ; on recense interieur strict et coquille ; q_min = plus petite taille
de support positif donnant la meme boule. Une boule est au catalogue a Kmax
si |I| + q_min <= min(Kmax + 1, n) (fenetre de rang, tower_chain.cpp:493-494).
Cle : multiple entier primitif de (1, -2c, |c|^2 - r^2), A > 0
(tests/tower/full_ball_tower_gate.cpp:66-83).
"""
from fractions import Fraction as F
from itertools import combinations
from math import gcd
import json, sys

def solve(M, rhs):
    n = len(rhs)
    A = [row[:] + [rhs[i]] for i, row in enumerate(M)]
    for col in range(n):
        piv = next((r for r in range(col, n) if A[r][col] != 0), None)
        if piv is None:
            return None
        A[col], A[piv] = A[piv], A[col]
        d = A[col][col]
        A[col] = [v / d for v in A[col]]
        for r in range(n):
            if r != col and A[r][col] != 0:
                m = A[r][col]
                A[r] = [a - m * b for a, b in zip(A[r], A[col])]
    return [A[i][n] for i in range(n)]

def support_ball(P, S):
    base = [F(v) for v in P[S[0]]]
    E = [[F(P[j][a]) - base[a] for a in range(3)] for j in S[1:]]
    dot = lambda u, v: sum(x * y for x, y in zip(u, v))
    G = [[dot(e, f) for f in E] for e in E]
    rhs = [dot(e, e) / 2 for e in E]
    w = solve(G, rhs)
    if w is None or any(x <= 0 for x in w) or 1 - sum(w) <= 0:
        return None
    c = [base[a] + sum(w[i] * E[i][a] for i in range(len(E))) for a in range(3)]
    r2 = sum((c[a] - base[a]) ** 2 for a in range(3))
    return tuple(c), r2

def key(c, r2):
    coef = [F(1), -2 * c[0], -2 * c[1], -2 * c[2], sum(x * x for x in c) - r2]
    den = 1
    for x in coef:
        den = den * x.denominator // gcd(den, x.denominator)
    ints = [int(x * den) for x in coef]
    g = 0
    for v in ints:
        g = gcd(g, abs(v))
    return tuple(v // g for v in ints)

def balls(P):
    n = len(P)
    out = {}
    for q in (2, 3, 4):
        for S in combinations(range(n), q):
            b = support_ball(P, S)
            if b is None:
                continue
            k = key(*b)
            if k not in out:
                c, r2 = b
                d2 = [sum((F(P[i][a]) - c[a]) ** 2 for a in range(3)) for i in range(n)]
                inner = [i for i in range(n) if d2[i] < r2]
                shell = [i for i in range(n) if d2[i] == r2]
                out[k] = {"qmin": q, "interior": inner, "shell": shell, "supports": 0}
            out[k]["supports"] += 1
    return out

def catalogue(P, kmax):
    n = len(P)
    return {k: v for k, v in balls(P).items() if len(v["interior"]) + v["qmin"] <= min(kmax + 1, n)}

if __name__ == "__main__":
    fixtures = json.load(open(sys.argv[1]))
    for name, P in fixtures.items():
        allb = balls(P)
        row = [name, len(P), len(allb)]
        for K in (1, 2, 3, 5, 10):
            row.append(sum(1 for v in allb.values() if len(v["interior"]) + v["qmin"] <= min(K + 1, len(P))))
        print(*row)
