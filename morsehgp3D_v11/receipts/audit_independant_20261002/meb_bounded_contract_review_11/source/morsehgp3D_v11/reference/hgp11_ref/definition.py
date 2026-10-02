"""Etage A de la reference : oracle de DEFINITION, exhaustif, en fractions exactes.

Verite. Pour un nuage X de n points a coordonnees entieres (doublons permis : deux points de meme position sont deux
points) et un ordre k, D_k(y) est la k-ieme plus petite distance carree de y aux points, multiplicites comprises, et
L_k(a) = { y : D_k(y) <= a }. Le theoreme 2 du manuscrit (identite de definitions : L_k(a) est la reunion des regions
temoins convexes des k-parties) identifie les composantes de L_k(a) a celles du graphe Gamma_k(a) :
  sommets : les k-parties F de X, au niveau beta(F) = rayon carre de leur boule minimale englobante ;
  aretes  : les paires de k-parties dont la reunion G a k + 1 points, au niveau beta(G).
Cet etage ne contient QUE cela : aucune boule critique, aucun support canonique, aucun morceau, aucune descente,
aucun ordre de Morton. Il ne partage avec l'etage B que les enregistrements de model.py : la numerotation
canonique, les parents et la lecture d'une coupe sont ecrits ici, pour cet etage seul.

Boule minimale englobante : par enumeration exacte des supports (pas Welzl). La boule minimale de F est la plus
petite, parmi les spheres circonscrites des parties de 1 a 4 points affinement independants de F, de celles qui
contiennent F : la boule minimale a un tel support, donc elle est candidate ; toute candidate contient F, donc son
rayon est au moins le rayon minimal ; la boule minimale est unique.

Ce que l'etage publie par ordre (OrderResult) : l'arbre de fusion (naissances, fusions N-aires par plateau exact),
les coupes ouvertes et fermees a chaque niveau d'evenement (par composante : noeud, points couverts = amas discret,
points de coeur = C n X), les entrees core et cover de chaque point, l'application verticale vers l'ordre k - 1.

Cout : C(n, k) + C(n, k + 1) boules minimales par ordre. Domaine d'usage : n <= 12 a 14.
"""
from fractions import Fraction
from itertools import combinations
from math import gcd

from .model import Cut, Entry, InvariantError, Node, OrderResult, mask_of


def _solve(rows, rhs):
    """Systeme lineaire carre en fractions (elimination de Gauss-Jordan) ; None s'il est singulier."""
    size = len(rows)
    m = [[Fraction(v) for v in rows[i]] + [Fraction(rhs[i])] for i in range(size)]
    for col in range(size):
        piv = next((r for r in range(col, size) if m[r][col] != 0), None)
        if piv is None:
            return None
        m[col], m[piv] = m[piv], m[col]
        for r in range(size):
            if r != col and m[r][col] != 0:
                f = m[r][col] / m[col][col]
                m[r] = [x - f * y for x, y in zip(m[r], m[col])]
    return [m[i][size] / m[i][i] for i in range(size)]


def circumsphere(pts):
    """Sphere circonscrite de centre dans l'enveloppe affine des points : (centre, rayon carre), ou None si les
    points ne sont pas affinement independants. Le centre p0 + sum lam_j d_j (d_j = p_j - p0) est equidistant de
    p0 et de p_i si et seulement si 2 d_i . (sum lam_j d_j) = |d_i|^2."""
    p0 = pts[0]
    if len(pts) == 1:
        return tuple(Fraction(c) for c in p0), Fraction(0)
    d = [tuple(a - b for a, b in zip(p, p0)) for p in pts[1:]]
    gram = [[2 * sum(x * y for x, y in zip(a, b)) for b in d] for a in d]
    lam = _solve(gram, [sum(x * x for x in a) for a in d])
    if lam is None:
        return None
    center = tuple(Fraction(p0[j]) + sum(w * v[j] for w, v in zip(lam, d)) for j in range(3))
    return center, sum((center[j] - p0[j]) ** 2 for j in range(3))


class Definition(object):
    """Oracle de definition d'un nuage. order(k) rend l'OrderResult de l'ordre k (1 <= k <= n), calcule une fois."""

    def __init__(self, points):
        self.points = []
        for p in points:
            if len(p) != 3:
                raise ValueError('point a trois coordonnees attendu : %r' % (p,))
            self.points.append(tuple(int(c) for c in p))
        if not self.points:
            raise ValueError('nuage vide')
        self.n = len(self.points)
        self._spheres = {}
        self._mebs = {}
        self._near = {}
        self._orders = {}
        self._vertex_node = {}
        self._parent = {}

    # ------------------------------------------------------------ boules minimales

    def _sphere(self, support):
        """Sphere circonscrite du support (indices de points) : (rayon carre, centre, masque de la boule fermee)."""
        if support not in self._spheres:
            cs = circumsphere([self.points[i] for i in support])
            if cs is not None:
                center, level = cs
                # |p - c|^2 <= r^2 en entiers : avec c = num / den (den commun), den^2 |p - c|^2 = |den p - num|^2
                den = 1
                for c in center:
                    den = den * c.denominator // gcd(den, c.denominator)
                num = [int(c * den) for c in center]
                bound = level * den * den
                closed = 0
                for i, p in enumerate(self.points):
                    if sum((p[j] * den - num[j]) ** 2 for j in range(3)) <= bound:
                        closed |= 1 << i
                cs = (level, center, closed)
            self._spheres[support] = cs
        return self._spheres[support]

    def meb(self, part):
        """Boule minimale englobante d'une partie (tuple trie d'indices) : (rayon carre, centre, masque ferme)."""
        best = self._mebs.get(part)
        if best is None:
            inside = mask_of(part)
            for q in range(1, min(4, len(part)) + 1):
                for support in combinations(part, q):
                    cand = self._sphere(support)
                    if cand is None or (best is not None and cand[0] >= best[0]) or inside & ~cand[2]:
                        continue
                    best = cand
            if best is None:
                raise InvariantError('aucune sphere candidate ne contient la partie %r' % (part,))
            self._mebs[part] = best
        return best

    def beta(self, part):
        """Rayon carre de la boule minimale d'une partie (indices quelconques)."""
        return self.meb(tuple(sorted(part)))[0]

    def nearest(self, x):
        """Points tries par (distance carree a x, indice), x compris (distance 0)."""
        if x not in self._near:
            px = self.points[x]
            self._near[x] = sorted((sum((a - b) ** 2 for a, b in zip(px, p)), y) for y, p in enumerate(self.points))
        return self._near[x]

    # ------------------------------------------------------------ un ordre

    def order(self, k):
        if not 1 <= k <= self.n:
            raise ValueError('ordre %d hors de [1, %d]' % (k, self.n))
        if k not in self._orders:
            self._orders[k] = self._sweep(k)
        return self._orders[k]

    def node_at(self, k, part, level):
        """Noeud de l'ordre k vivant a la coupe fermee dont la composante contient la k-partie (beta(part) <= level).
        La coupe est lue dans le balayage du graphe : on remonte du noeud du sommet jusqu'a un noeud de la coupe."""
        res = self.order(k)
        part = tuple(sorted(part))
        if self.meb(part)[0] > level:
            raise InvariantError('partie %r nee apres la coupe %s' % (part, level))
        return _climb(self._parent[k], self._vertex_node[k][part], _alive_at(res.cuts, level))

    def _sweep(self, k):
        """Balayage de Gamma_k par niveaux croissants ; a chaque niveau : sommets, puis aretes, puis entrees."""
        n = self.n
        at = {}

        def bucket(level):
            if level not in at:
                at[level] = ([], [], [])
            return at[level]
        vcenter = {}
        for part in combinations(range(n), k):
            level, center, _closed = self.meb(part)
            vcenter[part] = center
            bucket(level)[0].append(part)
        if k < n:
            for union in combinations(range(n), k + 1):
                bucket(self.meb(union)[0])[1].append(union)
        knn = []
        for x in range(n):
            near = self.nearest(x)
            knn.append(tuple(sorted(y for _d, y in near[:k])))
            bucket(Fraction(near[k - 1][0]))[2].append(x)
        parent = {}

        def find(v):
            root = v
            while parent[root] != root:
                root = parent[root]
            while parent[v] != root:
                parent[v], v = root, parent[v]
            return root
        state = {}             # racine -> [couverture, coeur, noeud provisoire]
        births, merges = [], []  # noeuds provisoires : naissance i -> i ; fusion j -> -1 - j
        witness = {}           # noeud provisoire -> un sommet de sa composante, a sa creation
        vnode = {}             # sommet -> noeud provisoire vivant a la coupe fermee du niveau du sommet
        core = [None] * n
        cover = [None] * n
        raw = []
        last = ()
        for level in sorted(at):
            new_parts, edges, entering = at[level]
            opened = last
            # racine modifiee a ce niveau -> [noeuds anterieurs contenus, centre commun de ses sommets neufs]
            dirty = {}
            for part in new_parts:
                parent[part] = part
                state[part] = [mask_of(part), 0, None]
                dirty[part] = [set(), vcenter[part]]
            for union in edges:
                r0 = find(union[1:])
                for j in range(1, k + 1):
                    r1 = find(union[:j] + union[j + 1:])
                    if r1 == r0:
                        continue
                    d0 = dirty.pop(r0, None)
                    if d0 is None:
                        d0 = [set([state[r0][2]]), None]
                    d1 = dirty.pop(r1, None)
                    if d1 is None:
                        d1 = [set([state[r1][2]]), None]
                    fresh = not d0[0] and not d1[0]
                    if fresh and d0[1] != d1[1]:
                        raise InvariantError('k=%d niveau %s : composante neuve a deux boules minimales' % (k, level))
                    s1 = state.pop(r1)
                    s0 = state[r0]
                    s0[0] |= s1[0]
                    s0[1] |= s1[1]
                    s0[2] = None
                    parent[r1] = r0
                    dirty[r0] = [d0[0] | d1[0], d0[1] if fresh else None]
            for root, (olds, center) in dirty.items():
                st = state[root]
                if not olds:                 # aucune composante de la coupe ouverte : naissance
                    st[2] = len(births)
                    births.append((level, center))
                    witness[st[2]] = root
                elif len(olds) == 1:         # une seule : la composante continue
                    st[2] = next(iter(olds))
                else:                        # plusieurs : une fusion N-aire, plateau atomique
                    st[2] = -1 - len(merges)
                    merges.append((level, sorted(olds)))
                    witness[st[2]] = root
            for part in new_parts:
                node = state[find(part)][2]
                vnode[part] = node
                for x in part:
                    if cover[x] is None:
                        cover[x] = (level, set())
                    if cover[x][0] == level:
                        cover[x][1].add(node)
            for x in entering:
                st = state[find(knn[x])]
                st[1] |= 1 << x
                core[x] = (level, st[2])
            last = tuple((st[2], st[0], st[1]) for st in state.values())
            raw.append((level, opened, last))
        return self._publish(k, births, merges, witness, vnode, core, cover, raw)

    def _publish(self, k, births, merges, witness, vnode, core, cover, raw):
        """Numerotation canonique (naissances par niveau puis centre, fusions par niveau puis plus petite naissance),
        puis application verticale vers l'ordre k - 1."""
        if len(set(births)) != len(births):
            raise InvariantError('k=%d : deux naissances de meme boule' % k)
        final = {}  # noeud provisoire -> noeud canonique
        for new, old in enumerate(sorted(range(len(births)), key=lambda i: births[i])):
            final[old] = new
        first = dict(final)  # noeud provisoire -> plus petite naissance canonique de son sous-arbre
        for j, (_level, kids) in enumerate(merges):  # une fusion est creee apres ses enfants
            first[-1 - j] = min(first[c] for c in kids)
        ranked = sorted(range(len(merges)), key=lambda j: (merges[j][0], first[-1 - j]))
        for new, j in enumerate(ranked):
            final[-1 - j] = len(births) + new
        nodes = [Node(level, (), center) for level, center in sorted(births)]
        nodes += [Node(merges[j][0], tuple(sorted(final[c] for c in merges[j][1])), None) for j in ranked]
        parent = [-1] * len(nodes)
        for v, node in enumerate(nodes):
            for c in node.children:
                parent[c] = v
        memo = {}

        def canon(snapshot):
            key = id(snapshot)
            if key not in memo:
                memo[key] = (snapshot, tuple(sorted((final[v], cov, cor) for v, cov, cor in snapshot)))
            return memo[key][1]
        cuts = [Cut(level, canon(opened), canon(closed)) for level, opened, closed in raw]
        self._vertex_node[k] = dict((part, final[p]) for part, p in vnode.items())
        self._parent[k] = parent
        lower = None
        if k > 1:
            prev = self.order(k - 1)
            below = self._vertex_node[k - 1]
            lower = [None] * len(nodes)
            for p, part in witness.items():
                v = final[p]
                lower[v] = _climb(self._parent[k - 1], below[part[:-1]], _alive_at(prev.cuts, nodes[v].level))
        return OrderResult(k, nodes, lower,
                           [Entry(level, final[p]) for level, p in core],
                           [Entry(level, frozenset(final[p] for p in ps)) for level, ps in cover], cuts)


def _alive_at(cuts, level):
    """Noeuds de la coupe fermee a un niveau quelconque : ceux de la derniere coupe d'evenement de niveau <= level
    (entre deux niveaux d'evenement rien ne change)."""
    alive = ()
    for cut in cuts:
        if cut.level > level:
            break
        alive = cut.closed
    return set(entry[0] for entry in alive)


def _climb(parent, node, alive):
    """Premier ancetre (au sens large) du noeud qui appartient a l'ensemble des noeuds de la coupe."""
    while node not in alive:
        node = parent[node]
        if node < 0:
            raise InvariantError('aucun ancetre du noeud dans la coupe')
    return node
