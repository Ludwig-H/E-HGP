#!/usr/bin/env python3
"""Classification du § 4.1 du rapport recodee depuis sa SEULE description (vf_oracle) :
pour chaque fusion FULL nu (niveau h) et paire d'enfants (s, t) : U = premier niveau de coupe < h ou la fermeture
place dans un meme bloc des couvertures qualifiees des deux lignees ; S(a), T(a) = reunions des couvertures des noeuds
vivants des sous-arbres ; mu = max_{a in [U,h)} min(|S\\T|, |T\\S|) ; theta = max(m, k+1) ;
absorption si mu = 0 ; site partage si 1 <= mu < theta ; deux amas entiers si mu >= theta.
Memes nuages que frequences.py (graine 11, generateurs recopies). Compte des nuages avec au moins un evenement."""
import random
import sys
import time
sys.dont_write_bytecode = True
sys.path.insert(0, '/workspaces/E-HGP/build/v11-points-math/verif_fermeture')
import vf_oracle as vo
from frequences_vf import cloud_gate, cloud_generic, cloud_two_blobs


def subtree(F, v):
    out, st = set(), [v]
    while st:
        x = st.pop()
        out.add(x)
        st.extend(F.children[x])
    return out


def events(F, m, k, n):
    par = list(range(n))

    def f(x):
        while par[x] != x:
            par[x] = par[par[x]]
            x = par[x]
        return x
    merges = [(v, F.levels_of_node[v], F.children[v]) for v in range(len(F.children)) if F.children[v]]
    desc = {v: subtree(F, v) for v in range(len(F.children))}
    state = {}
    for v, h, kids in merges:
        for x in range(len(kids)):
            for y in range(x + 1, len(kids)):
                state[(v, kids[x], kids[y])] = dict(h=h, U=None, mu=0)
    for a, snap in F.snaps:
        for _v, cov in snap:
            if vo.popcount(cov) >= m:
                s = vo.bits(cov)
                for i in s[1:]:
                    ri, r0 = f(i), f(s[0])
                    if ri != r0:
                        par[ri] = r0
        for (v, s, t), st in state.items():
            if not a < st['h']:
                continue
            S = T = 0
            bs, bt = set(), set()
            for x, cov in snap:
                if x in desc[s]:
                    S |= cov
                    if vo.popcount(cov) >= m:
                        bs.add(f(vo.bits(cov)[0]))
                elif x in desc[t]:
                    T |= cov
                    if vo.popcount(cov) >= m:
                        bt.add(f(vo.bits(cov)[0]))
            if not S or not T:
                continue
            mm = min(vo.popcount(S & ~T), vo.popcount(T & ~S))
            if st['U'] is None and bs & bt:
                st['U'] = a
                st['mu'] = mm
            elif st['U'] is not None:
                st['mu'] = max(st['mu'], mm)
    theta = max(m, k + 1)
    out = []
    for st in state.values():
        if st['U'] is None:
            continue
        out.append('absorption' if st['mu'] == 0 else ('deux' if st['mu'] >= theta else 'site'))
    return out


rng = random.Random(11)
tab = {}
t0 = time.time()
for c in range(900):
    gen = ('gate', 'generic', 'two_blobs')[c % 3]
    pts = cloud_gate(rng) if gen == 'gate' else (cloud_generic(rng) if gen == 'generic' else cloud_two_blobs(rng)[0])
    n = len(pts)
    cl = vo.Cloud(pts)
    for k in (2, 3):
        if k >= n:
            continue
        F = vo.Full(cl, k)
        for m in sorted(set([1, k + 1, k + 2])):
            if m > n:
                continue
            ev = events(F, m, k, n)
            r = tab.setdefault((gen, k, m), [0, 0, 0, 0, 0])
            r[0] += 1
            r[1] += bool(ev)
            r[2] += 'absorption' in ev
            r[3] += 'site' in ev
            r[4] += 'deux' in ev
print('%.1f s' % (time.time() - t0))
print('generateur k m : nuages, avec anticipation, absorption, site partage, deux amas entiers')
for key in sorted(tab):
    print('%-10s k=%d m=%d : %s' % (key[0], key[1], key[2], tab[key]))
