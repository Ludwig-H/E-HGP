#!/usr/bin/env python3
"""L04 audit : erreur absolue de la distance carree approchee de SiteTree / tower (modele IEEE double exact).

Reproduit approx_center (cq = a + N / D en double) et approx_d2 (dx*dx + dy*dy + dz*dz, sans FMA) avec les flottants
Python (binary64, arrondi au plus proche), et compare a la valeur exacte (fractions). Centres : circonscrits a des
triplets / quadruplets de sites, retenus seulement s'ils sont dans la boite [0, M]^3 (cas de la tour : centre dans
l'enveloppe convexe), puis, a part, a distance croissante de la boite.
"""
import random
import sys
from fractions import Fraction

rng = random.Random(42)


def sub(p, q):
    return (p[0] - q[0], p[1] - q[1], p[2] - q[2])


def dot(p, q):
    return p[0] * q[0] + p[1] * q[1] + p[2] * q[2]


def cross(p, q):
    return (p[1] * q[2] - p[2] * q[1], p[2] * q[0] - p[0] * q[2], p[0] * q[1] - p[1] * q[0])


def center3(a, b, c):
    u, v = sub(b, a), sub(c, a)
    w = cross(u, v)
    if w == (0, 0, 0):
        return None
    uu, vv = dot(u, u), dot(v, v)
    t = tuple(uu * v[i] - vv * u[i] for i in range(3))
    return cross(t, w), 2 * dot(w, w)


def center4(a, b, c, d):
    u, v, s = sub(b, a), sub(c, a), sub(d, a)
    det = dot(u, cross(v, s))
    if det == 0:
        return None
    uu, vv, ss = dot(u, u), dot(v, v), dot(s, s)
    vs, su, uv = cross(v, s), cross(s, u), cross(u, v)
    N = tuple(uu * vs[i] + vv * su[i] + ss * uv[i] for i in range(3))
    D = 2 * det
    if D < 0:
        D, N = -D, tuple(-x for x in N)
    return N, D


def approx_center(a, N, D):
    Df = float(D)
    return tuple(float(a[i]) + float(N[i]) / Df for i in range(3))


def approx_d2(q, p):
    dx, dy, dz = float(p[0]) - q[0], float(p[1]) - q[1], float(p[2]) - q[2]
    return dx * dx + dy * dy + dz * dz


def exact_d2(a, N, D, p):
    return sum((Fraction(p[i]) - Fraction(a[i]) - Fraction(N[i], D)) ** 2 for i in range(3))


def run(bits, trials):
    M = (1 << bits) - 1

    def pt():
        r = rng.random()
        if r < 0.4:
            return tuple(rng.choice((0, M)) - rng.choice((0, M)) // max(1, rng.choice((1, 1, 1, M))) * 0 + 0 if False else
                         min(M, max(0, rng.choice((0, M)) + rng.randint(-4, 4))) for _ in range(3))
        return tuple(rng.randint(0, M) for _ in range(3))

    worst, worst_cfg, kept = 0.0, None, 0
    for _ in range(trials):
        a = pt()
        if rng.random() < 0.5:
            r = center3(a, pt(), pt())
        else:
            r = center4(a, pt(), pt(), pt())
        if r is None:
            continue
        N, D = r
        c = tuple(Fraction(a[i]) + Fraction(N[i], D) for i in range(3))
        if not all(0 <= c[i] <= M for i in range(3)):
            continue
        kept += 1
        q = approx_center(a, N, D)
        for p in [a] + [pt() for _ in range(6)] + [(0, 0, 0), (M, M, M), (M, 0, M), (0, M, 0)]:
            err = abs(Fraction(approx_d2(q, p)) - exact_d2(a, N, D, p))
            if err > worst:
                worst, worst_cfg = err, (a, p)
    return kept, float(worst)


for bits in (18, 19, 20, 21, 24):
    kept, w = run(bits, 60000)
    print('bits=%d centres_dans_la_boite=%d erreur_abs_max=%.3e  (kMargin/2 = 1e-2 ; borne annoncee 1e-3) %s'
          % (bits, kept, w, 'OK' if w < 1e-2 else 'DEPASSE kMargin/2'))

# centre a distance croissante : triangle isocele aplati a = (0,0,0), b = (L, h, 0), c = (2L, 0, 0) en u18
print('centres eloignes (u18), a=(0,0,0), b=(L,h,0), c=(2L,0,0), L=100000 :')
M = (1 << 18) - 1
for h in (60000, 20000, 5000, 1000, 200, 50, 10, 1):
    a, b, c = (0, 0, 0), (100000, h, 0), (200000, 0, 0)
    N, D = center3(a, b, c)
    q = approx_center(a, N, D)
    R = float(Fraction(dot(N, N), D * D)) ** 0.5
    w = 0.0
    for _ in range(4000):
        p = tuple(rng.randint(0, M) for _ in range(3))
        w = max(w, float(abs(Fraction(approx_d2(q, p)) - exact_d2(a, N, D, p))))
    print('  h=%6d rayon=%.3e erreur_abs_max=%.3e %s' % (h, R, w, 'OK' if w < 1e-2 else 'DEPASSE kMargin/2'))
