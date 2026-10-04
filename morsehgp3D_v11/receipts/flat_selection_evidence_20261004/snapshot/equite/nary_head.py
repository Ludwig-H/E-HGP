#!/usr/bin/env python3
"""Tete N-aire pour toute hierarchie ultrametrique (tour v11 ou HDBSCAN) : condensation a mcs, stabilite,
excès de masse (EOM), feuilles, epsilon, etiquettes ; binarisation canonique vers le code de sklearn ;
antichaines oracles contre la verite.

Cadre : exploration_v11_hors_registre / cpu_reference / not_claimed. Petits nuages seulement (oracle de
correction). Aucun appel GCP, aucune construction native. Lancer avec PYTHONDONTWRITEBYTECODE=1 python3 -B.

Objet d'entree : un dendrogramme N-aire « atomise » (Dendrogram) : feuilles = points 0..n-1 au niveau 0,
noeuds internes = blocs a leur niveau de naissance, chaque enfant strictement plus bas que son parent (un
plateau d'egalites exactes forme UN noeud, jamais une chaine binaire). Une ultrametrique a diagonale
(u(i, i) = e_i, la date d'entree) donne le meme dendrogramme que sa partie hors diagonale des que mcs >= 2
(voir RAPPORT.md, lemme D) : la diagonale n'entre donc pas ici.

Definitions (RAPPORT.md, section 3) :
  - condensation N-aire : un noeud v du cluster c, de niveau l, a pour enfants gros B (taille >= mcs) et
    petits ; les points des petits sortent de c a l ; |B| >= 2 : c meurt a l, chaque gros devient un cluster
    ne a l ; |B| = 1 : le gros continue c ; |B| = 0 : c meurt a l (tous ses points restants sortent a l) ;
  - stabilite : S(c) = somme sur les sorties (x, l_x) de (phi(l_x) - phi(b_c)) + somme sur les enfants d de
    |d| (phi(mort_c) - phi(b_c)), phi decroissante, phi(b_racine) = 0 ;
  - EOM : de bas en haut, c retenu si S(c) >= somme des S~ de ses enfants (egalite : parent, comme sklearn) ;
    racine exclue sauf allow_single_cluster ;
  - feuilles : clusters sans enfant, racine exclue ; aucune scission -> tout est bruit (comme sklearn) ;
  - epsilon : un cluster retenu ne a b < eps est remplace par son plus bas ancetre strict non racine ne a
    b >= eps (a defaut l'enfant de la racine sur le chemin, ou la racine si allow_single_cluster). sklearn
    utilise '<' pour le cluster et '>' pour les ancetres : mode 'sklearn' reproduit cette asymetrie ;
  - etiquette d'un point : le plus bas cluster retenu sur la chaine de son cluster de sortie, sinon bruit (-1).
"""
from fractions import Fraction
from functools import cmp_to_key
import math

import numpy as np

INF = float('inf')


# --------------------------------------------------------------------------------------------------------
# Dendrogramme N-aire
# --------------------------------------------------------------------------------------------------------

class Dendrogram(object):
    """Feuilles 0..n-1 (niveau 0, rang -1) ; noeuds internes n..N-1 dans l'ordre de creation (enfants avant
    parents). value[v] : niveau flottant (lambda, lecture) ; rank[v] : rang exact (egalite exacte <=> meme rang)."""

    def __init__(self, n):
        self.n = n
        self.children = [[] for _ in range(n)]
        self.value = [0.0] * n
        self.rank = [-1] * n
        self.size = [1] * n
        self.parent = [-1] * n

    def add(self, children, value, rank):
        v = len(self.children)
        children = list(children)
        if len(children) < 2:
            raise ValueError('un noeud interne a au moins deux enfants')
        for c in children:
            if self.rank[c] >= rank:
                raise ValueError('plateau non atomise : enfant pas strictement plus bas')
            if self.parent[c] >= 0:
                raise ValueError('enfant deja rattache')
        self.children.append(children)
        self.value.append(float(value))
        self.rank.append(int(rank))
        self.size.append(sum(self.size[c] for c in children))
        self.parent.append(-1)
        for c in children:
            self.parent[c] = v
        return v

    @property
    def nodes(self):
        return len(self.children)

    @property
    def root(self):
        roots = [v for v in range(self.nodes) if self.parent[v] < 0]
        if len(roots) != 1:
            raise ValueError('racine non unique : %d racines' % len(roots))
        return roots[0]

    def leaves(self, v):
        out, stack = [], [v]
        while stack:
            x = stack.pop()
            if x < self.n:
                out.append(x)
            else:
                stack.extend(self.children[x])
        return sorted(out)

    def plateaus(self):
        """Noeuds a au moins trois enfants (multifusions)."""
        return [v for v in range(self.n, self.nodes) if len(self.children[v]) >= 3]

    def blocks_at(self, rank):
        """Blocs (ensembles de points) a la coupe fermee de rang exact `rank` (diagonale ignoree)."""
        out = []
        for v in range(self.nodes):
            if self.rank[v] <= rank and (self.parent[v] < 0 or self.rank[self.parent[v]] > rank):
                out.append(self.leaves(v))
        return sorted(out)


def from_linkage(tree, n):
    """Dendrogramme N-aire d'un arbre du lien simple au format sklearn/scipy (gauche, droite, valeur, taille) :
    toute chaine de noeuds binaires de meme valeur flottante est fusionnee en un noeud (plateau atomise)."""
    if hasattr(tree, 'dtype') and tree.dtype.names:
        left, right, value = tree['left_node'], tree['right_node'], tree['value']
    else:
        tree = np.asarray(tree)
        left, right, value = tree[:, 0], tree[:, 1], tree[:, 2]
    left = [int(x) for x in left]
    right = [int(x) for x in right]
    value = [float(x) for x in value]
    m = len(value)
    if m != n - 1:
        raise ValueError('arbre du lien simple : %d lignes pour %d points' % (m, n))
    parent = [-1] * (n + m)
    for i in range(m):
        parent[left[i]] = n + i
        parent[right[i]] = n + i
    # representant : plus haut ancetre de meme valeur, par une chaine continue
    rep = [None] * (n + m)
    for b in range(n + m - 1, n - 1, -1):  # parents avant enfants (indices decroissants)
        p = parent[b]
        if p >= 0 and value[p - n] == value[b - n]:
            rep[b] = rep[p]
        else:
            rep[b] = b
    distinct = sorted(set(value))
    rank_of = {x: r for r, x in enumerate(distinct)}
    kids = {}
    for c in range(n + m):
        p = parent[c]
        if p < 0:
            continue
        r = rep[p]
        if c >= n and rep[c] == r:
            continue  # noeud binaire absorbe dans le plateau
        kids.setdefault(r, []).append(c)
    reps = sorted(set(r for r in rep[n:] if r is not None), key=lambda b: (value[b - n], b))
    d = Dendrogram(n)
    new_id = {}
    pending = list(reps)
    # creation dans un ordre topologique : un plateau apres tous ses enfants
    done = set()
    while pending:
        rest = []
        for r in pending:
            ok = all((c < n) or (new_id.get(rep[c]) is not None) for c in kids[r])
            if ok:
                children = [c if c < n else new_id[rep[c]] for c in kids[r]]
                new_id[r] = d.add(children, value[r - n], rank_of[value[r - n]])
                done.add(r)
            else:
                rest.append(r)
        if len(rest) == len(pending):
            raise ValueError('cycle dans l arbre')
        pending = rest
    d.root  # controle
    return d


def from_ultrametric(n, pairs, cmp):
    """Dendrogramme N-aire d'une ultrametrique donnee par ses paires hors diagonale : pairs[(i, j)] = valeur
    (objet compare par cmp(a, b) exact), i < j. Les classes d'egalite exacte forment les plateaux."""
    items = sorted(pairs.items(), key=cmp_to_key(lambda a, b: cmp(a[1], b[1])))
    groups = []
    for key, val in items:
        if groups and cmp(groups[-1][0], val) == 0:
            groups[-1][1].append(key)
        else:
            groups.append([val, [key]])
    d = Dendrogram(n)
    comp = list(range(n))  # union-find sur les points
    node_of = list(range(n))  # noeud courant de chaque racine

    def find(x):
        while comp[x] != x:
            comp[x] = comp[comp[x]]
            x = comp[x]
        return x

    for rank, (val, keys) in enumerate(groups):
        # graphe des composantes reliees a ce niveau
        adj = {}
        for i, j in keys:
            a, b = find(i), find(j)
            if a == b:
                continue
            adj.setdefault(a, set()).add(b)
            adj.setdefault(b, set()).add(a)
        seen = set()
        for start in sorted(adj):
            if start in seen:
                continue
            group, stack = [], [start]
            seen.add(start)
            while stack:
                x = stack.pop()
                group.append(x)
                for y in adj[x]:
                    if y not in seen:
                        seen.add(y)
                        stack.append(y)
            v = d.add([node_of[g] for g in sorted(group)], float(_approx(val)), rank)
            top = min(group)
            for g in group:
                comp[g] = top
            node_of[top] = v
    d.root
    return d


def _approx(val):
    if hasattr(val, 'approx'):
        return val.approx()
    return float(val)


def from_matrix(u, tol=0.0):
    """Ultrametrique flottante (petits nuages, valeurs exactes en flottant : egalite flottante)."""
    n = len(u)
    pairs = {(i, j): float(u[i][j]) for i in range(n) for j in range(i + 1, n)}
    return from_ultrametric(n, pairs, lambda a, b: (a > b) - (a < b))


def ultrametric(d):
    """Matrice de l'ultrametrique (niveaux flottants) du dendrogramme ; diagonale 0."""
    n = d.n
    u = np.zeros((n, n))
    for v in range(n, d.nodes):
        kids = [d.leaves(c) for c in d.children[v]]
        for a in range(len(kids)):
            for b in range(a + 1, len(kids)):
                for i in kids[a]:
                    for j in kids[b]:
                        u[i, j] = u[j, i] = d.value[v]
    return u


# --------------------------------------------------------------------------------------------------------
# Condensation N-aire, stabilite, selection, etiquettes
# --------------------------------------------------------------------------------------------------------

class Cluster(object):
    __slots__ = ('node', 'birth', 'birth_rank', 'parent', 'children', 'death', 'death_rank', 'fall', 'size')

    def __init__(self, node, birth, birth_rank, parent, size):
        self.node, self.birth, self.birth_rank, self.parent, self.size = node, birth, birth_rank, parent, size
        self.children, self.fall = [], []
        self.death, self.death_rank = None, None


def condense(d, mcs):
    """Arbre condense N-aire. clusters[0] = racine (naissance None : lambda 0). fall : (point, niveau, rang)."""
    if mcs < 2:
        raise ValueError('mcs >= 2 (comme sklearn)')
    root = d.root
    clusters = [Cluster(root, None, None, -1, d.size[root])]
    queue = [(root, 0)]
    head = 0
    while head < len(queue):  # largeur d'abord : numerotation proche de celle de sklearn
        v, c = queue[head]
        head += 1
        if v < d.n:
            # une feuille atteinte par continuation (impossible si mcs >= 2) : sortie a son niveau
            clusters[c].fall.append((v, d.value[v], d.rank[v]))
            continue
        level, rank = d.value[v], d.rank[v]
        big = [ch for ch in d.children[v] if d.size[ch] >= mcs]
        small = [ch for ch in d.children[v] if d.size[ch] < mcs]
        for s in small:
            for x in d.leaves(s):
                clusters[c].fall.append((x, level, rank))
        if len(big) >= 2:
            clusters[c].death, clusters[c].death_rank = level, rank
            for b in big:
                k = len(clusters)
                clusters.append(Cluster(b, level, rank, c, d.size[b]))
                clusters[c].children.append(k)
                queue.append((b, k))
        elif len(big) == 1:
            queue.append((big[0], c))
        else:
            clusters[c].death, clusters[c].death_rank = level, rank
    return clusters


def phi_power(z):
    """lambda = niveau^(-z) ; +inf au niveau 0 (comme sklearn)."""
    def phi(level):
        if level is None:
            return 0.0
        if level <= 0.0:
            return INF
        return level ** (-z) if z != 1 else 1.0 / level
    return phi


def stability(clusters, phi):
    out = []
    for c in clusters:
        lb = 0.0 if c.birth is None else phi(c.birth)
        s = 0.0
        for _x, level, _r in c.fall:
            s += phi(level) - lb
        if c.children:
            ld = phi(c.death)
            for k in c.children:
                s += (ld - lb) * clusters[k].size
        out.append(s)
    return out


def descendants(clusters, k):
    out, stack = [], list(clusters[k].children)
    while stack:
        x = stack.pop()
        out.append(x)
        stack.extend(clusters[x].children)
    return out


def select(clusters, stab, method='eom', eps=0.0, asc=False, max_cluster_size=None, eps_mode='consistent'):
    """Ensemble des clusters retenus (indices). eps en unites de niveau (pas de lambda)."""
    n_clusters = len(clusters)
    candidates = list(range(n_clusters)) if asc else list(range(1, n_clusters))
    if method == 'eom':
        if max_cluster_size is None:
            max_cluster_size = INF
        chosen = {k: True for k in candidates}
        tilde = list(stab)
        for k in sorted(candidates, reverse=True):  # enfants (indices plus grands) avant parents
            sub = 0.0
            for ch in clusters[k].children:
                sub += tilde[ch]
            if sub > tilde[k] or clusters[k].size > max_cluster_size:
                chosen[k] = False
                tilde[k] = sub
            else:
                for x in descendants(clusters, k):
                    chosen[x] = False
        selected = set(k for k in candidates if chosen[k])
        if eps != 0.0 and n_clusters > 1:
            if selected == {0}:
                selected = selected if asc else set()
            else:
                selected = epsilon_search(clusters, selected, eps, asc, eps_mode)
        return selected
    if method == 'leaf':
        leaves = set(k for k in range(1, n_clusters) if not clusters[k].children)
        if not leaves:
            return set()
        if eps != 0.0:
            return epsilon_search(clusters, leaves, eps, asc, eps_mode)
        return leaves
    raise ValueError(method)


def epsilon_search(clusters, chosen, eps, asc, mode):
    selected, processed = [], set()
    for leaf in sorted(chosen):
        b = clusters[leaf].birth
        if b is None:
            selected.append(leaf)
            continue
        if b < eps:
            if leaf in processed:
                continue
            top = traverse_upwards(clusters, leaf, eps, asc, mode)
            selected.append(top)
            for x in descendants(clusters, top):
                processed.add(x)
        else:
            selected.append(leaf)
    return set(selected)


def traverse_upwards(clusters, leaf, eps, asc, mode):
    while True:
        parent = clusters[leaf].parent
        if parent == 0:
            return 0 if asc else leaf
        pb = clusters[parent].birth
        if (pb > eps) if mode == 'sklearn' else (pb >= eps):
            return parent
        leaf = parent


def labels(clusters, selected, n, asc=False, eps=0.0, phi=None):
    """Etiquette = rang (dans l'ordre des indices) du plus bas cluster retenu sur la chaine du cluster de sortie."""
    order = sorted(selected)
    label_of = {k: i for i, k in enumerate(order)}
    fall_of = [None] * n
    for k, c in enumerate(clusters):
        for x, level, _r in c.fall:
            fall_of[x] = (k, level)
    out = np.full(n, -1, dtype=np.int64)
    special = asc and selected == {0}
    if special:
        root = clusters[0]
        if eps != 0.0:
            threshold = phi(eps)
        else:
            values = [phi(level) for _x, level, _r in root.fall]
            if root.children:
                values.append(phi(root.death))
            threshold = max(values)
    for x in range(n):
        k, level = fall_of[x]
        while k >= 0 and k not in label_of:
            k = clusters[k].parent
        if k < 0:
            continue
        if k == 0:
            # sklearn (_do_labelling) : un point rattache a la racine n'est etiquete que si la racine est le
            # seul cluster retenu (allow_single_cluster), et seulement au-dela du seuil ; sinon bruit.
            if special and phi(level) >= threshold:
                out[x] = label_of[0]
            continue
        out[x] = label_of[k]
    return out


def head(d, mcs, z=1.0, method='eom', eps=0.0, asc=False, max_cluster_size=None, eps_mode='consistent'):
    """Tete N-aire complete : etiquettes (-1 bruit)."""
    phi = phi_power(z)
    clusters = condense(d, mcs)
    stab = stability(clusters, phi)
    sel = select(clusters, stab, method, eps, asc, max_cluster_size, eps_mode)
    return labels(clusters, sel, d.n, asc, eps, phi)


# --------------------------------------------------------------------------------------------------------
# Binarisation canonique et code de sklearn
# --------------------------------------------------------------------------------------------------------

HIERARCHY_dtype = np.dtype([('left_node', np.intp), ('right_node', np.intp), ('value', np.float64),
                            ('cluster_size', np.intp)])


def binarize(d, mcs=None, z=1.0, order='canonical', rng=None):
    """Arbre du lien simple binaire (format sklearn) equivalent au dendrogramme N-aire. Chaque plateau devient
    une chaine binaire au meme niveau. order='canonical' : gros enfants (taille >= mcs) d'abord, puis petits un a
    un (theoreme B du rapport) ; order='random' : chaine dans un ordre aleatoire (rng) ; order='smallfirst' :
    petits d'abord. value = niveau^z (sklearn lit lambda = 1 / value = niveau^(-z))."""
    n = d.n
    rows = []
    ident = {}  # noeud du dendrogramme -> identifiant sklearn

    def emit(a, b, level, size):
        rows.append((a, b, level ** z if level > 0 else 0.0, size))
        return n + len(rows) - 1

    for v in range(n):
        ident[v] = v
    for v in range(n, d.nodes):  # enfants crees avant parents
        kids = list(d.children[v])
        if order == 'canonical':
            big = sorted([c for c in kids if d.size[c] >= mcs], key=lambda c: d.leaves(c)[0])
            small = sorted([c for c in kids if d.size[c] < mcs], key=lambda c: d.leaves(c)[0])
            seq = big + small
        elif order == 'smallfirst':
            big = sorted([c for c in kids if d.size[c] >= mcs], key=lambda c: d.leaves(c)[0])
            small = sorted([c for c in kids if d.size[c] < mcs], key=lambda c: d.leaves(c)[0])
            seq = small + big
        elif order == 'random':
            seq = list(kids)
            rng.shuffle(seq)
        else:
            raise ValueError(order)
        cur, size = ident[seq[0]], d.size[seq[0]]
        for c in seq[1:]:
            size += d.size[c]
            cur = emit(cur, ident[c], d.value[v], size)
        ident[v] = cur
    if len(rows) != n - 1:
        raise ValueError('binarisation : %d lignes' % len(rows))
    return np.array(rows, dtype=HIERARCHY_dtype)


def sklearn_tree_to_labels(tree, mcs, method='eom', asc=False, eps_value=0.0, max_cluster_size=None):
    """Appel du code de sklearn (aucune reimplementation) : condensation, stabilite, selection, etiquettes."""
    from sklearn.cluster._hdbscan._tree import tree_to_labels
    tree = np.ascontiguousarray(tree, dtype=HIERARCHY_dtype)
    labs, _prob = tree_to_labels(tree, mcs, method, asc, eps_value, max_cluster_size)
    return np.asarray(labs, dtype=np.int64)


def sklearn_head(d, mcs, z=1.0, method='eom', eps=0.0, asc=False, max_cluster_size=None, order='canonical',
                 rng=None):
    tree = binarize(d, mcs, z, order, rng)
    return sklearn_tree_to_labels(tree, mcs, method, asc, eps ** z if eps else 0.0, max_cluster_size)


# --------------------------------------------------------------------------------------------------------
# HDBSCAN tel quel, et son arbre
# --------------------------------------------------------------------------------------------------------

def hdbscan_fit(X, k, mcs, method='eom', eps=0.0, asc=False, alpha=1.0, algorithm='kd_tree'):
    from sklearn.cluster import HDBSCAN
    model = HDBSCAN(min_cluster_size=mcs, min_samples=k, cluster_selection_method=method,
                    cluster_selection_epsilon=eps, allow_single_cluster=asc, alpha=alpha, algorithm=algorithm,
                    metric='euclidean', n_jobs=1, copy=True)
    model.fit(np.asarray(X, dtype=np.float64))
    return model


# --------------------------------------------------------------------------------------------------------
# Comparaison de partitions
# --------------------------------------------------------------------------------------------------------

def canonical_partition(labels):
    """Partition comme ensemble de frozensets (bruit a part)."""
    groups = {}
    for i, l in enumerate(np.asarray(labels).tolist()):
        if l >= 0:
            groups.setdefault(l, []).append(i)
    noise = frozenset(i for i, l in enumerate(np.asarray(labels).tolist()) if l < 0)
    return frozenset(frozenset(g) for g in groups.values()), noise


def same_partition(a, b):
    return canonical_partition(a) == canonical_partition(b)


def points_differing(a, b):
    """Nombre de points dont le bloc (bruit = singleton propre) differe entre deux etiquetages."""
    a, b = np.asarray(a), np.asarray(b)
    n = len(a)
    ga, gb = {}, {}
    for i in range(n):
        ga.setdefault(a[i], set()).add(i) if a[i] >= 0 else None
        gb.setdefault(b[i], set()).add(i) if b[i] >= 0 else None
    bad = 0
    for i in range(n):
        sa = frozenset(ga[a[i]]) if a[i] >= 0 else frozenset([i])
        sb = frozenset(gb[b[i]]) if b[i] >= 0 else frozenset([i])
        if sa != sb:
            bad += 1
    return bad


# --------------------------------------------------------------------------------------------------------
# Antichaines oracles contre la verite
# --------------------------------------------------------------------------------------------------------

def block_counts(d, gt, void=None):
    """Par noeud : taille hors void et comptes par objet vrai (gt >= 0). Rend (size, counts, totals)."""
    n = d.n
    gt = np.asarray(gt)
    void = np.zeros(n, dtype=bool) if void is None else np.asarray(void, dtype=bool)
    size = [0] * d.nodes
    counts = [None] * d.nodes
    for v in range(d.nodes):
        if v < n:
            size[v] = 0 if void[v] else 1
            counts[v] = {int(gt[v]): 1} if (gt[v] >= 0 and not void[v]) else {}
        else:
            s, c = 0, {}
            for ch in d.children[v]:
                s += size[ch]
                for o, x in counts[ch].items():
                    c[o] = c.get(o, 0) + x
            size[v], counts[v] = s, c
    totals = {}
    for i in range(n):
        if gt[i] >= 0 and not void[i]:
            totals[int(gt[i])] = totals.get(int(gt[i]), 0) + 1
    return size, counts, totals


def iou_table(d, gt, void=None):
    size, counts, totals = block_counts(d, gt, void)
    table = []
    for v in range(d.nodes):
        row = {}
        for o, c in counts[v].items():
            row[o] = Fraction(c, size[v] + totals[o] - c)
        table.append(row)
    return table, totals


def candidate_ok(d, v, mcs, root_allowed):
    if v < d.n and mcs > 1:
        return False
    if d.size[v] < mcs:
        return False
    if not root_allowed and d.parent[v] < 0:
        return False
    return True


def oracle_matched(d, gt, mcs=2, void=None, root_allowed=False, candidates=None):
    """Antichaine maximisant la somme des IoU > 1/2 (chaque objet apparie a au plus un bloc, unique) :
    programme dynamique exact sur l'arbre. Rend (somme, antichaine, appariement)."""
    table, totals = iou_table(d, gt, void)
    weight = []
    match = []
    for v in range(d.nodes):
        best, who = Fraction(0), None
        ok = candidate_ok(d, v, mcs, root_allowed) and (candidates is None or v in candidates)
        if ok:
            for o, x in table[v].items():
                if x > Fraction(1, 2) and x > best:
                    best, who = x, o
        weight.append(best)
        match.append(who)
    f = [Fraction(0)] * d.nodes
    take = [False] * d.nodes
    for v in range(d.nodes):  # enfants avant parents
        below = sum((f[c] for c in d.children[v]), Fraction(0)) if v >= d.n else Fraction(0)
        if weight[v] > below:
            f[v], take[v] = weight[v], True
        else:
            f[v] = below
    chosen, stack = [], [d.root]
    while stack:
        v = stack.pop()
        if take[v]:
            chosen.append(v)
        elif v >= d.n:
            stack.extend(d.children[v])
    return f[d.root], sorted(chosen), {v: match[v] for v in chosen}, totals


def oracle_pq(d, gt, mcs=2, void=None, root_allowed=False, candidates=None):
    """Antichaine maximisant PQ = somme IoU / (TP + FP/2 + FN/2). Les blocs non apparies n'aident jamais :
    FP = 0 a l'optimum, PQ = 2 S(t) / (|G| + t) avec S(t) la meilleure somme a t appariements exactement
    (programme dynamique en sac a dos sur l'arbre)."""
    table, totals = iou_table(d, gt, void)
    G = len(totals)
    NEG = None

    def merge(a, b):
        out = [NEG] * (min(len(a) + len(b) - 1, G + 1))
        for i, x in enumerate(a):
            if x is NEG:
                continue
            for j, y in enumerate(b):
                if y is NEG or i + j > G:
                    continue
                if out[i + j] is NEG or x + y > out[i + j]:
                    out[i + j] = x + y
        return out

    tab = [None] * d.nodes
    for v in range(d.nodes):
        if v < d.n:
            cur = [Fraction(0)]
        else:
            cur = [Fraction(0)]
            for c in d.children[v]:
                cur = merge(cur, tab[c])
        ok = candidate_ok(d, v, mcs, root_allowed) and (candidates is None or v in candidates)
        if ok:
            best = max([x for x in table[v].values() if x > Fraction(1, 2)], default=None)
            if best is not None:
                if len(cur) < 2:
                    cur = cur + [NEG]
                if cur[1] is NEG or best > cur[1]:
                    cur[1] = best
        tab[v] = cur
    top = tab[d.root]
    best_pq, best_t = Fraction(0), 0
    for t, s in enumerate(top):
        if s is NEG or t == 0:
            continue
        pq = 2 * s / (G + t)
        if pq > best_pq:
            best_pq, best_t = pq, t
    return best_pq, best_t, G


def oracle_bestmatch_milp(d, gt, mcs=2, void=None, root_allowed=False, candidates=None):
    """Antichaine maximisant la somme sur les objets du meilleur IoU (sans seuil) : programme lineaire en
    nombres entiers (scipy.optimize.milp, HiGHS). y_v binaire (bloc choisi), x_ov continu (objet o servi par v),
    x_ov <= y_v, somme_v x_ov <= 1, antichaine : pour chaque feuille, somme des y de ses ancetres <= 1."""
    from scipy.optimize import milp, LinearConstraint, Bounds
    from scipy.sparse import lil_matrix
    table, totals = iou_table(d, gt, void)
    cand = [v for v in range(d.nodes) if candidate_ok(d, v, mcs, root_allowed) and
            (candidates is None or v in candidates) and table[v]]
    yidx = {v: i for i, v in enumerate(cand)}
    pairs = [(o, v) for v in cand for o in table[v]]
    nx = len(pairs)
    ny = len(cand)
    nvar = ny + nx
    if nvar == 0:
        return Fraction(0), []
    c = np.zeros(nvar)
    for j, (o, v) in enumerate(pairs):
        c[ny + j] = -float(table[v][o])
    rows = []
    # x_ov - y_v <= 0
    A1 = lil_matrix((nx, nvar))
    for j, (o, v) in enumerate(pairs):
        A1[j, ny + j] = 1.0
        A1[j, yidx[v]] = -1.0
    objs = sorted(set(o for o, _ in pairs))
    A2 = lil_matrix((len(objs), nvar))
    oi = {o: i for i, o in enumerate(objs)}
    for j, (o, v) in enumerate(pairs):
        A2[oi[o], ny + j] = 1.0
    A3 = lil_matrix((d.n, nvar))
    for leaf in range(d.n):
        v = d.parent[leaf]
        while v >= 0:
            if v in yidx:
                A3[leaf, yidx[v]] = 1.0
            v = d.parent[v]
    cons = [LinearConstraint(A1.tocsr(), -np.inf, 0.0), LinearConstraint(A2.tocsr(), -np.inf, 1.0),
            LinearConstraint(A3.tocsr(), -np.inf, 1.0)]
    integrality = np.r_[np.ones(ny), np.zeros(nx)]
    res = milp(c, constraints=cons, integrality=integrality, bounds=Bounds(0, 1))
    if not res.success:
        raise RuntimeError('milp : ' + str(res.message))
    chosen = [cand[i] for i in range(ny) if res.x[i] > 0.5]
    # valeur exacte recalculee sur l'antichaine choisie
    total = Fraction(0)
    for o in totals:
        total += max([table[v].get(o, Fraction(0)) for v in chosen], default=Fraction(0))
    return total, sorted(chosen)


def antichains(d, mcs=2, root_allowed=False):
    """Toutes les antichaines de candidats (petits arbres seulement, controle par force brute)."""
    memo = {}

    def rec(v):
        if v in memo:
            return memo[v]
        opts = [frozenset()]
        if v >= d.n:
            opts = [frozenset()]
            for c in d.children[v]:
                sub = rec(c)
                opts = [a | b for a in opts for b in sub]
        if candidate_ok(d, v, mcs, root_allowed):
            opts = opts + [frozenset([v])]
        memo[v] = opts
        return opts
    return rec(d.root)


def score_flat(labels, gt, void=None):
    """Scores d'une sortie plate contre la verite (meme code pour toutes les lignes) :
    m05 = somme des IoU > 1/2 / |G| ; best = moyenne sur les objets du meilleur IoU ; pq, sq, rq."""
    labels, gt = np.asarray(labels), np.asarray(gt)
    n = len(labels)
    void = np.zeros(n, dtype=bool) if void is None else np.asarray(void, dtype=bool)
    keep = ~void
    totals, sizes, inter = {}, {}, {}
    for i in range(n):
        if not keep[i]:
            continue
        o, l = int(gt[i]), int(labels[i])
        if o >= 0:
            totals[o] = totals.get(o, 0) + 1
        if l >= 0:
            sizes[l] = sizes.get(l, 0) + 1
            if o >= 0:
                inter[(o, l)] = inter.get((o, l), 0) + 1
    G = len(totals)
    best = {o: Fraction(0) for o in totals}
    matched, tp_iou = set(), Fraction(0)
    for (o, l), c in inter.items():
        x = Fraction(c, sizes[l] + totals[o] - c)
        if x > best[o]:
            best[o] = x
        if x > Fraction(1, 2):
            matched.add(l)
            tp_iou += x
    tp = len(matched)
    fp = len(sizes) - tp
    fn = G - tp
    m05 = tp_iou / G if G else Fraction(0)
    mbest = sum(best.values(), Fraction(0)) / G if G else Fraction(0)
    denom = Fraction(2 * tp + fp + fn, 2)
    pq = tp_iou / denom if denom else Fraction(0)
    return dict(m05=float(m05), best=float(mbest), pq=float(pq), tp=tp, fp=fp, fn=fn, clusters=len(sizes),
                noise=int(np.sum((labels < 0) & keep)))
