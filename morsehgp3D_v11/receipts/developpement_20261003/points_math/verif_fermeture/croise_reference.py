#!/usr/bin/env python3
"""Validation de vf_oracle contre l'oracle de reference (lecture seule, sans .pyc) : niveaux des noeuds FULL,
fermeture (u) pour m = 1..k+2, et ultrametriques des regles core, cover, first, margin1, margin, EC.
Ce n'est qu'un controle de MON code : les verifications des affirmations n'utilisent que vf_oracle.

    PYTHONDONTWRITEBYTECODE=1 python3 croise_reference.py --clouds 120 --seed 99
"""
import argparse
import random
import sys
import time
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-points-math/verif_fermeture')
REF = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/reference'
BENCH = '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/bench'
sys.path.insert(0, REF)
sys.path.insert(0, BENCH)
import vf_oracle as vo  # noqa: E402
from hgp11_ref import Definition  # noqa: E402
import points_reference as pr  # noqa: E402


def ref_closure(res, n, m):
    # fermeture recalculee sur les coupes de la reference (code minimal, ecrit ici)
    par = list(range(n))

    def f(x):
        while par[x] != x:
            par[x] = par[par[x]]
            x = par[x]
        return x
    act = [False] * n
    u = [[None] * n for _ in range(n)]
    for cut in res.cuts:
        for _v, cov, _c in cut.closed:
            if bin(cov).count('1') < m:
                continue
            s = vo.bits(cov)
            for i in s:
                act[i] = True
            for i in s[1:]:
                a, b = f(i), f(s[0])
                if a != b:
                    par[a] = b
        for i in range(n):
            for j in range(n):
                if act[i] and act[j] and u[i][j] is None and f(i) == f(j):
                    u[i][j] = cut.level
    return u


def ref_ec(res, n, m):
    ent = [None] * n
    for cut in res.cuts:
        for i in range(n):
            if ent[i] is None:
                cs = [v for v, cov, _c in cut.closed if cov >> i & 1 and bin(cov).count('1') >= m]
                if len(cs) == 1:
                    ent[i] = (cut.level, cs[0])
    return ent


def gen(rng):
    if rng.random() < 0.5:
        n = rng.randint(4, 8)
        side = rng.choice([3, 4, 6])
        sc = rng.choice([1, 7, 1000])
        s = set()
        while len(s) < n:
            s.add((rng.randrange(side) * sc, rng.randrange(side) * sc, rng.choice([0, rng.randrange(side) * sc])))
        return sorted(s)
    n = rng.randint(4, 8)
    s = set()
    while len(s) < n:
        s.add(tuple(rng.randrange(1000) for _ in range(3)))
    return sorted(s)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--clouds', type=int, default=120)
    ap.add_argument('--seed', type=int, default=99)
    a = ap.parse_args()
    rng = random.Random(a.seed)
    checks = bad = 0
    t0 = time.time()
    for _ in range(a.clouds):
        pts = gen(rng)
        n = len(pts)
        cl = vo.Cloud(pts)
        D = Definition(pts)
        for k in range(1, min(3, n - 1) + 1):
            F = vo.Full(cl, k)
            res = D.order(k)
            checks += 1
            if sorted(F.levels_of_node) != sorted(nd.level for nd in res.nodes):
                bad += 1
                print('NOEUDS', pts, k)
            for m in range(1, k + 3):
                if m > n:
                    continue
                u1, _ = F.closure(m)
                u2 = ref_closure(res, n, m)
                checks += 1
                if u1 != u2:
                    bad += 1
                    print('FERMETURE', pts, k, m)
                ref, tree = pr.reference_rules(res, n, m)
                mine = dict(core=F.hang_core(), cover=F.hang_first(1), first=F.hang_first(m),
                            margin1=F.hang_margin(1), margin=F.hang_margin(m))
                for rule in ('core', 'cover', 'first', 'margin1', 'margin'):
                    ua = F.ultra(mine[rule])
                    ub = pr.reference_ultrametric(ref[rule], tree)
                    checks += 1
                    if ua != ub:
                        bad += 1
                        print('REGLE', rule, pts, k, m)
                ua = F.ultra(F.hang_ec(m))
                ub = pr.reference_ultrametric(ref_ec(res, n, m), tree)
                checks += 1
                if ua != ub:
                    bad += 1
                    print('EC', pts, k, m)
    print('controles %d, ecarts %d, %.1f s' % (checks, bad, time.time() - t0))


if __name__ == '__main__':
    main()
