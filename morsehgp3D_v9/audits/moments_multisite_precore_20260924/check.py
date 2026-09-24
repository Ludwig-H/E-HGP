#!/usr/bin/env python3
"""Petit contrôle exact du certificat multisite ; aucune dépendance ni HGP."""

from fractions import Fraction
from itertools import product


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def dot(x, y):
    return sum(a * b for a, b in zip(x, y))


def sub(x, y):
    return tuple(a - b for a, b in zip(x, y))


def cross(x, y):
    return (x[1] * y[2] - x[2] * y[1],
            x[2] * y[0] - x[0] * y[2],
            x[0] * y[1] - x[1] * y[0])


def moments(a, b, guards):
    n = len(guards)
    z = tuple(sum(g[i] for g in guards) for i in range(3))
    q = sum(dot(g, g) for g in guards)
    d = sub(b, a)
    s = tuple(a[i] + b[i] for i in range(3))
    D = dot(d, d)
    W = tuple(2 * z[i] - n * s[i] for i in range(3))
    H = 4 * (dot(s, z) - q - n * dot(a, b))
    X = dot(cross(d, W), cross(d, W))
    direct_H = n * D - sum(dot(sub(tuple(2 * g[i] for i in range(3)), s),
                               sub(tuple(2 * g[i] for i in range(3)), s))
                           for g in guards)
    need(H == direct_H, "identité des moments H")
    return D, H, W, X


def proves(D, H, X, K, lane):
    T = K - (1 if lane == 3 else 2)
    if T <= 0:
        return False
    if lane == 3:
        A = 3 * H - 4 * (T - 1) * D
        return A > 0 and A * A > 12 * X
    A = 2 * H - 3 * (T - 1) * D
    return A > 0 and A * A > 8 * X


def margin(a, b, g, t):
    s = tuple(a[i] + b[i] for i in range(3))
    c = tuple(Fraction(s[i], 2) + t[i] for i in range(3))
    return dot(sub(a, c), sub(a, c)) - dot(sub(g, c), sub(g, c))


def main():
    a, b = (5, 10, 10), (15, 10, 10)
    guards = tuple((10, 10 + u * p, 10 + v * q)
                   for p, q in ((1, 3), (3, 1))
                   for u, v in product((-1, 1), repeat=2))
    need(len(guards) == len(set(guards)) == 8, "huit IDs/sites distincts")
    D, H, W, X = moments(a, b, guards)
    need((D, H, W, X) == (100, 480, (0, 0, 0), 0), "moments K5")
    need(proves(D, H, X, 5, 3) and proves(D, H, X, 5, 4),
         "deux voies K5 prouvées")
    need(3 * H - 4 * 3 * D == 240 and 2 * H - 3 * 2 * D == 360,
         "valeurs A3/A4")

    for g in guards:
        t = (Fraction(0), -Fraction(4, 5) * (g[1] - 10),
             -Fraction(4, 5) * (g[2] - 10))
        need(dot(t, t) == Fraction(32, 5) < Fraction(D, 12),
             "centre témoin dans les deux disques")
        need(margin(a, b, g, t) == -1, "aucun garde universel isolé")

    for lane, T, bound in ((3, 4, Fraction(D, 12)),
                           (4, 3, Fraction(D, 8))):
        for iy, iz in product(range(-7, 8), repeat=2):
            t = (Fraction(0), Fraction(iy, 2), Fraction(iz, 2))
            if dot(t, t) <= bound:
                positive = sum(margin(a, b, g, t) > 0 for g in guards)
                need(positive >= T, "crédit à un centre rationnel")

    D7, H7, W7, X7 = moments(a, b, guards[:-1])
    need((D7, H7, X7) == (100, 420, 4000), "moment à sept gardes")
    need(not proves(D7, H7, X7, 5, 3) and proves(D7, H7, X7, 5, 4),
         "retrait indépendant des bits")
    need(not proves(D, 0, 0, 5, 3), "un bloc sans marge ne prouve rien")
    print("OK : moments, tests stricts, huit gardes et voie partielle")


if __name__ == "__main__":
    main()
