# Controle de conception (leger) : foret d'un ordre SANS LOTS.
#   noyau = union-find arete par arete (union par taille), evenements (rang, operandes = sommets courants),
#           historique d'attache (perdant -> survivant, rang), sommet apres chaque jonction (jtop) ;
#   materialisation = contraction des evenements de meme rang relies (plateaux) -> noeuds N-aires ;
#   requete component_at(feuille, rang) par l'historique ; image d'une jonction par jtop + une remontee conditionnelle.
# Reference : Kruskal par lots atomiques (semantique de la v10 : racines pre-lot lues avant toute union).
import random, sys

def reference_lots(nb, brank, joins):
    """joins : liste triee par rang de (rang, [naissances]). Rend noeuds = liste (rang, enfants tries) ; parent."""
    rank = list(brank)
    parent = [-1] * nb
    children = [[] for _ in range(nb)]
    dsu = list(range(nb))
    top = list(range(nb))
    def find(x):
        while dsu[x] != x:
            dsu[x] = dsu[dsu[x]]
            x = dsu[x]
        return x
    i = 0
    while i < len(joins):
        rk = joins[i][0]
        j = i
        while j < len(joins) and joins[j][0] == rk:
            j += 1
        pre = [[find(r) for r in reps] for _, reps in joins[i:j]]
        for roots in pre:
            for r in roots[1:]:
                x, y = find(roots[0]), find(r)
                if x != y:
                    dsu[max(x, y)] = min(x, y)
        groups = {}
        for roots in pre:
            for r in roots:
                groups.setdefault(find(r), set()).add(r)
        for root in sorted(groups):
            mem = groups[root]
            if len(mem) >= 2:
                node = len(rank)
                rank.append(rk)
                parent.append(-1)
                kids = sorted(top[m] for m in mem)
                children.append(kids)
                for c in kids:
                    parent[c] = node
                top[root] = node
        i = j
    return rank, parent, children

def kernel(nb, joins):
    par = list(range(nb)); size = [1] * nb; last = [-1] * nb; minleaf = list(range(nb))
    att_p = [-1] * nb; att_r = [0] * nb
    ev = []  # (rang, opA, opB, minleaf, survivant) ; operande = ('L', feuille) ou ('E', evenement)
    jtop = []
    depth_check = 0
    def find(x):
        while par[x] != x:
            par[x] = par[par[x]]
            x = par[x]
        return x
    def top(x):
        return ('E', last[x]) if last[x] >= 0 else ('L', x)
    for rk, reps in joins:
        x = find(reps[0])
        for r in reps[1:]:
            y = find(r)
            if x == y:
                continue
            a, b = top(x), top(y)
            s, l = (x, y) if size[x] >= size[y] else (y, x)
            ml = min(minleaf[x], minleaf[y])
            ev.append((rk, a, b, ml, s))
            par[l] = s; size[s] += size[l]; minleaf[s] = ml; last[s] = len(ev) - 1
            att_p[l] = s; att_r[l] = rk
            x = s
        jtop.append(top(find(reps[0])))
    return ev, att_p, att_r, jtop

def materialize(nb, brank, ev):
    ne = len(ev)
    loc = list(range(ne))
    def find(x):
        while loc[x] != x:
            loc[x] = loc[loc[x]]
            x = loc[x]
        return x
    for e, (rk, a, b, ml, s) in enumerate(ev):
        for op in (a, b):
            if op[0] == 'E' and ev[op[1]][0] == rk:
                loc[find(op[1])] = find(e)
    sets = {}
    for e in range(ne):
        sets.setdefault(find(e), []).append(e)
    keys = sorted((ev[root][0], min(ev[e][3] for e in mem), root) for root, mem in sets.items())
    nid_of_set = {root: nb + i for i, (_rk, _ml, root) in enumerate(keys)}
    nid = [nid_of_set[find(e)] for e in range(ne)]
    rank = list(brank) + [k[0] for k in keys]
    parent = [-1] * len(rank)
    children = [[] for _ in rank]
    for e, (rk, a, b, ml, s) in enumerate(ev):
        for op in (a, b):
            if op[0] == 'L':
                c = op[1]
            elif ev[op[1]][0] == rk:
                continue
            else:
                c = nid[op[1]]
            children[nid[e]].append(c)
            parent[c] = nid[e]
    for c in children:
        c.sort()
    return rank, parent, children, nid

def history(nb, ev, nid):
    by_root = [[] for _ in range(nb)]
    for e, (rk, a, b, ml, s) in enumerate(ev):
        by_root[s].append((rk, nid[e]))
    return by_root

def component_at(leaf, r, att_p, att_r, by_root):
    x = leaf; hops = 0
    while att_p[x] >= 0 and att_r[x] <= r:
        x = att_p[x]; hops += 1
    lst = by_root[x]
    lo, hi = 0, len(lst)
    while lo < hi:
        mid = (lo + hi) // 2
        if lst[mid][0] <= r:
            lo = mid + 1
        else:
            hi = mid
    return (lst[lo - 1][1] if lo else x), hops

def brute_at(leaf, r, rank, parent):
    v = leaf
    while parent[v] >= 0 and rank[parent[v]] <= r:
        v = parent[v]
    return v

def main():
    rng = random.Random(int(sys.argv[1]) if len(sys.argv) > 1 else 5)
    cases = int(sys.argv[2]) if len(sys.argv) > 2 else 3000
    fails = nodes = nary = plateaux = queries = images = maxhops = 0
    for it in range(cases):
        nb = rng.randint(2, 40)
        nranks = rng.choice((1, 2, 3, 5, 12, 40))
        brank = [0] * nb
        joins = []
        for _ in range(rng.randint(1, 3 * nb)):
            q = rng.choice((2, 2, 3, 3, 4, 5))
            reps = [rng.randrange(nb) for _ in range(q)]
            joins.append((rng.randint(1, nranks), reps))
        joins.sort(key=lambda t: t[0])  # tri stable : ordre des cles
        r_rank, r_parent, r_children = reference_lots(nb, brank, joins)
        ev, att_p, att_r, jtop = kernel(nb, joins)
        m_rank, m_parent, m_children, nid = materialize(nb, brank, ev)
        if (r_rank, r_parent, r_children) != (m_rank, m_parent, m_children):
            fails += 1
            if fails <= 3:
                print('ECART foret', nb, joins)
            continue
        nodes += len(r_rank)
        nary += sum(1 for c in r_children if len(c) >= 3)
        plateaux += sum(1 for a, b in zip(joins, joins[1:]) if a[0] == b[0])
        by_root = history(nb, ev, nid)
        for leaf in range(nb):
            for r in range(0, nranks + 2):
                got, hops = component_at(leaf, r, att_p, att_r, by_root)
                maxhops = max(maxhops, hops)
                queries += 1
                if got != brute_at(leaf, r, m_rank, m_parent):
                    fails += 1
                    if fails <= 3:
                        print('ECART requete', leaf, r)
        for (rk, reps), t in zip(joins, jtop):
            v = t[1] if t[0] == 'L' else nid[t[1]]
            if m_parent[v] >= 0 and m_rank[m_parent[v]] <= rk:
                v = m_parent[v]
            images += 1
            if v != brute_at(reps[0], rk, m_rank, m_parent):
                fails += 1
                if fails <= 3:
                    print('ECART image', rk, reps)
    print('foret_checks cases', cases, 'fails', fails, 'nodes', nodes, 'nary_merges', nary, 'equal_rank_neighbours',
          plateaux, 'queries', queries, 'junction_images', images, 'max_attach_hops', maxhops)
    return 1 if fails else 0

if __name__ == '__main__':
    sys.exit(main())
