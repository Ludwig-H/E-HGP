"""Modele canonique commun aux deux etages de la reference : des enregistrements, aucun calcul.

Un ordre k est decrit par un OrderResult. Les deux etages (definition.py et constructive.py) le remplissent chacun
par son propre code, numerotation et recherche d'ancetre comprises ; le juge (judge.py) relit la forme canonique par
un troisieme code et compare champ par champ. Ce fichier ne contient que les types, l'exception et deux conversions
entre masque de bits et liste d'indices.

Convention de numerotation des noeuds (intrinseque : ni rang de Morton, ni support) :
  - naissances d'abord, triees par (niveau, centre de la boule de naissance) ;
  - fusions ensuite, triees par (niveau, plus petite naissance du sous-arbre) ;
  - enfants d'une fusion tries par identifiant.
Deux naissances d'un meme ordre n'ont jamais la meme boule ; deux fusions de meme niveau portent des sous-arbres
disjoints : l'ordre est total. La numerotation des dumps de la v10 est une autre convention, deduite par dumps.py.

Ensembles de points : entiers utilises comme masques de bits, bit i = point d'indice i dans le nuage d'entree.
Niveaux : rayons carres, fractions.Fraction. Coupes : fermee (niveau <= a), ouverte (niveau < a).
"""
from collections import namedtuple

# Noeud : level (Fraction), children (identifiants tries ; vide pour une naissance), center (trois Fraction pour une
# naissance, None pour une fusion).
Node = namedtuple('Node', 'level children center')

# Coupe a un niveau d'evenement : opened et closed sont des tuples tries de (noeud vivant, couverture, coeur).
# Entre deux niveaux d'evenement rien ne change : les deux coupes y sont la coupe fermee du niveau precedent.
Cut = namedtuple('Cut', 'level opened closed')

# Entree d'un point : level (Fraction), nodes (identifiant pour core ; frozenset d'identifiants pour cover).
Entry = namedtuple('Entry', 'level nodes')


class InvariantError(Exception):
    """Invariant viole : une precondition interne ou un theoreme invoque ne tient pas (code de porte 3)."""


class OrderResult(object):
    """Resultat canonique d'un ordre k (enregistrement).

    nodes : tuple de Node, numerotation canonique.
    lower : tuple, image verticale de chaque noeud dans l'ordre k - 1 (noeud vivant a la coupe fermee du niveau
            du noeud) ; None a k = 1.
    core  : tuple par point d'Entry(D_k(x), noeud vivant a la coupe fermee D_k(x) dont la composante contient x).
    cover : tuple par point d'Entry(alpha_k(x)^2, frozenset des noeuds vivants a cette coupe fermee qui couvrent x).
    cuts  : tuple de Cut, un par niveau d'evenement de l'etage qui l'a produit, niveaux croissants.
    """

    def __init__(self, k, nodes, lower, core, cover, cuts):
        self.k = k
        self.nodes = tuple(nodes)
        self.lower = None if lower is None else tuple(lower)
        self.core = tuple(core)
        self.cover = tuple(cover)
        self.cuts = tuple(cuts)


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
