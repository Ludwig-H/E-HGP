#!/usr/bin/env python3
"""Verification locale, petits nuages, des deux invariants geometriques proposes par l'audit E-HGP/Zoltan.

Audit des transpositions vers la v11, 4 octobre 2026. Lecture seule du depot ; aucun binaire, aucun GCP.
Oracle : etage A de la reference v11 (`reference/hgp11_ref/definition.py`, graphe Gamma_k exhaustif, Fraction),
lu dans le worktree `build/v11-claude-20261003` (origin/main).

Invariant T (certificat de tranche d'E-HGP, `E-HGP/src/ehgp/engine/separation.py` l. 9-33, generalise a toute
surface fermee ou plan) : si au plus k-1 sites sont a distance carree <= a d'une surface S qui separe l'espace,
S ne rencontre pas L_k(a). Donc, avec A_k(S) = k-ieme plus petite distance carree site -> S :
  T1  toute naissance de niveau < A_k(S) a son centre hors de S ;
  T2  tout noeud de niveau < A_k(S) a toutes les naissances de son sous-arbre du meme cote de S ;
  T3  (verticales) tout noeud v d'ordre k >= 2 de niveau < A_{k-1}(S) a son image lower(v) dont les naissances
      du sous-arbre sont du meme cote que celles de v.
Invariant S (fait de segment d'E-HGP, `E-HGP/src/ehgp/engine/segment.py`) : pour deux naissances b1, b2 d'un
meme ordre, de centres c1, c2, le niveau du plus petit ancetre commun est <= max_t a_k(c1 + t (c2 - c1)).

Mutants (pouvoir de detection, pas une qualification) :
  MA  fusion precoce : deux noeuds vivants a une meme coupe fermee, non encore reunis, declares reunis a ce niveau ;
  MB  fusion tardive : le niveau de rencontre de deux naissances declare au niveau du parent du LCA (ou x2 a la racine) ;
  MC  verticale fausse : lower(v) remplace par un autre noeud vivant a la meme coupe fermee d'ordre k-1.
Usage : python3 -B ehgp_zoltan_verif.py [nuages] [graine]   (defaut 60 nuages, graine 4102026)
"""
from fractions import Fraction as F
import random
import sys

sys.path.insert(0, '/workspaces/E-HGP/build/v11-claude-20261003/morsehgp3D_v11/reference')
from hgp11_ref import Definition  # noqa: E402

N_SITES, KMAX, SPAN = 8, 3, 9


def tree(res):
    nodes = res.nodes
    parent = [None] * len(nodes)
    for i, nd in enumerate(nodes):
        for c in nd.children:
            parent[c] = i
    births = [None] * len(nodes)
    for i, nd in enumerate(nodes):  # enfants de niveau strictement inferieur, donc d'indice inferieur
        if not nd.children:
            births[i] = (i,)
        else:
            out = []
            for c in nd.children:
                if births[c] is None:
                    raise SystemExit('numerotation inattendue')
                out.extend(births[c])
            births[i] = tuple(sorted(out))
    return parent, births


def alive(res, parent, level):
    """Noeuds vivants a la coupe fermee `level`."""
    return [i for i, nd in enumerate(res.nodes)
            if nd.level <= level and (parent[i] is None or level < res.nodes[parent[i]].level)]


def lca_level(res, parent, a, b):
    seen = set()
    x = a
    while x is not None:
        seen.add(x)
        x = parent[x]
    y = b
    while y not in seen:
        y = parent[y]
    return res.nodes[y].level, y


# ---------------------------------------------------------------- surfaces exactes

def plane(rng, pts):
    while True:
        n = tuple(rng.randint(-2, 2) for _ in range(3))
        if any(n):
            break
    i, j = rng.randrange(len(pts)), rng.randrange(len(pts))
    off = F(sum(a * b for a, b in zip(n, pts[i])) + sum(a * b for a, b in zip(n, pts[j])), 2)
    nn = sum(a * a for a in n)

    def side(x):
        g = sum(a * b for a, b in zip(n, x)) - off
        return (g > 0) - (g < 0)

    def dist2(x):
        g = sum(a * b for a, b in zip(n, x)) - off
        return g * g / nn
    return side, dist2


def box(rng):
    lo, hi = [], []
    for _ in range(3):
        a, b = sorted(rng.sample(range(-1, 2 * SPAN + 2), 2))
        lo.append(F(a, 2))
        hi.append(F(b, 2))

    def side(x):
        if all(lo[i] < x[i] < hi[i] for i in range(3)):
            return -1
        if all(lo[i] <= x[i] <= hi[i] for i in range(3)):
            return 0
        return 1

    def dist2(x):
        if all(lo[i] < x[i] < hi[i] for i in range(3)):
            return min(min((x[i] - lo[i]) ** 2, (hi[i] - x[i]) ** 2) for i in range(3))
        return sum(max(lo[i] - x[i], 0, x[i] - hi[i]) ** 2 for i in range(3))
    return side, dist2


def threshold(dist2, pts, k):
    return sorted(dist2(p) for p in pts)[k - 1]


# ---------------------------------------------------------------- fait de segment, exact

def segmax(pts, p, q, k):
    d = tuple(b - a for a, b in zip(p, q))
    lead = sum(c * c for c in d)
    parts = []
    for x in pts:
        o = tuple(a - b for a, b in zip(p, x))
        parts.append((2 * sum(u * v for u, v in zip(d, o)), sum(c * c for c in o)))
    times = {F(0), F(1)}
    for i in range(len(parts)):
        for j in range(i + 1, len(parts)):
            g = parts[i][0] - parts[j][0]
            if g:
                t = F(parts[j][1] - parts[i][1], g)
                if 0 < t < 1:
                    times.add(t)
    best = None
    for t in times:
        v = lead * t * t + sorted(s * t + c for s, c in parts)[k - 1]
        best = v if best is None or v > best else best
    return best


def kruskal_plateaus(pts):
    """Ordre 1 par la seule geometrie entiere (J2) : unions simultanees a chaque longueur carree, niveau d2 / 4.
    Rend l'ensemble des fusions (niveau, frozenset des enfants vus comme ensembles de sites)."""
    n = len(pts)
    edges = sorted((sum((a - b) ** 2 for a, b in zip(pts[i], pts[j])), i, j) for i in range(n) for j in range(i + 1, n))
    comp = {i: frozenset([i]) for i in range(n)}
    merges = set()
    e = 0
    while e < len(edges):
        d2 = edges[e][0]
        group = []
        while e < len(edges) and edges[e][0] == d2:
            group.append(edges[e])
            e += 1
        olds = {}
        adj = {}
        for _, i, j in group:  # graphe des anciennes composantes relie au plateau
            ci, cj = comp[i], comp[j]
            if ci != cj:
                adj.setdefault(ci, set()).add(cj)
                adj.setdefault(cj, set()).add(ci)
        seen = set()
        for start in adj:
            if start in seen:
                continue
            stack, block = [start], []
            seen.add(start)
            while stack:
                c = stack.pop()
                block.append(c)
                for o in adj[c]:
                    if o not in seen:
                        seen.add(o)
                        stack.append(o)
            union = frozenset().union(*block)
            merges.add((F(d2, 4), frozenset(block)))
            for x in union:
                olds[x] = union
        comp.update(olds)
    return merges


def forest_merges(res):
    parent, births = tree(res)
    sites = {}
    for i, nd in enumerate(res.nodes):  # a l'ordre 1 une naissance est un site : centre = position
        if not nd.children:
            sites[i] = nd.center
    return births, sites


def main():
    if len(sys.argv) > 1 and sys.argv[1] == 'j2':
        rng = random.Random(int(sys.argv[2]) if len(sys.argv) > 2 else 2)
        clouds, plateaus, nary, bad = 0, 0, 0, 0
        for _ in range(int(sys.argv[3]) if len(sys.argv) > 3 else 200):
            pts = set()
            while len(pts) < 11:
                pts.add(tuple(rng.randint(0, 6) for _ in range(3)))
            pts = sorted(pts)
            res = Definition(pts).order(1)
            births, sites = forest_merges(res)
            index = {tuple(int(c) for c in sites[b]): b for b in sites}
            site_of_birth = {b: pts.index(tuple(int(c) for c in sites[b])) for b in sites}
            got = set()
            for v, nd in enumerate(res.nodes):
                if nd.children:
                    got.add((nd.level, frozenset(frozenset(site_of_birth[b] for b in births[c]) for c in nd.children)))
                    nary += len(nd.children) > 2
            want = kruskal_plateaus(pts)
            clouds += 1
            plateaus += len(want)
            bad += got != want
            del index
        print('j2 nuages %d fusions %d dont n-aires %d desaccords %d' % (clouds, plateaus, nary, bad))
        return 0 if bad == 0 and nary > 0 else 1
    clouds = int(sys.argv[1]) if len(sys.argv) > 1 else 60
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 4102026
    rng = random.Random(seed)
    stat = dict(clouds=0, orders=0, nodes=0, surfaces=0,
                t1_checks=0, t2_checks=0, t3_checks=0, violations=0,
                s_pairs=0, s_equal=0, s_violations=0,
                ma=0, ma_caught=0, mb=0, mb_caught=0, mc=0, mc_caught=0)
    tight_lo, tight_hi = [], []
    while stat['clouds'] < clouds:
        pts = set()
        while len(pts) < N_SITES:  # petites coordonnees entieres : ex aequo et cospheriques frequents
            pts.add(tuple(rng.randint(0, SPAN) for _ in range(3)))
        pts = sorted(pts)
        ref = Definition(pts)
        orders = [ref.order(k) for k in range(1, KMAX + 1)]
        trees = [tree(r) for r in orders]
        surfaces = [plane(rng, pts) for _ in range(120)] + [box(rng) for _ in range(120)]
        stat['clouds'] += 1
        stat['surfaces'] += len(surfaces)
        for k in range(1, KMAX + 1):
            res = orders[k - 1]
            parent, births = trees[k - 1]
            stat['orders'] += 1
            stat['nodes'] += len(res.nodes)
            centers = {i: res.nodes[i].center for i in range(len(res.nodes)) if res.nodes[i].center is not None}
            thr = [threshold(d2, pts, k) for _, d2 in surfaces]
            thr_low = [threshold(d2, pts, k - 1) for _, d2 in surfaces] if k > 1 else None
            sides = [{b: sd(centers[b]) for b in centers} for sd, _ in surfaces]
            low_sides = None
            if k > 1:
                lres = orders[k - 2]
                lcent = {i: lres.nodes[i].center for i in range(len(lres.nodes)) if lres.nodes[i].center is not None}
                low_sides = [{b: sd(lcent[b]) for b in lcent} for sd, _ in surfaces]
                lparent, lbirths = trees[k - 2]

            def one_side(si, bs, side_map):
                vals = {side_map[si][b] for b in bs}
                return len(vals) == 1 and 0 not in vals

            for si in range(len(surfaces)):
                for v, nd in enumerate(res.nodes):
                    if nd.level < thr[si]:
                        if not nd.children:
                            stat['t1_checks'] += 1
                            stat['violations'] += sides[si][v] == 0
                        else:
                            stat['t2_checks'] += 1
                            stat['violations'] += not one_side(si, births[v], sides)
                    if k > 1 and nd.level < thr_low[si]:
                        stat['t3_checks'] += 1
                        u = res.lower[v]
                        vals = {sides[si][b] for b in births[v]} | {low_sides[si][b] for b in lbirths[u]}
                        stat['violations'] += not (len(vals) == 1 and 0 not in vals)
            # invariant S, toutes les paires de naissances
            blist = sorted(centers)
            for a in range(len(blist)):
                for b in range(a + 1, len(blist)):
                    b1, b2 = blist[a], blist[b]
                    level, top = lca_level(res, parent, b1, b2)
                    m = segmax(pts, centers[b1], centers[b2], k)
                    stat['s_pairs'] += 1
                    stat['s_equal'] += level == m
                    stat['s_violations'] += level > m
                    if level > 0:
                        tight_hi.append(float(m / level))
                    # MB : fusion tardive, au niveau du parent du LCA (ou 2x a la racine)
                    late = res.nodes[parent[top]].level if parent[top] is not None else 2 * level
                    stat['mb'] += 1
                    stat['mb_caught'] += late > m
                    # meilleur minorant de tranche parmi les surfaces : separe b1 de b2
                    best = F(0)
                    for si in range(len(surfaces)):
                        s1, s2 = sides[si][b1], sides[si][b2]
                        if s1 and s2 and s1 != s2 and thr[si] > best:
                            best = thr[si]
                    if level > 0:
                        tight_lo.append(float(best / level))
            # MA : deux noeuds vivants a une meme coupe, reunis trop tot au niveau de cette coupe
            levels = sorted({nd.level for nd in res.nodes})
            for lv in levels:
                live = alive(res, parent, lv)
                for x in range(len(live)):
                    for y in range(x + 1, len(live)):
                        u, w = live[x], live[y]
                        stat['ma'] += 1
                        bs = births[u] + births[w]
                        stat['ma_caught'] += any(lv < thr[si] and not one_side(si, bs, sides)
                                                 for si in range(len(surfaces)))
            # MC : verticale remplacee par un autre noeud vivant a la meme coupe fermee de l'ordre k-1
            if k > 1:
                lres = orders[k - 2]
                for v, nd in enumerate(res.nodes):
                    u = res.lower[v]
                    for alt in alive(lres, lparent, nd.level):
                        if alt == u:
                            continue
                        stat['mc'] += 1
                        stat['mc_caught'] += any(nd.level < thr_low[si] and
                                                 len({sides[si][b] for b in births[v]} |
                                                     {low_sides[si][b] for b in lbirths[alt]}) > 1
                                                 for si in range(len(surfaces)))

    def quant(xs, q):
        xs = sorted(xs)
        return round(xs[min(len(xs) - 1, int(q * len(xs)))], 3) if xs else None
    print('parametres', dict(n=N_SITES, kmax=KMAX, span=SPAN, seed=seed, surfaces_par_nuage=240))
    for key in sorted(stat):
        print('%-12s %d' % (key, stat[key]))
    print('minorant de tranche / niveau LCA (paires de naissances, niveau > 0) : mediane %s, q10 %s, part = 1 : %.3f'
          % (quant(tight_lo, .5), quant(tight_lo, .1), sum(1 for x in tight_lo if x >= 1 - 1e-12) / max(1, len(tight_lo))))
    print('majorant de segment / niveau LCA : mediane %s, q90 %s, part = 1 : %.3f'
          % (quant(tight_hi, .5), quant(tight_hi, .9), sum(1 for x in tight_hi if x <= 1 + 1e-12) / max(1, len(tight_hi))))
    code = 0 if stat['violations'] == 0 and stat['s_violations'] == 0 and stat['t2_checks'] > 0 else 1
    print('code', code)
    return code


if __name__ == '__main__':
    sys.exit(main())
