#!/usr/bin/env python3
"""Monte Carlo synthetique LEGER (annexe de fouille/v7.md, idee v7-3) - aucune donnee du depot, aucune execution native.

Compare, pour H(y) = D*|y-a|^2 - 2*N.(y-a) (forme ancree de la v11, a = site d'ancrage SUR la sphere,
c = a + N/D) sur une boite entiere [lo, hi] :
  - les bornes SEPAREES de la v11 (src/num/predicates.cpp, bound_terms l. 104 et native_bounds l. 132 :
    minimum du terme quadratique + minimum du terme lineaire, pris separement ; idem pour les maxima) ;
  - les extrema EXACTS sur la boite entiere par minimiseur entier par axe (AxisBounds de la v7,
    morsehgp3D_v7/src/pipeline/census.hpp l. 61-100) : min_j f_j(clip(m_j)), max_j max(f_j(lo_j), f_j(hi_j)),
    f_j(t) = D*t^2 - 2*N_j*t, H etant separable par axe.
Une boite est "decidee" si sa borne basse est > 0 (exterieure) ou sa borne haute < 0 (interieure).
Spheres q2 (paire diametrale entiere), rayon ~ 10-100 unites ; boites de cote w = (w/r) * r placees a une distance
r + U(-1.5w, 2w) du centre (bande proche de la surface, la ou le parcours census descend).
Ce n'est PAS une mesure du census v11 sur LiDAR : c'est une estimation de la largeur relative des bandes non decidees.

    python3 -B v7_bornes_mc.py      (une seconde environ, code 0 ; refus code 1 si une borne exacte sort des bornes separees)
"""
import random
import sys


def separated(D, N, a, lo, hi):
    nl = nu = ll = lu = 0
    for j in range(3):
        l, h = lo[j] - a[j], hi[j] - a[j]
        l2, h2 = l * l, h * h
        nl += l2 if l > 0 else (h2 if h < 0 else 0)
        nu += max(l2, h2)
        ll += N[j] * (-2 * (h if N[j] >= 0 else l))
        lu += N[j] * (-2 * (l if N[j] >= 0 else h))
    return D * nl + ll, D * nu + lu


def exact(D, N, a, lo, hi):
    mn = mx = 0
    for j in range(3):
        l, h = lo[j] - a[j], hi[j] - a[j]

        def f(t, j=j):
            return D * t * t - 2 * N[j] * t
        m = N[j] // D  # minimiseur reel N_j/D : le minimiseur entier est m ou m+1
        mn += min(f(min(max(m, l), h)), f(min(max(m + 1, l), h)))
        mx += max(f(l), f(h))
    return mn, mx


def decided(bounds):
    return bounds[0] > 0 or bounds[1] < 0


def main():
    rng = random.Random(7)
    print('w/r    boites  decidees(separees)  decidees(exactes)  non decidees : separees -> exactes')
    for scale in (0.25, 0.5, 1.0, 2.0, 4.0, 8.0):
        total = sep = exa = 0
        for _ in range(4000):
            a = [rng.randint(0, 1000) for _ in range(3)]
            d = [rng.randint(-120, 120) for _ in range(3)]
            if d == [0, 0, 0]:
                continue
            D, N = 2, d[:]  # c = a + N/D, sphere de diametre [a, a+d]
            r = (sum(x * x for x in d) ** 0.5) / 2
            w = max(1, int(scale * r))
            c = [a[j] + N[j] / D for j in range(3)]
            u = [rng.gauss(0, 1) for _ in range(3)]
            nu = sum(x * x for x in u) ** 0.5
            dist = r + rng.uniform(-1.5 * w, 2.0 * w)
            p = [c[j] + u[j] / nu * dist for j in range(3)]
            lo = [int(p[j] - w / 2) for j in range(3)]
            hi = [lo[j] + w for j in range(3)]
            s, e = separated(D, N, a, lo, hi), exact(D, N, a, lo, hi)
            if e[0] < s[0] or e[1] > s[1]:
                print('refus : borne exacte hors des bornes separees', a, d, lo, hi, s, e)
                return 1
            total += 1
            sep += decided(s)
            exa += decided(e)
        print('%4.2f  %6d  %17.1f %%  %16.1f %%   %5.1f %% -> %5.1f %%' % (
            scale, total, 100 * sep / total, 100 * exa / total, 100 * (total - sep) / total, 100 * (total - exa) / total))
    return 0


if __name__ == '__main__':
    sys.exit(main())
