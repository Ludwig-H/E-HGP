#!/usr/bin/env python3
"""Lentille 2 (auditeur C, v9) : invariant global K-NN => q2, controle exact.

Lemme : si z est strictement interieur a la boule diametrale de (a,b), alors
|z-a| < |b-a| et |z-b| < |b-a|. Donc p(a,b) <= #{z != a : |z-a| < |b-a|}.
Consequence : toute paire (a,b) dont b a moins de K sites strictement plus
proches de a (ou a moins de K sites strictement plus proches de b) a p < K et
DOIT etre emise par la voie q2 a Kmax=K, avec profondeur <= ce rang.

Ce script (entiers exacts, pas d'assert) :
 1. verifie le lemme sur des nuages aleatoires entiers (petites coordonnees,
    avec egalites de distances) contre la force brute ;
 2. mesure, sur des nuages synthetiques de type LiDAR (plans, lignes, bruit
    entier), la part des paires q2 acceptees (p<K) couvertes par l'invariant.
Diagnostic synthetique seulement : aucune conclusion LiDAR reelle.
"""
import random
import sys


def interior_count(pts, a, b):
    pa, pb = pts[a], pts[b]
    c = 0
    for i, z in enumerate(pts):
        if i == a or i == b:
            continue
        if sum((z[k] - pa[k]) * (pb[k] - z[k]) for k in range(3)) > 0:
            c += 1
    return c


def d2(p, q):
    return sum((p[k] - q[k]) ** 2 for k in range(3))


def closer_count(pts, a, b):
    r = d2(pts[a], pts[b])
    return sum(1 for i, z in enumerate(pts) if i != a and d2(pts[a], z) < r)


def cloud_random(rng, n, lim):
    s = set()
    while len(s) < n:
        s.add((rng.randint(0, lim), rng.randint(0, lim), rng.randint(0, lim)))
    return list(s)


def cloud_lidar_like(rng, n):
    s = set()
    while len(s) < n:
        kind = rng.random()
        if kind < 0.45:   # facade verticale x ~ 2000, bruit 1-3 mm
            p = (2000 + rng.randint(-3, 3), rng.randint(0, 4000), rng.randint(0, 3000))
        elif kind < 0.75:  # anneau de balayage (arc horizontal) sur un obstacle
            t = rng.random() * 1.2
            import math
            R = 6000
            p = (int(R * math.cos(t)) + rng.randint(-2, 2), int(R * math.sin(t)) + rng.randint(-2, 2), 1500 + rng.randint(-2, 2))
        elif kind < 0.9:   # poteau vertical
            p = (4000 + rng.randint(-40, 40), 3000 + rng.randint(-40, 40), rng.randint(0, 5000))
        else:              # vegetation diffuse
            p = (rng.randint(3000, 5000), rng.randint(0, 1500), rng.randint(0, 2500))
        s.add(p)
    return list(s)


def main():
    rng = random.Random(923)
    bad = 0
    tests = 0
    # 1. lemme contre force brute (egalites frequentes : lim=6)
    for _ in range(12):
        pts = cloud_random(rng, 24, 6)
        n = len(pts)
        for a in range(n):
            for b in range(a + 1, n):
                p = interior_count(pts, a, b)
                tests += 1
                if p > closer_count(pts, a, b) or p > closer_count(pts, b, a):
                    bad += 1
    print(flush=True)
    print(f"lemme p <= rang-1 : {tests} paires, violations={bad}")
    # 2. couverture de la sortie q2 par l'invariant K-NN
    for label, pts in (("aleatoire_u18_n110", cloud_random(rng, 110, 262143)),
                       ("lidar_like_n110", cloud_lidar_like(rng, 110))):
        n = len(pts)
        CC = {}
        for a in range(n):
            ds = sorted(d2(pts[a], z) for i, z in enumerate(pts) if i != a)
            import bisect
            for b in range(n):
                if b != a:
                    CC[(a, b)] = bisect.bisect_left(ds, d2(pts[a], pts[b]))
        P = {}
        for a in range(n):
            for b in range(a + 1, n):
                P[(a, b)] = interior_count(pts, a, b)
        for K in (5, 10):
            acc = [ab for ab, p in P.items() if p < K]
            knn = [ab for ab in acc if min(CC[(ab[0], ab[1])], CC[(ab[1], ab[0])]) < K]
            viol = [ab for ab, p in P.items()
                    if min(CC[(ab[0], ab[1])], CC[(ab[1], ab[0])]) < K and p >= K]
            bad += len(viol)
            print(f"{label} K={K}: q2 acceptees={len(acc)} ({len(acc)/n:.1f}/site), "
                  f"couvertes par K-NN={len(knn)} ({100*len(knn)/max(1,len(acc)):.1f} %), violations={len(viol)}", flush=True)
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
