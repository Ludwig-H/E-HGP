#!/usr/bin/env python3
"""Juge d'Euler independant (audit L01, 2 oct. 2026).

Identite (preuve purement combinatoire, sans theorie de Morse, valable sans position generale, sites distincts) :
pour tout k >= 1, chaque partie G de X avec |G| >= k est comptee une fois, en sa plus petite boule englobante b(G) :
    sum_{G, |G| >= k} (-1)^(|G|-k) C(|G|-1, k-1) = 1          (caracteristique d'Euler du nerf complet, un simplexe)
En regroupant par boule b (centre c, interieur strict I de cardinal p, coquille U) et en sommant sur J inclus dans I
(difference finie d'un polynome) :
    e_k(b) = sum_{B inclus dans U, c dans conv(B), |B| >= t} (-1)^(|B|-t) C(|B|-1, t-1),   t = k - p >= 1 (0 sinon)
    [k = 1] * n + sum_{b, r > 0} e_k(b) = 1.
Coquille reguliere (U = support de q points) : e_k(b) = (-1)^(q-t) C(q-1, t-1).
Les boules utiles a l'ordre k verifient p <= k - 1, donc p + q_min <= k + 3 : le catalogue a K' = k + 2 suffit.

Modes :
  petit  [graine] [nuages]              validation de l'identite sur petits nuages (toutes les boules critiques, brut)
  echelle RUN.json ETENDUES.txt KMAX    somme a l'echelle : comptes by_q_p du JSON du binaire (K' = KMAX + 2) et
                                        lignes de dump des seules coquilles etendues
"""
import itertools
import json
import random
import sys
from fractions import Fraction as Fr
from math import comb

sys.path.insert(0, __import__('os').path.dirname(__import__('os').path.abspath(__file__)))
import oracle_indep as O  # noqa: E402


def in_hull(c, pts):
    """c (Fractions) dans l'enveloppe convexe fermee de pts (entiers) : Caratheodory, simplexe affinement independant."""
    for q in range(1, min(4, len(pts)) + 1):
        for T in itertools.combinations(pts, q):
            a = T[0]
            if q == 1:
                if all(Fr(a[j]) == c[j] for j in range(3)):
                    return True
                continue
            D = [tuple(x - y for x, y in zip(t, a)) for t in T[1:]]
            G = [[sum(u * v for u, v in zip(D[i], D[j])) for j in range(q - 1)] for i in range(q - 1)]
            rhs = [sum(Fr(D[i][j]) * (c[j] - a[j]) for j in range(3)) for i in range(q - 1)]
            if q == 2:
                if G[0][0] == 0:
                    continue
                mu = [rhs[0] / G[0][0]]
            elif q == 3:
                d = G[0][0] * G[1][1] - G[0][1] * G[1][0]
                if d == 0:
                    continue
                mu = [(rhs[0] * G[1][1] - G[0][1] * rhs[1]) / d, (G[0][0] * rhs[1] - rhs[0] * G[1][0]) / d]
            else:
                d = O.det3(G)
                if d == 0:
                    continue
                mu = []
                for col in range(3):
                    M = [row[:] for row in G]
                    for r in range(3):
                        M[r][col] = rhs[r]
                    mu.append(O.det3(M) / d)
            if any(m < 0 for m in mu) or sum(mu) > 1:
                continue
            if all(Fr(a[j]) + sum(m * dd[j] for m, dd in zip(mu, D)) == c[j] for j in range(3)):
                return True
    return False


def e_ball(c, U, p, k):
    t = k - p
    if t < 1 or t > len(U):
        return 0
    tot = 0
    for s in range(t, len(U) + 1):
        coef = (-1) ** (s - t) * comb(s - 1, t - 1)
        for B in itertools.combinations(U, s):
            if in_hull(c, B):
                tot += coef
    return tot


def mode_petit(seed, count):
    rnd = random.Random(seed)
    checks = fails = balls_total = ext_total = 0
    for name, pts in O.families(rnd, count):
        pos = sorted(set(pts), key=O.morton)
        if len(pos) < 2 or len(pos) > 16:
            continue
        w = [1] * len(pos)
        balls = O.critical_balls(pos, w)
        if max(len(v[3]) for v in balls.values()) > 12:
            continue
        balls_total += len(balls)
        ext_total += sum(1 for v in balls.values() if len(v[3]) > v[0])
        cache = {}
        for k in range(1, len(pos) + 1):
            tot = len(pos) if k == 1 else 0
            for (c, r2), (q, S, I, U) in balls.items():
                if len(U) == q:
                    t = k - len(I)
                    tot += (-1) ** (q - t) * comb(q - 1, t - 1) if 1 <= t <= q else 0
                else:
                    tot += e_ball(c, [pos[s] for s in U], len(I), k)
            checks += 1
            if tot != 1:
                fails += 1
                print('ECART Euler %s k=%d somme=%d n=%d %s' % (name, k, tot, len(pos), pos))
    print('euler_petit checks %d fails %d boules %d etendues %d' % (checks, fails, balls_total, ext_total))
    return 1 if fails else (3 if checks < 200 or ext_total < 100 else 0)


def mode_echelle(run_json, ext_path, kmax):
    d = json.load(open(run_json))
    n, kcat = d['sites'], d['K']
    if d['n'] != d['sites'] or d['weighted'] != 0:
        print('entree ponderee : juge non applicable')
        return 2
    if kcat < kmax + 2:
        print('catalogue K\'=%d insuffisant pour juger k <= %d' % (kcat, kmax))
        return 2
    by = {q: list(d['by_q_p']['q%d' % q]) for q in (2, 3, 4)}
    ext = []
    for line in open(ext_path):
        head, sup, inner, shell = line.rstrip('\n').split('|')
        rank, q, p, u, flags = (int(t) for t in head.split())
        conv = lambda s: [tuple(int(v) for v in t.split(',')) for t in s.split()]  # noqa: E731
        S, I, U = conv(sup), conv(inner), conv(shell)
        if not flags & 1:
            continue
        b = O.ball_of(tuple(range(len(S))), S)
        if b is None:
            print('support invalide dans le dump : %s' % line)
            return 1
        ext.append((b[0], U, p))
        by[q][p] -= 1
    if len(ext) != d['extended']:
        print('coquilles etendues : %d lues contre %d annoncees' % (len(ext), d['extended']))
        return 1
    ok = True
    for k in range(1, kmax + 1):
        reg = n if k == 1 else 0
        for q in (2, 3, 4):
            for p, cnt in enumerate(by[q]):
                t = k - p
                if 1 <= t <= q:
                    reg += (-1) ** (q - t) * comb(q - 1, t - 1) * cnt
        e_ext = sum(e_ball(c, U, p, k) for c, U, p in ext)
        tot = reg + e_ext
        print('k=%2d somme reguliere %8d + etendues %6d = %d' % (k, reg, e_ext, tot))
        ok = ok and tot == 1
    print('euler_echelle %s sites=%d boules=%d etendues=%d kcat=%d ordres 1..%d : %s'
          % (run_json, n, d['balls'], len(ext), kcat, kmax, 'CONFORME' if ok else 'ECART'))
    return 0 if ok else 1


if __name__ == '__main__':
    if sys.argv[1] == 'petit':
        sys.exit(mode_petit(int(sys.argv[2]) if len(sys.argv) > 2 else 7, int(sys.argv[3]) if len(sys.argv) > 3 else 3))
    sys.exit(mode_echelle(sys.argv[2], sys.argv[3], int(sys.argv[4])))
