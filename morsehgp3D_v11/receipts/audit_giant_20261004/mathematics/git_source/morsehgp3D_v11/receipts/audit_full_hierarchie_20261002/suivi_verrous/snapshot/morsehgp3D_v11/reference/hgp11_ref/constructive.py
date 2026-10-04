"""Etage B de la reference : la voie du moteur, en arithmetique exacte et par force brute.

  1. Sites : positions distinctes dans l'ordre de Morton, poids = multiplicite. Les points portent un identifiant
     interne, rang dans l'ordre (cle de Morton, indice d'entree) ; sans doublon c'est l'indice de site.
  2. Catalogue critique : tous les supports de 1 a 4 sites affinement independants dont le centre est dans
     l'interieur relatif de l'enveloppe convexe ; recensement exact de l'interieur strict I et de la coquille U ;
     une sphere est identifiee par (centre, rayon carre) ; q_min et le support canonique S* sont ceux du premier
     support qui l'engendre (cardinal croissant, puis ordre lexicographique des rangs de Morton).
     Admission pour les ordres <= K, regle 'v10' (defaut, celle du binaire fige) : coquille sans site pondere,
     p + q_min <= K + 1 ; coquille ponderee, p <= K - 1. Regle 'single' : p + q_min <= K + 1 dans les deux cas.
     Les boules que la regle 'v10' admet en plus n'ont aucune cellule d'ordre <= K (leur fenetre commence a
     p + q_min - 1 > K) : les deux regles donnent la meme tour, et le meme catalogue sur un nuage sans doublon.
     Les boules de rayon nul (un site) sont dans ce catalogue interne ; celui du moteur commence a q_min = 2.
  3. Cellule (boule, k), k dans la fenetre [p + q_min - 1, p + m] (p, m : points de I et de U, multiplicites
     comprises), t = k - p. Une partie A de U de taille t est SEPARABLE si le centre n'est pas dans son enveloppe
     convexe fermee (theoreme de Gordan). Aucune partie separable : NAISSANCE. Sinon les MORCEAUX sont les
     composantes du graphe A ~ A' ssi A u A' est separable : au moins deux morceaux, JONCTION (un representant
     I u A par morceau) ; un seul, cellule inerte. Coquille reguliere (U est le support, sans doublon) : raccourci
     analytique du moteur, naissance a t = m, jonction a m representants a t = m - 1, inerte en dessous.
  4. Descente d'une k-partie F vers une naissance : boule minimale exacte de F ; au moins k points strictement
     interieurs, saut aux k plus proches du centre ; sinon la cellule de naissance de cette boule, ou le premier
     representant de sa structure locale. Le niveau decroit strictement a chaque pas.
  5. Foret d'un ordre : Kruskal par plateaux exacts (racines lues avant les unions du plateau, une fusion N-aire par
     groupe d'au moins deux racines), une seule racine.
  6. Verticales, entrees core, relation de couverture (une resolution par boule de population >= k), coupes.

Sous-fenetre : pour t <= q_min - 2 toute partie de taille t + 1 a moins de q_min positions, donc est separable
(sinon Caratheodory donnerait un support plus petit que q_min) ; deux parties de taille t qui different d'un point
sont donc voisines, et il n'y a qu'un morceau. C'est pourquoi la fenetre commence a p + q_min - 1, doublons compris.
"""
from fractions import Fraction
from itertools import combinations

from . import intgeom as G
from .model import Cut, Entry, InvariantError, Node, OrderResult


class Ball(object):
    """Sphere critique recensee. inner et shell : les points de l'interieur strict I et de la coquille U
    (identifiants internes croissants) ; inner_sites et shell_sites : leurs sites ; p et m : leurs nombres de points
    (poids)."""
    __slots__ = ('level', 'center', 'anchor', 'ctr', 'qmin', 'support', 'inner_sites', 'shell_sites', 'inner', 'shell',
                 'p', 'm', 'extended', 'weighted', 'regular', 'index', 'lo', 'hi', 'cells', 'site_masks', 'witnesses')


class Reference(object):
    """Reference constructive d'un nuage pour les ordres 1 .. min(kmax, n). order(k) rend l'OrderResult."""

    def __init__(self, points, kmax, admission='v10'):
        if admission not in ('v10', 'single'):
            raise ValueError('regle d\'admission %r inconnue' % (admission,))
        self.admission = admission
        self.input = [tuple(int(c) for c in p) for p in points]
        if not self.input or any(len(p) != 3 for p in points):
            raise ValueError('nuage vide ou point mal forme')
        if any(not 0 <= c < (1 << G.MORTON_BITS) for p in self.input for c in p):
            raise ValueError('coordonnee hors de [0, 2^%d)' % G.MORTON_BITS)
        if kmax < 1:
            raise ValueError('kmax < 1')
        self.kmax = kmax
        self.n = len(self.input)
        self.orders = min(kmax, self.n)
        self.inp = sorted(range(self.n), key=lambda i: (G.morton(self.input[i]), i))  # interne -> entree
        self.internal = [0] * self.n
        self.sites, self.weights, self.site_of, self.site_points = [], [], [], []
        for pid, i in enumerate(self.inp):
            self.internal[i] = pid
            if not self.sites or self.sites[-1] != self.input[i]:
                self.sites.append(self.input[i])
                self.weights.append(0)
                self.site_points.append([])
            self.weights[-1] += 1
            self.site_of.append(len(self.sites) - 1)
            self.site_points[-1].append(pid)
        self.stats = dict(descents=0, steps=0, jumps=0, adhoc=0, births=0, joins=0, inert=0, ext_births=0,
                          ext_joins=0, ext_inert=0)
        self._spheres, self._adhoc, self._memo, self._results, self._trees = {}, {}, {}, {}, {}
        self._birth_node, self._birth_ball = {}, {}  # par ordre : boule -> noeud de naissance, et l'inverse
        # par ordre et par point d'entree : (rang, noeud) de la premiere boule couvrante dans l'ordre du catalogue
        # (regle de depart de la v10 aux egalites de niveau ; la verite est l'ensemble Entry.nodes de cover)
        self.cover_choice = {}
        self._build_catalogue()

    # ------------------------------------------------------------ spheres et catalogue

    def _sphere(self, support):
        """Sphere circonscrite d'un support de sites : (ancre, (N, D), cle), ou None si affinement dependant."""
        if support not in self._spheres:
            pts = [self.sites[s] for s in support]
            ctr = G.through(pts)
            self._spheres[support] = None if ctr is None else (pts[0], ctr, G.sphere_key(pts[0], ctr))
        return self._spheres[support]

    def _census(self, anchor, ctr, limit=None):
        """Sites strictement interieurs et sites de la coquille ; None des que le poids interieur depasse limit."""
        inner, shell, p = [], [], 0
        for s, z in enumerate(self.sites):
            key = G.side_key(anchor, ctr, z)
            if key < 0:
                inner.append(s)
                p += self.weights[s]
                if limit is not None and p > limit:
                    return None
            elif key == 0:
                shell.append(s)
        return inner, shell

    def _ball(self, anchor, ctr, key, inner, shell):
        b = Ball()
        b.center, b.level = key
        b.anchor, b.ctr = anchor, ctr
        b.inner_sites, b.shell_sites = tuple(inner), tuple(shell)
        b.inner = tuple(x for s in inner for x in self.site_points[s])
        b.shell = tuple(x for s in shell for x in self.site_points[s])
        b.p, b.m = len(b.inner), len(b.shell)
        if b.level == 0:
            b.qmin, b.support = 1, tuple(shell)
        else:
            found = G.canonical_support([self.sites[s] for s in shell], anchor, ctr)
            if found is None:
                raise InvariantError('sphere sans support : centre hors de l\'interieur relatif de sa coquille')
            b.qmin, b.support = found[0], tuple(shell[i] for i in found[1])
        b.extended = len(shell) > b.qmin
        b.weighted = any(self.weights[s] > 1 for s in shell)
        b.regular = b.m == b.qmin
        b.index, b.lo, b.hi = None, 1, 0
        b.cells, b.site_masks, b.witnesses = {}, None, None
        return b

    def _build_catalogue(self):
        found = {}
        ns = len(self.sites)
        for q in range(1, min(4, ns) + 1):
            for support in combinations(range(ns), q):
                sph = self._sphere(support)
                if sph is None or sph[2] in found:
                    continue
                anchor, ctr, key = sph
                pts = [self.sites[s] for s in support]
                if q == 3 and not G.acute(pts[0], pts[1], pts[2]):
                    continue
                if q == 4 and G.tetra_position(pts, anchor, ctr) != 1:
                    continue
                census = self._census(anchor, ctr, self.kmax - 1)
                if census is None:  # p >= K : admise par aucune des deux regles
                    found[key] = None
                    continue
                ball = self._ball(anchor, ctr, key, census[0], census[1])
                if ball.qmin != q or ball.support != support:
                    raise InvariantError('support canonique %r, premier support engendrant %r'
                                         % (ball.support, support))
                if ball.weighted and self.admission == 'v10':
                    admitted = ball.p <= self.kmax - 1
                else:
                    admitted = ball.p + ball.qmin <= self.kmax + 1
                found[key] = ball if admitted else None
        none = ns  # bourrage des supports : plus grand que tout rang de site (kNone du moteur)
        self.balls = sorted((b for b in found.values() if b is not None),
                            key=lambda b: (b.level, b.support + (none,) * (4 - b.qmin)))
        self._by_key = {}
        for i, b in enumerate(self.balls):
            b.index = i
            b.lo, b.hi = max(b.p + b.qmin - 1, 1), min(b.p + b.m, self.orders)
            self._by_key[(b.center, b.level)] = b

    # ------------------------------------------------------------ structure locale

    def _separable(self, ball):
        """Test de separabilite d'une partie de la coquille, donnee par le masque de ses sites."""
        if ball.witnesses is None:
            rank = dict((s, i) for i, s in enumerate(ball.shell_sites))
            ball.site_masks = [1 << rank[self.site_of[x]] for x in ball.shell]
            if ball.level == 0:
                ball.witnesses = [1]  # rayon nul : le site est le centre
            else:
                ball.witnesses = G.hull_witnesses([self.sites[s] for s in ball.shell_sites], ball.anchor, ball.ctr)
        wit = ball.witnesses
        return lambda mask: not any((w & ~mask) == 0 for w in wit)

    def cell(self, ball, k):
        """Cellule (boule, k) : ('birth', ()), ('join', representants) ou ('inert', (representant,)). Un
        representant est la partie A de U (points) ; la k-partie est I u A."""
        if k not in ball.cells:
            ball.cells[k] = self._local(ball, k - ball.p)
            kind = ball.cells[k][0]
            self.stats[{'birth': 'births', 'join': 'joins', 'inert': 'inert'}[kind]] += 1
            if not ball.regular:
                self.stats[{'birth': 'ext_births', 'join': 'ext_joins', 'inert': 'ext_inert'}[kind]] += 1
        return ball.cells[k]

    def _local(self, ball, t):
        shell, m = ball.shell, ball.m
        if not 1 <= t <= m:
            raise InvariantError('cellule hors fenetre : t = %d, coquille de %d points' % (t, m))
        if ball.regular:
            if t == m:
                return 'birth', ()
            if t == m - 1:
                return 'join', tuple(shell[:j] + shell[j + 1:] for j in range(m))
            return 'inert', (shell[:t],)
        separable = self._separable(ball)
        sep = []
        for idx in combinations(range(m), t):
            mask = 0
            for i in idx:
                mask |= ball.site_masks[i]
            if separable(mask):
                sep.append((idx, mask))
        if not sep:
            return 'birth', ()
        root = list(range(len(sep)))

        def find(x):
            while root[x] != x:
                root[x] = root[root[x]]
                x = root[x]
            return x
        for i in range(len(sep)):
            for j in range(i + 1, len(sep)):
                ri, rj = find(i), find(j)
                if ri != rj and separable(sep[i][1] | sep[j][1]):
                    root[max(ri, rj)] = min(ri, rj)
        reps = tuple(tuple(shell[i] for i in sep[r][0]) for r in range(len(sep)) if find(r) == r)
        return ('join' if len(reps) >= 2 else 'inert'), reps

    def _first_rep(self, ball, t):
        """Premier representant de la structure locale (seul utile a la descente), regle du moteur."""
        shell, m = ball.shell, ball.m
        if ball.regular:
            if t == m:
                raise InvariantError('naissance absente du catalogue ou hors de sa fenetre')
            return shell[1:] if t + 1 == m else shell[:t]
        separable = self._separable(ball)
        for idx in combinations(range(m), t):
            mask = 0
            for i in idx:
                mask |= ball.site_masks[i]
            if separable(mask):
                return tuple(shell[i] for i in idx)
        raise InvariantError('naissance absente du catalogue ou hors de sa fenetre')

    # ------------------------------------------------------------ descente

    def _meb(self, part):
        """Boule minimale exacte d'une partie de points : la plus petite sphere circonscrite d'un support de 1 a 4
        de ses sites qui la contient. Rend (ancre, (N, D), cle)."""
        sites = sorted(set(self.site_of[x] for x in part))
        best = None
        for q in range(1, min(4, len(sites)) + 1):
            for support in combinations(sites, q):
                sph = self._sphere(support)
                if sph is None or (best is not None and sph[2][1] >= best[2][1]):
                    continue
                if all(G.side_key(sph[0], sph[1], self.sites[s]) <= 0 for s in sites):
                    best = sph
        if best is None:
            raise InvariantError('aucune sphere candidate ne contient la partie %r' % (part,))
        return best

    def descend(self, part, k):
        """k-partie (identifiants internes tries) -> rang de la boule dont la cellule (boule, k) est la naissance de
        la composante de la partie a son propre niveau. Fonction pure de (part, k) ; le memo n'est qu'un cache."""
        self.stats['descents'] += 1
        chain, last = [], None
        while True:
            hit = self._memo.get((k, part))
            if hit is not None:
                break
            chain.append(part)
            self.stats['steps'] += 1
            anchor, ctr, key = self._meb(part)
            if last is not None and not key[1] < last:
                raise InvariantError('descente sans decroissance stricte du niveau (ordre %d)' % k)
            last = key[1]
            ball = self._by_key.get(key)
            if ball is None:
                ball = self._adhoc.get(key)
                if ball is None:
                    inner, shell = self._census(anchor, ctr)
                    ball = self._adhoc[key] = self._ball(anchor, ctr, key, inner, shell)
                    self.stats['adhoc'] += 1
            if ball.p >= k:  # saut : les k points les plus proches du centre (cle exacte, puis identifiant)
                self.stats['jumps'] += 1
                near = sorted((G.side_key(anchor, ctr, self.sites[self.site_of[x]]), x) for x in ball.inner)
                part = tuple(sorted(x for _key, x in near[:k]))
                continue
            if ball.index is not None and ball.lo <= k <= ball.hi and self.cell(ball, k)[0] == 'birth':
                hit = ball.index
                break
            part = tuple(sorted(ball.inner + self._first_rep(ball, k - ball.p)))
        for seen in chain:
            self._memo[(k, seen)] = hit
        return hit

    # ------------------------------------------------------------ un ordre

    def order(self, k):
        if not 1 <= k <= self.orders:
            raise ValueError('ordre %d hors de [1, %d]' % (k, self.orders))
        if k not in self._results:
            self._results[k] = self._build(k)
        return self._results[k]

    def node_at(self, k, part, level):
        """Noeud de l'ordre k vivant a la coupe fermee dont la composante contient la k-partie (indices d'entree)."""
        self.order(k)
        start = self._birth_node[k][self.descend(tuple(sorted(self.internal[i] for i in part)), k)]
        return self._trees[k].ancestor(start, level)

    def _forest(self, k):
        """Naissances et jonctions de l'ordre k dans l'ordre du catalogue, puis Kruskal par plateaux."""
        births, joins = [], []
        for ball in self.balls:
            if ball.lo <= k <= ball.hi:
                kind, reps = self.cell(ball, k)
                if kind == 'birth':
                    births.append(ball)
                elif kind == 'join':
                    joins.append((ball, [tuple(sorted(ball.inner + rep)) for rep in reps]))
        slot = dict((b.index, i) for i, b in enumerate(births))
        nb = len(births)
        dsu, top = list(range(nb)), list(range(nb))

        def find(x):
            while dsu[x] != x:
                dsu[x] = dsu[dsu[x]]
                x = dsu[x]
            return x
        merges = []
        i = 0
        while i < len(joins):
            level = joins[i][0].level
            j = i
            while j < len(joins) and joins[j][0].level == level:
                j += 1
            pre = [[find(slot[self.descend(part, k)]) for part in parts] for _ball, parts in joins[i:j]]
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
                if len(groups[root]) >= 2:
                    kids = sorted(top[r] for r in groups[root])
                    top[root] = nb + len(merges)
                    merges.append((level, kids))
            i = j
        if len(set(find(x) for x in range(nb))) != 1:
            raise InvariantError('ordre %d : %d racines' % (k, len(set(find(x) for x in range(nb)))))
        # numerotation canonique : naissances par (niveau, centre), fusions par (niveau, plus petite naissance)
        remap = [None] * (nb + len(merges))
        for new, old in enumerate(sorted(range(nb), key=lambda i: (births[i].level, births[i].center))):
            remap[old] = new
        leaf = remap[:nb] + [None] * len(merges)
        for j, (_level, kids) in enumerate(merges):  # les enfants d'une fusion sont crees avant elle
            leaf[nb + j] = min(leaf[c] for c in kids)
        for new, j in enumerate(sorted(range(len(merges)), key=lambda j: (merges[j][0], leaf[nb + j]))):
            remap[nb + j] = nb + new
        nodes = [None] * len(remap)
        for i, ball in enumerate(births):
            nodes[remap[i]] = Node(ball.level, (), ball.center)
        for j, (level, kids) in enumerate(merges):
            nodes[remap[nb + j]] = Node(level, tuple(sorted(remap[c] for c in kids)), None)
        self._birth_node[k] = dict((b.index, remap[i]) for i, b in enumerate(births))
        self._birth_ball[k] = dict((remap[i], b) for i, b in enumerate(births))
        return _Tree(nodes)

    def _build(self, k):
        tree = self._trees[k] = self._forest(k)
        nodes = tree.nodes
        birth_node = self._birth_node[k]
        lower = None
        if k > 1:  # verticales : une (k - 1)-partie de la boule fermee de naissance ; fusions par naturalite
            self.order(k - 1)
            prev = self._trees[k - 1]
            below = self._birth_node[k - 1]
            lower = [None] * len(nodes)
            for v, node in enumerate(nodes):
                if node.center is not None:
                    ball = self._birth_ball[k][v]
                    part = tuple(sorted((ball.inner + ball.shell)[:k - 1]))
                    lower[v] = prev.ancestor(below[self.descend(part, k - 1)], node.level)
                else:
                    images = set(prev.ancestor(lower[c], node.level) for c in node.children)
                    if len(images) != 1:
                        raise InvariantError('ordre %d noeud %d : images verticales des enfants distinctes' % (k, v))
                    lower[v] = images.pop()
        core = [None] * self.n
        entries = []  # (niveau, noeud, point d'entree)
        for x in range(self.n):
            px = self.sites[self.site_of[x]]
            near = sorted((G.dot(G.sub(px, self.sites[self.site_of[y]]), G.sub(px, self.sites[self.site_of[y]])), y)
                          for y in range(self.n))
            level = Fraction(near[k - 1][0])
            start = birth_node[self.descend(tuple(sorted(y for _d, y in near[:k])), k)]
            node = tree.ancestor(start, level)
            core[self.inp[x]] = Entry(level, node)
            entries.append((level, node, self.inp[x]))
        cover = [None] * self.n
        first = [None] * self.n
        events = []  # (niveau, noeud, masque des points de la boule fermee)
        for ball in self.balls:  # ordre du catalogue : niveaux croissants
            if ball.p + ball.m < k:
                continue
            pop = sorted(ball.inner + ball.shell)
            node = tree.ancestor(birth_node[self.descend(tuple(pop[:k]), k)], ball.level)
            mask = 0
            for x in pop:
                i = self.inp[x]
                mask |= 1 << i
                if cover[i] is None:
                    cover[i] = (ball.level, set())
                    first[i] = (ball.index, node)
                if cover[i][0] == ball.level:
                    cover[i][1].add(node)
            events.append((ball.level, node, mask))
        if any(c is None for c in cover):
            raise InvariantError('ordre %d : point couvert par aucune boule du catalogue' % k)
        self.cover_choice[k] = first
        cuts = _sweep_cuts(nodes, events, entries)
        return OrderResult(k, nodes, lower, core, [Entry(lv, frozenset(ns)) for lv, ns in cover], cuts)


class _Tree(object):
    """Foret d'un ordre, numerotation canonique, et ses parents (-1 pour la racine)."""

    def __init__(self, nodes):
        self.nodes = nodes
        self.parent = [-1] * len(nodes)
        for v, node in enumerate(nodes):
            for c in node.children:
                self.parent[c] = v

    def ancestor(self, node, level):
        """Ancetre du noeud vivant a la coupe fermee : on remonte tant que le parent est de niveau <= level."""
        if self.nodes[node].level > level:
            raise InvariantError('noeud %d ne apres le niveau %s' % (node, level))
        while self.parent[node] >= 0 and self.nodes[self.parent[node]].level <= level:
            node = self.parent[node]
        return node


def _sweep_cuts(nodes, events, entries):
    """Coupes ouvertes et fermees a chaque niveau d'evenement (noeuds, boules couvrantes, entrees core)."""
    at = {}
    for v, node in enumerate(nodes):
        at.setdefault(node.level, ([], [], []))[0].append(v)
    for level, node, mask in events:
        at.setdefault(level, ([], [], []))[1].append((node, mask))
    for level, node, x in entries:
        at.setdefault(level, ([], [], []))[2].append((node, x))
    state = {}  # noeud vivant -> [couverture, coeur]
    cuts, last = [], ()
    for level in sorted(at):
        created, covering, entering = at[level]
        for v in created:  # identifiants croissants : les enfants d'une fusion sont deja vivants
            cov = cor = 0
            for c in nodes[v].children:
                if c not in state:
                    raise InvariantError('enfant %d absent de la coupe a la fusion %d' % (c, v))
                cov |= state[c][0]
                cor |= state[c][1]
                del state[c]
            state[v] = [cov, cor]
        for node, mask in covering:
            if node not in state:
                raise InvariantError('boule couvrante au noeud %d, absent de la coupe fermee %s' % (node, level))
            state[node][0] |= mask
        for node, x in entering:
            if node not in state:
                raise InvariantError('entree core au noeud %d, absent de la coupe fermee %s' % (node, level))
            state[node][1] |= 1 << x
        shut = tuple(sorted((v, s[0], s[1]) for v, s in state.items()))
        cuts.append(Cut(level, last, shut))
        last = shut
    return cuts
