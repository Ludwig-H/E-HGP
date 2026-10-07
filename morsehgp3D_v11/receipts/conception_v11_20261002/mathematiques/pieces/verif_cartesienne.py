#!/usr/bin/env python3
"""Piece de MATHEMATIQUES.md (v11), enonces TOUR-J et TOUR-J2 : regle cartesienne et contraction contre plateaux
atomiques.

Hypergraphe a rangs aleatoires tres repetes. Reference : pour chaque rang t, composantes de l'hypergraphe des
hyperaretes de rang <= t ; un noeud par composante qui reunit au moins deux composantes du rang t - 1, ses enfants
etant ces composantes (TOUR-C). Regle cartesienne : hyperaretes traitees par rang croissant, dans un ordre tire au
hasard a rang egal, listes concatenees dans un ordre tire au hasard, jonction J = rang de la concatenation ; les
noeuds sont les intervalles maximaux de jonctions <= t dont le maximum vaut t, leurs enfants les morceaux separes par
les jonctions egales a t.

Usage : python3 -B verif_cartesienne.py [graine] [essais]
"""
import random
import sys


def by_plateaus(nleaves, edges):
    parent = list(range(nleaves))

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    nodes = set()
    for t in sorted(set(r for r, _ in edges)):
        before = {}
        for x in range(nleaves):
            before.setdefault(find(x), []).append(x)
        pre = dict((x, find(x)) for x in range(nleaves))
        for r, leaves in edges:
            if r == t:
                for y in leaves[1:]:
                    a, b = find(leaves[0]), find(y)
                    if a != b:
                        parent[a] = b
        groups = {}
        for x in range(nleaves):
            groups.setdefault(find(x), set()).add(pre[x])
        for olds in groups.values():
            if len(olds) >= 2:
                nodes.add((t, frozenset(frozenset(before[o]) for o in olds)))
    return nodes


def by_cartesian(nleaves, edges, rnd):
    order = sorted(edges, key=lambda e: (e[0], rnd.random()))
    lists = dict((x, [x]) for x in range(nleaves))      # racine -> liste
    junc = dict((x, []) for x in range(nleaves))        # racine -> jonctions internes de sa liste
    root = list(range(nleaves))
    for r, leaves in order:
        roots = []
        for x in leaves:
            if root[x] not in roots:
                roots.append(root[x])
        if len(roots) < 2:
            continue
        rnd.shuffle(roots)
        head = roots[0]
        for other in roots[1:]:
            junc[head] = junc[head] + [r] + junc[other]
            lists[head] = lists[head] + lists[other]
            for x in lists[other]:
                root[x] = head
            del lists[other], junc[other]
    # ordre final : concatenation des listes restantes, jonction infinie entre elles
    pi, jj = [], []
    for head in lists:
        if pi:
            jj.append(None)
        pi += lists[head]
        jj += junc[head]
    nodes = set()
    for t in sorted(set(r for r, _ in edges)):
        i = 0
        while i < len(pi):
            j = i
            while j < len(pi) - 1 and jj[j] is not None and jj[j] <= t:
                j += 1
            inner = jj[i:j]
            if inner and max(inner) == t:
                pieces, cur = [], [pi[i]]
                for pos in range(i, j):
                    if jj[pos] == t:
                        pieces.append(frozenset(cur))
                        cur = []
                    cur.append(pi[pos + 1])
                pieces.append(frozenset(cur))
                nodes.add((t, frozenset(pieces)))
            i = j + 1
    return nodes


def by_contraction(nleaves, edges, rnd):
    """TOUR-J2 : union-find ordinaire, un noeud binaire par reunion, puis contraction des noeuds de meme rang relies
    par un lien parent-enfant."""
    order = sorted(edges, key=lambda e: (e[0], rnd.random()))
    parent = list(range(nleaves))

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    top = dict((x, ('leaf', x)) for x in range(nleaves))   # racine -> noeud courant
    rank, kids = {}, {}
    count = 0
    for r, leaves in order:
        for y in leaves[1:]:
            a, b = find(leaves[0]), find(y)
            if a == b:
                continue
            node = ('bin', count)
            count += 1
            rank[node] = r
            kids[node] = [top[a], top[b]]
            parent[a] = b
            top[b] = node
    nodes = set()

    def leafset(node):
        return [node[1]] if node[0] == 'leaf' else [x for c in kids[node] for x in leafset(c)]

    def gather(node, r):
        # enfants exterieurs du bloc de rang r contenant node
        out = []
        for c in kids[node]:
            if c[0] == 'bin' and rank[c] == r:
                out += gather(c, r)
            else:
                out.append(frozenset(leafset(c)))
        return out
    child_of_same = set()
    for node in kids:
        for c in kids[node]:
            if c[0] == 'bin' and rank[c] == rank[node]:
                child_of_same.add(c)
    for node in kids:
        if node not in child_of_same:
            nodes.add((rank[node], frozenset(gather(node, rank[node]))))
    return nodes


def main():
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 20261002
    trials = int(sys.argv[2]) if len(sys.argv) > 2 else 3000
    rnd = random.Random(seed)
    bad = bad2 = total = wide = 0
    for _ in range(trials):
        n = rnd.randint(2, 14)
        ranks = rnd.randint(1, 5)
        edges = []
        for _e in range(rnd.randint(1, 16)):
            size = rnd.randint(2, min(4, n))
            edges.append((rnd.randint(1, ranks), rnd.sample(range(n), size)))
        want = by_plateaus(n, edges)
        got = by_cartesian(n, edges, rnd)
        got2 = by_contraction(n, edges, rnd)
        total += len(want)
        wide += sum(1 for _t, kids in want if len(kids) >= 3)
        bad += want != got
        bad2 += want != got2
    print('graine %d ; essais %d ; noeuds %d dont %d a trois enfants ou plus ; ecarts regle cartesienne %d ; ecarts '
          'contraction %d' % (seed, trials, total, wide, bad, bad2))
    return 1 if bad or bad2 else 0


if __name__ == '__main__':
    sys.exit(main())
