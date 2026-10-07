"""Oracle borne des supports d'ordre K (tranche S1 de la sortie parametree) : la verite depuis l'etage A seul.

Ce module ne s'appuie QUE sur definition.py (et sur les enregistrements de model.py qu'il importe), comme
tests/tower/forest_oracle.py : ni constructive, ni judge, ni catalogue, ni ordre de Morton. Pour un nuage de positions
DISTINCTES (le moteur refuse les entrees ponderees) et un ordre K, il calcule par la definition :

  W_K      les boules minimales des K- et (K+1)-parties (Definition.meb), dedoublonnees par (centre, niveau) exacts,
           de niveau positif, avec p + q <= K + 1 ; donc p + q - 1 <= K <= p + m (une boule de W_K est la boule
           minimale d'une K-partie si p + q <= K, de I_b u S si p + q = K + 1). I_b, U_b par distances exactes.
           Perimetre (lemme W) : toute liaison de Gabriel (these, Def. 28 : (K+1)-partie G, I_{B(G)} dans G) a sa
           boule dans W_K, et leur nombre egale la somme des gabriel_cofaces (point 3) ; une liaison qui n'est pas de
           Gabriel n'est jamais separante, ses K-faces strictes sont dans une meme composante de la coupe ouverte
           beta(G) (point 4, Th. 4 sans position generale) ; window_tree rend le temoin du point 2 (E5).
  Q_b      les parties Q de U_b, 2 <= |Q| <= 4, affinement independantes, dont le centre c_b a des poids
           barycentriques strictement positifs (formulation de Gram en Fraction, reconstruction exacte) ; q est la
           plus petite arite. Lemme F controle par force brute : Q_b egale la famille des parties NON SEPARABLES
           MINIMALES de U_b, la separabilite etant lue sur beta(A) < lambda_b (M1, T2), sans borne d'arite.
  att(b)   Definition.node_at sur TOUTES les K-parties de P_b a la coupe fermee lambda_b ; toutes doivent donner le
           meme noeud (controle direct de T3, lemme A).
  ant(b)   noeuds vivants a la coupe ouverte (niveau d'evenement precedent) des K-parties STRICTES de P_b.
  role     naissance sans K-partie stricte ; fusion si le niveau de att(b) est lambda_b ; interne sinon. Lemmes B et C
           controles (rangs, enfants, branches, regle du parent att(b) = parent(u) si a_parent(u) = lambda_b, u
           sinon, union des branches d'une fusion egale a ses enfants).
  comptes  kparties_reliees = C(p+m, K), compressed_parts = C(m, t), strict_traces = C(m, t) - N_t, cofaces par
           boule et par support, cofaces de Gabriel (these, Def. 28) : tous recomptes par enumeration brute des
           parties de P_b (avec controle MEB(G) = b pour chaque (K+1)-partie, M2), puis compares aux formules du
           lemme G. Rien n'est stocke dans le format natif : ce sont les comptes que le lecteur derivera.
  lemme H  pour chaque noeud v et chaque coupe d'evenement a ou il est vivant, l'ensemble des sites des K-parties de
           sa composante (Def. 21 : le K-polyedre) egale l'union des P_b, b dans W_K, att(b) dans le sous-arbre de v,
           lambda_b <= a ; a K >= 2 aussi en se restreignant aux boules fortes p + q <= K <= p + m (P3). A K = 1 le
           K-polyedre est l'ensemble des feuilles du sous-arbre ; l'union des P_b l'egale des qu'il a deux sites.
  instantanes  dans l'ordre canonique des boules (postordre du noeud, niveau, centre), celles du sous-arbre de v
           forment une tranche contigue ; celles du sous-arbre strict sont toutes de niveau < a_v ; l'instantane date
           a a de v (boules de niveau <= a) est donc un prefixe de la tranche, et ses supports sont des sites du
           K-polyedre ; seules des boules internes depassent a_v.

Tout controle viole leve model.InvariantError avec le nom du lemme ; la porte (test_supports.py) le compte comme un
ecart. La numerotation des noeuds est celle de l'etage A (naissances par (niveau, centre), fusions par (niveau, plus
petite naissance)) : c'est celle de la foret d'ordre K du moteur (tests/tower/forest_oracle.py la compare noeud a
noeud).

Sortie canonique (canonical(k), un dict JSON, serialise trie et sans espace par la porte) : ce que les differentiels
natifs S3 et S6 relieront, apres traduction des lignes de SITES en coordonnees et tri des boules par la meme cle.
  format, version, k, n ; sites : coordonnees triees (ordre lexicographique, pas Morton) ; ids : facultatif, meme
  ordre (un reetiquetage ne change que cette colonne) ;
  nodes[v] (numerotation canonique) : level (Fraction ecrite 'a/b'), parent (None a la racine), children, kind (0
  feuille-site a K = 1, 1 naissance, 2 fusion), post (rang de postordre, enfants par identifiant croissant), balls
  (indices des boules propres), birth_center ;
  balls (ordre : postordre du noeud, niveau, centre) : node, level, center, role, p, m, qmin, components (|ant|),
  prior (ant pour le role fusion seulement, comme la section PRIOR), supports (listes de coordonnees, ordre (arite,
  coordonnees)), kparties_reliees, compressed_parts, strict_traces, cofaces, cofaces_support, gabriel_cofaces,
  gabriel_cofaces_support.
Les deux ordres qui different du format natif (sites lexicographiques, boules d'un meme noeud et d'un meme niveau par
centre au lieu de S* en SiteIdx) sont geometriques : la sortie ne depend pas de l'ordre des points d'entree.

Domaine : n <= 12 a 14 (cout de l'etage A) ; coquilles quelconques (pas de plafond ici : le plafond natif de 24 et le
refus support_shell_capacity se constatent sur m, voir shell_ball). Seule la force brute du lemme F
(_minimal_nonseparable) a un budget : au plus 2^m - 1 parties candidates (MINIMAL_BUDGET, coquilles de 16 sites au
plus) ; au-dela, refus explicite BudgetRefusal AVANT tout calcul, jamais un resultat partiel (apport des auditeurs du
5 octobre 2026). Sur une coquille plus grande (sphere5, m = 24), seules les primitives Q_b (_supports, Gram) et
N_j pour j petit (closure_upto, par combinaisons) se calculent : elles ne qualifient pas tout S1 a 24 sites.
"""
from fractions import Fraction
from itertools import combinations
from math import comb, gcd

from .definition import Definition
from .model import InvariantError, mask_of, members

ROLE_BIRTH, ROLE_MERGE, ROLE_INTERNAL = 'naissance', 'fusion', 'interne'
KIND_SITE, KIND_BIRTH, KIND_MERGE = 0, 1, 2
SHELL_CAPACITY = 24  # plafond natif de coquille etendue (kMaxShell) ; au-dela, refus support_shell_capacity
MINIMAL_BUDGET = (1 << 16) - 1  # parties candidates de _minimal_nonseparable (au plus 2^m - 1) : m <= 16


class BudgetRefusal(Exception):
    """Refus explicite d'un calcul de l'oracle au-dela de son budget, pris avant tout calcul : jamais une censure
    silencieuse ni un resultat partiel. Ce n'est pas un lemme viole (InvariantError) : l'oracle est borne."""


def _gauss(rows, rhs):
    """Systeme lineaire carre en fractions (Gauss-Jordan, pivot non nul) ; None s'il est singulier."""
    size = len(rows)
    m = [[Fraction(v) for v in rows[i]] + [Fraction(rhs[i])] for i in range(size)]
    for col in range(size):
        piv = next((r for r in range(col, size) if m[r][col] != 0), None)
        if piv is None:
            return None
        m[col], m[piv] = m[piv], m[col]
        head = m[col][col]
        m[col] = [x / head for x in m[col]]
        for r in range(size):
            if r != col and m[r][col] != 0:
                f = m[r][col]
                m[r] = [x - f * y for x, y in zip(m[r], m[col])]
    return [m[i][size] for i in range(size)]


def barycentric(pts, target):
    """Poids barycentriques exacts de target dans l'enveloppe affine des points (Gram : G alpha = (u_i . (target -
    p0)), u_i = p_i - p0) ; None si les points sont affinement dependants ou si target n'est pas dans leur enveloppe
    affine (la reconstruction exacte echoue)."""
    p0 = pts[0]
    edges = [tuple(a - b for a, b in zip(p, p0)) for p in pts[1:]]
    gram = [[sum(x * y for x, y in zip(u, v)) for v in edges] for u in edges]
    alpha = _gauss(gram, [sum(x * (t - y) for x, t, y in zip(u, target, p0)) for u in edges])
    if alpha is None:
        return None
    weights = (1 - sum(alpha),) + tuple(alpha)
    for j in range(3):
        if sum(w * p[j] for w, p in zip(weights, pts)) != target[j]:
            return None
    return weights


def split(points, center, level):
    """(I, U) : indices des points strictement interieurs et sur la sphere, en entiers exacts."""
    den = 1
    for c in center:
        den = den * c.denominator // gcd(den, c.denominator)
    num = [int(c * den) for c in center]
    bound = level * den * den
    if bound.denominator != 1:
        raise InvariantError('niveau %s de centre %r : rayon carre non entier a l\'echelle du centre' % (level, center))
    bound = bound.numerator
    inner, shell = [], []
    for i, p in enumerate(points):
        d = sum((p[j] * den - num[j]) ** 2 for j in range(3))
        if d < bound:
            inner.append(i)
        elif d == bound:
            shell.append(i)
    return tuple(inner), tuple(shell)


class Ball(object):
    """Boule critique positive (enregistrement). center : trois Fraction ; level : rayon carre ; inner, shell :
    indices tries de I_b et U_b ; supports : Q_b (tuples d'indices tries), ordre (arite, coordonnees) ; q : plus petite
    arite. Rempli par Supports pour une boule de W_K : node (att), ant, role, strong, counts."""

    def __init__(self, center, level, inner, shell):
        self.center, self.level, self.inner, self.shell = center, level, inner, shell
        self.p, self.m = len(inner), len(shell)
        self.pop = tuple(sorted(inner + shell))
        self.mask = mask_of(self.pop)
        self.supports, self.q = (), None
        self.node, self.ant, self.role, self.strong, self.counts = None, frozenset(), None, False, {}


class OrderSupports(object):
    """Resultat de l'ordre k (enregistrement) : tree (OrderResult de l'etage A), parent, post (rang de postordre,
    enfants par identifiant croissant), size (taille de sous-arbre), balls (W_K, ordre canonique : postordre du noeud,
    niveau, centre), own (indices des boules propres de chaque noeud, par niveau puis centre), cuts (couples (noeud,
    niveau) juges par le lemme H), counters."""

    def __init__(self, k, tree, parent, post, size, balls, own, cuts, counters):
        self.k, self.tree, self.parent, self.post, self.size = k, tree, parent, post, size
        self.balls, self.own, self.cuts, self.counters = balls, own, cuts, counters


def _postorder(nodes, root):
    post, size = [None] * len(nodes), [1] * len(nodes)
    rank = 0
    stack = [(root, False)]
    while stack:
        v, done = stack.pop()
        if done:
            post[v] = rank
            rank += 1
            size[v] = 1 + sum(size[c] for c in nodes[v].children)
            continue
        stack.append((v, True))
        for c in reversed(nodes[v].children):
            stack.append((c, False))
    if rank != len(nodes):
        raise InvariantError('arbre : %d noeuds atteints depuis la racine sur %d' % (rank, len(nodes)))
    return post, size


def _before(levels, level):
    """Plus grand niveau d'evenement strictement inferieur (la coupe ouverte de level), ou None."""
    out = None
    for lv in levels:
        if lv >= level:
            break
        out = lv
    return out


class Supports(object):
    """Oracle des supports d'un nuage a positions distinctes. order(k) rend l'OrderSupports de l'ordre k (1 <= k <= n),
    calcule une fois ; canonical(k, ids) sa sortie canonique."""

    def __init__(self, points):
        pts = []
        for p in points:
            if len(p) != 3:
                raise ValueError('point a trois coordonnees attendu : %r' % (p,))
            pts.append(tuple(int(c) for c in p))
        if not pts:
            raise ValueError('nuage vide')
        if len(set(pts)) != len(pts):
            raise ValueError('positions distinctes attendues (le moteur refuse les entrees ponderees)')
        self.points = pts
        self.n = len(pts)
        self.definition = Definition(pts)
        self._orders = {}
        self._shapes = {}  # (centre, niveau) -> (I_b, U_b, Q_b) : une fois par boule, quel que soit l'ordre

    # ------------------------------------------------------------ boules et supports

    def shell_ball(self, part):
        """Boule minimale d'une partie (indices) avec I_b et U_b, sans supports : sert a constater une coquille hors
        plafond (sphere50) sans enumerer Q_b."""
        level, center, closed = self.definition.meb(tuple(sorted(part)))
        inner, shell = split(self.points, center, level)
        if mask_of(inner + shell) != closed:
            raise InvariantError('population : masque ferme de l\'etage A et partage (I, U) divergent')
        return Ball(center, level, inner, shell)

    def ball_of(self, part):
        """Boule minimale d'une partie avec I_b, U_b et Q_b, q ; enregistrement neuf (les champs d'un ordre sont vides),
        geometrie et Q_b calcules une fois par boule."""
        level, center, _closed = self.definition.meb(tuple(sorted(part)))
        if (center, level) not in self._shapes:
            ball = self.shell_ball(part)
            self._shapes[(center, level)] = (ball.inner, ball.shell, self._supports(ball))
        inner, shell, supports = self._shapes[(center, level)]
        ball = Ball(center, level, inner, shell)
        ball.supports, ball.q = supports, len(supports[0])
        return ball

    def is_support(self, ball, q):
        """Q (indices de U_b) est-il un support positif : affinement independant, c_b de poids tous > 0 ?"""
        weights = barycentric([self.points[i] for i in q], ball.center)
        return weights is not None and all(w > 0 for w in weights)

    def _supports(self, ball):
        """Q_b par la formulation de Gram, sur toute la coquille (aucune limite a q), ordre (arite, coordonnees)."""
        found = []
        for size in range(2, min(4, ball.m) + 1):
            for q in combinations(ball.shell, size):
                if self.is_support(ball, q):
                    found.append(q)
        found.sort(key=lambda q: (len(q), sorted(self.points[i] for i in q)))
        if not found:
            raise InvariantError('lemme F : boule %s de niveau %s sans support positif' % (ball.center, ball.level))
        supports = found
        return tuple(supports)

    def _minimal_nonseparable(self, ball, budget=MINIMAL_BUDGET):
        """Lemme F par force brute : parties non separables minimales de U_b, toutes arites. A est separable si et
        seulement si beta(A) < lambda_b (M1 : B(A) = b si et seulement si c_b est dans conv(A), A sur la sphere ; T2).
        Les candidats de taille s prolongent les parties separables de taille s - 1 (la separabilite descend aux
        sous-parties) : toute partie minimale est examinee. Budget : un candidat est une partie non vide de U_b, formee
        une seule fois (sa base est elle-meme privee de son plus grand site), donc au plus 2^m - 1 candidats et autant
        de boules minimales ; si cette borne depasse le budget, refus BudgetRefusal avant tout calcul."""
        if (1 << ball.m) - 1 > budget:
            raise BudgetRefusal('lemme F : %d parties candidates possibles sur une coquille de %d sites, au-dela du '
                                'budget %d de _minimal_nonseparable : refus explicite, aucun resultat partiel'
                                % ((1 << ball.m) - 1, ball.m, budget))
        beta = self.definition.beta
        separable = set([()])
        frontier = [()]
        minimal = []
        for size in range(1, ball.m + 1):
            grown = []
            for base in frontier:
                for x in ball.shell:
                    if base and x <= base[-1]:
                        continue
                    cand = base + (x,)
                    if any(cand[:i] + cand[i + 1:] not in separable for i in range(size)):
                        continue
                    if beta(cand) < ball.level:
                        separable.add(cand)
                        grown.append(cand)
                    else:
                        minimal.append(cand)
            frontier = grown
            if not frontier:
                break
        return minimal

    def closure_upto(self, ball, jmax):
        """N_0 .. N_jmax par combinaisons : nombre de parties de U_b a j sites qui contiennent un support de Q_b
        (ball.supports, rempli par ball_of), sans parcourir les 2^m masques de _counts. Primitive des coquilles hors
        de la force brute (sphere5, m = 24, suite primitives de test_supports.py)."""
        pos = dict((x, j) for j, x in enumerate(ball.shell))
        masks = [sum(1 << pos[x] for x in q) for q in ball.supports]
        closure = []
        for size in range(jmax + 1):
            count = 0
            for part in combinations(range(ball.m), size):
                mask = sum(1 << i for i in part)
                count += any(q & mask == q for q in masks)
            closure.append(count)
        return closure

    # ------------------------------------------------------------ un ordre

    def order(self, k):
        if not 1 <= k <= self.n:
            raise ValueError('ordre %d hors de [1, %d]' % (k, self.n))
        if k not in self._orders:
            self._orders[k] = self._build(k)
        return self._orders[k]

    def _window(self, k):
        """W_K : boules minimales des K- et (K+1)-parties, positives, dedoublonnees par (centre, niveau), p+q <= K+1.
        Perimetre (spec 2.2) : toute liaison de Gabriel, (K+1)-partie G avec I_{B(G)} dans G (these, Def. 28), a sa
        boule dans W_K ; rend aussi le nombre de ces liaisons, que la somme des gabriel_cofaces doit egaler."""
        D = self.definition
        found = {}
        for size in ((k, k + 1) if k < self.n else (k,)):
            for part in combinations(range(self.n), size):
                level, center, _closed = D.meb(part)
                if level > 0 and (center, level) not in found:
                    found[(center, level)] = part
        balls, every = [], {}
        for key, part in found.items():
            ball = self.ball_of(part)
            every[key] = ball
            if ball.p + ball.q <= k + 1:
                if ball.p + ball.m < k:
                    raise InvariantError('W_K : boule minimale d\'une %d-partie avec p + m < K' % len(part))
                balls.append(ball)
        kept = set(id(b) for b in balls)
        gabriel = 0
        for part in (combinations(range(self.n), k + 1) if k < self.n else ()):
            level, center, _closed = D.meb(part)
            ball = every[(center, level)]
            if mask_of(ball.inner) & ~mask_of(part) == 0:
                gabriel += 1
                if id(ball) not in kept:
                    raise InvariantError('perimetre : liaison de Gabriel %r de boule hors de W_K (p=%d, q=%d)'
                                         % (part, ball.p, ball.q))
        return balls, gabriel, every

    def window_tree(self, k, all_vertices=True):
        """Temoin du lemme W, point 2 : fusions (niveau, nombre d'enfants), triees, de l'arbre du graphe restreint
        aux liaisons dont la boule est dans W_K. all_vertices : toutes les K-parties restent des sommets, a leur
        niveau ; sinon seules celles dont la boule minimale est dans W_K, les autres n'entrant qu'avec une liaison
        retenue qui les contient (sommets neufs a son niveau). Les deux lectures different de T_K en general : une
        boule hors fenetre rattache des sommets, qui decident plus tard des composantes touchees (E5, K = 2)."""
        D = self.definition
        _balls, _gabriel, every = self._window(k)

        def kept(part):
            level, center, _closed = D.meb(part)
            ball = every.get((center, level))
            return ball is not None and ball.p + ball.q <= k + 1
        at = {}
        for part in combinations(range(self.n), k):
            if all_vertices or kept(part):
                at.setdefault(D.beta(part), ([], []))[0].append(part)
        for part in (combinations(range(self.n), k + 1) if k < self.n else ()):
            if kept(part):
                at.setdefault(D.beta(part), ([], []))[1].append(part)
        parent = {}

        def find(x):
            while parent[x] != x:
                parent[x] = parent[parent[x]]
                x = parent[x]
            return x
        merges = []
        for level in sorted(at):
            vertices, edges = at[level]
            old = dict((x, find(x)) for x in parent)
            for x in vertices:
                parent[x] = x
            for g in edges:
                faces = [g[:s] + g[s + 1:] for s in range(k + 1)]
                for face in faces:
                    parent.setdefault(face, face)
                for face in faces[1:]:
                    parent[find(face)] = find(faces[0])
            children = {}
            for x, root in old.items():
                children.setdefault(find(x), set()).add(root)
            merges.extend((level, len(kids)) for kids in children.values() if len(kids) >= 2)
        return sorted(merges)

    def _build(self, k):
        D = self.definition
        tree = D.order(k)
        nodes = tree.nodes
        parent = [-1] * len(nodes)
        for v, node in enumerate(nodes):
            for c in node.children:
                parent[c] = v
        roots = [v for v in range(len(nodes)) if parent[v] < 0]
        if len(roots) != 1:
            raise InvariantError('arbre d\'ordre %d : %d racines' % (k, len(roots)))
        post, size = _postorder(nodes, roots[0])
        levels = [cut.level for cut in tree.cuts]
        home = {}  # K-partie -> noeud vivant a la coupe fermee de son propre niveau
        for part in combinations(range(self.n), k):
            home[part] = D.node_at(k, part, D.beta(part))
        balls, gabriel, every = self._window(k)
        for ball in balls:
            self._attach(k, ball, nodes, parent, levels)
            self._lemma_f(ball)
            self._counts(k, ball)
        if sum(b.counts['gabriel_cofaces'] for b in balls) != gabriel:
            raise InvariantError('perimetre : %d liaisons de Gabriel, %d comptees par les boules de W_K'
                                 % (gabriel, sum(b.counts['gabriel_cofaces'] for b in balls)))
        others = self._theorem_4(k, nodes, parent, home, every)
        balls.sort(key=lambda b: (post[b.node], b.level, b.center))
        own = [[] for _ in nodes]
        for i, ball in enumerate(balls):
            own[ball.node].append(i)
        self._nodes(k, nodes, parent, balls, own)
        cuts = self._lemma_h(k, nodes, parent, post, size, levels, balls, home)
        counters = _order_counters(k, nodes, balls, cuts)
        counters['gabriel'] = gabriel
        counters['non_gabriel'] = others
        return OrderSupports(k, tree, parent, post, size, balls, own, cuts, counters)

    def _theorem_4(self, k, nodes, parent, home, every):
        """Lemme W, point 4 (Th. 4 de la these sans position generale) : une liaison G qui n'est pas de Gabriel n'est
        jamais separante : ses K-faces strictes sont dans une meme composante de la coupe ouverte beta(G). La coupe
        ouverte est lue ici par une remontee propre (ancetre de niveau < beta(G) dont le parent ne l'est pas),
        independante de Definition.node_at. Rend le nombre de liaisons jugees."""
        D = self.definition
        judged = 0
        for g in (combinations(range(self.n), k + 1) if k < self.n else ()):
            level, center, _closed = D.meb(g)
            if mask_of(every[(center, level)].inner) & ~mask_of(g) == 0:
                continue
            seen = set()
            for s in range(k + 1):
                face = g[:s] + g[s + 1:]
                if D.beta(face) < level:
                    node = home[face]
                    while parent[node] >= 0 and nodes[parent[node]].level < level:
                        node = parent[node]
                    seen.add(node)
            if len(seen) > 1:
                raise InvariantError('lemme W.4 : liaison non Gabriel %r separante (branches %r)' % (g, sorted(seen)))
            judged += 1
        return judged

    def _attach(self, k, ball, nodes, parent, levels):
        """att(b), ant(b), role ; lemmes A, B et C (points 1 et 2)."""
        D = self.definition
        parts = list(combinations(ball.pop, k))
        strict = [part for part in parts if D.beta(part) < ball.level]
        opened = _before(levels, ball.level)
        seen = set(D.node_at(k, part, ball.level) for part in parts)
        if len(seen) != 1:
            raise InvariantError('lemme A (T3) : les %d K-parties de P_b donnent les noeuds %r' % (len(parts), seen))
        ball.node = seen.pop()
        ball.ant = frozenset(D.node_at(k, part, opened) for part in strict)
        node = nodes[ball.node]
        if not strict:
            ball.role = ROLE_BIRTH
        elif node.level == ball.level:
            ball.role = ROLE_MERGE
        else:
            ball.role = ROLE_INTERNAL
        ball.strong = ball.p + ball.q <= k <= ball.p + ball.m
        # lemme B
        if ball.role == ROLE_BIRTH:
            if k == 1 or node.children or node.level != ball.level or node.center != ball.center:
                raise InvariantError('lemme B : naissance de niveau %s rattachee au noeud %d (niveau %s, %d enfants)'
                                     % (ball.level, ball.node, node.level, len(node.children)))
        else:
            if len(ball.pop) < k + 1:
                raise InvariantError('lemme B : cellule a trace stricte et |P_b| = %d < K + 1' % len(ball.pop))
            if ball.role == ROLE_MERGE and not node.children:
                raise InvariantError('lemme B : fusion rattachee a une naissance de meme niveau %s' % ball.level)
            if ball.role == ROLE_INTERNAL and not (node.level < ball.level and
                                                   (parent[ball.node] < 0 or
                                                    nodes[parent[ball.node]].level > ball.level)):
                raise InvariantError('lemme B : interne hors de la vie [a_v, a_parent) du noeud %d' % ball.node)
        # lemme C, point 1 : les traces comprimees strictes I_b u A donnent les memes branches
        t = k - ball.p
        if not 1 <= t <= ball.m:
            raise InvariantError('W_K : t = K - p = %d hors de [1, m = %d]' % (t, ball.m))
        traces = [tuple(sorted(ball.inner + a)) for a in combinations(ball.shell, t)]
        compressed = frozenset(D.node_at(k, f, opened) for f in traces if D.beta(f) < ball.level)
        if compressed != ball.ant:
            raise InvariantError('lemme C.1 : branches des traces comprimees %r contre %r' % (compressed, ball.ant))
        # lemme C, point 2, et regle du parent (lemme D) : att(b) = parent(u) si a_parent(u) = lambda_b, u sinon
        if ball.role == ROLE_INTERNAL and ball.ant != frozenset([ball.node]):
            raise InvariantError('lemme C.2 : interne de branches %r sur le noeud %d' % (sorted(ball.ant), ball.node))
        if ball.role == ROLE_MERGE and not ball.ant <= frozenset(node.children):
            raise InvariantError('lemme C.2 : fusion de branches %r hors des enfants %r du noeud %d'
                                 % (sorted(ball.ant), node.children, ball.node))
        for u in ball.ant:
            up = parent[u]
            rule = up if up >= 0 and nodes[up].level == ball.level else u
            if rule != ball.node:
                raise InvariantError('lemme C.2 (regle du parent) : branche %d donne %d, att(b) = %d'
                                     % (u, rule, ball.node))
        ball.counts['parts'] = len(parts)
        ball.counts['strict_parts'] = len(strict)

    def _lemma_f(self, ball):
        """Q_b (Gram) egale les parties non separables minimales (force brute) ; chaque support redonne sa boule (M1) ;
        premier support d'arite q ; coquille reguliere : Q_b = {U_b}."""
        brute = sorted(self._minimal_nonseparable(ball))
        if brute != sorted(ball.supports):
            raise InvariantError('lemme F : Q_b %r contre parties non separables minimales %r'
                                 % (sorted(ball.supports), brute))
        for q in ball.supports:
            level, center, _closed = self.definition.meb(q)
            if (level, center) != (ball.level, ball.center):
                raise InvariantError('lemme F (M1) : le support %r ne redonne pas sa boule' % (q,))
        if min(len(q) for q in ball.supports) != ball.q or not 2 <= ball.q <= 4:
            raise InvariantError('lemme F : q = %r' % ball.q)
        if ball.m == ball.q and ball.supports != (ball.shell,):
            raise InvariantError('lemme F : coquille reguliere de supports %r' % (ball.supports,))

    def _counts(self, k, ball):
        """Comptes du lemme G : enumeration brute des parties de P_b, controle M2, puis formules."""
        D = self.definition
        p, m, t = ball.p, ball.m, k - ball.p
        key = (ball.level, ball.center)
        inner = frozenset(ball.inner)
        pos = dict((x, j) for j, x in enumerate(ball.shell))
        sup_masks = [sum(1 << pos[x] for x in q) for q in ball.supports]
        closure = [0] * (m + 1)  # N_j : parties de U_b de taille j qui contiennent un support
        for mask in range(1 << m):
            if any(s & mask == s for s in sup_masks):
                closure[bin(mask).count('1')] += 1
        parts = list(combinations(ball.pop, k))
        compressed = sum(1 for f in parts if inner <= frozenset(f))
        strict_traces = sum(1 for a in combinations(ball.shell, t)
                            if D.beta(tuple(sorted(ball.inner + a))) < ball.level)
        cofaces = gabriel = 0
        per_support = [0] * len(ball.supports)
        per_gabriel = [0] * len(ball.supports)
        shared = 0
        for coface in combinations(ball.pop, k + 1):
            g = frozenset(coface)
            inside = [j for j, q in enumerate(ball.supports) if g >= frozenset(q)]
            level, center, _closed = D.meb(coface)
            if inside and (level, center) != key:
                raise InvariantError('M2 : %r contient un support et sa boule minimale n\'est pas b' % (coface,))
            if (level, center) != key:
                continue
            cofaces += 1
            gab = inner <= g
            gabriel += gab
            shared += len(inside) >= 2
            for j in inside:
                per_support[j] += 1
                per_gabriel[j] += gab
        want = dict(
            parts=comb(p + m, k), compressed=comb(m, t), strict_traces=comb(m, t) - closure[t],
            cofaces=sum(comb(p, k + 1 - j) * closure[j] for j in range(m + 1) if 0 <= k + 1 - j <= p),
            gabriel=closure[t + 1] if t + 1 <= m else 0,
            per_support=[comb(p + m - len(q), k + 1 - len(q)) if k + 1 >= len(q) else 0 for q in ball.supports],
            per_gabriel=[comb(m - len(q), t + 1 - len(q)) if t + 1 >= len(q) else 0 for q in ball.supports])
        got = dict(parts=len(parts), compressed=compressed, strict_traces=strict_traces, cofaces=cofaces,
                   gabriel=gabriel, per_support=per_support, per_gabriel=per_gabriel)
        for name in sorted(want):
            if got[name] != want[name]:
                raise InvariantError('lemme G : %s = %r par enumeration, %r par formule (boule %s, p=%d, m=%d, K=%d)'
                                     % (name, got[name], want[name], ball.level, p, m, k))
        if ball.counts.get('parts', len(parts)) != len(parts):
            raise InvariantError('lemme G : kparties_reliees du rattachement et du compte divergent')
        if (ball.role == ROLE_BIRTH) != (strict_traces == 0):
            raise InvariantError('lemme G : role %s avec strict_traces = %d' % (ball.role, strict_traces))
        if per_support and not max(per_support) <= cofaces <= sum(per_support):
            raise InvariantError('lemme G : max_Q cofaces(Q) <= cofaces(b) <= somme non tenu')
        if (cofaces == sum(per_support)) != (shared == 0):
            raise InvariantError('lemme G : somme des cofaces par support et cofaces partagees')
        if m == ball.q:
            regular = {k: dict(parts=1, cofaces=0, strict_traces=0),
                       k + 1: dict(parts=k + 1, cofaces=1, strict_traces=ball.q)}.get(p + ball.q)
            if regular and any(got[name] != value for name, value in regular.items()):
                raise InvariantError('lemme G : cas regulier p + q = %d a K = %d : %r' % (p + ball.q, k, got))
        ball.counts.update(kparties_reliees=len(parts), compressed_parts=compressed, strict_traces=strict_traces,
                           cofaces=cofaces, cofaces_support=tuple(per_support), gabriel_cofaces=gabriel,
                           gabriel_cofaces_support=tuple(per_gabriel), components=len(ball.ant))

    def _nodes(self, k, nodes, parent, balls, own):
        """Partition de W_K sur les noeuds : lemme C point 3, naissances et listes propres (I11)."""
        for v, node in enumerate(nodes):
            mine = [balls[i] for i in own[v]]
            if k == 1 and not node.children:
                if mine:
                    raise InvariantError('K = 1 : la feuille %d porte %d boule(s)' % (v, len(mine)))
                continue
            if node.children:
                merges = [b for b in mine if b.role == ROLE_MERGE]
                union = frozenset()
                for b in merges:
                    union |= b.ant
                if not merges or union != frozenset(node.children):
                    raise InvariantError('lemme C.3 : fusion %d, branches %r contre enfants %r'
                                         % (v, sorted(union), node.children))
                if any(b.role == ROLE_BIRTH for b in mine):
                    raise InvariantError('lemme B : naissance rattachee a la fusion %d' % v)
            else:
                births = [b for b in mine if b.role == ROLE_BIRTH]
                if len(births) != 1 or births[0].center != node.center:
                    raise InvariantError('lemme B : naissance %d avec %d boule(s) de naissance' % (v, len(births)))
            if mine[0].level != node.level:
                raise InvariantError('liste propre du noeud %d : premiere boule hors du niveau %s' % (v, node.level))
            if any(b.level != node.level and b.role != ROLE_INTERNAL for b in mine):
                raise InvariantError('liste propre du noeud %d : boule non interne apres sa naissance' % v)

    def _lemma_h(self, k, nodes, parent, post, size, levels, balls, home_of):
        """Lemme H, tranches contigues et instantanes dates, pour chaque noeud et chaque coupe d'evenement ou il est
        vivant. Rend le nombre de couples (noeud, coupe) juges."""
        D = self.definition
        home = [[] for _ in nodes]  # K-parties par noeud vivant a leur propre niveau : (beta, masque)
        for part, node in home_of.items():
            home[node].append((D.beta(part), mask_of(part)))
        by_post = [None] * len(nodes)
        for v in range(len(nodes)):
            by_post[post[v]] = v
        site = dict((tuple(Fraction(c) for c in p), x) for x, p in enumerate(self.points))
        checked = 0
        for v, node in enumerate(nodes):
            low = post[v] - size[v]
            inside = [by_post[j] for j in range(low + 1, post[v] + 1)]
            parts = sorted(entry for w in inside for entry in home[w])
            # boules du sous-arbre : une tranche contigue de l'ordre canonique (postordre du noeud d'abord)
            span = [i for i, b in enumerate(balls) if low < post[b.node] <= post[v]]
            if span and span != list(range(span[0], span[-1] + 1)):
                raise InvariantError('tranche : boules du sous-arbre de %d non contigues %r' % (v, span))
            subtree_balls = [balls[i] for i in span]
            if any(b.level >= node.level for b in subtree_balls if b.node != v):
                raise InvariantError('instantane : boule du sous-arbre strict de %d au niveau >= a_v' % v)
            covered = [(b.level, b.mask) for b in subtree_balls]
            strong = [(b.level, b.mask) for b in subtree_balls if b.strong]
            end = None if parent[v] < 0 else nodes[parent[v]].level
            leaves = mask_of(site[nodes[w].center] for w in inside if k == 1 and not nodes[w].children)
            for a in levels:
                if a < node.level or (end is not None and a >= end):
                    continue
                pts = rhs = rhs_strong = 0
                for beta, mask in parts:
                    if beta <= a:
                        pts |= mask
                for level, mask in covered:
                    if level <= a:
                        rhs |= mask
                for level, mask in strong:
                    if level <= a:
                        rhs_strong |= mask
                if k == 1:
                    want = pts if bin(pts).count('1') >= 2 else 0
                    if pts != leaves or rhs != want:
                        raise InvariantError('lemme H (K = 1) : noeud %d coupe %s : feuilles %r, union %r'
                                             % (v, a, members(leaves), members(rhs)))
                elif pts != rhs or pts != rhs_strong:
                    raise InvariantError('lemme H : noeud %d coupe %s : K-polyedre %r, union W_K %r, fortes %r'
                                         % (v, a, members(pts), members(rhs), members(rhs_strong)))
                # instantane date : prefixe de la tranche (les boules propres, de niveau croissant, viennent en
                # dernier) ; ses supports sont des sites du K-polyedre
                prefix = [b for b in subtree_balls if b.level <= a]
                if prefix != subtree_balls[:len(prefix)]:
                    raise InvariantError('instantane : noeud %d coupe %s, pas un prefixe de la tranche' % (v, a))
                for b in prefix:
                    if any(mask_of(q) & ~pts for q in b.supports):
                        raise InvariantError('instantane : support hors du K-polyedre (noeud %d coupe %s)' % (v, a))
                checked += 1
        return checked

    # ------------------------------------------------------------ sortie canonique

    def canonical(self, k, ids=None):
        """Sortie canonique de l'ordre k (dict JSON). Sites designes par leurs coordonnees ; liste des sites triee ;
        ids (facultatif, meme ordre que l'entree) publie en colonne a part : un reetiquetage ne change qu'elle.
        Noeuds : numerotation de l'etage A ; boules : (postordre du noeud, niveau, centre) ; supports : (arite,
        coordonnees). prior : ant(b) pour le role fusion seulement (comme la section PRIOR du format), components :
        |ant(b)| pour tout role."""
        res = self.order(k)
        pts = self.points

        def site(i):
            return list(pts[i])
        out = dict(format='hgp11_supports_oracle', version=1, k=k, n=self.n, sites=[list(p) for p in sorted(pts)])
        if ids is not None:
            label = dict(zip(pts, ids))
            out['ids'] = [label[p] for p in sorted(pts)]
        nodes = []
        for v, node in enumerate(res.tree.nodes):
            kind = KIND_MERGE if node.children else (KIND_SITE if k == 1 else KIND_BIRTH)
            nodes.append(dict(level=str(node.level), parent=None if res.parent[v] < 0 else res.parent[v],
                              children=list(node.children), kind=kind, post=res.post[v], balls=list(res.own[v]),
                              birth_center=None if node.center is None else [str(c) for c in node.center]))
        out['nodes'] = nodes
        balls = []
        for ball in res.balls:
            c = ball.counts
            balls.append(dict(
                node=ball.node, level=str(ball.level), center=[str(x) for x in ball.center], role=ball.role,
                p=ball.p, m=ball.m, qmin=ball.q, components=c['components'],
                prior=sorted(ball.ant) if ball.role == ROLE_MERGE else [],
                supports=[[site(i) for i in sorted(q, key=lambda i: pts[i])] for q in ball.supports],
                kparties_reliees=c['kparties_reliees'], compressed_parts=c['compressed_parts'],
                strict_traces=c['strict_traces'], cofaces=c['cofaces'], cofaces_support=list(c['cofaces_support']),
                gabriel_cofaces=c['gabriel_cofaces'], gabriel_cofaces_support=list(c['gabriel_cofaces_support'])))
        out['balls'] = balls
        return out


def _order_counters(k, nodes, balls, cuts):
    """Compteurs d'un ordre (sommes sur la porte)."""
    cnt = dict(orders=1, nodes=len(nodes), merges=sum(1 for n in nodes if n.children),
               nary_merges=sum(1 for n in nodes if len(n.children) >= 3), balls=len(balls), cuts=cuts,
               births=0, merge_balls=0, internal=0, passing=0, supports=0, arity2=0, arity3=0, arity4=0,
               extended=0, multi_support=0, extended_higher_arity=0, strong=0, weak=0, cofaces=0,
               kparties=0, shared_cofaces=0)
    for b in balls:
        cnt['births'] += b.role == ROLE_BIRTH
        cnt['merge_balls'] += b.role == ROLE_MERGE
        cnt['internal'] += b.role == ROLE_INTERNAL
        cnt['passing'] += b.role == ROLE_MERGE and len(b.ant) == 1
        cnt['supports'] += len(b.supports)
        for q in b.supports:
            cnt['arity%d' % len(q)] += 1
        cnt['extended'] += b.m > b.q
        cnt['multi_support'] += len(b.supports) >= 2
        cnt['extended_higher_arity'] += b.m > b.q and any(len(q) > b.q for q in b.supports)
        cnt['strong'] += b.strong
        cnt['weak'] += b.p + b.q == k + 1
        cnt['cofaces'] += b.counts['cofaces']
        cnt['kparties'] += b.counts['kparties_reliees']
        cnt['shared_cofaces'] += b.counts['cofaces'] != sum(b.counts['cofaces_support'])
    return cnt
