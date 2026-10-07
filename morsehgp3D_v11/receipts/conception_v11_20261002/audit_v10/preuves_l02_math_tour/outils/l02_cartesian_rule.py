#!/usr/bin/env python3
"""Controle independant de la « regle cartesienne » de TOWER_v2 (PO-T18), non implementee en v10 mais candidate pour la v11.

Enonce teste : on traite les aretes (hyperaretes en etoile) par rang croissant, dans un ordre QUELCONQUE a rang egal,
par union-find + concatenation de listes ; pi = ordre final des feuilles, J[i] = rang de l'arete qui a colle les
positions i et i+1. Alors les noeuds N-aires de la foret a plateaux (composantes de G_t qui reunissent >= 2 composantes
de G_{t-1}) sont exactement les intervalles maximaux de jonctions <= t dont le maximum vaut t ; leurs enfants sont les
morceaux separes par les jonctions egales a t.
Reference : balayage par lots atomiques (racines pre-lot figees), ecrit separement.
Donnees : graphes aleatoires a poids entiers tres repetes (plateaux massifs), hyperaretes de 2 a 5 sommets.
"""
import random
import sys


def reference(n, hyper):
    """Noeuds N-aires par lots atomiques : ensemble de (rang, frozenset(feuilles), frozenset(frozenset enfants))."""
    par = list(range(n))

    def find(x):
        while par[x] != x:
            par[x] = par[par[x]]
            x = par[x]
        return x
    leaves = {i: frozenset([i]) for i in range(n)}
    nodes = set()
    by_rank = {}
    for r, vs in hyper:
        by_rank.setdefault(r, []).append(vs)
    for r in sorted(by_rank):
        pre = {}
        for vs in by_rank[r]:
            for v in vs:
                pre[find(v)] = leaves[find(v)]
        for vs in by_rank[r]:
            r0 = find(vs[0])
            for v in vs[1:]:
                rv = find(v)
                if rv != r0:
                    par[rv] = r0
        groups = {}
        for old, ls in pre.items():
            groups.setdefault(find(old), []).append(ls)
        for root, kids in groups.items():
            if len(kids) >= 2:
                allv = frozenset().union(*kids)
                nodes.add((r, allv, frozenset(kids)))
                leaves[root] = allv
            else:
                leaves[root] = kids[0]
    return nodes


def cartesian(n, hyper, rnd):
    """Noyau sans lots + regle cartesienne."""
    edges = []
    for r, vs in hyper:
        for v in vs[1:]:
            edges.append((r, vs[0], v))
    # ordre quelconque a rang egal
    rnd.shuffle(edges)
    edges.sort(key=lambda e: e[0])
    par = list(range(n))
    head = list(range(n))
    tail = list(range(n))
    nxt = [None] * n
    jr = [None] * n

    def find(x):
        while par[x] != x:
            par[x] = par[par[x]]
            x = par[x]
        return x
    for r, a, b in edges:
        ra, rb = find(a), find(b)
        if ra == rb:
            continue
        if rnd.random() < 0.5:
            ra, rb = rb, ra
        nxt[tail[ra]] = head[rb]
        jr[tail[ra]] = r
        par[rb] = ra
        tail[ra] = tail[rb]
    nodes = set()
    for root in sorted({find(i) for i in range(n)}):
        pi, J = [], []
        x = head[root]
        while x is not None:
            pi.append(x)
            if nxt[x] is not None:
                J.append(jr[x])
            x = nxt[x]
        L = len(pi)
        for i in range(L - 1):
            t = J[i]
            lo = i
            while lo > 0 and J[lo - 1] <= t:
                lo -= 1
            # ouvre un noeud si c'est la premiere jonction egale a t de son intervalle maximal
            if any(J[j] == t for j in range(lo, i)):
                continue
            hi = i
            while hi + 1 < L - 1 and J[hi + 1] <= t:
                hi += 1
            # intervalle de positions [lo, hi + 1] ; enfants separes par les jonctions egales a t
            kids, cur = [], [pi[lo]]
            for j in range(lo, hi + 1):
                if J[j] == t:
                    kids.append(frozenset(cur))
                    cur = []
                cur.append(pi[j + 1])
            kids.append(frozenset(cur))
            nodes.add((t, frozenset(pi[lo:hi + 2]), frozenset(kids)))
    return nodes


def main():
    rnd = random.Random(20261002)
    trials = int(sys.argv[1]) if len(sys.argv) > 1 else 400
    bad = nn = big = 0
    for _ in range(trials):
        n = rnd.randint(2, 40)
        m = rnd.randint(1, 3 * n)
        ranks = rnd.randint(1, 6)
        hyper = []
        for _ in range(m):
            k = rnd.randint(2, min(5, n))
            hyper.append((rnd.randint(1, ranks), rnd.sample(range(n), k)))
        a = reference(n, hyper)
        b = cartesian(n, hyper, rnd)
        nn += len(a)
        big += sum(1 for (_, _, kids) in a if len(kids) >= 3)
        if a != b:
            bad += 1
    print('regle_cartesienne essais %d ecarts %d noeuds %d dont >= 3 enfants %d' % (trials, bad, nn, big))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
