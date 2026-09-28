"""Clustering hierarchique a partir de la mesure du paragraphe 9.1.

Chaine, telle que la these et son code la posent :

  cofaces -> graphe des facettes -> Kruskal -> condensation sur les MASSES
          -> selection d'une antichaine -> vote des points

Trois choix sont explicites ici parce qu'ils ne vont pas de soi.

**Les plateaux ne sont jamais binarises.** Toutes les aretes induites par une
meme coface portent le meme niveau, et plusieurs cofaces peuvent partager un
niveau exactement. Un niveau est donc traite en bloc : toutes les fusions d'un
meme niveau forment **une seule action** a plusieurs enfants. Cela rend aussi
sans objet la question clique/chemin/etoile : les trois sous-graphes couvrants
d'une coface ont les memes composantes a tout seuil, seul le choix d'aretes du
MST differe, et ce choix n'est qu'un departage d'egalites.

**La taille d'un cluster est une masse, pas un compte.** `min_cluster_size` est
une somme de `m_tau`, comme dans le code de la these (`core.py:41-60`). Un
comptage de facettes serait la faute que le paragraphe 9.1 interdit.

**L'echelle de stabilite est declaree.** HGP-C3D construit sur le rayon
normalise, HGP-old sur `r^z`. Les deux donnent le meme arbre puisque l'un est
monotone en l'autre, mais pas les memes stabilites, donc pas la meme selection.
Le mode est un parametre publie, jamais un defaut cache.
"""

import collections
import math

LAMBDA_MODES = ('radius', 'psi')
STABILITY_SCALES = ('lambda', 'log')


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


class Union:
    """Union-find par rang, sans compression recursive."""

    def __init__(self, items):
        self.parent = {item: item for item in items}
        self.rank = {item: 0 for item in items}

    def find(self, item):
        root = item
        while self.parent[root] != root:
            root = self.parent[root]
        while self.parent[item] != root:
            self.parent[item], item = root, self.parent[item]
        return root

    def union(self, left, right):
        left, right = self.find(left), self.find(right)
        if left == right:
            return False
        if self.rank[left] < self.rank[right]:
            left, right = right, left
        self.parent[right] = left
        if self.rank[left] == self.rank[right]:
            self.rank[left] += 1
        return True


def facet_levels(cofaces, keep):
    """Rend, par niveau exact, la liste des groupes de facettes a reunir.

    Chaque coface produit un groupe : ses facettes retenues, toutes reunies au
    niveau `beta` de la coface. Les groupes de meme niveau forment un plateau.
    """
    plateaus = collections.defaultdict(list)
    seen = set()
    for vertices, beta in cofaces:
        group = [tuple(v for v in vertices if v != drop) for drop in vertices]
        group = [facet for facet in group if keep is None or facet in keep]
        if not group:
            continue
        seen.update(group)
        plateaus[beta].append(group)
    need(seen, 'no facet survives')
    return seen, sorted(plateaus.items(), key=lambda item: item[0])


def merge_tree(facets, plateaus, births):
    """Arbre de fusion sur les facettes, un noeud par action de plateau.

    Rend (noeuds, racines). Un noeud est un dict : `children` (noeuds ou
    facettes), `level` (beta exact de naissance du noeud), `members`.
    Les feuilles sont les facettes elles-memes, de niveau `births[facet]`.
    """
    union = Union(facets)
    top = {facet: facet for facet in facets}
    nodes = {}
    counter = [0]
    for beta, groups in plateaus:
        # Toutes les unions du plateau d'abord : le plateau est une seule date.
        merged = collections.defaultdict(set)
        for group in groups:
            roots = {union.find(facet) for facet in group}
            if len(roots) < 2:
                continue
            anchor = min(roots, key=lambda r: str(r))
            for root in roots:
                union.union(anchor, root)
            merged[union.find(anchor)].update(roots)
        for root, roots in merged.items():
            children = sorted({top[r] for r in roots}, key=str)
            if len(children) < 2:
                continue
            name = 'n%d' % counter[0]
            counter[0] += 1
            members = set()
            for child in children:
                members.update(nodes[child]['members'] if child in nodes else {child})
            nodes[name] = dict(children=children, level=beta, members=members)
            for r in roots:
                top[r] = name
            top[root] = name
    roots = sorted({top[union.find(facet)] for facet in facets}, key=str)
    return nodes, roots


def _lam(beta, mode, z):
    """Echelle de stabilite : 1/rayon, ou psi = rayon^(-z)."""
    radius = math.sqrt(float(beta))
    if radius <= 0.0:
        return float('inf')
    return 1.0 / radius if mode == 'radius' else radius ** (-z)


def condense(nodes, roots, masses, births, min_cluster_mass, mode, z, scale='lambda', relative=0.0):
    """Condensation a la HDBSCAN, sur les masses et non sur des comptes.

    En descendant, un enfant dont la masse est sous le seuil ne devient pas un
    cluster : ses facettes quittent le cluster courant a ce niveau. Un noeud
    dont au moins deux enfants survivent est une vraie scission.
    """
    need(mode in LAMBDA_MODES, 'lambda mode is one of ' + ', '.join(LAMBDA_MODES))
    need(scale in STABILITY_SCALES, 'stability scale is one of ' + ', '.join(STABILITY_SCALES))
    # Masses cumulees, en post-ordre EXPLICITE : l'arbre de fusion d'un nuage de
    # quelques milliers de points depasse la profondeur de recursion de Python.
    mass = {}

    def total(item):
        return mass[item]

    pending = list(roots)
    while pending:
        item = pending[-1]
        if item in mass:
            pending.pop()
            continue
        if item not in nodes:
            mass[item] = float(masses.get(item, 0.0))
            pending.pop()
            continue
        missing = [child for child in nodes[item]['children'] if child not in mass]
        if missing:
            pending.extend(missing)
            continue
        mass[item] = sum(mass[child] for child in nodes[item]['children'])
        pending.pop()
    clusters = {}
    order = []

    # Descente en largeur, sans recursion : une file de clusters a ouvrir.
    # Un cluster racine NAIT au niveau de sa propre fusion sommitale, pas a
    # zero : en echelle logarithmique une naissance nulle rend sa stabilite
    # indefinie, et en echelle lambda elle la gonfle artificiellement.
    def root_birth(root):
        level = nodes[root]['level'] if root in nodes else births[root]
        return _lam(level, mode, z)

    queue = [(root, None, root_birth(root)) for root in roots]
    while queue:
        item, parent, birth_lambda = queue.pop()
        name = 'c%d' % len(order)
        order.append(name)
        clusters[name] = dict(parent=parent, birth=birth_lambda, falls=[], children=[], node=item)
        if parent is not None:
            clusters[parent]['children'].append(name)
        stack = [item]
        while stack:
            current = stack.pop()
            if current not in nodes:
                clusters[name]['falls'].append((current, _lam(births[current], mode, z)))
                continue
            level = _lam(nodes[current]['level'], mode, z)
            # Seuil ABSOLU, et facultativement RELATIF au parent. Un seuil
            # absolu seul ne peut pas etre juste a toutes les echelles : il
            # laisse passer une sous-structure dans un gros amas et refuse un
            # vrai petit amas. Le seuil relatif demande a un enfant de porter
            # une fraction de son parent, ce qui est sans echelle.
            floor = min_cluster_mass
            if relative > 0.0:
                floor = max(floor, relative * total(current))
            big = [child for child in nodes[current]['children'] if total(child) >= floor]
            for child in nodes[current]['children']:
                if len(big) >= 2 and child in big:
                    # Le parent a PORTE cet enfant depuis sa propre naissance
                    # jusqu'a la scission : ses facettes quittent le parent ici.
                    # Les omettre viderait le parent de toute sa masse et
                    # ferait preferer les enfants a chaque scission, quel que
                    # soit le contraste. C'est la definition de HDBSCAN.
                    for facet in (nodes[child]['members'] if child in nodes else {child}):
                        clusters[name]['falls'].append((facet, level))
                    queue.append((child, name, level))
                elif child in big or not big:
                    stack.append(child)
                else:
                    for facet in (nodes[child]['members'] if child in nodes else {child}):
                        clusters[name]['falls'].append((facet, level))
    # Echelle de stabilite. En `lambda`, c'est la formule de HDBSCAN. En `log`,
    # on integre en log-densite : la contribution d'une facette est le nombre de
    # DOUBLEMENTS de densite qu'elle traverse dans le cluster, et non la
    # difference brute. La difference brute favorise mecaniquement les enfants,
    # parce qu'un sous-amas serre a des lambda enormes ; le logarithme ramene
    # parent et enfants a la meme echelle et rend l'arbitrage sensible au
    # CONTRASTE plutot qu'a l'amplitude. C'est ce qui decide la famille
    # `hierarchical`, ou l'arbre est juste et seule la selection echoue.
    for name, cluster in clusters.items():
        birth = cluster['birth']
        if scale == 'lambda':
            span = lambda lam: max(0.0, lam - birth)
        else:
            span = lambda lam: (math.log(lam / birth) if birth > 0.0 and lam > birth else 0.0)
        cluster['stability'] = sum(float(masses.get(facet, 0.0)) * span(lam)
                                   for facet, lam in cluster['falls'])
        cluster['mass'] = sum(float(masses.get(facet, 0.0)) for facet, lam in cluster['falls'])
    return clusters, order


def select_excess_of_mass(clusters, order):
    """Antichaine maximisant l'exces de masse, en remontant depuis les feuilles."""
    best, chosen = {}, {}
    for name in reversed(order):
        children = clusters[name]['children']
        below = sum(best[child] for child in children)
        own = clusters[name]['stability']
        if not children or own >= below:
            best[name], chosen[name] = own, True
        else:
            best[name], chosen[name] = below, False
    selected, stack = [], [name for name in order if clusters[name]['parent'] is None]
    while stack:
        name = stack.pop()
        if chosen[name]:
            selected.append(name)
        else:
            stack.extend(clusters[name]['children'])
    return sorted(selected)


def label_facets(clusters, selected):
    """Chaque facette tombee dans un cluster retenu, ou dans un de ses descendants."""
    label = {}
    for index, name in enumerate(selected):
        stack = [name]
        while stack:
            current = stack.pop()
            for facet, _ in clusters[current]['falls']:
                label[facet] = index
            stack.extend(clusters[current]['children'])
    return label


def vote(points, facet_label, sums, totals, clusters_count):
    """V_x(c) = somme des w_x,tau sur les facettes de c contenant x, puis argmax.

    Un point sans aucun vote est du bruit. Les egalites sont tranchees par le
    plus petit indice, de facon deterministe, et leur nombre est publie.
    """
    scores = collections.defaultdict(lambda: [0.0] * clusters_count)
    for facet, value in sums.items():
        cluster = facet_label.get(facet)
        if cluster is None:
            continue
        for point in facet:
            if totals.get(point, 0) > 0:
                scores[point][cluster] += float(value) / float(totals[point])
    labels = [-1] * points
    ties = 0
    for point, row in scores.items():
        top = max(row)
        if top <= 0.0:
            continue
        if row.count(top) > 1:
            ties += 1
        labels[point] = row.index(top)
    return labels, ties
