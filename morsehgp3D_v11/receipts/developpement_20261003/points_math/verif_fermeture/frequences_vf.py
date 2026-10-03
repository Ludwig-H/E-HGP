#!/usr/bin/env python3
"""Rejoue la statistique two_blobs de frequences.py (graine 11, 900 nuages) avec vf_oracle.
Les GENERATEURS sont recopies a l'identique (meme suite aleatoire) ; tout le calcul est celui de vf_oracle.
Mesures : reunion des germes (plus petit D_k, ex aequo par indice) selon la fermeture et selon FULL (lignees des
germes, comme le rapport) ; en plus, la premiere fusion FULL entre une lignee de coeur d'un site de A et une d'un
site de B (tous germes), et la fraction de A avant reunion (fermeture, FULL par lignee de germe)."""
import math
import random
import statistics
import sys
import time
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-points-math/verif_fermeture')
import vf_oracle as vo
from fractions import Fraction


def cloud_gate(rng):
    n = rng.randint(4, 9)
    side = rng.choice([3, 4, 6, 10, 40])
    scale = rng.choice([1, 7, 1000])
    pts = set()
    while len(pts) < n:
        pts.add((rng.randrange(side) * scale, rng.randrange(side) * scale,
                 rng.choice([0, rng.randrange(side) * scale])))
    pts = sorted(pts)
    rng.shuffle(pts)
    return pts


def cloud_generic(rng, nmin=5, nmax=9, box=1000):
    n = rng.randint(nmin, nmax)
    pts = set()
    while len(pts) < n:
        pts.add(tuple(rng.randrange(box) for _ in range(3)))
    pts = sorted(pts)
    rng.shuffle(pts)
    return pts


def cloud_two_blobs(rng):
    na, nb = rng.choice([3, 4]), rng.choice([3, 4])
    nv = rng.choice([1, 2])
    t = rng.randint(80, 200)
    big = rng.randint(900, 1500)
    pts, groups = [], ([], [], [])
    used = set()

    def add(p, g):
        if p in used:
            return False
        used.add(p)
        groups[g].append(len(pts))
        pts.append(p)
        return True
    for g, (cx, cnt) in enumerate(((0, na), (big, nb))):
        while len(groups[g]) < cnt:
            add((cx + rng.randrange(t), rng.randrange(t), rng.randrange(t)), g)
    while len(groups[2]) < nv:
        add((rng.randrange(t, big), rng.randrange(-t, 2 * t), rng.randrange(-t, 2 * t)), 2)
    return pts, groups[0], groups[1], groups[2]


def main():
    rng = random.Random(11)
    t0 = time.time()
    stats = {}
    for c in range(900):
        gen = ('gate', 'generic', 'two_blobs')[c % 3]
        if gen == 'gate':
            cloud_gate(rng)
            continue
        if gen == 'generic':
            cloud_generic(rng)
            continue
        pts, A, B, V = cloud_two_blobs(rng)
        n = len(pts)
        cl = vo.Cloud(pts)
        for k in (2, 3):
            if k >= n:
                continue
            F = vo.Full(cl, k)
            core = F.hang_core()

            def seed(group):
                return min(group, key=lambda i: (cl.Dk(i, k), i))
            sa, sb = seed(A), seed(B)

            def lin_merge(ia, ib):
                w = F.lca(core[ia][1], core[ib][1])
                return max(F.levels_of_node[w], core[ia][0], core[ib][0])
            mf = lin_merge(sa, sb)
            mf_any = min(lin_merge(a, b) for a in A for b in B)
            for m in sorted(set([1, k + 1, k + 2])):
                if m > n:
                    continue
                u, w = F.closure(m)
                mc = u[sa][sb]
                key = 'k=%d m=%d' % (k, m)
                s = stats.setdefault(key, dict(clouds=0, before=0, equal=0, after=0, ratios=[],
                                               before_any=0, ratios_any=[]))
                s['clouds'] += 1
                if mc < mf:
                    s['before'] += 1
                    s['ratios'].append(math.sqrt(mc / mf))
                elif mc == mf:
                    s['equal'] += 1
                else:
                    s['after'] += 1
                if mc < mf_any:
                    s['before_any'] += 1
                    s['ratios_any'].append(math.sqrt(mc / mf_any))
    print('two_blobs, graine 11, %.1f s' % (time.time() - t0))
    for key in sorted(stats):
        s = stats[key]
        r = s['ratios']
        ra = s['ratios_any']
        print('%s : nuages %d ; fermeture avant FULL(lignees des germes) %d, egale %d, apres %d ; rapport rayon min %s'
              ' med %s | avant la 1re fusion FULL A/B (tous germes) %d ; rapport min %s med %s' % (
                  key, s['clouds'], s['before'], s['equal'], s['after'],
                  '%.4f' % min(r) if r else '-', '%.4f' % statistics.median(r) if r else '-',
                  s['before_any'], '%.4f' % min(ra) if ra else '-', '%.4f' % statistics.median(ra) if ra else '-'))


if __name__ == '__main__':
    main()
