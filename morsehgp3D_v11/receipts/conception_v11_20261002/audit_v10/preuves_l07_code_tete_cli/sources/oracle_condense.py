"""Audit L07 (2 octobre 2026) : oracle de condensation par COUPES STRICTES, hors produit.

Entree : l'export `--tree` de mhgp10_cluster (niveaux, noeuds (rang, parent), points (id, noeud, rang, poids)).
Objet juge : l'ultrametrique des points d(x, y) = max(entree(x), entree(y), rang du plus petit ancetre commun des
noeuds d'attache), en RANGS entiers (aucun flottant dans la structure).

Semantique (Campello, Moulavi, Sander 2013, plateaux atomiques) : un cluster C ne a la coupe r se scinde au plus
grand rang r' ou deux de ses points se separent ; ses classes sont celles de la coupe stricte d < r'. Deux classes
ou plus de masse >= mcs : scission (chacune ouvre un cluster) ; une seule : elle continue C, les autres sortent a
lambda(r') ; aucune : tous les points restants sortent a lambda(r'). Stabilite = somme des w (lambda_sortie -
lambda_naissance), lambda = niveau^(-z/2), racine nee a lambda = 0. EOM : parent retenu si sa stabilite >= somme des
meilleures de ses enfants. Arithmetique : Fraction exacte sur les doubles publies pour z pair ; Decimal a 80
chiffres sinon, avec refus des quasi-egalites.

Ce n'est pas une reimplementation d'HDBSCAN comme adversaire : c'est le juge de la tete, lui-meme confronte a
sklearn.cluster.HDBSCAN (metric='precomputed') par check_cases.py.
"""
import sys
from decimal import Decimal, getcontext
from fractions import Fraction

getcontext().prec = 80


class Ambiguous(Exception):
    pass


def read_tree(path):
    lines = open(path).read().split('\n')
    L = int(lines[0].split()[1])
    levels = [float(x) for x in lines[1:1 + L]]
    N = int(lines[1 + L].split()[1])
    nodes = [tuple(int(t) for t in ln.split()) for ln in lines[2 + L:2 + L + N]]
    off = 2 + L + N
    P = int(lines[off].split()[1])
    pts = [tuple(int(t) for t in ln.split()) for ln in lines[off + 1:off + 1 + P]]
    pts.sort()
    if [p[0] for p in pts] != list(range(P)):
        raise ValueError('identifiants de points non denses')
    return levels, nodes, pts


def lam(level, z):
    """lambda exact (Fraction) pour z pair, Decimal sinon ; niveau nul : +infini (None)."""
    if level <= 0:
        return None
    if z % 2 == 0:
        return Fraction(1) / (Fraction(level) ** (z // 2))
    return Decimal(1) / (Decimal(level).sqrt() ** z)


def rank_matrix(nodes, pts):
    """d(x, y) en rangs, par remontee des parents."""
    n = len(pts)
    depth = [0] * len(nodes)
    for v in range(len(nodes) - 1, -1, -1):  # parents d'indice superieur : profondeur depuis la racine
        par = nodes[v][1]
        depth[v] = 0 if par < 0 else depth[par] + 1

    def lca(a, b):
        while a != b:
            if depth[a] >= depth[b]:
                a = nodes[a][1]
            else:
                b = nodes[b][1]
        return a
    D = [[0] * n for _ in range(n)]
    for i in range(n):
        for j in range(i + 1, n):
            D[i][j] = D[j][i] = max(pts[i][2], pts[j][2], nodes[lca(pts[i][1], pts[j][1])][0])
    return D


def condense(D, weights, mcs):
    """Arbre condense par coupes strictes. Rend (clusters, sortie) : clusters = liste de dict(parent, birth_rank,
    members, exits=[(rang, masse)]) ; sortie[x] = (cluster, rang de sortie)."""
    n = len(weights)
    clusters = [dict(parent=None, birth=None, members=list(range(n)), exits=[])]
    out = [None] * n
    work = [(0, list(range(n)))]
    while work:
        c, C = work.pop()
        while True:
            if len(C) == 1:  # point lourd isole (poids >= mcs) : hors du domaine de la tour (poids 1)
                raise ValueError('point isole de poids >= mcs : hors domaine')
            r = max(D[a][b] for i, a in enumerate(C) for b in C[i + 1:])
            # classes de la coupe stricte d < r
            cls = {a: a for a in C}

            def find(a):
                while cls[a] != a:
                    cls[a] = cls[cls[a]]
                    a = cls[a]
                return a
            for i, a in enumerate(C):
                for b in C[i + 1:]:
                    if D[a][b] < r:
                        cls[find(a)] = find(b)
            groups = {}
            for a in C:
                groups.setdefault(find(a), []).append(a)
            groups = sorted(groups.values())
            big = [g for g in groups if sum(weights[a] for a in g) >= mcs]
            if len(big) >= 2:
                clusters[c]['exits'].append((r, sum(weights[a] for a in C)))
                for g in groups:
                    if g in big:
                        clusters.append(dict(parent=c, birth=r, members=g, exits=[]))
                        work.append((len(clusters) - 1, g))
                    else:
                        for a in g:
                            out[a] = (c, r)
                break
            if len(big) == 1:
                gone = [a for g in groups if g is not big[0] for a in g]
                clusters[c]['exits'].append((r, sum(weights[a] for a in gone)))
                for a in gone:
                    out[a] = (c, r)
                C = big[0]
                continue
            clusters[c]['exits'].append((r, sum(weights[a] for a in C)))
            for a in C:
                out[a] = (c, r)
            break
    return clusters, out


def stabilities(clusters, levels, z):
    S = []
    for cl in clusters:
        birth = 0 if cl['birth'] is None else lam(levels[cl['birth']], z)
        s = 0
        for r, m in cl['exits']:
            l = lam(levels[r], z)
            if l is None or birth is None:
                raise ValueError('niveau nul : lambda infini, hors domaine de cet oracle')
            s += m * (l - birth)
        S.append(s)
    return S


def select(clusters, S, eom=True, allow_single=False):
    m = len(clusters)
    kids = [[] for _ in range(m)]
    for c in range(1, m):
        kids[clusters[c]['parent']].append(c)
    chosen = [False] * m
    if eom:
        best = [0] * m
        for c in range(m - 1, -1, -1):
            sub = sum(best[k] for k in kids[c])
            if not kids[c]:
                best[c], chosen[c] = S[c], True
            elif c == 0 and not allow_single:
                best[c] = sub
            else:
                gap = sub - S[c]
                scale = max(abs(sub), abs(S[c]), 1)
                if isinstance(gap, Decimal) and gap != 0 and abs(gap) < scale * Decimal(10) ** -40:
                    raise Ambiguous('quasi-egalite EOM au cluster %d' % c)
                if gap > 0:
                    best[c] = sub
                else:
                    best[c], chosen[c] = S[c], True
        for c in range(m):
            if chosen[c]:
                a = clusters[c]['parent']
                while a is not None:
                    if chosen[a]:
                        chosen[c] = False
                        break
                    a = clusters[a]['parent']
    else:
        chosen = [not kids[c] for c in range(m)]
    if not allow_single:
        chosen[0] = False
    return chosen


def labels(clusters, out, chosen):
    ids, k = {}, 0
    for c in range(len(clusters)):
        if chosen[c]:
            ids[c] = k
            k += 1
    lab = []
    for c, _ in out:
        while c is not None and not chosen[c]:
            c = clusters[c]['parent']
        lab.append(-1 if c is None else ids[c])
    return lab


def run(tree_path, mcs, z, eom=True, allow_single=False):
    levels, nodes, pts = read_tree(tree_path)
    D = rank_matrix(nodes, pts)
    clusters, out = condense(D, [p[3] for p in pts], mcs)
    S = stabilities(clusters, levels, z)
    chosen = select(clusters, S, eom, allow_single)
    return dict(levels=levels, nodes=nodes, pts=pts, D=D, clusters=clusters, out=out, S=S, chosen=chosen,
                labels=labels(clusters, out, chosen))


if __name__ == '__main__':
    r = run(sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), True, len(sys.argv) > 4 and sys.argv[4] == '1')
    print('labels', r['labels'])
    for c, cl in enumerate(r['clusters']):
        print('cluster', c, 'parent', cl['parent'], 'membres', cl['members'], 'stabilite', r['S'][c], float(r['S'][c]),
              'retenu', r['chosen'][c])
