"""Arbre FULL_K et relation de couverture (definition 8), en arithmetique exacte.

Deux constructions independantes du meme objet :
  - `gamma_tree(P, K)` : enumeration de toutes les K-parties et (K+1)-parties (theoreme 2 : composantes de
    Gamma_K = composantes de L_K), plateaux exacts traites en un seul evenement ; borne n <= 12 environ ;
  - `native_tree(export)` : foret FULL de l'export natif `export_frontier` et temoins forts du lemme de couverture
    (population >= K, p + q_min <= K, incidences I u U), remontes par ancetres.

Objet commun (classe `Tree` + table `cover`) :
  - noeud v : naissance b_v, mort d_v (None pour la racine : d = +infini), parent, enfants ; coupe FERMEE :
    v vivant a beta ssi b_v <= beta < d_v ;
  - cover[x] = {v : c_x(v)} ou c_x(v) = premier niveau (rayon carre) de la vie de v auquel v couvre x
    (dist(x, C_v) <= r). V_x = cles de cover[x] est un ensemble clos vers le haut, et c_x(parent) = b_parent.

Niveaux : Fraction (rayons CARRES). Aucune decision flottante, aucun assert (tient sous python3 -O).
"""
from fractions import Fraction
from itertools import combinations
import importlib.util
import os

HERE = os.path.dirname(os.path.abspath(__file__))
SNAP = os.path.join(os.path.dirname(HERE), 'source_snapshot')


class MathError(RuntimeError):
    """Incoherence mathematique ou d'entree (jamais un assert)."""


def require(cond, msg):
    if not cond:
        raise MathError(msg)


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


REF = load_module('hgp10_ref_snapshot', os.path.join(SNAP, 'hgp10_ref.py'))


class Tree:
    """Arbre de fusion : listes birth, death (None = infini), parent (-1 = racine), children."""

    def __init__(self, birth, death, parent, children, tag=None):
        self.birth = [Fraction(b) for b in birth]
        self.death = [None if d is None else Fraction(d) for d in death]
        self.parent = list(parent)
        self.children = [list(c) for c in children]
        self.tag = tag if tag is not None else [None] * len(birth)
        n = len(self.birth)
        roots = [v for v in range(n) if self.parent[v] < 0]
        require(len(roots) == 1, 'arbre a %d racines' % len(roots))
        self.root = roots[0]
        for v in range(n):
            p = self.parent[v]
            if p >= 0:
                require(self.death[v] == self.birth[p], 'mort != naissance du parent (noeud %d)' % v)
                require(self.birth[v] < self.birth[p], 'parent non posterieur (noeud %d)' % v)
                require(v in self.children[p], 'enfant manquant')
            else:
                require(self.death[v] is None, 'racine mortelle')
        depth = [None] * n
        depth[self.root] = 0
        stack = [self.root]
        order = []
        while stack:
            v = stack.pop()
            order.append(v)
            for c in self.children[v]:
                depth[c] = depth[v] + 1
                stack.append(c)
        require(all(d is not None for d in depth), 'noeud hors arbre')
        self.depth = depth
        self.topdown = order                     # parents avant enfants
        self.bottomup = list(reversed(order))    # enfants avant parents
        # rangs entiers des niveaux (comparaisons exactes rapides : une conversion par coupe)
        self.level_list = sorted(set(self.birth))
        rank = {b: i for i, b in enumerate(self.level_list)}
        big = len(self.level_list) + 1
        self.brank = [rank[b] for b in self.birth]
        self.drank = [big if d is None else rank[d] for d in self.death]

    def __len__(self):
        return len(self.birth)

    def alive(self, v, beta):
        d = self.death[v]
        return self.birth[v] <= beta and (d is None or beta < d)

    def cut_rank(self, beta):
        """Indice du plus grand niveau <= beta (coupe fermee), -1 si aucun."""
        from bisect import bisect_right
        return bisect_right(self.level_list, beta) - 1

    def anc(self, v, beta):
        """Ancetre de v vivant a beta (coupe fermee) ; None si v n'est pas encore ne."""
        return self.anc_rank(v, self.cut_rank(beta))

    def anc_rank(self, v, r):
        """Ancetre de v vivant au rang de coupe r (b_v <= niveau r < d_v)."""
        if self.brank[v] > r:
            return None
        drank, parent = self.drank, self.parent
        while drank[v] <= r:
            v = parent[v]
        return v

    def is_desc(self, u, v):
        """u descendant de v (ou egal)."""
        while self.depth[u] > self.depth[v]:
            u = self.parent[u]
        return u == v

    def lca(self, u, v):
        while self.depth[u] > self.depth[v]:
            u = self.parent[u]
        while self.depth[v] > self.depth[u]:
            v = self.parent[v]
        while u != v:
            u, v = self.parent[u], self.parent[v]
        return u

    def levels(self):
        return sorted(set(self.birth))


# ------------------------------------------------------------------ construction par Gamma_K (petits nuages)

def gamma_tree(P, K):
    """FULL_K par Gamma_K exhaustif ; rend (Tree, cover, info). P : liste de triplets entiers distincts."""
    n = len(P)
    require(len(set(P)) == n, 'sites non distincts')
    require(2 <= K <= n, 'K hors domaine (K = 1 : la couverture est deja une partition)')
    beta_v = {F: REF.meb(P, F)[0] for F in combinations(range(n), K)}
    beta_e = {G: REF.meb(P, G)[0] for G in combinations(range(n), K + 1)} if K < n else {}
    levels = sorted(set(beta_v.values()) | set(beta_e.values()))
    dsu = REF.DSU()
    alive = []
    branch = {}
    birth, death, parent, children, first = [], [], [], [], []
    vertex_node = {}
    for beta in levels:
        prior = dict(branch)
        for F, b in beta_v.items():
            if b == beta:
                dsu.find(F)
                alive.append(F)
        for G, b in beta_e.items():
            if b == beta:
                fs = list(combinations(G, K))
                for F in fs[1:]:
                    dsu.union(fs[0], F)
        comps = {}
        for F in alive:
            comps.setdefault(dsu.find(F), []).append(F)
        branch = {}
        for root in sorted(comps):
            Fs = comps[root]
            prev = sorted({prior[F] for F in Fs if F in prior})
            if len(prev) == 1:
                v = prev[0]
            else:
                v = len(birth)
                birth.append(beta)
                death.append(None)
                parent.append(-1)
                children.append(prev)
                first.append({})
                for c in prev:
                    require(death[c] is None, 'branche morte deux fois')
                    death[c] = beta
                    parent[c] = v
            for F in Fs:
                branch[F] = v
                if beta_v[F] == beta:
                    vertex_node[F] = v
            for x in {x for F in Fs for x in F}:
                first[v].setdefault(x, beta)
    T = Tree(birth, death, parent, children)
    cover = [dict() for _ in range(n)]
    for v in range(len(T)):
        for x, b in first[v].items():
            cover[x][v] = b
    check_cover(T, cover)
    info = {'n': n, 'K': K, 'levels': len(levels), 'nodes': len(T),
            'vertices': len(beta_v), 'edges': len(beta_e), 'construction': 'gamma_exhaustive',
            'beta_v': beta_v, 'beta_e': beta_e, 'vertex_node': vertex_node}
    return T, cover, info


def check_cover(T, cover):
    """Invariants de la couverture : c_x(v) dans la vie de v ; clos vers le haut ; c_x(parent) = b_parent."""
    for x, cv in enumerate(cover):
        require(cv, 'point %d jamais couvert' % x)
        for v, c in cv.items():
            require(T.birth[v] <= c and (T.death[v] is None or c < T.death[v]), 'c_x hors vie (%d,%d)' % (x, v))
            p = T.parent[v]
            if p >= 0:
                require(p in cv and cv[p] == T.birth[p], 'couverture non close vers le haut (%d,%d)' % (x, v))


# ------------------------------------------------------------------ construction depuis l'export natif

def native_tree(export, k=None):
    """FULL_K natif + couverture par le lemme du catalogue (temoins forts). Rend (Tree, cover, info)."""
    fc = FC()
    o = export.order(k)
    f = o.forest
    K = o.k
    require(K >= 2, 'K = 1 hors domaine')
    n_nodes = len(f)
    birth = [f.level(v) for v in range(n_nodes)]
    death = [None if f.parent[v] == fc.NONE else f.level(f.parent[v]) for v in range(n_nodes)]
    parent = [-1 if f.parent[v] == fc.NONE else f.parent[v] for v in range(n_nodes)]
    children = [list(f.children[v]) for v in range(n_nodes)]
    T = Tree(birth, death, parent, children)
    W = fc.witness_universe(export, K, strong=True)
    cover = []
    n_wit = 0
    for s in range(len(export.sites)):
        cv = {}
        for _b, lvl, node in sorted(W[s], key=lambda t: (t[1], t[2])):
            n_wit += 1
            lvl = Fraction(lvl)
            v = node
            while True:
                cand = max(T.birth[v], lvl)
                d = T.death[v]
                if d is not None and cand >= d:
                    v = T.parent[v]
                    continue
                old = cv.get(v)
                if old is not None and old <= cand:
                    break
                cv[v] = cand
                if T.parent[v] < 0:
                    break
                v = T.parent[v]
                lvl = T.birth[v]
        cover.append(cv)
    check_cover(T, cover)
    info = {'n': len(export.sites), 'K': K, 'nodes': n_nodes, 'witness_incidences': n_wit,
            'construction': 'native_export_plus_covering_lemma'}
    return T, cover, info


_FC = None


def FC():
    global _FC
    if _FC is None:
        _FC = load_module('frontier_core_snapshot', os.path.join(SNAP, 'frontier_core.py'))
    return _FC


# ------------------------------------------------------------------ signatures canoniques (comparaison d'arbres)

def point_signature(T, cover, x):
    """Multi-ensemble trie des (c_x(v), d_v) : invariant de la couverture de x, independant des numerotations."""
    return sorted((str(c), 'inf' if T.death[v] is None else str(T.death[v])) for v, c in cover[x].items())


def tree_signature(T):
    """Multi-ensemble des (naissance, mort) des noeuds."""
    return sorted((str(T.birth[v]), 'inf' if T.death[v] is None else str(T.death[v])) for v in range(len(T)))
