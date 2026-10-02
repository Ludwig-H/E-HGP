"""Modele canonique commun aux deux etages de la reference : donnees passives, aucune geometrie.

Un ordre k est decrit par un OrderResult. Les deux etages (definition.py et constructive.py) le remplissent par
des chemins sans idee commune ; le juge (judge.py) compare champ par champ.

Convention de numerotation (intrinseque : ni rang de Morton, ni support) :
  - naissances d'abord, triees par (niveau, centre de la boule de naissance) ;
  - fusions ensuite, triees par (niveau, plus petite naissance du sous-arbre) ;
  - enfants d'une fusion tries par identifiant.
Deux naissances d'un meme ordre n'ont jamais la meme boule ; deux fusions de meme niveau portent des sous-arbres
disjoints : l'ordre est total. La numerotation des dumps de la v10 est une autre convention, deduite de celle-ci
par dumps.py.

Ensembles de points : entiers utilises comme masques de bits, bit i = point d'indice i dans le nuage d'entree.
Niveaux : rayons carres, fractions.Fraction (0 pour un entier nul). Coupes : fermee (niveau <= a), ouverte (< a).
"""
from bisect import bisect_right
from collections import namedtuple

# Noeud : level (Fraction), children (identifiants tries ; vide pour une naissance), center (trois Fraction pour une
# naissance, None pour une fusion).
Node = namedtuple('Node', 'level children center')

# Coupe a un niveau d'evenement : opened et closed sont des tuples tries de (noeud vivant, couverture, coeur).
Cut = namedtuple('Cut', 'level opened closed')

# Entree d'un point : level (Fraction), nodes (identifiant pour core ; frozenset d'identifiants pour cover).
Entry = namedtuple('Entry', 'level nodes')


class InvariantError(Exception):
    """Invariant viole : une precondition interne ou un theoreme invoque ne tient pas (code de porte 3)."""


class OrderResult(object):
    """Resultat canonique d'un ordre k.

    nodes  : tuple de Node, numerotation canonique.
    parent : tuple, parent de chaque noeud (-1 pour la racine).
    lower  : tuple, image verticale de chaque noeud dans l'ordre k - 1 (noeud vivant a la coupe fermee du niveau
             du noeud) ; None a k = 1.
    core   : tuple par point d'Entry(D_k(x), noeud vivant a la coupe fermee D_k(x) dont la composante contient x).
    cover  : tuple par point d'Entry(alpha_k(x)^2, frozenset des noeuds vivants a cette coupe fermee qui couvrent x).
    cuts   : tuple de Cut, un par niveau d'evenement de l'etage qui l'a produit, niveaux croissants.
    """

    def __init__(self, k, nodes, lower, core, cover, cuts):
        self.k = k
        self.nodes = tuple(nodes)
        self.parent = parents_of(self.nodes)
        self.lower = None if lower is None else tuple(lower)
        self.core = tuple(core)
        self.cover = tuple(cover)
        self.cuts = tuple(cuts)
        self._cut_levels = [c.level for c in self.cuts]

    def cut(self, level):
        """(ouverte, fermee) a un niveau quelconque. Entre deux niveaux d'evenement rien ne change : les deux coupes
        sont la coupe fermee du dernier niveau d'evenement strictement inferieur (ou inferieur ou egal)."""
        i = bisect_right(self._cut_levels, level)
        if i == 0:
            return (), ()
        last = self.cuts[i - 1]
        if last.level == level:
            return last.opened, last.closed
        return last.closed, last.closed

    def alive(self, node, level, closed=True):
        """Ancetre du noeud vivant a la coupe : remontee tant que le parent est ne au plus tard a la coupe."""
        if not (self.nodes[node].level <= level if closed else self.nodes[node].level < level):
            raise InvariantError('noeud %d ne apres la coupe %s' % (node, level))
        while True:
            par = self.parent[node]
            if par < 0:
                return node
            plevel = self.nodes[par].level
            if not (plevel <= level if closed else plevel < level):
                return node
            node = par


def parents_of(nodes):
    parent = [-1] * len(nodes)
    for v, node in enumerate(nodes):
        for c in node.children:
            if parent[c] != -1:
                raise InvariantError('noeud %d : deux parents' % c)
            parent[c] = v
    return tuple(parent)


def canonical_nodes(births, merges):
    """Numerotation canonique.

    births : liste de (niveau, centre) ; identifiants provisoires 0 .. B - 1 dans l'ordre recu.
    merges : liste de (niveau, enfants provisoires) ; identifiants provisoires B, B + 1, ... dans l'ordre recu ; un
             enfant est cree avant sa fusion.
    Rend (nodes, remap) : remap[provisoire] = canonique.
    """
    nb = len(births)
    keys = [(lv, c) for lv, c in births]
    if len(set(keys)) != nb:
        raise InvariantError('deux naissances de meme boule dans un ordre')
    order = sorted(range(nb), key=lambda i: keys[i])
    remap = [None] * (nb + len(merges))
    for new, old in enumerate(order):
        remap[old] = new
    minleaf = [None] * (nb + len(merges))
    for old in range(nb):
        minleaf[old] = remap[old]
    for j, (_lv, kids) in enumerate(merges):
        if any(c >= nb + j for c in kids):
            raise InvariantError('fusion creee avant un de ses enfants')
        minleaf[nb + j] = min(minleaf[c] for c in kids)
    morder = sorted(range(len(merges)), key=lambda j: (merges[j][0], minleaf[nb + j]))
    for new, j in enumerate(morder):
        remap[nb + j] = nb + new
    nodes = [None] * (nb + len(merges))
    for old in range(nb):
        nodes[remap[old]] = Node(births[old][0], (), births[old][1])
    for j, (lv, kids) in enumerate(merges):
        nodes[remap[nb + j]] = Node(lv, tuple(sorted(remap[c] for c in kids)), None)
    return tuple(nodes), remap


def validate_tree(nodes):
    """Arbre de fusion bien forme. Rend None, ou la premiere faute (texte).

    Une racine ; naissances sans enfant ; fusion d'au moins deux enfants, tous nes strictement avant elle (plateaux
    atomiques : une fusion n'est jamais l'enfant d'une fusion de meme niveau exact, et une naissance n'est jamais
    absorbee a son propre niveau) ; ordre canonique des identifiants.
    """
    if not nodes:
        return 'arbre vide'
    try:
        parent = parents_of(nodes)
    except InvariantError as exc:
        return str(exc)
    if sum(1 for p in parent if p < 0) != 1:
        return '%d racines' % sum(1 for p in parent if p < 0)
    seen_merge = False
    for v, node in enumerate(nodes):
        if node.center is None:
            seen_merge = True
            if len(node.children) < 2:
                return 'noeud %d : fusion de %d enfant(s)' % (v, len(node.children))
            if list(node.children) != sorted(set(node.children)):
                return 'noeud %d : enfants non tries ou repetes' % v
            for c in node.children:
                if not 0 <= c < v:
                    return 'noeud %d : enfant %d cree apres lui' % (v, c)
                if not nodes[c].level < node.level:
                    return 'noeud %d : enfant %d de niveau %s, fusion de niveau %s' % (v, c, nodes[c].level,
                                                                                         node.level)
        else:
            if seen_merge:
                return 'noeud %d : naissance apres une fusion' % v
            if node.children:
                return 'noeud %d : naissance avec enfants' % v
    births = [(n.level, n.center) for n in nodes if n.center is not None]
    if births != sorted(births) or len(set(births)) != len(births):
        return 'naissances hors de l\'ordre canonique'
    leaf = list(range(len(nodes)))
    for v, node in enumerate(nodes):
        if node.children:
            leaf[v] = min(leaf[c] for c in node.children)
    merges = [(n.level, leaf[v]) for v, n in enumerate(nodes) if n.center is None]
    if merges != sorted(merges) or len(set(merges)) != len(merges):
        return 'fusions hors de l\'ordre canonique'
    return None


def mask_of(indices):
    m = 0
    for i in indices:
        m |= 1 << i
    return m


def members(mask):
    out = []
    i = 0
    while mask:
        if mask & 1:
            out.append(i)
        mask >>= 1
        i += 1
    return out
