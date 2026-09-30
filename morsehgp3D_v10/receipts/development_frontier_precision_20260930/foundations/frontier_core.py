"""Noyau exact de l'experience frontiere (Fondations A) : export natif FULL a K fixe, requetes exactes.

Contenu :
  - lecture d'un export `export_frontier` (schema mhgp10_frontier_export_v1) : niveaux exacts, sites, catalogue
    critique (centre exact, I, U, q_min, poids), foret FULL d'ordre K, entrees core et cover, ball_node ;
  - foret : ancetre vivant a une coupe (convention FERMEE par defaut, plateaux compris), composantes vivantes, LCA ;
  - hierarchies de points par attaches (date, noeud) fixees une fois : partitions emboitees a toute coupe,
    hauteurs de reunion exactes ;
  - resolveur exact K = 2 : composante du milieu d'une paire quelconque a son propre rayon (sans ball_node) ;
  - q_min exact (plus petit support positif du centre dans la coquille) et support canonique S* ;
  - univers des temoins de couverture a K fixe (lemme de couverture du catalogue : population >= K et, en mode
    renforce, p + q_min <= K ; a K = 1 les temoins sont les sites au niveau nul).

Conventions :
  - un NIVEAU (beta) est un rayon CARRE exact : Fraction ou int, jamais un flottant ;
  - coupe fermee : un noeud de niveau l est vivant a beta ssi l <= beta et (racine ou niveau du parent > beta) ;
    coupe ouverte : l < beta et (racine ou niveau du parent >= beta) ; tous les evenements d'un meme niveau exact
    (plateau) sont pris ensemble ;
  - un site est designe par son indice moteur (rang de Morton) ; `Export.point_id[s]` est l'indice du point dans le
    fichier d'entree ;
  - aucune verification ne repose sur `assert` (le module se comporte de meme sous python3 -O).

Bibliotheque standard seulement.
"""
from bisect import bisect_left, bisect_right
from fractions import Fraction
from itertools import combinations
import hashlib
import json
import math
import os
import subprocess

SCHEMA = 'mhgp10_frontier_export_v1'
NONE = -1
U32_NONE = 2 ** 32 - 1


class FrontierError(RuntimeError):
    """Incoherence d'un export, d'une requete ou d'un invariant (jamais un assert)."""


def require(condition, message):
    if not condition:
        raise FrontierError(message)


# ------------------------------------------------------------------ arithmetique exacte

def sq_dist(a, b):
    """Distance carree entiere entre deux points entiers."""
    dx, dy, dz = a[0] - b[0], a[1] - b[1], a[2] - b[2]
    return dx * dx + dy * dy + dz * dz


def as_fraction(v):
    if isinstance(v, Fraction):
        return v
    require(isinstance(v, int) and not isinstance(v, bool), 'niveau exact attendu (int ou Fraction), recu %r' % (v,))
    return Fraction(v)


def morton3(x, y, z):
    """Cle de Morton du moteur (bits de x aux positions 0, 3, 6, ... ; y en 1, 4, ... ; z en 2, 5, ...)."""
    key = 0
    for i in range(21):
        key |= ((x >> i) & 1) << (3 * i)
        key |= ((y >> i) & 1) << (3 * i + 1)
        key |= ((z >> i) & 1) << (3 * i + 2)
    return key


# ------------------------------------------------------------------ catalogue

class Ball:
    """Boule critique du catalogue : niveau exact (rayon carre), centre exact num/den, I et U tries."""
    __slots__ = ('index', 'lv', 'level', 'q', 'p', 'u', 'flags', 'support', 'num', 'den', 'I', 'U')

    def __init__(self, index, lv, level, q, p, u, flags, support, num, den, I, U):
        self.index, self.lv, self.level = index, lv, level
        self.q, self.p, self.u, self.flags = q, p, u, flags
        self.support, self.num, self.den = support, num, den
        self.I, self.U = I, U

    @property
    def centre(self):
        return tuple(Fraction(v, self.den) for v in self.num)

    @property
    def population(self):
        return len(self.I) + len(self.U)

    def members(self):
        return self.I + self.U


def ball_side(ball, point):
    """-1 interieur strict, 0 coquille, 1 exterieur (entiers exacts : |D z - N|^2 den_l compare a num_l D^2)."""
    D = ball.den
    t = sq_dist((D * point[0], D * point[1], D * point[2]), ball.num)
    lhs = t * ball.level.denominator
    rhs = ball.level.numerator * D * D
    return -1 if lhs < rhs else (0 if lhs == rhs else 1)


# ------------------------------------------------------------------ q_min et support canonique

def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _scaled(point, D):
    return (D * point[0], D * point[1], D * point[2])


def positive_support(points, num, den):
    """Vrai ssi le centre c = num/den est dans l'interieur relatif de conv(points), points affinement independants
    (1 a 4 points). Arithmetique entiere exacte ; c est suppose equidistant des points (coquille d'une sphere) pour
    q = 3 (c dans le plan <=> centre circonscrit)."""
    q = len(points)
    D = den
    if q == 1:
        return _scaled(points[0], D) == tuple(num)
    if q == 2:
        a, b = points
        return a != b and all(2 * num[i] == D * (a[i] + b[i]) for i in range(3))
    if q == 3:
        a, b, c = points
        n = _cross(_sub(b, a), _sub(c, a))
        if n == (0, 0, 0):
            return False  # alignes
        if _dot(n, num) != D * _dot(n, a):
            return False  # centre hors du plan
        # barycentriques proportionnelles a n.((s_i - p) x (s_j - p)), p = num / D (facteur D^2 > 0)
        P = tuple(num)
        A, B, C = _scaled(a, D), _scaled(b, D), _scaled(c, D)
        for s, t in ((B, C), (C, A), (A, B)):
            if _dot(n, _cross(_sub(s, P), _sub(t, P))) <= 0:
                return False
        return True
    if q == 4:
        a, b, c, d = points
        vol = _dot(_sub(b, a), _cross(_sub(c, a), _sub(d, a)))
        if vol == 0:
            return False  # coplanaires
        P = tuple(num)
        verts = (a, b, c, d)
        for f in range(4):
            u, v, w = verts[(f + 1) % 4], verts[(f + 2) % 4], verts[(f + 3) % 4]
            e1, e2 = _sub(v, u), _sub(w, u)
            normal = _cross(e1, e2)
            so = _dot(normal, _sub(verts[f], u))
            sc = _dot(normal, _sub(P, _scaled(u, D)))  # meme signe que pour c (D > 0)
            if sc == 0 or (sc > 0) != (so > 0):
                return False
        return True
    return False


def qmin_exact(sites, shell, num, den):
    """q_min exact : plus petit cardinal d'un sous-ensemble affinement independant de la coquille (indices de sites)
    dont l'interieur relatif contient le centre num/den ; None si aucun (sphere non critique)."""
    pts = [sites[s] for s in shell]
    for q in range(1, min(4, len(pts)) + 1):
        for S in combinations(range(len(pts)), q):
            if positive_support([pts[i] for i in S], num, den):
                return q
    return None


def canonical_support(sites, shell, num, den):
    """S* : plus petit sous-ensemble (ordre lexicographique des indices de sites tries) de cardinal q_min ; None si
    aucun support."""
    sh = sorted(shell)
    q = qmin_exact(sites, sh, num, den)
    if q is None:
        return None
    for S in combinations(sh, q):
        if positive_support([sites[s] for s in S], num, den):
            return S
    return None


def centre_of_support(sites, support):
    """Centre exact (num entiers, den > 0) de la sphere circonscrite a un support de 2 a 4 sites dans son affine ;
    None si degenere. Resolution de Gauss sur Fraction (independante du moteur)."""
    pts = [sites[s] for s in support]
    p0 = pts[0]
    if len(pts) == 1:
        return tuple(p0), 1
    Dv = [_sub(p, p0) for p in pts[1:]]
    m = len(Dv)
    M = [[Fraction(_dot(Dv[i], Dv[j])) for j in range(m)] + [Fraction(_dot(Dv[i], Dv[i]), 2)] for i in range(m)]
    for col in range(m):
        piv = next((r for r in range(col, m) if M[r][col] != 0), None)
        if piv is None:
            return None
        M[col], M[piv] = M[piv], M[col]
        for r in range(m):
            if r != col and M[r][col] != 0:
                f = M[r][col] / M[col][col]
                M[r] = [x - f * y for x, y in zip(M[r], M[col])]
    lam = [M[i][m] / M[i][i] for i in range(m)]
    c = [Fraction(p0[j]) + sum(lam[i] * Dv[i][j] for i in range(m)) for j in range(3)]
    den = 1
    for v in c:
        den = den * v.denominator // math.gcd(den, v.denominator)
    return tuple(int(v * den) for v in c), den


# ------------------------------------------------------------------ foret FULL d'un ordre

class Forest:
    """Foret FULL d'un ordre K : noeuds [lv, parent, naissance, enfants], niveaux exacts par table `levels`.

    Les niveaux des noeuds sont des indices lv dans la table triee strictement croissante des niveaux exacts : toute
    comparaison de niveaux est une comparaison d'entiers apres conversion exacte du seuil (bisect sur Fraction)."""

    def __init__(self, k, levels, lv, parent, birth, children):
        self.k = k
        self.levels = levels
        self.lv = lv
        self.parent = parent
        self.birth = birth
        self.children = children
        n = len(lv)
        require(len(parent) == n and len(birth) == n and len(children) == n, 'tailles de foret')
        roots = [v for v in range(n) if parent[v] == NONE]
        require(len(roots) == 1, 'foret a %d racines' % len(roots))
        self.root = roots[0]
        for v in range(n):
            require(0 <= lv[v] < len(levels), 'lv hors table au noeud %d' % v)
            p = parent[v]
            if p != NONE:
                require(0 <= p < n and lv[p] >= lv[v], 'parent invalide au noeud %d' % v)
                require(v in children[p], 'parent sans l\'enfant %d' % v)
            for c in children[v]:
                require(0 <= c < n and parent[c] == v, 'enfant invalide au noeud %d' % v)
            require((birth[v] == NONE) == (len(children[v]) >= 2), 'naissance/fusion incoherente au noeud %d' % v)
        # profondeurs (iteratif ; les racines ont 0)
        depth = [None] * n
        depth[self.root] = 0
        stack = [self.root]
        while stack:
            v = stack.pop()
            for c in children[v]:
                depth[c] = depth[v] + 1
                stack.append(c)
        require(all(d is not None for d in depth), 'noeud hors de l\'arbre')
        self.depth = depth
        self.births = [v for v in range(n) if birth[v] != NONE]
        self.birth_node = {birth[v]: v for v in self.births}
        require(len(self.birth_node) == len(self.births), 'deux noeuds pour une meme naissance')
        self._up = None

    def __len__(self):
        return len(self.lv)

    def level(self, v):
        return self.levels[self.lv[v]]

    def threshold(self, beta, closed=True):
        """Plus grand lv vivant a la coupe : niveau <= beta (fermee) ou < beta (ouverte) ; -1 si aucun."""
        beta = as_fraction(beta)
        return (bisect_right(self.levels, beta) if closed else bisect_left(self.levels, beta)) - 1

    def _lifting(self):
        if self._up is None:
            n = len(self.lv)
            up = [[p for p in self.parent]]
            span = max(self.depth) if n else 0
            j = 1
            while (1 << j) <= span:
                prev = up[-1]
                up.append([NONE if prev[v] == NONE else prev[prev[v]] for v in range(n)])
                j += 1
            self._up = up
        return self._up

    def ancestor_lv(self, v, t):
        """Ancetre vivant au seuil entier t (lv) ; None si v n'est pas encore ne."""
        if self.lv[v] > t:
            return None
        up = self._lifting()
        for j in range(len(up) - 1, -1, -1):
            u = up[j][v]
            if u != NONE and self.lv[u] <= t:
                v = u
        return v

    def ancestor(self, v, beta, closed=True):
        """Composante vivante a la coupe beta (rayon carre exact) qui contient le noeud v ; None si v est posterieur."""
        return self.ancestor_lv(v, self.threshold(beta, closed))

    def is_alive(self, v, beta, closed=True):
        t = self.threshold(beta, closed)
        p = self.parent[v]
        return self.lv[v] <= t and (p == NONE or self.lv[p] > t)

    def components(self, beta, closed=True):
        """Noeuds vivants a la coupe (composantes de L_K), tries."""
        t = self.threshold(beta, closed)
        return [v for v in range(len(self.lv))
                if self.lv[v] <= t and (self.parent[v] == NONE or self.lv[self.parent[v]] > t)]

    def lca(self, u, v):
        """Plus proche ancetre commun (la foret a une seule racine)."""
        up = self._lifting()
        if self.depth[u] < self.depth[v]:
            u, v = v, u
        diff = self.depth[u] - self.depth[v]
        j = 0
        while diff:
            if diff & 1:
                u = up[j][u]
            diff >>= 1
            j += 1
        if u == v:
            return u
        for j in range(len(up) - 1, -1, -1):
            if up[j][u] != up[j][v]:
                u, v = up[j][u], up[j][v]
        return self.parent[u]

    def is_ancestor(self, a, v):
        """Vrai si a est v ou un ancetre de v."""
        return self.depth[a] <= self.depth[v] and self.lca(a, v) == a

    def births_under(self, v):
        out, stack = [], [v]
        while stack:
            w = stack.pop()
            if self.birth[w] != NONE:
                out.append(w)
            stack.extend(self.children[w])
        return sorted(out)

    def signatures(self, birth_key):
        """Empreinte canonique de chaque sous-arbre : sha256(niveau exact, cle de naissance, empreintes des enfants
        triees). Independante des numerotations (rangs, indices de noeuds et de boules)."""
        order = sorted(range(len(self.lv)), key=lambda v: -self.depth[v])
        sig = [None] * len(self.lv)
        for v in order:
            lvl = self.level(v)
            key = birth_key(self.birth[v]) if self.birth[v] != NONE else None
            text = json.dumps([str(lvl), key, sorted(sig[c] for c in self.children[v])])
            sig[v] = hashlib.sha256(text.encode()).hexdigest()
        return sig


class Order:
    """Donnees d'un ordre K de l'export : foret, entrees core/cover, ball_node."""

    def __init__(self, k, forest, lower, core_level, core_node, cover_lv, cover_node, ball_node):
        self.k = k
        self.forest = forest
        self.lower = lower
        self.core_level = core_level
        self.core_node = core_node
        self.cover_lv = cover_lv
        self.cover_node = cover_node
        self.ball_node = ball_node

    def cover_level(self, s):
        return self.forest.levels[self.cover_lv[s]]


class Export:
    """Export natif charge : meta-donnees, niveaux exacts, sites, catalogue, ordres."""

    def __init__(self, meta, levels, sites, point_id, balls, orders, path=None, sha256=None):
        self.meta = meta
        self.K = meta['K']
        self.kmax_catalogue = meta['kmax_catalogue']
        self.levels = levels
        self.sites = sites
        self.point_id = point_id
        self.balls = balls
        self.orders = orders
        self.path = path
        self.sha256 = sha256
        self.site_of_point = {p: s for s, p in enumerate(point_id)}
        self.site_of_coord = {c: s for s, c in enumerate(sites)}

    def order(self, k=None):
        k = self.K if k is None else k
        require(k in self.orders, 'ordre %d absent de l\'export' % k)
        return self.orders[k]

    @property
    def forest(self):
        return self.order().forest

    def ball_key(self, b):
        """Identite geometrique d'une boule, independante des rangs et indices : (niveau, I, U) en coordonnees."""
        ball = self.balls[b]
        return (str(ball.level), [list(self.sites[s]) for s in ball.I], [list(self.sites[s]) for s in ball.U])

    def birth_key(self, k=None):
        o = self.order(k)
        if o.k == 1:
            return lambda s: list(self.sites[s])
        return self.ball_key


def _ints(v, what):
    require(isinstance(v, list) and all(isinstance(x, int) and not isinstance(x, bool) for x in v), what)
    return v


def load_export(path):
    """Charge et controle un export `export_frontier` (schema mhgp10_frontier_export_v1)."""
    with open(path, 'rb') as f:
        raw = f.read()
    digest = hashlib.sha256(raw).hexdigest()
    data = json.loads(raw.decode('ascii'))
    require(data.get('schema') == SCHEMA, 'schema inattendu : %r' % data.get('schema'))
    meta = {k: data[k] for k in ('schema', 'engine_commit', 'K', 'kmax_catalogue', 'orders_built',
                                 'coordinate_bits', 'n_points', 'n_sites')}
    levels = []
    for i, (a, b) in enumerate(data['levels']):
        levels.append(Fraction(int(a), int(b)))
    require(levels and levels[0] == 0, 'table des niveaux : lv 0 doit etre nul')
    require(all(levels[i] < levels[i + 1] for i in range(len(levels) - 1)), 'niveaux non strictement croissants')
    sites, point_id = [], []
    lim = (1 << meta['coordinate_bits']) - 1
    for row in data['sites']:
        _ints(row, 'site')
        require(len(row) == 4 and all(0 <= c <= lim for c in row[1:]), 'site invalide')
        point_id.append(row[0])
        sites.append((row[1], row[2], row[3]))
    ns = len(sites)
    require(ns == meta['n_sites'] and len(set(sites)) == ns and len(set(point_id)) == ns, 'sites incoherents')
    keys = [morton3(*p) for p in sites]
    require(all(keys[i] < keys[i + 1] for i in range(ns - 1)), 'sites hors de l\'ordre de Morton')
    balls = []
    for b, row in enumerate(data['balls']):
        lv, q = row['lv'], row['q']
        require(1 <= lv < len(levels) and 2 <= q <= 4, 'boule %d : lv/q' % b)
        S = tuple(_ints(row['S'], 'support'))
        I = tuple(_ints(row['I'], 'I'))
        U = tuple(_ints(row['U'], 'U'))
        require(len(S) == q and set(S) <= set(U), 'boule %d : support hors coquille' % b)
        require(list(I) == sorted(set(I)) and list(U) == sorted(set(U)) and not set(I) & set(U), 'boule %d : I/U' % b)
        require(all(0 <= s < ns for s in I + U), 'boule %d : site hors bornes' % b)
        require(row['p'] == len(I) and row['u'] == len(U), 'boule %d : poids (multiplicites non supportees)' % b)
        c = [int(t) for t in row['c']]
        require(len(c) == 4 and c[3] > 0, 'boule %d : centre' % b)
        balls.append(Ball(b, lv, levels[lv], q, row['p'], row['u'], row['flags'], S, tuple(c[:3]), c[3], I, U))
    require(all(balls[i].lv <= balls[i + 1].lv for i in range(len(balls) - 1)), 'catalogue hors ordre des niveaux')
    orders = {}
    for od in data['orders']:
        k = od['k']
        nodes = od['nodes']
        lv = [r[0] for r in nodes]
        parent = [r[1] for r in nodes]
        birth = [r[2] for r in nodes]
        children = [list(r[3]) for r in nodes]
        forest = Forest(k, levels, lv, parent, birth, children)
        for v in forest.births:
            if k == 1:
                require(0 <= birth[v] < ns and lv[v] == 0, 'naissance de site invalide')
            else:
                require(0 <= birth[v] < len(balls) and lv[v] == balls[birth[v]].lv, 'naissance de boule invalide')
        core_level = _ints(od['core_level'], 'core_level')
        core_node = _ints(od['core_node'], 'core_node')
        cover_lv = _ints(od['cover_lv'], 'cover_lv')
        cover_node = _ints(od['cover_node'], 'cover_node')
        require(len(core_level) == ns and len(core_node) == ns and len(cover_lv) == ns and len(cover_node) == ns,
                'tailles des entrees')
        require(all(0 <= v < len(lv) for v in core_node + cover_node), 'entree hors bornes')
        require(all(0 <= r < len(levels) for r in cover_lv), 'lv cover hors table')
        ball_node = od['ball_node']
        if k == 1:
            require(ball_node is None, 'ball_node attendu nul a K = 1')
        else:
            _ints(ball_node, 'ball_node')
            require(len(ball_node) == len(balls), 'taille de ball_node')
            for b, v in enumerate(ball_node):
                pop = balls[b].population
                require((v == NONE) == (pop < k), 'ball_node %d : population' % b)
                require(v == NONE or (0 <= v < len(lv) and lv[v] <= balls[b].lv), 'ball_node %d : date' % b)
        lower = od['lower']
        orders[k] = Order(k, forest, lower, core_level, core_node, cover_lv, cover_node, ball_node)
    require(meta['K'] in orders, 'ordre K absent')
    return Export(meta, levels, sites, point_id, balls, orders, path=path, sha256=digest)


# ------------------------------------------------------------------ hierarchies de points par attaches

class Attachments:
    """Attaches fixees une fois : le point x entre a la date d(x) (rayon carre exact) dans la composante a(x), puis
    suit ses seuls ancetres. Avant d(x) (ou sans attache), x est un singleton de completion. Les partitions sont donc
    emboitees par construction ; la hauteur de reunion de x et y est max(d(x), d(y), niveau(LCA)).

    Le noeud donne est normalise a son ancetre vivant a d(x) (cible du contrat d'ancrage) ; il doit etre ne a d(x)."""

    def __init__(self, forest, dates, nodes):
        require(len(dates) == len(nodes), 'attaches : tailles')
        self.forest = forest
        self.dates = []
        self.nodes = []
        for x, (d, v) in enumerate(zip(dates, nodes)):
            if d is None or v is None or v == NONE:
                self.dates.append(None)
                self.nodes.append(None)
                continue
            d = as_fraction(d)
            a = forest.ancestor(v, d, True)
            require(a is not None, 'attache du point %d posterieure a sa date' % x)
            self.dates.append(d)
            self.nodes.append(a)

    def label(self, x, beta, closed=True, t=None):
        d = self.dates[x]
        if d is None:
            return ('s', x)
        beta = as_fraction(beta)
        if d > beta or (d == beta and not closed):
            return ('s', x)
        if t is None:
            t = self.forest.threshold(beta, closed)
        return ('c', self.forest.ancestor_lv(self.nodes[x], t))

    def labels(self, beta, closed=True):
        beta = as_fraction(beta)
        t = self.forest.threshold(beta, closed)
        return [self.label(x, beta, closed, t) for x in range(len(self.dates))]

    def blocks(self, beta, closed=True):
        """Partition de tous les points a la coupe : blocs tries (singletons compris), tries par premier element."""
        groups = {}
        for x, lab in enumerate(self.labels(beta, closed)):
            groups.setdefault(lab, []).append(x)
        return sorted(groups.values())

    def merge_height(self, x, y):
        """Plus petit beta (coupe fermee) ou x et y sont dans un meme bloc ; None si l'un n'est jamais attache."""
        if x == y:
            return Fraction(0)
        if self.dates[x] is None or self.dates[y] is None:
            return None
        w = self.forest.lca(self.nodes[x], self.nodes[y])
        return max(self.dates[x], self.dates[y], self.forest.level(w))

    def entered(self, beta, closed=True):
        beta = as_fraction(beta)
        return sum(1 for d in self.dates if d is not None and (d < beta or (closed and d == beta)))


def core_attachments(export, k=None):
    o = export.order(k)
    return Attachments(o.forest, [Fraction(v) for v in o.core_level], o.core_node)


def cover_attachments(export, k=None):
    o = export.order(k)
    return Attachments(o.forest, [o.cover_level(s) for s in range(len(export.sites))], o.cover_node)


def nested(before, after):
    """Deux partitions (listes de blocs) : chaque bloc de `before` est inclus dans un bloc de `after`."""
    where = {}
    for i, blk in enumerate(after):
        for x in blk:
            where[x] = i
    return all(len({where[x] for x in blk}) == 1 for blk in before)


# ------------------------------------------------------------------ index spatial exact des sites

class SiteGrid:
    """Grille entiere uniforme sur les sites : requetes exactes de boules fermees (elagage par boites entieres,
    decision par comparaison entiere exacte ; aucun flottant)."""

    def __init__(self, sites, per_cell=2):
        self.sites = sites
        n = len(sites)
        lo = [min(p[i] for p in sites) for i in range(3)]
        hi = [max(p[i] for p in sites) for i in range(3)]
        vol = 1
        for i in range(3):
            vol *= hi[i] - lo[i] + 1
        cells = max(1, n // per_cell)
        h = 1
        while h ** 3 * cells < vol:
            h *= 2
        self.h = h
        self.lo = lo
        self.hi = hi
        self.cells = {}
        for s, p in enumerate(sites):
            self.cells.setdefault(self._cell(p), []).append(s)

    def _cell(self, p):
        return tuple((p[i] - self.lo[i]) // self.h for i in range(3))

    def candidates(self, box_lo, box_hi):
        """Sites dont la case coupe la boite entiere [box_lo, box_hi] (sur-ensemble exact)."""
        blo = [max(box_lo[i], self.lo[i]) for i in range(3)]
        bhi = [min(box_hi[i], self.hi[i]) for i in range(3)]
        if any(blo[i] > bhi[i] for i in range(3)):
            return []
        clo = [(blo[i] - self.lo[i]) // self.h for i in range(3)]
        chi = [(bhi[i] - self.lo[i]) // self.h for i in range(3)]
        span = (chi[0] - clo[0] + 1) * (chi[1] - clo[1] + 1) * (chi[2] - clo[2] + 1)
        if span > len(self.cells):
            return [s for s, p in enumerate(self.sites)
                    if all(blo[i] <= p[i] <= bhi[i] for i in range(3))]
        out = []
        for cx in range(clo[0], chi[0] + 1):
            for cy in range(clo[1], chi[1] + 1):
                for cz in range(clo[2], chi[2] + 1):
                    out.extend(self.cells.get((cx, cy, cz), ()))
        return out

    def ball_candidates(self, num, den, level):
        """Sur-ensemble des sites de la boule fermee (centre num/den, rayon carre `level`)."""
        level = as_fraction(level)
        r = math.isqrt(level.numerator // level.denominator) + 1  # r >= rayon reel
        lo = [num[i] // den - r - 1 for i in range(3)]
        hi = [-((-num[i]) // den) + r + 1 for i in range(3)]
        return self.candidates(lo, hi)

    def closed_ball(self, num, den, level):
        """(I, U) exacts de la boule fermee : interieur strict et coquille, tries par indice."""
        level = as_fraction(level)
        I, U = [], []
        ln, ld = level.numerator, level.denominator
        for s in self.ball_candidates(num, den, level):
            t = sq_dist(_scaled(self.sites[s], den), num) * ld
            rhs = ln * den * den
            if t < rhs:
                I.append(s)
            elif t == rhs:
                U.append(s)
        return sorted(I), sorted(U)

    def within(self, point, r2):
        """Sites a distance carree <= r2 (int ou Fraction) d'un point entier, tries par indice (exact)."""
        r2 = as_fraction(r2)
        r = math.isqrt(r2.numerator // r2.denominator) + 1
        lo = [point[i] - r for i in range(3)]
        hi = [point[i] + r for i in range(3)]
        return sorted(s for s in self.candidates(lo, hi)
                      if sq_dist(point, self.sites[s]) * r2.denominator <= r2.numerator)

    def k_nearest(self, point, k):
        """k plus proches sites d'un point entier (departage par indice) : liste de (distance carree, site)."""
        n = len(self.sites)
        require(1 <= k <= n, 'k hors bornes')
        r = self.h
        while True:
            lo = [point[i] - r for i in range(3)]
            hi = [point[i] + r for i in range(3)]
            cand = sorted((sq_dist(point, self.sites[s]), s) for s in self.candidates(lo, hi))
            # la boite de demi-cote r contient la boule de rayon r : les k premiers sont surs si leur distance <= r^2
            if len(cand) >= k and cand[k - 1][0] <= r * r:
                return cand[:k]
            if len(cand) >= n:
                return cand[:k]
            r *= 2


# ------------------------------------------------------------------ resolveur exact des paires a K = 2

class PairResolverK2:
    """Composante de L_2 qui contient le milieu m d'une paire {a, b} a son propre niveau |a - b|^2 / 4.

    Descente exacte, independante de ball_node : si la boule fermee de diametre [a, b] contient un troisieme site z,
    le milieu de {a, z} et m sont dans la lentille convexe B(a, r) n B(z, r) (r = |a - b| / 2), donc dans la meme
    composante au niveau r^2 ; |a - z| < |a - b| : le niveau decroit strictement. Sinon la boule est vide hors de a, b
    (p = 0, U = {a, b}) : c'est une NAISSANCE d'ordre 2, lue dans la foret par sa boule du catalogue. Le resultat est
    l'ancetre, au niveau de la paire, du noeud de naissance atteint. Toutes les decisions sont entieres
    (|2 z - a - b|^2 compare a |a - b|^2)."""

    def __init__(self, export, grid=None):
        o = export.order(2)
        self.export = export
        self.forest = o.forest
        self.sites = export.sites
        self.grid = grid if grid is not None else SiteGrid(export.sites)
        self.ball_by_key = {}
        for ball in export.balls:
            key = (tuple(Fraction(v, ball.den) for v in ball.num), ball.level)
            self.ball_by_key[key] = ball.index
        self.memo = {}
        self.steps = 0
        self.births_reached = 0
        self.census_calls = 0

    def pair_level(self, a, b):
        return Fraction(sq_dist(self.sites[a], self.sites[b]), 4)

    def third_sites(self, a, b):
        """Sites de la boule fermee de diametre [a, b], autres que a et b."""
        A, B = self.sites[a], self.sites[b]
        S = (A[0] + B[0], A[1] + B[1], A[2] + B[2])  # 2 m
        d2 = sq_dist(A, B)
        r = math.isqrt(d2) // 2 + 1
        lo = [S[i] // 2 - r - 1 for i in range(3)]
        hi = [(S[i] + 1) // 2 + r + 1 for i in range(3)]
        self.census_calls += 1
        out = []
        for z in self.grid.candidates(lo, hi):
            if z == a or z == b:
                continue
            Z = self.sites[z]
            if sq_dist((2 * Z[0], 2 * Z[1], 2 * Z[2]), S) <= d2:
                out.append(z)
        return out

    def birth_of_empty_pair(self, a, b):
        A, B = self.sites[a], self.sites[b]
        centre = tuple(Fraction(A[i] + B[i], 2) for i in range(3))
        key = (centre, self.pair_level(a, b))
        bi = self.ball_by_key.get(key)
        require(bi is not None, 'paire vide {%d,%d} absente du catalogue' % (a, b))
        ball = self.export.balls[bi]
        require(ball.p == 0 and ball.U == tuple(sorted((a, b))), 'boule de la paire {%d,%d} non vide' % (a, b))
        v = self.forest.birth_node.get(bi)
        require(v is not None, 'paire vide {%d,%d} sans naissance d\'ordre 2' % (a, b))
        return v

    def resolve(self, a, b, keep=0):
        """Noeud vivant au niveau |a - b|^2 / 4 qui contient le milieu de {a, b}. `keep` choisit l'extremite gardee
        pendant la descente (0 : a, 1 : b) ; les deux chemins doivent donner le meme noeud (auto-controle)."""
        require(a != b, 'paire degeneree')
        key = (min(a, b), max(a, b))
        if keep == 0 and key in self.memo:
            return self.memo[key]
        path = []
        cur = (a, b) if keep == 0 else (b, a)
        while True:
            ck = (min(cur), max(cur))
            if keep == 0 and ck in self.memo:
                node = self.memo[ck]
                break
            third = self.third_sites(*cur)
            if not third:
                node = self.birth_of_empty_pair(*cur)
                self.births_reached += 1
                if keep == 0:
                    self.memo[ck] = node
                break
            x = cur[0]
            X = self.sites[x]
            z = min(third, key=lambda s: (sq_dist(X, self.sites[s]), s))
            path.append(cur)
            self.steps += 1
            cur = (x, z)
        for p in reversed(path):
            node = self.forest.ancestor(node, self.pair_level(*p), True)
            require(node is not None, 'descente : noeud posterieur au niveau de la paire')
            if keep == 0:
                self.memo[(min(p), max(p))] = node
        return node

    def component(self, a, b, beta, closed=True):
        """Composante, a la coupe beta, du milieu de {a, b} ; None si beta precede le niveau de la paire."""
        lvl = self.pair_level(a, b)
        beta = as_fraction(beta)
        if lvl > beta or (lvl == beta and not closed):
            return None
        return self.forest.ancestor(self.resolve(a, b), beta, closed)


# ------------------------------------------------------------------ temoins de couverture a K fixe

def witness_universe(export, k=None, strong=True):
    """Par site : liste de (boule, niveau exact, noeud) des temoins de couverture a l'ordre k.

    K >= 2 : boules de population >= k (et p + q_min <= k en mode renforce), avec toutes leurs incidences I u U ;
    le noeud est la composante du centre a son propre niveau (ball_node, controle vivant a ce niveau).
    K = 1 : le site lui-meme au niveau nul (boule -1), dans sa composante de naissance."""
    o = export.order(k)
    f = o.forest
    W = [[] for _ in export.sites]
    if o.k == 1:
        for s in range(len(export.sites)):
            v = f.birth_node.get(s)
            require(v is not None, 'site %d sans naissance a K = 1' % s)
            W[s].append((NONE, Fraction(0), v))
        return W
    for ball in export.balls:
        if ball.population < o.k:
            continue
        if strong and ball.p + ball.q > o.k:
            continue
        v = o.ball_node[ball.index]
        require(v != NONE, 'boule couvrante %d sans ball_node' % ball.index)
        require(f.ancestor(v, ball.level, True) == v, 'ball_node %d non vivant a son niveau' % ball.index)
        for s in ball.members():
            W[s].append((ball.index, ball.level, v))
    return W


def first_covering_ball(export, k=None):
    """Par site : premiere boule (ordre canonique du catalogue, donc niveau croissant puis S*) de population >= k qui
    le contient ; c'est la boule dont la composante definit l'entree cover native (niveau alpha_k(x)^2). None a
    K = 1 (entree au niveau nul par le site lui-meme)."""
    o = export.order(k)
    first = [None] * len(export.sites)
    if o.k == 1:
        return first
    missing = len(first)
    for ball in export.balls:
        if ball.population < o.k:
            continue
        for s in ball.members():
            if first[s] is None:
                first[s] = ball.index
                missing -= 1
        if missing == 0:
            break
    require(missing == 0, 'site sans boule couvrante')
    return first


def covering_components(forest, witnesses, beta, closed=True):
    """Composantes vivantes a la coupe qui couvrent le point (dedupliquees par ancetre), triees."""
    t = forest.threshold(beta, closed)
    beta = as_fraction(beta)
    out = set()
    for _b, lvl, v in witnesses:
        if lvl < beta or (closed and lvl == beta):
            out.add(forest.ancestor_lv(v, t))
    return sorted(out)


# ------------------------------------------------------------------ outils d'execution

def write_cloud(points, path):
    """Ecrit un nuage u32le (x y z par point) ; coordonnees entieres dans [0, 2^18)."""
    with open(path, 'wb') as f:
        for p in points:
            require(len(p) == 3 and all(isinstance(c, int) and 0 <= c < (1 << 18) for c in p), 'point hors domaine')
            for c in p:
                f.write(int(c).to_bytes(4, 'little'))


def run_exporter(exe, cloud_path, out_path, k, threads=1, kmax_catalogue=None, all_orders=False, timeout=600):
    """Lance export_frontier ; rend (code, resume JSON ou None, stdout, stderr)."""
    argv = [exe, cloud_path, '--k=%d' % k, '--out=%s' % out_path, '--threads=%d' % threads]
    if kmax_catalogue is not None:
        argv.append('--kmax-catalogue=%d' % kmax_catalogue)
    if all_orders:
        argv.append('--all-orders')
    r = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout)
    summary = None
    try:
        summary = json.loads(r.stdout.decode().strip().splitlines()[-1]) if r.stdout.strip() else None
    except (ValueError, IndexError):
        summary = None
    return r.returncode, summary, r.stdout.decode(), r.stderr.decode(), argv


def file_sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


def default_exporter():
    """Chemin par defaut de l'exporteur construit dans build/v10-frontiere/frontier-build."""
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.normpath(os.path.join(here, '..', '..', '..', 'frontier-build', 'export_frontier'))
